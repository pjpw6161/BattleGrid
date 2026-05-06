// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerBotGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerBotGhostActor::ABattleGridServerBotGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	AliveScale = 0.8f;
	DeadScale = 0.35f;
	InvincibleScale = 0.9f;
	LabelHeight = 130.0f;
	BotId = 0;
	TargetLocation = FVector::ZeroVector;
	TargetYaw = 0.0f;
	HP = 100;
	MaxHP = 100;
	bAlive = true;
	bInvincible = false;
	DefaultMaterial = nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(AliveScale, AliveScale, AliveScale * 1.5f));

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
	LabelComponent->SetText(FText::FromString(TEXT("BOT")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
	LabelComponent->SetWorldSize(30.0f);
}

void ABattleGridServerBotGhostActor::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent)
	{
		DefaultMaterial = MeshComponent->GetMaterial(0);
	}
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

	ApplyVisualState(bAlive, bInvincible);

	if (LabelComponent)
	{
		const FString DisplayName = Name.IsEmpty()
			? FString::Printf(TEXT("BOT-%d"), BotId)
			: Name;
		const FString Label = !bAlive
			? FString::Printf(TEXT("%s DOWN"), *DisplayName)
			: bInvincible
				? FString::Printf(TEXT("%s INV"), *DisplayName)
				: FString::Printf(TEXT("%s %d/%d"), *DisplayName, HP, MaxHP);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
		LabelComponent->SetTextRenderColor(
			!bAlive ? FColor::Red : (bInvincible ? FColor::Yellow : FColor::Green)
		);
	}
}

int32 ABattleGridServerBotGhostActor::GetBotId() const
{
	return BotId;
}

void ABattleGridServerBotGhostActor::ApplyVisualState(bool bIsAlive, bool bIsInvincible)
{
	if (!MeshComponent)
	{
		return;
	}

	UMaterialInterface* DesiredMaterial = AliveMaterial.Get();
	float DesiredScale = AliveScale;

	if (!bIsAlive)
	{
		DesiredMaterial = DeadMaterial.Get();
		DesiredScale = DeadScale;
	}
	else if (bIsInvincible)
	{
		DesiredMaterial = InvincibleMaterial.Get();
		DesiredScale = InvincibleScale;
	}

	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetRelativeScale3D(FVector(DesiredScale, DesiredScale, DesiredScale * 1.5f));
	if (DesiredMaterial)
	{
		MeshComponent->SetMaterial(0, DesiredMaterial);
	}
	else if (DefaultMaterial)
	{
		MeshComponent->SetMaterial(0, DefaultMaterial.Get());
	}
}
