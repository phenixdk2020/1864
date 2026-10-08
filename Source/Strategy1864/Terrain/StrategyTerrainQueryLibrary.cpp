#include "StrategyTerrainQueryLibrary.h"

#include "StrategyTerrainFeature.h"
#include "../Campaign/StrategyCampaignBattlefield.h"
#include "EngineUtils.h"
#include "Engine/World.h"

namespace StrategyTerrainElevationCache
{
    constexpr float CellSizeCm = 10000.0f;
    struct FWorldCache
    {
        bool bBuilt = false;
        TMap<FIntPoint, TArray<TWeakObjectPtr<AStrategyTerrainFeature>>> Cells;
        TMap<FVector2D, float> Offsets;
    };
    TMap<TWeakObjectPtr<UWorld>, FWorldCache> Worlds;
    FIntPoint CellAt(const FVector& Point)
    {
        return FIntPoint(FMath::FloorToInt(Point.X / CellSizeCm), FMath::FloorToInt(Point.Y / CellSizeCm));
    }
}

void UStrategyTerrainQueryLibrary::InvalidateFeatureElevationCache(UWorld* World)
{
    StrategyTerrainElevationCache::Worlds.Remove(TWeakObjectPtr<UWorld>(World));
}

UWorld* UStrategyTerrainQueryLibrary::ResolveWorld(
    const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    return WorldContextObject->GetWorld();
}

float UStrategyTerrainQueryLibrary::GetPhysicalGroundZ(
    UWorld* World,
    const FVector& WorldLocation)
{
    if (!World)
    {
        return WorldLocation.Z;
    }

    const FVector Start(
        WorldLocation.X,
        WorldLocation.Y,
        WorldLocation.Z + 100000.0f);

    const FVector End(
        WorldLocation.X,
        WorldLocation.Y,
        WorldLocation.Z - 100000.0f);

    FHitResult Hit;

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(StrategyTerrainGround),
        false);

    const bool bHit =
        World->LineTraceSingleByObjectType(
            Hit,
            Start,
            End,
            ObjectParams,
            QueryParams);

    return bHit ? Hit.ImpactPoint.Z : WorldLocation.Z;
}

float UStrategyTerrainQueryLibrary::GetFeatureElevationOffset(
    const UObject* WorldContextObject,
    const FVector& WorldLocation)
{
    UWorld* World = ResolveWorld(WorldContextObject);
    if (!World)
    {
        return 0.0f;
    }

    // Features are static. Index their conservative XY bounds once and cache exact
    // offsets without quantization, so narrow crests retain their original shape.
    for (auto CacheIt = StrategyTerrainElevationCache::Worlds.CreateIterator(); CacheIt; ++CacheIt)
    {
        if (!CacheIt.Key().IsValid()) { CacheIt.RemoveCurrent(); }
    }
    auto& ElevationCache = StrategyTerrainElevationCache::Worlds.FindOrAdd(TWeakObjectPtr<UWorld>(World));
    if (!ElevationCache.bBuilt)
    {
        for (TActorIterator<AStrategyTerrainFeature> FeatureIt(World); FeatureIt; ++FeatureIt)
        {
            AStrategyTerrainFeature* TerrainFeature = *FeatureIt;
            if (!IsValid(TerrainFeature) || !TerrainFeature->bAffectsGameplay) { continue; }
            const float BoundsRadius = FMath::Max(1.0f, FMath::Max(TerrainFeature->RadiusXcm, TerrainFeature->RadiusYcm));
            const FVector FeatureCenter = TerrainFeature->GetActorLocation();
            const FIntPoint MinCell = StrategyTerrainElevationCache::CellAt(FeatureCenter - FVector(BoundsRadius, BoundsRadius, 0.0f));
            const FIntPoint MaxCell = StrategyTerrainElevationCache::CellAt(FeatureCenter + FVector(BoundsRadius, BoundsRadius, 0.0f));
            for (int32 CellX = MinCell.X; CellX <= MaxCell.X; ++CellX)
            {
                for (int32 CellY = MinCell.Y; CellY <= MaxCell.Y; ++CellY)
                {
                    ElevationCache.Cells.FindOrAdd(FIntPoint(CellX, CellY)).Add(TerrainFeature);
                }
            }
        }
        ElevationCache.bBuilt = true;
    }
    const FVector2D ElevationKey(WorldLocation.X, WorldLocation.Y);
    if (const float* CachedOffset = ElevationCache.Offsets.Find(ElevationKey)) { return *CachedOffset; }
    float Offset = 0.0f;
    if (const auto* LocalFeatures = ElevationCache.Cells.Find(StrategyTerrainElevationCache::CellAt(WorldLocation)))
    {
        for (const auto& FeaturePtr : *LocalFeatures)
        {
            if (const AStrategyTerrainFeature* TerrainFeature = FeaturePtr.Get())
            {
                Offset += TerrainFeature->GetHeightOffsetAt(WorldLocation);
            }
        }
    }
    // Bound memory for continuously moving observers without changing sample heights.
    if (ElevationCache.Offsets.Num() >= 8192) { ElevationCache.Offsets.Reset(); }
    ElevationCache.Offsets.Add(ElevationKey, Offset);
    return Offset;
}

float UStrategyTerrainQueryLibrary::GetEffectiveGroundZ(
    const UObject* WorldContextObject,
    const FVector& WorldLocation)
{
    UWorld* World = ResolveWorld(WorldContextObject);

    return GetPhysicalGroundZ(World, WorldLocation) +
        GetFeatureElevationOffset(WorldContextObject, WorldLocation);
}

FVector UStrategyTerrainQueryLibrary::ProjectPointToTerrain(
    const UObject* WorldContextObject,
    const FVector& WorldLocation)
{
    FVector Result = WorldLocation;
    Result.Z = GetEffectiveGroundZ(WorldContextObject, WorldLocation);
    return Result;
}

float UStrategyTerrainQueryLibrary::GetLocalSlopeDegrees(
    const UObject* WorldContextObject,
    const FVector& WorldLocation,
    float SampleRadiusCm)
{
    const float R = FMath::Max(100.0f, SampleRadiusCm);

    const FVector PX =
        ProjectPointToTerrain(
            WorldContextObject,
            WorldLocation + FVector(R, 0.0f, 0.0f));

    const FVector NX =
        ProjectPointToTerrain(
            WorldContextObject,
            WorldLocation - FVector(R, 0.0f, 0.0f));

    const FVector PY =
        ProjectPointToTerrain(
            WorldContextObject,
            WorldLocation + FVector(0.0f, R, 0.0f));

    const FVector NY =
        ProjectPointToTerrain(
            WorldContextObject,
            WorldLocation - FVector(0.0f, R, 0.0f));

    const FVector XSpan = PX - NX;
    const FVector YSpan = PY - NY;

    FVector Normal =
        FVector::CrossProduct(XSpan, YSpan).GetSafeNormal();

    if (Normal.Z < 0.0f)
    {
        Normal *= -1.0f;
    }

    if (Normal.IsNearlyZero())
    {
        return 0.0f;
    }

    const float UpDot =
        FMath::Clamp(
            FVector::DotProduct(Normal, FVector::UpVector),
            -1.0f,
            1.0f);

    return FMath::RadiansToDegrees(FMath::Acos(UpDot));
}

bool UStrategyTerrainQueryLibrary::IsTerrainProfileOccluded(
    const UObject* WorldContextObject,
    const FVector& Start,
    const FVector& End,
    float ClearanceCm,
    int32 SampleCount)
{
    const int32 Samples = FMath::Clamp(SampleCount, 6, 128);

    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const float Alpha =
            static_cast<float>(Index) /
            static_cast<float>(Samples);

        const FVector LinePoint =
            FMath::Lerp(Start, End, Alpha);

        const float GroundZ =
            GetEffectiveGroundZ(
                WorldContextObject,
                LinePoint);

        if (GroundZ + ClearanceCm >= LinePoint.Z)
        {
            return true;
        }
    }

    return false;
}

bool UStrategyTerrainQueryLibrary::FindCrestPoint(
    const UObject* WorldContextObject,
    const FVector& Start,
    const FVector& End,
    FVector& OutCrestPoint,
    float& OutExcessHeightCm,
    int32 SampleCount,
    float CrestToleranceCm)
{
    const int32 Samples = FMath::Clamp(SampleCount, 6, 128);
    float BestExcess = -TNumericLimits<float>::Max();
    FVector BestPoint = FVector::ZeroVector;

    for (int32 Index = 1; Index < Samples; ++Index)
    {
        const float Alpha =
            static_cast<float>(Index) /
            static_cast<float>(Samples);

        FVector Point = FMath::Lerp(Start, End, Alpha);

        const float GroundZ =
            GetEffectiveGroundZ(
                WorldContextObject,
                Point);

        const float Excess = GroundZ - Point.Z;

        if (Excess > BestExcess)
        {
            BestExcess = Excess;
            Point.Z = GroundZ;
            BestPoint = Point;
        }
    }

    OutCrestPoint = BestPoint;
    OutExcessHeightCm = BestExcess;

    return BestExcess > -FMath::Abs(CrestToleranceCm);
}

bool UStrategyTerrainQueryLibrary::IsPointInDeadGroundFrom(
    const UObject* WorldContextObject,
    const FVector& ObserverLocation,
    const FVector& TargetLocation,
    float ObserverEyeHeightCm,
    float TargetHeightCm)
{
    FVector Start =
        ProjectPointToTerrain(
            WorldContextObject,
            ObserverLocation);

    FVector End =
        ProjectPointToTerrain(
            WorldContextObject,
            TargetLocation);

    Start.Z += ObserverEyeHeightCm;
    End.Z += TargetHeightCm;

    return IsTerrainProfileOccluded(
        WorldContextObject,
        Start,
        End,
        15.0f,
        40);
}

float UStrategyTerrainQueryLibrary::GetElevationAdvantageCm(
    const UObject* WorldContextObject,
    const FVector& ObserverLocation,
    const FVector& TargetLocation)
{
    return
        GetEffectiveGroundZ(WorldContextObject, ObserverLocation) -
        GetEffectiveGroundZ(WorldContextObject, TargetLocation);
}

namespace
{
    TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<AStrategyCampaignBattlefield>> StrategyForestBattlefields;
}
void UStrategyTerrainQueryLibrary::RegisterForestBattlefield(AStrategyCampaignBattlefield* Battlefield)
{
    if (!IsValid(Battlefield) || !Battlefield->GetWorld()) return;
    for (auto ForestIt = StrategyForestBattlefields.CreateIterator(); ForestIt; ++ForestIt)
        if (!ForestIt.Key().IsValid() || !ForestIt.Value().IsValid()) ForestIt.RemoveCurrent();
    StrategyForestBattlefields.Add(TWeakObjectPtr<UWorld>(Battlefield->GetWorld()), TWeakObjectPtr<AStrategyCampaignBattlefield>(Battlefield));
}
AStrategyCampaignBattlefield* UStrategyTerrainQueryLibrary::GetForestBattlefield(const UObject* WorldContextObject)
{
    const auto* ForestEntry = StrategyForestBattlefields.Find(TWeakObjectPtr<UWorld>(ResolveWorld(WorldContextObject)));
    return ForestEntry ? ForestEntry->Get() : nullptr;
}
float UStrategyTerrainQueryLibrary::GetForestDensityAt(const UObject* WorldContextObject, const FVector& WorldLocation)
{
    const auto* ForestField = GetForestBattlefield(WorldContextObject);
    return ForestField ? ForestField->GetForestDensityAt(WorldLocation) : 0.0f;
}
float UStrategyTerrainQueryLibrary::ForestDepthAlong(const UObject* WorldContextObject, const FVector& Start, const FVector& End)
{
    const auto* ForestField = GetForestBattlefield(WorldContextObject);
    if (!ForestField) return 0.0f;
    const float ForestLengthM = FVector::Dist2D(Start, End) / 100.0f;
    const int32 ForestSamples = FMath::Clamp(FMath::CeilToInt(ForestLengthM / 5.0f), 1, 40);
    float ForestDensitySum = 0.0f;
    for (int32 ForestSample = 0; ForestSample < ForestSamples; ++ForestSample)
        ForestDensitySum += ForestField->GetForestDensityAt(FMath::Lerp(Start, End, (ForestSample + 0.5f) / ForestSamples));
    return ForestLengthM * ForestDensitySum / ForestSamples;
}
