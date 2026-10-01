#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyHumanVisualTypes.h"
#include "StrategyHorseAnimationStateComponent.generated.h"

class ACavalryUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyHorseAnimationStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyHorseAnimationStateComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Horse")
    EStrategyHorseGait CurrentGait = EStrategyHorseGait::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Horse")
    float WalkThresholdCmPerSecond = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Horse")
    float TrotThresholdCmPerSecond = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Horse")
    float CanterThresholdCmPerSecond = 650.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visual|Horse")
    float GallopThresholdCmPerSecond = 900.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Visual|Horse")
    float MeasuredSpeedCmPerSecond = 0.0f;

private:
    void RefreshGait();

    UPROPERTY()
    TObjectPtr<ACavalryUnit> OwnerCavalry;

    FVector PreviousLocation = FVector::ZeroVector;
};
