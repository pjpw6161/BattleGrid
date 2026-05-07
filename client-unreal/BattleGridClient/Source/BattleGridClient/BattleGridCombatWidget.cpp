// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridCombatWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

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

void UBattleGridCombatWidget::UpdateHud(
	float CurrentHealth,
	float MaxHealth,
	int32 Score,
	int32 TargetScore,
	int32 ServerScore,
	int32 ServerHP,
	int32 ServerMaxHP,
	int32 ServerKills,
	int32 ServerDeaths,
	int32 ServerBotKills,
	int32 ServerPlayerKills,
	int32 ServerTargetScore,
	bool bHasServerOwnPlayerSnapshot,
	bool bServerInvincible,
	bool bServerGameOver,
	bool bUseServerAuthoritativeHud,
	bool bShowLocalDebugHud,
	bool bShowCombatEventFeed,
	int32 CurrentAmmo,
	int32 MagazineSize,
	bool bIsReloading,
	float LastShotSpreadDegrees,
	const FString& CombatMessage,
	bool bShowCombatMessage,
	bool bHasWon,
	const FString& ServerLifeStateText,
	bool bServerDead,
	const FString& NetworkStatusText,
	float ServerPositionError,
	bool bHasServerPositionError,
	bool bUseServerCorrection,
	bool bShowScoreboard,
	const FString& ScoreboardText,
	const FString& ServerCombatEventFeedText,
	const FString& ServerShotResultText,
	const FString& LocalDebugHudText
)
{
	static_cast<void>(ServerPositionError);
	static_cast<void>(bHasServerPositionError);
	static_cast<void>(bUseServerCorrection);

	const bool bShowServerHudValues =
		bUseServerAuthoritativeHud && bHasServerOwnPlayerSnapshot;
	const float DisplayHealth = bShowServerHudValues
		? static_cast<float>(ServerHP)
		: CurrentHealth;
	const float DisplayMaxHealth = bShowServerHudValues
		? static_cast<float>(ServerMaxHP)
		: MaxHealth;
	const float HealthPercent = DisplayMaxHealth > 0.0f
		? FMath::Clamp(DisplayHealth / DisplayMaxHealth, 0.0f, 1.0f)
		: 0.0f;

	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	if (HealthText)
	{
		if (bShowServerHudValues)
		{
			const FString LifeSuffix = bServerDead
				? FString(TEXT(" | DEAD"))
				: (bServerInvincible ? FString(TEXT(" | INV")) : FString());
			HealthText->SetText(FText::FromString(FString::Printf(
				TEXT("SERVER HP: %d / %d%s"),
				ServerHP,
				ServerMaxHP,
				*LifeSuffix
			)));
		}
		else
		{
			HealthText->SetText(FText::FromString(FString::Printf(
				TEXT("Local HP: %.0f / %.0f"),
				CurrentHealth,
				MaxHealth
			)));
		}
	}

	if (ScoreText)
	{
		const FString AmmoText = bIsReloading
			? FString(TEXT("Reloading..."))
			: FString::Printf(TEXT("%d/%d"), CurrentAmmo, MagazineSize);
		if (bShowServerHudValues)
		{
			ScoreText->SetText(FText::FromString(FString::Printf(
				TEXT("SERVER Score: %d / %d | K/D %d/%d | Bots %d | PvP %d | Ammo %s"),
				ServerScore,
				ServerTargetScore,
				ServerKills,
				ServerDeaths,
				ServerBotKills,
				ServerPlayerKills,
				*AmmoText
			)));
		}
		else
		{
			ScoreText->SetText(FText::FromString(FString::Printf(
				TEXT("Local Score: %d / %d | Ammo: %s | Spread: %.1f"),
				Score,
				TargetScore,
				*AmmoText,
				LastShotSpreadDegrees
			)));
		}
	}

	if (CombatMessageText)
	{
		FString DisplayMessage;
		if (bShowScoreboard)
		{
			DisplayMessage = ToGamePrototypeHudText(ScoreboardText);
		}
		else if (bServerDead)
		{
			DisplayMessage = ServerLifeStateText;
		}
		else if (bServerGameOver)
		{
			DisplayMessage = TEXT("SERVER GAME OVER");
		}
		else if (bHasWon && !bUseServerAuthoritativeHud)
		{
			DisplayMessage = TEXT("Victory! Press F5/Enter to Restart");
		}
		else if (bServerInvincible && !ServerLifeStateText.IsEmpty())
		{
			DisplayMessage = ServerLifeStateText;
		}
		else if (!ServerShotResultText.IsEmpty())
		{
			DisplayMessage = ServerShotResultText;
		}
		else if (bShowCombatEventFeed && !ServerCombatEventFeedText.IsEmpty())
		{
			DisplayMessage = FString::Printf(
				TEXT("Events:\n%s"),
				*ServerCombatEventFeedText
			);
		}
		else if (bShowCombatMessage)
		{
			DisplayMessage = CombatMessage;
		}

		CombatMessageText->SetText(FText::FromString(DisplayMessage));
		CombatMessageText->SetVisibility(
			!DisplayMessage.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (ControlsText)
	{
		const FString PolishedNetworkStatusText = ToGamePrototypeHudText(NetworkStatusText);
		const FString BaseControlsMessage = bHasWon
			? FString(TEXT("Victory! Press F5/Enter to Restart"))
			: FString(TEXT("WASD Move | Mouse Look | LMB Fire | RMB ADS | Shift Sprint | Space Jump | R Reload | F5/Enter Restart | Tab Scoreboard"));
		TArray<FString> ControlLines;
		if (!PolishedNetworkStatusText.IsEmpty())
		{
			ControlLines.Add(PolishedNetworkStatusText);
		}
		if (bShowLocalDebugHud && !LocalDebugHudText.IsEmpty())
		{
			ControlLines.Add(LocalDebugHudText);
		}
		ControlLines.Add(BaseControlsMessage);
		const FString FullControlsMessage = FString::Join(ControlLines, TEXT("\n"));

		ControlsText->SetText(FText::FromString(FullControlsMessage));
	}

	if (CrosshairText)
	{
		CrosshairText->SetText(FText::FromString(TEXT("+")));
	}
}
