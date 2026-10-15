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
        float AttackNearDistance = 16000.0f; // A point within about 95 m of an enemy company's centre (its line is up to ~90 m wide) identifies the intended enemy.
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
    FVector AttackBack = (AttackFrom - AttackFoe).GetSafeNormal2D();
    if (AttackBack.IsNearlyZero()) AttackBack = -AttackUnit->GetActorForwardVector();
    // The lines are rarely parallel. The unit stands where the corner of its line that is farthest from the enemy still reaches the
    // nearest point of his front: it may go a little further than the plain distance, so the whole first rank can fire.
    const float AttackReach = AttackRange - 300.0f;
    float AttackStand = AttackReach;
    // The order's own sideways place (the flank plan gives every company a lateral slot) is kept: the stand-off point is
    // measured along the line from the enemy, then moved sideways by the same amount, so the companies stand side by side
    // instead of all on the same spot.
    const FVector AttackOurFront = -AttackBack;
    const FVector AttackOurRight(-AttackOurFront.Y, AttackOurFront.X, 0.0f);
    const float AttackLateral = FMath::Clamp(FVector::DotProduct(AttackOrder.TargetLocation - AttackFoe, AttackOurRight), -12000.0f, 12000.0f);
    {
        const float OurHalf = AttackUnit->FormationComponent ? 0.5f * AttackUnit->FormationComponent->EstimateFrontageCm(FMath::Max(1, AttackUnit->CurrentStrength)) : 2400.0f;
        const float FoeHalf = AttackEnemy->FormationComponent ? 0.5f * AttackEnemy->FormationComponent->EstimateFrontageCm(FMath::Max(1, AttackEnemy->CurrentStrength)) : 2400.0f;
        FVector FoeFront = AttackEnemy->GetActorForwardVector();
        FoeFront.Z = 0.0f;
        const FVector FoeRight = FVector(-FoeFront.Y, FoeFront.X, 0.0f).GetSafeNormal();
        const FVector FoeA = AttackFoe - FoeRight * FoeHalf, FoeB = AttackFoe + FoeRight * FoeHalf;
        [[maybe_unused]] auto DistanceToFoeFront = [&](const FVector& Point)
        {
            const FVector Along = FoeB - FoeA;
            const float AlongLength = Along.SizeSquared2D();
            const float T = AlongLength > 1.0f ? FMath::Clamp(FVector::DotProduct(Point - FoeA, Along) / AlongLength, 0.0f, 1.0f) : 0.0f;
            return FVector::Dist2D(Point, FoeA + Along * T);
        };
        const FVector OurFront = -AttackBack;
        const FVector OurRight(-OurFront.Y, OurFront.X, 0.0f);
        // The man in the enemy's far corner decides: both ends of his line must lie within the range of OUR front line (the nearest
        // point on it), whichever way his line is turned. Stand as far out as that allows.
        auto DistanceToOurFront = [&](const FVector& Point, const FVector& Centre)
        {
            const FVector SegA = Centre - OurRight * OurHalf, SegB = Centre + OurRight * OurHalf;
            const FVector Along = SegB - SegA;
            const float AlongLength = Along.SizeSquared2D();
            const float T = AlongLength > 1.0f ? FMath::Clamp(FVector::DotProduct(Point - SegA, Along) / AlongLength, 0.0f, 1.0f) : 0.0f;
            return FVector::Dist2D(Point, SegA + Along * T);
        };
        for (float Distance = AttackReach; Distance > AttackReach * 0.4f; Distance -= 100.0f)
        {
            const FVector Centre = AttackFoe + AttackBack * Distance + AttackOurRight * AttackLateral;
            const float Worst = FMath::Max(DistanceToOurFront(FoeA, Centre), DistanceToOurFront(FoeB, Centre));
            AttackStand = Distance;
            if (Worst <= AttackReach) break;
        }
    }
    AttackOrder.TargetLocation = AttackFoe + AttackBack * AttackStand + AttackOurRight * AttackLateral;
    AttackOrder.TargetLocation.Z = AttackUnit->GetActorLocation().Z;
    AttackOrder.FacingYaw = (-AttackBack).Rotation().Yaw;
    AttackOrder.bHasFacing = true;
    if (FVector::Dist2D(AttackUnit->GetActorLocation(), AttackFoe) < AttackRange - 300.f)
        AttackOrder.bKeepFacing = true; // Already too close: back out facing the enemy.
    if (!AttackOrder.Waypoints.IsEmpty()) AttackOrder.Waypoints.Last() = AttackOrder.TargetLocation;
}
