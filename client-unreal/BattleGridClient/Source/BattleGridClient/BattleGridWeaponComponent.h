// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "BattleGridWeaponComponent.generated.h"

UCLASS(ClassGroup = (BattleGrid), meta = (BlueprintSpawnableComponent))
class BATTLEGRIDCLIENT_API UBattleGridWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBattleGridWeaponComponent();

	bool CanFire() const;
	bool TryConsumeAmmoForShot();
	bool StartReload();
	void CancelReload();
	void FinishReload();
	void ResetAmmoToFull();
	bool IsReloading() const;
	int32 GetCurrentAmmo() const;
	int32 GetMagazineSize() const;
	float GetReloadRemainingSeconds() const;
	float CalculateCurrentSpread(bool bIsADS, bool bIsSprinting, bool bIsJumping) const;
	float GetHipSpreadDegrees() const;
	float GetAdsSpreadDegrees() const;
	float GetSprintSpreadDegrees() const;
	float GetJumpSpreadDegrees() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Ammo", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MagazineSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Ammo", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 CurrentAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Ammo", meta = (AllowPrivateAccess = "true"))
	bool bInfiniteReserveAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Reload", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ReloadTimeSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Weapon|Reload", meta = (AllowPrivateAccess = "true"))
	bool bIsReloading;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Weapon|Reload", meta = (AllowPrivateAccess = "true"))
	float ReloadTimerSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Fire", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float FireRatePerSecond;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Weapon|Fire", meta = (AllowPrivateAccess = "true"))
	float LastFireTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 BodyDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 HeadshotDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Spread", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HipSpreadDegrees;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Spread", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AdsSpreadDegrees;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Spread", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintSpreadDegrees;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Weapon|Spread", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float JumpSpreadDegrees;

	FTimerHandle ReloadTimerHandle;
};
