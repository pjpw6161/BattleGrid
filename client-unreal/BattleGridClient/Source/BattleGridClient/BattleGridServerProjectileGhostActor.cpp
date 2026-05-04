// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerProjectileGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerProjectileGhostActor::ABattleGridServerProjectileGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 20.0f;
	ProjectileId = 0;
	TargetLocation = FVector::ZeroVector;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.3f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);
	if (SphereMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereMesh.Object);
	}
}

void ABattleGridServerProjectileGhostActor::Tick(float DeltaSeconds)
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

void ABattleGridServerProjectileGhostActor::SetSnapshotData(
	const FBattleGridServerProjectileSnapshot& Snapshot,
	const FVector& WorldLocation,
	const FVector& UnrealDirection
)
{
	ProjectileId = Snapshot.ProjectileId;
	TargetLocation = WorldLocation;

	FVector FlatDirection(UnrealDirection.X, UnrealDirection.Y, 0.0f);
	if (FlatDirection.Normalize())
	{
		SetActorRotation(FlatDirection.Rotation());
	}
}

int32 ABattleGridServerProjectileGhostActor::GetProjectileId() const
{
	return ProjectileId;
}
