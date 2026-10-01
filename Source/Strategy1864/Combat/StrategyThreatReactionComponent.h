#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Formations/StrategyFormationTypes.h"
#include "StrategyThreatReactionComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyThreatReactionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyThreatReactionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float CavalryThreatDistanceCm = 12000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float EvaluationIntervalSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float SquareReleaseDelaySeconds = 8.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Threat")
    bool IsRespondingToCavalry() const { return bRespondingToCavalry; }

private:
    AStrategyUnit* FindVisibleEnemyCavalry() const;
    void EnterSquare();
    void TryLeaveSquare(float DeltaTime);

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float EvaluationAccumulator = 0.0f;
    float NoThreatSeconds = 0.0f;
    bool bRespondingToCavalry = false;
    EStrategyFormationType PreThreatFormation = EStrategyFormationType::Line;
};
