// Copyright Epic Games, Inc. All Rights Reserved.

#include "BattleGridCombatWidget.h"

#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace
{
FString ToGamePrototypeHudText(const FString& Text)
{
	FString Result = Text;
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
	const bool bEffectiveShowDebugHud = bShowDebugHud || bShowLocalDebugHud;

	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	if (HealthText)
	{
		if (bUseGameplayHudLayout && !GameplayHpText.IsEmpty())
		{
			HealthText->SetText(FText::FromString(GameplayHpText));
		}
		else if (bShowServerHudValues)
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
		const FString AmmoValueText = bIsReloading
			? FString(TEXT("Reloading..."))
			: FString::Printf(TEXT("%d/%d"), CurrentAmmo, MagazineSize);
		if (bUseGameplayHudLayout)
		{
			const FString CompactAmmoText = GameplayAmmoText.IsEmpty()
				? FString::Printf(TEXT("Ammo %s"), *AmmoValueText)
				: GameplayAmmoText;
			TArray<FString> ScoreParts;
			if (this->AmmoText == nullptr && !CompactAmmoText.IsEmpty())
			{
				ScoreParts.Add(CompactAmmoText);
			}
			if (KdText == nullptr && !GameplayKdText.IsEmpty())
			{
				ScoreParts.Add(GameplayKdText);
			}
			if (MatchText == nullptr && !GameplayMatchText.IsEmpty())
			{
				ScoreParts.Add(GameplayMatchText);
			}
			const FString GameplayScoreText = FString::Join(ScoreParts, TEXT(" | "));
			ScoreText->SetText(FText::FromString(GameplayScoreText));
			ScoreText->SetVisibility(
				!GameplayScoreText.IsEmpty()
					? ESlateVisibility::Visible
					: ESlateVisibility::Collapsed
			);
		}
		else if (bShowServerHudValues)
		{
			ScoreText->SetText(FText::FromString(FString::Printf(
				TEXT("SERVER Kills: %d / %d | K/D %d/%d | Bots %d | PvP %d | Ammo %s"),
				ServerScore,
				ServerTargetScore,
				ServerKills,
				ServerDeaths,
				ServerBotKills,
				ServerPlayerKills,
				*AmmoValueText
			)));
			ScoreText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ScoreText->SetText(FText::FromString(FString::Printf(
				TEXT("Local Score: %d / %d | Ammo: %s | Spread: %.1f"),
				Score,
				TargetScore,
				*AmmoValueText,
				LastShotSpreadDegrees
			)));
			ScoreText->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (AmmoText != nullptr)
	{
		AmmoText->SetText(FText::FromString(GameplayAmmoText));
		AmmoText->SetVisibility(
			bUseGameplayHudLayout && !GameplayAmmoText.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (MatchText != nullptr)
	{
		MatchText->SetText(FText::FromString(GameplayMatchText));
		MatchText->SetVisibility(
			bUseGameplayHudLayout && !GameplayMatchText.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (KdText != nullptr)
	{
		KdText->SetText(FText::FromString(GameplayKdText));
		KdText->SetVisibility(
			bUseGameplayHudLayout && !GameplayKdText.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (SmallServerStatusText != nullptr)
	{
		SmallServerStatusText->SetText(FText::FromString(SmallServerStatusDisplayText));
		SmallServerStatusText->SetVisibility(
			bUseGameplayHudLayout && bShowSmallServerStatus && !SmallServerStatusDisplayText.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (DebugText != nullptr)
	{
		DebugText->SetText(FText::FromString(DebugHudText));
		DebugText->SetVisibility(
			bEffectiveShowDebugHud && !DebugHudText.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
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
		else if (bServerInvincible && !ServerLifeStateText.IsEmpty())
		{
			DisplayMessage = ServerLifeStateText;
		}
		else if (!ServerHealMessageText.IsEmpty())
		{
			DisplayMessage = ServerHealMessageText;
		}
		else if (!WeaponStatusMessageText.IsEmpty())
		{
			DisplayMessage = WeaponStatusMessageText;
		}
		else if (!ServerShotResultText.IsEmpty())
		{
			DisplayMessage = ServerShotResultText;
		}
		else if (bServerGameOver)
		{
			DisplayMessage = TEXT("SERVER GAME OVER");
		}
		else if (bHasWon && !bUseServerAuthoritativeHud)
		{
			DisplayMessage = TEXT("Victory! Press F5/Enter to Restart");
		}
		else if (bShowCombatEventFeed && !ServerCombatEventFeedText.IsEmpty())
		{
			DisplayMessage = FString::Printf(
				TEXT("Events:\n%s"),
				*ServerCombatEventFeedText
			);
		}
		else if (bShowCombatMessage && (!bUseGameplayHudLayout || !bUseServerAuthoritativeHud))
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
		const bool bHasSmallStatusWidget = SmallServerStatusText != nullptr;
		const bool bHasDebugWidget = DebugText != nullptr;
		if (!bUseGameplayHudLayout && !PolishedNetworkStatusText.IsEmpty())
		{
			ControlLines.Add(PolishedNetworkStatusText);
		}
		else if (
			bUseGameplayHudLayout
			&& bShowSmallServerStatus
			&& !bHasSmallStatusWidget
			&& !SmallServerStatusDisplayText.IsEmpty()
		)
		{
			ControlLines.Add(SmallServerStatusDisplayText);
		}
		if (!bUseGameplayHudLayout && bShowLocalDebugHud && !LocalDebugHudText.IsEmpty())
		{
			ControlLines.Add(LocalDebugHudText);
		}
		if (bUseGameplayHudLayout && bEffectiveShowDebugHud && !bHasDebugWidget && !DebugHudText.IsEmpty())
		{
			ControlLines.Add(DebugHudText);
		}
		if (bShowRanking && RankingText == nullptr && !TopFiveRankingText.IsEmpty())
		{
			ControlLines.Add(ToGamePrototypeHudText(TopFiveRankingText));
		}
		const bool bHasKillFeedWidgets =
			KillFeedLine1.Get() != nullptr
			|| KillFeedLine2.Get() != nullptr
			|| KillFeedLine3.Get() != nullptr
			|| KillFeedLine4.Get() != nullptr
			|| KillFeedLine5.Get() != nullptr;
		if (bShowKillFeed && !bHasKillFeedWidgets && KillFeedLines.Num() > 0)
		{
			TArray<FString> KillFeedTextLines;
			KillFeedTextLines.Add(TEXT("Kill Log:"));
			for (const FBattleGridKillFeedLine& Line : KillFeedLines)
			{
				KillFeedTextLines.Add(Line.Text);
			}
			ControlLines.Add(FString::Join(KillFeedTextLines, TEXT("\n")));
		}
		if (!bUseGameplayHudLayout || bShowControlsHelp)
		{
			ControlLines.Add(BaseControlsMessage);
		}
		const FString FullControlsMessage = FString::Join(ControlLines, TEXT("\n"));

		ControlsText->SetText(FText::FromString(FullControlsMessage));
		ControlsText->SetVisibility(
			!FullControlsMessage.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	if (RankingText != nullptr)
	{
		RankingText->SetText(FText::FromString(ToGamePrototypeHudText(TopFiveRankingText)));
		RankingText->SetVisibility(
			bShowRanking && !TopFiveRankingText.IsEmpty()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}

	UpdateKillFeed(KillFeedLines, bShowKillFeed);

	LatestCrosshairSpreadDegrees = CurrentSpreadDegrees;
	bLatestCrosshairADS = bCrosshairADS;
	bLatestCrosshairSprinting = bCrosshairSprinting;
	bLatestCrosshairJumping = bCrosshairJumping;
	bLatestCrosshairReloading = bCrosshairReloading;
	bLatestCrosshairServerDead = bCrosshairServerDead;
	bLatestShowCrosshair = bShowCrosshair;

	if (CrosshairText)
	{
		const bool bHasDynamicCrosshair =
			CrosshairTop.Get() != nullptr
			|| CrosshairBottom.Get() != nullptr
			|| CrosshairLeft.Get() != nullptr
			|| CrosshairRight.Get() != nullptr
			|| CrosshairCenter.Get() != nullptr;
		CrosshairText->SetText(FText::FromString(TEXT("+")));
		CrosshairText->SetVisibility(
			bShowCrosshair && !bHasDynamicCrosshair
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	}
}

void UBattleGridCombatWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime
)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateCrosshair(
		InDeltaTime,
		LatestCrosshairSpreadDegrees,
		bLatestCrosshairADS,
		bLatestCrosshairSprinting,
		bLatestCrosshairJumping,
		bLatestCrosshairReloading,
		bLatestCrosshairServerDead,
		bLatestShowCrosshair
	);
}

void UBattleGridCombatWidget::UpdateKillFeed(
	const TArray<FBattleGridKillFeedLine>& Lines,
	bool bShowKillFeed
)
{
	UTextBlock* Widgets[] = {
		KillFeedLine1.Get(),
		KillFeedLine2.Get(),
		KillFeedLine3.Get(),
		KillFeedLine4.Get(),
		KillFeedLine5.Get()
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Widgets); ++Index)
	{
		UTextBlock* LineWidget = Widgets[Index];
		if (!LineWidget)
		{
			continue;
		}

		if (bShowKillFeed && Lines.IsValidIndex(Index) && !Lines[Index].Text.IsEmpty())
		{
			LineWidget->SetText(FText::FromString(Lines[Index].Text));
			LineWidget->SetColorAndOpacity(FSlateColor(Lines[Index].Color));
			LineWidget->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			LineWidget->SetText(FText::GetEmpty());
			LineWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UBattleGridCombatWidget::UpdateCrosshair(
	float DeltaTime,
	float SpreadDegrees,
	bool bADS,
	bool bSprinting,
	bool bJumping,
	bool bReloading,
	bool bServerDead,
	bool bShowCrosshair
)
{
	UBorder* Widgets[] = {
		CrosshairTop.Get(),
		CrosshairBottom.Get(),
		CrosshairLeft.Get(),
		CrosshairRight.Get(),
		CrosshairCenter.Get()
	};

	bool bHasDynamicCrosshair = false;
	for (UBorder* Widget : Widgets)
	{
		if (Widget)
		{
			bHasDynamicCrosshair = true;
			break;
		}
	}

	FLinearColor CrosshairColor = CrosshairNormalColor;
	if (bReloading || bServerDead)
	{
		CrosshairColor = CrosshairUnavailableColor;
	}
	else if (bJumping || bSprinting)
	{
		CrosshairColor = CrosshairBadAccuracyColor;
	}
	else if (bADS)
	{
		CrosshairColor = CrosshairAdsColor;
	}

	const float MinGap = FMath::Max(0.0f, CrosshairMinGap);
	const float MaxGap = FMath::Max(MinGap, CrosshairMaxGap);
	const float TargetGap = FMath::GetMappedRangeValueClamped(
		FVector2D(0.8f, 9.0f),
		FVector2D(MinGap, MaxGap),
		FMath::Max(0.0f, SpreadDegrees)
	);

	const float InterpSpeed = FMath::Max(0.0f, CrosshairInterpSpeed);
	CurrentCrosshairGap = (DeltaTime <= 0.0f || InterpSpeed <= 0.0f)
		? TargetGap
		: FMath::FInterpTo(CurrentCrosshairGap, TargetGap, DeltaTime, InterpSpeed);

	if (CrosshairText && !bHasDynamicCrosshair)
	{
		CrosshairText->SetColorAndOpacity(FSlateColor(CrosshairColor));
		CrosshairText->SetVisibility(
			bShowCrosshair
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
		return;
	}

	if (CrosshairText && bHasDynamicCrosshair)
	{
		CrosshairText->SetVisibility(ESlateVisibility::Collapsed);
	}

	auto ApplySegment = [bShowCrosshair, CrosshairColor](UBorder* Segment, const FVector2D& Translation)
	{
		if (!Segment)
		{
			return;
		}

		Segment->SetBrushColor(CrosshairColor);
		Segment->SetRenderTranslation(Translation);
		Segment->SetVisibility(
			bShowCrosshair
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed
		);
	};

	ApplySegment(CrosshairTop.Get(), FVector2D(0.0f, -CurrentCrosshairGap));
	ApplySegment(CrosshairBottom.Get(), FVector2D(0.0f, CurrentCrosshairGap));
	ApplySegment(CrosshairLeft.Get(), FVector2D(-CurrentCrosshairGap, 0.0f));
	ApplySegment(CrosshairRight.Get(), FVector2D(CurrentCrosshairGap, 0.0f));
	ApplySegment(CrosshairCenter.Get(), FVector2D::ZeroVector);
}
