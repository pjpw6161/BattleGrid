// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerHealthPackGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerHealthPackGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerHealthPackGhostActor();

	virtual void BeginPlay() override;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> ActiveMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> InactiveMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float ActiveScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float InactiveScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.0"))
	float LabelHeight;

private:
	void ApplyVisualState(bool bIsActive);

	int32 HealthPackId;
	FVector TargetLocation;
	bool bActive;
	int32 HealAmount;
	float RespawnTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultMaterial;
};
