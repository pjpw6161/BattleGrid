// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridClientPlayerController.h"

#include "BattleGridNetworkSubsystem.h"
#include "BattleGridProjectile.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

ABattleGridClientPlayerController::ABattleGridClientPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	PrimaryActorTick.bCanEverTick = true;

	ProjectileSpawnDistance = 120.0f;
	FireCooldownSeconds = 0.2f;
	LastFireTime = -100000.0f;

	MaxPlayerHealth = 100.0f;
	CurrentPlayerHealth = 100.0f;
	Score = 0;
	TargetScore = 5;
	bAutoConnectToServer = true;
	ServerUrl = TEXT("ws://127.0.0.1:7777");
	Nickname = TEXT("player1");
	InputSendIntervalSeconds = 0.05f;
	CombatMessageExpireTime = 0.0f;
	bPlayerDead = false;
	bHasWon = false;
	CurrentMoveForward = 0.0f;
	CurrentMoveRight = 0.0f;
	bPendingFireInput = false;
	InputSequence = 0;
	LastInputSendTime = 0.0f;
	LastSentMoveForward = 0.0f;
	LastSentMoveRight = 0.0f;
	LastSentAimX = 0.0f;
	LastSentAimY = 0.0f;
	bHasLastSentInput = false;
}

float ABattleGridClientPlayerController::GetMaxPlayerHealth() const
{
	return MaxPlayerHealth;
}

float ABattleGridClientPlayerController::GetCurrentPlayerHealth() const
{
	return CurrentPlayerHealth;
}

int32 ABattleGridClientPlayerController::GetScore() const
{
	return Score;
}

int32 ABattleGridClientPlayerController::GetTargetScore() const
{
	return FMath::Max(1, TargetScore);
}

FString ABattleGridClientPlayerController::GetCombatMessage() const
{
	return CombatMessage;
}

bool ABattleGridClientPlayerController::HasActiveCombatMessage() const
{
	const UWorld* World = GetWorld();

	return World
		&& !CombatMessage.IsEmpty()
		&& World->GetTimeSeconds() < CombatMessageExpireTime;
}

bool ABattleGridClientPlayerController::HasWon() const
{
	return bHasWon;
}

bool ABattleGridClientPlayerController::IsPlayerDead() const
{
	return bPlayerDead;
}

void ABattleGridClientPlayerController::AddScore(int32 Amount)
{
	if (bHasWon)
	{
		return;
	}

	Score += Amount;

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Score: %d / %d"), Score, GetTargetScore());

	if (Score >= GetTargetScore())
	{
		bHasWon = true;
		SetCombatMessage(TEXT("Victory! Press R to Restart"), 3600.0f);
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Victory!"));
	}
}

void ABattleGridClientPlayerController::SetCombatMessage(const FString& Message, float DurationSeconds)
{
	CombatMessage = Message;

	if (const UWorld* World = GetWorld())
	{
		CombatMessageExpireTime = World->GetTimeSeconds() + FMath::Max(0.0f, DurationSeconds);
	}
	else
	{
		CombatMessageExpireTime = 0.0f;
	}
}

void ABattleGridClientPlayerController::SetPlayerHealth(float Current, float Max)
{
	CurrentPlayerHealth = FMath::Max(0.0f, Current);
	MaxPlayerHealth = FMath::Max(0.0f, Max);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] SetPlayerHealth: %.1f / %.1f"),
		CurrentPlayerHealth,
		MaxPlayerHealth
	);
}

void ABattleGridClientPlayerController::SetPlayerDead(bool bDead)
{
	bPlayerDead = bDead;
}

void ABattleGridClientPlayerController::RestartGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (CurrentLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Cannot restart level: current level name is empty."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Restarting level."));
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName));
}

void ABattleGridClientPlayerController::RestartStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	RestartGame();
}

FString ABattleGridClientPlayerController::GetNetworkStatusText() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetConnectionStatusText();
		}
	}

	return TEXT("Server: Disconnected");
}

bool ABattleGridClientPlayerController::IsServerConnected() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->IsConnected();
		}
	}

	return false;
}

bool ABattleGridClientPlayerController::HasJoinedServer() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->HasJoined();
		}
	}

	return false;
}

int32 ABattleGridClientPlayerController::GetServerPlayerId() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetPlayerId();
		}
	}

	return 0;
}

int32 ABattleGridClientPlayerController::GetServerRoomId() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetRoomId();
		}
	}

	return 0;
}

void ABattleGridClientPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->ClearAllMappings();

			if (BattleGridMappingContext)
			{
				Subsystem->AddMappingContext(BattleGridMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Input Mapping Context added."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] BattleGridMappingContext is not assigned."));
			}
		}
	}

	if (bAutoConnectToServer)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UBattleGridNetworkSubsystem* NetworkSubsystem =
				GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
			{
				UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Connecting to server: %s"), *ServerUrl);
				NetworkSubsystem->Connect(ServerUrl, Nickname);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] BattleGridNetworkSubsystem is missing."));
			}
		}
	}
}

void ABattleGridClientPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[BattleGrid] EnhancedInputComponent is missing."));
		return;
	}

	if (MoveForwardAction)
	{
		EnhancedInputComponent->BindAction(
			MoveForwardAction,
			ETriggerEvent::Triggered,
			this,
			&ABattleGridClientPlayerController::MoveForward
		);
		EnhancedInputComponent->BindAction(
			MoveForwardAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::StopMoveForward
		);
		EnhancedInputComponent->BindAction(
			MoveForwardAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::StopMoveForward
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] MoveForwardAction is not assigned."));
	}

	if (MoveRightAction)
	{
		EnhancedInputComponent->BindAction(
			MoveRightAction,
			ETriggerEvent::Triggered,
			this,
			&ABattleGridClientPlayerController::MoveRight
		);
		EnhancedInputComponent->BindAction(
			MoveRightAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::StopMoveRight
		);
		EnhancedInputComponent->BindAction(
			MoveRightAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::StopMoveRight
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] MoveRightAction is not assigned."));
	}

	if (FireAction)
	{
		EnhancedInputComponent->BindAction(
			FireAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::FireStarted
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] FireAction is not assigned."));
	}

	if (RestartAction)
	{
		EnhancedInputComponent->BindAction(
			RestartAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::RestartStarted
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] RestartAction is not assigned."));
	}
}

void ABattleGridClientPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateAimRotation();
	SendInputToServerIfNeeded();
}

void ABattleGridClientPlayerController::MoveForward(const FInputActionValue& Value)
{
	if (IsPlayerDead() || HasWon())
	{
		CurrentMoveForward = 0.0f;
		return;
	}

	const float AxisValue = Value.Get<float>();
	CurrentMoveForward = AxisValue;

	if (FMath::IsNearlyZero(AxisValue))
	{
		CurrentMoveForward = 0.0f;
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(FVector::ForwardVector, AxisValue);
	}
}

void ABattleGridClientPlayerController::MoveRight(const FInputActionValue& Value)
{
	if (IsPlayerDead() || HasWon())
	{
		CurrentMoveRight = 0.0f;
		return;
	}

	const float AxisValue = Value.Get<float>();
	CurrentMoveRight = AxisValue;

	if (FMath::IsNearlyZero(AxisValue))
	{
		CurrentMoveRight = 0.0f;
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(FVector::RightVector, AxisValue);
	}
}

void ABattleGridClientPlayerController::StopMoveForward(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	CurrentMoveForward = 0.0f;
}

void ABattleGridClientPlayerController::StopMoveRight(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	CurrentMoveRight = 0.0f;
}

void ABattleGridClientPlayerController::FireStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (IsPlayerDead() || HasWon())
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();

	if (!World || !ControlledPawn)
	{
		return;
	}

	bPendingFireInput = true;

	const float CurrentTime = World->GetTimeSeconds();
	const float CooldownSeconds = FMath::Max(0.0f, FireCooldownSeconds);

	if (CurrentTime - LastFireTime < CooldownSeconds)
	{
		return;
	}

	FVector FireDirection = ControlledPawn->GetActorForwardVector();
	FireDirection.Z = 0.0f;
	FireDirection.Normalize();

	if (FireDirection.IsNearlyZero())
	{
		FireDirection = ControlledPawn->GetActorRotation().Vector();
	}

	const FVector ProjectileSpawnLocation =
		ControlledPawn->GetActorLocation()
		+ FireDirection * ProjectileSpawnDistance
		+ FVector(0.0f, 0.0f, 50.0f);
	const FRotator SpawnRotation = FireDirection.Rotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = ControlledPawn;
	SpawnParameters.Instigator = ControlledPawn;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	TSubclassOf<ABattleGridProjectile> ProjectileClassToSpawn = ProjectileClass;
	if (!ProjectileClassToSpawn)
	{
		ProjectileClassToSpawn = ABattleGridProjectile::StaticClass();
	}

	if (World->SpawnActor<ABattleGridProjectile>(
		ProjectileClassToSpawn,
		ProjectileSpawnLocation,
		SpawnRotation,
		SpawnParameters
	))
	{
		LastFireTime = CurrentTime;
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Projectile fired."));
	}
}

void ABattleGridClientPlayerController::UpdateAimRotation()
{
	if (IsPlayerDead())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn)
	{
		return;
	}

	FHitResult HitResult;
	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (!bHit)
	{
		return;
	}

	FVector Direction = HitResult.ImpactPoint - ControlledPawn->GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.SizeSquared() < 1.0f)
	{
		return;
	}

	const FRotator AimRotation = Direction.Rotation();
	ControlledPawn->SetActorRotation(FRotator(0.0f, AimRotation.Yaw, 0.0f));
}

void ABattleGridClientPlayerController::SendInputToServerIfNeeded()
{
	if (IsPlayerDead() || HasWon())
	{
		CurrentMoveForward = 0.0f;
		CurrentMoveRight = 0.0f;
		bPendingFireInput = false;
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float SendInterval = FMath::Max(0.0f, InputSendIntervalSeconds);
	if (CurrentTime - LastInputSendTime < SendInterval)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UBattleGridNetworkSubsystem* NetworkSubsystem =
		GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>();
	if (!NetworkSubsystem || !NetworkSubsystem->IsConnected() || !NetworkSubsystem->HasJoined())
	{
		bPendingFireInput = false;
		return;
	}

	const bool bHasMovementInput =
		!FMath::IsNearlyZero(CurrentMoveForward)
		|| !FMath::IsNearlyZero(CurrentMoveRight);

	float AimX = 0.0f;
	float AimY = 0.0f;
	if (const APawn* ControlledPawn = GetPawn())
	{
		FVector AimDirection = ControlledPawn->GetActorForwardVector();
		AimDirection.Z = 0.0f;
		if (!AimDirection.Normalize())
		{
			AimDirection = FVector::ForwardVector;
		}

		AimX = AimDirection.X;
		AimY = AimDirection.Y;
	}

	constexpr float InputChangeThreshold = 0.01f;
	const bool bMovementChanged =
		!bHasLastSentInput
		|| FMath::Abs(CurrentMoveForward - LastSentMoveForward) > InputChangeThreshold
		|| FMath::Abs(CurrentMoveRight - LastSentMoveRight) > InputChangeThreshold;
	const bool bAimChanged =
		!bHasLastSentInput
		|| FMath::Abs(AimX - LastSentAimX) > InputChangeThreshold
		|| FMath::Abs(AimY - LastSentAimY) > InputChangeThreshold;
	if (!bHasMovementInput && !bPendingFireInput && !bMovementChanged && !bAimChanged)
	{
		return;
	}

	const bool bFire = bPendingFireInput;
	++InputSequence;
	LastInputSendTime = CurrentTime;

	NetworkSubsystem->SendInput(
		InputSequence,
		CurrentMoveRight,
		CurrentMoveForward,
		AimX,
		AimY,
		bFire
	);

	LastSentMoveForward = CurrentMoveForward;
	LastSentMoveRight = CurrentMoveRight;
	LastSentAimX = AimX;
	LastSentAimY = AimY;
	bHasLastSentInput = true;

	if (bFire)
	{
		bPendingFireInput = false;
	}
}
