#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Templates/SubclassOf.h"
#include "BattleGridHUD.generated.h"

class UBattleGridCombatWidget;

UCLASS(Blueprintable)
class BATTLEGRIDCLIENT_API ABattleGridHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABattleGridHUD();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BattleGrid|HUD", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UBattleGridCombatWidget> CombatWidgetClass;

	UPROPERTY(Transient)
	UBattleGridCombatWidget* CombatWidget;
};
