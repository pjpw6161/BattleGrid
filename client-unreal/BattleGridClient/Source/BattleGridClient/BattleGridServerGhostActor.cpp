// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerGhostActor::ABattleGridServerGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 30.0f;
	bSnapToServerLocation = false;
	PlayerId = 0;
	TargetLocation = FVector::ZeroVector;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);
	if (SphereMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereMesh.Object);
	}

	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelComponent"));
	LabelComponent->SetupAttachment(SceneRoot);
	LabelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetTextRenderColor(FColor::Cyan);
	LabelComponent->SetText(FText::FromString(TEXT("Server")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	LabelComponent->SetWorldSize(32.0f);
}

void ABattleGridServerGhostActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bSnapToServerLocation)
	{
		SetActorLocation(TargetLocation);
		return;
	}

	const FVector NewLocation = FMath::VInterpTo(
		GetActorLocation(),
		TargetLocation,
		DeltaSeconds,
		InterpSpeed
	);
	SetActorLocation(NewLocation);
}

void ABattleGridServerGhostActor::SetSnapshotData(
	const FBattleGridServerPlayerSnapshot& Snapshot,
	const FVector& WorldLocation
)
{
	PlayerId = Snapshot.PlayerId;
	Nickname = Snapshot.Nickname;
	TargetLocation = WorldLocation;

	if (LabelComponent)
	{
		const FString Label = FString::Printf(
			TEXT("P%d %s"),
			PlayerId,
			*Nickname
		);
		LabelComponent->SetText(FText::FromString(Label));
	}
}

int32 ABattleGridServerGhostActor::GetPlayerId() const
{
	return PlayerId;
}
