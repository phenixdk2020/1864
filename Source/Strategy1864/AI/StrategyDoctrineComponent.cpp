#include "StrategyDoctrineComponent.h"

#include "StrategyOfficerProfileComponent.h"

UStrategyDoctrineComponent::UStrategyDoctrineComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

float UStrategyDoctrineComponent::GetEffectiveAggression(
    const UStrategyOfficerProfileComponent* OfficerProfile) const
{
    const float OfficerAggression =
        OfficerProfile
        ? FMath::Clamp(
            OfficerProfile->Aggression * 0.80f +
            (100.0f - OfficerProfile->Caution) * 0.20f,
            0.0f,
            100.0f)
        : 50.0f;

    float Result =
        OfficerAggression * 0.70f +
        FMath::Clamp(CommanderOrderAggression, 0.0f, 100.0f) * 0.30f;

    switch (Doctrine)
    {
        case EStrategyDoctrine::Defensive:
            Result -= 12.0f;
            break;

        case EStrategyDoctrine::Offensive:
            Result += 12.0f;
            break;

        case EStrategyDoctrine::Balanced:
        default:
            break;
    }

    return FMath::Clamp(Result, 0.0f, 100.0f);
}

float UStrategyDoctrineComponent::GetPreferredEngagementRangeFraction(
    const UStrategyOfficerProfileComponent* OfficerProfile) const
{
    const float Aggression01 =
        GetEffectiveAggression(OfficerProfile) / 100.0f;

    return FMath::Lerp(0.92f, 0.52f, Aggression01);
}
