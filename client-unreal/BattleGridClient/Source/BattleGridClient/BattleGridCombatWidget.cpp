// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridCombatWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBattleGridCombatWidget::UpdateHud(
	float CurrentHealth,
	float MaxHealth,
	int32 Score,
	int32 TargetScore,
	int32 ServerScore,
	int32 CurrentAmmo,
	int32 MagazineSize,
	bool bIsReloading,
	float LastShotSpreadDegrees,
	const FString& CombatMessage,
	bool bShowCombatMessage,
	bool bHasWon,
	const FString& NetworkStatusText,
	float ServerPositionError,
	bool bHasServerPositionError,
	bool bUseServerCorrection,
	bool bShowScoreboard,
	const FString& ScoreboardText
)
{
	static_cast<void>(ServerPositionError);
	static_cast<void>(bHasServerPositionError);
	static_cast<void>(bUseServerCorrection);

	const float HealthPercent = MaxHealth > 0.0f
		? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f)
		: 0.0f;

	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(
			TEXT("Local HP: %.0f / %.0f"),
			CurrentHealth,
			MaxHealth
		)));
	}

	if (ScoreText)
	{
		const FString AmmoText = bIsReloading
			? FString(TEXT("Reloading..."))
			: FString::Printf(TEXT("%d/%d"), CurrentAmmo, MagazineSize);
		ScoreText->SetText(FText::FromString(FString::Printf(
			TEXT("Local Score: %d / %d | Server Score: %d | Ammo: %s | Spread: %.1f"),
			Score,
			TargetScore,
			ServerScore,
			*AmmoText,
			LastShotSpreadDegrees
		)));
	}

	if (CombatMessageText)
	{
		const FString DisplayMessage = bShowScoreboard
			? ScoreboardText
			: (bHasWon
			? FString(TEXT("Victory! Press F5/Enter to Restart"))
			: CombatMessage);

		CombatMessageText->SetText(FText::FromString(DisplayMessage));
		CombatMessageText->SetVisibility(
			(bShowScoreboard || bShowCombatMessage || bHasWon)
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (ControlsText)
	{
		const FString BaseControlsMessage = bHasWon
			? FString(TEXT("Victory! Press F5/Enter to Restart"))
			: FString(TEXT("WASD Move | Mouse Look | LMB Fire | RMB ADS | Shift Sprint | Space Jump | R Reload | F5/Enter Restart | Tab Scoreboard"));
		const FString FullControlsMessage = NetworkStatusText.IsEmpty()
			? BaseControlsMessage
			: FString::Printf(TEXT("%s\n%s"), *NetworkStatusText, *BaseControlsMessage);

		ControlsText->SetText(FText::FromString(FullControlsMessage));
	}

	if (CrosshairText)
	{
		CrosshairText->SetText(FText::FromString(TEXT("+")));
	}
}
