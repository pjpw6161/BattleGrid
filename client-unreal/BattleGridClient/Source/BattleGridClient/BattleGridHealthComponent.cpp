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

	CurrentHealth = FMath::Max(0.0f, MaxHealth);
}

void UBattleGridHealthComponent::ApplyDamage(float DamageAmount)
{
	if (IsDead() || DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] Took %.1f damage. HP: %.1f / %.1f"),
		DamageAmount,
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
