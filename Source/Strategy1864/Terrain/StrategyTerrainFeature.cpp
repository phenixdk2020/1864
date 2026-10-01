#include "StrategyTerrainFeature.h"

#include "Components/SceneComponent.h"

AStrategyTerrainFeature::AStrategyTerrainFeature()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root =
        CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

float AStrategyTerrainFeature::GetNormalizedRadiusAt(
    const FVector& WorldLocation) const
{
    const FTransform Transform(
        FRotator(0.0f, GetActorRotation().Yaw, 0.0f),
        GetActorLocation());

    const FVector Local =
        Transform.InverseTransformPosition(WorldLocation);

    const float RX = FMath::Max(1.0f, RadiusXcm);
    const float RY = FMath::Max(1.0f, RadiusYcm);

    return FMath::Sqrt(
        FMath::Square(Local.X / RX) +
        FMath::Square(Local.Y / RY));
}

bool AStrategyTerrainFeature::ContainsXY(
    const FVector& WorldLocation) const
{
    return GetNormalizedRadiusAt(WorldLocation) < 1.0f;
}

float AStrategyTerrainFeature::GetHeightOffsetAt(
    const FVector& WorldLocation) const
{
    if (!bAffectsGameplay)
    {
        return 0.0f;
    }

    const float Radius = GetNormalizedRadiusAt(WorldLocation);
    if (Radius >= 1.0f)
    {
        return 0.0f;
    }

    const float OneMinus =
        FMath::Clamp(1.0f - FMath::Square(Radius), 0.0f, 1.0f);

    float Shape = FMath::Square(OneMinus);

    if (FeatureType == EStrategyTerrainFeatureType::Ridge)
    {
        // Broader crown than a round hill while retaining smooth shoulders.
        Shape = FMath::Pow(OneMinus, 1.35f);
    }

    const float SignedHeight =
        FeatureType == EStrategyTerrainFeatureType::Depression
        ? -FMath::Abs(PeakHeightCm)
        : FMath::Abs(PeakHeightCm);

    return SignedHeight * Shape;
}
