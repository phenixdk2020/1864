#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyAutonomyComponent.generated.h"

UENUM(BlueprintType)
enum class EStrategyAutonomyLevel : uint8
{
    Strict,
    Normal,
    Independent
};

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyAutonomyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyAutonomyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Autonomy")
    EStrategyAutonomyLevel Autonomy = EStrategyAutonomyLevel::Normal;

    UFUNCTION(BlueprintPure, Category="Strategy|Autonomy")
    bool AllowsLocalRetask(bool bHasCommandParent) const
    {
        if (!bHasCommandParent)
        {
            return true;
        }

        return Autonomy == EStrategyAutonomyLevel::Independent;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Autonomy")
    float GetMissionDeviationAllowanceCm() const
    {
        switch (Autonomy)
        {
            case EStrategyAutonomyLevel::Strict:
                return 500.0f;
            case EStrategyAutonomyLevel::Independent:
                return 6000.0f;
            case EStrategyAutonomyLevel::Normal:
            default:
                return 2000.0f;
        }
    }
};
