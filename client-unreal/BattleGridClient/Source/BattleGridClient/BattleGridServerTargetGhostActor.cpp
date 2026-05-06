// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerTargetGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerTargetGhostActor::ABattleGridServerTargetGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	AliveScale = 1.0f;
	DeadScale = 0.35f;
	LabelHeight = 140.0f;
	TargetId = 0;
	TargetLocation = FVector::ZeroVector;
	HP = 100;
	MaxHP = 100;
	bAlive = true;
	DefaultMaterial = nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(AliveScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
	}

	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelComponent"));
	LabelComponent->SetupAttachment(SceneRoot);
	LabelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetTextRenderColor(FColor(255, 165, 0));
	LabelComponent->SetText(FText::FromString(TEXT("CORE")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
	LabelComponent->SetWorldSize(30.0f);
}

void ABattleGridServerTargetGhostActor::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent)
	{
		DefaultMaterial = MeshComponent->GetMaterial(0);
	}
}

void ABattleGridServerTargetGhostActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector NewLocation = FMath::VInterpTo(
		GetActorLocation(),
		TargetLocation,
		DeltaSeconds,
		InterpSpeed
	);
	SetActorLocation(NewLocation);
}

void ABattleGridServerTargetGhostActor::SetSnapshotData(
	const FBattleGridServerTargetSnapshot& Snapshot,
	const FVector& WorldLocation
)
{
	TargetId = Snapshot.TargetId;
	HP = Snapshot.HP;
	MaxHP = Snapshot.MaxHP;
	bAlive = Snapshot.bAlive;
	TargetLocation = WorldLocation;

	ApplyVisualState(bAlive);

	if (LabelComponent)
	{
		const FString Label = bAlive
			? FString::Printf(TEXT("CORE-%d %d/%d"), TargetId, HP, MaxHP)
			: FString::Printf(TEXT("CORE-%d DESTROYED"), TargetId);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
		LabelComponent->SetTextRenderColor(bAlive ? FColor(255, 165, 0) : FColor::Red);
	}
}

int32 ABattleGridServerTargetGhostActor::GetTargetId() const
{
	return TargetId;
}

void ABattleGridServerTargetGhostActor::ApplyVisualState(bool bIsAlive)
{
	if (!MeshComponent)
	{
		return;
	}

	UMaterialInterface* DesiredMaterial = bIsAlive ? AliveMaterial.Get() : DeadMaterial.Get();
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetRelativeScale3D(FVector(bIsAlive ? AliveScale : DeadScale));
	if (DesiredMaterial)
	{
		MeshComponent->SetMaterial(0, DesiredMaterial);
	}
	else if (DefaultMaterial)
	{
		MeshComponent->SetMaterial(0, DefaultMaterial.Get());
	}
}
