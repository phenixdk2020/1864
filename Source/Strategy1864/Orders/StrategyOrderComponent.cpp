#include "StrategyOrderComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyUnit.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyFieldworksComponent.h"
#include "../Engineering/StrategyPositionOccupancyComponent.h"
#include "EngineUtils.h"

UStrategyOrderComponent::UStrategyOrderComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyOrderComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasDelayedOrder)
    {
        SetComponentTickEnabled(false);
        return;
    }

    DelayedOrderRemainingSeconds =
        FMath::Max(0.0f, DelayedOrderRemainingSeconds - DeltaTime);

    if (DelayedOrderRemainingSeconds > 0.0f)
    {
        return;
    }

    const FStrategyOrder OrderToIssue = DelayedOrder;
    bHasDelayedOrder = false;
    DelayedOrder = FStrategyOrder();
    SetComponentTickEnabled(false);

    SetOrder(OrderToIssue);
}

bool UStrategyOrderComponent::SetOrder(const FStrategyOrder& NewOrder)
{
    if (!NewOrder.IsValidOrder() || !CanReplaceCurrentOrder(NewOrder))
    {
        return false;
    }

    bHasDelayedOrder = false;
    DelayedOrder = FStrategyOrder();
    DelayedOrderRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);

    if (CurrentOrder.IsValidOrder() && IsPhysicallyExecuting())
    {
        SetExecutionState(EStrategyOrderExecutionState::Superseded);
    }

    if (NewOrder.Type != EStrategyOrderType::Hold && NewOrder.Type != EStrategyOrderType::ArtilleryFireMission)
    {
        if (AStrategyUnit* CoverOrderUnit = Cast<AStrategyUnit>(GetOwner()))
        {
            if (CoverOrderUnit->FieldOfficerComponent) CoverOrderUnit->FieldOfficerComponent->LeaveAutomaticFireCover();
            if (CoverOrderUnit->FieldworksComponent) CoverOrderUnit->FieldworksComponent->CancelFieldworks();
            if (CoverOrderUnit->PositionOccupancyComponent) CoverOrderUnit->PositionOccupancyComponent->LeavePosition();
        }
    }
    FStrategyOrder RouteOrder = NewOrder;
    // A courier may carry an extension of a route whose earlier points have since been reached.
    if (RouteOrder.WaypointRouteId.IsValid() && RouteOrder.WaypointRouteId == CurrentOrder.WaypointRouteId)
        RouteOrder.NextWaypointIndex = FMath::Max(RouteOrder.NextWaypointIndex, CurrentOrder.NextWaypointIndex);
    if (RouteOrder.Type == EStrategyOrderType::Hold) RouteOrder.Waypoints.Reset();
    ResolveAttackDestination(RouteOrder);
    CurrentOrder = RouteOrder;
    CurrentOrder.OrderSerial = NextOrderSerial++;
    SetExecutionState(EStrategyOrderExecutionState::Pending);
    if (const AStrategyUnit* LogUnit = Cast<AStrategyUnit>(GetOwner()))
    {
        const TCHAR* OrderName = TEXT("ordre");
        switch (CurrentOrder.Type)
        {
            case EStrategyOrderType::Move: OrderName = TEXT("FLYT"); break;
            case EStrategyOrderType::AttackHere: OrderName = TEXT("ANGRIB"); break;
            case EStrategyOrderType::DefendHere: OrderName = TEXT("FORSVAR"); break;
            case EStrategyOrderType::Hold: OrderName = TEXT("STOP/HOLD"); break;
            case EStrategyOrderType::Advance: OrderName = TEXT("RYK FREM"); break;
            case EStrategyOrderType::Withdraw: OrderName = TEXT("TILBAGE"); break;
            case EStrategyOrderType::Assemble: OrderName = TEXT("SAML"); break;
            case EStrategyOrderType::ScoutHere: OrderName = TEXT("SPEJD"); break;
            case EStrategyOrderType::Charge: OrderName = TEXT("CHARGE"); break;
            case EStrategyOrderType::ArtilleryFireMission: OrderName = TEXT("ILDMISSION"); break;
            case EStrategyOrderType::Disengage: OrderName = TEXT("AFBRYD"); break;
            default: break;
        }
        const TCHAR* Source = CurrentOrder.Authority == EStrategyOrderAuthority::DirectPlayer ? TEXT("spilleren") :
            CurrentOrder.Authority == EStrategyOrderAuthority::OfficerAI ? TEXT("officerens AI") : TEXT("arvet fra overordnet");
        LogUnit->AddUnitLog(TEXT("Ordre"), FString::Printf(TEXT("%s modtaget"), OrderName),
            FString::Printf(TEXT("givet af %s; mål %.0f m væk%s"), Source, FVector::Dist2D(LogUnit->GetActorLocation(), CurrentOrder.TargetLocation) / 100.0f,
                CurrentOrder.Waypoints.Num() > 1 ? *FString::Printf(TEXT("; %d vejpunkter"), CurrentOrder.Waypoints.Num()) : TEXT("")), 0.0f);
    }
    OnOrderChanged.Broadcast(CurrentOrder);
    return true;
}

bool UStrategyOrderComponent::QueueDelayedOrder(
    const FStrategyOrder& NewOrder,
    float DelaySeconds)
{
    if (!NewOrder.IsValidOrder() || !CanReplaceCurrentOrder(NewOrder))
    {
        return false;
    }

    if (DelaySeconds <= KINDA_SMALL_NUMBER)
    {
        return SetOrder(NewOrder);
    }

    DelayedOrder = NewOrder;
    ResolveAttackDestination(DelayedOrder);
    DelayedOrderRemainingSeconds = DelaySeconds;
    bHasDelayedOrder = true;
    SetComponentTickEnabled(true);
    return true;
}

void UStrategyOrderComponent::ClearOrder()
{
    bHasDelayedOrder = false;
    DelayedOrder = FStrategyOrder();
    DelayedOrderRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);

    CurrentOrder = FStrategyOrder();
    SetExecutionState(EStrategyOrderExecutionState::Idle);
    OnOrderChanged.Broadcast(CurrentOrder);
}

void UStrategyOrderComponent::BeginExecution()
{
    if (!CurrentOrder.IsValidOrder())
    {
        return;
    }

    SetExecutionState(EStrategyOrderExecutionState::Executing);
}

void UStrategyOrderComponent::CompleteExecution()
{
    if (!CurrentOrder.IsValidOrder())
    {
        SetExecutionState(EStrategyOrderExecutionState::Idle);
        return;
    }

    SetExecutionState(EStrategyOrderExecutionState::Completed);
}

void UStrategyOrderComponent::FailExecution()
{
    if (!CurrentOrder.IsValidOrder())
    {
        return;
    }

    SetExecutionState(EStrategyOrderExecutionState::Failed);
}

void UStrategyOrderComponent::MarkPendingTarget()
{
    SetExecutionState(EStrategyOrderExecutionState::PendingTarget);
}

bool UStrategyOrderComponent::IsPhysicallyExecuting() const
{
    return bHasDelayedOrder ||
           ExecutionState == EStrategyOrderExecutionState::PendingTarget ||
           ExecutionState == EStrategyOrderExecutionState::Pending ||
           ExecutionState == EStrategyOrderExecutionState::Executing;
}

bool UStrategyOrderComponent::HasStandingIntent() const
{
    return CurrentOrder.IsValidOrder() && CurrentOrder.IsStandingIntent();
}

bool UStrategyOrderComponent::CanReplaceCurrentOrder(const FStrategyOrder& NewOrder) const
{
    if (!CurrentOrder.IsValidOrder())
    {
        return true;
    }

    const bool bNewAuthorityAtLeastCurrent =
        static_cast<uint8>(NewOrder.Authority) >=
        static_cast<uint8>(CurrentOrder.Authority);

    if (bNewAuthorityAtLeastCurrent)
    {
        return true;
    }

    // A standing player/officer intent remains authoritative until explicitly replaced.
    if (CurrentOrder.IsStandingIntent())
    {
        return false;
    }

    // A finite higher-authority order only blocks lower authority while it is physically active.
    // Once Completed/Failed/Superseded, delegated AI may resume normal tasking.
    return !IsPhysicallyExecuting();
}

void UStrategyOrderComponent::SetExecutionState(EStrategyOrderExecutionState NewState)
{
    if (ExecutionState == NewState)
    {
        return;
    }

    const EStrategyOrderExecutionState OldState = ExecutionState;
    ExecutionState = NewState;
    OnExecutionStateChanged.Broadcast(OldState, ExecutionState);
}


EStrategyCommandVisualState UStrategyOrderComponent::GetCommandVisualState() const
{
    if (IsPhysicallyExecuting())
    {
        return EStrategyCommandVisualState::Blue;
    }

    // Green is reserved for explicit toggle/state controls, not completed movement.
    return EStrategyCommandVisualState::Red;
}

void UStrategyOrderComponent::ResolveAttackDestination(FStrategyOrder& AttackOrder) const
{
    // MOVE is deliberately exact; CHARGE remains a contact/bayonet order.
    if (AttackOrder.Type != EStrategyOrderType::AttackHere && AttackOrder.Type != EStrategyOrderType::Advance) return;
    const AStrategyUnit* AttackUnit = Cast<AStrategyUnit>(GetOwner());
    if (!AttackUnit || !AttackUnit->FireControlComponent || !GetWorld()) return;
    const float AttackRange = AttackUnit->FireControlComponent->GetActiveRangeCm();
    if (AttackRange <= 300.0f) return; // HOLD FIRE has no chosen firing destination.
    AStrategyUnit* AttackEnemy = AttackOrder.AttackTarget.Get();
    if (!IsValid(AttackEnemy) || !AttackEnemy->IsCombatEffective() || AttackEnemy->Side == AttackUnit->Side)
    {
        AttackEnemy = nullptr;
        float AttackNearDistance = 16000.0f; // A point within about 160 m of an enemy company's centre (its line is up to ~90 m wide) identifies the intended enemy.
        for (TActorIterator<AStrategyUnit> AttackIt(GetWorld()); AttackIt; ++AttackIt)
        {
            AStrategyUnit* AttackCandidate = *AttackIt;
            if (!AttackCandidate->IsCombatEffective() || AttackCandidate->Side == AttackUnit->Side ||
                AttackCandidate->Side == EStrategySide::Neutral || AttackCandidate->Echelon == EStrategyEchelon::Headquarters ||
                AttackCandidate->Echelon == EStrategyEchelon::Supply) continue;
            const float AttackDistance = FVector::Dist2D(AttackCandidate->GetActorLocation(), AttackOrder.TargetLocation);
            if (AttackDistance < AttackNearDistance) { AttackNearDistance = AttackDistance; AttackEnemy = AttackCandidate; }
        }
    }
    if (!AttackEnemy) return;
    AttackOrder.AttackTarget = AttackEnemy;
    const FVector AttackFoe = AttackEnemy->GetActorLocation();
    // With a waypoint route the unit arrives from the point before the last one, not from where it stands now.
    FVector AttackFrom = AttackUnit->GetActorLocation();
    if (AttackOrder.Waypoints.Num() >= 2) AttackFrom = AttackOrder.Waypoints[AttackOrder.Waypoints.Num() - 2];
    const FVector AttackFront = (AttackFoe - AttackFrom).GetSafeNormal2D();
    const FVector AttackRight(-AttackFront.Y, AttackFront.X, 0.0f);
    const float AttackLateral = FMath::Clamp(FVector::DotProduct(AttackOrder.TargetLocation - AttackFoe, AttackRight), -12000.0f, 12000.0f);
    if (!ComputeAttackStand(AttackEnemy, AttackUnit, AttackOrder.TargetLocation, AttackOrder.FacingYaw, AttackFrom, AttackLateral)) return;
    AttackOrder.bHasFacing = true;
    if (!AttackOrder.Waypoints.IsEmpty()) AttackOrder.Waypoints.Last() = AttackOrder.TargetLocation;
}

bool UStrategyOrderComponent::ComputeAttackStand(const AStrategyUnit* AttackEnemy, const AStrategyUnit* AttackUnit,
    FVector& OutLocation, float& OutFacingYaw, const FVector& AttackFrom, float AttackLateral)
{
    if (!IsValid(AttackEnemy) || !IsValid(AttackUnit) || !AttackEnemy->IsCombatEffective() ||
        AttackEnemy->Side == AttackUnit->Side || !AttackUnit->FireControlComponent ||
        AttackUnit->FireControlComponent->GetActiveRangeCm() <= 300.0f) return false;
    const FVector AttackFoe = AttackEnemy->GetActorLocation();
    FVector AttackBack = (AttackFrom - AttackFoe).GetSafeNormal2D();
    if (AttackBack.IsNearlyZero()) AttackBack = -AttackUnit->GetActorForwardVector();
    // The lines are rarely parallel. The unit stands where the corner of its line that is farthest from the enemy still reaches the
    // nearest point of his front: it may go a little further than the plain distance, so the whole first rank can fire.
    // The reach is clearly INSIDE the range (about 88 %, at least 5 m): fire control lets a marching unit open fire only when the enemy's
    // far corner is within the range, so a stand-off on the arc itself (range minus 3 m, as before) left part of the front unable to fire.
    const float AttackReach = AttackUnit->FireControlComponent->GetApproachStandRangeCm();
    float AttackStand = AttackReach;
    // The order's own sideways place (the flank plan gives every company a lateral slot) is kept: the stand-off point is
    // measured along the line from the enemy, then moved sideways by the same amount, so the companies stand side by side
    // instead of all on the same spot.
    const FVector AttackOurFront = -AttackBack;
    const FVector AttackOurRight(-AttackOurFront.Y, AttackOurFront.X, 0.0f);
    {
        const float AttackOurHalf = AttackUnit->FormationComponent ? 0.5f * AttackUnit->FormationComponent->EstimateFrontageCm(FMath::Max(1, AttackUnit->CurrentStrength)) : 2400.0f;
        const float AttackFoeHalf = AttackEnemy->FormationComponent ? 0.5f * AttackEnemy->FormationComponent->EstimateFrontageCm(FMath::Max(1, AttackEnemy->CurrentStrength)) : 2400.0f;
        FVector AttackFoeFront = AttackEnemy->GetActorForwardVector();
        AttackFoeFront.Z = 0.0f;
        const FVector AttackFoeRight = FVector(-AttackFoeFront.Y, AttackFoeFront.X, 0.0f).GetSafeNormal();
        const FVector AttackFoeA = AttackFoe - AttackFoeRight * AttackFoeHalf, AttackFoeB = AttackFoe + AttackFoeRight * AttackFoeHalf;
        // The man in the enemy's far corner decides: both ends of his line must lie within the range of OUR front line (the nearest
        // point on it), whichever way his line is turned. Stand as far out as that allows.
        auto AttackDistanceToOurFront = [&](const FVector& AttackPoint, const FVector& AttackCentre)
        {
            const FVector AttackSegA = AttackCentre - AttackOurRight * AttackOurHalf, AttackSegB = AttackCentre + AttackOurRight * AttackOurHalf;
            const FVector AttackAlong = AttackSegB - AttackSegA;
            const float AttackAlongLength = AttackAlong.SizeSquared2D();
            const float AttackT = AttackAlongLength > 1.0f ? FMath::Clamp(FVector::DotProduct(AttackPoint - AttackSegA, AttackAlong) / AttackAlongLength, 0.0f, 1.0f) : 0.0f;
            return FVector::Dist2D(AttackPoint, AttackSegA + AttackAlong * AttackT);
        };
        for (float AttackDistance = AttackReach; AttackDistance > AttackReach * 0.4f; AttackDistance -= 100.0f)
        {
            const FVector AttackCentre = AttackFoe + AttackBack * AttackDistance + AttackOurRight * AttackLateral;
            const float AttackWorst = FMath::Max(AttackDistanceToOurFront(AttackFoeA, AttackCentre), AttackDistanceToOurFront(AttackFoeB, AttackCentre));
            AttackStand = AttackDistance;
            if (AttackWorst <= AttackReach) break;
        }
    }
    OutLocation = AttackFoe + AttackBack * AttackStand + AttackOurRight * AttackLateral;
    OutLocation.Z = AttackUnit->GetActorLocation().Z;
    OutFacingYaw = (-AttackBack).Rotation().Yaw;
    return true;
}

void UStrategyOrderComponent::RetargetActiveOrder(const FVector& NewTarget, float NewFacingYaw)
{
    if (CurrentOrder.Type != EStrategyOrderType::AttackHere && CurrentOrder.Type != EStrategyOrderType::Advance) return;
    CurrentOrder.TargetLocation = NewTarget;
    CurrentOrder.FacingYaw = NewFacingYaw;
    CurrentOrder.bHasFacing = true;
    if (!CurrentOrder.Waypoints.IsEmpty()) CurrentOrder.Waypoints.Last() = NewTarget;
    // Same serial, authority and execution state; no order/execution delegates.
}
