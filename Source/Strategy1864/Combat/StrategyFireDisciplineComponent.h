#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyFireControlTypes.h"
#include "StrategyFireDisciplineComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyFireDisciplineComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyFireDisciplineComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Discipline")
    EStrategyFireDiscipline Discipline = EStrategyFireDiscipline::Volley;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Discipline")
    bool bConserveAmmunition = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Discipline", meta=(ClampMin="0.05", ClampMax="1.0"))
    float IndependentFireFraction = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Discipline", meta=(ClampMin="0.05", ClampMax="1.0"))
    float ConservationShotFraction = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Discipline", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ConservationMinimumRangeFraction = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fire Discipline")
    float IndependentReloadMultiplier = 0.65f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Discipline")
    void SetDiscipline(EStrategyFireDiscipline NewDiscipline)
    {
        Discipline = NewDiscipline;
    }

    UFUNCTION(BlueprintCallable, Category="Strategy|Fire Discipline")
    void SetConserveAmmunition(bool bConserve)
    {
        bConserveAmmunition = bConserve;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Discipline")
    bool AllowsAutomaticFire() const
    {
        return Discipline != EStrategyFireDiscipline::HoldFire;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Discipline")
    int32 CalculateShotBudget(
        int32 Strength,
        int32 MaxShots,
        int32 Ammunition,
        float DistanceCm,
        float ActiveRangeCm) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fire Discipline")
    float GetReloadMultiplier() const;
};
