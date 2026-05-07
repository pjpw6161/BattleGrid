// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridServerBotGhostActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridServerBotGhostActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleGridServerBotGhostActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetSnapshotData(
		const FBattleGridServerBotSnapshot& Snapshot,
		const FVector& WorldLocation
	);
	int32 GetBotId() const;
	FVector GetApproximateMuzzleWorldLocation() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BattleGrid|Server Snapshot")
	TObjectPtr<UTextRenderComponent> LabelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Server Snapshot", meta = (ClampMin = "0.0"))
	float InterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> AliveMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> DeadMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	TObjectPtr<UMaterialInterface> InvincibleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float AliveScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float DeadScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float InvincibleScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.0"))
	float LabelHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	bool bUseSkeletalMeshVisual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	FName BotWeaponSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	FVector BotWeaponRelativeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	FRotator BotWeaponRelativeRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	FVector BotWeaponRelativeScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	FName MuzzleSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual")
	FVector MuzzleFallbackOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float HumanoidAliveScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.01"))
	float HumanoidDeadScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Visual", meta = (ClampMin = "0.0"))
	float HumanoidLabelHeight;

private:
	void ApplyVisualState(bool bIsAlive, bool bIsInvincible);
	void AttachWeaponToSkeletalMesh();

	int32 BotId;
	FString Name;
	FVector TargetLocation;
	float TargetYaw;
	int32 HP;
	int32 MaxHP;
	bool bAlive;
	bool bInvincible;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	bool bLoggedMissingWeaponSocketWarning;
	bool bLoggedWeaponAttachment;
	bool bLoggedSkeletalMaterialPreservation;
};
