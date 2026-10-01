#include "StrategyConditionComponent.h"

#include "../Units/StrategyUnit.h"

UStrategyConditionComponent::UStrategyConditionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyConditionComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

void UStrategyConditionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerUnit ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        return;
    }

    float DeltaFatigue = -RecoveryPerSecond;

    if (OwnerUnit->UnitState == EStrategyUnitState::Moving)
    {
        DeltaFatigue = MovingFatiguePerSecond;
    }
    else if (OwnerUnit->UnitState == EStrategyUnitState::Reforming)
    {
        DeltaFatigue = ReformFatiguePerSecond;
    }
    else if (OwnerUnit->UnitState == EStrategyUnitState::Engaged ||
             OwnerUnit->UnitState == EStrategyUnitState::UnderFire)
    {
        DeltaFatigue = CombatFatiguePerSecond;
    }

    OwnerUnit->Fatigue = FMath::Clamp(
        OwnerUnit->Fatigue + DeltaFatigue * DeltaTime,
        0.0f,
        100.0f);
}

float UStrategyConditionComponent::GetMovementSpeedMultiplier() const
{
    if (!OwnerUnit)
    {
        return 1.0f;
    }

    return FMath::Lerp(
        1.0f,
        0.65f,
        FMath::Clamp(OwnerUnit->Fatigue / 100.0f, 0.0f, 1.0f));
}

float UStrategyConditionComponent::GetAccuracyMultiplier() const
{
    if (!OwnerUnit)
    {
        return 1.0f;
    }

    const float ExperienceFactor =
        FMath::Lerp(
            0.85f,
            1.20f,
            FMath::Clamp(OwnerUnit->Experience / 100.0f, 0.0f, 1.0f));

    const float FatigueFactor =
        FMath::Lerp(
            1.0f,
            0.70f,
            FMath::Clamp(OwnerUnit->Fatigue / 100.0f, 0.0f, 1.0f));

    return ExperienceFactor * FatigueFactor;
}

float UStrategyConditionComponent::GetMoraleShockMultiplier() const
{
    if (!OwnerUnit)
    {
        return 1.0f;
    }

    const float ExperienceResistance =
        FMath::Lerp(
            1.10f,
            0.70f,
            FMath::Clamp(OwnerUnit->Experience / 100.0f, 0.0f, 1.0f));

    const float FatiguePenalty =
        FMath::Lerp(
            1.0f,
            1.35f,
            FMath::Clamp(OwnerUnit->Fatigue / 100.0f, 0.0f, 1.0f));

    return ExperienceResistance * FatiguePenalty;
}
