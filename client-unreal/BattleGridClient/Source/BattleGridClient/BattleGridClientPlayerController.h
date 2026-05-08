// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleGridNetworkSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Templates/SubclassOf.h"
#include "BattleGridClientPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ABattleGridProjectile;
class ABattleGridServerGhostActor;
class ABattleGridServerProjectileGhostActor;
class ABattleGridServerTargetGhostActor;
class ABattleGridServerBotGhostActor;
class ABattleGridServerHealthPackGhostActor;
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
	FString GetServerCombatEventFeedText() const;
	FString GetRecentServerShotResultText() const;
	FString GetServerScoreboardText() const;
	FString GetTopFiveRankingText() const;
	void GetKillFeedLines(TArray<FBattleGridKillFeedLine>& OutLines) const;
	bool ShouldShowScoreboard() const;
	bool UseServerAuthoritativeHud() const;
	bool ShowLocalDebugHud() const;
	bool ShowServerDebugDetails() const;
	bool ShowCombatEventFeed() const;
	FString GetServerPrimaryHudText() const;
	FString GetLocalDebugHudText() const;
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
	float GetLastShotSpreadDegrees() const;
	float GetCurrentWeaponSpreadDegrees() const;
	bool IsCrosshairAds() const;
	bool IsCrosshairSprinting() const;
	bool IsCrosshairJumping() const;
	bool IsCrosshairReloading() const;
	bool ShouldShowCrosshair() const;
	bool IsServerDead() const;
	bool IsServerInvincible() const;
	float GetLastServerRespawnTimer() const;
	float GetLastServerInvincibleTimer() const;
	FString GetServerLifeStateText() const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Input")
	TObjectPtr<UInputAction> ScoreboardAction;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server State")
	bool bRespectServerDeathState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server State")
	bool bServerDeathLocksInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server State")
	bool bSnapLocalPawnOnServerRespawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server State")
	bool bShowServerDeathStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement")
	bool bSnapLocalPawnToServerOnJoin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement")
	bool bSnapLocalPawnToServerOnRespawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement", meta = (ClampMin = "0.0"))
	float JoinSnapDelaySeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement")
	bool bSendClientPositionToServer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement")
	bool bUseGentleServerPositionCorrection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement", meta = (ClampMin = "0.0"))
	float GentleCorrectionThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement", meta = (ClampMin = "0.0"))
	float GentleCorrectionInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Movement", meta = (ClampMin = "0.0"))
	float HardCorrectionThreshold;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bApplyDemoServerSettingsOnJoin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bApplySafeDemoModeOnJoin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bDemoBotAttacksEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	FString DemoBotDifficulty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Demo")
	bool bDemoAutoEndMatchByTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bScoreboardToggleMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bUseServerAuthoritativeHud;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bShowLocalDebugHud;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bShowServerDebugDetails;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bShowCombatEventFeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bShowTopFiveRanking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD")
	bool bShowKillFeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|HUD", meta = (ClampMin = "0.1"))
	float ShotResultDisplayDurationSeconds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat")
	TSubclassOf<ABattleGridProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (ClampMin = "0.0"))
	float ProjectileSpawnDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (ClampMin = "0.0"))
	float FireCooldownSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bUseServerAuthoritativeFireVisuals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bSpawnLegacyLocalProjectileWhenConnected;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bAllowLegacyLocalProjectileDamageWhenConnected;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bSpawnLegacyLocalProjectileWhenOffline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bSpawnLocalProjectileFromMuzzle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bUseClientMuzzleForServerTracerStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (ClampMin = "100.0"))
	float AimTraceDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual")
	bool bShowAimDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Debug")
	bool bShowServerShotDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Debug", meta = (ClampMin = "100.0"))
	float ServerShotDebugLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Debug")
	bool bDrawServerArenaBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Debug")
	float ServerArenaBoundsDebugZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Debug")
	bool bDrawServerBotAreaBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Debug")
	float BotAreaBoundsDebugZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowProjectileGhostDebug;

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
	bool bUseSnapshotZForServerPlayerGhosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerPlayerGhostZOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bUseLocalPawnZForOwnServerGhost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bAutoCalibrateServerSnapshotOrigin;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowServerBotGhosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	TSubclassOf<ABattleGridServerBotGhostActor> ServerBotGhostActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerBotGhostHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bShowServerHealthPackGhosts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	TSubclassOf<ABattleGridServerHealthPackGhostActor> ServerHealthPackGhostActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	float ServerHealthPackGhostHeight;

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
	void FireEnded(const FInputActionValue& Value);
	void TryFireWeapon();
	void AdsStarted(const FInputActionValue& Value);
	void AdsEnded(const FInputActionValue& Value);
	void SprintStarted(const FInputActionValue& Value);
	void SprintEnded(const FInputActionValue& Value);
	void JumpStarted(const FInputActionValue& Value);
	void JumpEnded(const FInputActionValue& Value);
	void ReloadStarted(const FInputActionValue& Value);
	void ScoreboardStarted(const FInputActionValue& Value);
	void ScoreboardEnded(const FInputActionValue& Value);

	FString ResolveServerProfileLabel() const;
	void UpdateAimRotation();
	void ApplyMovementAndADSState();
	bool IsControlledPawnFalling() const;
	bool IsServerAuthoritativeFireMode() const;
	FVector GetCameraRelativeMovementDirection(float ForwardAxis, float RightAxis) const;
	FVector CalculateShotDirectionWithSpread(float SpreadDegrees) const;
	FVector CalculateShotDirectionWithSpread(float SpreadDegrees, const FVector& BaseDirection) const;
	bool GetCrosshairAimPoint(FVector& OutAimPoint, FVector& OutAimDirection) const;
	bool GetCurrentFireOriginAndDirectionForServer(
		FVector& OutFireOriginWorld,
		FVector& OutShotDirectionWorld
	) const;
	void SendInputToServerIfNeeded();
	void SendInputToServer(bool bForceSend);
	void ApplyDemoServerSettingsIfNeeded();
	bool ShouldBlockServerGameplayInput() const;
	void UpdateServerGhostsFromSnapshot();
	FVector ConvertServerPositionToWorldNoOrigin(float ServerX, float ServerY, float WorldHeight) const;
	FVector ConvertServerPositionToWorld(float ServerX, float ServerY) const;
	FVector ConvertServerPositionToWorld(float ServerX, float ServerY, float WorldHeight) const;
	bool ConvertWorldPositionToServerPosition(
		const FVector& WorldLocation,
		FVector& OutServerLocation
	) const;
	FVector2D ConvertUnrealDirectionToServerDirection(const FVector& UnrealForward) const;
	FVector2D ConvertServerDirectionToUnrealDirection(float ServerDirX, float ServerDirY) const;
	float ConvertServerYawToUnrealYaw(float ServerYawDegrees) const;
	void CalibrateServerSnapshotOriginIfNeeded();
	void ResetServerSnapshotOriginCalibration();
	void SnapLocalPawnToServerOnJoinIfNeeded(float DeltaTime);
	bool SnapLocalPawnToOwnServerSnapshot(const TCHAR* ReasonText);
	FString GetServerCorrectionModeText() const;
	FString GetServerArenaBoundsDebugText() const;
	FString GetServerBotAreaBoundsDebugText() const;
	void DrawServerArenaBoundsIfEnabled() const;
	void DrawServerBotAreaBoundsIfEnabled() const;
	bool BuildServerProjectileVisualWorldSegment(
		const FBattleGridServerProjectileSnapshot& Snapshot,
		FVector& OutStartWorld,
		FVector& OutEndWorld
	) const;
	void UpdateServerProjectileGhostsFromSnapshot();
	void UpdateServerTargetGhostsFromSnapshot();
	void UpdateServerBotGhostsFromSnapshot();
	void UpdateServerHealthPackGhostsFromSnapshot();
	void UpdateOwnServerPositionErrorAndCorrection(float DeltaTime);
	void UpdateServerLifeStateFromSnapshot(float DeltaTime);

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
	bool bFireHeld;
	bool bPendingFireInput;
	bool bPendingReloadInput;
	bool bScoreboardHeld;
	bool bScoreboardVisible;
	bool bDemoServerSettingsAppliedForJoin;
	bool bSafeDemoModeAppliedForJoin;
	int32 LastDemoSettingsPlayerId;
	int32 LastSafeDemoModePlayerId;
	int32 ShotSequence;
	FVector2D LastShotDirectionServer;
	float LastShotDirectionServerZ;
	FVector LastFireOriginWorld;
	FVector LastFireOriginServer;
	bool bLastFireOriginWorldValid;
	bool bLastFireOriginServerValid;
	float LastShotSpreadDegrees;
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
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ABattleGridServerBotGhostActor>> ServerBotGhostActors;
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ABattleGridServerHealthPackGhostActor>> ServerHealthPackGhostActors;
	FVector ServerSnapshotOrigin;
	bool bServerSnapshotOriginInitialized;
	bool bServerSnapshotOriginCalibrated;
	int32 LastCalibratedServerPlayerId;
	int32 LastCalibrationSnapshotTick;
	int32 LastCalibrationMatchId;
	bool bHasSnappedLocalPawnToServerOnJoin;
	int32 LastJoinSnapServerPlayerId;
	int32 LastJoinSnapMatchId;
	float JoinSnapTimer;
	int32 LastProcessedSnapshotTick;
	float LastServerPositionError;
	FVector LastOwnServerWorldLocation;
	bool bHasOwnServerWorldLocation;
	int32 LastServerPositionErrorLogSnapshotTick;
	int32 LastCorrectionObservedSnapshotTick;
	float LastCorrectionObservedSnapshotWorldTime;
	bool bLoggedBothCorrectionModesWarning;
	bool bWasServerAlive;
	bool bIsServerDead;
	bool bWasServerInvincible;
	float LastServerRespawnTimer;
	float LastServerInvincibleTimer;
};
