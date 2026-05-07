// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerProjectileGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerProjectileGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerProjectileGhostActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetSnapshotData(
		const FBattleGridServerProjectileSnapshot& Snapshot,
		const FVector& WorldLocation,
		const FVector& UnrealDirection
	);
	int32 GetProjectileId() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UPointLightComponent> PointLightComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> AliveMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> PlayerProjectileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> BotProjectileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float ProjectileScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	bool bUsePointLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.0"))
	float PointLightIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.0"))
	float PointLightRadius;

private:
	int32 ProjectileId;
	FString OwnerType;
	FVector TargetLocation;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultMaterial;
};
