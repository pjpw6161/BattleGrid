// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridHazardActor.h"

#include "BattleGridClientCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridHazardActor::ABattleGridHazardActor()
{
	PrimaryActorTick.bCanEverTick = true;

	DamageAmount = 25.0f;
	DamageCooldownSeconds = 1.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DamageVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageVolume"));
	DamageVolume->SetupAttachment(SceneRoot);
	DamageVolume->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
	ConfigureDamageVolumeCollision();

	HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
	HazardMesh->SetupAttachment(SceneRoot);
	HazardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HazardMesh->SetRelativeScale3D(FVector(2.0f, 2.0f, 0.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
	if (CubeMesh.Succeeded())
	{
		HazardMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void ABattleGridHazardActor::BeginPlay()
{
	Super::BeginPlay();

	ConfigureDamageVolumeCollision();

	if (DamageVolume)
	{
		DamageVolume->UpdateOverlaps();
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Hazard BeginPlay: %s"), *GetName());
}

void ABattleGridHazardActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (auto ActorIterator = OverlappingActors.CreateIterator(); ActorIterator; ++ActorIterator)
	{
		TWeakObjectPtr<AActor> ActorPtr = *ActorIterator;
		AActor* OverlappingActor = ActorPtr.Get();

		if (!IsValid(OverlappingActor))
		{
			LastDamageTimes.Remove(ActorPtr);
			ActorIterator.RemoveCurrent();
			continue;
		}

		TryDamageActor(OverlappingActor);
	}
}

void ABattleGridHazardActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Hazard Actor BeginOverlap: %s"), *GetNameSafe(OtherActor));
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] OtherActor class: %s"),
		OtherActor ? *GetNameSafe(OtherActor->GetClass()) : TEXT("None")
	);

	ABattleGridClientCharacter* Character = Cast<ABattleGridClientCharacter>(OtherActor);
	if (Character)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Hazard overlap actor is BattleGridClientCharacter."));
		OverlappingActors.Add(OtherActor);
		TryDamageActor(OtherActor);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Hazard overlap actor is not ABattleGridClientCharacter."));
		TryDamageActor(OtherActor);
	}
}

void ABattleGridHazardActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Hazard Actor EndOverlap: %s"), *GetNameSafe(OtherActor));

	const TWeakObjectPtr<AActor> ActorPtr(OtherActor);
	OverlappingActors.Remove(ActorPtr);
	LastDamageTimes.Remove(ActorPtr);
}

void ABattleGridHazardActor::ConfigureDamageVolumeCollision() const
{
	if (!DamageVolume)
	{
		return;
	}

	DamageVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageVolume->SetCollisionObjectType(ECC_WorldDynamic);
	DamageVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageVolume->SetGenerateOverlapEvents(true);
	DamageVolume->CanCharacterStepUpOn = ECB_No;
}

void ABattleGridHazardActor::TryDamageActor(AActor* ActorToDamage)
{
	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] TryDamageActor: %s"), *GetNameSafe(ActorToDamage));

	ABattleGridClientCharacter* Character = Cast<ABattleGridClientCharacter>(ActorToDamage);
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] TryDamageActor cast failed: not ABattleGridClientCharacter."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] TryDamageActor cast succeeded: BattleGridClientCharacter."));

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float CooldownSeconds = FMath::Max(0.0f, DamageCooldownSeconds);
	const TWeakObjectPtr<AActor> ActorPtr(ActorToDamage);

	if (const float* LastDamageTime = LastDamageTimes.Find(ActorPtr))
	{
		if (CurrentTime - *LastDamageTime < CooldownSeconds)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Hazard damage skipped due to cooldown.")
			);
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Hazard Tick damage attempt: %s"), *GetNameSafe(ActorToDamage));

	UGameplayStatics::ApplyDamage(Character, DamageAmount, nullptr, this, nullptr);

	LastDamageTimes.Add(ActorPtr, CurrentTime);

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Hazard damaged player."));
}
