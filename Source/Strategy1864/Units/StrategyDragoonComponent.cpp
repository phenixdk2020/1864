#include "StrategyDragoonComponent.h"

#include "../Combat/StrategyCombatComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Orders/StrategyOrderComponent.h"
#include "CavalryUnit.h"
#include "../Visual/StrategyHumanAnimationStateComponent.h"

UStrategyDragoonComponent::UStrategyDragoonComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyDragoonComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCavalry = Cast<ACavalryUnit>(GetOwner());

    if (OwnerCavalry && OwnerCavalry->MovementExecutor)
    {
        OwnerCavalry->MovementExecutor->MoveSpeedCmPerSecond =
            MountedMoveSpeedCmPerSecond;
    }
}

void UStrategyDragoonComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCavalry ||
        Role != EStrategyCavalryRole::Dragoon ||
        MountedState != EStrategyMountedState::ReturningToMounts)
    {
        return;
    }

    const float DistanceCm = FVector::Dist2D(
        OwnerCavalry->GetActorLocation(),
        HorseParkAnchor);

    const bool bMovementFinished =
        !OwnerCavalry->MovementExecutor ||
        !OwnerCavalry->MovementExecutor->HasMovementGoal();

    if (DistanceCm <= RemountToleranceCm && bMovementFinished)
    {
        CompleteRemount();
    }
}

bool UStrategyDragoonComponent::DismountAtCurrentPosition()
{
    if (!OwnerCavalry ||
        Role != EStrategyCavalryRole::Dragoon ||
        MountedState != EStrategyMountedState::Mounted)
    {
        return false;
    }

    HorseParkAnchor = OwnerCavalry->GetActorLocation();

    DismountedCombatStrength =
        FMath::Clamp(
            FMath::RoundToInt(
                OwnerCavalry->CurrentStrength *
                FMath::Clamp(CombatGroupFraction, 0.0f, 1.0f)),
            0,
            OwnerCavalry->CurrentStrength);

    HorseHolderStrength =
        FMath::Max(
            0,
            OwnerCavalry->CurrentStrength - DismountedCombatStrength);

    MountedState = EStrategyMountedState::Dismounted;

    if (OwnerCavalry->HumanAnimationStateComponent)
    {
        OwnerCavalry->HumanAnimationStateComponent->RequestAction(
            EStrategyHumanAnimationAction::Dismount,
            1.25f);
        OwnerCavalry->HumanAnimationStateComponent->SetMounted(false);
    }

    if (OwnerCavalry->FormationComponent)
    {
        OwnerCavalry->FormationComponent->SetFormation(
            EStrategyFormationType::Line);
    }

    if (OwnerCavalry->MovementExecutor)
    {
        OwnerCavalry->MovementExecutor->MoveSpeedCmPerSecond =
            DismountedMoveSpeedCmPerSecond;
    }

    if (OwnerCavalry->CombatComponent)
    {
        OwnerCavalry->CombatComponent->MaxShotsPerVolley =
            DismountedCombatStrength;
    }

    return true;
}

bool UStrategyDragoonComponent::RequestRemount()
{
    if (!OwnerCavalry ||
        Role != EStrategyCavalryRole::Dragoon ||
        MountedState == EStrategyMountedState::Mounted)
    {
        return false;
    }

    const float DistanceCm = FVector::Dist2D(
        OwnerCavalry->GetActorLocation(),
        HorseParkAnchor);

    if (DistanceCm <= RemountToleranceCm)
    {
        CompleteRemount();
        return true;
    }

    if (!OwnerCavalry->OrderComponent)
    {
        return false;
    }

    FStrategyOrder ReturnOrder;
    ReturnOrder.Type = EStrategyOrderType::Move;
    ReturnOrder.TargetLocation = HorseParkAnchor;
    ReturnOrder.Authority = EStrategyOrderAuthority::DirectPlayer;

    if (!OwnerCavalry->OrderComponent->SetOrder(ReturnOrder))
    {
        return false;
    }

    MountedState = EStrategyMountedState::ReturningToMounts;
    return true;
}

void UStrategyDragoonComponent::CompleteRemount()
{
    if (!OwnerCavalry)
    {
        return;
    }

    MountedState = EStrategyMountedState::Mounted;

    if (OwnerCavalry->HumanAnimationStateComponent)
    {
        OwnerCavalry->HumanAnimationStateComponent->RequestAction(
            EStrategyHumanAnimationAction::Mount,
            1.25f);
        OwnerCavalry->HumanAnimationStateComponent->SetMounted(true);
    }

    DismountedCombatStrength = 0;
    HorseHolderStrength = 0;

    if (OwnerCavalry->FormationComponent)
    {
        OwnerCavalry->FormationComponent->SetFormation(
            EStrategyFormationType::CavalryLine);
    }

    if (OwnerCavalry->MovementExecutor)
    {
        OwnerCavalry->MovementExecutor->MoveSpeedCmPerSecond =
            MountedMoveSpeedCmPerSecond;
    }

    if (OwnerCavalry->CombatComponent)
    {
        OwnerCavalry->CombatComponent->MaxShotsPerVolley =
            FMath::Max(1, OwnerCavalry->CurrentStrength);
    }
}
