#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyArtilleryTypes.h"
#include "../Orders/StrategyOrderTypes.h"
#include "StrategyArtilleryDeploymentComponent.generated.h"

class AStrategyArtilleryBatteryUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyArtilleryDeploymentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStrategyArtilleryDeploymentComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    EStrategyArtilleryMobilityState MobilityState =
        EStrategyArtilleryMobilityState::Limbered;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float UnlimberSeconds = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float LimberSeconds = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float ManhandleSpeedCmPerSecond = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float MaximumManhandleDistanceCm = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float MaximumDeploySlopeDegrees = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float DeployTerrainSampleRadiusCm = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Artillery")
    float ManhandlingFatiguePerSecond = 0.35f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Artillery")
    float TransitionRemainingSeconds = 0.0f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool RequestDeploy();

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool RequestLimber();

    UFUNCTION(BlueprintCallable, Category="Strategy|Artillery")
    bool RequestManhandle(
        const FVector& TargetLocation,
        float FacingYaw,
        EStrategyOrderAuthority Authority);

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    float GetCurrentGroundSlopeDegrees() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool CanDeployAtCurrentLocation() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool IsLimbered() const
    {
        return MobilityState == EStrategyArtilleryMobilityState::Limbered;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool IsDeployed() const
    {
        return MobilityState == EStrategyArtilleryMobilityState::Deployed;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool IsManhandling() const
    {
        return MobilityState == EStrategyArtilleryMobilityState::Manhandling;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool IsTransitioning() const
    {
        return MobilityState == EStrategyArtilleryMobilityState::Deploying ||
               MobilityState == EStrategyArtilleryMobilityState::Limbering;
    }

    UFUNCTION(BlueprintPure, Category="Strategy|Artillery")
    bool AllowsMovementExecutor() const
    {
        return IsLimbered() || IsManhandling();
    }

private:
    void CompleteTransition();

    UPROPERTY()
    TObjectPtr<AStrategyArtilleryBatteryUnit> OwnerBattery;

    float PreviousMoveSpeedCmPerSecond = 0.0f;
};
