// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerProjectileGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerProjectileGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerProjectileGhostActor();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

private:
	int32 ProjectileId;
	FVector TargetLocation;
};
