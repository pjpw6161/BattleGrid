// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerHealthPackGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerHealthPackGhostActor::ABattleGridServerHealthPackGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	HealthPackId = 0;
	TargetLocation = FVector::ZeroVector;
	bActive = true;
	HealAmount = 35;
	RespawnTimer = 0.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HealthPackMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
	if (HealthPackMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(HealthPackMesh.Object);
	}

	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelComponent"));
	LabelComponent->SetupAttachment(SceneRoot);
	LabelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetTextRenderColor(FColor::Cyan);
	LabelComponent->SetText(FText::FromString(TEXT("Server Health Pack")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	LabelComponent->SetWorldSize(28.0f);
}

void ABattleGridServerHealthPackGhostActor::Tick(float DeltaSeconds)
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

void ABattleGridServerHealthPackGhostActor::SetSnapshotData(
	const FBattleGridServerHealthPackSnapshot& Snapshot,
	const FVector& WorldLocation
)
{
	HealthPackId = Snapshot.HealthPackId;
	TargetLocation = WorldLocation;
	bActive = Snapshot.bActive;
	HealAmount = Snapshot.HealAmount;
	RespawnTimer = Snapshot.RespawnTimer;

	if (MeshComponent)
	{
		MeshComponent->SetHiddenInGame(!bActive);
		MeshComponent->SetRelativeScale3D(
			bActive
				? FVector(0.35f, 0.35f, 0.35f)
				: FVector(0.18f, 0.18f, 0.18f)
		);
	}

	if (LabelComponent)
	{
		const FString Label = bActive
			? FString::Printf(TEXT("HPACK-%d +%d"), HealthPackId, HealAmount)
			: FString::Printf(TEXT("HPACK-%d Respawn %.1f"), HealthPackId, RespawnTimer);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetTextRenderColor(bActive ? FColor::Cyan : FColor(180, 180, 180));
	}
}

int32 ABattleGridServerHealthPackGhostActor::GetHealthPackId() const
{
	return HealthPackId;
}
