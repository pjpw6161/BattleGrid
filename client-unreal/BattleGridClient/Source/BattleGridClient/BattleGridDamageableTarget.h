// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Actor.h"
#include "BattleGridDamageableTarget.generated.h"

class AController;
class UBattleGridHealthComponent;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridDamageableTarget : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridDamageableTarget();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Target", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Target", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Target", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBattleGridHealthComponent> HealthComponent;
};
