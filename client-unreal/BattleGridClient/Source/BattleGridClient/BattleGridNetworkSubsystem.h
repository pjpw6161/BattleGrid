// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleGridNetworkSubsystem.generated.h"

class IWebSocket;
class FJsonObject;

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerPlayerSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 PlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Z = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bAlive = true;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bInvincible = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float RespawnTimer = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float InvincibleTimer = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 PlayerKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 TargetKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 BotKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 LastSeq = 0;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerProjectileSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 ProjectileId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 OwnerPlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	FString OwnerType = TEXT("player");

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 OwnerBotId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Z = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float DirX = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float DirY = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float DirZ = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bVisualOnly = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	FVector ServerStart = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	FVector ServerEnd = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerArenaBoundsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bHasBounds = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float MinX = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float MaxX = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float MinY = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float MaxY = 0.0f;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerTargetSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 TargetId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bAlive = false;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerBotSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 BotId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Z = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Yaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bAlive = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bInvincible = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 TargetPlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float BodyRadius = 90.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float HeadRadius = 45.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float BodyHeight = 90.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float HeadHeight = 160.0f;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerHealthPackSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 HealthPackId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Z = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 HealAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float RespawnTimer = 0.0f;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerMatchSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	FString State = TEXT("in_progress");

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	float TimeLeft = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	float Duration = 300.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 TargetScore = 20;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	bool bGameOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 WinnerPlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	FString WinnerNickname;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 WinnerScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 WinnerKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	bool bIsDraw = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	bool bNoWinner = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 MatchId = 1;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerScoreboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 PlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	FString Nickname;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 BotKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 PlayerKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 TargetKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Match")
	bool bAlive = false;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridServerCombatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 EventId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	FString Type;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	FString ShortMessage;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	float Time = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 ActorPlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 TargetPlayerId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 BotId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 TargetId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 HealthPackId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	bool bHeadshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	bool bVictimIsPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	bool bVictimIsBot = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	bool bKillerIsBot = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	bool bKillerIsPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 Damage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	int32 HealAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	FString HitGroup;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	float HitX = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	float HitY = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Events")
	float HitZ = 0.0f;
};

USTRUCT(BlueprintType)
struct BATTLEGRIDCLIENT_API FBattleGridKillFeedLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Kill Feed")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Kill Feed")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Kill Feed")
	int32 EventId = 0;
};

UCLASS()
class BATTLEGRIDCLIENT_API UBattleGridNetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	void Connect(const FString& InServerUrl, const FString& InNickname);
	void Disconnect();
	void SendPing();
	void SendJoin();
	void SendDebugRestartMatch();
	void SendDebugApplyDemoMode();
	void SendDebugApplyDemoPreset(const FString& PresetName);
	void SendDebugSetBotAttacks(bool bEnabled);
	void SendDebugSetBotDifficulty(const FString& Difficulty);
	void SendDebugSetMatchTimer(bool bEnabled);
	void SendInput(
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
		float SpreadDegrees,
		bool bHasFireOrigin,
		float FireOriginX,
		float FireOriginY,
		float FireOriginZ,
		bool bHasClientPosition,
		float ClientX,
		float ClientY,
		float ClientZ
	);
	void ConfigureDemoLogging(
		bool bInVerboseNetworkLogs,
		bool bInVerboseSnapshotLogs,
		bool bInVerboseInputLogs,
		int32 InSnapshotLogInterval,
		int32 InInputAckLogInterval
	);
	void SetShotResultDisplayDuration(float InDurationSeconds);

	bool IsConnected() const;
	bool HasJoined() const;
	int32 GetPlayerId() const;
	int32 GetRoomId() const;
	FString GetServerUrl() const;
	FString GetNickname() const;
	FString GetLastServerMessage() const;
	FString GetLastError() const;
	FString GetLastDebugMessage() const;
	FString GetConnectionStatusText() const;
	bool HasSnapshot() const;
	int32 GetLastSnapshotTick() const;
	int32 GetLastSnapshotRoomId() const;
	bool HasArenaBounds() const;
	FBattleGridServerArenaBoundsSnapshot GetLatestArenaBounds() const;
	FString GetServerArenaBoundsText() const;
	bool HasBotAreaBounds() const;
	FBattleGridServerArenaBoundsSnapshot GetLatestBotAreaBounds() const;
	FString GetServerBotAreaBoundsText() const;
	void GetLatestPlayerSnapshots(TArray<FBattleGridServerPlayerSnapshot>& OutSnapshots) const;
	bool GetPlayerSnapshotById(int32 InPlayerId, FBattleGridServerPlayerSnapshot& OutSnapshot) const;
	void GetLatestProjectileSnapshots(TArray<FBattleGridServerProjectileSnapshot>& OutProjectiles) const;
	void GetLatestTargetSnapshots(TArray<FBattleGridServerTargetSnapshot>& OutTargets) const;
	void GetLatestBotSnapshots(TArray<FBattleGridServerBotSnapshot>& OutBots) const;
	void GetLatestHealthPackSnapshots(TArray<FBattleGridServerHealthPackSnapshot>& OutHealthPacks) const;
	int32 GetServerProjectileCount() const;
	int32 GetServerTargetCount() const;
	int32 GetServerAliveTargetCount() const;
	int32 GetServerBotCount() const;
	int32 GetServerAliveBotCount() const;
	int32 GetServerHealthPackCount() const;
	int32 GetServerActiveHealthPackCount() const;
	bool HasMatchSnapshot() const;
	FBattleGridServerMatchSnapshot GetLatestMatchSnapshot() const;
	void GetLatestScoreboard(TArray<FBattleGridServerScoreboardEntry>& OutScoreboard) const;
	FString GetServerScoreboardSummaryText() const;
	FString GetTopFiveRankingText() const;
	FString GetScoreboardTableText() const;
	FString GetMatchHeaderText() const;
	FString GetFullScoreboardText() const;
	void GetRecentCombatEvents(TArray<FBattleGridServerCombatEvent>& OutEvents) const;
	FString GetCombatEventFeedText() const;
	void GetKillFeedLines(TArray<FBattleGridKillFeedLine>& OutLines) const;
	FString GetLastDeathCauseText() const;
	FString GetLastShotResultMessage() const;
	bool HasRecentShotResult() const;
	FString GetRecentHealMessage() const;
	bool HasRecentHealMessage() const;
	bool GetOwnPlayerSnapshot(FBattleGridServerPlayerSnapshot& OutSnapshot) const;
	int32 GetOwnServerScore() const;
	int32 GetOwnServerHP() const;
	FString GetServerPrimaryStatusText() const;
	FString GetServerMatchStatusText() const;
	bool IsMatchGameOver() const;
	FString GetMatchStatusText() const;
	FString GetGameOverText() const;
	FString GetServerOwnPlayerStatusText() const;
	FString GetServerWorldCountsText() const;
	FString GetServerScoreboardCompactText() const;
	FString GetServerCombatEventFeedText() const;
	FString GetGameplayHpText() const;
	FString GetGameplayMatchText() const;
	FString GetGameplayKdText() const;
	FString GetSmallServerStatusText() const;
	FString GetDebugHudText() const;
	FString GetServerSummaryText() const;

private:
	void HandleConnected();
	void HandleConnectionError(const FString& Error);
	void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleMessage(const FString& Message);
	void HandleSnapshotMessage(const TSharedPtr<FJsonObject>& JsonObject);
	bool SendJsonObject(const TSharedRef<FJsonObject>& JsonObject, const TCHAR* LogLabel);

private:
	TSharedPtr<IWebSocket> Socket;
	bool bIsConnected = false;
	bool bHasJoined = false;
	FString ServerUrl = TEXT("ws://127.0.0.1:7777");
	FString Nickname = TEXT("player1");
	int32 PlayerId = 0;
	int32 RoomId = 0;
	FString LastServerMessage;
	FString LastError;
	FString LastDebugMessage;
	FString CurrentDemoPreset;
	int32 LastSnapshotTick = 0;
	int32 LastSnapshotRoomId = 0;
	FBattleGridServerArenaBoundsSnapshot LatestArenaBounds;
	FBattleGridServerArenaBoundsSnapshot LatestBotAreaBounds;
	TMap<int32, FBattleGridServerPlayerSnapshot> LatestPlayerSnapshots;
	TMap<int32, FBattleGridServerProjectileSnapshot> LatestProjectileSnapshots;
	TMap<int32, FBattleGridServerTargetSnapshot> LatestTargetSnapshots;
	TMap<int32, FBattleGridServerBotSnapshot> LatestBotSnapshots;
	TMap<int32, FBattleGridServerHealthPackSnapshot> LatestHealthPackSnapshots;
	FBattleGridServerMatchSnapshot LatestMatchSnapshot;
	TArray<FBattleGridServerScoreboardEntry> LatestScoreboard;
	TArray<FBattleGridServerCombatEvent> RecentCombatEvents;
	TSet<int32> SeenCombatEventIds;
	int32 MaxRecentCombatEvents = 20;
	FString LastShotResultMessage;
	double LastShotResultTimestampSeconds = -1000.0;
	float LastShotResultDisplaySeconds = 1.25f;
	FString LastHealMessage;
	double LastHealMessageTimestampSeconds = -1000.0;
	float LastHealMessageDisplaySeconds = 1.5f;
	bool bHasMatchSnapshot = false;
	bool bHasLoggedServerSummary = false;
	bool bVerboseNetworkLogs = false;
	bool bVerboseSnapshotLogs = false;
	bool bVerboseInputLogs = false;
	int32 SnapshotLogInterval = 60;
	int32 InputAckLogInterval = 60;
	int32 InputAckLogCounter = 0;
	int32 InputSendLogCounter = 0;
	int32 SnapshotLogCounter = 0;
};
