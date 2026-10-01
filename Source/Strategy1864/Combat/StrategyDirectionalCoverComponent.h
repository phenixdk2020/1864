#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyDirectionalCoverComponent.generated.h"

class AStrategyNavigationObstacle;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyDirectionalCoverComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyDirectionalCoverComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Cover")
    float MaximumCoverUseDistanceCm = 1200.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Cover")
    float CalculateIncomingHitMultiplier(const FVector& ShooterLocation) const;

private:
    float GetObstacleProtectionMultiplier(
        const AStrategyNavigationObstacle* Obstacle) const;
};
