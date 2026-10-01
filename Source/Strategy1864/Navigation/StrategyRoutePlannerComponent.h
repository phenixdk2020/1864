#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyRouteTypes.h"
#include "StrategyRoutePlannerComponent.generated.h"

class AStrategyRiverBarrier;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyRoutePlannerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyRoutePlannerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    bool bUseNavigationSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    float MaxTraversableSlopeDegrees = 28.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    float TerrainSampleStepCm = 1000.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Navigation")
    TArray<FVector> BuildRoute(
        const FVector& StartLocation,
        const FVector& EndLocation) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Navigation")
    FStrategyRoutePlan BuildRoutePlan(
        const FVector& StartLocation,
        const FVector& EndLocation) const;

private:
    void ApplyStaticObstacleDetours(
        const FVector& StartLocation,
        const FVector& EndLocation,
        TArray<FVector>& InOutPoints) const;

    void ApplyTacticalTerrainElevation(
        TArray<FVector>& InOutPoints) const;

    bool ValidateSlopeProfile(
        const FVector& StartLocation,
        const TArray<FVector>& Points,
        FString& OutFailureReason) const;

    AStrategyRiverBarrier* FindRelevantRiverBarrier(
        const FVector& StartLocation,
        const FVector& EndLocation) const;

    void AppendNavSegment(
        const FVector& SegmentStart,
        const FVector& SegmentEnd,
        TArray<FVector>& InOutPoints) const;
};
