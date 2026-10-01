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
};
