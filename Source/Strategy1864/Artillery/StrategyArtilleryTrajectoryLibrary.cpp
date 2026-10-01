#include "StrategyArtilleryTrajectoryLibrary.h"

#include "../Terrain/StrategyTerrainQueryLibrary.h"

float UStrategyArtilleryTrajectoryLibrary::EstimateFlightSeconds(
    EStrategyArtilleryAmmoType AmmoType,
    float DistanceCm)
{
    const float DistanceM = FMath::Max(1.0f, DistanceCm / 100.0f);

    float MetersPerSecond = 300.0f;

    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::Shell:
            MetersPerSecond = 230.0f;
            break;

        case EStrategyArtilleryAmmoType::Shrapnel:
            MetersPerSecond = 250.0f;
            break;

        case EStrategyArtilleryAmmoType::Canister:
            MetersPerSecond = 340.0f;
            break;

        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            MetersPerSecond = 320.0f;
            break;
    }

    return FMath::Clamp(
        DistanceM / MetersPerSecond,
        0.18f,
        6.0f);
}

EStrategyProjectilePresentationStyle
UStrategyArtilleryTrajectoryLibrary::GetPresentationStyle(
    EStrategyArtilleryAmmoType AmmoType)
{
    switch (AmmoType)
    {
        case EStrategyArtilleryAmmoType::Shell:
            return EStrategyProjectilePresentationStyle::Shell;
        case EStrategyArtilleryAmmoType::Shrapnel:
            return EStrategyProjectilePresentationStyle::Shrapnel;
        case EStrategyArtilleryAmmoType::Canister:
            return EStrategyProjectilePresentationStyle::Canister;
        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            return EStrategyProjectilePresentationStyle::RoundShot;
    }
}

void UStrategyArtilleryTrajectoryLibrary::AppendArc(
    const UObject* WorldContextObject,
    const FVector& Start,
    const FVector& End,
    float ArcHeightCm,
    int32 SampleCount,
    TArray<FVector>& InOutPoints,
    bool& bOutTerrainHit,
    FVector& OutTerrainHitPoint)
{
    bOutTerrainHit = false;
    OutTerrainHitPoint = End;

    const int32 Samples = FMath::Clamp(SampleCount, 6, 128);

    if (InOutPoints.Num() == 0)
    {
        InOutPoints.Add(Start);
    }

    for (int32 Index = 1; Index <= Samples; ++Index)
    {
        const float Alpha =
            static_cast<float>(Index) /
            static_cast<float>(Samples);

        FVector Point = FMath::Lerp(Start, End, Alpha);

        const float Arc =
            4.0f * ArcHeightCm * Alpha * (1.0f - Alpha);

        Point.Z += Arc;

        const float GroundZ =
            UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(
                WorldContextObject,
                Point);

        if (Index < Samples &&
            Point.Z <= GroundZ + 6.0f)
        {
            Point.Z = GroundZ + 6.0f;
            InOutPoints.Add(Point);

            bOutTerrainHit = true;
            OutTerrainHitPoint = Point;
            return;
        }

        if (Index == Samples)
        {
            Point.Z = FMath::Max(Point.Z, GroundZ + 6.0f);
        }

        InOutPoints.Add(Point);
    }
}

void UStrategyArtilleryTrajectoryLibrary::AppendRoundShotRicochets(
    const UObject* WorldContextObject,
    const FVector& FirstImpact,
    const FVector& HorizontalDirection,
    float InitialTravelDistanceCm,
    TArray<FVector>& InOutPoints,
    FVector& OutFinalPoint)
{
    FVector Current = FirstImpact;
    float Travel = FMath::Clamp(
        InitialTravelDistanceCm * 0.14f,
        600.0f,
        6000.0f);

    float Height = FMath::Clamp(
        InitialTravelDistanceCm * 0.012f,
        45.0f,
        300.0f);

    for (int32 Bounce = 0; Bounce < 2; ++Bounce)
    {
        FVector End =
            Current +
            HorizontalDirection * Travel;

        End =
            UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
                WorldContextObject,
                End);

        End.Z += 6.0f;

        bool bTerrainHit = false;
        FVector TerrainHit = End;

        AppendArc(
            WorldContextObject,
            Current,
            End,
            Height,
            8,
            InOutPoints,
            bTerrainHit,
            TerrainHit);

        Current = bTerrainHit ? TerrainHit : End;
        Travel *= 0.55f;
        Height *= 0.45f;
    }

    OutFinalPoint = Current;
}

TArray<FVector> UStrategyArtilleryTrajectoryLibrary::BuildTrajectory(
    const UObject* WorldContextObject,
    const FStrategyArtilleryProjectileSpec& Spec,
    int32 SampleCount,
    bool bEnableRoundShotRicochet,
    FVector& OutFinalPoint)
{
    TArray<FVector> Result;

    if (!WorldContextObject)
    {
        OutFinalPoint = Spec.PrimaryImpactLocation;
        return Result;
    }

    const FVector Start = Spec.LaunchLocation;

    FVector End =
        UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
            WorldContextObject,
            Spec.PrimaryImpactLocation);
    End.Z += 6.0f;

    const float DistanceCm = FVector::Dist2D(Start, End);

    float ArcHeightCm = 150.0f;

    switch (Spec.AmmoType)
    {
        case EStrategyArtilleryAmmoType::Shell:
            ArcHeightCm =
                FMath::Clamp(DistanceCm * 0.18f, 700.0f, 9000.0f);
            break;

        case EStrategyArtilleryAmmoType::Shrapnel:
            ArcHeightCm =
                FMath::Clamp(DistanceCm * 0.22f, 900.0f, 11000.0f);
            break;

        case EStrategyArtilleryAmmoType::Canister:
            ArcHeightCm =
                FMath::Clamp(DistanceCm * 0.01f, 20.0f, 120.0f);
            break;

        case EStrategyArtilleryAmmoType::RoundShot:
        default:
            ArcHeightCm =
                FMath::Clamp(DistanceCm * 0.025f, 80.0f, 650.0f);
            break;
    }

    bool bTerrainHit = false;
    FVector FirstImpact = End;

    AppendArc(
        WorldContextObject,
        Start,
        End,
        ArcHeightCm,
        SampleCount,
        Result,
        bTerrainHit,
        FirstImpact);

    if (Result.Num() == 0)
    {
        Result.Add(Start);
        Result.Add(End);
        FirstImpact = End;
    }

    OutFinalPoint = FirstImpact;

    if (Spec.AmmoType == EStrategyArtilleryAmmoType::RoundShot &&
        bEnableRoundShotRicochet)
    {
        FVector Direction = End - Start;
        Direction.Z = 0.0f;
        Direction = Direction.GetSafeNormal();

        if (!Direction.IsNearlyZero())
        {
            AppendRoundShotRicochets(
                WorldContextObject,
                FirstImpact,
                Direction,
                DistanceCm,
                Result,
                OutFinalPoint);
        }
    }

    return Result;
}
