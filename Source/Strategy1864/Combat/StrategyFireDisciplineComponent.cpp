#include "StrategyFireDisciplineComponent.h"

UStrategyFireDisciplineComponent::UStrategyFireDisciplineComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

int32 UStrategyFireDisciplineComponent::CalculateShotBudget(
    int32 Strength,
    int32 MaxShots,
    int32 Ammunition,
    float DistanceCm,
    float ActiveRangeCm) const
{
    if (Discipline == EStrategyFireDiscipline::HoldFire ||
        Strength <= 0 ||
        MaxShots <= 0 ||
        Ammunition <= 0)
    {
        return 0;
    }

    float Fraction = 1.0f;

    if (Discipline == EStrategyFireDiscipline::Independent)
    {
        Fraction *= FMath::Clamp(IndependentFireFraction, 0.05f, 1.0f);
    }

    if (bConserveAmmunition)
    {
        const float SafeRange = FMath::Max(1.0f, ActiveRangeCm);
        const float RangeFraction =
            FMath::Clamp(DistanceCm / SafeRange, 0.0f, 1.0f);

        if (RangeFraction < ConservationMinimumRangeFraction)
        {
            return 0;
        }

        Fraction *= FMath::Clamp(ConservationShotFraction, 0.05f, 1.0f);
    }

    const int32 Desired =
        FMath::Max(
            1,
            FMath::RoundToInt(
                static_cast<float>(FMath::Min(Strength, MaxShots)) *
                Fraction));

    return FMath::Min(Desired, Ammunition);
}

float UStrategyFireDisciplineComponent::GetReloadMultiplier() const
{
    if (Discipline == EStrategyFireDiscipline::Independent)
    {
        return FMath::Max(0.10f, IndependentReloadMultiplier);
    }

    return 1.0f;
}
