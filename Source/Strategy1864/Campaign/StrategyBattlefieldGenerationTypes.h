#pragma once

#include "CoreMinimal.h"
#include "StrategyBattlefieldGenerationTypes.generated.h"

UENUM(BlueprintType)
enum class EStrategyCampaignBattlefieldFeatureType : uint8
{
    HeightSample,
    Hill,
    Ridge,
    Depression,
    River,
    Road,
    Bridge,
    Settlement,
    Forest,
    Field,
    Marsh,
    Water
};

USTRUCT(BlueprintType)
struct FStrategyCampaignBattlefieldFeatureSeed
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SourceFeatureId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStrategyCampaignBattlefieldFeatureType FeatureType =
        EStrategyCampaignBattlefieldFeatureType::HeightSample;

    // Campaign-map coordinate in the campaign map's own world/reference units.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D CampaignPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D CampaignDirection = FVector2D(1.0f, 0.0f);

    // Feature footprint in real-world metres before tactical scaling.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D ExtentMeters = FVector2D(100.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ElevationMeters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WidthMeters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Importance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector2D> CampaignPolyline;
};

USTRUCT(BlueprintType)
struct FStrategyBattlefieldGenerationRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BattleId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D CampaignBattleCenter = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D BattlefieldSizeMeters = FVector2D(12000.0f, 12000.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CampaignMetersPerMapUnit = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GenerationSeed = 1864;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D AttackerApproachDirection = FVector2D(1.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D DefenderApproachDirection = FVector2D(-1.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FStrategyCampaignBattlefieldFeatureSeed> SourceFeatures;

    bool IsValidRequest() const
    {
        return BattlefieldSizeMeters.X > 100.0f &&
               BattlefieldSizeMeters.Y > 100.0f &&
               CampaignMetersPerMapUnit > KINDA_SMALL_NUMBER &&
               !AttackerApproachDirection.IsNearlyZero() &&
               !DefenderApproachDirection.IsNearlyZero();
    }
};

USTRUCT(BlueprintType)
struct FStrategyBattlefieldGenerationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSucceeded = false;

    UPROPERTY(BlueprintReadOnly)
    FName BattleId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int32 GenerationSeed = 0;

    UPROPERTY(BlueprintReadOnly)
    FVector TacticalWorldOrigin = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> MaterializedSourceFeatureIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Warnings;
};
