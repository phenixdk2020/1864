#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StrategyArtilleryProjectileTypes.h"
#include "StrategyArtilleryTrajectoryLibrary.generated.h"

UCLASS()
class STRATEGY1864_API UStrategyArtilleryTrajectoryLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Projectile", meta=(WorldContext="WorldContextObject"))
    static TArray<FVector> BuildTrajectory(
        const UObject* WorldContextObject,
        const FStrategyArtilleryProjectileSpec& Spec,
        int32 SampleCount,
        bool bEnableRoundShotRicochet,
        FVector& OutFinalPoint);

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Projectile")
    static float EstimateFlightSeconds(
        EStrategyArtilleryAmmoType AmmoType,
        float DistanceCm);

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Projectile")
    static EStrategyProjectilePresentationStyle GetPresentationStyle(
        EStrategyArtilleryAmmoType AmmoType);

private:
    static void AppendArc(
        const UObject* WorldContextObject,
        const FVector& Start,
        const FVector& End,
        float ArcHeightCm,
        int32 SampleCount,
        TArray<FVector>& InOutPoints,
        bool& bOutTerrainHit,
        FVector& OutTerrainHitPoint);

    static void AppendRoundShotRicochets(
        const UObject* WorldContextObject,
        const FVector& FirstImpact,
        const FVector& HorizontalDirection,
        float InitialTravelDistanceCm,
        TArray<FVector>& InOutPoints,
        FVector& OutFinalPoint);
};
