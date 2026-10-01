#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyTerrainAwarenessComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyTerrainAwarenessComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyTerrainAwarenessComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float HighGroundRangeBonusPer1000Cm = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float MaximumHighGroundRangeBonus = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float MaximumLowGroundRangePenalty = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float CrestNearDistanceCm = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float CrestExposureHitMultiplier = 0.88f;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetGroundZ() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetLocalSlopeDegrees(float SampleRadiusCm = 500.0f) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetElevationAdvantageTo(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetObservationRangeMultiplierTo(const AStrategyUnit* Target) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    bool IsInDeadGroundFrom(const FVector& ObserverLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    bool IsNearCrestRelativeTo(const FVector& ThreatLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    bool IsOnReverseSlopeFrom(const FVector& ThreatLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetIncomingHitMultiplierFrom(const FVector& ThreatLocation) const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
