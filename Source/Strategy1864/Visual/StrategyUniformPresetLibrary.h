#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyUniformPresetLibrary.generated.h"

UCLASS()
class STRATEGY1864_API UStrategyUniformPresetLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Uniform")
    static FStrategyUniformPreset MakeNeutralQAPreset();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Uniform")
    static FStrategyUniformPreset MakeDanishQAPreset();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Uniform")
    static FStrategyUniformPreset MakePrussianQAPreset();

    UFUNCTION(BlueprintPure, Category="Strategy|Visual|Uniform")
    static FStrategyUniformPreset MakeArtilleryQAPreset();
};
