#include "StrategyMovementExecutorComponent.h"
#include "../Visual/StrategyColumnDrill.h"
#include "../Visual/StrategyInfantryVisualComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../AI/StrategyDecisionLog.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Formations/StrategyFormationPolicyComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyThreatReactionComponent.h"
#include "../Combat/StrategyFireDisciplineComponent.h"
#include "../Formations/StrategyFormationTransitionComponent.h"
#include "../Units/StrategyCompanyUnit.h"
#include "../Player/StrategyPlayerController.h"

#include "../Orders/StrategyOrderComponent.h"
#include "../Campaign/StrategyCampaignBattlefield.h"
#include "EngineUtils.h"
#include "../Command/StrategyCommandComponent.h"
#include "../Units/StrategyUnit.h"
#include "../Navigation/StrategyRoutePlannerComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Combat/StrategyConditionComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategyFieldworksComponent.h"
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
    PrimaryComponentTick.TickInterval = 0.f;
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
    AttackRetargetElapsed = 1.0f;
    AttackStandRangeCm = 0.0f;
    AttackLateralSlotCm = 0.0f;
    if (OwnerUnit && NewOrder.AttackTarget.IsValid())
    {
        const FVector RetargetFrom = NewOrder.Waypoints.Num() >= 2 ? NewOrder.Waypoints[NewOrder.Waypoints.Num() - 2] : OwnerUnit->GetActorLocation();
        const FVector RetargetFront = (NewOrder.AttackTarget->GetActorLocation() - RetargetFrom).GetSafeNormal2D();
        const FVector RetargetRight(-RetargetFront.Y, RetargetFront.X, 0.0f);
        AttackLateralSlotCm = FVector::DotProduct(NewOrder.TargetLocation - NewOrder.AttackTarget->GetActorLocation(), RetargetRight);
    }
    SuspendedMission = FStrategyOrder();
    FireReactionQuietSeconds = 0.0f;
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

    if (NewOrder.Type == EStrategyOrderType::Disengage && !NewOrder.bDisengageStep)
    {
        StopMovement();
        OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        OwnerUnit->OrderComponent->BeginExecution();
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

    if (bCommandParentMission && NewOrder.Waypoints.IsEmpty())
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
    if (OwnerUnit)
    {
        OwnerUnit->AddUnitLog(TEXT("Bevægelse"), TEXT("March påbegyndt"), FString::Printf(TEXT("mål %.0f m væk%s"), FVector::Dist2D(OwnerUnit->GetActorLocation(), Order.TargetLocation) / 100.0f,
            Order.Waypoints.Num() > 1 ? *FString::Printf(TEXT(", %d vejpunkter"), Order.Waypoints.Num()) : TEXT("")), 3.0f);
        if (OwnerUnit->FieldOfficerComponent) OwnerUnit->FieldOfficerComponent->LeaveAutomaticFireCover();
        if (OwnerUnit->FieldworksComponent) OwnerUnit->FieldworksComponent->CancelFieldworks();
    }
    DrillApproachAttemptSerial = INDEX_NONE;
    DrillApproachSerial = INDEX_NONE;
    bDrillArrivalArmed = false;
    bHoldingForFire = false;
    ReleaseBridgeSlot();
    bFormationHoldPending = true;
    FormationHoldStarted = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

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

    MovementGoal = Order.ActiveDestination();
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
    bApplyGoalFacing = Order.bHasFacing && Order.NextWaypointIndex >= Order.Waypoints.Num() - 1;
    ConfigureColumnDrillApproach(Order);
    bKeepFacingMove = Order.bKeepFacing && Order.bHasFacing;
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
    ExecutedVelocity = FVector::ZeroVector;

    if (!OwnerUnit || !OwnerUnit->OrderComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if (HasColumnDrillApproach())
    {
        const AStrategyCompanyUnit* DrillCompany = Cast<AStrategyCompanyUnit>(OwnerUnit);
        if (!DrillCompany || !DrillCompany->InfantryVisualComponent ||
            !DrillCompany->InfantryVisualComponent->CanUseColumnDrill() ||
            DrillCompany->InfantryVisualComponent->GetRenderedSoldierCount() != DrillApproachCount)
            ClearColumnDrillApproach();
    }
    if (bHoldingForFire && !OwnerUnit->IsCombatEffective())
    {
        StopMovement();
        return;
    }
    FStrategyOrder CurrentOrder = OwnerUnit->OrderComponent->GetCurrentOrder();
    if (CurrentOrder.OrderSerial != ExecutingOrderSerial ||
        (!IsMovementOrder(CurrentOrder.Type) && !(bHoldingForFire && CurrentOrder.Type == EStrategyOrderType::Hold)))
    {
        StopMovement();
        return;
    }

    AStrategyUnit* RetargetEnemy = CurrentOrder.AttackTarget.Get();
    const bool bRetargetAttack = (CurrentOrder.Type == EStrategyOrderType::AttackHere || CurrentOrder.Type == EStrategyOrderType::Advance) &&
        IsValid(RetargetEnemy) && RetargetEnemy->IsCombatEffective() && RetargetEnemy->Side != OwnerUnit->Side;
    AttackRetargetElapsed += DeltaTime;
    if (bRetargetAttack && AttackRetargetElapsed >= 1.0f)
    {
        AttackRetargetElapsed = 0.0f;
        const FVector RetargetPosition = OwnerUnit->GetActorLocation();
        const FVector RetargetFrom = CurrentOrder.Waypoints.Num() >= 2 ?
            CurrentOrder.Waypoints[CurrentOrder.Waypoints.Num() - 2] : RetargetPosition;
        FVector RetargetStand;
        float RetargetFacing = 0.0f;
        if (UStrategyOrderComponent::ComputeAttackStand(RetargetEnemy, OwnerUnit, RetargetStand, RetargetFacing, RetargetFrom, AttackLateralSlotCm))
        {
            AttackStandRangeCm = FVector::Dist2D(RetargetStand, RetargetEnemy->GetActorLocation());
            const float RetargetDistance = FVector::Dist2D(RetargetPosition, RetargetEnemy->GetActorLocation());
            // An approaching enemy must not pull us through its position, or cause a backwards march.
            if (RetargetDistance <= AttackStandRangeCm)
            {
                RetargetStand = RetargetPosition;
                RetargetFacing = (RetargetEnemy->GetActorLocation() - RetargetPosition).Rotation().Yaw;
            }
            const float RetargetThreshold = FMath::Max(1000.0f, RetargetDistance * 0.1f);
            if (FVector::Dist2D(CurrentOrder.TargetLocation, RetargetStand) > RetargetThreshold)
            {
                const FVector RetargetDestination = CurrentOrder.Waypoints.IsValidIndex(CurrentOrder.NextWaypointIndex) &&
                    CurrentOrder.NextWaypointIndex < CurrentOrder.Waypoints.Num() - 1 ? CurrentOrder.ActiveDestination() : RetargetStand;
                FStrategyRoutePlan RetargetPlan;
                if (OwnerUnit->RoutePlanner) RetargetPlan = OwnerUnit->RoutePlanner->BuildRoutePlan(RetargetPosition, RetargetDestination);
                // Reject an unsafe route without discarding the last usable destination.
                if (!OwnerUnit->RoutePlanner || RetargetPlan.bValid)
                {
                    ReleaseBridgeSlot();
                    OwnerUnit->OrderComponent->RetargetActiveOrder(RetargetStand, RetargetFacing);
                    CurrentOrder = OwnerUnit->OrderComponent->GetCurrentOrder();
                    if (bHoldingForFire) SuspendedMission = CurrentOrder;
                    MovementGoal = RetargetDestination;
                    ActiveRoutePlan = RetargetPlan;
                    RoutePoints = RetargetPlan.Points;
                    if (RoutePoints.IsEmpty()) RoutePoints.Add(MovementGoal);
                    RoutePointIndex = 0;
                    GoalFacingYaw = RetargetFacing;
                    bApplyGoalFacing = CurrentOrder.NextWaypointIndex >= CurrentOrder.Waypoints.Num() - 1;
                    bTurningToGoalFacing = false;
                    bHasMovementGoal = !bHoldingForFire;
                    RefreshColumnDrillApproach();
                    if (bHasMovementGoal && PauseRemainingSeconds <= 0.0f &&
                        (!OwnerUnit->FormationTransition || !OwnerUnit->FormationTransition->IsReforming()))
                        OwnerUnit->SetUnitState(EStrategyUnitState::Moving);
                }
            }
        }
    }
    const bool bInsideAttackStand = bRetargetAttack && AttackStandRangeCm > 0.0f &&
        CurrentOrder.NextWaypointIndex >= CurrentOrder.Waypoints.Num() - 1 &&
        FVector::Dist2D(OwnerUnit->GetActorLocation(), RetargetEnemy->GetActorLocation()) <= AttackStandRangeCm;
    if (!bHasMovementGoal && !bHoldingForFire && !bRetargetAttack)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if (OwnerUnit->FieldOfficerComponent && OwnerUnit->FieldOfficerComponent->IsStandingUpFromFireCover()) return;

    // Higher local reactions halt execution before stop-and-fire can turn the
    // front or deploy a column. Manual squares retain their existing movement.
    const bool bReactionSquare = OwnerUnit->ThreatReactionComponent &&
        OwnerUnit->ThreatReactionComponent->IsRespondingToCavalry();
    const bool bReactionCover = OwnerUnit->FieldOfficerComponent &&
        OwnerUnit->FieldOfficerComponent->IsTakingFireCover();
    if (bReactionSquare || bReactionCover)
    {
        SuspendMissionForReaction();
        FireReactionQuietSeconds = 0.0f;
        return;
    }

    // MOVE reacts only to incoming fire; advance/attack also stop at a legal target.
    const bool bFireMission = bHoldingForFire || CurrentOrder.Type == EStrategyOrderType::Advance ||
        CurrentOrder.Type == EStrategyOrderType::AttackHere;
    AStrategyUnit* MovementFireTarget = OwnerUnit->bOfficerAIEnabled && bFireMission && OwnerUnit->IsA<AStrategyCompanyUnit>() &&
        OwnerUnit->CombatComponent && OwnerUnit->FireControlComponent &&
        (!OwnerUnit->FireDisciplineComponent || OwnerUnit->FireDisciplineComponent->AllowsAutomaticFire()) ?
        OwnerUnit->CombatComponent->FindBestTarget(false) : nullptr;
    // A designated enemy has a tracked stand-off goal; an enemy already inside it halts the march.
    if (MovementFireTarget && CurrentOrder.AttackTarget.Get() == MovementFireTarget &&
        !bHoldingForFire && !bInsideAttackStand && FVector::Dist2D(OwnerUnit->GetActorLocation(), CurrentOrder.TargetLocation) > FMath::Max(ArrivalToleranceCm, 1200.0f))
        MovementFireTarget = nullptr;   // (within 12 m of the stand-off point it may halt and fire: no crawl over the last metres before the first volley)
    // The halt must not come on the arc: an enemy in front of the unit (inside the fire cone) stops an attack/advance only when the
    // far corner of his front, measured from OUR front line as fire control does, is inside the approach range. Before, the middle of
    // his line within the range stopped it, so only the men nearest him could fire. An enemy outside the cone still halts as before.
    if (MovementFireTarget && !bHoldingForFire && OwnerUnit->FireControlComponent->IsInsideFireCone(MovementFireTarget) &&
        !OwnerUnit->FireControlComponent->IsTargetFarSideInFireField(MovementFireTarget, OwnerUnit->FireControlComponent->GetApproachHaltRangeCm()))
        MovementFireTarget = nullptr;
    if (MovementFireTarget)
    {
        FireReactionQuietSeconds = 0.0f;
        const EStrategyFormationType FireBattleFormation = OwnerUnit->FormationPolicy ?
            OwnerUnit->FormationPolicy->GetDestinationFormation() : EStrategyFormationType::Line;
        if (!bHoldingForFire)
        {
            HaltForFire();
        }
        if (OwnerUnit->FormationComponent &&
            (OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
             OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::DefileColumn ||
             OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::CavalryColumn))
            OwnerUnit->FormationReason = TEXT("stop-og-ild: ildkontakt under march, går fra kolonne i kampformation");
            OwnerUnit->FormationComponent->SetFormation(FireBattleFormation);
        if (!OwnerUnit->FormationTransition || !OwnerUnit->FormationTransition->IsReforming())
            OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        if (!CurrentOrder.bKeepFacing || CurrentOrder.Type == EStrategyOrderType::Move)
        {
            const float FireFacingYaw = (MovementFireTarget->GetActorLocation() - OwnerUnit->GetActorLocation()).Rotation().Yaw;
            FRotator FireRotation = OwnerUnit->GetActorRotation();
            FireRotation.Yaw = FMath::FixedTurn(FireRotation.Yaw, FireFacingYaw, TurnSpeedDegreesPerSecond * DeltaTime);
            OwnerUnit->SetActorRotation(FireRotation);
        }
        return;
    }
    if (bHoldingForFire)
    {
        const bool bFireReactionBlocked = OwnerUnit->bOfficerAIEnabled && OwnerUnit->CombatComponent &&
            OwnerUnit->CombatComponent->UnderFireRemainingSeconds > 0.f;
        FireReactionQuietSeconds = bFireReactionBlocked ? 0.f : FireReactionQuietSeconds + DeltaTime;
        STRATEGY1864_DECISION(OwnerUnit, TEXT("Stop-og-ild"), TEXT("Vurder genoptagelse"), TEXT("Missionen må kun genoptages efter ro og uden en højere igangværende reaktion/ny ordre"),
            FString::Printf(TEXT("quietSeconds=%.2f releaseSeconds=%.2f suspendedSerial=%d incomingFire=%d delayedOrder=%d reforming=%d square=%d"),
                FireReactionQuietSeconds, FireReactionReleaseSeconds, SuspendedMission.OrderSerial, bFireReactionBlocked,
                OwnerUnit->OrderComponent->HasDelayedOrder(), OwnerUnit->FormationTransition && OwnerUnit->FormationTransition->IsReforming(),
                OwnerUnit->FormationComponent && OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Square),
            TEXT("resume afvises ved serial-mismatch, utilstrækkelig ro, ventende ordre, reformering eller carré"));
        if (SuspendedMission.IsValidOrder() && SuspendedMission.OrderSerial == CurrentOrder.OrderSerial &&
            FireReactionQuietSeconds >= FMath::Max(0.f, FireReactionReleaseSeconds) &&
            !OwnerUnit->OrderComponent->HasDelayedOrder() &&
            (!OwnerUnit->FieldOfficerComponent || !OwnerUnit->FieldOfficerComponent->IsTakingFireCover()) &&
            (!OwnerUnit->FieldworksComponent || !OwnerUnit->FieldworksComponent->bBuilding) &&
            (!OwnerUnit->FormationTransition || !OwnerUnit->FormationTransition->IsReforming()) &&
            (!OwnerUnit->FormationComponent || OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::Square))
        {
            STRATEGY1864_DECISION(OwnerUnit, TEXT("Stop-og-ild"), TEXT("Genoptag mission"), TEXT("Ingen ildkontakt eller indgående ild i release-perioden"),
                FString::Printf(TEXT("quietSeconds=%.2f suspendedSerial=%d waypoint=%d"), FireReactionQuietSeconds, SuspendedMission.OrderSerial, SuspendedMission.NextWaypointIndex),
                TEXT("fortsat pause: release-hysterese opfyldt"));
            SuspendedMission = FStrategyOrder();
            FireReactionQuietSeconds = 0.f;
            BeginMovementForOrder(CurrentOrder); // Same serial, authority and remaining waypoint route.
        }
        return;
    }

    if (bInsideAttackStand)
    {
        FRotator RetargetRotation = OwnerUnit->GetActorRotation();
        const float RetargetYaw = (RetargetEnemy->GetActorLocation() - OwnerUnit->GetActorLocation()).Rotation().Yaw;
        RetargetRotation.Yaw = FMath::FixedTurn(RetargetRotation.Yaw, RetargetYaw, TurnSpeedDegreesPerSecond * DeltaTime);
        OwnerUnit->SetActorRotation(RetargetRotation);
        if (!OwnerUnit->FormationTransition || !OwnerUnit->FormationTransition->IsReforming()) OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
        return;
    }
    if (!bHasMovementGoal) return;

    // The men first finish the formation they are making (a line, not a column: gathering into column has its own hold).
    if (bFormationHoldPending)
    {
        const AStrategyCompanyUnit* HoldCompany = Cast<AStrategyCompanyUnit>(OwnerUnit);
        const UStrategyInfantryVisualComponent* HoldVisual = HoldCompany ? HoldCompany->InfantryVisualComponent.Get() : nullptr;
        const bool bHoldLine = OwnerUnit->FormationComponent && OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Line;
        bool bHoldForming = false;
        if (HoldVisual && bHoldLine)
        {
            const UStrategyInfantryVisualComponent::FFormationFit HoldFit = HoldVisual->MeasureFormationFit(false);
            bHoldForming = HoldVisual->IsDrilling() || !HoldVisual->IsVisualFormationComplete() || (HoldFit.Men > 0 && HoldFit.Ready < 0.95f);
        }
        const float HoldWaited = GetWorld()->GetTimeSeconds() - FormationHoldStarted;
        if (bHoldForming && HoldWaited < FormationHoldMaxSeconds)
        {
            ExecutedVelocity = FVector::ZeroVector;
            return;
        }
        bFormationHoldPending = false;
        if (HoldWaited > 0.2f)
        {
            UE_LOG(LogTemp, Display, TEXT("Strategy1864FormationHold %s waited %.1f s for the men to form%s"), *OwnerUnit->StableUnitId.ToString(),
                HoldWaited, bHoldForming ? TEXT(" (gave up: still forming)") : TEXT(""));
            OwnerUnit->AddUnitLog(TEXT("Bevægelse"), TEXT("Venter på at formere"), FString::Printf(TEXT("%.0f s"), HoldWaited), 3.0f);
        }
    }

    if (PauseRemainingSeconds > 0.0f)
    {
        PauseRemainingSeconds = FMath::Max(0.0f, PauseRemainingSeconds - DeltaTime);

        // Forming a column for a march: the column's head turns to face the way the unit will go while the men gather.
        if (OwnerUnit->FormationComponent && CurrentOrder.IsValidOrder() &&
            (OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn ||
             OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::CavalryColumn) &&
            OwnerUnit->FormationTransition && OwnerUnit->FormationTransition->IsReforming())
        {
            const FVector MarchTarget = CurrentOrder.Waypoints.IsValidIndex(CurrentOrder.NextWaypointIndex)
                ? CurrentOrder.Waypoints[CurrentOrder.NextWaypointIndex] : CurrentOrder.TargetLocation;
            const FVector ToTarget = (MarchTarget - OwnerUnit->GetActorLocation()).GetSafeNormal2D();
            if (!ToTarget.IsNearlyZero())
            {
                FRotator ColumnFront = OwnerUnit->GetActorRotation();
                ColumnFront.Yaw = FMath::FixedTurn(ColumnFront.Yaw, ToTarget.Rotation().Yaw, TurnSpeedDegreesPerSecond * DeltaTime);
                OwnerUnit->SetActorRotation(ColumnFront);
            }
        }

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

    while (RoutePointIndex < RoutePoints.Num() - 1 &&
           FVector::Dist2D(CurrentLocation, RoutePoints[RoutePointIndex]) <= FMath::Max(ArrivalToleranceCm, 100.0f))
        ++RoutePointIndex;

    if (RoutePointIndex == RoutePoints.Num() - 1 && !HasColumnDrillApproach())
        ConfigureColumnDrillApproach(CurrentOrder);
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

        if (CurrentOrder.Waypoints.IsValidIndex(CurrentOrder.NextWaypointIndex))
        {
            OwnerUnit->OrderComponent->AdvanceWaypoint();
            const FStrategyOrder NextRouteOrder = OwnerUnit->OrderComponent->GetCurrentOrder();
            if (NextRouteOrder.Waypoints.IsValidIndex(NextRouteOrder.NextWaypointIndex))
            {
                BeginMovementForOrder(NextRouteOrder);
                TickComponent(DeltaTime, TickType, ThisTickFunction);
                return;
            }
        }

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

    // Wading a brook (the campaign's field): slow; on the bridges and dry ground at full pace.
    float WadingMultiplier = 1.0f;
    if (!CachedField.IsValid() && !bFieldLookedUp && GetWorld())
    {
        bFieldLookedUp = true;
        for (TActorIterator<AStrategyCampaignBattlefield> It(GetWorld()); It; ++It)
        {
            CachedField = *It;
            break;
        }
    }
    if (const AStrategyCampaignBattlefield* Field = CachedField.Get())
    {
        WadingMultiplier = Field->WadingFactor(CurrentLocation);
    }

    // A side-step goes at three fifths of the pace when the way is more than 50 degrees off the front.
    const float SidestepMultiplier = bKeepFacingMove && FMath::Abs(FMath::FindDeltaAngleDegrees(OwnerUnit->GetActorRotation().Yaw, Direction.Rotation().Yaw)) > 50.0f ? 0.6f : 1.0f;
    const float Step =
        MoveSpeedCmPerSecond *
        (OwnerUnit->FormationComponent && OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::Square ? SquareMoveSpeedMultiplier : 1.0f) *
        SidestepMultiplier *
        WadingMultiplier *
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
    // A march column walks forward along its own front and turns towards the route on a 3 m radius (the head of the snake): it
    // went along the route's corners and turned only its front, so the trail folded at every corner and the inner lane's men stood
    // on one another (2026-10-22 test: 1-19 cm). Inside the turning circle (close and far off to the side) it walks straight.
    bool bColumnArc = false;
    float ColumnArcYaw = OwnerUnit->GetActorRotation().Yaw;
    if (!bKeepFacingMove && !Direction.IsNearlyZero() && AppliedStep > KINDA_SMALL_NUMBER && OwnerUnit->FormationComponent &&
        OwnerUnit->FormationComponent->CurrentFormation == EStrategyFormationType::MarchColumn)
    {
        const float ColumnArcRadius = 300.f;
        const float ColumnArcOff = FMath::FindDeltaAngleDegrees(ColumnArcYaw, Direction.Rotation().Yaw);
        if (!(HorizontalDistance < 2.f * ColumnArcRadius && FMath::Abs(ColumnArcOff) > 60.f))
        {
            const float ColumnArcMax = FMath::RadiansToDegrees(AppliedStep / ColumnArcRadius);
            ColumnArcYaw = FRotator::NormalizeAxis(ColumnArcYaw + FMath::Clamp(ColumnArcOff, -ColumnArcMax, ColumnArcMax));
            const FVector ColumnArcStep = FRotator(0.f, ColumnArcYaw, 0.f).Vector() * AppliedStep;
            NewLocation.X = CurrentLocation.X + ColumnArcStep.X;
            NewLocation.Y = CurrentLocation.Y + ColumnArcStep.Y;
            bColumnArc = true;
        }
    }

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
    if (DeltaTime > SMALL_NUMBER)
    {
        ExecutedVelocity = (OwnerUnit->GetActorLocation() - CurrentLocation) / DeltaTime;
        ExecutedVelocity.Z = 0.0f;
    }

    if (bColumnArc)
    {
        OwnerUnit->SetActorRotation(FRotator(0.f, ColumnArcYaw, 0.f));
    }
    else if (bKeepFacingMove)
    {
        // The front stays to the enemy.
        FRotator Front = OwnerUnit->GetActorRotation();
        Front.Yaw = FMath::FixedTurn(Front.Yaw, GoalFacingYaw, TurnSpeedDegreesPerSecond * DeltaTime);
        OwnerUnit->SetActorRotation(Front);
    }
    else if (!Direction.IsNearlyZero())
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
    if (OwnerUnit) OwnerUnit->AddUnitLog(TEXT("Bevægelse"), TEXT("Nået målet"), TEXT("ankommet til ordrens destination"), 3.0f);
    if (OwnerUnit && OwnerUnit->OrderComponent)
    {
        const FStrategyOrder FinishedRoute = OwnerUnit->OrderComponent->GetCurrentOrder();
        if (FinishedRoute.WaypointRouteId.IsValid() && FinishedRoute.Waypoints.IsEmpty() && FinishedRoute.NextWaypointIndex == 0)
            OwnerUnit->OrderComponent->AdvanceWaypoint();
    }
    ExecutedVelocity = FVector::ZeroVector;
    bHasMovementGoal = false;
    const FStrategyOrder RetargetFinishedOrder = OwnerUnit && OwnerUnit->OrderComponent ? OwnerUnit->OrderComponent->GetCurrentOrder() : FStrategyOrder();
    SetComponentTickEnabled((RetargetFinishedOrder.Type == EStrategyOrderType::AttackHere || RetargetFinishedOrder.Type == EStrategyOrderType::Advance) &&
        RetargetFinishedOrder.AttackTarget.IsValid());

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
    if (OwnerUnit && bHasMovementGoal) OwnerUnit->AddUnitLog(TEXT("Bevægelse"), TEXT("Marchen standset"), TEXT("stop, ildreaktion, ny ordre eller afbrydelse (se linjerne omkring)"), 5.0f);
    SuspendedMission = FStrategyOrder();
    FireReactionQuietSeconds = 0.f;
    bHoldingForFire = false;
    ExecutedVelocity = FVector::ZeroVector;
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
        case EStrategyOrderType::Disengage:
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

    ExecutedVelocity = FVector::ZeroVector;
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
            Order.ActiveDestination());

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

    DrillApproachAttemptSerial = INDEX_NONE;
    DrillApproachSerial = INDEX_NONE;
    MovementGoal = Order.ActiveDestination();
    GoalFacingYaw = Order.FacingYaw;
    bApplyGoalFacing = Order.bHasFacing && Order.NextWaypointIndex >= Order.Waypoints.Num() - 1;
    RefreshColumnDrillApproach();
    bKeepFacingMove = Order.bKeepFacing && Order.bHasFacing;
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

    // A unit that has not got the bridge must stand still before the water, not drift into it: it asks for the slot already
    // when its centre is within about 15 m of the river's edge.
    const bool bNearWater = FMath::Abs(Bridge->SignedBankDistance(CurrentLocation)) <= Bridge->RiverHalfWidthCm + 1500.0f &&
        RoutePointIndex >= ActiveRoutePlan.BridgeEnterPointIndex - 1;
    const bool bAtApproach =
        RoutePointIndex >= ActiveRoutePlan.BridgeEnterPointIndex || bNearWater ||
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

void UStrategyMovementExecutorComponent::HaltForFire()
{
    if (!OwnerUnit || !OwnerUnit->bOfficerAIEnabled) return;
    SuspendMissionForReaction();
}

void UStrategyMovementExecutorComponent::SuspendMissionForReaction()
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent) return;
    const FStrategyOrder FireOldOrder = OwnerUnit->OrderComponent->GetCurrentOrder();
    if (!IsMovementOrder(FireOldOrder.Type) || !OwnerUnit->IsCombatEffective()) return;
    // A completed march must not be restarted by a later reaction at its goal.
    if (!bHasMovementGoal && !bHoldingForFire) return;
    FireReactionQuietSeconds = 0.f;
    if (bHoldingForFire) return;
    StopMovement();
    SuspendedMission = FireOldOrder;
    ExecutingOrderSerial = FireOldOrder.OrderSerial;
    bHoldingForFire = true;
    if (!OwnerUnit->FormationTransition || !OwnerUnit->FormationTransition->IsReforming())
        OwnerUnit->SetUnitState(EStrategyUnitState::Ready);
    STRATEGY1864_DECISION(OwnerUnit, TEXT("ReactionAI"), TEXT("Suspender udførelse"), TEXT("Lokal reaktion bevarer mission, autoritet og waypoint-rute"),
        FString::Printf(TEXT("suspendedType=%d suspendedSerial=%d waypoint=%d releaseSeconds=%.2f"),
            int32(FireOldOrder.Type), FireOldOrder.OrderSerial, FireOldOrder.NextWaypointIndex, FireReactionReleaseSeconds),
        TEXT("permanent HOLD og annullering af rute: afvist; fortsat march: afvist under ildreaktionen"));
    SetComponentTickEnabled(true); // Turn toward a legal fire target without a movement goal.
}


bool UStrategyMovementExecutorComponent::HasColumnDrillApproach() const
{
    return DrillApproachSerial != INDEX_NONE && OwnerUnit && OwnerUnit->OrderComponent &&
        OwnerUnit->OrderComponent->GetCurrentOrder().OrderSerial == DrillApproachSerial;
}

void UStrategyMovementExecutorComponent::ConfigureColumnDrillApproach(const FStrategyOrder& DrillOrder)
{
    const AStrategyCompanyUnit* DrillCompany = Cast<AStrategyCompanyUnit>(OwnerUnit);
    const bool DrillDebug = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugFormation"));
    const auto DrillSkip = [&](const TCHAR* DrillWhy)
    {
        if (DrillDebug && OwnerUnit)
            UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s approach not set: %s (order=%d)"), *OwnerUnit->StableUnitId.ToString(), DrillWhy, DrillOrder.OrderSerial);
    };
    if (!FStrategyColumnDrill::Enabled() || !DrillCompany || !DrillCompany->InfantryVisualComponent || !OwnerUnit->FormationComponent) return;
    if (DrillOrder.bKeepFacing) return;   // a side-step keeps its front: no column drill
    if (!DrillCompany->InfantryVisualComponent->CanUseColumnDrill()) { DrillSkip(TEXT("visual cannot use the drill now")); return; }
    if (OwnerUnit->FormationComponent->CurrentFormation != EStrategyFormationType::MarchColumn) { DrillSkip(TEXT("not in march column yet")); return; }
    if (OwnerUnit->FormationComponent->RankCount != 3 || OwnerUnit->FormationComponent->ColumnWidth != 3) { DrillSkip(TEXT("not 3 ranks")); return; }
    if (DrillOrder.NextWaypointIndex < DrillOrder.Waypoints.Num() - 1 || RoutePoints.IsEmpty()) { DrillSkip(TEXT("not on the last leg")); return; }
    if (DrillApproachAttemptSerial == DrillOrder.OrderSerial) { DrillSkip(TEXT("already attempted for this order")); return; }
    DrillApproachAttemptSerial = DrillOrder.OrderSerial;
    const int32 DrillCount = DrillCompany->InfantryVisualComponent->GetRenderedSoldierCount();
    if (DrillCount <= 0) return;
    const float DrillWidth = ((DrillCount + 2) / 3 - 1) * OwnerUnit->FormationComponent->SoldierLateralSpacingCm;
    const float DrillRadius = FStrategyColumnDrill::Radius(OwnerUnit->FormationComponent->SoldierRankSpacingCm, 0.f);
    const FVector DrillPrevious = RoutePoints.Num() > 1 ? RoutePoints[RoutePoints.Num() - 2] : OwnerUnit->GetActorLocation();
    FVector DrillDirection = (DrillOrder.TargetLocation - DrillPrevious).GetSafeNormal2D();
    // Without a given front the line faces the way the column marched.
    const FVector DrillFacing = DrillOrder.bHasFacing ? FRotator(0.f, DrillOrder.FacingYaw, 0.f).Vector() :
        (DrillDirection.IsNearlyZero() ? OwnerUnit->GetActorForwardVector().GetSafeNormal2D() : DrillDirection);
    if (DrillDirection.IsNearlyZero()) DrillDirection = DrillFacing;
    // The column enters the line axis from the side it comes from: the head (the actor in a column) stops two turn radii
    // short of the near flank on the middle-rank line, and the visual head curves in and walks to the far flank.
    // The order target is the line's middle-rank centre (front-anchored slots put the front rank one rank ahead).
    // Always from the line's left end towards its right (user's sketch 2026-10-20): the head, with the colours, ends on the right flank.
    const float DrillSide = 1.f;
    const FVector DrillAxis = FStrategyColumnDrill::Right(DrillFacing) * DrillSide;
    FVector DrillGoal = DrillOrder.TargetLocation - DrillAxis * (DrillWidth * 0.5f + 2.f * DrillRadius);
    DrillGoal.Z = MovementGoal.Z;
    MovementGoal = DrillGoal;
    // Re-route to the approach point itself: the planner's intermediate points lead towards the old target and would
    // walk the column past the approach point and back again.
    FStrategyRoutePlan DrillRoute;
    if (OwnerUnit->RoutePlanner) DrillRoute = OwnerUnit->RoutePlanner->BuildRoutePlan(OwnerUnit->GetActorLocation(), DrillGoal);
    if (DrillRoute.bValid && DrillRoute.Points.Num() > 0)
    {
        ActiveRoutePlan = DrillRoute;
        RoutePoints = DrillRoute.Points;
        RoutePoints.Last() = DrillGoal;
        RoutePointIndex = 0;
    }
    else RoutePoints.Last() = DrillGoal;
    DrillApproachDirection = DrillAxis;
    DrillApproachSerial = DrillOrder.OrderSerial;
    DrillApproachCount = DrillCount;
    bApplyGoalFacing = false;
    bDrillArrivalArmed = true;
    DrillArrivalCentre = DrillOrder.TargetLocation;
    DrillArrivalPoint = DrillGoal;
    DrillArrivalFacing = DrillFacing.Rotation().Yaw;
    UE_LOG(LogTemp, Display, TEXT("Strategy1864Drill %s approach set order=%d goal=%s axis-yaw=%.1f line-centre=%s front=%.1f"), *OwnerUnit->StableUnitId.ToString(),
        DrillOrder.OrderSerial, *DrillGoal.ToString(), DrillAxis.Rotation().Yaw, *DrillOrder.TargetLocation.ToString(), DrillArrivalFacing);
}

bool UStrategyMovementExecutorComponent::GetColumnDrillArrival(FVector& OutLineCentre, float& OutFacingYaw) const
{
    if (!bDrillArrivalArmed || bHasMovementGoal || !OwnerUnit ||
        FVector::Dist2D(OwnerUnit->GetActorLocation(), DrillArrivalPoint) > FMath::Max(300.f, ArrivalToleranceCm * 3.f)) return false;
    OutLineCentre = DrillArrivalCentre;
    OutFacingYaw = DrillArrivalFacing;
    return true;
}

void UStrategyMovementExecutorComponent::RelocateAfterFormationDrill(const FTransform& DrillFrame)
{
    if (!OwnerUnit) return;
    FVector DrillLocation = DrillFrame.GetLocation();
    // Same ground-height convention as movement: retain the current route-interpolated Z.
    DrillLocation.Z = OwnerUnit->GetActorLocation().Z;
    OwnerUnit->SetActorLocationAndRotation(DrillLocation, FRotator(0.f, DrillFrame.Rotator().Yaw, 0.f));
    ExecutedVelocity = FVector::ZeroVector;
}


void UStrategyMovementExecutorComponent::FinishFormationDrillHold(float DrillRemainingSeconds)
{
    // Release only the long hold owned by this drill; the replacement morph retains its budget.
    PauseRemainingSeconds = FMath::Min(PauseRemainingSeconds, FMath::Max(0.f, DrillRemainingSeconds));
    if (OwnerUnit && OwnerUnit->FormationTransition)
        OwnerUnit->FormationTransition->ReformRemainingSeconds = FMath::Min(
            OwnerUnit->FormationTransition->ReformRemainingSeconds, FMath::Max(0.01f, DrillRemainingSeconds));
}


void UStrategyMovementExecutorComponent::ClearColumnDrillApproach()
{
    if (HasColumnDrillApproach() && bHasMovementGoal)
    {
        const FStrategyOrder DrillCurrent = OwnerUnit->OrderComponent->GetCurrentOrder();
        MovementGoal = DrillCurrent.ActiveDestination();
        if (!RoutePoints.IsEmpty()) RoutePoints.Last() = MovementGoal;
        bApplyGoalFacing = DrillCurrent.bHasFacing && DrillCurrent.NextWaypointIndex >= DrillCurrent.Waypoints.Num() - 1;
    }
    DrillApproachSerial = INDEX_NONE;
}


void UStrategyMovementExecutorComponent::RefreshColumnDrillApproach()
{
    if (!OwnerUnit || !OwnerUnit->OrderComponent || !bHasMovementGoal) return;
    ClearColumnDrillApproach();
    DrillApproachAttemptSerial = INDEX_NONE;
    ConfigureColumnDrillApproach(OwnerUnit->OrderComponent->GetCurrentOrder());
}
