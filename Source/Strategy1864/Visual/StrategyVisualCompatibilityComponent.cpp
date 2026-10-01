#include "StrategyVisualCompatibilityComponent.h"

UStrategyVisualCompatibilityComponent::UStrategyVisualCompatibilityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UStrategyVisualCompatibilityComponent::ValidateProfile(
    const FStrategyHumanVisualProfile& Profile,
    FString& OutFailureReason) const
{
    if (Profile.SharedSkeletonId != RequiredSkeletonId)
    {
        OutFailureReason =
            FString::Printf(
                TEXT("Skeleton mismatch: expected %s, got %s"),
                *RequiredSkeletonId.ToString(),
                *Profile.SharedSkeletonId.ToString());

        return false;
    }

    if (Profile.SharedAnimationSetId != RequiredAnimationSetId)
    {
        OutFailureReason =
            FString::Printf(
                TEXT("Animation set mismatch: expected %s, got %s"),
                *RequiredAnimationSetId.ToString(),
                *Profile.SharedAnimationSetId.ToString());

        return false;
    }

    OutFailureReason.Reset();
    return true;
}
