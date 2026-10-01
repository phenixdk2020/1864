#include "StrategyDirectionalCoverComponent.h"

#include "../Navigation/StrategyNavigationObstacle.h"
#include "EngineUtils.h"

UStrategyDirectionalCoverComponent::UStrategyDirectionalCoverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

float UStrategyDirectionalCoverComponent::CalculateIncomingHitMultiplier(
    const FVector& ShooterLocation) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !GetWorld())
    {
        return 1.0f;
    }

    const FVector UnitLocation = OwnerActor->GetActorLocation();
    float BestMultiplier = 1.0f;

    for (TActorIterator<AStrategyNavigationObstacle> It(GetWorld()); It; ++It)
    {
        const AStrategyNavigationObstacle* Obstacle = *It;
        if (!IsValid(Obstacle))
        {
            continue;
        }

        const FBox Bounds = Obstacle->GetExpandedBounds();
        const FVector Closest = Bounds.GetClosestPointTo(UnitLocation);

        if (FVector::Dist2D(UnitLocation, Closest) > MaximumCoverUseDistanceCm)
        {
            continue;
        }

        // Cover is directional: the shooter-to-unit line must pass through
        // the obstacle before reaching the unit.
        if (!Obstacle->IntersectsSegment2D(ShooterLocation, UnitLocation))
        {
            continue;
        }

        BestMultiplier =
            FMath::Min(
                BestMultiplier,
                GetObstacleProtectionMultiplier(Obstacle));
    }

    return BestMultiplier;
}

float UStrategyDirectionalCoverComponent::GetObstacleProtectionMultiplier(
    const AStrategyNavigationObstacle* Obstacle) const
{
    if (!Obstacle)
    {
        return 1.0f;
    }

    switch (Obstacle->ObstacleType)
    {
        case EStrategyObstacleType::Building:
            return 0.45f;

        case EStrategyObstacleType::Fieldworks:
            return 0.50f;

        case EStrategyObstacleType::Fence:
            return 0.80f;

        case EStrategyObstacleType::Generic:
        default:
            return 0.75f;
    }
}
