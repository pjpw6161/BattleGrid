// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridDamageableTarget.h"

#include "BattleGridHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridDamageableTarget::ABattleGridDamageableTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(false);
	RootComponent = CollisionComponent;

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	TargetMesh->SetupAttachment(CollisionComponent);
	TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
	if (CubeMesh.Succeeded())
	{
		TargetMesh->SetStaticMesh(CubeMesh.Object);
	}

	HealthComponent = CreateDefaultSubobject<UBattleGridHealthComponent>(TEXT("HealthComponent"));
}

float ABattleGridDamageableTarget::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);

	if (!HealthComponent)
	{
		return AppliedDamage;
	}

	HealthComponent->ApplyDamage(DamageAmount);

	if (HealthComponent->IsDead())
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Target died."));
		Destroy();
	}

	return DamageAmount;
}
