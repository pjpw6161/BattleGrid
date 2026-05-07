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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);
	if (SphereMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereMesh.Object);
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
	const FVector& UnrealDirection
)
{
	ProjectileId = Snapshot.ProjectileId;
	OwnerType = Snapshot.OwnerType.IsEmpty() ? FString(TEXT("player")) : Snapshot.OwnerType;
	TargetLocation = WorldLocation;

	if (MeshComponent)
	{
		MeshComponent->SetRelativeScale3D(FVector(ProjectileScale));
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
		PointLightComponent->SetVisibility(bUsePointLight);
	}

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
