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

    void FinishFormationDrillHold(float DrillRemainingSeconds);
    void RelocateAfterFormationDrill(const FTransform& DrillFrame);
    bool HasColumnDrillApproach() const;
    FVector GetColumnDrillDirection() const { return DrillApproachDirection; }
    void ClearColumnDrillApproach();
    void RefreshColumnDrillApproach();
    /** A column that marched to its approach point: the line the drill must form (middle-rank centre and front),
        kept after the order completes (its serial changes to a hold on arrival). False once a new march begins. */
    bool GetColumnDrillArrival(FVector& OutLineCentre, float& OutFacingYaw) const;
    void ClearColumnDrillArrival() { bDrillArrivalArmed = false; }

    UFUNCTION(BlueprintCallable, Category="Strategy|Movement")
    void RestartCurrentOrderExecution();

    UFUNCTION(BlueprintCallable, Category="Strategy|Movement")
    void PauseMovementForSeconds(float DurationSeconds);

    UFUNCTION(BlueprintPure, Category="Strategy|Movement")
    bool IsTemporarilyPaused() const { return PauseRemainingSeconds > 0.0f || bHoldingForFire; }

    void HaltForFire();

    // Shared execution pause; never writes a replacement order.
    void SuspendMissionForReaction();

    bool IsHoldingForFire() const { return bHoldingForFire; }

    // Stop-and-fire, automatic square and cover share this snapshot. The
    // authoritative order stays in OrderComponent; any new order invalidates it.
    FStrategyOrder GetSuspendedMission() const { return SuspendedMission; }

    UPROPERTY(EditAnywhere, Category="Strategy|Movement")
    float FireReactionReleaseSeconds = 10.0f;

    UFUNCTION(BlueprintPure, Category="Strategy|Movement")
    bool HasMovementGoal() const { return bHasMovementGoal; }
    bool IsWaitingForBridge() const { return bWaitingForBridge; }
    int32 GetRoutePointIndexForTest() const { return RoutePointIndex; }

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
    float AttackRetargetElapsed = 1.0f;
    float AttackLateralSlotCm = 0.0f;
    float AttackStandRangeCm = 0.0f;
    float FireReactionQuietSeconds = 0.0f;
    bool bHoldingForFire = false;
    FVector ExecutedVelocity = FVector::ZeroVector;
    UFUNCTION()
    void HandleOrderChanged(const FStrategyOrder& NewOrder);

    void ConfigureColumnDrillApproach(const FStrategyOrder& DrillOrder);
    int32 DrillApproachCount = 0;
    int32 DrillApproachAttemptSerial = INDEX_NONE;
    int32 DrillApproachSerial = INDEX_NONE;
    FVector DrillApproachDirection = FVector::ForwardVector;
    bool bDrillArrivalArmed = false;
    FVector DrillArrivalCentre = FVector::ZeroVector;
    FVector DrillArrivalPoint = FVector::ZeroVector;
    float DrillArrivalFacing = 0.f;
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
    /** A new order while the men are still forming (a drill, a morph, or under 95 % on their places): the march waits for
        them, at most FormationHoldMaxSeconds (2026-10-22: an order in the middle of a re-form scattered the company). */
    bool bFormationHoldPending = false;
    float FormationHoldStarted = 0.f;
    static constexpr float FormationHoldMaxSeconds = 10.f;

    /** The campaign's field (for wading the brooks), looked up once. */
    TWeakObjectPtr<class AStrategyCampaignBattlefield> CachedField;
    bool bFieldLookedUp = false;

    void UpdateBridgeFormationState(const FVector& CurrentLocation);
    void UpdateBridgeQueueState(const FVector& CurrentLocation);
    void ReleaseBridgeSlot();
};
