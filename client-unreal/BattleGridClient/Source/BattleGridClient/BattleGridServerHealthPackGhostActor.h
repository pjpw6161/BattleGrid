// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerHealthPackGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerHealthPackGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerHealthPackGhostActor();

	virtual void Tick(float DeltaSeconds) override;

	void SetSnapshotData(
		const FBattleGridServerHealthPackSnapshot& Snapshot,
		const FVector& WorldLocation
	);
	int32 GetHealthPackId() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UTextRenderComponent> LabelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

private:
	int32 HealthPackId;
	FVector TargetLocation;
	bool bActive;
	int32 HealAmount;
	float RespawnTimer;
};
