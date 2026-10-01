#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyCavalryScreenAIComponent.generated.h"

class ACavalryUnit;
class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyCavalryScreenAIComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyCavalryScreenAIComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry AI")
    float EvaluationIntervalSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry AI")
    float DetectionRangeCm = 30000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry AI")
    float ScreenDistanceFromParentCm = 12000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cavalry AI")
    float RepositionThresholdCm = 2500.0f;

private:
    AStrategyUnit* FindNearestVisibleEnemy() const;
    bool HasProtectedMission() const;

    UPROPERTY()
    TObjectPtr<ACavalryUnit> OwnerCavalry;

    FVector LastScreenGoal = FVector::ZeroVector;
    float EvaluationAccumulator = 0.0f;
};
