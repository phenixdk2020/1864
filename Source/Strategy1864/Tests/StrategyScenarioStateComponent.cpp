#include "StrategyScenarioStateComponent.h"

#include "../Units/StrategyUnit.h"
#include "EngineUtils.h"

UStrategyScenarioStateComponent::UStrategyScenarioStateComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UStrategyScenarioStateComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < EvaluationIntervalSeconds)
    {
        return;
    }

    EvaluationAccumulator = 0.0f;
    EvaluateOutcome();
}

void UStrategyScenarioStateComponent::ResetOutcome()
{
    const EStrategyBattleOutcome OldOutcome = Outcome;
    Outcome = EStrategyBattleOutcome::InProgress;
    bSawFriendlyForce = false;
    bSawOppositionForce = false;
    EvaluationAccumulator = 0.0f;

    if (OldOutcome != Outcome)
    {
        OnOutcomeChanged.Broadcast(OldOutcome, Outcome);
    }
}

void UStrategyScenarioStateComponent::EvaluateOutcome()
{
    if (!GetWorld() || Outcome != EStrategyBattleOutcome::InProgress)
    {
        return;
    }

    int32 FriendlyEffective = 0;
    int32 OppositionEffective = 0;
    int32 FriendlyPresent = 0;
    int32 OppositionPresent = 0;

    for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
    {
        const AStrategyUnit* Unit = *It;
        if (!IsValid(Unit))
        {
            continue;
        }

        const bool bBattleEntity =
            Unit->Echelon == EStrategyEchelon::Company ||
            Unit->Echelon == EStrategyEchelon::Cavalry ||
            Unit->Echelon == EStrategyEchelon::Artillery;

        if (!bBattleEntity)
        {
            continue;
        }

        if (Unit->Side == EStrategySide::Denmark ||
            Unit->Side == EStrategySide::Allied)
        {
            ++FriendlyPresent;
            if (Unit->IsCombatEffective())
            {
                ++FriendlyEffective;
            }
        }
        else if (Unit->Side == EStrategySide::Prussia ||
                 Unit->Side == EStrategySide::Austria ||
                 Unit->Side == EStrategySide::Enemy)
        {
            ++OppositionPresent;
            if (Unit->IsCombatEffective())
            {
                ++OppositionEffective;
            }
        }
    }

    bSawFriendlyForce |= FriendlyPresent > 0;
    bSawOppositionForce |= OppositionPresent > 0;

    if (!bSawFriendlyForce || !bSawOppositionForce)
    {
        return;
    }

    EStrategyBattleOutcome NewOutcome = EStrategyBattleOutcome::InProgress;

    if (FriendlyEffective <= 0 && OppositionEffective <= 0)
    {
        NewOutcome = EStrategyBattleOutcome::Draw;
    }
    else if (OppositionEffective <= 0)
    {
        NewOutcome = EStrategyBattleOutcome::DenmarkVictory;
    }
    else if (FriendlyEffective <= 0)
    {
        NewOutcome = EStrategyBattleOutcome::OppositionVictory;
    }

    if (NewOutcome != Outcome)
    {
        const EStrategyBattleOutcome OldOutcome = Outcome;
        Outcome = NewOutcome;
        OnOutcomeChanged.Broadcast(OldOutcome, Outcome);

        UE_LOG(
            LogTemp,
            Display,
            TEXT("PROJECT1864-SCENARIO: outcome %s"),
            *StaticEnum<EStrategyBattleOutcome>()->GetNameStringByValue(
                static_cast<int64>(Outcome)));
    }
}
