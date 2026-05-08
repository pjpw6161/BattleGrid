// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "BattleGridClientCharacter.generated.h"

class UCameraComponent;
class AController;
class UBattleGridHealthComponent;
class UBattleGridWeaponComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UAnimationAsset;

/**
 *  A controllable third-person BattleGrid character.
 */
UCLASS(abstract)
class ABattleGridClientCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Third-person follow camera. Kept with the old component name for Blueprint compatibility. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** Third-person follow camera pointer exposed with the new PvPvE naming. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Spring arm positioning the camera behind the character. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

public:

	/** Constructor */
	ABattleGridClientCharacter();

	/** Initialization */
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

	/** Returns the camera component **/
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }

	UCameraComponent* GetFollowCamera() const { return FollowCamera.Get(); }

	/** Returns the Camera Boom component **/
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

	void SetADSActive(bool bNewADSActive);
	bool IsADSActive() const { return bADSActive; }
	void SetAimingDownSights(bool bInADS);
	bool IsAimingDownSights() const { return bADSActive; }
	void SetSprinting(bool bInSprinting);
	bool IsSprinting() const { return bSprinting; }
	UBattleGridWeaponComponent* GetWeaponComponent() const { return WeaponComponent.Get(); }
	void AttachWeaponToCharacterMesh();
	FVector GetApproximateMuzzleWorldLocation() const;
	void ApplyPlayerMeshVisualSettings();

private:
	void HandleDeath();
	void Respawn();
	void SyncHealthToPlayerController() const;
	void ApplyCameraSettings(float DeltaSeconds);
	void UpdateSimplePlayerAnimation(float DeltaSeconds);
	bool PlaySimplePlayerAnimation(UAnimationAsset* Animation, FName StateName, bool bLoopAnimation);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBattleGridHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBattleGridWeaponComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	FName WeaponSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	FVector WeaponRelativeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	FRotator WeaponRelativeRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	FVector WeaponRelativeScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	FName MuzzleSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	FVector MuzzleFallbackOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true"))
	bool bShowMuzzleDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MuzzleDebugSphereRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (AllowPrivateAccess = "true"))
	FVector PlayerMeshRelativeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (AllowPrivateAccess = "true"))
	FRotator PlayerMeshRelativeRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (AllowPrivateAccess = "true"))
	FVector PlayerMeshRelativeScale3D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	bool bUseSimplePlayerAnimationPlayback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimationAsset> PlayerIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimationAsset> PlayerRunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimationAsset> PlayerJumpAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimationAsset> PlayerJumpStartAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimationAsset> PlayerJumpLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimationAsset> PlayerJumpLandAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true"))
	bool bLoopPlayerJumpLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PlayerLandAnimationLockSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PlayerRunSpeedThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DefaultArmLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AdsArmLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true"))
	FVector DefaultSocketOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true"))
	FVector AdsSocketOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraLagSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PlayerCameraCollisionProbeSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "170.0"))
	float DefaultFOV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "170.0"))
	float AdsFOV;

	bool bIsDead;
	bool bADSActive;
	bool bSprinting;
	FVector RespawnLocation;
	FRotator RespawnRotation;
	FTimerHandle RespawnTimerHandle;
	float RespawnDelaySeconds;
	bool bLoggedMissingWeaponSocketWarning;
	mutable bool bLoggedMissingMuzzleSocketWarning;
	bool bLoggedWeaponAttachment;
	FVector LastPlayerWorldLocation;
	float PlayerVisualSpeed;
	FName CurrentPlayerVisualAnimState;
	bool bWasPlayerFalling;
	bool bPlayerJumpStartPlayed;
	bool bPlayerJumpLoopPlayed;
	bool bPlayerLandPlaying;
	float PlayerLandLockTimer;
};

