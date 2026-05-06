// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerBotGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerBotGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerBotGhostActor();

	virtual void Tick(float DeltaSeconds) override;

	void SetSnapshotData(
		const FBattleGridServerBotSnapshot& Snapshot,
		const FVector& WorldLocation
	);
	int32 GetBotId() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UTextRenderComponent> LabelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

private:
	int32 BotId;
	FString Name;
	FVector TargetLocation;
	float TargetYaw;
	int32 HP;
	int32 MaxHP;
	bool bAlive;
	bool bInvincible;
};
