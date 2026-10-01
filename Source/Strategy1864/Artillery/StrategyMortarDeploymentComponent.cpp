#include "StrategyMortarDeploymentComponent.h"

UStrategyMortarDeploymentComponent::UStrategyMortarDeploymentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UStrategyMortarDeploymentComponent::RequestEmplace()
{
    if (MobilityState != EStrategyMortarMobilityState::Transport)
    {
        return false;
    }

    MobilityState = EStrategyMortarMobilityState::Emplacing;
    TransitionRemainingSeconds =
        FMath::Max(
            1.0f,
            BaseEmplaceSeconds /
            FMath::Max(0.10f, GetWorkRate() * TerrainPreparationFactor));

    SetComponentTickEnabled(true);
    return true;
}

bool UStrategyMortarDeploymentComponent::RequestPack()
{
    if (MobilityState != EStrategyMortarMobilityState::Deployed)
    {
        return false;
    }

    MobilityState = EStrategyMortarMobilityState::Packing;
    TransitionRemainingSeconds =
        FMath::Max(
            1.0f,
            BasePackSeconds /
            FMath::Max(0.10f, GetWorkRate()));

    SetComponentTickEnabled(true);
    return true;
}

void UStrategyMortarDeploymentComponent::DisableMortar()
{
    MobilityState = EStrategyMortarMobilityState::Disabled;
    TransitionRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);
}

bool UStrategyMortarDeploymentComponent::CanMoveNormally() const
{
    return MobilityState == EStrategyMortarMobilityState::Transport;
}

bool UStrategyMortarDeploymentComponent::CanFire() const
{
    return MobilityState == EStrategyMortarMobilityState::Deployed;
}

float UStrategyMortarDeploymentComponent::GetTransportMobilityFactor() const
{
    const float HorseFactor = FMath::Clamp(static_cast<float>(HorseStrength) / 36.0f, 0.0f, 1.0f);
    const float WagonFactor = FMath::Clamp(static_cast<float>(WagonStrength) / 6.0f, 0.0f, 1.0f);
    const float WeightFactor =
        MortarClass == EStrategyMortarClass::LightHand
        ? 1.0f
        : FMath::Clamp(900.0f / FMath::Max(300.0f, PieceWeightKg), 0.35f, 1.0f);

    return FMath::Clamp(HorseFactor * WagonFactor * WeightFactor, 0.0f, 1.0f);
}

float UStrategyMortarDeploymentComponent::GetManhandlingFactor() const
{
    if (MortarClass == EStrategyMortarClass::HeavySiege)
    {
        return FMath::Clamp(250.0f / FMath::Max(250.0f, PieceWeightKg), 0.05f, 0.35f);
    }

    return FMath::Clamp(450.0f / FMath::Max(100.0f, PieceWeightKg), 0.25f, 1.0f);
}

float UStrategyMortarDeploymentComponent::GetWorkRate() const
{
    const float Party = FMath::Clamp(static_cast<float>(WorkingPartyStrength) / 24.0f, 0.20f, 2.0f);
    const float ClassFactor = MortarClass == EStrategyMortarClass::LightHand ? 1.35f : 1.0f;
    return Party * ClassFactor;
}

void UStrategyMortarDeploymentComponent::SetTerrainPreparationFactor(float NewFactor)
{
    TerrainPreparationFactor = FMath::Clamp(NewFactor, 0.35f, 1.50f);
}

void UStrategyMortarDeploymentComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (MobilityState != EStrategyMortarMobilityState::Emplacing &&
        MobilityState != EStrategyMortarMobilityState::Packing)
    {
        SetComponentTickEnabled(false);
        return;
    }

    TransitionRemainingSeconds =
        FMath::Max(0.0f, TransitionRemainingSeconds - DeltaTime);

    if (TransitionRemainingSeconds <= 0.0f)
    {
        CompleteTransition();
    }
}

void UStrategyMortarDeploymentComponent::CompleteTransition()
{
    if (MobilityState == EStrategyMortarMobilityState::Emplacing)
    {
        MobilityState = EStrategyMortarMobilityState::Deployed;
    }
    else if (MobilityState == EStrategyMortarMobilityState::Packing)
    {
        MobilityState = EStrategyMortarMobilityState::Transport;
    }

    TransitionRemainingSeconds = 0.0f;
    SetComponentTickEnabled(false);
}
