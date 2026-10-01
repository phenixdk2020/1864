#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Terrain/StrategyTerrainTypes.h"
#include "StrategyArtilleryPositioningComponent.generated.h"

class AStrategyArtilleryBatteryUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryPositioningComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryPositioningComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float MaximumDirectFireSlopeDegrees = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float CrestExposureDistanceCm = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float DirectLOSScore = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float DeadGroundPenalty = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float CrestExposurePenalty = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float SlopePenaltyPerDegree = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float ElevationScorePer100Cm = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery|Positioning")
    float MovementPenaltyPer1000Cm = 0.75f;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery|Positioning")
    FStrategyTerrainPositionAssessment EvaluatePosition(
        const FVector& CandidatePosition,
        const FVector& ThreatPosition) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Positioning")
    TArray<FVector> GenerateCandidatePositions(
        const FVector& Center,
        float SearchRadiusCm,
        int32 CandidateCount) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery|Positioning")
    bool FindBestDirectFirePosition(
        const FVector& ThreatPosition,
        float SearchRadiusCm,
        int32 CandidateCount,
        FVector& OutPosition,
        FStrategyTerrainPositionAssessment& OutAssessment) const;

private:
    bool HasPhysicalStaticLOS(
        const FVector& Start,
        const FVector& End) const;

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;
};
