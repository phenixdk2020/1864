#include "StrategyVisibilityComponent.h"

#include "../Units/StrategyUnit.h"
#include "StrategySmokeField.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"

UStrategyVisibilityComponent::UStrategyVisibilityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UStrategyVisibilityComponent::HasLineOfSightTo(
    const AStrategyUnit* Target) const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !IsValid(Target) || !GetWorld())
    {
        return false;
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
        40);
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
