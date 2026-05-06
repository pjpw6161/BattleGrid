// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerGhostActor::ABattleGridServerGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 30.0f;
	bSnapToServerLocation = false;
	AliveScale = 0.6f;
	DeadScale = 0.35f;
	InvincibleScale = 0.75f;
	LabelHeight = 120.0f;
	PlayerId = 0;
	TargetLocation = FVector::ZeroVector;
	DefaultMaterial = nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(AliveScale));

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
	LabelComponent->SetText(FText::FromString(TEXT("SERVER ECHO")));
	LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
	LabelComponent->SetWorldSize(32.0f);
}

void ABattleGridServerGhostActor::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent)
	{
		DefaultMaterial = MeshComponent->GetMaterial(0);
	}
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
	ApplyVisualState(Snapshot.bAlive, Snapshot.bInvincible);

	if (LabelComponent)
	{
		const FString StatusText = !Snapshot.bAlive
			? FString(TEXT(" DOWN"))
			: (Snapshot.bInvincible ? FString(TEXT(" INV")) : FString());
		const FString Label = FString::Printf(
			TEXT("SERVER ECHO P%d%s\n%s"),
			PlayerId,
			*StatusText,
			*Nickname
		);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, LabelHeight));
		LabelComponent->SetTextRenderColor(
			!Snapshot.bAlive ? FColor::Red : (Snapshot.bInvincible ? FColor::Yellow : FColor::Cyan)
		);
	}
}

int32 ABattleGridServerGhostActor::GetPlayerId() const
{
	return PlayerId;
}

void ABattleGridServerGhostActor::ApplyVisualState(bool bIsAlive, bool bIsInvincible)
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
	MeshComponent->SetRelativeScale3D(FVector(DesiredScale));
	if (DesiredMaterial)
	{
		MeshComponent->SetMaterial(0, DesiredMaterial);
	}
	else if (DefaultMaterial)
	{
		MeshComponent->SetMaterial(0, DefaultMaterial.Get());
	}
}
