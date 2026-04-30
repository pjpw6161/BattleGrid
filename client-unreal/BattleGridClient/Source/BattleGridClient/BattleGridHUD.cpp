#include "BattleGridHUD.h"

#include "BattleGridClientPlayerController.h"

void ABattleGridHUD::DrawHUD()
{
	Super::DrawHUD();

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
		TEXT("BattleGrid Client"),
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
		FString::Printf(TEXT("Score: %d"), BattleGridController->GetScore()),
		FLinearColor::White,
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 2.0f,
		nullptr,
		1.0f
	);

	DrawText(
		TEXT("WASD Move | Mouse Aim | LMB Fire"),
		FLinearColor(0.8f, 0.9f, 1.0f, 1.0f),
		HudX + 20.0f,
		HudY + 15.0f + LineHeight * 3.0f,
		nullptr,
		0.9f
	);

	if (BattleGridController->HasActiveCombatMessage())
	{
		DrawText(
			BattleGridController->GetCombatMessage(),
			FLinearColor(0.2f, 1.0f, 0.35f, 1.0f),
			HudX + 20.0f,
			HudY + 15.0f + LineHeight * 4.0f,
			nullptr,
			1.0f
		);
	}
}
