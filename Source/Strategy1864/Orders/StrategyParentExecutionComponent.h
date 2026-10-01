#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyParentExecutionComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyParentExecutionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyParentExecutionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Orders")
    float EvaluationIntervalSeconds = 0.20f;

private:
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    bool HasExecutingSubordinates() const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float EvaluationAccumulator = 0.0f;
};
