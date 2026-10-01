#include "StrategyArtilleryProjectilePresentationComponent.h"

#include "StrategyArtilleryBatteryUnit.h"
#include "StrategyArtilleryProjectilePresentation.h"
#include "StrategyArtilleryTrajectoryLibrary.h"
#include "../Terrain/StrategyTerrainQueryLibrary.h"
#include "Engine/World.h"

UStrategyArtilleryProjectilePresentationComponent::
UStrategyArtilleryProjectilePresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyArtilleryProjectilePresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerBattery = Cast<AStrategyArtilleryBatteryUnit>(GetOwner());
}

void UStrategyArtilleryProjectilePresentationComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ActiveProjectiles.RemoveAll(
        [](const TObjectPtr<AStrategyArtilleryProjectilePresentation>& Projectile)
        {
            return !IsValid(Projectile);
        });
}

FVector UStrategyArtilleryProjectilePresentationComponent::
GetVirtualMuzzleLocation(
    int32 GunIndex,
    int32 VisibleCount) const
{
    if (!OwnerBattery)
    {
        return FVector::ZeroVector;
    }

    FVector Base =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            OwnerBattery,
            OwnerBattery->GetActorLocation());

    const FVector Forward =
        OwnerBattery->GetActorForwardVector().GetSafeNormal2D();

    const FVector Right(-Forward.Y, Forward.X, 0.0f);

    const float CenterIndex =
        (static_cast<float>(FMath::Max(1, VisibleCount)) - 1.0f) * 0.5f;

    const float Lateral =
        (static_cast<float>(GunIndex) - CenterIndex) *
        VirtualGunSpacingCm;

    Base +=
        Forward * VirtualMuzzleForwardOffsetCm +
        Right * Lateral;

    Base.Z =
        UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(
            OwnerBattery,
            Base) +
        VirtualMuzzleHeightCm;

    return Base;
}

void UStrategyArtilleryProjectilePresentationComponent::
PresentResolvedSalvo(
    EStrategyArtilleryAmmoType AmmoType,
    const FVector& AimLocation,
    const TArray<FVector>& ImpactLocations,
    const TArray<uint8>& HitFlags,
    const TArray<int32>& CasualtiesPerProjectile)
{
    if (!OwnerBattery ||
        !GetWorld() ||
        ImpactLocations.Num() <= 0)
    {
        return;
    }

    const int32 VisibleCount =
        FMath::Clamp(
            ImpactLocations.Num(),
            1,
            FMath::Max(1, MaxVisibleProjectilesPerSalvo));

    for (int32 Index = 0; Index < VisibleCount; ++Index)
    {
        FStrategyArtilleryProjectileSpec Spec;
        Spec.ShotSerial = NextShotSerial++;
        Spec.GunIndex = Index;
        Spec.AmmoType = AmmoType;
        Spec.Style =
            UStrategyArtilleryTrajectoryLibrary::GetPresentationStyle(
                AmmoType);

        Spec.LaunchLocation =
            GetVirtualMuzzleLocation(Index, VisibleCount);

        Spec.AimLocation = AimLocation;
        Spec.PrimaryImpactLocation = ImpactLocations[Index];

        Spec.bAuthoritativeHit =
            HitFlags.IsValidIndex(Index) &&
            HitFlags[Index] != 0;

        Spec.ResolvedCasualties =
            CasualtiesPerProjectile.IsValidIndex(Index)
            ? CasualtiesPerProjectile[Index]
            : 0;

        Spec.FlightSeconds =
            UStrategyArtilleryTrajectoryLibrary::EstimateFlightSeconds(
                AmmoType,
                FVector::Dist2D(
                    Spec.LaunchLocation,
                    Spec.PrimaryImpactLocation));

        FVector FinalPoint = Spec.PrimaryImpactLocation;

        const TArray<FVector> Path =
            UStrategyArtilleryTrajectoryLibrary::BuildTrajectory(
                OwnerBattery,
                Spec,
                TrajectorySampleCount,
                bEnableRoundShotRicochet,
                FinalPoint);

        float PathLengthCm = 0.0f;
        for (int32 PathIndex = 1; PathIndex < Path.Num(); ++PathIndex)
        {
            PathLengthCm +=
                FVector::Dist(
                    Path[PathIndex - 1],
                    Path[PathIndex]);
        }

        const float DirectDistanceCm =
            FMath::Max(
                1.0f,
                FVector::Dist(
                    Spec.LaunchLocation,
                    Spec.PrimaryImpactLocation));

        Spec.FlightSeconds *=
            FMath::Clamp(
                PathLengthCm / DirectDistanceCm,
                1.0f,
                2.5f);

        AStrategyArtilleryProjectilePresentation* Projectile =
            GetWorld()->SpawnActor<AStrategyArtilleryProjectilePresentation>(
                AStrategyArtilleryProjectilePresentation::StaticClass(),
                Spec.LaunchLocation,
                FRotator::ZeroRotator);

        if (!Projectile)
        {
            continue;
        }

        Projectile->InitializePresentation(
            Spec,
            Path,
            FinalPoint,
            bDrawDebugTrajectory);

        ActiveProjectiles.Add(Projectile);
        AddHistory(Spec, FinalPoint);
    }
}

void UStrategyArtilleryProjectilePresentationComponent::
SetDebugTrajectoryEnabled(bool bEnabled)
{
    bDrawDebugTrajectory = bEnabled;

    for (AStrategyArtilleryProjectilePresentation* Projectile : ActiveProjectiles)
    {
        if (IsValid(Projectile))
        {
            Projectile->bDrawTrajectory = bEnabled;
        }
    }
}

AStrategyArtilleryProjectilePresentation*
UStrategyArtilleryProjectilePresentationComponent::
GetLatestActiveProjectile() const
{
    for (int32 Index = ActiveProjectiles.Num() - 1; Index >= 0; --Index)
    {
        AStrategyArtilleryProjectilePresentation* Projectile =
            ActiveProjectiles[Index];

        if (IsValid(Projectile) &&
            Projectile->IsFollowable())
        {
            return Projectile;
        }
    }

    return nullptr;
}

int32 UStrategyArtilleryProjectilePresentationComponent::
GetActiveProjectileCount() const
{
    int32 Count = 0;

    for (const AStrategyArtilleryProjectilePresentation* Projectile : ActiveProjectiles)
    {
        if (IsValid(Projectile))
        {
            ++Count;
        }
    }

    return Count;
}

void UStrategyArtilleryProjectilePresentationComponent::AddHistory(
    const FStrategyArtilleryProjectileSpec& Spec,
    const FVector& FinalImpact)
{
    FStrategyArtilleryProjectileHistoryRecord Record;
    Record.ShotSerial = Spec.ShotSerial;
    Record.GunIndex = Spec.GunIndex;
    Record.AmmoType = Spec.AmmoType;
    Record.LaunchLocation = Spec.LaunchLocation;
    Record.AimLocation = Spec.AimLocation;
    Record.ImpactLocation = FinalImpact;
    Record.bAuthoritativeHit = Spec.bAuthoritativeHit;
    Record.ResolvedCasualties = Spec.ResolvedCasualties;
    Record.WorldTimeSeconds =
        GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    RecentHistory.Add(Record);

    while (RecentHistory.Num() > FMath::Max(1, MaximumHistoryRecords))
    {
        RecentHistory.RemoveAt(0);
    }
}
