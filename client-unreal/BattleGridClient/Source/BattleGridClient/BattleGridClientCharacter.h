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
 *  A controllable top-down perspective character
 */
UCLASS(abstract)
class ABattleGridClientCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
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

	/** Returns the Camera Boom component **/
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

private:
	void HandleDeath();
	void Respawn();
	void SyncHealthToPlayerController() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBattleGridHealthComponent> HealthComponent;

	bool bIsDead;
	FVector RespawnLocation;
	FRotator RespawnRotation;
	FTimerHandle RespawnTimerHandle;
	float RespawnDelaySeconds;
};

