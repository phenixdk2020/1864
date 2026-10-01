#pragma once

#include "CoreMinimal.h"
#include "StrategyTerrainTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyTerrainFeatureType : uint8
{
    Hill,
    Ridge,
    Depression
};

USTRUCT(BlueprintType)
struct FStrategyTerrainPositionAssessment
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float GroundZ = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float LocalSlopeDegrees = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float ElevationAdvantageCm = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bHasDirectLOS = false;

    UPROPERTY(BlueprintReadOnly)
    bool bNearCrest = false;

    UPROPERTY(BlueprintReadOnly)
    bool bInDeadGroundFromThreat = false;

    UPROPERTY(BlueprintReadOnly)
    float Score = 0.0f;
};
