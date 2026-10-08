#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyFormationTypes.h"
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

    /** Formation at the destination; a marching column displays its battle footprint. */
    EStrategyFormationType GetDestinationFormation() const;

private:
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    bool IsMovementMission(EStrategyOrderType Type) const;
    void ApplyInitialMovementFormation(const FStrategyOrder& Order);
    void EvaluateEarlyDeployment();
    AStrategyUnit* FindNearestEnemy(float& OutDistanceCm) const;
    /** Infantry companies and cavalry change formation on the march; guns, headquarters and trains do not. */
    bool MarchesInColumn() const;
    bool IsColumn(EStrategyFormationType Formation) const;
    EStrategyFormationType ColumnFormation() const;
    float DeployDistanceCm(const AStrategyUnit* Enemy) const;
    /** Back from column into the formation it fought in before the march. */
    void Deploy();

    /** The formation before the march (line, or square, or the cavalry's line). */
    EStrategyFormationType BattleFormation = EStrategyFormationType::Line;
    bool bMarching = false;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float ThreatScanAccumulator = 0.0f;
};
