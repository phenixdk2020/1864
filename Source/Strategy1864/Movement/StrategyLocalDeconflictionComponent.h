#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyLocalDeconflictionComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyLocalDeconflictionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyLocalDeconflictionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float MinimumCompanyCenterSeparationCm = 6800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float MaxSidestepCmPerSecond = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float EvaluationIntervalSeconds = 0.10f;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float EvaluationAccumulator = 0.0f;
};
