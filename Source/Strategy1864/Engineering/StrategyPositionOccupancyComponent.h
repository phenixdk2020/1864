#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyPositionOccupancyComponent.generated.h"

class AStrategyDefensivePosition;
class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyPositionOccupancyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyPositionOccupancyComponent();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Fieldworks")
    TObjectPtr<AStrategyDefensivePosition> OccupiedPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Fieldworks")
    float MaximumOccupationDistanceCm = 1200.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    bool OccupyPosition(AStrategyDefensivePosition* Position);

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    void LeavePosition();

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    AStrategyDefensivePosition* FindNearestUsablePosition(float SearchRadiusCm) const;

    UFUNCTION(BlueprintCallable, Category="Strategy|Fieldworks")
    bool AutoOccupyNearest(float SearchRadiusCm);

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    float GetIncomingHitMultiplier(const FVector& ShooterLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool HasArtilleryEmplacement() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool IsPositionFacingThreat(const FVector& ThreatLocation) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    bool IsPositionUsable() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    float GetPositionCondition() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Fieldworks")
    FString GetPositionStatusText() const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
