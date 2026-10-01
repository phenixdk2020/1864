#include "StrategyFireDrillComponent.h"

#include "../Formations/StrategyFormationComponent.h"
#include "../Units/StrategyUnit.h"

UStrategyFireDrillComponent::UStrategyFireDrillComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

EStrategyFireDrillMode UStrategyFireDrillComponent::NormalizeLegacyMode(
    EStrategyFireDrillMode Mode)
{
    switch (Mode)
    {
        case EStrategyFireDrillMode::AlternatingSections:
            return EStrategyFireDrillMode::FireByRank;

        case EStrategyFireDrillMode::KneelingFrontRank:
            return EStrategyFireDrillMode::TwoRankFire;

        default:
            return Mode;
    }
}

int32 UStrategyFireDrillComponent::GetConfiguredRankCount() const
{
    const AStrategyUnit* Unit = Cast<AStrategyUnit>(GetOwner());

    return Unit && Unit->FormationComponent
        ? FMath::Max(1, Unit->FormationComponent->RankCount)
        : 2;
}

bool UStrategyFireDrillComponent::SetDrillMode(
    EStrategyFireDrillMode NewMode)
{
    const AStrategyUnit* Unit = Cast<AStrategyUnit>(GetOwner());
    if (!Unit ||
        Unit->UnitState == EStrategyUnitState::Routed ||
        Unit->UnitState == EStrategyUnitState::Destroyed)
    {
        return false;
    }

    NewMode = NormalizeLegacyMode(NewMode);

    if (!IsDrillModeUnlocked(NewMode))
    {
        return false;
    }

    DrillMode = NewMode;

    if (DrillMode != EStrategyFireDrillMode::FireByRank)
    {
        ResetFireByRankCycle();
    }

    return true;
}

void UStrategyFireDrillComponent::SetResearchLevel(
    EStrategyFireDrillResearchLevel NewLevel)
{
    ResearchLevel = NewLevel;

    if (!IsDrillModeUnlocked(DrillMode))
    {
        DrillMode = GetHighestUnlockedDrillMode();
    }

    if (DrillMode != EStrategyFireDrillMode::FireByRank)
    {
        ResetFireByRankCycle();
    }
}

void UStrategyFireDrillComponent::SetLoadingMethod(
    EStrategyLoadingMethod NewMethod)
{
    LoadingMethod = NewMethod;
}

EStrategyFireDrillResearchLevel
UStrategyFireDrillComponent::GetRequiredResearchLevel(
    EStrategyFireDrillMode Mode) const
{
    switch (NormalizeLegacyMode(Mode))
    {
        case EStrategyFireDrillMode::TwoRankFire:
            return EStrategyFireDrillResearchLevel::TwoRankFire;

        case EStrategyFireDrillMode::FireByRank:
            return EStrategyFireDrillResearchLevel::FireByRank;

        case EStrategyFireDrillMode::Volley:
            return EStrategyFireDrillResearchLevel::ControlledVolley;

        case EStrategyFireDrillMode::Independent:
            return EStrategyFireDrillResearchLevel::IndependentFire;

        case EStrategyFireDrillMode::FrontRank:
        default:
            return EStrategyFireDrillResearchLevel::FrontRankFire;
    }
}

bool UStrategyFireDrillComponent::IsDrillModeUnlocked(
    EStrategyFireDrillMode Mode) const
{
    const uint8 CurrentLevel =
        static_cast<uint8>(ResearchLevel);

    const uint8 RequiredLevel =
        static_cast<uint8>(GetRequiredResearchLevel(Mode));

    return CurrentLevel >= RequiredLevel;
}

EStrategyFireDrillMode
UStrategyFireDrillComponent::GetHighestUnlockedDrillMode() const
{
    switch (ResearchLevel)
    {
        case EStrategyFireDrillResearchLevel::AdvancedFireDrill:
        case EStrategyFireDrillResearchLevel::IndependentFire:
            return EStrategyFireDrillMode::Independent;

        case EStrategyFireDrillResearchLevel::ControlledVolley:
            return EStrategyFireDrillMode::Volley;

        case EStrategyFireDrillResearchLevel::FireByRank:
            return EStrategyFireDrillMode::FireByRank;

        case EStrategyFireDrillResearchLevel::TwoRankFire:
            return EStrategyFireDrillMode::TwoRankFire;

        case EStrategyFireDrillResearchLevel::FrontRankFire:
        default:
            return EStrategyFireDrillMode::FrontRank;
    }
}

bool UStrategyFireDrillComponent::CanUseAutomaticDrillSelection() const
{
    return ResearchLevel ==
        EStrategyFireDrillResearchLevel::AdvancedFireDrill;
}

bool UStrategyFireDrillComponent::SelectAutomaticDrillMode(
    bool bPreferShockVolley,
    bool bPreferContinuousFire)
{
    if (!CanUseAutomaticDrillSelection())
    {
        return false;
    }

    if (bPreferShockVolley)
    {
        return SetDrillMode(EStrategyFireDrillMode::Volley);
    }

    if (bPreferContinuousFire)
    {
        return SetDrillMode(EStrategyFireDrillMode::Independent);
    }

    return SetDrillMode(EStrategyFireDrillMode::FireByRank);
}

bool UStrategyFireDrillComponent::IsFormationSlotEligibleToFire(
    int32 SlotIndex) const
{
    if (SlotIndex < 0)
    {
        return false;
    }

    const int32 RankCount = GetConfiguredRankCount();
    const int32 RankIndex = SlotIndex % RankCount;

    switch (NormalizeLegacyMode(DrillMode))
    {
        case EStrategyFireDrillMode::FrontRank:
            return RankIndex == 0;

        case EStrategyFireDrillMode::TwoRankFire:
            return RankIndex < FMath::Min(2, RankCount);

        case EStrategyFireDrillMode::FireByRank:
            return RankIndex == ActiveFireByRankIndex;

        case EStrategyFireDrillMode::Volley:
        case EStrategyFireDrillMode::Independent:
        default:
            return true;
    }
}

float UStrategyFireDrillComponent::GetEligibleFiringFraction(
    EStrategyStance Stance) const
{
    const int32 RankCount = GetConfiguredRankCount();
    const EStrategyFireDrillMode EffectiveMode =
        NormalizeLegacyMode(DrillMode);

    float Fraction = 1.0f;

    switch (EffectiveMode)
    {
        case EStrategyFireDrillMode::FrontRank:
            Fraction = 1.0f / static_cast<float>(RankCount);
            break;

        case EStrategyFireDrillMode::TwoRankFire:
            Fraction =
                static_cast<float>(FMath::Min(2, RankCount)) /
                static_cast<float>(RankCount);
            break;

        case EStrategyFireDrillMode::FireByRank:
            Fraction = 1.0f / static_cast<float>(RankCount);
            break;

        case EStrategyFireDrillMode::Independent:
            Fraction = 0.90f;
            break;

        case EStrategyFireDrillMode::Volley:
        default:
            Fraction = 1.0f;
            break;
    }

    if (Stance == EStrategyStance::Prone &&
        LoadingMethod == EStrategyLoadingMethod::MuzzleLoader)
    {
        Fraction *= 0.65f;
    }

    return FMath::Clamp(Fraction, 0.10f, 1.0f);
}

float UStrategyFireDrillComponent::GetReloadMultiplier(
    EStrategyStance Stance) const
{
    const float Training =
        FMath::Clamp(DrillTraining / 100.0f, 0.0f, 1.0f);

    float Multiplier = FMath::Lerp(1.15f, 0.90f, Training);

    const EStrategyFireDrillMode EffectiveMode =
        NormalizeLegacyMode(DrillMode);

    if (EffectiveMode == EStrategyFireDrillMode::Independent)
    {
        Multiplier *= 0.92f;
    }
    else if (EffectiveMode == EStrategyFireDrillMode::FireByRank)
    {
        // Each pulse represents one rank. With N ranks, the next rank fires
        // after roughly 1/N of a full reload cycle, so the first rank has had
        // a complete reload period when the cycle returns to it.
        Multiplier *=
            1.0f / static_cast<float>(GetConfiguredRankCount());
    }

    if (Stance == EStrategyStance::Prone)
    {
        Multiplier *=
            LoadingMethod == EStrategyLoadingMethod::BreechLoader
            ? 1.02f
            : 1.45f;
    }
    else if (Stance == EStrategyStance::Kneeling)
    {
        Multiplier *=
            LoadingMethod == EStrategyLoadingMethod::BreechLoader
            ? 0.98f
            : 1.08f;
    }

    return FMath::Clamp(Multiplier, 0.30f, 1.75f);
}

float UStrategyFireDrillComponent::GetVolleyCoordinationMultiplier() const
{
    const float Discipline =
        FMath::Clamp(FireDiscipline / 100.0f, 0.0f, 1.0f);

    return FMath::Lerp(0.75f, 1.08f, Discipline);
}

float UStrategyFireDrillComponent::GetIndependentFireCadenceMultiplier() const
{
    const float Training =
        FMath::Clamp(DrillTraining / 100.0f, 0.0f, 1.0f);

    return NormalizeLegacyMode(DrillMode) ==
        EStrategyFireDrillMode::Independent
        ? FMath::Lerp(0.95f, 1.20f, Training)
        : 1.0f;
}

bool UStrategyFireDrillComponent::SupportsProneReload() const
{
    return LoadingMethod == EStrategyLoadingMethod::BreechLoader ||
           DrillTraining >= 70.0f;
}

bool UStrategyFireDrillComponent::SupportsKneelingFrontRank() const
{
    return static_cast<uint8>(ResearchLevel) >=
               static_cast<uint8>(
                   EStrategyFireDrillResearchLevel::TwoRankFire) &&
           DrillTraining >= 35.0f;
}

bool UStrategyFireDrillComponent::IsAlternatingFire() const
{
    return NormalizeLegacyMode(DrillMode) ==
        EStrategyFireDrillMode::FireByRank;
}

void UStrategyFireDrillComponent::AdvanceFireByRankCycle()
{
    if (NormalizeLegacyMode(DrillMode) !=
        EStrategyFireDrillMode::FireByRank)
    {
        ActiveFireByRankIndex = 0;
        return;
    }

    const int32 RankCount = GetConfiguredRankCount();

    ActiveFireByRankIndex =
        (ActiveFireByRankIndex + 1) % RankCount;
}

void UStrategyFireDrillComponent::ResetFireByRankCycle()
{
    ActiveFireByRankIndex = 0;
}

float UStrategyFireDrillComponent::GetDoctrineReadiness() const
{
    return FMath::Clamp(
        (DrillTraining + FireDiscipline) / 200.0f,
        0.0f,
        1.0f);
}
