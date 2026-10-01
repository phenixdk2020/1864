#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategySemanticZoomTypes.h"
#include "StrategySemanticZoomComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategySemanticZoomComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategySemanticZoomComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Semantic Zoom")
    float MediumThresholdCm = 13500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Semantic Zoom")
    float OperationalThresholdCm = 23500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Semantic Zoom")
    float StrategicThresholdCm = 39000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Semantic Zoom")
    float VeryFarThresholdCm = 52500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Semantic Zoom")
    float EvaluationIntervalSeconds = 0.10f;

private:
    EStrategySemanticZoomState ResolveZoomState() const;
    void ApplyZoomState(EStrategySemanticZoomState NewState);

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float EvaluationAccumulator = 0.0f;
};
