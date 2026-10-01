#include "StrategyTerrainQueryLibrary.h"

#include "StrategyTerrainFeature.h"
#include "EngineUtils.h"
#include "Engine/World.h"

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

    return bHit ? Hit.ImpactPoint.Z : 0.0f;
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

    float Offset = 0.0f;

    for (TActorIterator<AStrategyTerrainFeature> It(World); It; ++It)
    {
        const AStrategyTerrainFeature* Feature = *It;

        if (!IsValid(Feature) || !Feature->bAffectsGameplay)
        {
            continue;
        }

        Offset += Feature->GetHeightOffsetAt(WorldLocation);
    }

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

    for (int32 Index = 1; Index < Samples; ++Index)
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
