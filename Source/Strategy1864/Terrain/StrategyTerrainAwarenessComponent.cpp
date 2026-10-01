#include "StrategyTerrainAwarenessComponent.h"

#include "StrategyTerrainQueryLibrary.h"
#include "../Units/StrategyUnit.h"

UStrategyTerrainAwarenessComponent::UStrategyTerrainAwarenessComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStrategyTerrainAwarenessComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

float UStrategyTerrainAwarenessComponent::GetGroundZ() const
{
    return OwnerUnit
        ? UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(
            OwnerUnit,
            OwnerUnit->GetActorLocation())
        : 0.0f;
}

float UStrategyTerrainAwarenessComponent::GetLocalSlopeDegrees(
    float SampleRadiusCm) const
{
    return OwnerUnit
        ? UStrategyTerrainQueryLibrary::GetLocalSlopeDegrees(
            OwnerUnit,
            OwnerUnit->GetActorLocation(),
            SampleRadiusCm)
        : 0.0f;
}

float UStrategyTerrainAwarenessComponent::GetElevationAdvantageTo(
    const AStrategyUnit* Target) const
{
    if (!OwnerUnit || !IsValid(Target))
    {
        return 0.0f;
    }

    return UStrategyTerrainQueryLibrary::GetElevationAdvantageCm(
        OwnerUnit,
        OwnerUnit->GetActorLocation(),
        Target->GetActorLocation());
}

float UStrategyTerrainAwarenessComponent::GetObservationRangeMultiplierTo(
    const AStrategyUnit* Target) const
{
    if (!OwnerUnit || !IsValid(Target))
    {
        return 1.0f;
    }

    const float AdvantageCm =
        GetElevationAdvantageTo(Target);

    if (AdvantageCm >= 0.0f)
    {
        const float Bonus =
            FMath::Min(
                MaximumHighGroundRangeBonus,
                (AdvantageCm / 1000.0f) *
                HighGroundRangeBonusPer1000Cm);

        return 1.0f + Bonus;
    }

    const float Penalty =
        FMath::Min(
            MaximumLowGroundRangePenalty,
            (FMath::Abs(AdvantageCm) / 1000.0f) *
            HighGroundRangeBonusPer1000Cm);

    return 1.0f - Penalty;
}

bool UStrategyTerrainAwarenessComponent::IsInDeadGroundFrom(
    const FVector& ObserverLocation) const
{
    if (!OwnerUnit)
    {
        return false;
    }

    return UStrategyTerrainQueryLibrary::IsPointInDeadGroundFrom(
        OwnerUnit,
        ObserverLocation,
        OwnerUnit->GetActorLocation(),
        160.0f,
        120.0f);
}

bool UStrategyTerrainAwarenessComponent::IsNearCrestRelativeTo(
    const FVector& ThreatLocation) const
{
    if (!OwnerUnit)
    {
        return false;
    }

    FVector ThreatGround =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerUnit,
            ThreatLocation);

    FVector UnitGround =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerUnit,
            OwnerUnit->GetActorLocation());

    ThreatGround.Z += 160.0f;
    UnitGround.Z += 120.0f;

    FVector CrestPoint;
    float ExcessHeight = 0.0f;

    const bool bHasCrest =
        UStrategyTerrainQueryLibrary::FindCrestPoint(
            OwnerUnit,
            ThreatGround,
            UnitGround,
            CrestPoint,
            ExcessHeight,
            40);

    return bHasCrest &&
        FVector::Dist2D(
            CrestPoint,
            OwnerUnit->GetActorLocation()) <=
            CrestNearDistanceCm;
}

bool UStrategyTerrainAwarenessComponent::IsOnReverseSlopeFrom(
    const FVector& ThreatLocation) const
{
    return IsInDeadGroundFrom(ThreatLocation) &&
        GetLocalSlopeDegrees() > 2.0f;
}

float UStrategyTerrainAwarenessComponent::GetIncomingHitMultiplierFrom(
    const FVector& ThreatLocation) const
{
    if (IsOnReverseSlopeFrom(ThreatLocation))
    {
        return 0.0f;
    }

    if (IsNearCrestRelativeTo(ThreatLocation))
    {
        return FMath::Clamp(
            CrestExposureHitMultiplier,
            0.25f,
            1.0f);
    }

    return 1.0f;
}
