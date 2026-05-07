#include "BattleGridHUD.h"

#include "BattleGridClientCharacter.h"
#include "BattleGridClientPlayerController.h"
#include "BattleGridCombatWidget.h"
#include "BattleGridNetworkSubsystem.h"
#include "BattleGridWeaponComponent.h"
#include "Blueprint/UserWidget.h"

namespace
{
FString ToGamePrototypeHudText(const FString& Text)
{
	FString Result = Text.Replace(TEXT("TargetKills"), TEXT("CoreKills"));
	Result = Result.Replace(TEXT(" | Target"), TEXT(" | Core"));
	Result = Result.Replace(TEXT("Targets"), TEXT("Cores"));
	Result = Result.Replace(TEXT("HealthPacks"), TEXT("HPacks"));
	return Result;
}
}

ABattleGridHUD::ABattleGridHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	CombatWidget = nullptr;
}

void ABattleGridHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!CombatWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] CombatWidgetClass is not assigned."));
		return;
	}

	APlayerController* OwningController = GetOwningPlayerController();
	if (!OwningController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] HUD has no owning player controller."));
		return;
	}

	CombatWidget = CreateWidget<UBattleGridCombatWidget>(OwningController, CombatWidgetClass);
	if (!CombatWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleGrid] Failed to create combat HUD widget."));
		return;
	}

	CombatWidget->AddToViewport();
}

void ABattleGridHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CombatWidget)
	{
		return;
	}

	ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(GetOwningPlayerController());

	if (!BattleGridController)
	{
		return;
	}

	int32 CurrentAmmo = 0;
	int32 MagazineSize = 0;
	bool bIsReloading = false;
	if (const ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(BattleGridController->GetPawn()))
	{
		if (const UBattleGridWeaponComponent* WeaponComponent =
			BattleGridCharacter->GetWeaponComponent())
		{
			CurrentAmmo = WeaponComponent->GetCurrentAmmo();
			MagazineSize = WeaponComponent->GetMagazineSize();
			bIsReloading = WeaponComponent->IsReloading();
		}
	}

	int32 ServerHP = 0;
	int32 ServerMaxHP = 0;
	int32 ServerKills = 0;
	int32 ServerDeaths = 0;
	int32 ServerBotKills = 0;
	int32 ServerPlayerKills = 0;
	int32 ServerTargetScore = 20;
	bool bHasServerOwnPlayerSnapshot = false;
	bool bServerInvincible = false;
	bool bServerGameOver = false;
	FString ServerCombatEventFeedText;

	if (const UGameInstance* GameInstance = BattleGridController->GetGameInstance())
	{
		if (const UBattleGridNetworkSubsystem* NetworkSubsystem =
			GameInstance->GetSubsystem<UBattleGridNetworkSubsystem>())
		{
			FBattleGridServerPlayerSnapshot OwnSnapshot;
			if (NetworkSubsystem->GetOwnPlayerSnapshot(OwnSnapshot))
			{
				bHasServerOwnPlayerSnapshot = true;
				ServerHP = OwnSnapshot.HP;
				ServerMaxHP = OwnSnapshot.MaxHP;
				ServerKills = OwnSnapshot.Kills;
				ServerDeaths = OwnSnapshot.Deaths;
				ServerBotKills = OwnSnapshot.BotKills;
				ServerPlayerKills = OwnSnapshot.PlayerKills;
				bServerInvincible = OwnSnapshot.bInvincible;
			}

			if (NetworkSubsystem->HasMatchSnapshot())
			{
				const FBattleGridServerMatchSnapshot MatchSnapshot =
					NetworkSubsystem->GetLatestMatchSnapshot();
				ServerTargetScore = MatchSnapshot.TargetScore;
				bServerGameOver = MatchSnapshot.bGameOver;
			}

			ServerCombatEventFeedText = NetworkSubsystem->GetServerCombatEventFeedText();
		}
	}

	CombatWidget->UpdateHud(
		BattleGridController->GetCurrentPlayerHealth(),
		BattleGridController->GetMaxPlayerHealth(),
		BattleGridController->GetScore(),
		BattleGridController->GetTargetScore(),
		BattleGridController->GetOwnServerScore(),
		ServerHP,
		ServerMaxHP,
		ServerKills,
		ServerDeaths,
		ServerBotKills,
		ServerPlayerKills,
		ServerTargetScore,
		bHasServerOwnPlayerSnapshot,
		bServerInvincible,
		bServerGameOver,
		BattleGridController->UseServerAuthoritativeHud(),
		BattleGridController->ShowLocalDebugHud(),
		BattleGridController->ShowCombatEventFeed(),
		CurrentAmmo,
		MagazineSize,
		bIsReloading,
		BattleGridController->GetLastShotSpreadDegrees(),
		BattleGridController->GetCombatMessage(),
		BattleGridController->HasActiveCombatMessage(),
		BattleGridController->HasWon(),
		BattleGridController->GetServerLifeStateText(),
		BattleGridController->IsServerDead(),
		BattleGridController->GetServerPrimaryHudText(),
		BattleGridController->GetLastServerPositionError(),
		BattleGridController->bShowServerPositionError
			&& BattleGridController->HasOwnServerWorldLocation(),
		BattleGridController->IsUsingServerPositionCorrection(),
		BattleGridController->ShouldShowScoreboard(),
		BattleGridController->GetServerScoreboardText(),
		ServerCombatEventFeedText,
		BattleGridController->GetLocalDebugHudText()
	);
}

void ABattleGridHUD::DrawHUD()
{
	Super::DrawHUD();

	if (CombatWidget)
	{
		return;
	}

	ABattleGridClientPlayerController* BattleGridController =
		Cast<ABattleGridClientPlayerController>(GetOwningPlayerController());

	const float HudX = 30.0f;
	const float HudY = 30.0f;
	const float LineHeight = 24.0f;

	DrawRect(
		FLinearColor(0.0f, 0.0f, 0.0f, 0.65f),
		HudX,
		HudY,
		620.0f,
		185.0f
	);

	DrawText(
		TEXT("BattleGrid Client - CombatWidgetClass not assigned"),
		FLinearColor::Yellow,
		HudX + 20.0f,
		HudY + 15.0f,
		nullptr,
		1.35f
	);

	if (!BattleGridController)
	{
		return;
	}

	int32 CurrentAmmo = 0;
	int32 MagazineSize = 0;
	bool bIsReloading = false;
	if (const ABattleGridClientCharacter* BattleGridCharacter =
		Cast<ABattleGridClientCharacter>(BattleGridController->GetPawn()))
	{
		if (const UBattleGridWeaponComponent* WeaponComponent =
			BattleGridCharacter->GetWeaponComponent())
		{
			CurrentAmmo = WeaponComponent->GetCurrentAmmo();
			MagazineSize = WeaponComponent->GetMagazineSize();
			bIsReloading = WeaponComponent->IsReloading();
		}
	}
	const FString AmmoText = bIsReloading
		? FString(TEXT("Reloading..."))
		: FString::Printf(TEXT("%d/%d"), CurrentAmmo, MagazineSize);

	DrawText(
		BattleGridController->UseServerAuthoritativeHud()
			? FString(TEXT("SERVER authoritative HUD active"))
			: FString::Printf(
				TEXT("Local HP: %.0f / %.0f"),
				BattleGridController->GetCurrentPlayerHealth(),
				BattleGridController->GetMaxPlayerHealth()
			),
		FLinearColor::White,
		HudX + 20.0f,
		HudY + 15.0f + LineHeight,
		nullptr,
		1.0f
	);

	DrawText(
		BattleGridController->UseServerAuthoritativeHud()
			? FString::Printf(
				TEXT("Server state is primary | Ammo: %s | Local Debug: %s"),
				*AmmoText,
				BattleGridController->ShowLocalDebugHud() ? TEXT("On") : TEXT("Off")
			)
			: FString::Printf(
				TEXT("Local Score: %d / %d | Ammo: %s | Spread: %.1f"),
				BattleGridController->GetScore(),
				BattleGridController->GetTargetScore(),
				*AmmoText,
				BattleGridController->GetLastShotSpreadDegrees()
			),
		FLinearColor::White,
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 2.0f,
		nullptr,
		1.0f
	);

	DrawText(
		ToGamePrototypeHudText(
			BattleGridController->UseServerAuthoritativeHud()
				? BattleGridController->GetServerPrimaryHudText()
				: BattleGridController->GetDetailedNetworkStatusText()
		),
		FLinearColor(0.8f, 0.9f, 1.0f, 1.0f),
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 3.0f,
		nullptr,
		0.9f
	);

	DrawText(
		BattleGridController->HasWon()
			? FString(TEXT("Victory! Press F5/Enter to Restart"))
			: FString(TEXT("WASD Move | Mouse Look | LMB Fire | RMB ADS | Shift Sprint | Space Jump | R Reload | F5/Enter Restart | Tab Scoreboard")),
		FLinearColor(0.8f, 0.9f, 1.0f, 1.0f),
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 5.0f,
		nullptr,
		0.9f
	);

	if (BattleGridController->ShouldShowScoreboard())
	{
		DrawText(
			ToGamePrototypeHudText(BattleGridController->GetServerScoreboardText()),
			FLinearColor(0.9f, 1.0f, 0.75f, 1.0f),
			HudX + 20.0f,
			HudY + 15.0f + LineHeight * 6.0f,
			nullptr,
			0.85f
		);
	}
	else if (
		BattleGridController->IsServerDead()
		|| BattleGridController->HasActiveCombatMessage()
		|| BattleGridController->HasWon()
	)
	{
		const FString CombatDisplayMessage = BattleGridController->IsServerDead()
			? BattleGridController->GetServerLifeStateText()
			: (BattleGridController->HasWon()
			? FString(TEXT("Victory! Press F5/Enter to Restart"))
			: BattleGridController->GetCombatMessage());

		DrawText(
			CombatDisplayMessage,
			FLinearColor(0.2f, 1.0f, 0.35f, 1.0f),
			HudX + 20.0f,
			HudY + 15.0f + LineHeight * 6.0f,
			nullptr,
			1.0f
		);
	}
}
