#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyVisibilityComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyVisibilityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyVisibilityComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility")
    float EyeHeightCm = 160.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility|Forest")
    float ForestBlockingDepthM = 50.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility|Forest")
    float ForestSightRangeCm = 60000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility|Forest")
    float ForestConcealmentDensity = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility|Forest")
    float ForestConcealedSightRangeCm = 12000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility|Forest")
    float ForestFiringRevealSeconds = 8.0f;

    bool PassesForestVisibility(const FVector& TargetLocation, const AStrategyUnit* Target = nullptr,
        float SightRangeCm = -1.0f) const;
    bool CanDetectTarget(const AStrategyUnit* Target, float SightRangeCm) const;
    bool IsConcealedInForest(const AStrategyUnit* Target) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility")
    float TargetHeightCm = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Visibility")
    float MinimumSmokeTransmissionForLOS = 0.20f;

    UFUNCTION(BlueprintPure, Category="Strategy|Visibility")
    bool HasLineOfSightTo(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Visibility")
    bool HasLineOfSightToLocation(
        const FVector& TargetLocation,
        float LocationTargetHeightCm = 50.0f) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Visibility")
    float GetSmokeTransmissionTo(const AStrategyUnit* Target) const;

private:
    float GetSmokeTransmissionAlong(const FVector& Start, const FVector& End) const;
};
