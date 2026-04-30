// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridDamageableTarget.h"

#include "BattleGridClientPlayerController.h"
#include "BattleGridHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
ABattleGridClientPlayerController* FindBattleGridController(
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(EventInstigator))
	{
		return BattleGridController;
	}

	if (!DamageCauser)
	{
		return nullptr;
	}

	if (ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(DamageCauser))
	{
		return BattleGridController;
	}

	if (APawn* DamageCauserPawn = Cast<APawn>(DamageCauser))
	{
		if (ABattleGridClientPlayerController* BattleGridController =
			Cast<ABattleGridClientPlayerController>(DamageCauserPawn->GetController()))
		{
			return BattleGridController;
		}
	}

	if (AController* DamageInstigator = DamageCauser->GetInstigatorController())
	{
		if (ABattleGridClientPlayerController* BattleGridController =
			Cast<ABattleGridClientPlayerController>(DamageInstigator))
		{
			return BattleGridController;
		}
	}

	AActor* DamageCauserOwner = DamageCauser->GetOwner();
	if (ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(DamageCauserOwner))
	{
		return BattleGridController;
	}

	if (APawn* OwnerPawn = Cast<APawn>(DamageCauserOwner))
	{
		return Cast<ABattleGridClientPlayerController>(OwnerPawn->GetController());
	}

	return nullptr;
}
}

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
		if (ABattleGridClientPlayerController* BattleGridController =
			FindBattleGridController(EventInstigator, DamageCauser))
		{
			BattleGridController->AddScore(1);
			BattleGridController->SetCombatMessage(TEXT("Target eliminated! +1 Score"));
		}

		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Target died."));
		Destroy();
	}

	return DamageAmount;
}
