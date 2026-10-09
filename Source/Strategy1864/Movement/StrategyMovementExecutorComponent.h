#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Orders/StrategyOrderTypes.h"
#include "../Navigation/StrategyRouteTypes.h"
#include "StrategyMovementExecutorComponent.generated.h"

class AStrategyUnit;

UCLASS(ClassGroup=(Strategy1864), meta=(BlueprintSpawnableComponent))
class STRATEGY1864_API UStrategyMovementExecutorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Strategy|Square")
    float SquareMoveSpeedMultiplier = 0.08f;

public:
    UStrategyMovementExecutorComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float MoveSpeedCmPerSecond = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float ArrivalToleranceCm = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float TurnSpeedDegreesPerSecond = 36.0f;   // a body of men wheels slowly (cavalry sets its own)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float FinalFacingToleranceDegrees = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float MinimumUphillSpeedMultiplier = 0.60f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Movement")
    float DownhillSpeedMultiplier = 0.90f;

    UFUNCTION(BlueprintCallable, Category="Strategy|Movement")
    void StopMovement();

    UFUNCTION(BlueprintCallable, Category="Strategy|Movement")
    void RestartCurrentOrderExecution();

    UFUNCTION(BlueprintCallable, Category="Strategy|Movement")
    void PauseMovementForSeconds(float DurationSeconds);

    UFUNCTION(BlueprintPure, Category="Strategy|Movement")
    bool IsTemporarilyPaused() const { return PauseRemainingSeconds > 0.0f || bHoldingForFire; }

    void HaltForFire();

    bool IsHoldingForFire() const { return bHoldingForFire; }

    // Only the stop-and-fire reaction suspends execution here. The authoritative
    // order stays in OrderComponent; new orders replace it normally.
    FStrategyOrder GetSuspendedMission() const { return SuspendedMission; }

    UPROPERTY(EditAnywhere, Category="Strategy|Movement")
    float FireReactionReleaseSeconds = 10.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Movement")
    bool HasMovementGoal() const { return bHasMovementGoal; }

    UFUNCTION(BlueprintPure, Category="Strategy|Movement")
    FVector GetMovementGoal() const { return MovementGoal; }

    UFUNCTION(BlueprintPure, Category="Strategy|Movement")
    TArray<FVector> GetRoutePoints() const { return RoutePoints; }

    TArray<FVector> GetRemainingRoutePoints() const
    {
        TArray<FVector> Remaining;
        for (int32 RouteDrawIndex = RoutePointIndex; RouteDrawIndex < RoutePoints.Num(); ++RouteDrawIndex)
            Remaining.Add(RoutePoints[RouteDrawIndex]);
        return Remaining;
    }

    // Actual displacement velocity: APawn::GetVelocity does not track SetActorLocation.
    FVector GetExecutedVelocity() const { return ExecutedVelocity; }

private:
    UPROPERTY(Transient)
    FStrategyOrder SuspendedMission;
    float FireReactionQuietSeconds = 0.0f;
    bool bHoldingForFire = false;
    FVector ExecutedVelocity = FVector::ZeroVector;
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    void BeginMovementForOrder(const FStrategyOrder& Order);
    bool TryRetargetDuringBridge(const FStrategyOrder& Order);
    void FinishMovement();
    bool IsMovementOrder(EStrategyOrderType Type) const;

    UPROPERTY()
    TObjectPtr<AStrategyUnit> OwnerUnit;

    FVector MovementGoal = FVector::ZeroVector;
    FStrategyRoutePlan ActiveRoutePlan;
    TArray<FVector> RoutePoints;
    int32 RoutePointIndex = 0;
    float GoalFacingYaw = 0.0f;
    bool bHasMovementGoal = false;
    bool bApplyGoalFacing = false;
    bool bKeepFacingMove = false;   // side-step: keep the front while moving
    int32 ExecutingOrderSerial = 0;
    bool bCavalryDefileActive = false;
    bool bTurningToGoalFacing = false;
    bool bWaitingForBridge = false;
    bool bBridgeSlotAcquired = false;
    bool bPreserveRoutedState = false;
    float PauseRemainingSeconds = 0.0f;

    /** The campaign's field (for wading the brooks), looked up once. */
    TWeakObjectPtr<class AStrategyCampaignBattlefield> CachedField;
    bool bFieldLookedUp = false;

    void UpdateBridgeFormationState(const FVector& CurrentLocation);
    void UpdateBridgeQueueState(const FVector& CurrentLocation);
    void ReleaseBridgeSlot();
};
