// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridClientPlayerController.h"

#include "BattleGridClientCharacter.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridProjectile.h"
#include "BattleGridServerGhostActor.h"
#include "BattleGridServerProjectileGhostActor.h"
#include "BattleGridServerTargetGhostActor.h"
#include "BattleGridServerBotGhostActor.h"
#include "BattleGridServerHealthPackGhostActor.h"
#include "BattleGridWeaponComponent.h"
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
	bRespectServerDeathState = true;
	bServerDeathLocksInput = true;
	bSnapLocalPawnOnServerRespawn = true;
	bShowServerDeathStatus = true;
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
	bApplyDemoServerSettingsOnJoin = false;
	bApplySafeDemoModeOnJoin = false;
	bDemoBotAttacksEnabled = false;
	DemoBotDifficulty = TEXT("easy");
	bDemoAutoEndMatchByTimer = false;
	bScoreboardToggleMode = false;
	bUseServerAuthoritativeHud = true;
	bShowLocalDebugHud = false;
	bShowServerDebugDetails = true;
	bShowCombatEventFeed = true;
	bShowTopFiveRanking = true;
	bShowKillFeed = true;
	ShotResultDisplayDurationSeconds = 1.25f;
	CombatMessageExpireTime = 0.0f;
	bPlayerDead = false;
	bHasWon = false;
	CurrentMoveForward = 0.0f;
	CurrentMoveRight = 0.0f;
	bADSInputHeld = false;
	bIsADSActive = false;
	bIsSprinting = false;
	bFireHeld = false;
	bPendingFireInput = false;
	bPendingReloadInput = false;
	bScoreboardHeld = false;
	bScoreboardVisible = false;
	bDemoServerSettingsAppliedForJoin = false;
	bSafeDemoModeAppliedForJoin = false;
	LastDemoSettingsPlayerId = 0;
	LastSafeDemoModePlayerId = 0;
	ShotSequence = 0;
	LastShotDirectionServer = FVector2D::ZeroVector;
	LastShotDirectionServerZ = 0.0f;
	LastShotSpreadDegrees = 0.0f;
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
	bAutoCalibrateServerSnapshotOrigin = true;
	bShowServerPositionError = true;
	bUseServerPositionCorrection = false;
	ServerCorrectionStrength = 8.0f;
	ServerCorrectionSnapDistance = 500.0f;
	bShowServerProjectileGhosts = true;
	ServerProjectileGhostHeight = 80.0f;
	ServerAimSignX = 1.0f;
	ServerAimSignY = 1.0f;
	bShowServerTargetGhosts = false;
	ServerTargetGhostHeight = 60.0f;
	bShowServerBotGhosts = true;
	ServerBotGhostHeight = 70.0f;
	bShowServerHealthPackGhosts = true;
	ServerHealthPackGhostHeight = 50.0f;
	ServerSnapshotOrigin = FVector::ZeroVector;
	bServerSnapshotOriginInitialized = false;
	bServerSnapshotOriginCalibrated = false;
	LastCalibratedServerPlayerId = 0;
	LastCalibrationSnapshotTick = 0;
	LastCalibrationMatchId = 0;
	LastProcessedSnapshotTick = 0;
	LastServerPositionError = 0.0f;
	LastOwnServerWorldLocation = FVector::ZeroVector;
	bHasOwnServerWorldLocation = false;
	LastServerPositionErrorLogSnapshotTick = 0;
	bWasServerAlive = true;
	bIsServerDead = false;
	bWasServerInvincible = false;
	LastServerRespawnTimer = 0.0f;
	LastServerInvincibleTimer = 0.0f;
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
		SetCombatMessage(TEXT("Victory! Press F5/Enter to Restart"), 3600.0f);
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

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			if (
				NetworkSubsystem->IsConnected()
				&& NetworkSubsystem->HasJoined()
				&& NetworkSubsystem->HasMatchSnapshot()
				&& NetworkSubsystem->GetLatestMatchSnapshot().bGameOver
			)
			{
				UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Sending server match restart request."));
				NetworkSubsystem->SendDebugRestartMatch();
				return;
			}
		}
	}

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

FString ABattleGridClientPlayerController::GetServerCombatEventFeedText() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetServerCombatEventFeedText();
		}
	}

	return FString();
}

FString ABattleGridClientPlayerController::GetRecentServerShotResultText() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetLastShotResultMessage();
		}
	}

	return FString();
}

FString ABattleGridClientPlayerController::GetServerScoreboardText() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetFullScoreboardText();
		}
	}

	return TEXT("Match: Offline\nRank | Player | Kills | D | Bot | PvP\n- | No server ranking | - | - | - | -\nTab: Scoreboard");
}

FString ABattleGridClientPlayerController::GetTopFiveRankingText() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->GetTopFiveRankingText();
		}
	}

	return TEXT("TOP 5\nWaiting...");
}

void ABattleGridClientPlayerController::GetKillFeedLines(TArray<FBattleGridKillFeedLine>& OutLines) const
{
	OutLines.Reset();

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			NetworkSubsystem->GetKillFeedLines(OutLines);
		}
	}
}

bool ABattleGridClientPlayerController::ShouldShowScoreboard() const
{
	if (bScoreboardHeld || bScoreboardVisible)
	{
		return true;
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			return NetworkSubsystem->HasMatchSnapshot()
				&& NetworkSubsystem->GetLatestMatchSnapshot().bGameOver;
		}
	}

	return false;
}

bool ABattleGridClientPlayerController::UseServerAuthoritativeHud() const
{
	return bUseServerAuthoritativeHud;
}

bool ABattleGridClientPlayerController::ShowLocalDebugHud() const
{
	return bShowLocalDebugHud;
}

bool ABattleGridClientPlayerController::ShowServerDebugDetails() const
{
	return bShowServerDebugDetails;
}

bool ABattleGridClientPlayerController::ShowCombatEventFeed() const
{
	return bShowCombatEventFeed;
}

FString ABattleGridClientPlayerController::GetServerPrimaryHudText() const
{
	const FString ProfileText = FString::Printf(TEXT("Profile: %s"), *ResolveServerProfileLabel());

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			TArray<FString> Lines;
			Lines.Add(FString::Printf(
				TEXT("%s | %s"),
				*ProfileText,
				*NetworkSubsystem->GetServerPrimaryStatusText()
			));

			if (NetworkSubsystem->HasJoined())
			{
				Lines.Add(NetworkSubsystem->GetServerMatchStatusText());
				Lines.Add(NetworkSubsystem->GetServerScoreboardCompactText());
				Lines.Add(NetworkSubsystem->GetServerWorldCountsText());

				if (bShowServerDebugDetails)
				{
					const FString CorrectionText = IsUsingServerPositionCorrection()
						? FString(TEXT("On"))
						: FString(TEXT("Off"));
					const FString ErrorText = bShowServerPositionError && HasOwnServerWorldLocation()
						? FString::Printf(TEXT("%.1f"), GetLastServerPositionError())
						: FString(TEXT("-"));
					Lines.Add(FString::Printf(
						TEXT("Snapshot %d | Error %s | Correction %s | ADS %s | Sprint %s"),
						NetworkSubsystem->GetLastSnapshotTick(),
						*ErrorText,
						*CorrectionText,
						bIsADSActive ? TEXT("On") : TEXT("Off"),
						bIsSprinting ? TEXT("On") : TEXT("Off")
					));

					if (!NetworkSubsystem->GetLastDebugMessage().IsEmpty())
					{
						Lines.Add(FString::Printf(
							TEXT("Debug: %s"),
							*NetworkSubsystem->GetLastDebugMessage()
						));
					}
				}
			}

			return FString::Join(Lines, TEXT("\n"));
		}
	}

	return FString::Printf(
		TEXT("%s | Server: Disconnected | Offline local test mode"),
		*ProfileText
	);
}

FString ABattleGridClientPlayerController::GetLocalDebugHudText() const
{
	const FString CorrectionText = IsUsingServerPositionCorrection()
		? FString(TEXT("On"))
		: FString(TEXT("Off"));
	const FString ErrorText = bShowServerPositionError && HasOwnServerWorldLocation()
		? FString::Printf(TEXT("%.1f"), GetLastServerPositionError())
		: FString(TEXT("-"));

	return FString::Printf(
		TEXT("LOCAL DEBUG | HP %.0f/%.0f | Score %d/%d | Error %s | Correction %s | Profile %s"),
		GetCurrentPlayerHealth(),
		GetMaxPlayerHealth(),
		GetScore(),
		GetTargetScore(),
		*ErrorText,
		*CorrectionText,
		*ResolveServerProfileLabel()
	);
}

FString ABattleGridClientPlayerController::GetDetailedNetworkStatusText() const
{
	TArray<FString> Lines;

	if (bUseServerAuthoritativeHud)
	{
		Lines.Add(GetServerPrimaryHudText());
	}
	else
	{
		Lines.Add(FString::Printf(
			TEXT("Profile: %s | Server HUD disabled | Offline/local debug mode"),
			*ResolveServerProfileLabel()
		));
	}

	if (bShowLocalDebugHud)
	{
		Lines.Add(GetLocalDebugHudText());
	}

	return FString::Join(Lines, TEXT("\n"));
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

float ABattleGridClientPlayerController::GetLastShotSpreadDegrees() const
{
	return LastShotSpreadDegrees;
}

float ABattleGridClientPlayerController::GetCurrentWeaponSpreadDegrees() const
{
	const ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(GetPawn());
	if (!BattleGridCharacter)
	{
		return LastShotSpreadDegrees;
	}

	const UBattleGridWeaponComponent* WeaponComponent =
		BattleGridCharacter->GetWeaponComponent();
	if (!WeaponComponent)
	{
		return LastShotSpreadDegrees;
	}

	return WeaponComponent->CalculateCurrentSpread(
		IsCrosshairAds(),
		IsCrosshairSprinting(),
		IsCrosshairJumping()
	);
}

bool ABattleGridClientPlayerController::IsCrosshairAds() const
{
	return bIsADSActive;
}

bool ABattleGridClientPlayerController::IsCrosshairSprinting() const
{
	return bIsSprinting;
}

bool ABattleGridClientPlayerController::IsCrosshairJumping() const
{
	return IsControlledPawnFalling();
}

bool ABattleGridClientPlayerController::IsCrosshairReloading() const
{
	const ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(GetPawn());
	if (!BattleGridCharacter)
	{
		return false;
	}

	const UBattleGridWeaponComponent* WeaponComponent =
		BattleGridCharacter->GetWeaponComponent();
	return WeaponComponent && WeaponComponent->IsReloading();
}

bool ABattleGridClientPlayerController::ShouldShowCrosshair() const
{
	return GetPawn() != nullptr;
}

bool ABattleGridClientPlayerController::IsServerDead() const
{
	return bRespectServerDeathState && bIsServerDead;
}

bool ABattleGridClientPlayerController::IsServerInvincible() const
{
	return bRespectServerDeathState && !bIsServerDead && bWasServerInvincible;
}

float ABattleGridClientPlayerController::GetLastServerRespawnTimer() const
{
	return LastServerRespawnTimer;
}

float ABattleGridClientPlayerController::GetLastServerInvincibleTimer() const
{
	return LastServerInvincibleTimer;
}

FString ABattleGridClientPlayerController::GetServerLifeStateText() const
{
	if (!bShowServerDeathStatus)
	{
		return FString();
	}

	if (!bRespectServerDeathState)
	{
		return TEXT("Server Life: Local Only");
	}

	if (!IsServerConnected() || !HasJoinedServer())
	{
		return TEXT("Server Life: -");
	}

	if (IsServerDead())
	{
		return FString::Printf(
			TEXT("SERVER DEAD | Respawn %.1fs"),
			LastServerRespawnTimer
		);
	}

	if (IsServerInvincible())
	{
		return FString::Printf(
			TEXT("SERVER INVINCIBLE %.1fs"),
			LastServerInvincibleTimer
		);
	}

	return TEXT("Server Life: Alive");
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
				NetworkSubsystem->SetShotResultDisplayDuration(ShotResultDisplayDurationSeconds);
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
		EnhancedInputComponent->BindAction(
			FireAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::FireEnded
		);
		EnhancedInputComponent->BindAction(
			FireAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::FireEnded
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

	if (ReloadAction)
	{
		EnhancedInputComponent->BindAction(
			ReloadAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::ReloadStarted
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] ReloadAction is not assigned."));
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

	if (ScoreboardAction)
	{
		EnhancedInputComponent->BindAction(
			ScoreboardAction,
			ETriggerEvent::Started,
			this,
			&ABattleGridClientPlayerController::ScoreboardStarted
		);
		EnhancedInputComponent->BindAction(
			ScoreboardAction,
			ETriggerEvent::Completed,
			this,
			&ABattleGridClientPlayerController::ScoreboardEnded
		);
		EnhancedInputComponent->BindAction(
			ScoreboardAction,
			ETriggerEvent::Canceled,
			this,
			&ABattleGridClientPlayerController::ScoreboardEnded
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] ScoreboardAction is not assigned."));
	}
}

void ABattleGridClientPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ApplyMovementAndADSState();
	CalibrateServerSnapshotOriginIfNeeded();
	UpdateServerLifeStateFromSnapshot(DeltaTime);
	ApplyDemoServerSettingsIfNeeded();
	UpdateAimRotation();
	if (bFireHeld)
	{
		TryFireWeapon();
	}
	SendInputToServerIfNeeded();
	UpdateServerGhostsFromSnapshot();
	UpdateServerProjectileGhostsFromSnapshot();
	UpdateServerTargetGhostsFromSnapshot();
	UpdateServerBotGhostsFromSnapshot();
	UpdateServerHealthPackGhostsFromSnapshot();
	UpdateOwnServerPositionErrorAndCorrection(DeltaTime);
}

void ABattleGridClientPlayerController::MoveForward(const FInputActionValue& Value)
{
	if (ShouldBlockServerGameplayInput())
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
	if (ShouldBlockServerGameplayInput())
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

	if (ShouldBlockServerGameplayInput())
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

	if (ShouldBlockServerGameplayInput())
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

	if (ShouldBlockServerGameplayInput())
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

void ABattleGridClientPlayerController::ReloadStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (ShouldBlockServerGameplayInput())
	{
		return;
	}

	ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(GetPawn());
	if (!BattleGridCharacter)
	{
		return;
	}

	UBattleGridWeaponComponent* WeaponComponent =
		BattleGridCharacter->GetWeaponComponent();
	if (!WeaponComponent)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Reload input."));
	WeaponComponent->StartReload();
	bPendingReloadInput = true;
	SendInputToServer(true);
}

void ABattleGridClientPlayerController::ScoreboardStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (bScoreboardToggleMode)
	{
		bScoreboardVisible = !bScoreboardVisible;
		return;
	}

	bScoreboardHeld = true;
}

void ABattleGridClientPlayerController::ScoreboardEnded(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (bScoreboardToggleMode)
	{
		return;
	}

	bScoreboardHeld = false;
}

void ABattleGridClientPlayerController::FireStarted(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	if (ShouldBlockServerGameplayInput())
	{
		bFireHeld = false;
		return;
	}

	bFireHeld = true;
	TryFireWeapon();
}

void ABattleGridClientPlayerController::FireEnded(const FInputActionValue& Value)
{
	static_cast<void>(Value);

	bFireHeld = false;
}

void ABattleGridClientPlayerController::TryFireWeapon()
{
	if (ShouldBlockServerGameplayInput())
	{
		bFireHeld = false;
		return;
	}

	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();

	if (!World || !ControlledPawn)
	{
		return;
	}

	UpdateAimRotation();

	ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(ControlledPawn);
	UBattleGridWeaponComponent* WeaponComponent = BattleGridCharacter
		? BattleGridCharacter->GetWeaponComponent()
		: nullptr;
	if (!WeaponComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Cannot fire. WeaponComponent is missing."));
		return;
	}

	if (!WeaponComponent->TryConsumeAmmoForShot())
	{
		const bool bOutOfAmmo = WeaponComponent->GetCurrentAmmo() <= 0;
		const bool bIsReloading = WeaponComponent->IsReloading();
		if (bVerboseInputLogs || !bDemoMode || (bOutOfAmmo && !bIsReloading))
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Cannot fire. Ammo=%d Reloading=%s"),
				WeaponComponent->GetCurrentAmmo(),
				bIsReloading ? TEXT("true") : TEXT("false")
			);
		}

		if (bOutOfAmmo && !bIsReloading)
		{
			WeaponComponent->StartReload();
		}
		return;
	}

	const float ShotSpread = WeaponComponent->CalculateCurrentSpread(
		bIsADSActive,
		bIsSprinting,
		IsControlledPawnFalling()
	);
	LastShotSpreadDegrees = ShotSpread;

	const FVector FireDirection = CalculateShotDirectionWithSpread(ShotSpread);
	LastShotDirectionServer = ConvertUnrealDirectionToServerDirection(FireDirection);
	LastShotDirectionServerZ = FireDirection.Z;

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
		LastFireTime = World->GetTimeSeconds();
		++ShotSequence;
		bPendingFireInput = true;
		SendInputToServer(true);

		const int32 InputLogInterval = FMath::Max(1, InputAckLogInterval);
		if (bVerboseInputLogs || !bDemoMode || ShotSequence <= 3 || ShotSequence % InputLogInterval == 0)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[BattleGrid] Weapon fired. Ammo=%d/%d Spread=%.2f"),
				WeaponComponent->GetCurrentAmmo(),
				WeaponComponent->GetMagazineSize(),
				ShotSpread
			);
		}
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

	const bool bBlockGameplayInput = ShouldBlockServerGameplayInput();
	if (bBlockGameplayInput)
	{
		bADSInputHeld = false;
		bIsSprinting = false;
	}

	const bool bCanADS =
		bADSInputHeld
		&& !bIsSprinting
		&& !IsControlledPawnFalling()
		&& !bBlockGameplayInput;
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
			float DesiredSpeed = bBlockGameplayInput ? 0.0f : NormalMoveSpeed;
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

FVector ABattleGridClientPlayerController::CalculateShotDirectionWithSpread(
	float SpreadDegrees
) const
{
	FVector ShotDirection = GetControlRotation().Vector();
	if (!ShotDirection.Normalize())
	{
		ShotDirection = FVector::ForwardVector;
	}

	const float ClampedSpreadDegrees = FMath::Max(0.0f, SpreadDegrees);
	if (ClampedSpreadDegrees <= KINDA_SMALL_NUMBER)
	{
		return ShotDirection;
	}

	ShotDirection = FMath::VRandCone(
		ShotDirection,
		FMath::DegreesToRadians(ClampedSpreadDegrees)
	);
	if (!ShotDirection.Normalize())
	{
		ShotDirection = FVector::ForwardVector;
	}

	return ShotDirection;
}

void ABattleGridClientPlayerController::SendInputToServerIfNeeded()
{
	SendInputToServer(false);
}

void ABattleGridClientPlayerController::SendInputToServer(bool bForceSend)
{
	if (ShouldBlockServerGameplayInput())
	{
		CurrentMoveForward = 0.0f;
		CurrentMoveRight = 0.0f;
		bPendingFireInput = false;
		bPendingReloadInput = false;
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
		bPendingReloadInput = false;
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

	FVector2D ServerShotDirection = ServerAimDirection;
	float ServerShotDirectionZ = 0.0f;
	if (bPendingFireInput && LastShotDirectionServer.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		ServerShotDirection = LastShotDirectionServer;
		ServerShotDirectionZ = LastShotDirectionServerZ;
	}

	int32 CurrentAmmo = 0;
	if (const ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(GetPawn()))
	{
		if (const UBattleGridWeaponComponent* WeaponComponent =
			BattleGridCharacter->GetWeaponComponent())
		{
			CurrentAmmo = WeaponComponent->GetCurrentAmmo();
		}
	}

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
		&& !bPendingReloadInput
		&& !bMovementChanged
		&& !bAimChanged
	)
	{
		return;
	}

	const bool bFire = bPendingFireInput;
	const bool bReload = bPendingReloadInput;
	const bool bJump = IsControlledPawnFalling();
	++InputSequence;
	LastInputSendTime = CurrentTime;

	const int32 InputLogInterval = FMath::Max(1, InputAckLogInterval);
	if ((bVerboseInputLogs || !bDemoMode) && (bFire || bReload || InputSequence % InputLogInterval == 0))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Aim convert unreal=(%.2f,%.2f) server=(%.2f,%.2f) shot=(%.2f,%.2f)"),
			UnrealAimDirection.X,
			UnrealAimDirection.Y,
			AimX,
			AimY,
			ServerShotDirection.X,
			ServerShotDirection.Y
		);
	}

	if (bFire)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Fire input shot_dir_server=(%.2f,%.2f,%.2f) aim=(%.2f,%.2f) ammo=%d"),
			ServerShotDirection.X,
			ServerShotDirection.Y,
			ServerShotDirectionZ,
			AimX,
			AimY,
			CurrentAmmo
		);
	}

	NetworkSubsystem->SendInput(
		InputSequence,
		ServerMoveX,
		ServerMoveY,
		AimX,
		AimY,
		ServerShotDirection.X,
		ServerShotDirection.Y,
		ServerShotDirectionZ,
		bFire,
		bReload,
		bIsADSActive,
		bIsSprinting,
		bJump,
		CurrentAmmo,
		LastShotSpreadDegrees
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
	if (bReload)
	{
		bPendingReloadInput = false;
	}
}

void ABattleGridClientPlayerController::ApplyDemoServerSettingsIfNeeded()
{
	if (!bApplyDemoServerSettingsOnJoin && !bApplySafeDemoModeOnJoin)
	{
		bDemoServerSettingsAppliedForJoin = false;
		bSafeDemoModeAppliedForJoin = false;
		LastDemoSettingsPlayerId = 0;
		LastSafeDemoModePlayerId = 0;
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
		bDemoServerSettingsAppliedForJoin = false;
		bSafeDemoModeAppliedForJoin = false;
		LastDemoSettingsPlayerId = 0;
		LastSafeDemoModePlayerId = 0;
		return;
	}

	const int32 CurrentServerPlayerId = NetworkSubsystem->GetPlayerId();
	if (CurrentServerPlayerId <= 0)
	{
		return;
	}

	if (bApplySafeDemoModeOnJoin)
	{
		if (
			!bSafeDemoModeAppliedForJoin
			|| LastSafeDemoModePlayerId != CurrentServerPlayerId
		)
		{
			UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Applying safe demo mode on join."));
			NetworkSubsystem->SendDebugApplyDemoMode();
			bSafeDemoModeAppliedForJoin = true;
			LastSafeDemoModePlayerId = CurrentServerPlayerId;
		}

		bDemoServerSettingsAppliedForJoin = true;
		LastDemoSettingsPlayerId = CurrentServerPlayerId;
		return;
	}

	bSafeDemoModeAppliedForJoin = false;
	LastSafeDemoModePlayerId = 0;

	if (!bApplyDemoServerSettingsOnJoin)
	{
		bDemoServerSettingsAppliedForJoin = false;
		LastDemoSettingsPlayerId = 0;
		return;
	}

	if (
		bDemoServerSettingsAppliedForJoin
		&& LastDemoSettingsPlayerId == CurrentServerPlayerId
	)
	{
		return;
	}

	const FString DifficultyToSend = DemoBotDifficulty.IsEmpty()
		? FString(TEXT("easy"))
		: DemoBotDifficulty.ToLower();

	NetworkSubsystem->SendDebugSetBotDifficulty(DifficultyToSend);
	NetworkSubsystem->SendDebugSetBotAttacks(bDemoBotAttacksEnabled);
	NetworkSubsystem->SendDebugSetMatchTimer(bDemoAutoEndMatchByTimer);

	bDemoServerSettingsAppliedForJoin = true;
	LastDemoSettingsPlayerId = CurrentServerPlayerId;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] Applied demo server settings. difficulty=%s bot_attacks=%s auto_timer=%s"),
		*DifficultyToSend,
		bDemoBotAttacksEnabled ? TEXT("true") : TEXT("false"),
		bDemoAutoEndMatchByTimer ? TEXT("true") : TEXT("false")
	);
}

bool ABattleGridClientPlayerController::ShouldBlockServerGameplayInput() const
{
	return IsPlayerDead()
		|| HasWon()
		|| (
			bRespectServerDeathState
			&& bServerDeathLocksInput
			&& bIsServerDead
		);
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

FVector ABattleGridClientPlayerController::ConvertServerPositionToWorldNoOrigin(
	float ServerX,
	float ServerY,
	float WorldHeight
) const
{
	// The server uses logical 2D arena coordinates. The current Unreal camera
	// orientation needs X/Y swapped for ghost visualization.
	const float WorldX = ServerY * ServerToUnrealScale;
	const float WorldY = ServerX * ServerToUnrealScale;

	return FVector(WorldX, WorldY, WorldHeight);
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
	return ServerSnapshotOrigin + ConvertServerPositionToWorldNoOrigin(
		ServerX,
		ServerY,
		WorldHeight
	);
}

void ABattleGridClientPlayerController::CalibrateServerSnapshotOriginIfNeeded()
{
	if (!bAutoCalibrateServerSnapshotOrigin)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	APawn* ControlledPawn = GetPawn();
	if (!GameInstance || !ControlledPawn)
	{
		ResetServerSnapshotOriginCalibration();
		return;
	}

	UBattleGridNetworkSubsystem* NetworkSubsystem =
		GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>();
	if (
		!NetworkSubsystem
		|| !NetworkSubsystem->IsConnected()
		|| !NetworkSubsystem->HasJoined()
		|| !NetworkSubsystem->HasSnapshot()
	)
	{
		ResetServerSnapshotOriginCalibration();
		return;
	}

	const int32 LocalServerPlayerId = NetworkSubsystem->GetPlayerId();
	const int32 SnapshotTick = NetworkSubsystem->GetLastSnapshotTick();
	const int32 MatchId = NetworkSubsystem->HasMatchSnapshot()
		? NetworkSubsystem->GetLatestMatchSnapshot().MatchId
		: 0;
	const bool bKnownMatchChanged =
		MatchId > 0 && LastCalibrationMatchId > 0 && MatchId != LastCalibrationMatchId;

	if (
		LocalServerPlayerId != LastCalibratedServerPlayerId
		|| SnapshotTick < LastCalibrationSnapshotTick
		|| bKnownMatchChanged
	)
	{
		ResetServerSnapshotOriginCalibration();
	}

	LastCalibratedServerPlayerId = LocalServerPlayerId;
	LastCalibrationSnapshotTick = SnapshotTick;
	if (MatchId > 0)
	{
		LastCalibrationMatchId = MatchId;
	}

	if (bServerSnapshotOriginCalibrated || LocalServerPlayerId <= 0)
	{
		return;
	}

	FBattleGridServerPlayerSnapshot OwnSnapshot;
	if (!NetworkSubsystem->GetPlayerSnapshotById(LocalServerPlayerId, OwnSnapshot))
	{
		return;
	}

	const FVector ServerOwnNoOrigin = ConvertServerPositionToWorldNoOrigin(
		OwnSnapshot.X,
		OwnSnapshot.Y,
		0.0f
	);
	const FVector PawnLocation = ControlledPawn->GetActorLocation();

	ServerSnapshotOrigin = FVector(
		PawnLocation.X - ServerOwnNoOrigin.X,
		PawnLocation.Y - ServerOwnNoOrigin.Y,
		0.0f
	);
	bServerSnapshotOriginInitialized = true;
	bServerSnapshotOriginCalibrated = true;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BattleGrid] Calibrated server snapshot origin. player_id=%d server=(%.1f,%.1f) origin=(%.1f,%.1f)"),
		LocalServerPlayerId,
		OwnSnapshot.X,
		OwnSnapshot.Y,
		ServerSnapshotOrigin.X,
		ServerSnapshotOrigin.Y
	);
}

void ABattleGridClientPlayerController::ResetServerSnapshotOriginCalibration()
{
	bServerSnapshotOriginCalibrated = false;
	LastCalibratedServerPlayerId = 0;
	LastCalibrationSnapshotTick = 0;
	LastCalibrationMatchId = 0;
	LastProcessedSnapshotTick = 0;
	LastServerPositionErrorLogSnapshotTick = 0;
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
			ServerProjectileGhostHeight + (ProjectileSnapshot.Z * ServerToUnrealScale)
		);
		FVector WorldStartLocation = ConvertServerPositionToWorld(
			ProjectileSnapshot.ServerStart.X,
			ProjectileSnapshot.ServerStart.Y,
			ServerProjectileGhostHeight + (ProjectileSnapshot.ServerStart.Z * ServerToUnrealScale)
		);
		const FVector WorldEndLocation = ConvertServerPositionToWorld(
			ProjectileSnapshot.ServerEnd.X,
			ProjectileSnapshot.ServerEnd.Y,
			ServerProjectileGhostHeight + (ProjectileSnapshot.ServerEnd.Z * ServerToUnrealScale)
		);
		const FVector2D UnrealProjectileDirection =
			ConvertServerDirectionToUnrealDirection(
				ProjectileSnapshot.DirX,
				ProjectileSnapshot.DirY
			);
		FVector UnrealProjectileDirection3D(
			UnrealProjectileDirection.X,
			UnrealProjectileDirection.Y,
			ProjectileSnapshot.DirZ
		);
		if (!UnrealProjectileDirection3D.Normalize())
		{
			UnrealProjectileDirection3D = FVector(1.0f, 0.0f, 0.0f);
		}

		if (ProjectileSnapshot.OwnerType.Equals(TEXT("bot"), ESearchCase::IgnoreCase))
		{
			if (const TObjectPtr<ABattleGridServerBotGhostActor>* BotGhostActor =
				ServerBotGhostActors.Find(ProjectileSnapshot.OwnerBotId))
			{
				if (*BotGhostActor)
				{
					WorldStartLocation = (*BotGhostActor)->GetApproximateMuzzleWorldLocation();
				}
			}
		}
		else if (
			ProjectileSnapshot.OwnerType.Equals(TEXT("player"), ESearchCase::IgnoreCase)
			&& ProjectileSnapshot.OwnerPlayerId == NetworkSubsystem->GetPlayerId()
		)
		{
			if (const ABattleGridClientCharacter* Character =
				Cast<ABattleGridClientCharacter>(GetPawn()))
			{
				WorldStartLocation = Character->GetApproximateMuzzleWorldLocation();
			}
		}

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
				UnrealProjectileDirection3D,
				WorldStartLocation,
				WorldEndLocation
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

void ABattleGridClientPlayerController::UpdateServerBotGhostsFromSnapshot()
{
	if (!bShowServerBotGhosts)
	{
		for (auto Iterator = ServerBotGhostActors.CreateIterator(); Iterator; ++Iterator)
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
		}
		ServerBotGhostActors.Empty();
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

	TArray<FBattleGridServerBotSnapshot> BotSnapshots;
	NetworkSubsystem->GetLatestBotSnapshots(BotSnapshots);

	TSet<int32> ActiveBotIds;
	for (const FBattleGridServerBotSnapshot& BotSnapshot : BotSnapshots)
	{
		if (BotSnapshot.BotId <= 0)
		{
			continue;
		}

		ActiveBotIds.Add(BotSnapshot.BotId);

		const FVector WorldLocation = ConvertServerPositionToWorld(
			BotSnapshot.X,
			BotSnapshot.Y,
			ServerBotGhostHeight + BotSnapshot.Z
		);

		TObjectPtr<ABattleGridServerBotGhostActor>& GhostActor =
			ServerBotGhostActors.FindOrAdd(BotSnapshot.BotId);
		if (!GhostActor)
		{
			TSubclassOf<ABattleGridServerBotGhostActor> GhostClass =
				ServerBotGhostActorClass;
			if (!GhostClass)
			{
				GhostClass = ABattleGridServerBotGhostActor::StaticClass();
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GhostActor = World->SpawnActor<ABattleGridServerBotGhostActor>(
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
					TEXT("[BattleGrid] Spawned server bot ghost id=%d"),
					BotSnapshot.BotId
				);
			}
		}

		if (GhostActor)
		{
			GhostActor->SetSnapshotData(BotSnapshot, WorldLocation);
		}
	}

	for (auto Iterator = ServerBotGhostActors.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator.Value() || !ActiveBotIds.Contains(Iterator.Key()))
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
			Iterator.RemoveCurrent();
		}
	}
}

void ABattleGridClientPlayerController::UpdateServerHealthPackGhostsFromSnapshot()
{
	if (!bShowServerHealthPackGhosts)
	{
		for (auto Iterator = ServerHealthPackGhostActors.CreateIterator(); Iterator; ++Iterator)
		{
			if (Iterator.Value())
			{
				Iterator.Value()->Destroy();
			}
		}
		ServerHealthPackGhostActors.Empty();
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

	TArray<FBattleGridServerHealthPackSnapshot> HealthPackSnapshots;
	NetworkSubsystem->GetLatestHealthPackSnapshots(HealthPackSnapshots);

	TSet<int32> ActiveHealthPackIds;
	for (const FBattleGridServerHealthPackSnapshot& HealthPackSnapshot : HealthPackSnapshots)
	{
		if (HealthPackSnapshot.HealthPackId <= 0)
		{
			continue;
		}

		ActiveHealthPackIds.Add(HealthPackSnapshot.HealthPackId);

		const FVector WorldLocation = ConvertServerPositionToWorld(
			HealthPackSnapshot.X,
			HealthPackSnapshot.Y,
			ServerHealthPackGhostHeight + HealthPackSnapshot.Z
		);

		TObjectPtr<ABattleGridServerHealthPackGhostActor>& GhostActor =
			ServerHealthPackGhostActors.FindOrAdd(HealthPackSnapshot.HealthPackId);
		if (!GhostActor)
		{
			TSubclassOf<ABattleGridServerHealthPackGhostActor> GhostClass =
				ServerHealthPackGhostActorClass;
			if (!GhostClass)
			{
				GhostClass = ABattleGridServerHealthPackGhostActor::StaticClass();
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GhostActor = World->SpawnActor<ABattleGridServerHealthPackGhostActor>(
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
					TEXT("[BattleGrid] Spawned server health pack ghost id=%d"),
					HealthPackSnapshot.HealthPackId
				);
			}
		}

		if (GhostActor)
		{
			GhostActor->SetSnapshotData(HealthPackSnapshot, WorldLocation);
		}
	}

	for (auto Iterator = ServerHealthPackGhostActors.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator.Value() || !ActiveHealthPackIds.Contains(Iterator.Key()))
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
		|| IsServerDead()
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

void ABattleGridClientPlayerController::UpdateServerLifeStateFromSnapshot(float DeltaTime)
{
	static_cast<void>(DeltaTime);

	if (!bRespectServerDeathState)
	{
		bIsServerDead = false;
		bWasServerAlive = true;
		bWasServerInvincible = false;
		LastServerRespawnTimer = 0.0f;
		LastServerInvincibleTimer = 0.0f;
		return;
	}

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
		bIsServerDead = false;
		bWasServerAlive = true;
		bWasServerInvincible = false;
		LastServerRespawnTimer = 0.0f;
		LastServerInvincibleTimer = 0.0f;
		return;
	}

	FBattleGridServerPlayerSnapshot OwnSnapshot;
	if (!NetworkSubsystem->GetOwnPlayerSnapshot(OwnSnapshot))
	{
		return;
	}

	const bool bCurrentServerAlive = OwnSnapshot.bAlive;
	const bool bCurrentServerDead = !bCurrentServerAlive;
	const bool bCurrentServerInvincible = OwnSnapshot.bInvincible;
	const bool bWasDeadBeforeUpdate = bIsServerDead;
	const bool bWasInvincibleBeforeUpdate = bWasServerInvincible;

	bIsServerDead = bCurrentServerDead;
	LastServerRespawnTimer = FMath::Max(0.0f, OwnSnapshot.RespawnTimer);
	LastServerInvincibleTimer = FMath::Max(0.0f, OwnSnapshot.InvincibleTimer);

	if (!bWasDeadBeforeUpdate && bIsServerDead)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Server says player died. RespawnTimer=%.1f"),
			LastServerRespawnTimer
		);

		bADSInputHeld = false;
		bIsADSActive = false;
		bIsSprinting = false;
		bFireHeld = false;
		bPendingFireInput = false;
		bPendingReloadInput = false;
		CurrentMoveForward = 0.0f;
		CurrentMoveRight = 0.0f;
		ApplyMovementAndADSState();

		if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
		{
			if (UCharacterMovementComponent* MovementComponent =
				ControlledCharacter->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
			}
		}
	}
	else if (bWasDeadBeforeUpdate && !bIsServerDead)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Server says player respawned."));

		if (bSnapLocalPawnOnServerRespawn)
		{
			const FVector PawnLocation = ControlledPawn->GetActorLocation();
			FVector RespawnWorldLocation = ConvertServerPositionToWorld(
				OwnSnapshot.X,
				OwnSnapshot.Y,
				0.0f
			);
			RespawnWorldLocation.Z = PawnLocation.Z;
			ControlledPawn->SetActorLocation(RespawnWorldLocation);

			if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
			{
				if (UCharacterMovementComponent* MovementComponent =
					ControlledCharacter->GetCharacterMovement())
				{
					MovementComponent->StopMovementImmediately();
				}
			}
		}

		CurrentMoveForward = 0.0f;
		CurrentMoveRight = 0.0f;
		bHasLastSentInput = false;
	}

	if (!bWasInvincibleBeforeUpdate && bCurrentServerInvincible)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[BattleGrid] Server invincibility started. Timer=%.1f"),
			LastServerInvincibleTimer
		);
	}
	else if (bWasInvincibleBeforeUpdate && !bCurrentServerInvincible)
	{
		UE_LOG(LogTemp, Log, TEXT("[BattleGrid] Server invincibility ended."));
	}

	bWasServerAlive = bCurrentServerAlive;
	bWasServerInvincible = bCurrentServerInvincible;
}
