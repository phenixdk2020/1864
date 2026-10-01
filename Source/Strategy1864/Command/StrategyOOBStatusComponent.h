#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyOOBStatusComponent.generated.h"

class AStrategyUnit;

USTRUCT(BlueprintType)
struct FStrategyOOBAggregateStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 InitialStrength = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentStrength = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 UnitCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RoutedCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DestroyedCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ExecutingCount = 0;
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyOOBStatusComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyOOBStatusComponent();

    UFUNCTION(BlueprintPure, Category="Strategy|OOB")
    FStrategyOOBAggregateStatus CalculateAggregateStatus() const;

private:
    void AccumulateRecursive(
        const AStrategyUnit* Unit,
        TSet<const AStrategyUnit*>& Visited,
        FStrategyOOBAggregateStatus& InOutStatus) const;
};
