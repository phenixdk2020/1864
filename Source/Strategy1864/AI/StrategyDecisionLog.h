#pragma once

#include "../Units/StrategyUnit.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Observational only: no enemy scans, random draws or changes to telemetry/state.
inline void Strategy1864LogDecision(const AStrategyUnit* DecisionUnit, const TCHAR* DecisionLayer,
    const FString& DecisionWinner, const FString& DecisionReason,
    const FString& DecisionInputs, const FString& DecisionRejected)
{
    if (!DecisionUnit || !FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugDecisions"))) return;
    const FStrategyOrder DecisionOrder = DecisionUnit->OrderComponent
        ? DecisionUnit->OrderComponent->GetCurrentOrder() : FStrategyOrder();
    UE_LOG(LogTemp, Display,
        TEXT("PROJECT1864-DECISION|t=%.3f|unit=%s|layer=%s|winner=%s|reason=%s|order=%d|serial=%d|authority=%d|state=%d|strength=%d/%d|morale=%.2f|cohesion=%.2f|fatigue=%.2f|ammo=%d|formation=%d|inputs=%s|rejected=%s"),
        DecisionUnit->GetWorld() ? DecisionUnit->GetWorld()->GetTimeSeconds() : 0.0f,
        *DecisionUnit->StableUnitId.ToString(), DecisionLayer, *DecisionWinner, *DecisionReason,
        int32(DecisionOrder.Type), DecisionOrder.OrderSerial, int32(DecisionOrder.Authority),
        int32(DecisionUnit->UnitState), DecisionUnit->CurrentStrength, DecisionUnit->InitialStrength,
        DecisionUnit->Morale, DecisionUnit->Cohesion, DecisionUnit->Fatigue,
        DecisionUnit->CombatComponent ? DecisionUnit->CombatComponent->AmmunitionRounds : -1,
        DecisionUnit->FormationComponent ? int32(DecisionUnit->FormationComponent->CurrentFormation) : -1,
        *DecisionInputs, *DecisionRejected);
}

// Keep string formatting and input snapshots out of normal gameplay ticks.
#define STRATEGY1864_DECISION(...) \
    do { if (FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugDecisions"))) \
        Strategy1864LogDecision(__VA_ARGS__); } while (false)
