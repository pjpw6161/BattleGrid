// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridCombatWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBattleGridCombatWidget::UpdateHud(
	float CurrentHealth,
	float MaxHealth,
	int32 Score,
	int32 TargetScore,
	const FString& CombatMessage,
	bool bShowCombatMessage,
	bool bHasWon,
	const FString& NetworkStatusText,
	float ServerPositionError,
	bool bHasServerPositionError,
	bool bUseServerCorrection
)
{
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
			TEXT("HP: %.0f / %.0f"),
			CurrentHealth,
			MaxHealth
		)));
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::Printf(
			TEXT("Score: %d / %d"),
			Score,
			TargetScore
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
			: FString(TEXT("WASD Move | Mouse Aim | LMB Fire"));
		const FString ControlsMessage = NetworkStatusText.IsEmpty()
			? BaseControlsMessage
			: FString::Printf(TEXT("%s | %s"), *BaseControlsMessage, *NetworkStatusText);

		const FString CorrectionText = bUseServerCorrection ? TEXT("On") : TEXT("Off");
		const FString FullControlsMessage = bHasServerPositionError
			? FString::Printf(
				TEXT("%s | Error: %.1f | Correction: %s"),
				*ControlsMessage,
				ServerPositionError,
				*CorrectionText
			)
			: FString::Printf(
				TEXT("%s | Correction: %s"),
				*ControlsMessage,
				*CorrectionText
			);

		ControlsText->SetText(FText::FromString(FullControlsMessage));
	}

	if (CrosshairText)
	{
		CrosshairText->SetText(FText::FromString(TEXT("+")));
	}
}
