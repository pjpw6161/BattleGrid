// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridClientPlayerController.h"

#include "BattleGridClientCharacter.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridProjectile.h"
#include "BattleGridServerGhostActor.h"
#include "BattleGridServerProjectileGhostActor.h"
#include "BattleGridServerTargetGhostActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

ABattleGridClientPlayerController::ABattleGridClientPlayerController()
{
	bShowMouseCursor = false;
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
	bUseRemoteServer = false;
	LocalServerUrl = TEXT("ws://127.0.0.1:7777");
	RemoteServerUrl = TEXT("");
	ServerProfileLabel = TEXT("Local");
	Nickname = TEXT("player1");
	InputSendIntervalSeconds = 0.05f;
	NormalMoveSpeed = 600.0f;
	SprintMoveSpeed = 850.0f;
	ADSMoveSpeed = 400.0f;
	LookYawSensitivity = 1.0f;
	LookPitchSensitivity = -1.0f;
	ViewPitchMin = -55.0f;
	ViewPitchMax = 35.0f;
	bDemoMode = true;
	bVerboseNetworkLogs = false;
	bVerboseSnapshotLogs = false;
	bVerboseInputLogs = false;
	SnapshotLogInterval = 60;
	InputAckLogInterval = 60;
	CombatMessageExpireTime = 0.0f;
	bPlayerDead = false;
	bHasWon = false;
	CurrentMoveForward = 0.0f;
	CurrentMoveRight = 0.0f;
	bADSInputHeld = false;
	bIsADSActive = false;
	bIsSprinting = false;
	bPendingFireInput = false;
	InputSequence = 0;
	LastInputSendTime = 0.0f;
	LastSentMoveForward = 0.0f;
	LastSentMoveRight = 0.0f;
	LastSentAimX = 0.0f;
	LastSentAimY = 0.0f;
	bHasLastSentInput = false;
	bShowServerGhosts = true;
	bShowOwnServerGhost = true;
	ServerToUnrealScale = 1.0f;
	ServerGhostHeight = 100.0f;
	bShowServerPositionError = true;
	bUseServerPositionCorrection = false;
	ServerCorrectionStrength = 8.0f;
	ServerCorrectionSnapDistance = 500.0f;
	bShowServerProjectileGhosts = true;
	ServerProjectileGhostHeight = 80.0f;
	ServerAimSignX = 1.0f;
	ServerAimSignY = 1.0f;
	bShowServerTargetGhosts = true;
	ServerTargetGhostHeight = 60.0f;
	ServerSnapshotOrigin = FVector::ZeroVector;
	bServerSnapshotOriginInitialized = false;
	LastProcessedSnapshotTick = 0;
	LastServerPositionError = 0.0f;
	LastOwnServerWorldLocation = FVector::ZeroVector;
	bHasOwnServerWorldLocation = false;
	LastServerPositionErrorLogSnapshotTick = 0;
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

	if (bPlayerDead)
	{
		bADSInputHeld = false;
		bIsADSActive = false;
		bIsSprinting = false;
		ApplyMovementAndADSState();
	}
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

FString ABattleGridClientPlayerController::ResolveServerUrl() const
{
	if (bUseRemoteServer && !RemoteServerUrl.IsEmpty())
	{
		return RemoteServerUrl;
	}

	if (!LocalServerUrl.IsEmpty())
	{
		return LocalServerUrl;
	}

	if (!ServerUrl.IsEmpty())
	{
		return ServerUrl;
	}

	return TEXT("ws://127.0.0.1:7777");
}

FString ABattleGridClientPlayerController::GetServerProfileText() const
{
	return FString::Printf(
		TEXT("Profile=%s URL=%s"),
		*ResolveServerProfileLabel(),
		*ResolveServerUrl()
	);
}

FString ABattleGridClientPlayerController::GetNetworkStatusText() const
{
	return GetDetailedNetworkStatusText();
}

FString ABattleGridClientPlayerController::GetDetailedNetworkStatusText() const
{
	const FString ProfileLabel = ResolveServerProfileLabel();
	const FString CorrectionText = IsUsingServerPositionCorrection()
		? FString(TEXT("On"))
		: FString(TEXT("Off"));
	const FString ErrorText = bShowServerPositionError && HasOwnServerWorldLocation()
		? FString::Printf(TEXT("%.1f"), GetLastServerPositionError())
		: FString(TEXT("-"));
	const FString ADSStatusText = bIsADSActive ? FString(TEXT("On")) : FString(TEXT("Off"));
	const FString SprintStatusText = bIsSprinting ? FString(TEXT("On")) : FString(TEXT("Off"));

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			if (NetworkSubsystem->HasJoined())
			{
				return FString::Printf(
					TEXT("Profile: %s | Server: Connected | Player=%d Room=%d Snapshot=%d\nTargets: %d/%d | Projectiles: %d | Error: %s | Correction: %s | ADS: %s | Sprint: %s"),
					*ProfileLabel,
					NetworkSubsystem->GetPlayerId(),
					NetworkSubsystem->GetRoomId(),
					NetworkSubsystem->GetLastSnapshotTick(),
					NetworkSubsystem->GetServerAliveTargetCount(),
					NetworkSubsystem->GetServerTargetCount(),
					NetworkSubsystem->GetServerProjectileCount(),
					*ErrorText,
					*CorrectionText,
					*ADSStatusText,
					*SprintStatusText
				);
			}

			if (NetworkSubsystem->IsConnected())
			{
				return FString::Printf(
					TEXT("Profile: %s | Server: Connected | Joining...\nTargets: - | Projectiles: - | Error: %s | Correction: %s | ADS: %s | Sprint: %s"),
					*ProfileLabel,
					*ErrorText,
					*CorrectionText,
					*ADSStatusText,
					*SprintStatusText
				);
			}

			const FString LastNetworkError = NetworkSubsystem->GetLastError();
			if (!LastNetworkError.IsEmpty())
			{
				return FString::Printf(
					TEXT("Profile: %s | Server: Error | %s\nTargets: - | Projectiles: - | Error: %s | Correction: %s | ADS: %s | Sprint: %s"),
					*ProfileLabel,
					*LastNetworkError,
					*ErrorText,
					*CorrectionText,
					*ADSStatusText,
					*SprintStatusText
				);
			}
		}
	}

	return FString::Printf(
		TEXT("Profile: %s | Server: Disconnected\nTargets: - | Projectiles: - | Error: %s | Correction: %s | ADS: %s | Sprint: %s"),
		*ProfileLabel,
		*ErrorText,
		*CorrectionText,
		*ADSStatusText,
		*SprintStatusText
	);
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

int32 ABattleGridClientPlayerController::GetOwnServerScore() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetOwnServerScore();
		}
	}

	return 0;
}

int32 ABattleGridClientPlayerController::GetServerAliveTargetCount() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetServerAliveTargetCount();
		}
	}

	return 0;
}

int32 ABattleGridClientPlayerController::GetServerTargetCount() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetServerTargetCount();
		}
	}

	return 0;
}

int32 ABattleGridClientPlayerController::GetServerProjectileCount() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetServerProjectileCount();
		}
	}

	return 0;
}

float ABattleGridClientPlayerController::GetLastServerPositionError() const
{
	return LastServerPositionError;
}

bool ABattleGridClientPlayerController::HasOwnServerWorldLocation() const
{
	return bHasOwnServerWorldLocation;
}

FVector ABattleGridClientPlayerController::GetLastOwnServerWorldLocation() const
{
	return LastOwnServerWorldLocation;
}

bool ABattleGridClientPlayerController::IsUsingServerPositionCorrection() const
{
	return bUseServerPositionCorrection;
}

bool ABattleGridClientPlayerController::IsAimingDownSights() const
{
	return bIsADSActive;
}

bool ABattleGridClientPlayerController::IsSprinting() const
{
	return bIsSprinting;
}

void ABattleGridClientPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = FMath::Min(ViewPitchMin, ViewPitchMax);
		PlayerCameraManager->ViewPitchMax = FMath::Max(ViewPitchMin, ViewPitchMax);
	}

	if (const APawn* ControlledPawn = GetPawn())
	{
		ServerSnapshotOrigin = ControlledPawn->GetActorLocation();
		SetControlRotation(ControlledPawn->GetActorRotation());
	}
	else
	{
		ServerSnapshotOrigin = FVector::ZeroVector;
	}
	bServerSnapshotOriginInitialized = true;
	ApplyMovementAndADSState();

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
				ServerProfileLabel = ResolveServerProfileLabel();
				const FString ResolvedServerUrl = ResolveServerUrl();
				ServerUrl = ResolvedServerUrl;

				UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Server profile: %s"), *ServerProfileLabel);
				UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Connecting to server: %s"), *ResolvedServerUrl);
				const bool bUseVerboseDefaults = !bDemoMode;
				NetworkSubsystem->ConfigureDemoLogging(
					bVerboseNetworkLogs || bUseVerboseDefaults,
					bVerboseSnapshotLogs || bUseVerboseDefaults,
					bVerboseInputLogs || bUseVerboseDefaults,
					SnapshotLogInterval,
					InputAckLogInterval
				);
				NetworkSubsystem->Connect(ResolvedServerUrl, Nickname);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] BattleGridNetworkSubsystem is missing."));
			}
		}
	}
}

FString ABattleGridClientPlayerController::ResolveServerProfileLabel() const
{
	return bUseRemoteServer && !RemoteServerUrl.IsEmpty()
		? FString(TEXT("Remote"))
		: FString(TEXT("Local"));
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
			&ABattleGridClientPlayerController::MoveForwardReleased
		);
		EnhancedInputComponent->BindAction(
			MoveForwardAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::MoveForwardReleased
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
			&ABattleGridClientPlayerController::MoveRightReleased
		);
		EnhancedInputComponent->BindAction(
			MoveRightAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::MoveRightReleased
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] MoveRightAction is not assigned."));
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&ABattleGridClientPlayerController::Look
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] LookAction is not assigned."));
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

	if (AdsAction)
	{
		EnhancedInputComponent->BindAction(
			AdsAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::AdsStarted
		);
		EnhancedInputComponent->BindAction(
			AdsAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::AdsEnded
		);
		EnhancedInputComponent->BindAction(
			AdsAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::AdsEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] AdsAction is not assigned."));
	}

	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::SprintStarted
		);
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::SprintEnded
		);
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::SprintEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] SprintAction is not assigned."));
	}

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::JumpStarted
		);
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::JumpEnded
		);
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::JumpEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] JumpAction is not assigned."));
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

	ApplyMovementAndADSState();
	UpdateAimRotation();
	SendInputToServerIfNeeded();
	UpdateServerGhostsFromSnapshot();
	UpdateServerProjectileGhostsFromSnapshot();
	UpdateServerTargetGhostsFromSnapshot();
	UpdateOwnServerPositionErrorAndCorrection(DeltaTime);
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
		const FRotator ControlYawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDirection =
			FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::X);
		ControlledPawn->AddMovementInput(ForwardDirection, AxisValue);
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
		const FRotator ControlYawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
		const FVector RightDirection =
			FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::Y);
		ControlledPawn->AddMovementInput(RightDirection, AxisValue);
	}
}

void ABattleGridClientPlayerController::MoveForwardReleased(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	CurrentMoveForward = 0.0f;
	if (bVerboseInputLogs || !bDemoMode)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] MoveForward released."));
	}
	SendInputToServer(true);
}

void ABattleGridClientPlayerController::MoveRightReleased(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	CurrentMoveRight = 0.0f;
	if (bVerboseInputLogs || !bDemoMode)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] MoveRight released."));
	}
	SendInputToServer(true);
}

void ABattleGridClientPlayerController::Look(const FInputActionValue& Value)
{
	if (IsPlayerDead() || HasWon())
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	if (LookAxis.IsNearlyZero())
	{
		return;
	}

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = FMath::Min(ViewPitchMin, ViewPitchMax);
		PlayerCameraManager->ViewPitchMax = FMath::Max(ViewPitchMin, ViewPitchMax);
	}

	AddYawInput(LookAxis.X * LookYawSensitivity);
	AddPitchInput(LookAxis.Y * LookPitchSensitivity);
}

void ABattleGridClientPlayerController::AdsStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (IsPlayerDead() || HasWon())
	{
		return;
	}

	bADSInputHeld = true;
	ApplyMovementAndADSState();
}

void ABattleGridClientPlayerController::AdsEnded(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	bADSInputHeld = false;
	ApplyMovementAndADSState();
}

void ABattleGridClientPlayerController::SprintStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (IsPlayerDead() || HasWon())
	{
		return;
	}

	bIsSprinting = true;
	bADSInputHeld = false;
	ApplyMovementAndADSState();
}

void ABattleGridClientPlayerController::SprintEnded(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	bIsSprinting = false;
	ApplyMovementAndADSState();
}

void ABattleGridClientPlayerController::JumpStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (IsPlayerDead() || HasWon())
	{
		return;
	}

	bADSInputHeld = false;
	ApplyMovementAndADSState();

	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		ControlledCharacter->Jump();
	}
}

void ABattleGridClientPlayerController::JumpEnded(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		ControlledCharacter->StopJumping();
	}
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

	UpdateAimRotation();

	bPendingFireInput = true;
	SendInputToServer(true);

	const float CurrentTime = World->GetTimeSeconds();
	const float CooldownSeconds = FMath::Max(0.0f, FireCooldownSeconds);

	if (CurrentTime - LastFireTime < CooldownSeconds)
	{
		return;
	}

	const FRotator FireYawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	FVector FireDirection = FireYawRotation.Vector();
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

	const FRotator CurrentControlRotation = GetControlRotation();
	ControlledPawn->SetActorRotation(FRotator(0.0f, CurrentControlRotation.Yaw, 0.0f));
}

void ABattleGridClientPlayerController::ApplyMovementAndADSState()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (IsControlledPawnFalling())
	{
		bADSInputHeld = false;
	}

	const bool bCanADS =
		bADSInputHeld
		&& !bIsSprinting
		&& !IsControlledPawnFalling()
		&& !IsPlayerDead()
		&& !HasWon();
	bIsADSActive = bCanADS;

	if (ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(ControlledPawn))
	{
		BattleGridCharacter->SetAimingDownSights(bIsADSActive);
		BattleGridCharacter->SetSprinting(bIsSprinting);
	}

	if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (UCharacterMovementComponent* MovementComponent =
			ControlledCharacter->GetCharacterMovement())
		{
			float DesiredSpeed = NormalMoveSpeed;
			if (bIsSprinting && !bIsADSActive)
			{
				DesiredSpeed = SprintMoveSpeed;
			}
			else if (bIsADSActive)
			{
				DesiredSpeed = ADSMoveSpeed;
			}

			MovementComponent->MaxWalkSpeed = FMath::Max(0.0f, DesiredSpeed);
		}
	}
}

bool ABattleGridClientPlayerController::IsControlledPawnFalling() const
{
	const ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn());
	if (!ControlledCharacter)
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent =
		ControlledCharacter->GetCharacterMovement();
	return MovementComponent && MovementComponent->IsFalling();
}

FVector ABattleGridClientPlayerController::GetCameraRelativeMovementDirection(
	float ForwardAxis,
	float RightAxis
) const
{
	const FRotator ControlYawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection =
		FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection =
		FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::Y);
	FVector MovementDirection =
		ForwardDirection * ForwardAxis
		+ RightDirection * RightAxis;
	MovementDirection.Z = 0.0f;

	if (!MovementDirection.Normalize())
	{
		return FVector::ZeroVector;
	}

	return MovementDirection;
}

void ABattleGridClientPlayerController::SendInputToServerIfNeeded()
{
	SendInputToServer(false);
}

void ABattleGridClientPlayerController::SendInputToServer(bool bForceSend)
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
	if (!bForceSend && CurrentTime - LastInputSendTime < SendInterval)
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
	const float MovementInputMagnitude = FMath::Clamp(
		FVector2D(CurrentMoveForward, CurrentMoveRight).Size(),
		0.0f,
		1.0f
	);
	float ServerMoveX = 0.0f;
	float ServerMoveY = 0.0f;
	if (bHasMovementInput)
	{
		const FVector UnrealMovementDirection = GetCameraRelativeMovementDirection(
			CurrentMoveForward,
			CurrentMoveRight
		);
		const FVector2D ServerMovementDirection =
			ConvertUnrealDirectionToServerDirection(UnrealMovementDirection);
		ServerMoveX = ServerMovementDirection.X * MovementInputMagnitude;
		ServerMoveY = ServerMovementDirection.Y * MovementInputMagnitude;
	}

	float AimX = 0.0f;
	float AimY = 0.0f;
	FVector UnrealAimDirection = FVector::ForwardVector;
	const FRotator AimYawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	UnrealAimDirection = AimYawRotation.Vector();
	UnrealAimDirection.Z = 0.0f;
	if (!UnrealAimDirection.Normalize())
	{
		UnrealAimDirection = FVector::ForwardVector;
	}

	const FVector2D ServerAimDirection =
		ConvertUnrealDirectionToServerDirection(UnrealAimDirection);
	AimX = ServerAimDirection.X;
	AimY = ServerAimDirection.Y;

	constexpr float InputChangeThreshold = 0.01f;
	const bool bMovementChanged =
		!bHasLastSentInput
		|| FMath::Abs(ServerMoveY - LastSentMoveForward) > InputChangeThreshold
		|| FMath::Abs(ServerMoveX - LastSentMoveRight) > InputChangeThreshold;
	const bool bAimChanged =
		!bHasLastSentInput
		|| FMath::Abs(AimX - LastSentAimX) > InputChangeThreshold
		|| FMath::Abs(AimY - LastSentAimY) > InputChangeThreshold;
	if (
		!bForceSend
		&& !bHasMovementInput
		&& !bPendingFireInput
		&& !bMovementChanged
		&& !bAimChanged
	)
	{
		return;
	}

	const bool bFire = bPendingFireInput;
	++InputSequence;
	LastInputSendTime = CurrentTime;

	const int32 InputLogInterval = FMath::Max(1, InputAckLogInterval);
	if ((bVerboseInputLogs || !bDemoMode) && (bFire || InputSequence % InputLogInterval == 0))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Aim convert unreal=(%.2f,%.2f) server=(%.2f,%.2f)"),
			UnrealAimDirection.X,
			UnrealAimDirection.Y,
			AimX,
			AimY
		);
	}

	NetworkSubsystem->SendInput(
		InputSequence,
		ServerMoveX,
		ServerMoveY,
		AimX,
		AimY,
		bFire
	);

	LastSentMoveForward = ServerMoveY;
	LastSentMoveRight = ServerMoveX;
	LastSentAimX = AimX;
	LastSentAimY = AimY;
	bHasLastSentInput = true;

	if (bFire)
	{
		bPendingFireInput = false;
	}
}

void ABattleGridClientPlayerController::UpdateServerGhostsFromSnapshot()
{
	if (!bShowServerGhosts)
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (!World || !GameInstance)
	{
		return;
	}

	UBattleGridNetworkSubsystem* NetworkSubsystem =
		GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>();
	if (!NetworkSubsystem || !NetworkSubsystem->HasSnapshot())
	{
		return;
	}

	const int32 SnapshotTick = NetworkSubsystem->GetLastSnapshotTick();
	if (SnapshotTick <= LastProcessedSnapshotTick)
	{
		return;
	}

	if (!bServerSnapshotOriginInitialized)
	{
		if (const APawn* ControlledPawn = GetPawn())
		{
			ServerSnapshotOrigin = ControlledPawn->GetActorLocation();
		}
		else
		{
			ServerSnapshotOrigin = FVector::ZeroVector;
		}
		bServerSnapshotOriginInitialized = true;
	}

	if (LastProcessedSnapshotTick == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Processing snapshot tick=%d"), SnapshotTick);
	}

	TArray<FBattleGridServerPlayerSnapshot> PlayerSnapshots;
	NetworkSubsystem->GetLatestPlayerSnapshots(PlayerSnapshots);

	const int32 LocalServerPlayerId = NetworkSubsystem->GetPlayerId();
	for (const FBattleGridServerPlayerSnapshot& PlayerSnapshot : PlayerSnapshots)
	{
		if (!bShowOwnServerGhost && PlayerSnapshot.PlayerId == LocalServerPlayerId)
		{
			continue;
		}

		const FVector WorldLocation = ConvertServerPositionToWorld(
			PlayerSnapshot.X,
			PlayerSnapshot.Y
		);
		const FVector WorldOffset = WorldLocation - ServerSnapshotOrigin;

		const int32 EffectiveSnapshotLogInterval = FMath::Max(1, SnapshotLogInterval);
		if (
			(bVerboseSnapshotLogs || !bDemoMode)
			&& (SnapshotTick <= 5 || SnapshotTick % EffectiveSnapshotLogInterval == 0)
		)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] ServerToWorld player_id=%d server=(%.1f,%.1f) world=(%.1f,%.1f)"),
				PlayerSnapshot.PlayerId,
				PlayerSnapshot.X,
				PlayerSnapshot.Y,
				WorldOffset.X,
				WorldOffset.Y
			);
		}

		TObjectPtr<ABattleGridServerGhostActor>& GhostActor =
			ServerGhostActors.FindOrAdd(PlayerSnapshot.PlayerId);
		if (!GhostActor)
		{
			TSubclassOf<ABattleGridServerGhostActor> GhostClass = ServerGhostActorClass;
			if (!GhostClass)
			{
				GhostClass = ABattleGridServerGhostActor::StaticClass();
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GhostActor = World->SpawnActor<ABattleGridServerGhostActor>(
				GhostClass,
				WorldLocation,
				FRotator::ZeroRotator,
				SpawnParameters
			);

			if (GhostActor)
			{
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[BattleGrid] Spawned server ghost player_id=%d"),
					PlayerSnapshot.PlayerId
				);
			}
		}

		if (GhostActor)
		{
			GhostActor->SetSnapshotData(PlayerSnapshot, WorldLocation);
		}
	}

	LastProcessedSnapshotTick = SnapshotTick;
}

FVector ABattleGridClientPlayerController::ConvertServerPositionToWorld(
	float ServerX,
	float ServerY
) const
{
	return ConvertServerPositionToWorld(ServerX, ServerY, ServerGhostHeight);
}

FVector ABattleGridClientPlayerController::ConvertServerPositionToWorld(
	float ServerX,
	float ServerY,
	float WorldHeight
) const
{
	// The server uses logical 2D arena coordinates; this top-down template/camera
	// orientation needs X/Y swapped for ghost visualization.
	const float WorldX = ServerY * ServerToUnrealScale;
	const float WorldY = ServerX * ServerToUnrealScale;

	return ServerSnapshotOrigin + FVector(WorldX, WorldY, WorldHeight);
}

FVector2D ABattleGridClientPlayerController::ConvertUnrealDirectionToServerDirection(
	const FVector& UnrealForward
) const
{
	FVector NormalizedUnrealForward(UnrealForward.X, UnrealForward.Y, 0.0f);
	if (!NormalizedUnrealForward.Normalize())
	{
		NormalizedUnrealForward = FVector::ForwardVector;
	}

	FVector2D ServerDirection(
		NormalizedUnrealForward.Y * ServerAimSignX,
		NormalizedUnrealForward.X * ServerAimSignY
	);
	if (!ServerDirection.Normalize())
	{
		ServerDirection = FVector2D(1.0f, 0.0f);
	}

	return ServerDirection;
}

FVector2D ABattleGridClientPlayerController::ConvertServerDirectionToUnrealDirection(
	float ServerDirX,
	float ServerDirY
) const
{
	FVector2D UnrealDirection(ServerDirY, ServerDirX);
	if (!UnrealDirection.Normalize())
	{
		UnrealDirection = FVector2D(1.0f, 0.0f);
	}

	return UnrealDirection;
}

void ABattleGridClientPlayerController::UpdateServerProjectileGhostsFromSnapshot()
{
	if (!bShowServerProjectileGhosts)
	{
		for (auto Iterator = ServerProjectileGhostActors.CreateIterator(); Iterator; ++Iterator)
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
		}
		ServerProjectileGhostActors.Empty();
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (!World || !GameInstance)
	{
		return;
	}

	UBattleGridNetworkSubsystem* NetworkSubsystem =
		GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>();
	if (!NetworkSubsystem || !NetworkSubsystem->HasSnapshot())
	{
		return;
	}

	if (!bServerSnapshotOriginInitialized)
	{
		if (const APawn* ControlledPawn = GetPawn())
		{
			ServerSnapshotOrigin = ControlledPawn->GetActorLocation();
		}
		else
		{
			ServerSnapshotOrigin = FVector::ZeroVector;
		}
		bServerSnapshotOriginInitialized = true;
	}

	TArray<FBattleGridServerProjectileSnapshot> ProjectileSnapshots;
	NetworkSubsystem->GetLatestProjectileSnapshots(ProjectileSnapshots);

	TSet<int32> ActiveProjectileIds;
	for (const FBattleGridServerProjectileSnapshot& ProjectileSnapshot : ProjectileSnapshots)
	{
		if (ProjectileSnapshot.ProjectileId <= 0)
		{
			continue;
		}

		ActiveProjectileIds.Add(ProjectileSnapshot.ProjectileId);

		const FVector WorldLocation = ConvertServerPositionToWorld(
			ProjectileSnapshot.X,
			ProjectileSnapshot.Y,
			ServerProjectileGhostHeight
		);
		const FVector2D UnrealProjectileDirection =
			ConvertServerDirectionToUnrealDirection(
				ProjectileSnapshot.DirX,
				ProjectileSnapshot.DirY
			);

		TObjectPtr<ABattleGridServerProjectileGhostActor>& GhostActor =
			ServerProjectileGhostActors.FindOrAdd(ProjectileSnapshot.ProjectileId);
		if (!GhostActor)
		{
			TSubclassOf<ABattleGridServerProjectileGhostActor> GhostClass =
				ServerProjectileGhostActorClass;
			if (!GhostClass)
			{
				GhostClass = ABattleGridServerProjectileGhostActor::StaticClass();
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GhostActor = World->SpawnActor<ABattleGridServerProjectileGhostActor>(
				GhostClass,
				WorldLocation,
				FRotator::ZeroRotator,
				SpawnParameters
			);

			if (GhostActor)
			{
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[BattleGrid] Spawned server projectile ghost id=%d dir server=(%.2f,%.2f) unreal=(%.2f,%.2f)"),
					ProjectileSnapshot.ProjectileId,
					ProjectileSnapshot.DirX,
					ProjectileSnapshot.DirY,
					UnrealProjectileDirection.X,
					UnrealProjectileDirection.Y
				);
			}
		}

		if (GhostActor)
		{
			GhostActor->SetSnapshotData(
				ProjectileSnapshot,
				WorldLocation,
				FVector(UnrealProjectileDirection.X, UnrealProjectileDirection.Y, 0.0f)
			);
		}
	}

	for (auto Iterator = ServerProjectileGhostActors.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator.Value() || !ActiveProjectileIds.Contains(Iterator.Key()))
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
			Iterator.RemoveCurrent();
		}
	}
}

void ABattleGridClientPlayerController::UpdateServerTargetGhostsFromSnapshot()
{
	if (!bShowServerTargetGhosts)
	{
		for (auto Iterator = ServerTargetGhostActors.CreateIterator(); Iterator; ++Iterator)
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
		}
		ServerTargetGhostActors.Empty();
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (!World || !GameInstance)
	{
		return;
	}

	UBattleGridNetworkSubsystem* NetworkSubsystem =
		GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>();
	if (!NetworkSubsystem || !NetworkSubsystem->HasSnapshot())
	{
		return;
	}

	if (!bServerSnapshotOriginInitialized)
	{
		if (const APawn* ControlledPawn = GetPawn())
		{
			ServerSnapshotOrigin = ControlledPawn->GetActorLocation();
		}
		else
		{
			ServerSnapshotOrigin = FVector::ZeroVector;
		}
		bServerSnapshotOriginInitialized = true;
	}

	TArray<FBattleGridServerTargetSnapshot> TargetSnapshots;
	NetworkSubsystem->GetLatestTargetSnapshots(TargetSnapshots);

	TSet<int32> ActiveTargetIds;
	for (const FBattleGridServerTargetSnapshot& TargetSnapshot : TargetSnapshots)
	{
		if (TargetSnapshot.TargetId <= 0)
		{
			continue;
		}

		ActiveTargetIds.Add(TargetSnapshot.TargetId);

		const FVector WorldLocation = ConvertServerPositionToWorld(
			TargetSnapshot.X,
			TargetSnapshot.Y,
			ServerTargetGhostHeight
		);

		TObjectPtr<ABattleGridServerTargetGhostActor>& GhostActor =
			ServerTargetGhostActors.FindOrAdd(TargetSnapshot.TargetId);
		if (!GhostActor)
		{
			TSubclassOf<ABattleGridServerTargetGhostActor> GhostClass =
				ServerTargetGhostActorClass;
			if (!GhostClass)
			{
				GhostClass = ABattleGridServerTargetGhostActor::StaticClass();
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GhostActor = World->SpawnActor<ABattleGridServerTargetGhostActor>(
				GhostClass,
				WorldLocation,
				FRotator::ZeroRotator,
				SpawnParameters
			);

			if (GhostActor)
			{
				UE_LOG(
					LogTemp,
					Log,
					TEXT("[BattleGrid] Spawned server target ghost id=%d"),
					TargetSnapshot.TargetId
				);
			}
		}

		if (GhostActor)
		{
			GhostActor->SetSnapshotData(TargetSnapshot, WorldLocation);
		}
	}

	for (auto Iterator = ServerTargetGhostActors.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator.Value() || !ActiveTargetIds.Contains(Iterator.Key()))
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
			Iterator.RemoveCurrent();
		}
	}
}

void ABattleGridClientPlayerController::UpdateOwnServerPositionErrorAndCorrection(
	float DeltaTime
)
{
	UGameInstance* GameInstance = GetGameInstance();
	APawn* ControlledPawn = GetPawn();
	if (!GameInstance || !ControlledPawn)
	{
		return;
	}

	UBattleGridNetworkSubsystem* NetworkSubsystem =
		GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>();
	if (
		!NetworkSubsystem
		|| !NetworkSubsystem->IsConnected()
		|| !NetworkSubsystem->HasJoined()
	)
	{
		bHasOwnServerWorldLocation = false;
		LastServerPositionError = 0.0f;
		return;
	}

	const int32 LocalServerPlayerId = NetworkSubsystem->GetPlayerId();
	if (LocalServerPlayerId <= 0)
	{
		bHasOwnServerWorldLocation = false;
		LastServerPositionError = 0.0f;
		return;
	}

	FBattleGridServerPlayerSnapshot OwnSnapshot;
	if (!NetworkSubsystem->GetPlayerSnapshotById(LocalServerPlayerId, OwnSnapshot))
	{
		bHasOwnServerWorldLocation = false;
		LastServerPositionError = 0.0f;
		return;
	}

	const FVector ServerWorldLocation = ConvertServerPositionToWorld(
		OwnSnapshot.X,
		OwnSnapshot.Y
	);

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector ServerPawnLocation = ServerWorldLocation;
	ServerPawnLocation.Z = PawnLocation.Z;

	const FVector2D PawnLocation2D(PawnLocation.X, PawnLocation.Y);
	const FVector2D ServerLocation2D(ServerPawnLocation.X, ServerPawnLocation.Y);
	LastServerPositionError = FVector2D::Distance(PawnLocation2D, ServerLocation2D);
	LastOwnServerWorldLocation = ServerPawnLocation;
	bHasOwnServerWorldLocation = true;

	const int32 SnapshotTick = NetworkSubsystem->GetLastSnapshotTick();
	const int32 EffectiveSnapshotLogInterval = FMath::Max(1, SnapshotLogInterval);
	if (
		SnapshotTick > 0
		&& SnapshotTick != LastServerPositionErrorLogSnapshotTick
		&& (bVerboseSnapshotLogs || !bDemoMode)
		&& (SnapshotTick <= 5 || SnapshotTick % EffectiveSnapshotLogInterval == 0)
	)
	{
		LastServerPositionErrorLogSnapshotTick = SnapshotTick;
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Server position error: %.1f"),
			LastServerPositionError
		);
	}

	if (
		!bUseServerPositionCorrection
		|| IsPlayerDead()
		|| HasWon()
	)
	{
		return;
	}

	FVector CorrectedLocation = PawnLocation;
	if (LastServerPositionError > ServerCorrectionSnapDistance)
	{
		CorrectedLocation.X = ServerPawnLocation.X;
		CorrectedLocation.Y = ServerPawnLocation.Y;
	}
	else
	{
		const FVector InterpolatedLocation = FMath::VInterpTo(
			PawnLocation,
			ServerPawnLocation,
			DeltaTime,
			FMath::Max(0.0f, ServerCorrectionStrength)
		);
		CorrectedLocation.X = InterpolatedLocation.X;
		CorrectedLocation.Y = InterpolatedLocation.Y;
	}
	CorrectedLocation.Z = PawnLocation.Z;

	ControlledPawn->SetActorLocation(CorrectedLocation);
}
