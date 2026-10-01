#include "StrategyFormationTransitionComponent.h"

#include "StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Units/StrategyUnit.h"
#include "../AI/StrategyNCOComponent.h"

UStrategyFormationTransitionComponent::UStrategyFormationTransitionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyFormationTransitionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
    if (!OwnerUnit || !OwnerUnit->FormationComponent)
    {
        SetComponentTickEnabled(false);
        return;
    }

    OwnerUnit->FormationComponent->OnFormationChanged.AddDynamic(
        this,
        &UStrategyFormationTransitionComponent::HandleFormationChanged);
}

void UStrategyFormationTransitionComponent::HandleFormationChanged(
    EStrategyFormationType OldFormation,
    EStrategyFormationType NewFormation)
{
    if (!OwnerUnit || OldFormation == NewFormation)
    {
        return;
    }

    const float StrengthFactor =
        FMath::Max(0.0f, static_cast<float>(OwnerUnit->CurrentStrength) / 100.0f);

    const float NCOFormationFactor =
        OwnerUnit->NCOComponent
        ? OwnerUnit->NCOComponent->GetFormationSpeedMultiplier()
        : 1.0f;

    ReformRemainingSeconds =
        FMath::Max(
            0.1f,
            (BaseReformSeconds + StrengthFactor * SecondsPer100Men) /
            FMath::Max(0.20f, NCOFormationFactor));

    if (OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal())
    {
        OwnerUnit->MovementExecutor->PauseMovementForSeconds(
            ReformRemainingSeconds);
    }

    // Formation transition owns the visible state even though the same
    // movement-pause mechanism is reused under the hood.
    OwnerUnit->SetUnitState(EStrategyUnitState::Reforming);

    SetComponentTickEnabled(true);
}

void UStrategyFormationTransitionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit || ReformRemainingSeconds <= 0.0f)
    {
        SetComponentTickEnabled(false);
        return;
    }

    ReformRemainingSeconds =
        FMath::Max(0.0f, ReformRemainingSeconds - DeltaTime);

    if (ReformRemainingSeconds <= 0.0f)
    {
        CompleteReform();
    }
}

void UStrategyFormationTransitionComponent::CompleteReform()
{
    if (!OwnerUnit)
    {
        return;
    }

    const bool bMissionStillMoving =
        OwnerUnit->MovementExecutor &&
        OwnerUnit->MovementExecutor->HasMovementGoal() &&
        OwnerUnit->OrderComponent &&
        OwnerUnit->OrderComponent->IsPhysicallyExecuting();

    OwnerUnit->SetUnitState(
        bMissionStillMoving
        ? EStrategyUnitState::Moving
        : EStrategyUnitState::Ready);

    SetComponentTickEnabled(false);
}
