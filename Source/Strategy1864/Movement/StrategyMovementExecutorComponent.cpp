#include "StrategyMovementExecutorComponent.h"

#include "../Orders/StrategyOrderComponent.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Navigation/StrategyRoutePlannerComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Combat/StrategyConditionComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../AI/StrategyNCOComponent.h"
#include "../Artillery/StrategyMortarBatteryUnit.h"
#include "../Artillery/StrategyMortarDeploymentComponent.h"
#include "../AI/StrategyReconComponent.h"
#include "../Navigation/StrategyRiverBarrier.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryDeploymentComponent.h"
#include "../Logistics/StrategySupplyWagonUnit.h"

UStrategyMovementExecutorComponent::UStrategyMovementExecutorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyMovementExecutorComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    OwnerUnit->OrderComponent->OnOrderChanged.AddDynamic(
        this,
        &UStrategyMovementExecutorComponent::HandleOrderChanged);
}

void UStrategyMovementExecutorComponent::HandleOrderChanged(const FStrategyOrder& NewOrder)
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return;
    }

    if (!NewOrder.IsValidOrder())
    {
        const bool bWasRouted =
            OwnerUnit->UnitState == EStrategyUnitState::Routed;

        StopMovement();

        if (!bWasRouted)
        {
            OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        }

        return;
    }

    if (NewOrder.Type == EStrategyOrderType::Hold)
    {
        StopMovement();
        OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        OwnerUnit->OrderComponent->CompleteExecution();
        return;
    }

    const bool bCommandParentMission =
        OwnerUnit->CommandComponent &&
        OwnerUnit->CommandComponent->CurrentSubordinates.Num() > 0 &&
        (NewOrder.Type == EStrategyOrderType::AttackHere ||
         NewOrder.Type == EStrategyOrderType::DefendHere ||
         NewOrder.Type == EStrategyOrderType::Advance ||
         NewOrder.Type == EStrategyOrderType::Withdraw ||
         NewOrder.Type == EStrategyOrderType::Assemble);

    if (bCommandParentMission)
    {
        StopMovement();
        OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        OwnerUnit->OrderComponent->BeginExecution();
        return;
    }

    if (IsMovementOrder(NewOrder.Type))
    {
        if (!TryRetargetDuringBridge(NewOrder))
        {
            BeginMovementForOrder(NewOrder);
        }
    }
}

void UStrategyMovementExecutorComponent::BeginMovementForOrder(const FStrategyOrder& Order)
{
    ReleaseBridgeSlot();

    if (AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(OwnerUnit))
    {
        const bool bManhandling =
            Battery->DeploymentComponent &&
            Battery->DeploymentComponent->IsManhandling();

        if (!bManhandling && !Battery->CanNormalMove())
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("PROJECT1864-ART: movement rejected for %s because battery is not limbered/mobile."),
                *Battery->StableUnitId.ToString());

            if (Battery->OrderComponent)
            {
                Battery->OrderComponent->FailExecution();
            }

            return;
        }
    }

    if (AStrategySupplyWagonUnit* Wagon =
        Cast<AStrategySupplyWagonUnit>(OwnerUnit))
    {
        if (!Wagon->CanMoveSupplyWagon())
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("PROJECT1864-SUPPLY: movement rejected for %s because wagon mobility is unavailable."),
                *Wagon->StableUnitId.ToString());

            if (Wagon->OrderComponent)
            {
                Wagon->OrderComponent->FailExecution();
            }

            return;
        }
    }

    bPreserveRoutedState =
        OwnerUnit && OwnerUnit->UnitState == EStrategyUnitState::Routed;

    MovementGoal = Order.TargetLocation;
    ActiveRoutePlan = FStrategyRoutePlan();
    RoutePoints.Reset();
    RoutePointIndex = 0;
    bCavalryDefileActive = false;
    bTurningToGoalFacing = false;
    bWaitingForBridge = false;

    if (OwnerUnit && OwnerUnit->RoutePlanner)
    {
        ActiveRoutePlan = OwnerUnit->RoutePlanner->BuildRoutePlan(
            OwnerUnit->GetActorLocation(),
            MovementGoal);

        if (!ActiveRoutePlan.bValid)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("PROJECT1864-MOVE: route rejected for %s: %s"),
                *OwnerUnit->StableUnitId.ToString(),
                *ActiveRoutePlan.FailureReason);

            if (!bPreserveRoutedState)
            {
                OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
            }

            OwnerUnit->OrderComponent->FailExecution();
            bHasMovementGoal = false;
            SetComponentTickEnabled(false);
            return;
        }

        RoutePoints = ActiveRoutePlan.Points;
    }

    if (RoutePoints.Num() == 0)
    {
        RoutePoints.Add(MovementGoal);
    }

    GoalFacingYaw = Order.FacingYaw;
    bApplyGoalFacing = Order.bHasFacing;
    ExecutingOrderSerial = Order.OrderSerial;
    bHasMovementGoal = true;

    if (OwnerUnit && !bPreserveRoutedState)
    {
        OwnerUnit->SetUnitState(EStrategyUnitState::Moving);
    }

    if (OwnerUnit && OwnerUnit->OrderComponent)
    {
        OwnerUnit->OrderComponent->BeginExecution();
    }

    SetComponentTickEnabled(true);
}

void UStrategyMovementExecutorComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasMovementGoal || !OwnerUnit || !OwnerUnit->OrderComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    const FStrategyOrder CurrentOrder = OwnerUnit->OrderComponent->GetCurrentOrder();
    if (CurrentOrder.OrderSerial != ExecutingOrderSerial || !IsMovementOrder(CurrentOrder.Type))
    {
        StopMovement();
        return;
    }

    if (PauseRemainingSeconds > 0.0f)
    {
        PauseRemainingSeconds = FMath::Max(0.0f, PauseRemainingSeconds - DeltaTime);

        if (PauseRemainingSeconds <= 0.0f &&
            OwnerUnit->UnitState == EStrategyUnitState::UnderFire)
        {
            OwnerUnit->SetUnitState(EStrategyUnitState::Moving);
        }

        return;
    }

    const FVector CurrentLocation = OwnerUnit->GetActorLocation();
    UpdateBridgeQueueState(CurrentLocation);

    if (bWaitingForBridge)
    {
        return;
    }

    UpdateBridgeFormationState(CurrentLocation);

    if (bTurningToGoalFacing && bApplyGoalFacing)
    {
        const FRotator CurrentRotation = OwnerUnit->GetActorRotation();
        FRotator DesiredRotation = CurrentRotation;
        DesiredRotation.Yaw = GoalFacingYaw;

        const float RemainingYaw =
            FMath::Abs(
                FMath::FindDeltaAngleDegrees(
                    CurrentRotation.Yaw,
                    GoalFacingYaw));

        if (RemainingYaw <= FinalFacingToleranceDegrees)
        {
            OwnerUnit->SetActorRotation(DesiredRotation);
            bTurningToGoalFacing = false;
            FinishMovement();
            return;
        }

        OwnerUnit->SetActorRotation(
            FMath::RInterpConstantTo(
                CurrentRotation,
                DesiredRotation,
                DeltaTime,
                TurnSpeedDegreesPerSecond));
        return;
    }

    if (!RoutePoints.IsValidIndex(RoutePointIndex))
    {
        FinishMovement();
        return;
    }

    const FVector ActiveWaypoint = RoutePoints[RoutePointIndex];
    const FVector WaypointDelta = ActiveWaypoint - CurrentLocation;
    const FVector FlatDelta(WaypointDelta.X, WaypointDelta.Y, 0.0f);

    const bool bFinalWaypoint = RoutePointIndex == RoutePoints.Num() - 1;
    const float WaypointToleranceCm =
        bFinalWaypoint
        ? ArrivalToleranceCm
        : FMath::Max(ArrivalToleranceCm, 100.0f);

    if (FlatDelta.Size() <= WaypointToleranceCm)
    {
        if (!bFinalWaypoint)
        {
            ++RoutePointIndex;
            return;
        }

        OwnerUnit->SetActorLocation(
            FVector(MovementGoal.X, MovementGoal.Y, CurrentLocation.Z));

        if (bApplyGoalFacing)
        {
            const float RemainingYaw =
                FMath::Abs(
                    FMath::FindDeltaAngleDegrees(
                        OwnerUnit->GetActorRotation().Yaw,
                        GoalFacingYaw));

            if (RemainingYaw > FinalFacingToleranceDegrees)
            {
                bTurningToGoalFacing = true;
                if (!bPreserveRoutedState)
                {
                    OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);
                }
                return;
            }
        }

        FinishMovement();
        return;
    }

    const FVector Direction = FlatDelta.GetSafeNormal();
    const float ConditionMultiplier =
        OwnerUnit->ConditionComponent
        ? OwnerUnit->ConditionComponent->GetMovementSpeedMultiplier()
        : 1.0f;

    const float HorizontalDistance =
        FMath::Max(1.0f, FlatDelta.Size());

    const float SignedSlopeDegrees =
        FMath::RadiansToDegrees(
            FMath::Atan2(
                ActiveWaypoint.Z - CurrentLocation.Z,
                HorizontalDistance));

    float SlopeMultiplier = 1.0f;

    if (SignedSlopeDegrees > 0.0f)
    {
        SlopeMultiplier =
            FMath::Lerp(
                1.0f,
                MinimumUphillSpeedMultiplier,
                FMath::Clamp(SignedSlopeDegrees / 28.0f, 0.0f, 1.0f));
    }
    else if (SignedSlopeDegrees < -2.0f)
    {
        SlopeMultiplier = DownhillSpeedMultiplier;
    }

    const float StanceMultiplier =
        OwnerUnit->StanceComponent
        ? OwnerUnit->StanceComponent->GetMovementMultiplier()
        : 1.0f;

    const float NCOMovementMultiplier =
        OwnerUnit->NCOComponent
        ? OwnerUnit->NCOComponent->GetFormationSpeedMultiplier()
        : 1.0f;

    float ArtilleryMobilityMultiplier = 1.0f;

    if (const AStrategyMortarBatteryUnit* Mortar =
        Cast<AStrategyMortarBatteryUnit>(OwnerUnit))
    {
        ArtilleryMobilityMultiplier =
            Mortar->MortarDeploymentComponent &&
            Mortar->MortarDeploymentComponent->CanMoveNormally()
            ? FMath::Clamp(
                Mortar->MortarDeploymentComponent->GetTransportMobilityFactor(),
                0.05f,
                1.0f)
            : 0.0f;
    }
    else if (const AStrategyArtilleryBatteryUnit* Battery =
        Cast<AStrategyArtilleryBatteryUnit>(OwnerUnit))
    {
        const bool bManhandling =
            Battery->DeploymentComponent &&
            Battery->DeploymentComponent->IsManhandling();

        if (!bManhandling)
        {
            ArtilleryMobilityMultiplier =
                FMath::Clamp(
                    Battery->GetTowedMobilityFactor(),
                    0.05f,
                    1.0f);
        }
    }

    float SupplyMobilityMultiplier = 1.0f;

    if (const AStrategySupplyWagonUnit* Wagon =
        Cast<AStrategySupplyWagonUnit>(OwnerUnit))
    {
        SupplyMobilityMultiplier =
            FMath::Clamp(
                Wagon->GetMobilityFactor(),
                0.05f,
                1.0f);
    }

    const float Step =
        MoveSpeedCmPerSecond *
        ConditionMultiplier *
        SlopeMultiplier *
        StanceMultiplier *
        NCOMovementMultiplier *
        ArtilleryMobilityMultiplier *
        SupplyMobilityMultiplier *
        DeltaTime;
    const float AppliedStep =
        FMath::Min(Step, FlatDelta.Size());

    FVector NewLocation =
        CurrentLocation + Direction * AppliedStep;

    const float VerticalAlpha =
        FMath::Clamp(
            AppliedStep / HorizontalDistance,
            0.0f,
            1.0f);

    NewLocation.Z =
        FMath::Lerp(
            CurrentLocation.Z,
            ActiveWaypoint.Z,
            VerticalAlpha);

    OwnerUnit->SetActorLocation(NewLocation);

    if (!Direction.IsNearlyZero())
    {
        const FRotator DesiredRotation = Direction.Rotation();
        const FRotator NewRotation = FMath::RInterpConstantTo(
            OwnerUnit->GetActorRotation(),
            DesiredRotation,
            DeltaTime,
            TurnSpeedDegreesPerSecond);

        OwnerUnit->SetActorRotation(NewRotation);
    }
}

void UStrategyMovementExecutorComponent::FinishMovement()
{
    bHasMovementGoal = false;
    SetComponentTickEnabled(false);

    if (!OwnerUnit)
    {
        return;
    }

    if (!bPreserveRoutedState)
    {
        OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
    }
    else
    {
        OwnerUnit->SetUnitState(EStrategyUnitState::Routed);
    }

    if (OwnerUnit->OrderComponent)
    {
        const FStrategyOrder CurrentOrder =
            OwnerUnit->OrderComponent->GetCurrentOrder();

        if (CurrentOrder.Type == EStrategyOrderType::ScoutHere &&
            OwnerUnit->ReconComponent)
        {
            OwnerUnit->ReconComponent->OnScoutDestinationReached();
            return;
        }

        OwnerUnit->OrderComponent->CompleteExecution();
    }
}

void UStrategyMovementExecutorComponent::StopMovement()
{
    if (bCavalryDefileActive)
    {
        if (ACavalryUnit* Cavalry = Cast<ACavalryUnit>(OwnerUnit))
        {
            Cavalry->SetDefileMode(false);
        }
    }

    ReleaseBridgeSlot();
    bCavalryDefileActive = false;
    bTurningToGoalFacing = false;
    bWaitingForBridge = false;
    bPreserveRoutedState = false;
    PauseRemainingSeconds = 0.0f;
    bHasMovementGoal = false;
    ExecutingOrderSerial = 0;
    ActiveRoutePlan = FStrategyRoutePlan();
    RoutePoints.Reset();
    RoutePointIndex = 0;
    SetComponentTickEnabled(false);
}

void UStrategyMovementExecutorComponent::UpdateBridgeFormationState(const FVector& CurrentLocation)
{
    ACavalryUnit* Cavalry = Cast<ACavalryUnit>(OwnerUnit);
    if (!Cavalry || !ActiveRoutePlan.bUsesBridge)
    {
        return;
    }

    const bool bAtOrInsideBridgeTransaction =
        ActiveRoutePlan.BridgeEnterPointIndex != INDEX_NONE &&
        ActiveRoutePlan.BridgeExitPointIndex != INDEX_NONE &&
        RoutePointIndex >= ActiveRoutePlan.BridgeEnterPointIndex &&
        RoutePointIndex <= ActiveRoutePlan.BridgeExitPointIndex;

    bool bNearBridgeApproach = false;

    if (RoutePoints.IsValidIndex(ActiveRoutePlan.BridgeEnterPointIndex) &&
        RoutePointIndex <= ActiveRoutePlan.BridgeEnterPointIndex)
    {
        const float DistanceToApproach = FVector::Dist2D(
            CurrentLocation,
            RoutePoints[ActiveRoutePlan.BridgeEnterPointIndex]);

        bNearBridgeApproach = DistanceToApproach <= 3600.0f;
    }

    const bool bShouldUseDefile =
        bAtOrInsideBridgeTransaction || bNearBridgeApproach;

    if (bShouldUseDefile != bCavalryDefileActive)
    {
        Cavalry->SetDefileMode(bShouldUseDefile);
        bCavalryDefileActive = bShouldUseDefile;
    }
}

bool UStrategyMovementExecutorComponent::IsMovementOrder(EStrategyOrderType Type) const
{
    switch (Type)
    {
        case EStrategyOrderType::Move:
        case EStrategyOrderType::AttackHere:
        case EStrategyOrderType::DefendHere:
        case EStrategyOrderType::Advance:
        case EStrategyOrderType::Withdraw:
        case EStrategyOrderType::Assemble:
        case EStrategyOrderType::ScoutHere:
        case EStrategyOrderType::Charge:
            return true;

        default:
            return false;
    }
}


void UStrategyMovementExecutorComponent::PauseMovementForSeconds(float DurationSeconds)
{
    if (!bHasMovementGoal || DurationSeconds <= 0.0f || !OwnerUnit)
    {
        return;
    }

    PauseRemainingSeconds = FMath::Max(PauseRemainingSeconds, DurationSeconds);
    OwnerUnit->SetUnitState(EStrategyUnitState::UnderFire);

    // Deliberately do not change OrderComponent execution state or route.
    // The authoritative parent mission remains active and movement resumes.
    SetComponentTickEnabled(true);
}


bool UStrategyMovementExecutorComponent::TryRetargetDuringBridge(
    const FStrategyOrder& Order)
{
    if (!OwnerUnit ||
        !OwnerUnit->RoutePlanner ||
        !bHasMovementGoal ||
        !ActiveRoutePlan.bUsesBridge ||
        ActiveRoutePlan.BridgeExitPointIndex == INDEX_NONE ||
        RoutePointIndex > ActiveRoutePlan.BridgeExitPointIndex ||
        !RoutePoints.IsValidIndex(ActiveRoutePlan.BridgeExitPointIndex))
    {
        return false;
    }

    const int32 OriginalRoutePointIndex = RoutePointIndex;
    const int32 OriginalBridgeExitIndex = ActiveRoutePlan.BridgeExitPointIndex;

    const FVector PreservedBridgeExit =
        RoutePoints[OriginalBridgeExitIndex];

    TArray<FVector> PreservedPoints;
    const int32 KeepThroughIndex =
        FMath::Clamp(
            OriginalBridgeExitIndex,
            OriginalRoutePointIndex,
            RoutePoints.Num() - 1);

    for (int32 Index = RoutePointIndex; Index <= KeepThroughIndex; ++Index)
    {
        PreservedPoints.Add(RoutePoints[Index]);
    }

    const FStrategyRoutePlan TailPlan =
        OwnerUnit->RoutePlanner->BuildRoutePlan(
            PreservedBridgeExit,
            Order.TargetLocation);

    if (!TailPlan.bValid)
    {
        return false;
    }

    for (const FVector& Point : TailPlan.Points)
    {
        PreservedPoints.Add(Point);
    }

    RoutePoints = PreservedPoints;
    RoutePointIndex = 0;

    ActiveRoutePlan.BridgeEnterPointIndex =
        FMath::Max(
            0,
            ActiveRoutePlan.BridgeEnterPointIndex - OriginalRoutePointIndex);

    ActiveRoutePlan.BridgeExitPointIndex =
        FMath::Max(
            0,
            OriginalBridgeExitIndex - OriginalRoutePointIndex);
    ActiveRoutePlan.bUsesBridge = true;
    ActiveRoutePlan.bValid = true;

    MovementGoal = Order.TargetLocation;
    GoalFacingYaw = Order.FacingYaw;
    bApplyGoalFacing = Order.bHasFacing;
    ExecutingOrderSerial = Order.OrderSerial;
    bTurningToGoalFacing = false;

    OwnerUnit->OrderComponent->BeginExecution();
    SetComponentTickEnabled(true);

    UE_LOG(
        LogTemp,
        Display,
        TEXT("PROJECT1864-MOVE: %s retargeted while preserving active bridge transaction."),
        *OwnerUnit->StableUnitId.ToString());

    return true;
}


void UStrategyMovementExecutorComponent::RestartCurrentOrderExecution()
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        return;
    }

    const FStrategyOrder CurrentOrder =
        OwnerUnit->OrderComponent->GetCurrentOrder();

    if (CurrentOrder.IsValidOrder() && IsMovementOrder(CurrentOrder.Type))
    {
        BeginMovementForOrder(CurrentOrder);
    }
}


void UStrategyMovementExecutorComponent::UpdateBridgeQueueState(
    const FVector& CurrentLocation)
{
    bWaitingForBridge = false;

    if (!ActiveRoutePlan.bUsesBridge ||
        !IsValid(ActiveRoutePlan.BridgeBarrier) ||
        ActiveRoutePlan.BridgeEnterPointIndex == INDEX_NONE ||
        ActiveRoutePlan.BridgeExitPointIndex == INDEX_NONE)
    {
        return;
    }

    AStrategyRiverBarrier* Bridge = ActiveRoutePlan.BridgeBarrier;

    if (RoutePointIndex > ActiveRoutePlan.BridgeExitPointIndex)
    {
        ReleaseBridgeSlot();
        return;
    }

    if (!RoutePoints.IsValidIndex(ActiveRoutePlan.BridgeEnterPointIndex))
    {
        return;
    }

    const FVector Approach =
        RoutePoints[ActiveRoutePlan.BridgeEnterPointIndex];

    const bool bAtApproach =
        RoutePointIndex >= ActiveRoutePlan.BridgeEnterPointIndex ||
        FVector::Dist2D(CurrentLocation, Approach) <= 500.0f;

    if (!bAtApproach)
    {
        return;
    }

    if (!bBridgeSlotAcquired)
    {
        bBridgeSlotAcquired = Bridge->TryAcquireCrossing(OwnerUnit);
    }

    bWaitingForBridge = !bBridgeSlotAcquired;
}

void UStrategyMovementExecutorComponent::ReleaseBridgeSlot()
{
    if (bBridgeSlotAcquired &&
        IsValid(ActiveRoutePlan.BridgeBarrier) &&
        OwnerUnit)
    {
        ActiveRoutePlan.BridgeBarrier->ReleaseCrossing(OwnerUnit);
    }

    bBridgeSlotAcquired = false;
    bWaitingForBridge = false;
}
