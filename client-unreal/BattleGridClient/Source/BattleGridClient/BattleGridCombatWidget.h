// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleGridCombatWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API UBattleGridCombatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "BattleGrid|HUD")
	void UpdateHud(
		float CurrentHealth,
		float MaxHealth,
		int32 Score,
		int32 TargetScore,
		const FString& CombatMessage,
		bool bShowCombatMessage,
		bool bHasWon
	);

private:
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ScoreText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CombatMessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ControlsText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CrosshairText;
};
