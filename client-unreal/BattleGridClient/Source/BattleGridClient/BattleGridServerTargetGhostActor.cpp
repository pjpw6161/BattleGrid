// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerTargetGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerTargetGhostActor::ABattleGridServerTargetGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	TargetId = 0;
	TargetLocation = FVector::ZeroVector;
	HP = 100;
	MaxHP = 100;
	bAlive = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.8f));

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
	LabelComponent->SetText(FText::FromString(TEXT("Server Target")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	LabelComponent->SetWorldSize(30.0f);
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

	if (MeshComponent)
	{
		MeshComponent->SetRelativeScale3D(bAlive ? FVector(0.8f) : FVector(0.35f));
	}

	if (LabelComponent)
	{
		const FString Label = bAlive
			? FString::Printf(TEXT("T%d HP %d/%d"), TargetId, HP, MaxHP)
			: FString::Printf(TEXT("T%d DEAD"), TargetId);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetTextRenderColor(bAlive ? FColor(255, 165, 0) : FColor::Red);
	}
}

int32 ABattleGridServerTargetGhostActor::GetTargetId() const
{
	return TargetId;
}
