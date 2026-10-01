#include "StrategyHumanAnimationStateComponent.h"

#include "../Units/StrategyUnit.h"

UStrategyHumanAnimationStateComponent::
UStrategyHumanAnimationStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyHumanAnimationStateComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());

    if (OwnerUnit)
    {
        PreviousLocation = OwnerUnit->GetActorLocation();
    }

    RefreshBaseLocomotion();
    SetCurrentActionInternal(BaseLocomotionAction);
}

void UStrategyHumanAnimationStateComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (!OwnerUnit)
    {
        return;
    }

    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        LastMeasuredSpeedCmPerSecond =
            FVector::Dist2D(
                OwnerUnit->GetActorLocation(),
                PreviousLocation) /
            DeltaTime;
    }

    PreviousLocation = OwnerUnit->GetActorLocation();

    RefreshBaseLocomotion();

    if (ActionRemainingSeconds > 0.0f)
    {
        ActionRemainingSeconds =
            FMath::Max(
                0.0f,
                ActionRemainingSeconds - DeltaTime);

        if (ActionRemainingSeconds <= 0.0f)
        {
            SetCurrentActionInternal(BaseLocomotionAction);
        }

        return;
    }

    if (OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        SetCurrentActionInternal(
            EStrategyHumanAnimationAction::Die);
        return;
    }

    SetCurrentActionInternal(BaseLocomotionAction);
}

void UStrategyHumanAnimationStateComponent::RefreshBaseLocomotion()
{
    if (!OwnerUnit)
    {
        BaseLocomotionAction =
            EStrategyHumanAnimationAction::Idle;
        return;
    }

    if (OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        BaseLocomotionAction =
            EStrategyHumanAnimationAction::Die;
        return;
    }

    if (OwnerUnit->UnitState == EStrategyUnitState::Routed)
    {
        BaseLocomotionAction =
            bMounted
            ? EStrategyHumanAnimationAction::MountedGallop
            : EStrategyHumanAnimationAction::RoutedRun;
        return;
    }

    const bool bMoving =
        LastMeasuredSpeedCmPerSecond > 5.0f ||
        OwnerUnit->UnitState == EStrategyUnitState::Moving;

    if (!bMoving)
    {
        BaseLocomotionAction =
            bMounted
            ? EStrategyHumanAnimationAction::MountedIdle
            : EStrategyHumanAnimationAction::Idle;
        return;
    }

    if (bMounted)
    {
        if (LastMeasuredSpeedCmPerSecond >= 900.0f)
        {
            BaseLocomotionAction =
                EStrategyHumanAnimationAction::MountedGallop;
        }
        else if (LastMeasuredSpeedCmPerSecond >= 650.0f)
        {
            BaseLocomotionAction =
                EStrategyHumanAnimationAction::MountedCanter;
        }
        else if (LastMeasuredSpeedCmPerSecond >= 350.0f)
        {
            BaseLocomotionAction =
                EStrategyHumanAnimationAction::MountedTrot;
        }
        else
        {
            BaseLocomotionAction =
                EStrategyHumanAnimationAction::MountedWalk;
        }

        return;
    }

    BaseLocomotionAction =
        LastMeasuredSpeedCmPerSecond >=
            RunSpeedThresholdCmPerSecond
        ? EStrategyHumanAnimationAction::Run
        : EStrategyHumanAnimationAction::Walk;
}

void UStrategyHumanAnimationStateComponent::RequestAction(
    EStrategyHumanAnimationAction Action,
    float DurationSeconds)
{
    ActionRemainingSeconds =
        FMath::Max(0.0f, DurationSeconds);

    SetCurrentActionInternal(Action);

    if (ActionRemainingSeconds <= 0.0f &&
        Action == EStrategyHumanAnimationAction::None)
    {
        SetCurrentActionInternal(BaseLocomotionAction);
    }
}

void UStrategyHumanAnimationStateComponent::ClearAction()
{
    ActionRemainingSeconds = 0.0f;
    SetCurrentActionInternal(BaseLocomotionAction);
}

void UStrategyHumanAnimationStateComponent::SetMounted(
    bool bNewMounted)
{
    bMounted = bNewMounted;
    RefreshBaseLocomotion();

    if (ActionRemainingSeconds <= 0.0f)
    {
        SetCurrentActionInternal(BaseLocomotionAction);
    }
}

void UStrategyHumanAnimationStateComponent::SetCurrentActionInternal(
    EStrategyHumanAnimationAction NewAction)
{
    if (CurrentAction == NewAction)
    {
        return;
    }

    const EStrategyHumanAnimationAction Previous =
        CurrentAction;

    CurrentAction = NewAction;

    OnAnimationActionChanged.Broadcast(
        Previous,
        CurrentAction);
}
