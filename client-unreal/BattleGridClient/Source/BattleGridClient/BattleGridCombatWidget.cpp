// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridCombatWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBattleGridCombatWidget::UpdateHud(
	float CurrentHealth,
	float MaxHealth,
	int32 Score,
	const FString& CombatMessage,
	bool bShowCombatMessage
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
		ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), Score)));
	}

	if (CombatMessageText)
	{
		CombatMessageText->SetText(FText::FromString(CombatMessage));
		CombatMessageText->SetVisibility(
			bShowCombatMessage ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	if (ControlsText)
	{
		ControlsText->SetText(FText::FromString(TEXT("WASD Move | Mouse Aim | LMB Fire")));
	}

	if (CrosshairText)
	{
		CrosshairText->SetText(FText::FromString(TEXT("+")));
	}
}
