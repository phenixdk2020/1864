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
    if (!DecisionUnit) return;
    // The unit's own log (what it decided and why) is kept always; the console line only with the debug flag.
    {
        // Only the decisions a reader wants to see: the routine evaluations and observations repeat all the time and stay out of the log.
        const FString Layer(DecisionLayer);
        const bool bRoutine = Layer == TEXT("Ordrearv") || Layer == TEXT("Kontakt") || Layer == TEXT("Infanterivurdering") ||
            Layer == TEXT("Feltofficer-vurdering") || Layer == TEXT("Ryttervurdering") || DecisionWinner.StartsWith(TEXT("Vurder"));
        if (!bRoutine) DecisionUnit->AddUnitLog(DecisionLayer, DecisionWinner, DecisionReason, Layer == TEXT("Kavalerireaktion") || Layer == TEXT("Kavalerikandidat") ? 45.0f : 20.0f);
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugDecisions"))) return;
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

// The decision points feed the unit log (HUD, report file); the console line needs -Strategy1864DebugDecisions.
#define STRATEGY1864_DECISION(...)     do { Strategy1864LogDecision(__VA_ARGS__); } while (false)
