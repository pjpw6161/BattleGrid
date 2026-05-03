// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerGhostActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerGhostActor();

	virtual void Tick(float DeltaSeconds) override;

	void SetSnapshotData(
		const FBattleGridServerPlayerSnapshot& Snapshot,
		const FVector& WorldLocation
	);
	int32 GetPlayerId() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UTextRenderComponent> LabelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot")
	bool bSnapToServerLocation;

private:
	int32 PlayerId;
	FString Nickname;
	FVector TargetLocation;
};
