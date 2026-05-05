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
	LastSnapshotTick = 0;
	LastSnapshotRoomId = 0;
	LatestPlayerSnapshots.Empty();
	LatestProjectileSnapshots.Empty();
	LatestTargetSnapshots.Empty();
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

void UBattleGridNetworkSubsystem::SendInput(
	int32 Seq,
	float MoveX,
	float MoveY,
	float AimX,
	float AimY,
	bool bFire
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
	JsonObject->SetBoolField(TEXT("fire"), bFire);

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
			UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Sent input seq=%d"), Seq);
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

FString UBattleGridNetworkSubsystem::GetConnectionStatusText() const
{
	return GetServerSummaryText();
}

FString UBattleGridNetworkSubsystem::GetServerSummaryText() const
{
	if (bHasJoined)
	{
		return FString::Printf(
			TEXT("Server: Connected | Player=%d Room=%d Snapshot=%d | ServerScore=%d | Targets=%d/%d | Projectiles=%d"),
			PlayerId,
			RoomId,
			LastSnapshotTick,
			GetOwnServerScore(),
			GetServerAliveTargetCount(),
			GetServerTargetCount(),
			GetServerProjectileCount()
		);
	}

	if (bIsConnected)
	{
		return TEXT("Server: Connected | Joining...");
	}

	if (!LastError.IsEmpty())
	{
		return FString::Printf(TEXT("Server: Error | %s"), *LastError);
	}

	return TEXT("Server: Disconnected");
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
		double HPValue = 0.0;
		double ScoreValue = 0.0;
		double LastSeqValue = 0.0;

		PlayerObject->TryGetNumberField(TEXT("player_id"), PlayerIdValue);
		PlayerObject->TryGetStringField(TEXT("nickname"), PlayerSnapshot.Nickname);
		PlayerObject->TryGetNumberField(TEXT("x"), XValue);
		PlayerObject->TryGetNumberField(TEXT("y"), YValue);
		PlayerObject->TryGetNumberField(TEXT("hp"), HPValue);
		PlayerObject->TryGetNumberField(TEXT("score"), ScoreValue);
		PlayerObject->TryGetNumberField(TEXT("last_seq"), LastSeqValue);

		PlayerSnapshot.PlayerId = static_cast<int32>(PlayerIdValue);
		PlayerSnapshot.X = static_cast<float>(XValue);
		PlayerSnapshot.Y = static_cast<float>(YValue);
		PlayerSnapshot.HP = static_cast<int32>(HPValue);
		PlayerSnapshot.Score = static_cast<int32>(ScoreValue);
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
			TEXT("[BattleGrid] Snapshot received. tick=%d players=%d projectiles=%d targets=%d"),
			LastSnapshotTick,
			LatestPlayerSnapshots.Num(),
			LatestProjectileSnapshots.Num(),
			LatestTargetSnapshots.Num()
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
