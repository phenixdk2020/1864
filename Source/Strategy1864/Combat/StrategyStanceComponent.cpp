#include "StrategyStanceComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"

#include "../Units/StrategyUnit.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"

UStrategyStanceComponent::UStrategyStanceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UStrategyStanceComponent::SetStance(EStrategyStance NewStance)
{
    AStrategyUnit* Unit = Cast<AStrategyUnit>(GetOwner());
    if (!Unit ||
        Unit->Echelon != EStrategyEchelon::Company ||
        (NewStance != EStrategyStance::Standing &&
            ((Unit->FieldOfficerComponent && Unit->FieldOfficerComponent->IsCharging()) ||
             (Unit->FormationComponent && Unit->FormationComponent->CurrentFormation == EStrategyFormationType::Square))) ||
        Unit->UnitState == EStrategyUnitState::Routed ||
        Unit->UnitState == EStrategyUnitState::Destroyed)
    {
        return false;
    }

    if (NewStance != EStrategyStance::Prone && Unit->FieldOfficerComponent)
        Unit->FieldOfficerComponent->LeaveAutomaticFireCover();
    Stance = NewStance;
    return true;
}

float UStrategyStanceComponent::GetMovementMultiplier() const
{
    if (Stance == EStrategyStance::Prone)
    {
        return FMath::Max(0.05f, ProneMovementMultiplier);
    }
    if (Stance == EStrategyStance::Kneeling)
    {
        return FMath::Max(0.10f, KneelingMovementMultiplier);
    }
    return 1.0f;
}

float UStrategyStanceComponent::GetReloadMultiplier() const
{
    if (Stance == EStrategyStance::Prone)
    {
        return FMath::Max(0.10f, ProneReloadMultiplier);
    }
    if (Stance == EStrategyStance::Kneeling)
    {
        return FMath::Max(0.10f, KneelingReloadMultiplier);
    }
    return 1.0f;
}

float UStrategyStanceComponent::GetIncomingHitMultiplier() const
{
    const AStrategyUnit* CoverStanceUnit = Cast<AStrategyUnit>(GetOwner());
    if (Stance == EStrategyStance::Prone && CoverStanceUnit && CoverStanceUnit->FieldOfficerComponent &&
        CoverStanceUnit->FieldOfficerComponent->IsTakingFireCover()) return 0.60f;
    if (Stance == EStrategyStance::Prone)
    {
        return FMath::Clamp(ProneIncomingHitMultiplier, 0.05f, 1.0f);
    }
    if (Stance == EStrategyStance::Kneeling)
    {
        return FMath::Clamp(KneelingIncomingHitMultiplier, 0.05f, 1.0f);
    }
    return 1.0f;
}
