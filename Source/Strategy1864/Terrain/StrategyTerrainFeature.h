#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyTerrainTypes.h"
#include "StrategyTerrainFeature.generated.h"

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyTerrainFeature : public AActor
{
    GENERATED_BODY()

public:
    AStrategyTerrainFeature();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    EStrategyTerrainFeatureType FeatureType =
        EStrategyTerrainFeatureType::Hill;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float PeakHeightCm = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float RadiusXcm = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    float RadiusYcm = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Terrain")
    bool bAffectsGameplay = true;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetHeightOffsetAt(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    bool ContainsXY(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Terrain")
    float GetNormalizedRadiusAt(const FVector& WorldLocation) const;
};
