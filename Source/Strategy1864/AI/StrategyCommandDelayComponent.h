#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyCommandDelayComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCommandDelayComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCommandDelayComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Command Delay")
    float InnerBandDelaySeconds = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Command Delay")
    float OuterBandDelaySeconds = 0.80f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Command Delay")
    float OutsideBandDelaySeconds = 2.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Command Delay")
    float ExtraSecondsPer100mOutside = 0.35f;

    UFUNCTION(BlueprintPure, Category="Strategy|Command Delay")
    float CalculateDelayFromCurrentParent() const;
};
