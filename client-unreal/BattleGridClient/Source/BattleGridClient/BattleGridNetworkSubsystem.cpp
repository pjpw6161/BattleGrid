// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridNetworkSubsystem.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "IWebSocket.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "WebSocketsModule.h"

void UBattleGridNetworkSubsystem::Deinitialize()
{
	Disconnect();

	Super::Deinitialize();
}

void UBattleGridNetworkSubsystem::Connect(const FString& InServerUrl, const FString& InNickname)
{
	if (Socket.IsValid())
	{
		Socket->Close();
		Socket.Reset();
	}

	ServerUrl = InServerUrl.IsEmpty() ? FString(TEXT("ws://127.0.0.1:7777")) : InServerUrl;
	Nickname = InNickname.IsEmpty() ? FString(TEXT("anonymous")) : InNickname;
	PlayerId = 0;
	RoomId = 0;
	bIsConnected = false;
	bHasJoined = false;
	LastServerMessage.Empty();
	LastError.Empty();
	LastDebugMessage.Empty();
	LastSnapshotTick = 0;
	LastSnapshotRoomId = 0;
	LatestPlayerSnapshots.Empty();
	LatestProjectileSnapshots.Empty();
	LatestTargetSnapshots.Empty();
	LatestBotSnapshots.Empty();
	LatestHealthPackSnapshots.Empty();
	LatestScoreboard.Empty();
	LatestMatchSnapshot = FBattleGridServerMatchSnapshot();
	RecentCombatEvents.Empty();
	SeenCombatEventIds.Empty();
	bHasMatchSnapshot = false;
	bHasLoggedServerSummary = false;
	InputAckLogCounter = 0;
	InputSendLogCounter = 0;
	SnapshotLogCounter = 0;

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Connecting to server: %s"), *ServerUrl);

	Socket = FWebSocketsModule::Get().CreateWebSocket(ServerUrl);
	if (!Socket.IsValid())
	{
		LastError = TEXT("failed to create websocket");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket connection failed: %s"), *LastError);
		return;
	}

	Socket->OnConnected().AddUObject(this, &UBattleGridNetworkSubsystem::HandleConnected);
	Socket->OnConnectionError().AddUObject(this, &UBattleGridNetworkSubsystem::HandleConnectionError);
	Socket->OnClosed().AddUObject(this, &UBattleGridNetworkSubsystem::HandleClosed);
	Socket->OnMessage().AddUObject(this, &UBattleGridNetworkSubsystem::HandleMessage);
	Socket->Connect();
}

void UBattleGridNetworkSubsystem::Disconnect()
{
	if (Socket.IsValid())
	{
		Socket->Close();
		Socket.Reset();
	}

	bIsConnected = false;
	bHasJoined = false;
	LastSnapshotTick = 0;
	LastSnapshotRoomId = 0;
	LatestPlayerSnapshots.Empty();
	LatestProjectileSnapshots.Empty();
	LatestTargetSnapshots.Empty();
	LatestBotSnapshots.Empty();
	LatestHealthPackSnapshots.Empty();
	LatestScoreboard.Empty();
	LatestMatchSnapshot = FBattleGridServerMatchSnapshot();
	RecentCombatEvents.Empty();
	SeenCombatEventIds.Empty();
	bHasMatchSnapshot = false;
	bHasLoggedServerSummary = false;
	InputAckLogCounter = 0;
	InputSendLogCounter = 0;
	SnapshotLogCounter = 0;
}

void UBattleGridNetworkSubsystem::SendPing()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("ping"));

	SendJsonObject(JsonObject, TEXT("Ping sent"));
}

void UBattleGridNetworkSubsystem::SendJoin()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("join"));
	JsonObject->SetStringField(TEXT("nickname"), Nickname);

	SendJsonObject(JsonObject, TEXT("Join sent"));
}

void UBattleGridNetworkSubsystem::SendDebugRestartMatch()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("debug_restart_match"));

	SendJsonObject(JsonObject, TEXT("Debug restart match sent"));
}

void UBattleGridNetworkSubsystem::SendDebugApplyDemoMode()
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("debug_apply_demo_mode"));

	SendJsonObject(JsonObject, TEXT("Apply safe demo mode sent"));
}

void UBattleGridNetworkSubsystem::SendDebugSetBotAttacks(bool bEnabled)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("debug_set_bot_attacks"));
	JsonObject->SetBoolField(TEXT("enabled"), bEnabled);

	SendJsonObject(JsonObject, bEnabled ? TEXT("Enable bot attacks sent") : TEXT("Disable bot attacks sent"));
}

void UBattleGridNetworkSubsystem::SendDebugSetBotDifficulty(const FString& Difficulty)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("debug_set_bot_difficulty"));
	JsonObject->SetStringField(TEXT("difficulty"), Difficulty.IsEmpty() ? FString(TEXT("normal")) : Difficulty);

	SendJsonObject(JsonObject, TEXT("Set bot difficulty sent"));
}

void UBattleGridNetworkSubsystem::SendDebugSetMatchTimer(bool bEnabled)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("debug_set_match_timer"));
	JsonObject->SetBoolField(TEXT("enabled"), bEnabled);

	SendJsonObject(JsonObject, bEnabled ? TEXT("Enable match timer sent") : TEXT("Disable match timer sent"));
}

void UBattleGridNetworkSubsystem::SendInput(
	int32 Seq,
	float MoveX,
	float MoveY,
	float AimX,
	float AimY,
	float ShotDirX,
	float ShotDirY,
	float ShotDirZ,
	bool bFire,
	bool bReload,
	bool bADS,
	bool bSprint,
	bool bJump,
	int32 Ammo,
	float SpreadDegrees
)
{
	if (!Socket.IsValid() || !Socket->IsConnected() || !bHasJoined)
	{
		return;
	}

	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("type"), TEXT("input"));
	JsonObject->SetNumberField(TEXT("seq"), Seq);
	JsonObject->SetNumberField(TEXT("player_id"), PlayerId);
	JsonObject->SetNumberField(TEXT("move_x"), MoveX);
	JsonObject->SetNumberField(TEXT("move_y"), MoveY);
	JsonObject->SetNumberField(TEXT("aim_x"), AimX);
	JsonObject->SetNumberField(TEXT("aim_y"), AimY);
	JsonObject->SetNumberField(TEXT("shot_dir_x"), ShotDirX);
	JsonObject->SetNumberField(TEXT("shot_dir_y"), ShotDirY);
	JsonObject->SetNumberField(TEXT("shot_dir_z"), ShotDirZ);
	JsonObject->SetBoolField(TEXT("fire"), bFire);
	JsonObject->SetBoolField(TEXT("reload"), bReload);
	JsonObject->SetBoolField(TEXT("ads"), bADS);
	JsonObject->SetBoolField(TEXT("sprint"), bSprint);
	JsonObject->SetBoolField(TEXT("jump"), bJump);
	JsonObject->SetNumberField(TEXT("ammo"), Ammo);
	JsonObject->SetNumberField(TEXT("spread_deg"), SpreadDegrees);

	if (SendJsonObject(JsonObject, nullptr))
	{
		++InputSendLogCounter;
		const int32 EffectiveInputLogInterval = FMath::Max(1, InputAckLogInterval);
		if (
			bVerboseInputLogs
			|| InputSendLogCounter <= 3
			|| InputSendLogCounter % EffectiveInputLogInterval == 0
		)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Sent input seq=%d fire=%s reload=%s ammo=%d spread=%.2f"),
				Seq,
				bFire ? TEXT("true") : TEXT("false"),
				bReload ? TEXT("true") : TEXT("false"),
				Ammo,
				SpreadDegrees
			);
		}
	}
}

void UBattleGridNetworkSubsystem::ConfigureDemoLogging(
	bool bInVerboseNetworkLogs,
	bool bInVerboseSnapshotLogs,
	bool bInVerboseInputLogs,
	int32 InSnapshotLogInterval,
	int32 InInputAckLogInterval
)
{
	bVerboseNetworkLogs = bInVerboseNetworkLogs;
	bVerboseSnapshotLogs = bInVerboseSnapshotLogs;
	bVerboseInputLogs = bInVerboseInputLogs;
	SnapshotLogInterval = FMath::Max(1, InSnapshotLogInterval);
	InputAckLogInterval = FMath::Max(1, InInputAckLogInterval);
}

bool UBattleGridNetworkSubsystem::IsConnected() const
{
	return bIsConnected;
}

bool UBattleGridNetworkSubsystem::HasJoined() const
{
	return bHasJoined;
}

int32 UBattleGridNetworkSubsystem::GetPlayerId() const
{
	return PlayerId;
}

int32 UBattleGridNetworkSubsystem::GetRoomId() const
{
	return RoomId;
}

FString UBattleGridNetworkSubsystem::GetServerUrl() const
{
	return ServerUrl;
}

FString UBattleGridNetworkSubsystem::GetNickname() const
{
	return Nickname;
}

FString UBattleGridNetworkSubsystem::GetLastServerMessage() const
{
	return LastServerMessage;
}

FString UBattleGridNetworkSubsystem::GetLastError() const
{
	return LastError;
}

FString UBattleGridNetworkSubsystem::GetLastDebugMessage() const
{
	return LastDebugMessage;
}

FString UBattleGridNetworkSubsystem::GetConnectionStatusText() const
{
	return GetServerSummaryText();
}

FString UBattleGridNetworkSubsystem::GetServerPrimaryStatusText() const
{
	if (!bIsConnected)
	{
		return TEXT("Server: Disconnected | Offline local test mode");
	}

	if (!bHasJoined)
	{
		return TEXT("Server: Connected | Joining...");
	}

	FBattleGridServerPlayerSnapshot OwnSnapshot;
	if (!GetOwnPlayerSnapshot(OwnSnapshot))
	{
		return FString::Printf(
			TEXT("SERVER | Waiting for own snapshot | Player=%d Room=%d Snapshot=%d"),
			PlayerId,
			RoomId,
			LastSnapshotTick
		);
	}

	const int32 GoalScore = bHasMatchSnapshot ? LatestMatchSnapshot.TargetScore : 20;
	return FString::Printf(
		TEXT("SERVER | HP %d/%d | Score %d/%d | K/D %d/%d | Bots %d | PvP %d"),
		OwnSnapshot.HP,
		OwnSnapshot.MaxHP,
		OwnSnapshot.Score,
		GoalScore,
		OwnSnapshot.Kills,
		OwnSnapshot.Deaths,
		OwnSnapshot.BotKills,
		OwnSnapshot.PlayerKills
	);
}

FString UBattleGridNetworkSubsystem::GetServerMatchStatusText() const
{
	if (!bIsConnected)
	{
		return TEXT("Match --:-- | Goal -- | Offline");
	}

	if (!bHasJoined || !bHasMatchSnapshot)
	{
		return TEXT("Match --:-- | Goal -- | Waiting");
	}

	if (LatestMatchSnapshot.bGameOver)
	{
		const FString WinnerText = LatestMatchSnapshot.WinnerNickname.IsEmpty()
			? FString::Printf(TEXT("P%d"), LatestMatchSnapshot.WinnerPlayerId)
			: LatestMatchSnapshot.WinnerNickname;
		return FString::Printf(TEXT("SERVER GAME OVER | Winner: %s"), *WinnerText);
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::RoundToInt(LatestMatchSnapshot.TimeLeft));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;
	return FString::Printf(
		TEXT("Match %02d:%02d | Goal %d | In Progress"),
		Minutes,
		Seconds,
		LatestMatchSnapshot.TargetScore
	);
}

FString UBattleGridNetworkSubsystem::GetServerOwnPlayerStatusText() const
{
	if (!bIsConnected)
	{
		return TEXT("Server Life: Offline");
	}

	if (!bHasJoined)
	{
		return TEXT("Server Life: Joining");
	}

	FBattleGridServerPlayerSnapshot OwnSnapshot;
	if (!GetOwnPlayerSnapshot(OwnSnapshot))
	{
		return TEXT("Server Life: Waiting");
	}

	if (!OwnSnapshot.bAlive)
	{
		return FString::Printf(TEXT("SERVER DEAD | Respawn %.1fs"), OwnSnapshot.RespawnTimer);
	}

	if (OwnSnapshot.bInvincible)
	{
		return FString::Printf(TEXT("SERVER INVINCIBLE %.1fs"), OwnSnapshot.InvincibleTimer);
	}

	return TEXT("Server Life: Alive");
}

FString UBattleGridNetworkSubsystem::GetServerWorldCountsText() const
{
	if (!bIsConnected || !bHasJoined)
	{
		return TEXT("Cores -/- | Bots -/- | HPacks -/- | Projectiles -");
	}

	return FString::Printf(
		TEXT("Cores %d/%d | Bots %d/%d | HPacks %d/%d | Projectiles %d"),
		GetServerAliveTargetCount(),
		GetServerTargetCount(),
		GetServerAliveBotCount(),
		GetServerBotCount(),
		GetServerActiveHealthPackCount(),
		GetServerHealthPackCount(),
		GetServerProjectileCount()
	);
}

FString UBattleGridNetworkSubsystem::GetServerScoreboardCompactText() const
{
	if (!bIsConnected)
	{
		return TEXT("Top: - | You: -");
	}

	TArray<FBattleGridServerScoreboardEntry> SortedScoreboard = LatestScoreboard;
	SortedScoreboard.Sort(
		[](const FBattleGridServerScoreboardEntry& Left, const FBattleGridServerScoreboardEntry& Right)
		{
			if (Left.Score != Right.Score)
			{
				return Left.Score > Right.Score;
			}
			if (Left.PlayerKills != Right.PlayerKills)
			{
				return Left.PlayerKills > Right.PlayerKills;
			}
			if (Left.BotKills != Right.BotKills)
			{
				return Left.BotKills > Right.BotKills;
			}
			if (Left.Deaths != Right.Deaths)
			{
				return Left.Deaths < Right.Deaths;
			}
			return Left.PlayerId < Right.PlayerId;
		}
	);

	TArray<FString> TopEntries;
	const int32 TopCount = FMath::Min(3, SortedScoreboard.Num());
	for (int32 Index = 0; Index < TopCount; ++Index)
	{
		const FBattleGridServerScoreboardEntry& Entry = SortedScoreboard[Index];
		const FString Name = Entry.Nickname.IsEmpty()
			? FString::Printf(TEXT("P%d"), Entry.PlayerId)
			: Entry.Nickname;
		TopEntries.Add(FString::Printf(TEXT("%s %d"), *Name, Entry.Score));
	}

	int32 OwnScore = 0;
	bool bHasOwnScore = false;
	for (const FBattleGridServerScoreboardEntry& Entry : LatestScoreboard)
	{
		if (Entry.PlayerId == PlayerId)
		{
			OwnScore = Entry.Score;
			bHasOwnScore = true;
			break;
		}
	}

	const FString TopText = TopEntries.Num() > 0
		? FString::Join(TopEntries, TEXT(", "))
		: FString(TEXT("-"));
	const FString OwnScoreText = bHasOwnScore
		? FString::FromInt(OwnScore)
		: FString(TEXT("-"));

	return FString::Printf(
		TEXT("Top: %s | You: %s"),
		*TopText,
		*OwnScoreText
	);
}

FString UBattleGridNetworkSubsystem::GetServerCombatEventFeedText() const
{
	TArray<FString> EventLines;
	for (int32 Index = RecentCombatEvents.Num() - 1; Index >= 0 && EventLines.Num() < 5; --Index)
	{
		if (!RecentCombatEvents[Index].Message.IsEmpty())
		{
			EventLines.Add(RecentCombatEvents[Index].Message);
		}
	}

	return FString::Join(EventLines, TEXT("\n"));
}

FString UBattleGridNetworkSubsystem::GetServerSummaryText() const
{
	TArray<FString> Lines;
	Lines.Add(GetServerPrimaryStatusText());

	if (bIsConnected && bHasJoined)
	{
		Lines.Add(GetServerMatchStatusText());
		Lines.Add(GetServerWorldCountsText());
		Lines.Add(GetServerScoreboardCompactText());
	}

	if (!LastError.IsEmpty() && !bIsConnected)
	{
		Lines.Add(FString::Printf(TEXT("Last Error: %s"), *LastError));
	}

	return FString::Join(Lines, TEXT("\n"));
}

bool UBattleGridNetworkSubsystem::HasSnapshot() const
{
	return LastSnapshotTick > 0;
}

int32 UBattleGridNetworkSubsystem::GetLastSnapshotTick() const
{
	return LastSnapshotTick;
}

int32 UBattleGridNetworkSubsystem::GetLastSnapshotRoomId() const
{
	return LastSnapshotRoomId;
}

void UBattleGridNetworkSubsystem::GetLatestPlayerSnapshots(
	TArray<FBattleGridServerPlayerSnapshot>& OutSnapshots
) const
{
	OutSnapshots.Reset();
	LatestPlayerSnapshots.GenerateValueArray(OutSnapshots);
}

bool UBattleGridNetworkSubsystem::GetPlayerSnapshotById(
	int32 InPlayerId,
	FBattleGridServerPlayerSnapshot& OutSnapshot
) const
{
	if (const FBattleGridServerPlayerSnapshot* PlayerSnapshot =
		LatestPlayerSnapshots.Find(InPlayerId))
	{
		OutSnapshot = *PlayerSnapshot;
		return true;
	}

	return false;
}

void UBattleGridNetworkSubsystem::GetLatestProjectileSnapshots(
	TArray<FBattleGridServerProjectileSnapshot>& OutProjectiles
) const
{
	OutProjectiles.Reset();
	LatestProjectileSnapshots.GenerateValueArray(OutProjectiles);
}

void UBattleGridNetworkSubsystem::GetLatestTargetSnapshots(
	TArray<FBattleGridServerTargetSnapshot>& OutTargets
) const
{
	OutTargets.Reset();
	LatestTargetSnapshots.GenerateValueArray(OutTargets);
}

void UBattleGridNetworkSubsystem::GetLatestBotSnapshots(
	TArray<FBattleGridServerBotSnapshot>& OutBots
) const
{
	OutBots.Reset();
	LatestBotSnapshots.GenerateValueArray(OutBots);
}

void UBattleGridNetworkSubsystem::GetLatestHealthPackSnapshots(
	TArray<FBattleGridServerHealthPackSnapshot>& OutHealthPacks
) const
{
	OutHealthPacks.Reset();
	LatestHealthPackSnapshots.GenerateValueArray(OutHealthPacks);
}

int32 UBattleGridNetworkSubsystem::GetServerProjectileCount() const
{
	return LatestProjectileSnapshots.Num();
}

int32 UBattleGridNetworkSubsystem::GetServerTargetCount() const
{
	return LatestTargetSnapshots.Num();
}

int32 UBattleGridNetworkSubsystem::GetServerAliveTargetCount() const
{
	int32 AliveTargetCount = 0;
	for (const auto& TargetEntry : LatestTargetSnapshots)
	{
		if (TargetEntry.Value.bAlive)
		{
			++AliveTargetCount;
		}
	}

	return AliveTargetCount;
}

int32 UBattleGridNetworkSubsystem::GetServerBotCount() const
{
	return LatestBotSnapshots.Num();
}

int32 UBattleGridNetworkSubsystem::GetServerAliveBotCount() const
{
	int32 AliveBotCount = 0;
	for (const auto& BotEntry : LatestBotSnapshots)
	{
		if (BotEntry.Value.bAlive)
		{
			++AliveBotCount;
		}
	}

	return AliveBotCount;
}

int32 UBattleGridNetworkSubsystem::GetServerHealthPackCount() const
{
	return LatestHealthPackSnapshots.Num();
}

int32 UBattleGridNetworkSubsystem::GetServerActiveHealthPackCount() const
{
	int32 ActiveHealthPackCount = 0;
	for (const auto& HealthPackEntry : LatestHealthPackSnapshots)
	{
		if (HealthPackEntry.Value.bActive)
		{
			++ActiveHealthPackCount;
		}
	}

	return ActiveHealthPackCount;
}

bool UBattleGridNetworkSubsystem::HasMatchSnapshot() const
{
	return bHasMatchSnapshot;
}

FBattleGridServerMatchSnapshot UBattleGridNetworkSubsystem::GetLatestMatchSnapshot() const
{
	return LatestMatchSnapshot;
}

void UBattleGridNetworkSubsystem::GetLatestScoreboard(
	TArray<FBattleGridServerScoreboardEntry>& OutScoreboard
) const
{
	OutScoreboard = LatestScoreboard;
}

FString UBattleGridNetworkSubsystem::GetMatchHeaderText() const
{
	if (!bHasMatchSnapshot)
	{
		return TEXT("Match: Waiting for server snapshot");
	}

	if (LatestMatchSnapshot.bGameOver)
	{
		const FString WinnerText = LatestMatchSnapshot.WinnerNickname.IsEmpty()
			? FString::Printf(TEXT("P%d"), LatestMatchSnapshot.WinnerPlayerId)
			: LatestMatchSnapshot.WinnerNickname;
		return FString::Printf(
			TEXT("SERVER GAME OVER | Winner: %s | Goal: %d"),
			*WinnerText,
			LatestMatchSnapshot.TargetScore
		);
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::RoundToInt(LatestMatchSnapshot.TimeLeft));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	return FString::Printf(
		TEXT("Match %02d:%02d | Goal %d | In Progress"),
		Minutes,
		Seconds,
		LatestMatchSnapshot.TargetScore
	);
}

FString UBattleGridNetworkSubsystem::GetScoreboardTableText() const
{
	TArray<FBattleGridServerScoreboardEntry> SortedScoreboard = LatestScoreboard;
	SortedScoreboard.Sort(
		[](const FBattleGridServerScoreboardEntry& Left, const FBattleGridServerScoreboardEntry& Right)
		{
			if (Left.Score != Right.Score)
			{
				return Left.Score > Right.Score;
			}
			if (Left.PlayerKills != Right.PlayerKills)
			{
				return Left.PlayerKills > Right.PlayerKills;
			}
			if (Left.BotKills != Right.BotKills)
			{
				return Left.BotKills > Right.BotKills;
			}
			if (Left.Deaths != Right.Deaths)
			{
				return Left.Deaths < Right.Deaths;
			}
			return Left.PlayerId < Right.PlayerId;
		}
	);

	TArray<FString> Lines;
	Lines.Add(TEXT("Rank | Player | Score | K | D | Bot | PvP | Target"));

	for (int32 Index = 0; Index < SortedScoreboard.Num(); ++Index)
	{
		const FBattleGridServerScoreboardEntry& Entry = SortedScoreboard[Index];
		const FString Name = Entry.Nickname.IsEmpty()
			? FString::Printf(TEXT("P%d"), Entry.PlayerId)
			: Entry.Nickname;

		Lines.Add(FString::Printf(
			TEXT("%d | %s | %d | %d | %d | %d | %d | %d"),
			Index + 1,
			*Name,
			Entry.Score,
			Entry.Kills,
			Entry.Deaths,
			Entry.BotKills,
			Entry.PlayerKills,
			Entry.TargetKills
		));
	}

	if (SortedScoreboard.Num() == 0)
	{
		Lines.Add(TEXT("- | No server scoreboard yet | - | - | - | - | - | -"));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString UBattleGridNetworkSubsystem::GetFullScoreboardText() const
{
	return FString::Printf(
		TEXT("%s\n%s\nTab: Scoreboard"),
		*GetMatchHeaderText(),
		*GetScoreboardTableText()
	);
}

void UBattleGridNetworkSubsystem::GetRecentCombatEvents(
	TArray<FBattleGridServerCombatEvent>& OutEvents
) const
{
	OutEvents = RecentCombatEvents;
}

FString UBattleGridNetworkSubsystem::GetCombatEventFeedText() const
{
	TArray<FString> EventLines;
	for (int32 Index = RecentCombatEvents.Num() - 1; Index >= 0; --Index)
	{
		if (!RecentCombatEvents[Index].Message.IsEmpty())
		{
			EventLines.Add(RecentCombatEvents[Index].Message);
		}
	}

	return FString::Join(EventLines, TEXT("\n"));
}

FString UBattleGridNetworkSubsystem::GetServerScoreboardSummaryText() const
{
	if (!bHasMatchSnapshot)
	{
		return TEXT("Match: -");
	}

	if (LatestMatchSnapshot.bGameOver)
	{
		const FString WinnerText = LatestMatchSnapshot.WinnerNickname.IsEmpty()
			? FString::Printf(TEXT("P%d"), LatestMatchSnapshot.WinnerPlayerId)
			: LatestMatchSnapshot.WinnerNickname;
		return FString::Printf(TEXT("SERVER GAME OVER | Winner: %s | Press Restart"), *WinnerText);
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::RoundToInt(LatestMatchSnapshot.TimeLeft));
	const int32 Minutes = TotalSeconds / 60;
	const int32 Seconds = TotalSeconds % 60;

	TArray<FString> TopEntries;
	const int32 TopCount = FMath::Min(3, LatestScoreboard.Num());
	for (int32 Index = 0; Index < TopCount; ++Index)
	{
		const FBattleGridServerScoreboardEntry& Entry = LatestScoreboard[Index];
		const FString Name = Entry.Nickname.IsEmpty()
			? FString::Printf(TEXT("P%d"), Entry.PlayerId)
			: Entry.Nickname;
		TopEntries.Add(FString::Printf(TEXT("%s %d"), *Name, Entry.Score));
	}

	int32 OwnScore = 0;
	for (const FBattleGridServerScoreboardEntry& Entry : LatestScoreboard)
	{
		if (Entry.PlayerId == PlayerId)
		{
			OwnScore = Entry.Score;
			break;
		}
	}

	return FString::Printf(
		TEXT("Match %02d:%02d | Goal %d | Top: %s | You: %d"),
		Minutes,
		Seconds,
		LatestMatchSnapshot.TargetScore,
		TopEntries.Num() > 0 ? *FString::Join(TopEntries, TEXT(", ")) : TEXT("-"),
		OwnScore
	);
}

bool UBattleGridNetworkSubsystem::GetOwnPlayerSnapshot(
	FBattleGridServerPlayerSnapshot& OutSnapshot
) const
{
	return PlayerId > 0 && GetPlayerSnapshotById(PlayerId, OutSnapshot);
}

int32 UBattleGridNetworkSubsystem::GetOwnServerScore() const
{
	FBattleGridServerPlayerSnapshot OwnSnapshot;
	return GetOwnPlayerSnapshot(OwnSnapshot) ? OwnSnapshot.Score : 0;
}

int32 UBattleGridNetworkSubsystem::GetOwnServerHP() const
{
	FBattleGridServerPlayerSnapshot OwnSnapshot;
	return GetOwnPlayerSnapshot(OwnSnapshot) ? OwnSnapshot.HP : 0;
}

void UBattleGridNetworkSubsystem::HandleConnected()
{
	bIsConnected = true;
	bHasJoined = false;
	LastError.Empty();

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] WebSocket connected."));

	SendPing();
	SendJoin();
}

void UBattleGridNetworkSubsystem::HandleConnectionError(const FString& Error)
{
	bIsConnected = false;
	bHasJoined = false;
	LastError = Error.IsEmpty() ? FString(TEXT("connection failed")) : Error;

	UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket connection failed: %s"), *LastError);
}

void UBattleGridNetworkSubsystem::HandleClosed(
	int32 StatusCode,
	const FString& Reason,
	bool bWasClean
)
{
	bIsConnected = false;
	bHasJoined = false;
	LastSnapshotTick = 0;
	LastSnapshotRoomId = 0;
	LatestPlayerSnapshots.Empty();
	LatestProjectileSnapshots.Empty();
	LatestTargetSnapshots.Empty();
	LatestBotSnapshots.Empty();
	LatestHealthPackSnapshots.Empty();
	LatestScoreboard.Empty();
	LatestMatchSnapshot = FBattleGridServerMatchSnapshot();
	RecentCombatEvents.Empty();
	SeenCombatEventIds.Empty();
	LastDebugMessage.Empty();
	bHasMatchSnapshot = false;
	bHasLoggedServerSummary = false;
	InputAckLogCounter = 0;
	InputSendLogCounter = 0;
	SnapshotLogCounter = 0;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] WebSocket closed. Status=%d Clean=%s Reason=%s"),
		StatusCode,
		bWasClean ? TEXT("true") : TEXT("false"),
		*Reason
	);
}

void UBattleGridNetworkSubsystem::HandleMessage(const FString& Message)
{
	LastServerMessage = Message;

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		LastError = TEXT("invalid json from server");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Failed to parse server JSON."));
		return;
	}

	FString Type;
	if (!JsonObject->TryGetStringField(TEXT("type"), Type))
	{
		LastError = TEXT("server message missing type");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Server message missing type."));
		return;
	}

	if (bVerboseNetworkLogs && Type != TEXT("snapshot"))
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] WebSocket received raw message: %s"), *Message);
	}

	if (Type == TEXT("pong"))
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Pong received."));
		return;
	}

	if (Type == TEXT("join_ok"))
	{
		double PlayerIdValue = 0.0;
		double RoomIdValue = 0.0;
		FString ResponseNickname;

		if (JsonObject->TryGetNumberField(TEXT("player_id"), PlayerIdValue))
		{
			PlayerId = static_cast<int32>(PlayerIdValue);
		}

		if (JsonObject->TryGetNumberField(TEXT("room_id"), RoomIdValue))
		{
			RoomId = static_cast<int32>(RoomIdValue);
		}

		if (JsonObject->TryGetStringField(TEXT("nickname"), ResponseNickname) && !ResponseNickname.IsEmpty())
		{
			Nickname = ResponseNickname;
		}

		bHasJoined = true;

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] join_ok parsed. player_id=%d room_id=%d nickname=%s"),
			PlayerId,
			RoomId,
			*Nickname
		);
		return;
	}

	if (Type == TEXT("match_restarted"))
	{
		double MatchIdValue = 0.0;
		JsonObject->TryGetNumberField(TEXT("match_id"), MatchIdValue);
		LastDebugMessage = FString::Printf(
			TEXT("match restarted id=%d"),
			static_cast<int32>(MatchIdValue)
		);
		LatestMatchSnapshot = FBattleGridServerMatchSnapshot();
		LatestMatchSnapshot.MatchId = static_cast<int32>(MatchIdValue);
		LatestScoreboard.Empty();
		RecentCombatEvents.Empty();
		SeenCombatEventIds.Empty();
		bHasMatchSnapshot = false;

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Match restarted. match_id=%d"),
			static_cast<int32>(MatchIdValue)
		);
		return;
	}

	if (Type == TEXT("debug_ok"))
	{
		FString DebugMessage;
		JsonObject->TryGetStringField(TEXT("message"), DebugMessage);
		LastDebugMessage = DebugMessage.IsEmpty() ? FString(TEXT("debug ok")) : DebugMessage;
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Server debug response: %s"), *LastDebugMessage);
		return;
	}

	if (Type == TEXT("error"))
	{
		FString ErrorMessage;
		JsonObject->TryGetStringField(TEXT("message"), ErrorMessage);
		LastError = ErrorMessage.IsEmpty() ? FString(TEXT("server error")) : ErrorMessage;
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Server error response parsed: %s"), *LastError);
		return;
	}

	if (Type == TEXT("input_ack"))
	{
		double SequenceValue = 0.0;
		JsonObject->TryGetNumberField(TEXT("seq"), SequenceValue);
		++InputAckLogCounter;
		const int32 EffectiveInputLogInterval = FMath::Max(1, InputAckLogInterval);
		if (
			bVerboseInputLogs
			|| InputAckLogCounter <= 3
			|| InputAckLogCounter % EffectiveInputLogInterval == 0
		)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Input ack received. seq=%d"),
				static_cast<int32>(SequenceValue)
			);
		}
		return;
	}

	if (Type == TEXT("snapshot"))
	{
		HandleSnapshotMessage(JsonObject);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Unknown server message type: %s"), *Type);
}

void UBattleGridNetworkSubsystem::HandleSnapshotMessage(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!JsonObject.IsValid())
	{
		return;
	}

	double SnapshotTickValue = 0.0;
	double SnapshotRoomIdValue = 0.0;
	JsonObject->TryGetNumberField(TEXT("tick"), SnapshotTickValue);
	JsonObject->TryGetNumberField(TEXT("room_id"), SnapshotRoomIdValue);

	const int32 SnapshotTick = static_cast<int32>(SnapshotTickValue);
	const int32 SnapshotRoomId = static_cast<int32>(SnapshotRoomIdValue);

	LatestMatchSnapshot = FBattleGridServerMatchSnapshot();
	bHasMatchSnapshot = false;
	const TSharedPtr<FJsonObject>* MatchObjectPtr = nullptr;
	if (
		JsonObject->TryGetObjectField(TEXT("match"), MatchObjectPtr)
		&& MatchObjectPtr
		&& MatchObjectPtr->IsValid()
	)
	{
		const TSharedPtr<FJsonObject>& MatchObject = *MatchObjectPtr;
		double TimeLeftValue = 0.0;
		double DurationValue = 0.0;
		double TargetScoreValue = 0.0;
		double WinnerPlayerIdValue = 0.0;
		double MatchIdValue = 0.0;
		bool bGameOverValue = false;

		MatchObject->TryGetStringField(TEXT("state"), LatestMatchSnapshot.State);
		MatchObject->TryGetNumberField(TEXT("time_left"), TimeLeftValue);
		MatchObject->TryGetNumberField(TEXT("duration"), DurationValue);
		MatchObject->TryGetNumberField(TEXT("target_score"), TargetScoreValue);
		MatchObject->TryGetBoolField(TEXT("game_over"), bGameOverValue);
		MatchObject->TryGetNumberField(TEXT("winner_player_id"), WinnerPlayerIdValue);
		MatchObject->TryGetStringField(TEXT("winner_nickname"), LatestMatchSnapshot.WinnerNickname);
		MatchObject->TryGetNumberField(TEXT("match_id"), MatchIdValue);

		LatestMatchSnapshot.TimeLeft = static_cast<float>(TimeLeftValue);
		LatestMatchSnapshot.Duration = static_cast<float>(DurationValue);
		LatestMatchSnapshot.TargetScore = static_cast<int32>(TargetScoreValue);
		LatestMatchSnapshot.bGameOver = bGameOverValue;
		LatestMatchSnapshot.WinnerPlayerId = static_cast<int32>(WinnerPlayerIdValue);
		LatestMatchSnapshot.MatchId = static_cast<int32>(MatchIdValue);
		bHasMatchSnapshot = true;
	}

	LatestScoreboard.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ScoreboardArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("scoreboard"), ScoreboardArray))
	{
		for (const TSharedPtr<FJsonValue>& ScoreboardValue : *ScoreboardArray)
		{
			const TSharedPtr<FJsonObject> ScoreboardObject =
				ScoreboardValue.IsValid() ? ScoreboardValue->AsObject() : nullptr;
			if (!ScoreboardObject.IsValid())
			{
				continue;
			}

			FBattleGridServerScoreboardEntry Entry;
			double PlayerIdValue = 0.0;
			double ScoreValue = 0.0;
			double KillsValue = 0.0;
			double DeathsValue = 0.0;
			double BotKillsValue = 0.0;
			double PlayerKillsValue = 0.0;
			double TargetKillsValue = 0.0;
			double HPValue = 0.0;
			bool bAliveValue = false;

			ScoreboardObject->TryGetNumberField(TEXT("player_id"), PlayerIdValue);
			ScoreboardObject->TryGetStringField(TEXT("nickname"), Entry.Nickname);
			ScoreboardObject->TryGetNumberField(TEXT("score"), ScoreValue);
			ScoreboardObject->TryGetNumberField(TEXT("kills"), KillsValue);
			ScoreboardObject->TryGetNumberField(TEXT("deaths"), DeathsValue);
			ScoreboardObject->TryGetNumberField(TEXT("bot_kills"), BotKillsValue);
			ScoreboardObject->TryGetNumberField(TEXT("player_kills"), PlayerKillsValue);
			ScoreboardObject->TryGetNumberField(TEXT("target_kills"), TargetKillsValue);
			ScoreboardObject->TryGetNumberField(TEXT("hp"), HPValue);
			ScoreboardObject->TryGetBoolField(TEXT("alive"), bAliveValue);

			Entry.PlayerId = static_cast<int32>(PlayerIdValue);
			Entry.Score = static_cast<int32>(ScoreValue);
			Entry.Kills = static_cast<int32>(KillsValue);
			Entry.Deaths = static_cast<int32>(DeathsValue);
			Entry.BotKills = static_cast<int32>(BotKillsValue);
			Entry.PlayerKills = static_cast<int32>(PlayerKillsValue);
			Entry.TargetKills = static_cast<int32>(TargetKillsValue);
			Entry.HP = static_cast<int32>(HPValue);
			Entry.bAlive = bAliveValue;

			if (Entry.PlayerId > 0)
			{
				LatestScoreboard.Add(Entry);
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* EventsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("events"), EventsArray))
	{
		for (const TSharedPtr<FJsonValue>& EventValue : *EventsArray)
		{
			const TSharedPtr<FJsonObject> EventObject =
				EventValue.IsValid() ? EventValue->AsObject() : nullptr;
			if (!EventObject.IsValid())
			{
				continue;
			}

			FBattleGridServerCombatEvent Event;
			double EventIdValue = 0.0;
			double TimeValue = 0.0;
			double ActorPlayerIdValue = 0.0;
			double TargetPlayerIdValue = 0.0;
			double BotIdValue = 0.0;
			double TargetIdValue = 0.0;
			double HealthPackIdValue = 0.0;
			bool bHeadshotValue = false;

			EventObject->TryGetNumberField(TEXT("event_id"), EventIdValue);
			EventObject->TryGetStringField(TEXT("type"), Event.Type);
			EventObject->TryGetStringField(TEXT("message"), Event.Message);
			EventObject->TryGetNumberField(TEXT("time"), TimeValue);
			EventObject->TryGetNumberField(TEXT("actor_player_id"), ActorPlayerIdValue);
			EventObject->TryGetNumberField(TEXT("target_player_id"), TargetPlayerIdValue);
			EventObject->TryGetNumberField(TEXT("bot_id"), BotIdValue);
			EventObject->TryGetNumberField(TEXT("target_id"), TargetIdValue);
			EventObject->TryGetNumberField(TEXT("health_pack_id"), HealthPackIdValue);
			EventObject->TryGetBoolField(TEXT("headshot"), bHeadshotValue);

			Event.EventId = static_cast<int32>(EventIdValue);
			Event.Time = static_cast<float>(TimeValue);
			Event.ActorPlayerId = static_cast<int32>(ActorPlayerIdValue);
			Event.TargetPlayerId = static_cast<int32>(TargetPlayerIdValue);
			Event.BotId = static_cast<int32>(BotIdValue);
			Event.TargetId = static_cast<int32>(TargetIdValue);
			Event.HealthPackId = static_cast<int32>(HealthPackIdValue);
			Event.bHeadshot = bHeadshotValue;

			if (Event.EventId <= 0 || SeenCombatEventIds.Contains(Event.EventId))
			{
				continue;
			}

			SeenCombatEventIds.Add(Event.EventId);
			RecentCombatEvents.Add(Event);
			while (RecentCombatEvents.Num() > MaxRecentCombatEvents)
			{
				RecentCombatEvents.RemoveAt(0);
			}

			UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Combat event: %s"), *Event.Message);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* PlayersArray = nullptr;
	if (!JsonObject->TryGetArrayField(TEXT("players"), PlayersArray))
	{
		return;
	}

	LatestPlayerSnapshots.Empty();

	for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
	{
		const TSharedPtr<FJsonObject> PlayerObject =
			PlayerValue.IsValid() ? PlayerValue->AsObject() : nullptr;
		if (!PlayerObject.IsValid())
		{
			continue;
		}

		FBattleGridServerPlayerSnapshot PlayerSnapshot;
		double PlayerIdValue = 0.0;
		double XValue = 0.0;
		double YValue = 0.0;
		double ZValue = 0.0;
		double HPValue = 0.0;
		double MaxHPValue = 0.0;
		double RespawnTimerValue = 0.0;
		double InvincibleTimerValue = 0.0;
		double ScoreValue = 0.0;
		double KillsValue = 0.0;
		double DeathsValue = 0.0;
		double PlayerKillsValue = 0.0;
		double TargetKillsValue = 0.0;
		double BotKillsValue = 0.0;
		double LastSeqValue = 0.0;
		bool bAliveValue = true;
		bool bInvincibleValue = false;

		PlayerObject->TryGetNumberField(TEXT("player_id"), PlayerIdValue);
		PlayerObject->TryGetStringField(TEXT("nickname"), PlayerSnapshot.Nickname);
		PlayerObject->TryGetNumberField(TEXT("x"), XValue);
		PlayerObject->TryGetNumberField(TEXT("y"), YValue);
		PlayerObject->TryGetNumberField(TEXT("z"), ZValue);
		PlayerObject->TryGetNumberField(TEXT("hp"), HPValue);
		PlayerObject->TryGetNumberField(TEXT("max_hp"), MaxHPValue);
		PlayerObject->TryGetBoolField(TEXT("alive"), bAliveValue);
		PlayerObject->TryGetBoolField(TEXT("invincible"), bInvincibleValue);
		PlayerObject->TryGetNumberField(TEXT("respawn_timer"), RespawnTimerValue);
		PlayerObject->TryGetNumberField(TEXT("invincible_timer"), InvincibleTimerValue);
		PlayerObject->TryGetNumberField(TEXT("score"), ScoreValue);
		PlayerObject->TryGetNumberField(TEXT("kills"), KillsValue);
		PlayerObject->TryGetNumberField(TEXT("deaths"), DeathsValue);
		PlayerObject->TryGetNumberField(TEXT("player_kills"), PlayerKillsValue);
		PlayerObject->TryGetNumberField(TEXT("target_kills"), TargetKillsValue);
		PlayerObject->TryGetNumberField(TEXT("bot_kills"), BotKillsValue);
		PlayerObject->TryGetNumberField(TEXT("last_seq"), LastSeqValue);

		PlayerSnapshot.PlayerId = static_cast<int32>(PlayerIdValue);
		PlayerSnapshot.X = static_cast<float>(XValue);
		PlayerSnapshot.Y = static_cast<float>(YValue);
		PlayerSnapshot.Z = static_cast<float>(ZValue);
		PlayerSnapshot.HP = static_cast<int32>(HPValue);
		PlayerSnapshot.MaxHP = static_cast<int32>(MaxHPValue);
		PlayerSnapshot.bAlive = bAliveValue;
		PlayerSnapshot.bInvincible = bInvincibleValue;
		PlayerSnapshot.RespawnTimer = static_cast<float>(RespawnTimerValue);
		PlayerSnapshot.InvincibleTimer = static_cast<float>(InvincibleTimerValue);
		PlayerSnapshot.Score = static_cast<int32>(ScoreValue);
		PlayerSnapshot.Kills = static_cast<int32>(KillsValue);
		PlayerSnapshot.Deaths = static_cast<int32>(DeathsValue);
		PlayerSnapshot.PlayerKills = static_cast<int32>(PlayerKillsValue);
		PlayerSnapshot.TargetKills = static_cast<int32>(TargetKillsValue);
		PlayerSnapshot.BotKills = static_cast<int32>(BotKillsValue);
		PlayerSnapshot.LastSeq = static_cast<int32>(LastSeqValue);

		if (PlayerSnapshot.PlayerId > 0)
		{
			LatestPlayerSnapshots.Add(PlayerSnapshot.PlayerId, PlayerSnapshot);
		}
	}

	LatestProjectileSnapshots.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ProjectilesArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("projectiles"), ProjectilesArray))
	{
		for (const TSharedPtr<FJsonValue>& ProjectileValue : *ProjectilesArray)
		{
			const TSharedPtr<FJsonObject> ProjectileObject =
				ProjectileValue.IsValid() ? ProjectileValue->AsObject() : nullptr;
			if (!ProjectileObject.IsValid())
			{
				continue;
			}

			FBattleGridServerProjectileSnapshot ProjectileSnapshot;
			double ProjectileIdValue = 0.0;
			double OwnerPlayerIdValue = 0.0;
			double XValue = 0.0;
			double YValue = 0.0;
			double DirXValue = 1.0;
			double DirYValue = 0.0;

			ProjectileObject->TryGetNumberField(TEXT("projectile_id"), ProjectileIdValue);
			ProjectileObject->TryGetNumberField(TEXT("owner_player_id"), OwnerPlayerIdValue);
			ProjectileObject->TryGetNumberField(TEXT("x"), XValue);
			ProjectileObject->TryGetNumberField(TEXT("y"), YValue);
			ProjectileObject->TryGetNumberField(TEXT("dir_x"), DirXValue);
			ProjectileObject->TryGetNumberField(TEXT("dir_y"), DirYValue);

			ProjectileSnapshot.ProjectileId = static_cast<int32>(ProjectileIdValue);
			ProjectileSnapshot.OwnerPlayerId = static_cast<int32>(OwnerPlayerIdValue);
			ProjectileSnapshot.X = static_cast<float>(XValue);
			ProjectileSnapshot.Y = static_cast<float>(YValue);
			ProjectileSnapshot.DirX = static_cast<float>(DirXValue);
			ProjectileSnapshot.DirY = static_cast<float>(DirYValue);

			if (ProjectileSnapshot.ProjectileId > 0)
			{
				LatestProjectileSnapshots.Add(
					ProjectileSnapshot.ProjectileId,
					ProjectileSnapshot
				);
			}
		}
	}

	LatestTargetSnapshots.Empty();
	const TArray<TSharedPtr<FJsonValue>>* TargetsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("targets"), TargetsArray))
	{
		for (const TSharedPtr<FJsonValue>& TargetValue : *TargetsArray)
		{
			const TSharedPtr<FJsonObject> TargetObject =
				TargetValue.IsValid() ? TargetValue->AsObject() : nullptr;
			if (!TargetObject.IsValid())
			{
				continue;
			}

			FBattleGridServerTargetSnapshot TargetSnapshot;
			double TargetIdValue = 0.0;
			double XValue = 0.0;
			double YValue = 0.0;
			double HPValue = 0.0;
			double MaxHPValue = 0.0;
			bool bAliveValue = false;

			TargetObject->TryGetNumberField(TEXT("target_id"), TargetIdValue);
			TargetObject->TryGetNumberField(TEXT("x"), XValue);
			TargetObject->TryGetNumberField(TEXT("y"), YValue);
			TargetObject->TryGetNumberField(TEXT("hp"), HPValue);
			TargetObject->TryGetNumberField(TEXT("max_hp"), MaxHPValue);
			TargetObject->TryGetBoolField(TEXT("alive"), bAliveValue);

			TargetSnapshot.TargetId = static_cast<int32>(TargetIdValue);
			TargetSnapshot.X = static_cast<float>(XValue);
			TargetSnapshot.Y = static_cast<float>(YValue);
			TargetSnapshot.HP = static_cast<int32>(HPValue);
			TargetSnapshot.MaxHP = static_cast<int32>(MaxHPValue);
			TargetSnapshot.bAlive = bAliveValue;

			if (TargetSnapshot.TargetId > 0)
			{
				LatestTargetSnapshots.Add(TargetSnapshot.TargetId, TargetSnapshot);
			}
		}
	}

	LatestBotSnapshots.Empty();
	const TArray<TSharedPtr<FJsonValue>>* BotsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("bots"), BotsArray))
	{
		for (const TSharedPtr<FJsonValue>& BotValue : *BotsArray)
		{
			const TSharedPtr<FJsonObject> BotObject =
				BotValue.IsValid() ? BotValue->AsObject() : nullptr;
			if (!BotObject.IsValid())
			{
				continue;
			}

			FBattleGridServerBotSnapshot BotSnapshot;
			double BotIdValue = 0.0;
			double XValue = 0.0;
			double YValue = 0.0;
			double ZValue = 0.0;
			double YawValue = 0.0;
			double HPValue = 0.0;
			double MaxHPValue = 0.0;
			double TargetPlayerIdValue = 0.0;
			bool bAliveValue = false;
			bool bInvincibleValue = false;

			BotObject->TryGetNumberField(TEXT("bot_id"), BotIdValue);
			BotObject->TryGetStringField(TEXT("name"), BotSnapshot.Name);
			BotObject->TryGetNumberField(TEXT("x"), XValue);
			BotObject->TryGetNumberField(TEXT("y"), YValue);
			BotObject->TryGetNumberField(TEXT("z"), ZValue);
			BotObject->TryGetNumberField(TEXT("yaw"), YawValue);
			BotObject->TryGetNumberField(TEXT("hp"), HPValue);
			BotObject->TryGetNumberField(TEXT("max_hp"), MaxHPValue);
			BotObject->TryGetBoolField(TEXT("alive"), bAliveValue);
			BotObject->TryGetBoolField(TEXT("invincible"), bInvincibleValue);
			BotObject->TryGetNumberField(TEXT("target_player_id"), TargetPlayerIdValue);

			BotSnapshot.BotId = static_cast<int32>(BotIdValue);
			BotSnapshot.X = static_cast<float>(XValue);
			BotSnapshot.Y = static_cast<float>(YValue);
			BotSnapshot.Z = static_cast<float>(ZValue);
			BotSnapshot.Yaw = static_cast<float>(YawValue);
			BotSnapshot.HP = static_cast<int32>(HPValue);
			BotSnapshot.MaxHP = static_cast<int32>(MaxHPValue);
			BotSnapshot.bAlive = bAliveValue;
			BotSnapshot.bInvincible = bInvincibleValue;
			BotSnapshot.TargetPlayerId = static_cast<int32>(TargetPlayerIdValue);

			if (BotSnapshot.BotId > 0)
			{
				LatestBotSnapshots.Add(BotSnapshot.BotId, BotSnapshot);
			}
		}
	}

	LatestHealthPackSnapshots.Empty();
	const TArray<TSharedPtr<FJsonValue>>* HealthPacksArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("health_packs"), HealthPacksArray))
	{
		for (const TSharedPtr<FJsonValue>& HealthPackValue : *HealthPacksArray)
		{
			const TSharedPtr<FJsonObject> HealthPackObject =
				HealthPackValue.IsValid() ? HealthPackValue->AsObject() : nullptr;
			if (!HealthPackObject.IsValid())
			{
				continue;
			}

			FBattleGridServerHealthPackSnapshot HealthPackSnapshot;
			double HealthPackIdValue = 0.0;
			double XValue = 0.0;
			double YValue = 0.0;
			double ZValue = 0.0;
			double HealAmountValue = 0.0;
			double RespawnTimerValue = 0.0;
			bool bActiveValue = false;

			HealthPackObject->TryGetNumberField(TEXT("health_pack_id"), HealthPackIdValue);
			HealthPackObject->TryGetNumberField(TEXT("x"), XValue);
			HealthPackObject->TryGetNumberField(TEXT("y"), YValue);
			HealthPackObject->TryGetNumberField(TEXT("z"), ZValue);
			HealthPackObject->TryGetBoolField(TEXT("active"), bActiveValue);
			HealthPackObject->TryGetNumberField(TEXT("heal_amount"), HealAmountValue);
			HealthPackObject->TryGetNumberField(TEXT("respawn_timer"), RespawnTimerValue);

			HealthPackSnapshot.HealthPackId = static_cast<int32>(HealthPackIdValue);
			HealthPackSnapshot.X = static_cast<float>(XValue);
			HealthPackSnapshot.Y = static_cast<float>(YValue);
			HealthPackSnapshot.Z = static_cast<float>(ZValue);
			HealthPackSnapshot.bActive = bActiveValue;
			HealthPackSnapshot.HealAmount = static_cast<int32>(HealAmountValue);
			HealthPackSnapshot.RespawnTimer = static_cast<float>(RespawnTimerValue);

			if (HealthPackSnapshot.HealthPackId > 0)
			{
				LatestHealthPackSnapshots.Add(
					HealthPackSnapshot.HealthPackId,
					HealthPackSnapshot
				);
			}
		}
	}

	const bool bFirstSnapshot = LastSnapshotTick <= 0;
	LastSnapshotTick = SnapshotTick;
	LastSnapshotRoomId = SnapshotRoomId;
	++SnapshotLogCounter;

	const int32 EffectiveSnapshotLogInterval = FMath::Max(1, SnapshotLogInterval);
	if (
		bVerboseSnapshotLogs
		|| bFirstSnapshot
		|| SnapshotLogCounter <= 3
		|| SnapshotLogCounter % EffectiveSnapshotLogInterval == 0
	)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Snapshot received. tick=%d players=%d projectiles=%d targets=%d bots=%d health_packs=%d match=%s time_left=%.1f scoreboard=%d"),
			LastSnapshotTick,
			LatestPlayerSnapshots.Num(),
			LatestProjectileSnapshots.Num(),
			LatestTargetSnapshots.Num(),
			LatestBotSnapshots.Num(),
			LatestHealthPackSnapshots.Num(),
			*LatestMatchSnapshot.State,
			LatestMatchSnapshot.TimeLeft,
			LatestScoreboard.Num()
		);
	}

	if (bHasJoined && !bHasLoggedServerSummary)
	{
		bHasLoggedServerSummary = true;
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Server summary available: %s"), *GetServerSummaryText());
	}
}

bool UBattleGridNetworkSubsystem::SendJsonObject(
	const TSharedRef<FJsonObject>& JsonObject,
	const TCHAR* LogLabel
)
{
	if (!Socket.IsValid() || !Socket->IsConnected())
	{
		LastError = TEXT("not connected");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket send failed: %s"), *LastError);
		return false;
	}

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	if (!FJsonSerializer::Serialize(JsonObject, Writer))
	{
		LastError = TEXT("failed to serialize json");
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] WebSocket send failed: %s"), *LastError);
		return false;
	}

	Socket->Send(Payload);
	if (LogLabel)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] %s: %s"), LogLabel, *Payload);
	}

	return true;
}
