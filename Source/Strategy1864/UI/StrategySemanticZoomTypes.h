#pragma once

#include "CoreMinimal.h"
#include "StrategySemanticZoomTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategySemanticZoomState : uint8
{
    Close,
    Medium,
    Operational,
    Strategic,
    VeryFar
};
