// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridHazardActor.generated.h"

class ABattleGridClientCharacter;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridHazardActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridHazardActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Hazard", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Hazard", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> DamageVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Hazard", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> HazardMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Hazard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DamageAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|Hazard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DamageCooldownSeconds;

	TSet<TWeakObjectPtr<AActor>> OverlappingActors;
	TMap<TWeakObjectPtr<AActor>, float> LastDamageTimes;

	void ConfigureDamageVolumeCollision() const;
	void TryDamageActor(AActor* ActorToDamage);
};
