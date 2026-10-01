#pragma once

#include "CoreMinimal.h"
#include "StrategySupplyTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategySupplyOwnershipState : uint8
{
    Operational,
    Abandoned,
    Captured,
    Destroyed
};
