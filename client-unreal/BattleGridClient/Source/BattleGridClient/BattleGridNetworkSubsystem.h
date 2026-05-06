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
	int32 HP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	int32 MaxHP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bAlive = true;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	bool bInvincible = false;

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
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float DirX = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	float DirY = 0.0f;
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
		float SpreadDegrees
	);
	void ConfigureDemoLogging(
		bool bInVerboseNetworkLogs,
		bool bInVerboseSnapshotLogs,
		bool bInVerboseInputLogs,
		int32 InSnapshotLogInterval,
		int32 InInputAckLogInterval
	);

	bool IsConnected() const;
	bool HasJoined() const;
	int32 GetPlayerId() const;
	int32 GetRoomId() const;
	FString GetServerUrl() const;
	FString GetNickname() const;
	FString GetLastServerMessage() const;
	FString GetLastError() const;
	FString GetConnectionStatusText() const;
	bool HasSnapshot() const;
	int32 GetLastSnapshotTick() const;
	int32 GetLastSnapshotRoomId() const;
	void GetLatestPlayerSnapshots(TArray<FBattleGridServerPlayerSnapshot>& OutSnapshots) const;
	bool GetPlayerSnapshotById(int32 InPlayerId, FBattleGridServerPlayerSnapshot& OutSnapshot) const;
	void GetLatestProjectileSnapshots(TArray<FBattleGridServerProjectileSnapshot>& OutProjectiles) const;
	void GetLatestTargetSnapshots(TArray<FBattleGridServerTargetSnapshot>& OutTargets) const;
	void GetLatestBotSnapshots(TArray<FBattleGridServerBotSnapshot>& OutBots) const;
	int32 GetServerProjectileCount() const;
	int32 GetServerTargetCount() const;
	int32 GetServerAliveTargetCount() const;
	int32 GetServerBotCount() const;
	int32 GetServerAliveBotCount() const;
	bool GetOwnPlayerSnapshot(FBattleGridServerPlayerSnapshot& OutSnapshot) const;
	int32 GetOwnServerScore() const;
	int32 GetOwnServerHP() const;
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
	int32 LastSnapshotTick = 0;
	int32 LastSnapshotRoomId = 0;
	TMap<int32, FBattleGridServerPlayerSnapshot> LatestPlayerSnapshots;
	TMap<int32, FBattleGridServerProjectileSnapshot> LatestProjectileSnapshots;
	TMap<int32, FBattleGridServerTargetSnapshot> LatestTargetSnapshots;
	TMap<int32, FBattleGridServerBotSnapshot> LatestBotSnapshots;
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
