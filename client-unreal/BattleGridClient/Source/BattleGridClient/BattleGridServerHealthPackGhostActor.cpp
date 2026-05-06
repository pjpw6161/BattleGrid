// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerHealthPackGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerHealthPackGhostActor::ABattleGridServerHealthPackGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	ActiveScale = 0.7f;
	InactiveScale = 0.35f;
	LabelHeight = 100.0f;
	HealthPackId = 0;
	TargetLocation = FVector::ZeroVector;
	bActive = true;
	HealAmount = 35;
	RespawnTimer = 0.0f;
	DefaultMaterial = nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(ActiveScale));

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
	LabelComponent->SetText(FText::FromString(TEXT("HPACK")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
	LabelComponent->SetWorldSize(28.0f);
}

void ABattleGridServerHealthPackGhostActor::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent)
	{
		DefaultMaterial = MeshComponent->GetMaterial(0);
	}
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

	ApplyVisualState(bActive);

	if (LabelComponent)
	{
		const FString Label = bActive
			? FString::Printf(TEXT("HPACK-%d +%d"), HealthPackId, HealAmount)
			: FString::Printf(TEXT("HPACK-%d %.0fs"), HealthPackId, RespawnTimer);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
		LabelComponent->SetTextRenderColor(bActive ? FColor::Cyan : FColor(180, 180, 180));
	}
}

int32 ABattleGridServerHealthPackGhostActor::GetHealthPackId() const
{
	return HealthPackId;
}

void ABattleGridServerHealthPackGhostActor::ApplyVisualState(bool bIsActive)
{
	if (!MeshComponent)
	{
		return;
	}

	UMaterialInterface* DesiredMaterial = bIsActive ? ActiveMaterial.Get() : InactiveMaterial.Get();
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetRelativeScale3D(FVector(bIsActive ? ActiveScale : InactiveScale));
	if (DesiredMaterial)
	{
		MeshComponent->SetMaterial(0, DesiredMaterial);
	}
	else if (DefaultMaterial)
	{
		MeshComponent->SetMaterial(0, DefaultMaterial.Get());
	}
}
