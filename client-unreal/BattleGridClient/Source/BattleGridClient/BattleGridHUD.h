#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BattleGridHUD.generated.h"

UCLASS()
class BATTLEGRIDCLIENT_API ABattleGridHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};