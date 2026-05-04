// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Templates/SubclassOf.h"
#include "BattleGridClientPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ABattleGridProjectile;
class ABattleGridServerGhostActor;
class ABattleGridServerProjectileGhostActor;
struct FInputActionValue;

UCLASS(Blueprintable, BlueprintType)
class BATTLEGRIDCLIENT_API ABattleGridClientPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABattleGridClientPlayerController();

	float GetMaxPlayerHealth() const;
	float GetCurrentPlayerHealth() const;
	int32 GetScore() const;
	int32 GetTargetScore() const;
	FString GetCombatMessage() const;
	bool HasActiveCombatMessage() const;
	bool HasWon() const;
	bool IsPlayerDead() const;
	void AddScore(int32 Amount);
	void SetCombatMessage(const FString& Message, float DurationSeconds = 2.0f);
	void SetPlayerHealth(float Current, float Max);
	void SetPlayerDead(bool bDead);
	void RestartGame();
	void RestartStarted(const FInputActionValue& Value);
	FString GetNetworkStatusText() const;
	bool IsServerConnected() const;
	bool HasJoinedServer() const;
	int32 GetServerPlayerId() const;
	int32 GetServerRoomId() const;
	float GetLastServerPositionError() const;
	bool HasOwnServerWorldLocation() const;
	FVector GetLastOwnServerWorldLocation() const;
	bool IsUsingServerPositionCorrection() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputMappingContext> BattleGridMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Game", meta = (ClampMin = "1"))
	int32 TargetScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	bool bAutoConnectToServer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString ServerUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network", meta = (ClampMin = "0.0"))
	float InputSendIntervalSeconds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat")
	TSubclassOf<ABattleGridProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (ClampMin = "0.0"))
	float ProjectileSpawnDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (ClampMin = "0.0"))
	float FireCooldownSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowServerGhosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowOwnServerGhost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	TSubclassOf<ABattleGridServerGhostActor> ServerGhostActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerToUnrealScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerGhostHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowServerPositionError;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bUseServerPositionCorrection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float ServerCorrectionStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float ServerCorrectionSnapDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowServerProjectileGhosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	TSubclassOf<ABattleGridServerProjectileGhostActor> ServerProjectileGhostActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerProjectileGhostHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerAimSignX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerAimSignY;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void MoveForwardReleased(const FInputActionValue& Value);
	void MoveRightReleased(const FInputActionValue& Value);
	void FireStarted(const FInputActionValue& Value);

	void UpdateAimRotation();
	void SendInputToServerIfNeeded();
	void SendInputToServer(bool bForceSend);
	void UpdateServerGhostsFromSnapshot();
	FVector ConvertServerPositionToWorld(float ServerX, float ServerY) const;
	FVector ConvertServerPositionToWorld(float ServerX, float ServerY, float WorldHeight) const;
	FVector2D ConvertUnrealDirectionToServerDirection(const FVector& UnrealForward) const;
	FVector2D ConvertServerDirectionToUnrealDirection(float ServerDirX, float ServerDirY) const;
	void UpdateServerProjectileGhostsFromSnapshot();
	void UpdateOwnServerPositionErrorAndCorrection(float DeltaTime);

	float LastFireTime;
	float MaxPlayerHealth;
	float CurrentPlayerHealth;
	int32 Score;
	FString CombatMessage;
	float CombatMessageExpireTime;
	bool bPlayerDead;
	bool bHasWon;
	float CurrentMoveForward;
	float CurrentMoveRight;
	bool bPendingFireInput;
	int32 InputSequence;
	float LastInputSendTime;
	float LastSentMoveForward;
	float LastSentMoveRight;
	float LastSentAimX;
	float LastSentAimY;
	bool bHasLastSentInput;
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ABattleGridServerGhostActor>> ServerGhostActors;
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ABattleGridServerProjectileGhostActor>> ServerProjectileGhostActors;
	FVector ServerSnapshotOrigin;
	bool bServerSnapshotOriginInitialized;
	int32 LastProcessedSnapshotTick;
	float LastServerPositionError;
	FVector LastOwnServerWorldLocation;
	bool bHasOwnServerWorldLocation;
	int32 LastServerPositionErrorLogSnapshotTick;
};
