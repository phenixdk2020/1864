#pragma once

#include "CoreMinimal.h"
#include "StrategyFireControlTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyFireDiscipline : uint8
{
    HoldFire,
    FireAtWill,
    Volley,
    Independent
};

UENUM(BlueprintType)
enum class EStrategyFirePolicy : uint8
{
    Hold,
    Close,
    Medium,
    Long
};
