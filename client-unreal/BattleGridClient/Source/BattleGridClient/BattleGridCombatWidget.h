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
		const FString& LocalDebugHudText
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
