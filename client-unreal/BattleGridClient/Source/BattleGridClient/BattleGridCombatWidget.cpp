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
	const FString& CombatMessage,
	bool bShowCombatMessage,
	bool bHasWon,
	const FString& NetworkStatusText,
	float ServerPositionError,
	bool bHasServerPositionError,
	bool bUseServerCorrection
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
		ScoreText->SetText(FText::FromString(FString::Printf(
			TEXT("Local Score: %d / %d | Server Score: %d"),
			Score,
			TargetScore,
			ServerScore
		)));
	}

	if (CombatMessageText)
	{
		const FString DisplayMessage = bHasWon
			? FString(TEXT("Victory! Press R to Restart"))
			: CombatMessage;

		CombatMessageText->SetText(FText::FromString(DisplayMessage));
		CombatMessageText->SetVisibility(
			(bShowCombatMessage || bHasWon) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	if (ControlsText)
	{
		const FString BaseControlsMessage = bHasWon
			? FString(TEXT("Victory! Press R to Restart"))
			: FString(TEXT("WASD Move | Mouse Look | LMB Fire | RMB ADS | Shift Sprint | Space Jump | R Restart"));
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
