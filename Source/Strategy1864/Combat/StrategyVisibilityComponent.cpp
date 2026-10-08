#include "StrategyVisibilityComponent.h"

#include "../Units/StrategyUnit.h"
#include "StrategySmokeField.h"
#include "StrategyCombatComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"

UStrategyVisibilityComponent::UStrategyVisibilityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UStrategyVisibilityComponent::HasLineOfSightTo(const AStrategyUnit* Target) const
{
    return CanDetectTarget(Target, -1.0f);
}

bool UStrategyVisibilityComponent::CanDetectTarget(
    const AStrategyUnit* Target, float SightRangeCm) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !IsValid(Target) || !GetWorld())
    {
        return false;
    }

    if (!PassesForestVisibility(Target->GetActorLocation(), Target, SightRangeCm)) return false;

    FVector Start =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerActor,
            OwnerActor->GetActorLocation());

    FVector End =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerActor,
            Target->GetActorLocation());

    Start.Z += EyeHeightCm;
    End.Z += TargetHeightCm;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StrategyLOS), true);
    QueryParams.AddIgnoredActor(OwnerActor);

    FHitResult Hit;
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        QueryParams);

    const bool bPhysicalLOS =
        !bBlocked || Hit.GetActor() == Target;

    if (!bPhysicalLOS)
    {
        return false;
    }

    if (UStrategyTerrainQueryLibrary::IsTerrainProfileOccluded(
        OwnerActor,
        Start,
        End,
        15.0f,
        40))
    {
        return false;
    }

    return GetSmokeTransmissionTo(Target) >=
        MinimumSmokeTransmissionForLOS;
}

bool UStrategyVisibilityComponent::HasLineOfSightToLocation(
    const FVector& TargetLocation,
    float LocationTargetHeightCm) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !GetWorld())
    {
        return false;
    }

    if (!PassesForestVisibility(TargetLocation)) return false;

    FVector Start =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerActor,
            OwnerActor->GetActorLocation());

    FVector End =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerActor,
            TargetLocation);

    Start.Z += EyeHeightCm;
    End.Z += FMath::Max(0.0f, LocationTargetHeightCm);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(StrategyLocationLOS),
        true);
    QueryParams.AddIgnoredActor(OwnerActor);

    FHitResult Hit;
    const bool bBlocked =
        GetWorld()->LineTraceSingleByChannel(
            Hit,
            Start,
            End,
            ECC_Visibility,
            QueryParams);

    if (bBlocked)
    {
        return false;
    }

    return !UStrategyTerrainQueryLibrary::IsTerrainProfileOccluded(
        OwnerActor,
        Start,
        End,
        15.0f,
        40) && GetSmokeTransmissionAlong(Start, End) >= MinimumSmokeTransmissionForLOS;
}


float UStrategyVisibilityComponent::GetSmokeTransmissionTo(
    const AStrategyUnit* Target) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !IsValid(Target) || !GetWorld())
    {
        return 0.0f;
    }

    FVector Start =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerActor,
            OwnerActor->GetActorLocation());
    FVector End =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerActor,
            Target->GetActorLocation());

    Start.Z += EyeHeightCm;
    End.Z += TargetHeightCm;

    return GetSmokeTransmissionAlong(Start, End);
}

float UStrategyVisibilityComponent::GetSmokeTransmissionAlong(const FVector& Start, const FVector& End) const
{
    if (!GetWorld()) return 0.0f;
    float Transmission = 1.0f;

    for (TActorIterator<AStrategySmokeField> It(GetWorld()); It; ++It)
    {
        const AStrategySmokeField* Smoke = *It;
        if (!IsValid(Smoke) || !Smoke->IntersectsSightSegment(Start, End))
        {
            continue;
        }

        Transmission *=
            1.0f - FMath::Clamp(Smoke->GetCurrentDensity(), 0.0f, 0.95f);
    }

    return FMath::Clamp(Transmission, 0.0f, 1.0f);
}

bool UStrategyVisibilityComponent::IsConcealedInForest(const AStrategyUnit* Target) const
{
    if (!IsValid(Target) || !GetWorld()) return false;
    const bool bForestRecentlyFired = Target->CombatComponent &&
        GetWorld()->GetTimeSeconds() - Target->CombatComponent->LastFiredTimeSeconds <= ForestFiringRevealSeconds;
    return !bForestRecentlyFired && UStrategyTerrainQueryLibrary::GetForestDensityAt(Target, Target->GetActorLocation()) >= ForestConcealmentDensity;
}

bool UStrategyVisibilityComponent::PassesForestVisibility(const FVector& TargetLocation, const AStrategyUnit* Target, float SightRangeCm) const
{
    if (!GetOwner() || !GetWorld()) return false;
    const float ForestDepthM = UStrategyTerrainQueryLibrary::ForestDepthAlong(this, GetOwner()->GetActorLocation(), TargetLocation);
    const float ForestDistanceCm = FVector::Dist2D(GetOwner()->GetActorLocation(), TargetLocation);
    float ForestEffectiveRangeCm = SightRangeCm >= 0.0f ? SightRangeCm : ForestSightRangeCm;
    // Open ground retains existing LOS/range behaviour.
    if (ForestDepthM > 0.0f)
        ForestEffectiveRangeCm *= FMath::Clamp(1.0f - ForestDepthM / FMath::Max(1.0f, ForestBlockingDepthM), 0.0f, 1.0f);
    const bool bForestConcealed = IsConcealedInForest(Target);
    if (bForestConcealed) ForestEffectiveRangeCm = FMath::Min(ForestEffectiveRangeCm, ForestConcealedSightRangeCm);
    const bool bForestAllowed = ForestDepthM < FMath::Max(1.0f, ForestBlockingDepthM) &&
        ((ForestDepthM <= 0.0f && !bForestConcealed && SightRangeCm < 0.0f) || ForestDistanceCm <= ForestEffectiveRangeCm);
    static const bool bForestDebug = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugForest"));
    if (bForestDebug)
        UE_LOG(LogTemp, Log, TEXT("Strategy1864Forest: %s -> %s depth=%.1fm distance=%.1fm range=%.1fm concealed=%d allowed=%d"),
            *GetNameSafe(GetOwner()), *GetNameSafe(Target), ForestDepthM, ForestDistanceCm / 100.0f,
            ForestEffectiveRangeCm / 100.0f, bForestConcealed, bForestAllowed);
    return bForestAllowed;
}
