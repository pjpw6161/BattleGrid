// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridHealthComponent.h"

UBattleGridHealthComponent::UBattleGridHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
}

void UBattleGridHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	ResetHealth();
}

void UBattleGridHealthComponent::ResetHealth()
{
	CurrentHealth = FMath::Max(0.0f, MaxHealth);
}

void UBattleGridHealthComponent::ApplyDamage(float DamageAmount)
{
	if (IsDead() || DamageAmount <= 0.0f)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] HP before damage: %.1f / %.1f"),
		CurrentHealth,
		MaxHealth
	);

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] Took %.1f damage. HP: %.1f / %.1f"),
		DamageAmount,
		CurrentHealth,
		MaxHealth
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] HP after damage: %.1f / %.1f"),
		CurrentHealth,
		MaxHealth
	);

	if (IsDead())
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Health reached zero."));
	}
}

bool UBattleGridHealthComponent::IsDead() const
{
	return CurrentHealth <= 0.0f;
}

float UBattleGridHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UBattleGridHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}
