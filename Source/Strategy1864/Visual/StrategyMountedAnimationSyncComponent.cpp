#include "StrategyMountedAnimationSyncComponent.h"

#include "StrategyHorseAnimationStateComponent.h"
#include "StrategyHumanAnimationStateComponent.h"
#include "../Units/CavalryUnit.h"
#include "../Units/StrategyDragoonComponent.h"

UStrategyMountedAnimationSyncComponent::
UStrategyMountedAnimationSyncComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyMountedAnimationSyncComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCavalry = Cast<ACavalryUnit>(GetOwner());

    if (OwnerCavalry)
    {
        HorseState =
            OwnerCavalry->FindComponentByClass<
                UStrategyHorseAnimationStateComponent>();

        HumanState =
            OwnerCavalry->FindComponentByClass<
                UStrategyHumanAnimationStateComponent>();
    }
}

void UStrategyMountedAnimationSyncComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    SyncRiderToHorse();
}

void UStrategyMountedAnimationSyncComponent::SyncRiderToHorse()
{
    if (!OwnerCavalry ||
        !HorseState ||
        !HumanState)
    {
        return;
    }

    const bool bMounted =
        !OwnerCavalry->DragoonComponent ||
        OwnerCavalry->DragoonComponent->MountedState ==
            EStrategyMountedState::Mounted;

    HumanState->SetMounted(bMounted);

    if (!bMounted ||
        HumanState->IsTransientActionActive())
    {
        return;
    }

    switch (HorseState->CurrentGait)
    {
        case EStrategyHorseGait::Walk:
            HumanState->RequestAction(
                EStrategyHumanAnimationAction::MountedWalk);
            break;

        case EStrategyHorseGait::Trot:
            HumanState->RequestAction(
                EStrategyHumanAnimationAction::MountedTrot);
            break;

        case EStrategyHorseGait::Canter:
            HumanState->RequestAction(
                EStrategyHumanAnimationAction::MountedCanter);
            break;

        case EStrategyHorseGait::Gallop:
            HumanState->RequestAction(
                EStrategyHumanAnimationAction::MountedGallop);
            break;

        case EStrategyHorseGait::Idle:
        default:
            HumanState->RequestAction(
                EStrategyHumanAnimationAction::MountedIdle);
            break;
    }
}
