#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyConditionComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyConditionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyConditionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Condition")
    float MovingFatiguePerSecond = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Condition")
    float ReformFatiguePerSecond = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Condition")
    float CombatFatiguePerSecond = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Condition")
    float RecoveryPerSecond = 0.12f;

    UFUNCTION(BlueprintPure, Category="Strategy|Condition")
    float GetMovementSpeedMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Condition")
    float GetAccuracyMultiplier() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Condition")
    float GetMoraleShockMultiplier() const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
