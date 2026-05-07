// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerProjectileGhostActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerProjectileGhostActor::ABattleGridServerProjectileGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 20.0f;
	ProjectileScale = 0.25f;
	bUseTracerLineVisual = true;
	TracerThickness = 0.08f;
	TracerLengthScale = 1.0f;
	bUsePointLight = true;
	PointLightIntensity = 350.0f;
	PointLightRadius = 160.0f;
	ProjectileId = 0;
	OwnerType = TEXT("player");
	TargetLocation = FVector::ZeroVector;
	DefaultMaterial = nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(ProjectileScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TracerMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
	if (TracerMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(TracerMesh.Object);
	}

	PointLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLightComponent"));
	PointLightComponent->SetupAttachment(SceneRoot);
	PointLightComponent->SetIntensity(PointLightIntensity);
	PointLightComponent->SetAttenuationRadius(PointLightRadius);
	PointLightComponent->SetLightColor(FLinearColor(0.4f, 0.8f, 1.0f));
	PointLightComponent->SetVisibility(bUsePointLight);
}

void ABattleGridServerProjectileGhostActor::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent)
	{
		DefaultMaterial = MeshComponent->GetMaterial(0);
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
	const FVector& UnrealDirection,
	const FVector& WorldStartLocation,
	const FVector& WorldEndLocation
)
{
	ProjectileId = Snapshot.ProjectileId;
	OwnerType = Snapshot.OwnerType.IsEmpty() ? FString(TEXT("player")) : Snapshot.OwnerType;

	const FVector TracerDelta = WorldEndLocation - WorldStartLocation;
	const float TracerLength = TracerDelta.Size();
	const bool bCanUseTracerLine = bUseTracerLineVisual && TracerLength > KINDA_SMALL_NUMBER;
	TargetLocation = bCanUseTracerLine
		? (WorldStartLocation + (TracerDelta * 0.5f))
		: WorldLocation;

	if (MeshComponent)
	{
		if (bCanUseTracerLine)
		{
			constexpr float BasicShapeLength = 100.0f;
			MeshComponent->SetRelativeScale3D(FVector(
				FMath::Max(0.01f, (TracerLength / BasicShapeLength) * TracerLengthScale),
				TracerThickness,
				TracerThickness
			));
		}
		else
		{
			MeshComponent->SetRelativeScale3D(FVector(ProjectileScale));
		}

		UMaterialInterface* MaterialToApply = nullptr;
		if (OwnerType.Equals(TEXT("bot"), ESearchCase::IgnoreCase) && BotProjectileMaterial)
		{
			MaterialToApply = BotProjectileMaterial.Get();
		}
		else if (OwnerType.Equals(TEXT("player"), ESearchCase::IgnoreCase) && PlayerProjectileMaterial)
		{
			MaterialToApply = PlayerProjectileMaterial.Get();
		}
		else if (AliveMaterial)
		{
			MaterialToApply = AliveMaterial.Get();
		}

		if (MaterialToApply)
		{
			MeshComponent->SetMaterial(0, MaterialToApply);
		}
		else if (DefaultMaterial)
		{
			MeshComponent->SetMaterial(0, DefaultMaterial.Get());
		}
	}

	if (PointLightComponent)
	{
		PointLightComponent->SetIntensity(PointLightIntensity);
		PointLightComponent->SetAttenuationRadius(PointLightRadius);
		PointLightComponent->SetLightColor(
			OwnerType.Equals(TEXT("bot"), ESearchCase::IgnoreCase)
				? FLinearColor(1.0f, 0.45f, 0.1f)
				: FLinearColor(0.4f, 0.8f, 1.0f)
		);
		PointLightComponent->SetVisibility(bUsePointLight);
	}

	if (bCanUseTracerLine)
	{
		SetActorLocation(TargetLocation);
		SetActorRotation(TracerDelta.Rotation());
		return;
	}

	FVector Direction = UnrealDirection;
	if (Direction.Normalize())
	{
		SetActorRotation(Direction.Rotation());
	}
}

int32 ABattleGridServerProjectileGhostActor::GetProjectileId() const
{
	return ProjectileId;
}
