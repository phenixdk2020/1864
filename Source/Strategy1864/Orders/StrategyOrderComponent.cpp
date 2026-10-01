#include "StrategyOrderComponent.h"

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

    CurrentOrder = NewOrder;
    CurrentOrder.OrderSerial = NextOrderSerial++;
    SetExecutionState(EStrategyOrderExecutionState::Pending);
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
