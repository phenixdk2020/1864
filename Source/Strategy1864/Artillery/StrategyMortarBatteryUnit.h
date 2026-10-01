#pragma once

#include "CoreMinimal.h"
#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyMortarBatteryUnit.generated.h"

class UStrategyMortarDeploymentComponent;
class UStrategyMortarFireComponent;

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyMortarBatteryUnit : public AStrategyArtilleryBatteryUnit
{
    GENERATED_BODY()

public:
    AStrategyMortarBatteryUnit();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    TObjectPtr<UStrategyMortarDeploymentComponent> MortarDeploymentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Mortar")
    TObjectPtr<UStrategyMortarFireComponent> MortarFireComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 MortarPieceCount = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Mortar")
    int32 MortarCrewStrength = 32;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    bool IsMortarCombatReady() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Mortar")
    int32 GetOperationalMortarCount() const;
};
