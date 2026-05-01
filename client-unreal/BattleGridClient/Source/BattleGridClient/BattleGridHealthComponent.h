// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleGridHealthComponent.generated.h"

UCLASS(ClassGroup = (BattleGrid), meta = (BlueprintSpawnableComponent))
class BATTLEGRIDCLIENT_API UBattleGridHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBattleGridHealthComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "BattleGrid|Health")
	void ResetHealth();

	UFUNCTION(BlueprintCallable, Category = "BattleGrid|Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "BattleGrid|Health")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "BattleGrid|Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "BattleGrid|Health")
	float GetMaxHealth() const;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxHealth;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "BattleGrid|Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth;
};
