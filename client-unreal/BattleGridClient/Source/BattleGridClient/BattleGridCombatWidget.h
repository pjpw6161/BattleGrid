// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleGridNetworkSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "BattleGridCombatWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UBorder;

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
		const FString& ServerGameOverText,
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
		const FString& TopFiveRankingText,
		bool bShowRanking,
		const FString& ServerCombatEventFeedText,
		const FString& ServerHealMessageText,
		const FString& WeaponStatusMessageText,
		const FString& ServerShotResultText,
		const TArray<FBattleGridKillFeedLine>& KillFeedLines,
		bool bShowKillFeed,
		float CurrentSpreadDegrees,
		bool bCrosshairADS,
		bool bCrosshairSprinting,
		bool bCrosshairJumping,
		bool bCrosshairReloading,
		bool bCrosshairServerDead,
		bool bShowCrosshair,
		const FString& LocalDebugHudText,
		const FString& GameplayHpText,
		const FString& GameplayAmmoText,
		const FString& GameplayMatchText,
		const FString& GameplayKdText,
		const FString& SmallServerStatusDisplayText,
		const FString& DebugHudText,
		bool bUseGameplayHudLayout,
		bool bShowDebugHud,
		bool bShowControlsHelp,
		bool bShowSmallServerStatus
	);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateKillFeed(const TArray<FBattleGridKillFeedLine>& Lines, bool bShowKillFeed);
	void UpdateCrosshair(
		float DeltaTime,
		float SpreadDegrees,
		bool bADS,
		bool bSprinting,
		bool bJumping,
		bool bReloading,
		bool bServerDead,
		bool bShowCrosshair
	);

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ScoreText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MatchText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KdText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SmallServerStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DebugText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CombatMessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ControlsText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CrosshairText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RankingText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillFeedLine1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillFeedLine2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillFeedLine3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillFeedLine4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillFeedLine5;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CrosshairTop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CrosshairBottom;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CrosshairLeft;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CrosshairRight;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CrosshairCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CrosshairMinGap = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CrosshairMaxGap = 46.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CrosshairInterpSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true"))
	FLinearColor CrosshairNormalColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true"))
	FLinearColor CrosshairAdsColor = FLinearColor(0.6f, 1.0f, 0.6f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true"))
	FLinearColor CrosshairBadAccuracyColor = FLinearColor(1.0f, 0.75f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattleGrid|Crosshair", meta = (AllowPrivateAccess = "true"))
	FLinearColor CrosshairUnavailableColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.35f);

	float CurrentCrosshairGap = 20.0f;
	float LatestCrosshairSpreadDegrees = 3.5f;
	bool bLatestCrosshairADS = false;
	bool bLatestCrosshairSprinting = false;
	bool bLatestCrosshairJumping = false;
	bool bLatestCrosshairReloading = false;
	bool bLatestCrosshairServerDead = false;
	bool bLatestShowCrosshair = true;
};
