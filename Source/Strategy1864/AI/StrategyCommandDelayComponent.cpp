#include "StrategyCommandDelayComponent.h"

#include "StrategyOfficerProfileComponent.h"
#include "StrategyNCOComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Units/StrategyHQUnit.h"
#include "../Units/StrategyUnit.h"

UStrategyCommandDelayComponent::UStrategyCommandDelayComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

float UStrategyCommandDelayComponent::CalculateDelayFromCurrentParent() const
{
    const AStrategyUnit* Unit = Cast<AStrategyUnit>(GetOwner());
    if (!Unit || !Unit->CommandComponent)
    {
        return 0.0f;
    }

    const AStrategyHQUnit* Parent =
        Cast<AStrategyHQUnit>(Unit->CommandComponent->CurrentCommandParent);

    if (!Parent)
    {
        return 0.0f;
    }

    const float DistanceCm =
        FVector::Dist2D(Unit->GetActorLocation(), Parent->GetActorLocation());

    float Delay = InnerBandDelaySeconds;

    if (DistanceCm > Parent->CommandOuterRadius)
    {
        Delay = OutsideBandDelaySeconds +
            ((DistanceCm - Parent->CommandOuterRadius) / 10000.0f) *
            ExtraSecondsPer100mOutside;
    }
    else if (DistanceCm > Parent->CommandInnerRadius)
    {
        Delay = OuterBandDelaySeconds;
    }

    float Efficiency = 0.5f;

    if (Parent->OfficerProfileComponent)
    {
        Efficiency =
            Parent->OfficerProfileComponent->GetCommandEfficiency();
    }

    const float EfficiencyFactor =
        FMath::Lerp(1.35f, 0.65f, Efficiency);

    const float LocalResponseFactor =
        Unit->NCOComponent
        ? Unit->NCOComponent->GetResponseTimeMultiplier()
        : 1.0f;

    return FMath::Max(
        0.0f,
        Delay * EfficiencyFactor * LocalResponseFactor);
}
