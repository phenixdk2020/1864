#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyFormationPolicyComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFormationPolicyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFormationPolicyComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float LongMoveColumnThresholdCm = 12000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float DeploySafetyBufferCm = 3500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Formation")
    float ThreatScanIntervalSeconds = 0.25f;

private:
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    bool IsMovementMission(EStrategyOrderType Type) const;
    void ApplyInitialMovementFormation(const FStrategyOrder& Order);
    void EvaluateEarlyDeployment();
    AStrategyUnit* FindNearestEnemy(float& OutDistanceCm) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float ThreatScanAccumulator = 0.0f;
};
