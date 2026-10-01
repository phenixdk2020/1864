#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyNavigationObstacle.generated.h"

UENUM(BlueprintType)
enum class EStrategyObstacleType : uint8
{
    Building,
    Fence,
    Fieldworks,
    Generic
};

UCLASS(Blueprintable)
class STRATEGY1864_API AStrategyNavigationObstacle : public AActor
{
    GENERATED_BODY()

public:
    AStrategyNavigationObstacle();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    EStrategyObstacleType ObstacleType = EStrategyObstacleType::Generic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    FVector HalfExtentCm = FVector(1000.0f, 1000.0f, 300.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Navigation")
    float ClearanceCm = 600.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Navigation")
    FBox GetExpandedBounds() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Navigation")
    bool IntersectsSegment2D(const FVector& Start, const FVector& End) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Navigation")
    FVector BuildDetourPoint(const FVector& Start, const FVector& End) const;
};
