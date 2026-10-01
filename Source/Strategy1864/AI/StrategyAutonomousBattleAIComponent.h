#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyAutonomousBattleAIComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyAutonomousBattleAIComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyAutonomousBattleAIComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Battle AI")
    bool bEnableForNonPlayerSides = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Battle AI")
    float EvaluationIntervalSeconds = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Battle AI")
    float DesiredRangeFraction = 0.75f;

private:
    AStrategyUnit* FindCurrentVisibleEnemy() const;
    bool IsAutonomousEnemy() const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    float EvaluationAccumulator = 0.0f;
    FRandomStream DecisionRandom;
};
