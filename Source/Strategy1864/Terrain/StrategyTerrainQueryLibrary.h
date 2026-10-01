#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StrategyTerrainQueryLibrary.generated.h"

class UWorld;

UCLASS()
class STRATEGY1864_API UStrategyTerrainQueryLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static float GetFeatureElevationOffset(
        const UObject* WorldContextObject,
        const FVector& WorldLocation);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static float GetEffectiveGroundZ(
        const UObject* WorldContextObject,
        const FVector& WorldLocation);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static FVector ProjectPointToTerrain(
        const UObject* WorldContextObject,
        const FVector& WorldLocation);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static float GetLocalSlopeDegrees(
        const UObject* WorldContextObject,
        const FVector& WorldLocation,
        float SampleRadiusCm = 500.0f);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static bool IsTerrainProfileOccluded(
        const UObject* WorldContextObject,
        const FVector& Start,
        const FVector& End,
        float ClearanceCm = 20.0f,
        int32 SampleCount = 32);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static bool FindCrestPoint(
        const UObject* WorldContextObject,
        const FVector& Start,
        const FVector& End,
        FVector& OutCrestPoint,
        float& OutExcessHeightCm,
        int32 SampleCount = 32,
        float CrestToleranceCm = 120.0f);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static bool IsPointInDeadGroundFrom(
        const UObject* WorldContextObject,
        const FVector& ObserverLocation,
        const FVector& TargetLocation,
        float ObserverEyeHeightCm = 160.0f,
        float TargetHeightCm = 120.0f);

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain", meta=(WorldContext="WorldContextObject"))
    static float GetElevationAdvantageCm(
        const UObject* WorldContextObject,
        const FVector& ObserverLocation,
        const FVector& TargetLocation);

private:
    static UWorld* ResolveWorld(const UObject* WorldContextObject);
    static float GetPhysicalGroundZ(UWorld* World, const FVector& WorldLocation);
};
