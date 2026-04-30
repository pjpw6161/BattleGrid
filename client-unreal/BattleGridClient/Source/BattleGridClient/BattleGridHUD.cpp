#include "BattleGridHUD.h"

void ABattleGridHUD::DrawHUD()
{
    Super::DrawHUD();

    DrawRect(
        FLinearColor(0.0f, 0.0f, 0.0f, 0.6f),
        30.0f,
        30.0f,
        600.0f,
        70.0f
    );

    DrawText(
        TEXT("BattleGrid Client - Offline Setup"),
        FLinearColor::Yellow,
        50.0f,
        45.0f,
        nullptr,
        1.8f
    );
}