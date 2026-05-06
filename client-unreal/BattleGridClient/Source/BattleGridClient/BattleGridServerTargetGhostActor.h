// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerTargetGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerTargetGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerTargetGhostActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetSnapshotData(
		const FBattleGridServerTargetSnapshot& Snapshot,
		const FVector& WorldLocation
	);
	int32 GetTargetId() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UTextRenderComponent> LabelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> AliveMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> DeadMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float AliveScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float DeadScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.0"))
	float LabelHeight;

private:
	void ApplyVisualState(bool bIsAlive);

	int32 TargetId;
	FVector TargetLocation;
	int32 HP;
	int32 MaxHP;
	bool bAlive;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultMaterial;
};
