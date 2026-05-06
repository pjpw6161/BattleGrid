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
class USpringArmComponent;

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

private:
	void HandleDeath();
	void Respawn();
	void SyncHealthToPlayerController() const;
	void ApplyCameraSettings(float DeltaSeconds);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBattleGridHealthComponent> HealthComponent;

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
};

