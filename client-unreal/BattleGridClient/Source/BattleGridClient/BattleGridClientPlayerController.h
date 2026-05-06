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
class ABattleGridServerTargetGhostActor;
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
	FString ResolveServerUrl() const;
	FString GetServerProfileText() const;
	FString GetNetworkStatusText() const;
	FString GetDetailedNetworkStatusText() const;
	bool IsServerConnected() const;
	bool HasJoinedServer() const;
	int32 GetServerPlayerId() const;
	int32 GetServerRoomId() const;
	int32 GetOwnServerScore() const;
	int32 GetServerAliveTargetCount() const;
	int32 GetServerTargetCount() const;
	int32 GetServerProjectileCount() const;
	float GetLastServerPositionError() const;
	bool HasOwnServerWorldLocation() const;
	FVector GetLastOwnServerWorldLocation() const;
	bool IsUsingServerPositionCorrection() const;
	bool IsAimingDownSights() const;
	bool IsSprinting() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputMappingContext> BattleGridMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> AdsAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Game", meta = (ClampMin = "1"))
	int32 TargetScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	bool bAutoConnectToServer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString ServerUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	bool bUseRemoteServer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString LocalServerUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString RemoteServerUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString ServerProfileLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Network", meta = (ClampMin = "0.0"))
	float InputSendIntervalSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Movement", meta = (ClampMin = "0.0"))
	float NormalMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Movement", meta = (ClampMin = "0.0"))
	float SprintMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Movement", meta = (ClampMin = "0.0"))
	float ADSMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera")
	float LookYawSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera")
	float LookPitchSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera")
	float ViewPitchMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera")
	float ViewPitchMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bDemoMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bVerboseNetworkLogs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bVerboseSnapshotLogs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bVerboseInputLogs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo", meta = (ClampMin = "1"))
	int32 SnapshotLogInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo", meta = (ClampMin = "1"))
	int32 InputAckLogInterval;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowServerTargetGhosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	TSubclassOf<ABattleGridServerTargetGhostActor> ServerTargetGhostActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerTargetGhostHeight;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void MoveForwardReleased(const FInputActionValue& Value);
	void MoveRightReleased(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void FireStarted(const FInputActionValue& Value);
	void AdsStarted(const FInputActionValue& Value);
	void AdsEnded(const FInputActionValue& Value);
	void SprintStarted(const FInputActionValue& Value);
	void SprintEnded(const FInputActionValue& Value);
	void JumpStarted(const FInputActionValue& Value);
	void JumpEnded(const FInputActionValue& Value);
	void ReloadStarted(const FInputActionValue& Value);

	FString ResolveServerProfileLabel() const;
	void UpdateAimRotation();
	void ApplyMovementAndADSState();
	bool IsControlledPawnFalling() const;
	FVector GetCameraRelativeMovementDirection(float ForwardAxis, float RightAxis) const;
	void SendInputToServerIfNeeded();
	void SendInputToServer(bool bForceSend);
	void UpdateServerGhostsFromSnapshot();
	FVector ConvertServerPositionToWorld(float ServerX, float ServerY) const;
	FVector ConvertServerPositionToWorld(float ServerX, float ServerY, float WorldHeight) const;
	FVector2D ConvertUnrealDirectionToServerDirection(const FVector& UnrealForward) const;
	FVector2D ConvertServerDirectionToUnrealDirection(float ServerDirX, float ServerDirY) const;
	void UpdateServerProjectileGhostsFromSnapshot();
	void UpdateServerTargetGhostsFromSnapshot();
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
	bool bADSInputHeld;
	bool bIsADSActive;
	bool bIsSprinting;
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
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ABattleGridServerTargetGhostActor>> ServerTargetGhostActors;
	FVector ServerSnapshotOrigin;
	bool bServerSnapshotOriginInitialized;
	int32 LastProcessedSnapshotTick;
	float LastServerPositionError;
	FVector LastOwnServerWorldLocation;
	bool bHasOwnServerWorldLocation;
	int32 LastServerPositionErrorLogSnapshotTick;
};
