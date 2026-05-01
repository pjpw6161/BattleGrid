#include "BattleGridHUD.h"

#include "BattleGridClientPlayerController.h"
#include "BattleGridCombatWidget.h"
#include "Blueprint/UserWidget.h"

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

	CombatWidget->UpdateHud(
		BattleGridController->GetCurrentPlayerHealth(),
		BattleGridController->GetMaxPlayerHealth(),
		BattleGridController->GetScore(),
		BattleGridController->GetTargetScore(),
		BattleGridController->GetCombatMessage(),
		BattleGridController->HasActiveCombatMessage(),
		BattleGridController->HasWon()
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
		360.0f,
		135.0f
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

	DrawText(
		FString::Printf(
			TEXT("HP: %.0f / %.0f"),
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
		FString::Printf(
			TEXT("Score: %d / %d"),
			BattleGridController->GetScore(),
			BattleGridController->GetTargetScore()
		),
		FLinearColor::White,
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 2.0f,
		nullptr,
		1.0f
	);

	DrawText(
		BattleGridController->HasWon()
			? TEXT("Victory! Press R to Restart")
			: TEXT("WASD Move | Mouse Aim | LMB Fire"),
		FLinearColor(0.8f, 0.9f, 1.0f, 1.0f),
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 3.0f,
		nullptr,
		0.9f
	);

	if (BattleGridController->HasActiveCombatMessage() || BattleGridController->HasWon())
	{
		const FString CombatDisplayMessage = BattleGridController->HasWon()
			? FString(TEXT("Victory! Press R to Restart"))
			: BattleGridController->GetCombatMessage();

		DrawText(
			CombatDisplayMessage,
			FLinearColor(0.2f, 1.0f, 0.35f, 1.0f),
			HudX + 20.0f,
			HudY + 15.0f + LineHeight * 4.0f,
			nullptr,
			1.0f
		);
	}
}
