// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridClientCharacter.h"

#include "BattleGridClientPlayerController.h"
#include "BattleGridHealthComponent.h"
#include "BattleGridWeaponComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
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
	DefaultFOV = 90.0f;
	AdsFOV = 70.0f;
	WeaponSocketName = TEXT("hand_rSocket");
	WeaponRelativeLocation = FVector::ZeroVector;
	WeaponRelativeRotation = FRotator::ZeroRotator;
	WeaponRelativeScale = FVector(1.0f, 1.0f, 1.0f);
	bADSActive = false;
	bSprinting = false;

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
	bLoggedWeaponAttachment = false;
}

void ABattleGridClientCharacter::BeginPlay()
{
	Super::BeginPlay();

	RespawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();
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
