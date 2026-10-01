#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyScenarioStateComponent.generated.h"

UENUM(BlueprintType)
enum class EStrategyBattleOutcome : uint8
{
    InProgress,
    DenmarkVictory,
    OppositionVictory,
    Draw
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FStrategyBattleOutcomeChanged,
    EStrategyBattleOutcome,
    OldOutcome,
    EStrategyBattleOutcome,
    NewOutcome);

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyScenarioStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyScenarioStateComponent();

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Scenario")
    FStrategyBattleOutcomeChanged OnOutcomeChanged;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Scenario")
    EStrategyBattleOutcome Outcome = EStrategyBattleOutcome::InProgress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Scenario")
    float EvaluationIntervalSeconds = 0.5f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Scenario")
    void ResetOutcome();

private:
    void EvaluateOutcome();

    bool bSawFriendlyForce = false;
    bool bSawOppositionForce = false;
    float EvaluationAccumulator = 0.0f;
};
