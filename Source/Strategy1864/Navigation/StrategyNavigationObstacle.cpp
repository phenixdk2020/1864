#include "StrategyNavigationObstacle.h"
#include "Components/SceneComponent.h"

AStrategyNavigationObstacle::AStrategyNavigationObstacle()
{
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    PrimaryActorTick.bCanEverTick = false;
}

FBox AStrategyNavigationObstacle::GetExpandedBounds() const
{
    const FVector Extent =
        HalfExtentCm + FVector(ClearanceCm, ClearanceCm, 0.0f);

    return FBox(
        GetActorLocation() - Extent,
        GetActorLocation() + Extent);
}

bool AStrategyNavigationObstacle::IntersectsSegment2D(
    const FVector& Start,
    const FVector& End) const
{
    const FBox Bounds = GetExpandedBounds();
    const FVector Delta = End - Start;

    float TMin = 0.0f;
    float TMax = 1.0f;

    const auto ClipAxis =
        [&TMin, &TMax](float StartValue, float DeltaValue, float MinValue, float MaxValue)
        {
            if (FMath::Abs(DeltaValue) <= KINDA_SMALL_NUMBER)
            {
                return StartValue >= MinValue && StartValue <= MaxValue;
            }

            const float InvDelta = 1.0f / DeltaValue;
            float T1 = (MinValue - StartValue) * InvDelta;
            float T2 = (MaxValue - StartValue) * InvDelta;

            if (T1 > T2)
            {
                Swap(T1, T2);
            }

            TMin = FMath::Max(TMin, T1);
            TMax = FMath::Min(TMax, T2);
            return TMin <= TMax;
        };

    return ClipAxis(Start.X, Delta.X, Bounds.Min.X, Bounds.Max.X) &&
           ClipAxis(Start.Y, Delta.Y, Bounds.Min.Y, Bounds.Max.Y);
}

FVector AStrategyNavigationObstacle::BuildDetourPoint(
    const FVector& Start,
    const FVector& End) const
{
    FVector Direction = End - Start;
    Direction.Z = 0.0f;
    Direction = Direction.GetSafeNormal();

    if (Direction.IsNearlyZero())
    {
        Direction = FVector::ForwardVector;
    }

    const FVector Right(-Direction.Y, Direction.X, 0.0f);
    const FVector Center = GetActorLocation();

    const FVector CandidateA =
        Center + Right * (HalfExtentCm.Size2D() + ClearanceCm);
    const FVector CandidateB =
        Center - Right * (HalfExtentCm.Size2D() + ClearanceCm);

    const float CostA =
        FVector::DistSquared2D(Start, CandidateA) +
        FVector::DistSquared2D(CandidateA, End);

    const float CostB =
        FVector::DistSquared2D(Start, CandidateB) +
        FVector::DistSquared2D(CandidateB, End);

    return CostA <= CostB ? CandidateA : CandidateB;
}
