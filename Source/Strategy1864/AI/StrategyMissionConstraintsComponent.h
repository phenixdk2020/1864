#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyMissionConstraintsComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyMissionConstraintsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyMissionConstraintsComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Constraints")
    bool bDoNotPursue = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Constraints")
    bool bConserveAmmunition = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Constraints")
    bool bDoNotLeaveArea = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Constraints")
    FVector MissionAreaCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Constraints")
    float MissionAreaRadiusCm = 15000.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Constraints")
    void SetMissionArea(const FVector& Center, float RadiusCm);

    UFUNCTION(BlueprintPure, Category="Strategy|Constraints")
    FVector ClampGoalToMissionArea(const FVector& RequestedGoal) const;

    UFUNCTION(BlueprintPure, Category="Strategy|Constraints")
    bool CanPursueTarget(const AStrategyUnit* Target) const;

private:
    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;
};
