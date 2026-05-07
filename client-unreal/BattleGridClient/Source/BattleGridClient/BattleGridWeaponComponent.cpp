// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridWeaponComponent.h"

#include "Engine/World.h"

UBattleGridWeaponComponent::UBattleGridWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MagazineSize = 30;
	CurrentAmmo = 30;
	bInfiniteReserveAmmo = true;
	ReloadTimeSeconds = 2.0f;
	bIsReloading = false;
	FireRatePerSecond = 8.0f;
	LastFireTime = -999.0f;
	BodyDamage = 20;
	HeadshotDamage = 40;
	HipSpreadDegrees = 3.5f;
	AdsSpreadDegrees = 0.8f;
	SprintSpreadDegrees = 6.0f;
	JumpSpreadDegrees = 9.0f;
}

bool UBattleGridWeaponComponent::CanFire() const
{
	if (bIsReloading || CurrentAmmo <= 0)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	const float FireInterval = 1.0f / FMath::Max(0.01f, FireRatePerSecond);
	return World->GetTimeSeconds() - LastFireTime >= FireInterval;
}

bool UBattleGridWeaponComponent::TryConsumeAmmoForShot()
{
	if (!CanFire())
	{
		return false;
	}

	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);

	if (const UWorld* World = GetWorld())
	{
		LastFireTime = World->GetTimeSeconds();
	}

	return true;
}

void UBattleGridWeaponComponent::StartReload()
{
	if (bIsReloading || CurrentAmmo >= MagazineSize)
	{
		return;
	}

	bIsReloading = true;
	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Reload started."));

	if (ReloadTimeSeconds <= 0.0f)
	{
		FinishReload();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&UBattleGridWeaponComponent::FinishReload,
			ReloadTimeSeconds,
			false
		);
	}
	else
	{
		FinishReload();
	}
}

void UBattleGridWeaponComponent::FinishReload()
{
	CurrentAmmo = MagazineSize;
	bIsReloading = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] Reload finished. Ammo=%d/%d"),
		CurrentAmmo,
		MagazineSize
	);
}

bool UBattleGridWeaponComponent::IsReloading() const
{
	return bIsReloading;
}

int32 UBattleGridWeaponComponent::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 UBattleGridWeaponComponent::GetMagazineSize() const
{
	return MagazineSize;
}

float UBattleGridWeaponComponent::GetReloadRemainingSeconds() const
{
	if (!bIsReloading)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, World->GetTimerManager().GetTimerRemaining(ReloadTimerHandle));
}

float UBattleGridWeaponComponent::CalculateCurrentSpread(
	bool bIsADS,
	bool bIsSprinting,
	bool bIsJumping
) const
{
	if (bIsJumping)
	{
		return JumpSpreadDegrees;
	}

	if (bIsSprinting)
	{
		return SprintSpreadDegrees;
	}

	if (bIsADS)
	{
		return AdsSpreadDegrees;
	}

	return HipSpreadDegrees;
}

float UBattleGridWeaponComponent::GetHipSpreadDegrees() const
{
	return HipSpreadDegrees;
}

float UBattleGridWeaponComponent::GetAdsSpreadDegrees() const
{
	return AdsSpreadDegrees;
}

float UBattleGridWeaponComponent::GetSprintSpreadDegrees() const
{
	return SprintSpreadDegrees;
}

float UBattleGridWeaponComponent::GetJumpSpreadDegrees() const
{
	return JumpSpreadDegrees;
}
