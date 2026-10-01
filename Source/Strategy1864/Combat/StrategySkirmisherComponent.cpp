#include "StrategySkirmisherComponent.h"

#include "../Units/StrategyUnit.h"
#include "../Movement/StrategyMovementExecutorComponent.h"

UStrategySkirmisherComponent::UStrategySkirmisherComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategySkirmisherComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

bool UStrategySkirmisherComponent::DeploySkirmishers(
    EStrategySkirmisherRole NewRole,
    float Fraction)
{
    if (!OwnerUnit ||
        OwnerUnit->Echelon != EStrategyEchelon::Company ||
        OwnerUnit->CurrentStrength < 20 ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed ||
        State != EStrategySkirmisherState::Attached)
    {
        return false;
    }

    ActiveFraction =
        Fraction > 0.0f
        ? FMath::Clamp(Fraction, 0.05f, 0.40f)
        : FMath::Clamp(DefaultDeploymentFraction, 0.05f, 0.40f);

    DetachedStrength =
        FMath::Clamp(
            FMath::RoundToInt(
                OwnerUnit->CurrentStrength * ActiveFraction),
            1,
            OwnerUnit->CurrentStrength - 1);

    Role = NewRole;
    State = EStrategySkirmisherState::Deploying;
    TransitionRemainingSeconds = FMath::Max(0.1f, DeploySeconds);

    if (OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal())
    {
        OwnerUnit->MovementExecutor->PauseMovementForSeconds(
            TransitionRemainingSeconds);
    }

    OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);

    SetComponentTickEnabled(true);
    return true;
}

bool UStrategySkirmisherComponent::RecallSkirmishers()
{
    if (!OwnerUnit ||
        (State != EStrategySkirmisherState::Deployed &&
         State != EStrategySkirmisherState::Deploying))
    {
        return false;
    }

    State = EStrategySkirmisherState::Recalling;
    TransitionRemainingSeconds = FMath::Max(0.1f, RecallSeconds);

    if (OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal())
    {
        OwnerUnit->MovementExecutor->PauseMovementForSeconds(
            TransitionRemainingSeconds);
    }

    OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);

    SetComponentTickEnabled(true);
    return true;
}

void UStrategySkirmisherComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit)
    {
        SetComponentTickEnabled(false);
        return;
    }

    if (State == EStrategySkirmisherState::Deployed)
    {
        UpdateScreenAnchor();
        return;
    }

    if (State == EStrategySkirmisherState::Attached)
    {
        SetComponentTickEnabled(false);
        return;
    }

    TransitionRemainingSeconds =
        FMath::Max(0.0f, TransitionRemainingSeconds - DeltaTime);

    if (TransitionRemainingSeconds > 0.0f)
    {
        return;
    }

    if (State == EStrategySkirmisherState::Deploying)
    {
        CompleteDeploy();
    }
    else if (State == EStrategySkirmisherState::Recalling)
    {
        CompleteRecall();
    }
}

void UStrategySkirmisherComponent::CompleteDeploy()
{
    State = EStrategySkirmisherState::Deployed;
    UpdateScreenAnchor();

    if (OwnerUnit)
    {
        const bool bMoving =
            OwnerUnit->MovementExecutor &&
            OwnerUnit->MovementExecutor->HasMovementGoal();

        OwnerUnit->SetUnitState(
            bMoving
            ? EStrategyUnitState::Moving
            : EStrategyUnitState::Ready);
    }
}

void UStrategySkirmisherComponent::CompleteRecall()
{
    State = EStrategySkirmisherState::Attached;
    DetachedStrength = 0;
    ActiveFraction = 0.0f;
    ScreenAnchor = FVector::ZeroVector;

    if (OwnerUnit)
    {
        OwnerUnit->Cohesion =
            FMath::Clamp(OwnerUnit->Cohesion - 2.0f, 0.0f, 100.0f);

        const bool bMoving =
            OwnerUnit->MovementExecutor &&
            OwnerUnit->MovementExecutor->HasMovementGoal();

        OwnerUnit->SetUnitState(
            bMoving
            ? EStrategyUnitState::Moving
            : EStrategyUnitState::Ready);
    }

    SetComponentTickEnabled(false);
}

void UStrategySkirmisherComponent::UpdateScreenAnchor()
{
    if (!OwnerUnit)
    {
        return;
    }

    const FVector Forward =
        OwnerUnit->GetActorForwardVector().GetSafeNormal2D();
    const FVector Right =
        FVector(-Forward.Y, Forward.X, 0.0f);

    float ForwardScale = ScreenDistanceCm;
    float LateralScale = 0.0f;

    switch (Role)
    {
        case EStrategySkirmisherRole::Recon:
            ForwardScale *= 1.35f;
            break;

        case EStrategySkirmisherRole::Harass:
            ForwardScale *= 1.10f;
            LateralScale = ScreenDistanceCm * 0.35f;
            break;

        case EStrategySkirmisherRole::CoverAdvance:
            ForwardScale *= 0.85f;
            break;

        case EStrategySkirmisherRole::CoverRetreat:
            ForwardScale *= -0.65f;
            break;

        case EStrategySkirmisherRole::Screen:
        default:
            break;
    }

    ScreenAnchor =
        OwnerUnit->GetActorLocation() +
        Forward * ForwardScale +
        Right * LateralScale;
}

float UStrategySkirmisherComponent::GetVolleyDensityMultiplier() const
{
    if (!IsDeployed())
    {
        return 1.0f;
    }

    return FMath::Clamp(
        1.0f - ActiveFraction * 0.70f,
        0.60f,
        1.0f);
}

float UStrategySkirmisherComponent::GetIncomingHitMultiplier() const
{
    if (!IsDeployed())
    {
        return 1.0f;
    }

    return FMath::Clamp(
        1.0f - ActiveFraction * 0.45f,
        0.70f,
        1.0f);
}

float UStrategySkirmisherComponent::GetAwarenessRangeMultiplier() const
{
    if (!IsDeployed())
    {
        return 1.0f;
    }

    switch (Role)
    {
        case EStrategySkirmisherRole::Recon:
            return 1.40f;

        case EStrategySkirmisherRole::Screen:
            return 1.20f;

        case EStrategySkirmisherRole::Harass:
            return 1.15f;

        case EStrategySkirmisherRole::CoverAdvance:
        case EStrategySkirmisherRole::CoverRetreat:
        default:
            return 1.10f;
    }
}
