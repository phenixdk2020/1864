#include "StrategyOfficerProfileComponent.h"

UStrategyOfficerProfileComponent::UStrategyOfficerProfileComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

float UStrategyOfficerProfileComponent::GetCommandEfficiency() const
{
    return FMath::Clamp(
        (Leadership * 0.25f +
         Initiative * 0.20f +
         StaffQuality * 0.25f +
         TacticalSkill * 0.15f +
         Discipline * 0.15f) / 100.0f,
        0.10f,
        1.0f);
}

float UStrategyOfficerProfileComponent::GetStressReactionMultiplier() const
{
    const float Stability =
        FMath::Clamp(
            (Composure * 0.70f + Experience * 0.30f) / 100.0f,
            0.0f,
            1.0f);

    return FMath::Lerp(1.30f, 0.75f, Stability);
}

float UStrategyOfficerProfileComponent::GetDecisionStability() const
{
    return FMath::Clamp(
        (Composure * 0.40f +
         Experience * 0.30f +
         Discipline * 0.20f +
         TacticalSkill * 0.10f) / 100.0f,
        0.0f,
        1.0f);
}
