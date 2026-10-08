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
    float CavalryThreatDistanceCm = 35000.0f;

    // 1200 cm/s charge * 25 s preparation = 300 m warning.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float SquareFormationTimeSeconds = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float MinimumCavalryClosingSpeedCmPerSecond = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float MinimumApproachDot = 0.95f;

    // Fixed corridor avoids a wide cone triggering neighbouring companies.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float ApproachCorridorHalfWidthCm = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Threat")
    float PlayerOrderThreatDistanceScale = 0.65f;

    // Called before automatic column deployment; true also while awaiting release.
    bool ReactToCavalryThreat();

    UFUNCTION(BlueprintCallable, Category="Strategy|Threat")
    void NotifyPlayerFormationOrder(EStrategyFormationType RequestedFormation);

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
    bool bHasPlayerFormationOrder = false;
    bool bOwnsSquareBayonets = false;
    EStrategyFormationType PreThreatFormation = EStrategyFormationType::Line;
};
