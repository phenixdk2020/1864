#include "StrategyHorseAnimationStateComponent.h"

#include "../Units/CavalryUnit.h"

UStrategyHorseAnimationStateComponent::
UStrategyHorseAnimationStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyHorseAnimationStateComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCavalry = Cast<ACavalryUnit>(GetOwner());

    if (OwnerCavalry)
    {
        PreviousLocation = OwnerCavalry->GetActorLocation();
    }
}

void UStrategyHorseAnimationStateComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction);

    if (!OwnerCavalry)
    {
        return;
    }

    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        MeasuredSpeedCmPerSecond =
            FVector::Dist2D(
                OwnerCavalry->GetActorLocation(),
                PreviousLocation) /
            DeltaTime;
    }

    PreviousLocation = OwnerCavalry->GetActorLocation();
    RefreshGait();
}

void UStrategyHorseAnimationStateComponent::RefreshGait()
{
    if (MeasuredSpeedCmPerSecond < WalkThresholdCmPerSecond)
    {
        CurrentGait = EStrategyHorseGait::Idle;
    }
    else if (MeasuredSpeedCmPerSecond < TrotThresholdCmPerSecond)
    {
        CurrentGait = EStrategyHorseGait::Walk;
    }
    else if (MeasuredSpeedCmPerSecond < CanterThresholdCmPerSecond)
    {
        CurrentGait = EStrategyHorseGait::Trot;
    }
    else if (MeasuredSpeedCmPerSecond < GallopThresholdCmPerSecond)
    {
        CurrentGait = EStrategyHorseGait::Canter;
    }
    else
    {
        CurrentGait = EStrategyHorseGait::Gallop;
    }
}
