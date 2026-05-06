// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerBotGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerBotGhostActor::ABattleGridServerBotGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	BotId = 0;
	TargetLocation = FVector::ZeroVector;
	TargetYaw = 0.0f;
	HP = 100;
	MaxHP = 100;
	bAlive = true;
	bInvincible = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.9f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BotMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);
	if (BotMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(BotMesh.Object);
	}

	LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelComponent"));
	LabelComponent->SetupAttachment(SceneRoot);
	LabelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetTextRenderColor(FColor::Green);
	LabelComponent->SetText(FText::FromString(TEXT("Server Bot")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	LabelComponent->SetWorldSize(30.0f);
}

void ABattleGridServerBotGhostActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector NewLocation = FMath::VInterpTo(
		GetActorLocation(),
		TargetLocation,
		DeltaSeconds,
		InterpSpeed
	);
	SetActorLocation(NewLocation);

	const FRotator TargetRotation(0.0f, TargetYaw, 0.0f);
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		TargetRotation,
		DeltaSeconds,
		InterpSpeed
	));
}

void ABattleGridServerBotGhostActor::SetSnapshotData(
	const FBattleGridServerBotSnapshot& Snapshot,
	const FVector& WorldLocation
)
{
	BotId = Snapshot.BotId;
	Name = Snapshot.Name;
	TargetLocation = WorldLocation;
	TargetYaw = Snapshot.Yaw;
	HP = Snapshot.HP;
	MaxHP = Snapshot.MaxHP;
	bAlive = Snapshot.bAlive;
	bInvincible = Snapshot.bInvincible;

	if (MeshComponent)
	{
		MeshComponent->SetHiddenInGame(!bAlive);
		MeshComponent->SetRelativeScale3D(
			bAlive
				? FVector(0.45f, 0.45f, 0.9f)
				: FVector(0.25f, 0.25f, 0.25f)
		);
	}

	if (LabelComponent)
	{
		const FString DisplayName = Name.IsEmpty()
			? FString::Printf(TEXT("BOT-%d"), BotId)
			: Name;
		const FString Label = !bAlive
			? FString::Printf(TEXT("%s DEAD"), *DisplayName)
			: bInvincible
				? FString::Printf(TEXT("%s INV HP %d/%d"), *DisplayName, HP, MaxHP)
				: FString::Printf(TEXT("%s HP %d/%d"), *DisplayName, HP, MaxHP);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetTextRenderColor(
			!bAlive ? FColor::Red : (bInvincible ? FColor::Yellow : FColor::Green)
		);
	}
}

int32 ABattleGridServerBotGhostActor::GetBotId() const
{
	return BotId;
}
