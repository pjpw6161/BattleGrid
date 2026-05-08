// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridServerBotGhostActor.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

ABattleGridServerBotGhostActor::ABattleGridServerBotGhostActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 10.0f;
	AliveScale = 0.8f;
	DeadScale = 0.35f;
	InvincibleScale = 0.9f;
	LabelHeight = 130.0f;
	bUseSkeletalMeshVisual = false;
	BotWeaponSocketName = TEXT("hand_rSocket");
	BotWeaponRelativeLocation = FVector::ZeroVector;
	BotWeaponRelativeRotation = FRotator::ZeroRotator;
	BotWeaponRelativeScale = FVector(1.0f, 1.0f, 1.0f);
	MuzzleSocketName = TEXT("Muzzle");
	MuzzleFallbackOffset = FVector(100.0f, 25.0f, 100.0f);
	bShowMuzzleDebug = false;
	MuzzleDebugSphereRadius = 8.0f;
	HumanoidAliveScale = 1.0f;
	HumanoidDeadScale = 0.35f;
	HumanoidLabelHeight = 190.0f;
	HumanoidMeshRelativeLocation = FVector::ZeroVector;
	HumanoidMeshRelativeRotation = FRotator::ZeroRotator;
	HumanoidMeshRelativeScale3D = FVector(1.0f, 1.0f, 1.0f);
	bFaceMovementDirection = true;
	bUseServerYawWhenNotMoving = true;
	RotationInterpSpeed = 12.0f;
	MovementFacingThreshold = 5.0f;
	MeshForwardYawOffset = 0.0f;
	bUseSimpleBotAnimationPlayback = false;
	BotIdleAnimation = nullptr;
	BotRunAnimation = nullptr;
	BotDeathAnimation = nullptr;
	BotRunSpeedThreshold = 20.0f;
	bShowServerHitVolumes = false;
	DebugHeadSphereRadius = 45.0f;
	DebugBodySphereRadius = 90.0f;
	DebugBodyHeight = 90.0f;
	DebugHeadHeight = 160.0f;
	BotId = 0;
	TargetLocation = FVector::ZeroVector;
	TargetActorYaw = 0.0f;
	ConvertedServerYaw = 0.0f;
	HP = 100;
	MaxHP = 100;
	bAlive = true;
	bInvincible = false;
	DefaultMaterial = nullptr;
	bLoggedMissingWeaponSocketWarning = false;
	bLoggedMissingMuzzleSocketWarning = false;
	bLoggedWeaponAttachment = false;
	bLoggedSkeletalMaterialPreservation = false;
	bLoggedMovementFacing = false;
	PreviousWorldLocation = FVector::ZeroVector;
	bHasPreviousWorldLocation = false;
	VisualSpeed = 0.0f;
	CurrentAnimationState = TEXT("");

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

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetHiddenInGame(true);
	SkeletalMeshComponent->SetVisibility(false);

	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(SceneRoot);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetHiddenInGame(true);
	WeaponMeshComponent->SetVisibility(false);

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

	PreviousWorldLocation = GetActorLocation();
	bHasPreviousWorldLocation = true;
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

	FVector MovementDelta = FVector::ZeroVector;
	if (bHasPreviousWorldLocation)
	{
		MovementDelta = NewLocation - PreviousWorldLocation;
		MovementDelta.Z = 0.0f;
	}

	if (DeltaSeconds > KINDA_SMALL_NUMBER && bHasPreviousWorldLocation)
	{
		VisualSpeed = MovementDelta.Size() / DeltaSeconds;
	}
	else
	{
		VisualSpeed = 0.0f;
	}
	PreviousWorldLocation = NewLocation;
	bHasPreviousWorldLocation = true;

	float DesiredYaw = GetActorRotation().Yaw;
	if (bFaceMovementDirection && MovementDelta.Size() > MovementFacingThreshold)
	{
		DesiredYaw = MovementDelta.Rotation().Yaw;
		if (!bLoggedMovementFacing)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Bot visual facing movement yaw=%.2f"),
				DesiredYaw
			);
			bLoggedMovementFacing = true;
		}
	}
	else if (bUseServerYawWhenNotMoving)
	{
		DesiredYaw = TargetActorYaw;
	}

	const FRotator TargetRotation(0.0f, DesiredYaw, 0.0f);
	SetActorRotation(FMath::RInterpTo(
		GetActorRotation(),
		TargetRotation,
		DeltaSeconds,
		RotationInterpSpeed
	));

	UpdateSimpleAnimationPlayback(DeltaSeconds);

	if (bShowMuzzleDebug)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const FVector MuzzleLocation = GetApproximateMuzzleWorldLocation();
		DrawDebugSphere(
			GetWorld(),
			MuzzleLocation,
			MuzzleDebugSphereRadius,
			12,
			FColor::Orange,
			false,
			0.0f
		);
		DrawDebugLine(
			GetWorld(),
			MuzzleLocation,
			MuzzleLocation + (GetActorForwardVector() * 120.0f),
			FColor::Orange,
			false,
			0.0f,
			0,
			1.5f
		);
#endif
	}

	if (bShowServerHitVolumes)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const FVector BaseLocation = GetActorLocation();
		DrawDebugSphere(
			GetWorld(),
			BaseLocation + FVector(0.0f, 0.0f, DebugBodyHeight),
			DebugBodySphereRadius,
			16,
			FColor::Cyan,
			false,
			0.0f,
			0,
			1.0f
		);
		DrawDebugSphere(
			GetWorld(),
			BaseLocation + FVector(0.0f, 0.0f, DebugHeadHeight),
			DebugHeadSphereRadius,
			16,
			FColor::Magenta,
			false,
			0.0f,
			0,
			1.5f
		);
#endif
	}
}

void ABattleGridServerBotGhostActor::SetSnapshotData(
	const FBattleGridServerBotSnapshot& Snapshot,
	const FVector& WorldLocation,
	float ConvertedServerYawDegrees
)
{
	BotId = Snapshot.BotId;
	Name = Snapshot.Name;
	TargetLocation = WorldLocation;
	ConvertedServerYaw = ConvertedServerYawDegrees;
	TargetActorYaw = ConvertedServerYaw;
	HP = Snapshot.HP;
	MaxHP = Snapshot.MaxHP;
	bAlive = Snapshot.bAlive;
	bInvincible = Snapshot.bInvincible;
	DebugBodySphereRadius = Snapshot.BodyRadius;
	DebugHeadSphereRadius = Snapshot.HeadRadius;
	DebugBodyHeight = Snapshot.BodyHeight;
	DebugHeadHeight = Snapshot.HeadHeight;

	ApplyVisualState(bAlive, bInvincible);

	if (LabelComponent)
	{
		const bool bCanUseSkeletalVisual =
			bUseSkeletalMeshVisual
			&& SkeletalMeshComponent
			&& SkeletalMeshComponent->GetSkeletalMeshAsset();
		const FString DisplayName = Name.IsEmpty()
			? FString::Printf(TEXT("BOT-%d"), BotId)
			: Name;
		const FString Label = !bAlive
			? FString::Printf(TEXT("%s DOWN"), *DisplayName)
			: bInvincible
				? FString::Printf(TEXT("%s INV"), *DisplayName)
				: FString::Printf(TEXT("%s %d/%d"), *DisplayName, HP, MaxHP);
		LabelComponent->SetText(FText::FromString(Label));
		LabelComponent->SetRelativeLocation(FVector(
			0.0f,
			0.0f,
			bCanUseSkeletalVisual ? HumanoidLabelHeight : LabelHeight
		));
		LabelComponent->SetTextRenderColor(
			!bAlive ? FColor::Red : (bInvincible ? FColor::Yellow : FColor::Green)
		);
	}
}

int32 ABattleGridServerBotGhostActor::GetBotId() const
{
	return BotId;
}

FVector ABattleGridServerBotGhostActor::GetApproximateMuzzleWorldLocation() const
{
	if (
		WeaponMeshComponent
		&& MuzzleSocketName != NAME_None
		&& WeaponMeshComponent->DoesSocketExist(MuzzleSocketName)
	)
	{
		return WeaponMeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	if (
		bUseSkeletalMeshVisual
		&& SkeletalMeshComponent
		&& MuzzleSocketName != NAME_None
		&& SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName)
	)
	{
		return SkeletalMeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	if (
		bUseSkeletalMeshVisual
		&& SkeletalMeshComponent
		&& SkeletalMeshComponent->GetSkeletalMeshAsset()
	)
	{
		if (bShowMuzzleDebug && MuzzleSocketName != NAME_None && !bLoggedMissingMuzzleSocketWarning)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BattleGrid] Bot muzzle socket %s not found. Using skeletal muzzle fallback offset."),
				*MuzzleSocketName.ToString()
			);
			bLoggedMissingMuzzleSocketWarning = true;
		}

		return SkeletalMeshComponent->GetComponentTransform().TransformPosition(MuzzleFallbackOffset);
	}

	if (bShowMuzzleDebug && MuzzleSocketName != NAME_None && !bLoggedMissingMuzzleSocketWarning)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BattleGrid] Bot muzzle socket %s not found. Using actor muzzle fallback offset."),
			*MuzzleSocketName.ToString()
		);
		bLoggedMissingMuzzleSocketWarning = true;
	}

	return GetActorTransform().TransformPosition(MuzzleFallbackOffset);
}

void ABattleGridServerBotGhostActor::ApplyVisualState(bool bIsAlive, bool bIsInvincible)
{
	if (!MeshComponent && !SkeletalMeshComponent)
	{
		return;
	}

	UMaterialInterface* DesiredMaterial = AliveMaterial.Get();
	float DesiredScale = AliveScale;
	float DesiredHumanoidScale = HumanoidAliveScale;

	if (!bIsAlive)
	{
		DesiredMaterial = DeadMaterial.Get();
		DesiredScale = DeadScale;
		DesiredHumanoidScale = HumanoidDeadScale;
	}
	else if (bIsInvincible)
	{
		DesiredMaterial = InvincibleMaterial.Get();
		DesiredScale = InvincibleScale;
		DesiredHumanoidScale = HumanoidAliveScale;
	}

	const bool bCanUseSkeletalVisual =
		bUseSkeletalMeshVisual
		&& SkeletalMeshComponent
		&& SkeletalMeshComponent->GetSkeletalMeshAsset();

	if (MeshComponent)
	{
		MeshComponent->SetHiddenInGame(bCanUseSkeletalVisual);
		MeshComponent->SetVisibility(!bCanUseSkeletalVisual);
		if (!bCanUseSkeletalVisual)
		{
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
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetHiddenInGame(!bCanUseSkeletalVisual);
		SkeletalMeshComponent->SetVisibility(bCanUseSkeletalVisual);
		ApplyHumanoidMeshTransform(DesiredHumanoidScale);
		if (bCanUseSkeletalVisual && !bLoggedSkeletalMaterialPreservation)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Bot skeletal visual mode enabled; preserving original skeletal mesh materials.")
			);
			bLoggedSkeletalMaterialPreservation = true;
		}
	}

	if (LabelComponent)
	{
		LabelComponent->SetRelativeLocation(FVector(
			0.0f,
			0.0f,
			bCanUseSkeletalVisual ? HumanoidLabelHeight : LabelHeight
		));
	}

	AttachWeaponToSkeletalMesh();
}

void ABattleGridServerBotGhostActor::ApplyHumanoidMeshTransform(float StateScale)
{
	if (!SkeletalMeshComponent)
	{
		return;
	}

	SkeletalMeshComponent->SetRelativeLocation(HumanoidMeshRelativeLocation);
	FRotator FinalRelativeRotation = HumanoidMeshRelativeRotation;
	FinalRelativeRotation.Yaw += MeshForwardYawOffset;
	SkeletalMeshComponent->SetRelativeRotation(FinalRelativeRotation);
	SkeletalMeshComponent->SetRelativeScale3D(HumanoidMeshRelativeScale3D * StateScale);
}

void ABattleGridServerBotGhostActor::UpdateSimpleAnimationPlayback(float DeltaSeconds)
{
	static_cast<void>(DeltaSeconds);

	const bool bCanUseSkeletalVisual =
		bUseSkeletalMeshVisual
		&& SkeletalMeshComponent
		&& SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!bCanUseSkeletalVisual || !bUseSimpleBotAnimationPlayback)
	{
		return;
	}

	FString DesiredState;
	UAnimationAsset* DesiredAnimation = nullptr;
	bool bLoopAnimation = true;

	if (!bAlive)
	{
		DesiredState = TEXT("Dead");
		DesiredAnimation = BotDeathAnimation.Get();
		bLoopAnimation = false;
	}
	else if (VisualSpeed > BotRunSpeedThreshold)
	{
		DesiredState = TEXT("Run");
		DesiredAnimation = BotRunAnimation.Get();
	}
	else
	{
		DesiredState = TEXT("Idle");
		DesiredAnimation = BotIdleAnimation.Get();
	}

	if (CurrentAnimationState == DesiredState)
	{
		return;
	}

	CurrentAnimationState = DesiredState;
	if (DesiredAnimation)
	{
		SkeletalMeshComponent->PlayAnimation(DesiredAnimation, bLoopAnimation);
	}
}

void ABattleGridServerBotGhostActor::AttachWeaponToSkeletalMesh()
{
	if (!WeaponMeshComponent)
	{
		return;
	}

	const bool bCanUseSkeletalVisual =
		bUseSkeletalMeshVisual
		&& SkeletalMeshComponent
		&& SkeletalMeshComponent->GetSkeletalMeshAsset();
	const bool bHasWeaponMesh = WeaponMeshComponent->GetStaticMesh() != nullptr;
	if (!bCanUseSkeletalVisual)
	{
		WeaponMeshComponent->AttachToComponent(
			SceneRoot,
			FAttachmentTransformRules::KeepRelativeTransform
		);
		WeaponMeshComponent->SetHiddenInGame(true);
		WeaponMeshComponent->SetVisibility(false);
		return;
	}

	const bool bHasSocket = BotWeaponSocketName != NAME_None
		&& SkeletalMeshComponent->DoesSocketExist(BotWeaponSocketName);
	const FName AttachSocketName = bHasSocket ? BotWeaponSocketName : NAME_None;

	if (!bHasSocket && BotWeaponSocketName != NAME_None && !bLoggedMissingWeaponSocketWarning)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BattleGrid] Bot weapon socket %s not found. Attaching bot weapon mesh to skeletal mesh root."),
			*BotWeaponSocketName.ToString()
		);
		bLoggedMissingWeaponSocketWarning = true;
	}

	WeaponMeshComponent->AttachToComponent(
		SkeletalMeshComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName
	);
	WeaponMeshComponent->SetRelativeLocation(BotWeaponRelativeLocation);
	WeaponMeshComponent->SetRelativeRotation(BotWeaponRelativeRotation);
	WeaponMeshComponent->SetRelativeScale3D(BotWeaponRelativeScale);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetHiddenInGame(!bHasWeaponMesh);
	WeaponMeshComponent->SetVisibility(bHasWeaponMesh);

	if (bHasWeaponMesh && !bLoggedWeaponAttachment)
	{
		const FString SocketLogName = bHasSocket
			? BotWeaponSocketName.ToString()
			: FString(TEXT("<skeletal-root>"));
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Bot weapon mesh attached to socket %s"),
			*SocketLogName
		);
		bLoggedWeaponAttachment = true;
	}
}
