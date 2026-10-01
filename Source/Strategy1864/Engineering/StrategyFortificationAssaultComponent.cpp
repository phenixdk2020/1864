#include "StrategyFortificationAssaultComponent.h"
#include "StrategyDefensivePosition.h"
#include "../Units/StrategyUnit.h"
#include "../AI/StrategyNCOComponent.h"

UStrategyFortificationAssaultComponent::UStrategyFortificationAssaultComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UStrategyFortificationAssaultComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerUnit = Cast<AStrategyUnit>(GetOwner());
}

bool UStrategyFortificationAssaultComponent::PrepareAssault(
    AStrategyDefensivePosition* Position)
{
    if (!OwnerUnit ||
        !IsValid(Position) ||
        !Position->IsUsable() ||
        OwnerUnit->UnitState == EStrategyUnitState::Routed ||
        OwnerUnit->UnitState == EStrategyUnitState::Destroyed)
    {
        return false;
    }

    TargetPosition = Position;
    AssaultPhase = EStrategyAssaultPhase::Preparing;

    const float StrengthFactor =
        FMath::Clamp(
            static_cast<float>(WorkingPartyStrength) / 20.0f,
            0.25f,
            2.0f);

    const float NCOFactor =
        OwnerUnit->NCOComponent
        ? OwnerUnit->NCOComponent->GetDetachmentControlMultiplier()
        : 1.0f;

    RemainingSeconds =
        BasePreparationSeconds /
        FMath::Max(0.20f, StrengthFactor * NCOFactor * GetEquipmentFactor());

    SetComponentTickEnabled(true);
    return true;
}

bool UStrategyFortificationAssaultComponent::BeginBreach()
{
    if (!OwnerUnit ||
        !IsValid(TargetPosition) ||
        (AssaultPhase != EStrategyAssaultPhase::Approaching &&
         AssaultPhase != EStrategyAssaultPhase::Preparing) ||
        !HasBreachCapability())
    {
        return false;
    }

    AssaultPhase = EStrategyAssaultPhase::Breaching;

    const float StrengthFactor =
        FMath::Clamp(
            static_cast<float>(WorkingPartyStrength) / 20.0f,
            0.25f,
            2.0f);

    RemainingSeconds =
        BaseBreachSeconds /
        FMath::Max(0.20f, StrengthFactor * GetEquipmentFactor());

    SetComponentTickEnabled(true);
    return true;
}

void UStrategyFortificationAssaultComponent::CancelAssault()
{
    AssaultPhase = EStrategyAssaultPhase::Idle;
    RemainingSeconds = 0.0f;
    TargetPosition = nullptr;
    SetComponentTickEnabled(false);
}

float UStrategyFortificationAssaultComponent::GetEquipmentFactor() const
{
    float Factor = 0.60f;
    Factor += Ladders > 0 ? 0.15f : 0.0f;
    Factor += Planks > 0 ? 0.10f : 0.0f;
    Factor += Axes > 0 ? 0.10f : 0.0f;
    Factor += Crowbars > 0 ? 0.05f : 0.0f;
    Factor += ExplosiveCharges > 0 ? 0.30f : 0.0f;
    return FMath::Clamp(Factor, 0.40f, 1.30f);
}

float UStrategyFortificationAssaultComponent::GetExposureMultiplier() const
{
    return FMath::Clamp(
        1.45f - GetEquipmentFactor() * 0.35f,
        0.90f,
        1.35f);
}

float UStrategyFortificationAssaultComponent::GetObstacleCrossingMultiplier() const
{
    return FMath::Clamp(
        1.60f - GetEquipmentFactor() * 0.55f,
        0.70f,
        1.40f);
}

bool UStrategyFortificationAssaultComponent::HasLadderCapability() const
{
    return Ladders > 0;
}

bool UStrategyFortificationAssaultComponent::HasGapCrossingCapability() const
{
    return Planks > 0 || Ladders > 0;
}

bool UStrategyFortificationAssaultComponent::HasBreachCapability() const
{
    return Axes > 0 || Crowbars > 0 || ExplosiveCharges > 0;
}

bool UStrategyFortificationAssaultComponent::ConsumeAssaultEquipment()
{
    if (!HasBreachCapability())
    {
        return false;
    }

    if (ExplosiveCharges > 0)
    {
        --ExplosiveCharges;
    }
    else if (Axes > 0)
    {
        --Axes;
    }
    else if (Crowbars > 0)
    {
        --Crowbars;
    }

    return true;
}

void UStrategyFortificationAssaultComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (AssaultPhase == EStrategyAssaultPhase::Idle ||
        AssaultPhase == EStrategyAssaultPhase::AssaultReady ||
        AssaultPhase == EStrategyAssaultPhase::Failed)
    {
        SetComponentTickEnabled(false);
        return;
    }

    RemainingSeconds =
        FMath::Max(0.0f, RemainingSeconds - DeltaTime);

    if (RemainingSeconds <= 0.0f)
    {
        CompleteCurrentPhase();
    }
}

void UStrategyFortificationAssaultComponent::CompleteCurrentPhase()
{
    if (AssaultPhase == EStrategyAssaultPhase::Preparing)
    {
        AssaultPhase = EStrategyAssaultPhase::Approaching;
        SetComponentTickEnabled(false);
        return;
    }

    if (AssaultPhase == EStrategyAssaultPhase::Breaching)
    {
        const bool bUsedExplosive = ExplosiveCharges > 0;

        if (!IsValid(TargetPosition) || !ConsumeAssaultEquipment())
        {
            AssaultPhase = EStrategyAssaultPhase::Failed;
            SetComponentTickEnabled(false);
            return;
        }

        const float Damage =
            (bUsedExplosive ? 35.0f : 20.0f) *
            GetEquipmentFactor();

        TargetPosition->ApplyStructuralDamage(Damage);
        TargetPosition->MarkBreached(true);
        AssaultPhase = EStrategyAssaultPhase::AssaultReady;
        SetComponentTickEnabled(false);
    }
}
