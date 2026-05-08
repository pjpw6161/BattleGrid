// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridClientCharacter.h"

#include "BattleGridClientPlayerController.h"
#include "BattleGridHealthComponent.h"
#include "BattleGridWeaponComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimationAsset.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ABattleGridClientCharacter::ABattleGridClientCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	HealthComponent = CreateDefaultSubobject<UBattleGridHealthComponent>(TEXT("HealthComponent"));
	WeaponComponent = CreateDefaultSubobject<UBattleGridWeaponComponent>(TEXT("WeaponComponent"));
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(GetMesh());
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	DefaultArmLength = 450.0f;
	AdsArmLength = 300.0f;
	DefaultSocketOffset = FVector(0.0f, 60.0f, 60.0f);
	AdsSocketOffset = FVector(0.0f, 90.0f, 50.0f);
	CameraInterpSpeed = 12.0f;
	CameraLagSpeed = 12.0f;
	PlayerCameraCollisionProbeSize = 12.0f;
	DefaultFOV = 90.0f;
	AdsFOV = 70.0f;
	WeaponSocketName = TEXT("hand_rSocket");
	WeaponRelativeLocation = FVector::ZeroVector;
	WeaponRelativeRotation = FRotator::ZeroRotator;
	WeaponRelativeScale = FVector(1.0f, 1.0f, 1.0f);
	MuzzleSocketName = TEXT("Muzzle");
	MuzzleFallbackOffset = FVector(100.0f, 25.0f, 90.0f);
	bShowMuzzleDebug = false;
	MuzzleDebugSphereRadius = 8.0f;
	PlayerMeshRelativeLocation = FVector(0.0f, 0.0f, -90.0f);
	PlayerMeshRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
	PlayerMeshRelativeScale3D = FVector(1.0f, 1.0f, 1.0f);
	bUseSimplePlayerAnimationPlayback = false;
	PlayerIdleAnimation = nullptr;
	PlayerRunAnimation = nullptr;
	PlayerJumpAnimation = nullptr;
	PlayerJumpStartAnimation = nullptr;
	PlayerJumpLoopAnimation = nullptr;
	PlayerJumpLandAnimation = nullptr;
	bLoopPlayerJumpLoopAnimation = false;
	PlayerLandAnimationLockSeconds = 0.25f;
	PlayerRunSpeedThreshold = 20.0f;
	bADSActive = false;
	bSprinting = false;
	LastPlayerWorldLocation = FVector::ZeroVector;
	PlayerVisualSpeed = 0.0f;
	CurrentPlayerVisualAnimState = NAME_None;
	bWasPlayerFalling = false;
	bPlayerJumpStartPlayed = false;
	bPlayerJumpLoopPlayed = false;
	bPlayerLandPlaying = false;
	PlayerLandLockTimer = 0.0f;

	ApplyPlayerMeshVisualSettings();

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = false;
	GetCharacterMovement()->bSnapToPlaneAtStart = false;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.35f;

	// Create the camera boom component
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(false);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = true;

	// Create the camera component
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	FollowCamera = TopDownCameraComponent;

	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	ApplyCameraSettings(0.0f);

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bIsDead = false;
	RespawnDelaySeconds = 3.0f;
	bLoggedMissingWeaponSocketWarning = false;
	bLoggedMissingMuzzleSocketWarning = false;
	bLoggedWeaponAttachment = false;
}

void ABattleGridClientCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPlayerMeshVisualSettings();
	AttachWeaponToCharacterMesh();
	ApplyCameraSettings(0.0f);
}

void ABattleGridClientCharacter::BeginPlay()
{
	Super::BeginPlay();

	RespawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();
	LastPlayerWorldLocation = GetActorLocation();
	CurrentPlayerVisualAnimState = NAME_None;
	bWasPlayerFalling = false;
	bPlayerJumpStartPlayed = false;
	bPlayerJumpLoopPlayed = false;
	bPlayerLandPlaying = false;
	PlayerLandLockTimer = 0.0f;
	ApplyPlayerMeshVisualSettings();
	AttachWeaponToCharacterMesh();

	if (HealthComponent)
	{
		HealthComponent->ResetHealth();
	}

	SyncHealthToPlayerController();
}

void ABattleGridClientCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyCameraSettings(DeltaSeconds);
	UpdateSimplePlayerAnimation(DeltaSeconds);

	if (bShowMuzzleDebug)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const FVector MuzzleLocation = GetApproximateMuzzleWorldLocation();
		DrawDebugSphere(
			GetWorld(),
			MuzzleLocation,
			MuzzleDebugSphereRadius,
			12,
			FColor::Cyan,
			false,
			0.0f
		);
		DrawDebugLine(
			GetWorld(),
			MuzzleLocation,
			MuzzleLocation + (GetActorForwardVector() * 120.0f),
			FColor::Cyan,
			false,
			0.0f,
			0,
			1.5f
		);
#endif
	}
}

float ABattleGridClientCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Character TakeDamage called. Damage=%.1f"), DamageAmount);

	if (bIsDead || !HealthComponent)
	{
		return 0.0f;
	}

	HealthComponent->ApplyDamage(DamageAmount);
	SyncHealthToPlayerController();

	if (HealthComponent->IsDead())
	{
		HandleDeath();
	}

	return DamageAmount;
}

void ABattleGridClientCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	SetAimingDownSights(false);
	SetSprinting(false);

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Player died."));

	if (ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(GetController()))
	{
		BattleGridController->SetPlayerDead(true);
		BattleGridController->SetCombatMessage(TEXT("You died! Respawning..."), RespawnDelaySeconds);
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DisableMovement();
		MovementComponent->StopMovementImmediately();
	}

	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&ABattleGridClientCharacter::Respawn,
		RespawnDelaySeconds,
		false
	);
}

void ABattleGridClientCharacter::Respawn()
{
	SetActorLocationAndRotation(RespawnLocation, RespawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (HealthComponent)
	{
		HealthComponent->ResetHealth();
	}

	bIsDead = false;
	SetAimingDownSights(false);
	SetSprinting(false);

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	SyncHealthToPlayerController();

	if (ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(GetController()))
	{
		BattleGridController->SetPlayerDead(false);
		BattleGridController->SetCombatMessage(TEXT("Respawned!"), 2.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Player respawned."));
}

void ABattleGridClientCharacter::SyncHealthToPlayerController() const
{
	if (!HealthComponent)
	{
		return;
	}

	if (ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(GetController()))
	{
		BattleGridController->SetPlayerHealth(
			HealthComponent->GetCurrentHealth(),
			HealthComponent->GetMaxHealth()
		);
	}
}

void ABattleGridClientCharacter::SetADSActive(bool bNewADSActive)
{
	SetAimingDownSights(bNewADSActive);
}

void ABattleGridClientCharacter::SetAimingDownSights(bool bInADS)
{
	bADSActive = bInADS;
}

void ABattleGridClientCharacter::SetSprinting(bool bInSprinting)
{
	bSprinting = bInSprinting;
}

void ABattleGridClientCharacter::ApplyPlayerMeshVisualSettings()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	CharacterMesh->SetRelativeLocation(PlayerMeshRelativeLocation);
	CharacterMesh->SetRelativeRotation(PlayerMeshRelativeRotation);
	CharacterMesh->SetRelativeScale3D(PlayerMeshRelativeScale3D);
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABattleGridClientCharacter::UpdateSimplePlayerAnimation(float DeltaSeconds)
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !bUseSimplePlayerAnimationPlayback)
	{
		CurrentPlayerVisualAnimState = NAME_None;
		LastPlayerWorldLocation = GetActorLocation();
		PlayerVisualSpeed = 0.0f;
		bWasPlayerFalling = false;
		bPlayerJumpStartPlayed = false;
		bPlayerJumpLoopPlayed = false;
		bPlayerLandPlaying = false;
		PlayerLandLockTimer = 0.0f;
		return;
	}

	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	const FVector CurrentLocation = GetActorLocation();
	FVector MovementDelta = CurrentLocation - LastPlayerWorldLocation;
	MovementDelta.Z = 0.0f;
	PlayerVisualSpeed = DeltaSeconds > KINDA_SMALL_NUMBER
		? MovementDelta.Size() / DeltaSeconds
		: 0.0f;
	LastPlayerWorldLocation = CurrentLocation;

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const bool bIsFalling = MovementComponent && MovementComponent->IsFalling();
	UAnimationAsset* JumpLoopAnimation = PlayerJumpLoopAnimation.Get()
		? PlayerJumpLoopAnimation.Get()
		: PlayerJumpAnimation.Get();

	if (!bIsFalling && bWasPlayerFalling)
	{
		bWasPlayerFalling = false;
		bPlayerJumpStartPlayed = false;
		bPlayerJumpLoopPlayed = false;
		if (PlayerJumpLandAnimation)
		{
			bPlayerLandPlaying = true;
			PlayerLandLockTimer = PlayerLandAnimationLockSeconds;
			PlaySimplePlayerAnimation(PlayerJumpLandAnimation.Get(), TEXT("JumpLand"), false);
			return;
		}
	}

	if (bPlayerLandPlaying)
	{
		PlayerLandLockTimer -= DeltaSeconds;
		if (PlayerLandLockTimer > 0.0f)
		{
			return;
		}
		bPlayerLandPlaying = false;
		PlayerLandLockTimer = 0.0f;
	}

	if (bIsFalling && !bWasPlayerFalling)
	{
		bWasPlayerFalling = true;
		bPlayerLandPlaying = false;
		PlayerLandLockTimer = 0.0f;
		bPlayerJumpStartPlayed = false;
		bPlayerJumpLoopPlayed = false;

		if (PlayerJumpStartAnimation)
		{
			bPlayerJumpStartPlayed = true;
			PlaySimplePlayerAnimation(PlayerJumpStartAnimation.Get(), TEXT("JumpStart"), false);
			return;
		}

		bPlayerJumpStartPlayed = true;
		bPlayerJumpLoopPlayed = true;
		if (JumpLoopAnimation)
		{
			PlaySimplePlayerAnimation(JumpLoopAnimation, TEXT("JumpLoop"), bLoopPlayerJumpLoopAnimation);
		}
		return;
	}

	if (bIsFalling)
	{
		bWasPlayerFalling = true;
		if (!bPlayerJumpLoopPlayed)
		{
			bPlayerJumpLoopPlayed = true;
			if (JumpLoopAnimation)
			{
				PlaySimplePlayerAnimation(JumpLoopAnimation, TEXT("JumpLoop"), bLoopPlayerJumpLoopAnimation);
			}
		}
		return;
	}

	bPlayerJumpStartPlayed = false;
	bPlayerJumpLoopPlayed = false;

	if (PlayerVisualSpeed > PlayerRunSpeedThreshold && PlayerRunAnimation)
	{
		PlaySimplePlayerAnimation(PlayerRunAnimation.Get(), TEXT("Run"), true);
		return;
	}

	if (PlayerIdleAnimation)
	{
		PlaySimplePlayerAnimation(PlayerIdleAnimation.Get(), TEXT("Idle"), true);
	}
}

bool ABattleGridClientCharacter::PlaySimplePlayerAnimation(
	UAnimationAsset* Animation,
	FName StateName,
	bool bLoopAnimation
)
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !Animation || StateName == NAME_None)
	{
		return false;
	}

	if (CurrentPlayerVisualAnimState == StateName)
	{
		return false;
	}

	CurrentPlayerVisualAnimState = StateName;
	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	CharacterMesh->PlayAnimation(Animation, bLoopAnimation);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] Player anim state %s"),
		*StateName.ToString()
	);
	return true;
}

void ABattleGridClientCharacter::AttachWeaponToCharacterMesh()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !WeaponMeshComponent)
	{
		return;
	}

	const bool bHasSocket = WeaponSocketName != NAME_None
		&& CharacterMesh->DoesSocketExist(WeaponSocketName);
	const FName AttachSocketName = bHasSocket ? WeaponSocketName : NAME_None;

	if (!bHasSocket && WeaponSocketName != NAME_None && !bLoggedMissingWeaponSocketWarning)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BattleGrid] Weapon socket %s not found. Attaching weapon mesh to character mesh root."),
			*WeaponSocketName.ToString()
		);
		bLoggedMissingWeaponSocketWarning = true;
	}

	WeaponMeshComponent->AttachToComponent(
		CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName
	);
	WeaponMeshComponent->SetRelativeLocation(WeaponRelativeLocation);
	WeaponMeshComponent->SetRelativeRotation(WeaponRelativeRotation);
	WeaponMeshComponent->SetRelativeScale3D(WeaponRelativeScale);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const bool bHasWeaponMesh = WeaponMeshComponent->GetStaticMesh() != nullptr;
	WeaponMeshComponent->SetHiddenInGame(!bHasWeaponMesh);
	WeaponMeshComponent->SetVisibility(bHasWeaponMesh);

	if (bHasWeaponMesh && !bLoggedWeaponAttachment)
	{
		const FString SocketLogName = bHasSocket
			? WeaponSocketName.ToString()
			: FString(TEXT("<mesh-root>"));
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Weapon mesh attached to socket %s"),
			*SocketLogName
		);
		bLoggedWeaponAttachment = true;
	}
}

FVector ABattleGridClientCharacter::GetApproximateMuzzleWorldLocation() const
{
	if (
		WeaponMeshComponent
		&& MuzzleSocketName != NAME_None
		&& WeaponMeshComponent->DoesSocketExist(MuzzleSocketName)
	)
	{
		return WeaponMeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	const USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (
		CharacterMesh
		&& MuzzleSocketName != NAME_None
		&& CharacterMesh->DoesSocketExist(MuzzleSocketName)
	)
	{
		return CharacterMesh->GetSocketLocation(MuzzleSocketName);
	}

	if (bShowMuzzleDebug && MuzzleSocketName != NAME_None && !bLoggedMissingMuzzleSocketWarning)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BattleGrid] Muzzle socket %s not found. Using player muzzle fallback offset."),
			*MuzzleSocketName.ToString()
		);
		bLoggedMissingMuzzleSocketWarning = true;
	}

	return GetActorTransform().TransformPosition(MuzzleFallbackOffset);
}

void ABattleGridClientCharacter::ApplyCameraSettings(float DeltaSeconds)
{
	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	const float TargetArmLength = bADSActive ? AdsArmLength : DefaultArmLength;
	const FVector TargetSocketOffset = bADSActive ? AdsSocketOffset : DefaultSocketOffset;
	const float TargetFOV = bADSActive ? AdsFOV : DefaultFOV;
	const float BlendSpeed = FMath::Max(0.0f, CameraInterpSpeed);

	CameraBoom->SetUsingAbsoluteRotation(false);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->SetRelativeLocation(FVector::ZeroVector);
	CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
	CameraBoom->CameraLagSpeed = FMath::Max(0.0f, CameraLagSpeed);
	CameraBoom->ProbeSize = FMath::Max(0.0f, PlayerCameraCollisionProbeSize);
	FollowCamera->bUsePawnControlRotation = false;

	if (DeltaSeconds <= 0.0f || BlendSpeed <= 0.0f)
	{
		CameraBoom->TargetArmLength = TargetArmLength;
		CameraBoom->SocketOffset = TargetSocketOffset;
		FollowCamera->SetFieldOfView(TargetFOV);
		return;
	}

	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		TargetArmLength,
		DeltaSeconds,
		BlendSpeed
	);
	CameraBoom->SocketOffset = FMath::VInterpTo(
		CameraBoom->SocketOffset,
		TargetSocketOffset,
		DeltaSeconds,
		BlendSpeed
	);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(
		FollowCamera->FieldOfView,
		TargetFOV,
		DeltaSeconds,
		BlendSpeed
	));
}
