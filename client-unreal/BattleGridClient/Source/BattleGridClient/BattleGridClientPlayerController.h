// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Templates/SubclassOf.h"
#include "BattleGridClientPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ABattleGridProjectile;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat")
	TSubclassOf<ABattleGridProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (ClampMin = "0.0"))
	float ProjectileSpawnDistance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Combat", meta = (ClampMin = "0.0"))
	float FireCooldownSeconds;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void FireStarted(const FInputActionValue& Value);

	void UpdateAimRotation();

	float LastFireTime;
	float MaxPlayerHealth;
	float CurrentPlayerHealth;
	int32 Score;
	FString CombatMessage;
	float CombatMessageExpireTime;
	bool bPlayerDead;
	bool bHasWon;
};
