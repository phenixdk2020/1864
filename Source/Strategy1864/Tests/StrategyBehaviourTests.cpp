#include "StrategyOOBTestScenario.h"
#include "../Units/StrategyCompanyUnit.h"
#include "../Units/CavalryUnit.h"
#include "../Orders/StrategyOrderComponent.h"
#include "../Movement/StrategyMovementExecutorComponent.h"
#include "../Formations/StrategyFormationComponent.h"
#include "../Formations/StrategyFormationTransitionComponent.h"
#include "../Formations/StrategyFormationPolicyComponent.h"
#include "../Combat/StrategyCombatComponent.h"
#include "../Combat/StrategyContactComponent.h"
#include "../Combat/StrategyFireControlComponent.h"
#include "../Combat/StrategyFireDisciplineComponent.h"
#include "../Combat/StrategyFieldworksComponent.h"
#include "../Combat/StrategySkirmisherComponent.h"
#include "../Combat/StrategyStanceComponent.h"
#include "../Combat/StrategyThreatReactionComponent.h"
#include "../AI/StrategyFieldOfficerComponent.h"
#include "../AI/StrategyAutonomousBattleAIComponent.h"
#include "../Visual/StrategyEquipmentVisualComponent.h"
#include "../Navigation/StrategyRiverBarrier.h"
#include "../Engineering/StrategyDefensivePosition.h"
#include "../Artillery/StrategyArtilleryBatteryUnit.h"
#include "../Artillery/StrategyArtilleryDeploymentComponent.h"
#include "../Units/StrategyDragoonComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"

namespace Strategy1864Acceptance
{
    // Test input uses the real order/formation/combat entry points. No expected
    // output is injected into a component and no component tick is called by hand.
    void Order(AStrategyUnit* TestUnit, EStrategyOrderType Type, const FVector& Target)
    {
        FStrategyOrder TestOrder;
        TestOrder.Type = Type;
        TestOrder.TargetLocation = Target;
        TestOrder.Authority = EStrategyOrderAuthority::DirectPlayer;
        TestUnit->OrderComponent->SetOrder(TestOrder);
    }

    void Configure(AStrategyUnit* TestUnit)
    {
        TestUnit->bOfficerAIEnabled = false;
        TestUnit->bPlayerControllable = true;
        TestUnit->AutonomousBattleAIComponent->bEnableForNonPlayerSides = false;
        TestUnit->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::HoldFire);
        TestUnit->CombatComponent->SetDeterministicRandomSeed(1864);
        Order(TestUnit, EStrategyOrderType::Hold, TestUnit->GetActorLocation());
    }
}

bool AStrategyOOBTestScenario::BeginBehaviourAcceptance()
{
    FString RequestedAcceptance;
    const bool bAcceptanceAll = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864AcceptAll"));
    if (!bAcceptanceAll && !FParse::Value(FCommandLine::Get(), TEXT("Strategy1864Accept="), RequestedAcceptance)) return false;
    bBehaviourAcceptance = true;
    const TArray<FString> AvailableAcceptanceIds = {
        TEXT("T1"), TEXT("T2"), TEXT("T3"), TEXT("T4"), TEXT("T5"), TEXT("T6"), TEXT("T7"), TEXT("T8"), TEXT("T9"), TEXT("T10"),
        TEXT("MOVE"), TEXT("STOPFIRE"), TEXT("NEW-HOLD"), TEXT("DELAY"), TEXT("SPREAD"), TEXT("SCREEN"),
        TEXT("WORKS"), TEXT("WORKS-CANCEL"), TEXT("CONTACT"), TEXT("ROE"), TEXT("AMMO"), TEXT("ROUT"), TEXT("DESTINATION"), TEXT("SQUARE-ZONES"),
        TEXT("LONG-MOVE"), TEXT("SQUARE-GATES"), TEXT("FIREBLOCK"), TEXT("DISENGAGE"), TEXT("WORKS-MOVE"), TEXT("SUSPEND-WAYPOINT"), TEXT("CLEAR-SUSPEND"), TEXT("RELOAD"),
        TEXT("SCREEN-SQUARE"), TEXT("SPREAD-MOVE"), TEXT("ARTILLERY-DEPLOY"), TEXT("DRAGOON"), TEXT("CAVALRY-STILL"), TEXT("ADVANCE-FIRE"), TEXT("ATTACK-MOVING"), TEXT("ATTACK-APPROACH"), TEXT("LOSS-RANKS")};
    if (bAcceptanceAll) AcceptanceIds = AvailableAcceptanceIds;
    else AcceptanceIds.Add(RequestedAcceptance.ToUpper());
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864AcceptTimeout="), AcceptanceTimeout);
    AcceptanceTimeout = FMath::Clamp(AcceptanceTimeout, 60.0f, 120.0f);
    FParse::Value(FCommandLine::Get(), TEXT("Strategy1864AcceptSpeed="), AcceptanceDilation);
    AcceptanceDilation = FMath::Clamp(AcceptanceDilation, 0.5f, 10.0f);
    UGameplayStatics::SetGlobalTimeDilation(this, AcceptanceDilation);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT START cases=%d timeout=%.1f speed=%.1f seed=1864"),
        AcceptanceIds.Num(), AcceptanceTimeout, AcceptanceDilation);
    StartBehaviourAcceptanceCase();
    return true;
}

void AStrategyOOBTestScenario::StartBehaviourAcceptanceCase()
{
    ClearSpawnedUnits();
    AcceptanceCompany = nullptr;
    AcceptanceOther = nullptr;
    AcceptanceFar = nullptr;
    AcceptanceCavalry = nullptr;
    AcceptanceBattery = nullptr;
    AcceptanceElapsed = 0.0f;
    AcceptancePhase = 0;
    AcceptanceEventAt = 0.0f;
    AcceptanceStart = Origin;
    AcceptanceCompany = SpawnCompany(TEXT("ACCEPT-C1"), TEXT("Accept kompagni"), 1, AcceptanceStart, nullptr, static_cast<uint8>(EStrategySide::Denmark));
    AcceptanceOther = SpawnCompany(TEXT("ACCEPT-C2"), TEXT("Accept modpart"), 2, AcceptanceStart + FVector(6000, 0, 0), nullptr, static_cast<uint8>(EStrategySide::Enemy));
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT %s SETUP"), *AcceptanceIds[AcceptanceIndex]);
    if (!AcceptanceCompany || !AcceptanceOther) return; // Tick produces a fixture failure.
    Strategy1864Acceptance::Configure(AcceptanceCompany);
    Strategy1864Acceptance::Configure(AcceptanceOther);
    AcceptanceOther->SetActorRotation(FRotator(0, 180, 0));
    AcceptanceStart = AcceptanceCompany->GetActorLocation();
    AcceptanceSnapshot = AcceptanceOther->GetActorLocation();
    AcceptanceAmmo = AcceptanceCompany->CombatComponent->AmmunitionRounds;
    AcceptanceSerial = AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial;
}

void AStrategyOOBTestScenario::TickBehaviourAcceptance(float DeltaSeconds)
{
    if (bAcceptanceFinished) return;
    AcceptanceElapsed += DeltaSeconds; // World delta includes dilation; never BattleClock/ClockRate.
    const FString AcceptanceId = AcceptanceIds[AcceptanceIndex];
    if (!IsValid(AcceptanceCompany) || !IsValid(AcceptanceOther))
    {
        FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: kompagni kunne ikke oprettes"));
        return;
    }
    // Only movement contracts get a fourfold budget for the 600 -> 150 cm/s march.
    const bool bAcceptanceMarchBudget = AcceptanceId == TEXT("MOVE") || AcceptanceId == TEXT("LONG-MOVE") ||
        AcceptanceId == TEXT("STOPFIRE") || AcceptanceId == TEXT("SUSPEND-WAYPOINT") ||
        AcceptanceId == TEXT("SPREAD-MOVE") || AcceptanceId == TEXT("ADVANCE-FIRE") ||
        AcceptanceId == TEXT("ATTACK-MOVING") || AcceptanceId == TEXT("ATTACK-APPROACH") ||
        AcceptanceId == TEXT("T7") || AcceptanceId == TEXT("DISENGAGE");
    const float AcceptanceCaseTimeout = AcceptanceTimeout * (bAcceptanceMarchBudget ? 4.f : 1.f);
    if (AcceptanceElapsed >= AcceptanceCaseTimeout)
    {
        FinishBehaviourAcceptanceCase(false, FString::Printf(TEXT("TIMEOUT %s %.1fs: krav om maalopdatering/ankomst/ild ikke opfyldt; phase=%d order=%d state=%d formation=%d suspended=%d"),
            *AcceptanceId, AcceptanceElapsed, AcceptancePhase, int32(AcceptanceCompany->OrderComponent->GetCurrentOrder().Type),
            int32(AcceptanceCompany->UnitState), int32(AcceptanceCompany->FormationComponent->CurrentFormation),
            AcceptanceCompany->MovementExecutor->GetSuspendedMission().OrderSerial));
        return;
    }
    // Full design contracts that do not yet exist must fail explicitly. A smaller
    // existing feature must never be reported as a passing T2/T4/T5/T6/T8/T9/T10.
    FString AcceptanceGap;
    if (AcceptanceId == TEXT("T2")) AcceptanceGap = TEXT("FrontAllocator/CombatSlot/lane overlap-kontrakt mangler; ingen fuld T2-fixture");
    else if (AcceptanceId == TEXT("T4")) AcceptanceGap = TEXT("ordonnansdoed taber endnu ikke ordren; fuld doed/officer-fixture mangler (DELAY er separat)");
    else if (AcceptanceId == TEXT("T5")) AcceptanceGap = TEXT("ProtectionState/GUARD_BATTERY/COVER_BATTERY mangler");
    else if (AcceptanceId == TEXT("T6")) AcceptanceGap = TEXT("skaermgate/ENGAGE_BATTERY/counter-battery-request mangler");
    else if (AcceptanceId == TEXT("T8")) AcceptanceGap = TEXT("FEraProfile/FEquipmentProfile og battle-era swap mangler");
    else if (AcceptanceId == TEXT("T9")) AcceptanceGap = TEXT("node-lokal ControlMode/baseline/overtagelsesforsinkelse mangler");
    else if (AcceptanceId == TEXT("T10")) AcceptanceGap = TEXT("Conditions/ThenOrder/lokal evaluator mangler");
    if (!AcceptanceGap.IsEmpty()) { FinishBehaviourAcceptanceCase(false, TEXT("NOT_IMPLEMENTED: ") + AcceptanceGap); return; }

    auto AcceptanceCheck = [this](bool bCondition, const TCHAR* Explanation)
    { FinishBehaviourAcceptanceCase(bCondition, Explanation); };
    if (AcceptancePhase == 0 && AcceptanceElapsed >= 1.0f)
    {
        AcceptancePhase = 1;
        AcceptanceEventAt = AcceptanceElapsed;
        if (AcceptanceId == TEXT("MOVE") || AcceptanceId == TEXT("STOPFIRE") || AcceptanceId == TEXT("NEW-HOLD") || AcceptanceId == TEXT("T1"))
        {
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(200000, 0, 0));
            // A stop-and-fire reaction is the officer's: with the AI off the company marches on under fire (player's choice).
            if (AcceptanceId == TEXT("STOPFIRE") || AcceptanceId == TEXT("NEW-HOLD")) AcceptanceCompany->bOfficerAIEnabled = true;
            Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Move, AcceptanceStart + FVector(10000, 0, 0));
            AcceptanceSerial = AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial;
            if (AcceptanceId == TEXT("T1"))
            {
                AcceptanceCompany->bOfficerAIEnabled = true;
                AcceptanceCavalry = SpawnCavalry(TEXT("ACCEPT-CAV"), TEXT("Accept rytteri"), AcceptanceStart + FVector(20000, 0, 0), nullptr);
                if (!AcceptanceCavalry) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: rytteri mangler")); return; }
                Strategy1864Acceptance::Configure(AcceptanceCavalry);
                AcceptanceCavalry->Side = EStrategySide::Enemy;
                AcceptanceCavalry->SetActorRotation(FRotator(0, 180, 0));
                AcceptanceCavalry->MovementExecutor->MoveSpeedCmPerSecond = 1200.0f;
                Strategy1864Acceptance::Order(AcceptanceCavalry, EStrategyOrderType::Move, AcceptanceStart);
            }
        }
        else if (AcceptanceId == TEXT("DELAY"))
        {
            FStrategyOrder AcceptanceDelayed;
            AcceptanceDelayed.Type = EStrategyOrderType::Move;
            AcceptanceDelayed.TargetLocation = AcceptanceStart + FVector(5000, 0, 0);
            AcceptanceDelayed.Authority = EStrategyOrderAuthority::DirectPlayer;
            AcceptanceCompany->OrderComponent->QueueDelayedOrder(AcceptanceDelayed, 5.0f);
        }
        else if (AcceptanceId == TEXT("SPREAD"))
        {
            if (!AcceptanceCompany->FieldOfficerComponent->SetManualSpread(true)) { FinishBehaviourAcceptanceCase(false, TEXT("SPRED afvist")); return; }
        }
        else if (AcceptanceId == TEXT("SCREEN"))
        {
            if (!AcceptanceCompany->SkirmisherComponent->DeploySkirmishers(EStrategySkirmisherRole::Screen)) { FinishBehaviourAcceptanceCase(false, TEXT("skaerm afvist")); return; }
        }
        else if (AcceptanceId == TEXT("WORKS") || AcceptanceId == TEXT("WORKS-CANCEL") || AcceptanceId == TEXT("WORKS-MOVE"))
        {
            if (!AcceptanceCompany->FieldworksComponent->BeginHastyFieldworks()) { FinishBehaviourAcceptanceCase(false, TEXT("BYG afvist")); return; }
        }
        else if (AcceptanceId == TEXT("CONTACT"))
        {
            FStrategyContactRecord AcceptanceContact;
            if (!AcceptanceCompany->ContactComponent->GetLastKnownContact(AcceptanceOther->StableUnitId, AcceptanceContact) ||
                !AcceptanceContact.bCurrentlyVisible || AcceptanceContact.Confidence != 1.0f || AcceptanceContact.Source != EStrategyContactSource::OwnEyes)
            { FinishBehaviourAcceptanceCase(false, TEXT("lokal frisk OwnEyes-kontakt mangler")); return; }
            AcceptanceSnapshot = AcceptanceContact.LastKnownPosition;
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(200000, 0, 0));
        }
        else if (AcceptanceId == TEXT("T3"))
        {
            AcceptanceOther->Side = EStrategySide::Denmark;
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(6000, 0, 0));
            AcceptanceFar = SpawnCompany(TEXT("ACCEPT-FAR"), TEXT("Fjern nabo"), 3, AcceptanceStart + FVector(20000, 0, 0), nullptr, static_cast<uint8>(EStrategySide::Denmark));
            if (!AcceptanceFar) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: fjern nabo mangler")); return; }
            Strategy1864Acceptance::Configure(AcceptanceFar);
            AcceptanceSnapshot = FVector(AcceptanceCompany->Morale, AcceptanceFar->Morale, 0);
            AcceptanceOther->Morale = 0.0f;
            AcceptanceOther->CombatComponent->EvaluateRoutState();
        }
        else if (AcceptanceId == TEXT("ROUT"))
        {
            AcceptanceCompany->Morale = 0.0f;
            AcceptanceCompany->CombatComponent->EvaluateRoutState();
            AcceptanceCheck(AcceptanceCompany->UnitState == EStrategyUnitState::Routed && !AcceptanceCompany->IsCombatEffective(), TEXT("lav moral -> Routed og ikke kampdygtig")); return;
        }
        else if (AcceptanceId == TEXT("AMMO"))
        {
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
            if (!AcceptanceCompany->FireControlComponent->CanEngageTarget(AcceptanceOther) || !AcceptanceCompany->FireControlComponent->IsBattleFormationReady())
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: gyldigt skudmaal mangler; ammo-check ville vaere tom")); return; }
            AcceptanceCompany->CombatComponent->AmmunitionRounds = 0;
            AcceptanceCheck(!AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther), TEXT("ingen ammunition -> salve afvist")); return;
        }
        else if (AcceptanceId == TEXT("DESTINATION"))
        {
            FStrategyOrder AcceptanceAttack;
            AcceptanceAttack.Type = EStrategyOrderType::AttackHere;
            AcceptanceAttack.TargetLocation = AcceptanceOther->GetActorLocation();
            AcceptanceAttack.AttackTarget = AcceptanceOther;
            AcceptanceCompany->OrderComponent->ResolveAttackDestination(AcceptanceAttack);
            const float AcceptanceStandOff = FVector::Dist2D(AcceptanceAttack.TargetLocation, AcceptanceOther->GetActorLocation());
            const float AcceptanceApproach = AcceptanceCompany->FireControlComponent->GetApproachStandRangeCm();
            // Clearly inside the range (at least 5 m, about 88 %), never in bayonet reach, and the halt range lies between stand-off and range.
            AcceptanceCheck(AcceptanceStandOff <= AcceptanceApproach + 10.0f && AcceptanceStandOff >= AcceptanceApproach * 0.4f &&
                AcceptanceApproach <= AcceptanceCompany->FireControlComponent->GetActiveRangeCm() - 500.0f + 1.0f &&
                AcceptanceCompany->FireControlComponent->GetApproachHaltRangeCm() >= AcceptanceApproach &&
                AcceptanceCompany->FireControlComponent->GetApproachHaltRangeCm() < AcceptanceCompany->FireControlComponent->GetActiveRangeCm() &&
                AcceptanceAttack.bHasFacing,
                TEXT("ordrens angrebsmaal tydeligt inden for aktiv raekkevidde (ca. 88 %, mindst 5 m); halt-afstand mellem maal og raekkevidde")); return;
        }
        else if (AcceptanceId == TEXT("SQUARE-ZONES"))
        {
            AcceptanceCompany->FormationComponent->SetFormation(EStrategyFormationType::Square);
        }
        else if (AcceptanceId == TEXT("LONG-MOVE"))
        {
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(200000, 0, 0));
            Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Move, AcceptanceStart + FVector(25000, 0, 0));
            if (AcceptanceCompany->FormationComponent->CurrentFormation != EStrategyFormationType::MarchColumn ||
                AcceptanceCompany->FormationPolicy->GetDestinationFormation() != EStrategyFormationType::Line)
            { FinishBehaviourAcceptanceCase(false, TEXT("lang march giver ikke kolonne med linje ved destination")); return; }
        }
        else if (AcceptanceId == TEXT("SQUARE-GATES"))
        {
            AcceptanceCompany->FormationComponent->SetFormation(EStrategyFormationType::Square);
            AcceptanceCheck(!AcceptanceCompany->FieldOfficerComponent->SetManualSpread(true) &&
                !AcceptanceCompany->SkirmisherComponent->DeploySkirmishers(EStrategySkirmisherRole::Screen), TEXT("carre afviser SPRED og ny skyttekæde")); return;
        }
        else if (AcceptanceId == TEXT("FIREBLOCK"))
        {
            if (!AcceptanceCompany->FireControlComponent->CanEngageTarget(AcceptanceOther) || !AcceptanceCompany->FireControlComponent->IsBattleFormationReady())
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: gyldigt skudmaal mangler foer venlig blokker")); return; }
            AcceptanceFar = SpawnCompany(TEXT("ACCEPT-BLOCKER"), TEXT("Ven foran"), 3, AcceptanceStart + FVector(3000, 0, 0), nullptr, static_cast<uint8>(EStrategySide::Denmark));
            if (!AcceptanceFar) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: venlig blokker mangler")); return; }
            Strategy1864Acceptance::Configure(AcceptanceFar);
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
            AcceptanceCheck(!AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther) && AcceptanceCompany->CombatComponent->AmmunitionRounds == AcceptanceAmmo,
                TEXT("ven i skudlinjen skal blokere salve/ammo-forbrug; FireBlocked-kontrakt mangler aktuelt")); return;
        }
        else if (AcceptanceId == TEXT("DISENGAGE"))
        {
            // Fire opens when the far corner of the enemy's front is in range, so the enemy is placed close enough for all of it.
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(4000, 0, 0));
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
            Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Disengage, AcceptanceStart);
        }
        else if (AcceptanceId == TEXT("T7"))
        {
            AcceptanceOther->Side = EStrategySide::Denmark;
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(0, 6000, 0));
            SpawnedRiverBarrier = GetWorld()->SpawnActor<AStrategyRiverBarrier>(AStrategyRiverBarrier::StaticClass(), AcceptanceStart + FVector(15000, 0, 0), FRotator::ZeroRotator);
            if (!SpawnedRiverBarrier) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: bro mangler")); return; }
            // The across-river normal is (-axis.y, axis.x): axis (0,-1,0) gives +X, so the west bank is side -1 (bank A, offset -X)
            // and the east bank side +1 (bank B, offset +X), matching the approach offsets below.
            SpawnedRiverBarrier->RiverAxisDirection = FVector(0, -1, 0);
            SpawnedRiverBarrier->BankAApproachOffset = FVector(-1800, 0, 0);
            SpawnedRiverBarrier->BankBApproachOffset = FVector(1800, 0, 0);
            Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Move, AcceptanceStart + FVector(25000, 0, 0));
            Strategy1864Acceptance::Order(AcceptanceOther, EStrategyOrderType::Move, AcceptanceStart + FVector(25000, 6000, 0));
        }
        else if (AcceptanceId == TEXT("ATTACK-MOVING") || AcceptanceId == TEXT("ATTACK-APPROACH"))
        {
            const bool bAcceptanceMoving = AcceptanceId == TEXT("ATTACK-MOVING");
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(25000, bAcceptanceMoving ? -6000 : 0, 0));
            AcceptanceOther->SetActorRotation(FRotator(0, bAcceptanceMoving ? 90 : 180, 0));
            AcceptanceSnapshot = AcceptanceOther->GetActorLocation();
            AcceptanceOther->MovementExecutor->MoveSpeedCmPerSecond = 150.0f;
            AcceptanceCompany->MovementExecutor->MoveSpeedCmPerSecond = 150.0f;
            Strategy1864Acceptance::Order(AcceptanceOther, EStrategyOrderType::Move,
                AcceptanceStart + (bAcceptanceMoving ? FVector(25000, 6000, 0) : FVector(-10000, 0, 0)));
            AcceptanceCompany->bOfficerAIEnabled = true;
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
            FStrategyOrder AcceptanceTrackingOrder;
            AcceptanceTrackingOrder.Type = EStrategyOrderType::AttackHere;
            AcceptanceTrackingOrder.Authority = EStrategyOrderAuthority::DirectPlayer;
            AcceptanceTrackingOrder.AttackTarget = AcceptanceOther;
            AcceptanceTrackingOrder.TargetLocation = AcceptanceOther->GetActorLocation();
            if (!AcceptanceCompany->OrderComponent->SetOrder(AcceptanceTrackingOrder))
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: AttackHere afvist")); return; }
            AcceptanceSerial = AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial;
        }
        else if (AcceptanceId == TEXT("ADVANCE-FIRE"))
        {
            AcceptanceCompany->bOfficerAIEnabled = true;
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
            Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Advance, AcceptanceStart + FVector(15000, 0, 0));
            AcceptanceSerial = AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial;
        }
        else if (AcceptanceId == TEXT("SUSPEND-WAYPOINT") || AcceptanceId == TEXT("CLEAR-SUSPEND"))
        {
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(200000, 0, 0));
            AcceptanceCompany->bOfficerAIEnabled = true;
            FStrategyOrder AcceptanceRoute;
            AcceptanceRoute.Type = EStrategyOrderType::Move;
            AcceptanceRoute.Authority = EStrategyOrderAuthority::DirectPlayer;
            AcceptanceRoute.TargetLocation = AcceptanceStart + FVector(10000, 0, 0);
            AcceptanceRoute.Waypoints.Add(AcceptanceStart + FVector(5000, 0, 0));
            AcceptanceRoute.Waypoints.Add(AcceptanceRoute.TargetLocation);
            if (!AcceptanceCompany->OrderComponent->SetOrder(AcceptanceRoute))
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: ruteordre afvist")); return; }
            AcceptanceSerial = AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial;
        }
        else if (AcceptanceId == TEXT("RELOAD"))
        {
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
            if (!AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther))
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: foerste virkelige salve afvist")); return; }
            AcceptanceAmmo = AcceptanceCompany->CombatComponent->AmmunitionRounds;
            if (!AcceptanceCompany->CombatComponent->IsReloading() || AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther) ||
                AcceptanceCompany->CombatComponent->AmmunitionRounds != AcceptanceAmmo)
            { FinishBehaviourAcceptanceCase(false, TEXT("genladning afviste ikke anden salve/ammo-forbrug")); return; }
            // Disable subsequent automatic salvos; the real reload timer still ticks.
            AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::HoldFire);
        }
        else if (AcceptanceId == TEXT("SCREEN-SQUARE"))
        {
            if (!AcceptanceCompany->SkirmisherComponent->DeploySkirmishers(EStrategySkirmisherRole::Screen))
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: skaerm afvist")); return; }
        }
        else if (AcceptanceId == TEXT("SPREAD-MOVE"))
        {
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(200000, 0, 0));
            if (!AcceptanceCompany->FieldOfficerComponent->SetManualSpread(true))
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: SPRED afvist")); return; }
            AcceptanceSnapshot = AcceptanceCompany->GetActorLocation();
        }
        else if (AcceptanceId == TEXT("ARTILLERY-DEPLOY"))
        {
            AcceptanceBattery = GetWorld()->SpawnActor<AStrategyArtilleryBatteryUnit>(AStrategyArtilleryBatteryUnit::StaticClass(),
                AcceptanceStart + FVector(0, 10000, 0), FRotator::ZeroRotator);
            if (!AcceptanceBattery) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: batteri mangler")); return; }
            // The QA helper forces Deployed; spawn the real default Limbered actor instead.
            AcceptanceBattery->StableUnitId = TEXT("ACCEPT-GUNS");
            AcceptanceBattery->Side = EStrategySide::Denmark;
            SpawnedUnitObjects.Add(AcceptanceBattery);
            Strategy1864Acceptance::Configure(AcceptanceBattery);
            if (!AcceptanceBattery->DeploymentComponent->IsLimbered() || !AcceptanceBattery->DeploymentComponent->RequestDeploy())
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: batteri kan ikke afprotses")); return; }
        }
        else if (AcceptanceId == TEXT("DRAGOON") || AcceptanceId == TEXT("CAVALRY-STILL"))
        {
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(200000, 0, 0));
            AcceptanceCavalry = SpawnCavalry(TEXT("ACCEPT-CAV"), TEXT("Accept rytteri"), AcceptanceStart + FVector(20000, 0, 0), nullptr);
            if (!AcceptanceCavalry) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: rytteri mangler")); return; }
            Strategy1864Acceptance::Configure(AcceptanceCavalry);
            AcceptanceCavalry->Side = EStrategySide::Enemy;
            if (AcceptanceId == TEXT("DRAGOON"))
            {
                AcceptanceCavalry->DragoonComponent->Role = EStrategyCavalryRole::Dragoon;
                if (!AcceptanceCavalry->DragoonComponent->DismountAtCurrentPosition())
                { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: dragon kan ikke stige af")); return; }
            }
            else AcceptanceCompany->bOfficerAIEnabled = true;
        }
        else if (AcceptanceId == TEXT("LOSS-RANKS")) { /* no setup: the check below runs on the fixture company without ticking */ }
        else if (AcceptanceId != TEXT("ROE")) { FinishBehaviourAcceptanceCase(false, TEXT("UNKNOWN_ID: se Docs/TestFlags.md")); return; }
    }
    if (AcceptancePhase == 0) return;
    const float AcceptanceSinceEvent = AcceptanceElapsed - AcceptanceEventAt;
    if (AcceptanceId == TEXT("ATTACK-MOVING") || AcceptanceId == TEXT("ATTACK-APPROACH"))
    {
        const FStrategyOrder AcceptanceTrackingOrder = AcceptanceCompany->OrderComponent->GetCurrentOrder();
        if (AcceptanceTrackingOrder.OrderSerial != AcceptanceSerial || AcceptanceTrackingOrder.Type != EStrategyOrderType::AttackHere)
        { FinishBehaviourAcceptanceCase(false, TEXT("retargeting erstattede angrebsordren/serial")); return; }
        const FVector AcceptanceAttackerPosition = AcceptanceCompany->GetActorLocation();
        const FVector AcceptanceEnemyPosition = AcceptanceOther->GetActorLocation();
        const float AcceptanceEnemyDistance = FVector::Dist2D(AcceptanceAttackerPosition, AcceptanceEnemyPosition);
        const float AcceptanceEnemyYaw = (AcceptanceEnemyPosition - AcceptanceAttackerPosition).Rotation().Yaw;
        const bool bAcceptanceFacing = FMath::Abs(FMath::FindDeltaAngleDegrees(AcceptanceCompany->GetActorRotation().Yaw, AcceptanceEnemyYaw)) <= 30.0f;
        if (AcceptanceId == TEXT("ATTACK-APPROACH"))
        {
            // Check every tick, not just at the successful volley.
            if (AcceptanceAttackerPosition.X >= AcceptanceEnemyPosition.X)
            { FinishBehaviourAcceptanceCase(false, TEXT("ATTACK-APPROACH: angriberen passerede fjendens position")); return; }
            if (AcceptanceCompany->CombatComponent->AmmunitionRounds < AcceptanceAmmo)
            {
                AcceptanceCheck(AcceptanceEnemyDistance > 2000.0f && bAcceptanceFacing &&
                    AcceptanceCompany->MovementExecutor->IsHoldingForFire() &&
                    AcceptanceCompany->MovementExecutor->GetExecutedVelocity().SizeSquared2D() < 1.0f,
                    TEXT("ATTACK-APPROACH: standset og skudt foer 20 m, aldrig passeret fjenden, front inden for 30 grader"));
                return;
            }
        }
        else if (FVector::Dist2D(AcceptanceSnapshot, AcceptanceEnemyPosition) >= 4000.0f)
        {
            FVector AcceptanceCurrentStand;
            float AcceptanceCurrentFacing = 0.0f;
            if (!UStrategyOrderComponent::ComputeAttackStand(AcceptanceOther, AcceptanceCompany,
                AcceptanceCurrentStand, AcceptanceCurrentFacing, AcceptanceAttackerPosition))
            { FinishBehaviourAcceptanceCase(false, TEXT("ATTACK-MOVING: levende maal/standpunkt mangler")); return; }
            if (FVector::Dist2D(AcceptanceTrackingOrder.TargetLocation, AcceptanceCurrentStand) <= 1500.0f &&
                AcceptanceCompany->FireControlComponent->CanEngageTarget(AcceptanceOther) && bAcceptanceFacing &&
                AcceptanceCompany->MovementExecutor->IsHoldingForFire())
            {
                AcceptanceCheck(true, TEXT("ATTACK-MOVING: fjenden flyttet mindst 40 m; aktuelt standpunkt inden for 15 m; standset i ildafstand og front inden for 30 grader"));
                return;
            }
        }
        return; // The shared timeout gives a named FAIL if the contract is never reached.
    }
    if (AcceptanceId == TEXT("ADVANCE-FIRE"))
    {
        if (AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial != AcceptanceSerial)
        { FinishBehaviourAcceptanceCase(false, TEXT("stop-og-ild erstattede autoritativ fremrykningsmission")); return; }
        if (AcceptancePhase == 1 && AcceptanceSinceEvent > 5.0f)
        {
            if (!AcceptanceCompany->MovementExecutor->IsHoldingForFire() || AcceptanceCompany->MovementExecutor->HasMovementGoal() ||
                AcceptanceCompany->MovementExecutor->GetSuspendedMission().OrderSerial != AcceptanceSerial ||
                AcceptanceCompany->CombatComponent->AmmunitionRounds >= AcceptanceAmmo)
            { FinishBehaviourAcceptanceCase(false, TEXT("synlig ildkontakt gav ikke suspension og virkelig salve")); return; }
            AcceptanceOther->SetActorLocation(AcceptanceStart + FVector(26000, 0, 0));
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
            return;
        }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent < 9.9f && AcceptanceCompany->MovementExecutor->HasMovementGoal())
        { FinishBehaviourAcceptanceCase(false, TEXT("fremrykning genoptaget foer 10s uden ildkontakt")); return; }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent >= 10.0f &&
            AcceptanceCompany->MovementExecutor->IsHoldingForFire() &&
            FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceCompany->OrderComponent->GetCurrentOrder().TargetLocation) <= 1500.0f)
        {
            FVector AcceptanceAdvanceStand;
            float AcceptanceAdvanceFacing = 0.0f;
            const bool bAcceptanceAdvanceStand = UStrategyOrderComponent::ComputeAttackStand(AcceptanceOther, AcceptanceCompany,
                AcceptanceAdvanceStand, AcceptanceAdvanceFacing, AcceptanceCompany->GetActorLocation());
            AcceptanceCheck(bAcceptanceAdvanceStand &&
                FVector::Dist2D(AcceptanceCompany->OrderComponent->GetCurrentOrder().TargetLocation, AcceptanceAdvanceStand) <= 1500.0f &&
                AcceptanceCompany->MovementExecutor->IsHoldingForFire() &&
                AcceptanceCompany->FireControlComponent->CanEngageTarget(AcceptanceOther),
                TEXT("ADVANCE: salve, 10s hysterese, samme serial og genoptaget fremrykning til opdateret ildafstand")); return;
        }
    }

    if ((AcceptanceId == TEXT("SUSPEND-WAYPOINT") || AcceptanceId == TEXT("CLEAR-SUSPEND")) && AcceptancePhase == 1 && AcceptanceSinceEvent > 2.0f)
    {
        AcceptanceCompany->CombatComponent->NotifyIncomingVolley(0);
        const FStrategyOrder AcceptanceSavedRoute = AcceptanceCompany->MovementExecutor->GetSuspendedMission();
        if (AcceptanceSavedRoute.OrderSerial != AcceptanceSerial || AcceptanceSavedRoute.Authority != EStrategyOrderAuthority::DirectPlayer ||
            AcceptanceSavedRoute.Waypoints.Num() != 2 || !AcceptanceSavedRoute.Waypoints[0].Equals(AcceptanceStart + FVector(5000, 0, 0), 1.0f) ||
            AcceptanceCompany->MovementExecutor->HasMovementGoal())
        { FinishBehaviourAcceptanceCase(false, TEXT("snapshot mistede serial/autoritet/waypoint eller march standsede ikke")); return; }
        AcceptancePhase = 2;
        AcceptanceEventAt = AcceptanceElapsed;
        if (AcceptanceId == TEXT("CLEAR-SUSPEND")) AcceptanceCompany->OrderComponent->ClearOrder();
        return;
    }
    if ((AcceptanceId == TEXT("SUSPEND-WAYPOINT") || AcceptanceId == TEXT("CLEAR-SUSPEND")) && AcceptancePhase == 2)
    {
        if (AcceptanceSinceEvent < 9.9f && AcceptanceCompany->MovementExecutor->HasMovementGoal())
        { FinishBehaviourAcceptanceCase(false, TEXT("rute genoptaget foer 10s ro")); return; }
        // Keep the 10s quiet period; scale the remaining travel allowance by four.
        if (AcceptanceSinceEvent > (AcceptanceId == TEXT("SUSPEND-WAYPOINT") ? 110.0f : 35.0f))
        {
            const FStrategyOrder AcceptanceFinalOrder = AcceptanceCompany->OrderComponent->GetCurrentOrder();
            const bool bAcceptanceSnapshotCleared = !AcceptanceCompany->MovementExecutor->GetSuspendedMission().IsValidOrder();
            if (AcceptanceId == TEXT("SUSPEND-WAYPOINT"))
                AcceptanceCheck(bAcceptanceSnapshotCleared && AcceptanceFinalOrder.OrderSerial == AcceptanceSerial &&
                    AcceptanceFinalOrder.Authority == EStrategyOrderAuthority::DirectPlayer && AcceptanceFinalOrder.Waypoints.Num() == 2 &&
                    AcceptanceFinalOrder.NextWaypointIndex == 2 &&
                    FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceStart + FVector(10000, 0, 0)) < 100.0f,
                    TEXT("samme mission/waypoint/autoritet genoptaget og ankommet"));
            else AcceptanceCheck(bAcceptanceSnapshotCleared && !AcceptanceCompany->MovementExecutor->HasMovementGoal() && !AcceptanceFinalOrder.IsValidOrder(),
                TEXT("clear invaliderer gammel suspension og genstarter ikke march"));
            return;
        }
    }
    if (AcceptanceId == TEXT("RELOAD") && AcceptanceSinceEvent > 30.0f)
    {
        const bool bAcceptanceAmmoPreserved = AcceptanceCompany->CombatComponent->AmmunitionRounds == AcceptanceAmmo;
        AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
        AcceptanceCheck(bAcceptanceAmmoPreserved && !AcceptanceCompany->CombatComponent->IsReloading() &&
            AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther) && AcceptanceCompany->CombatComponent->AmmunitionRounds < AcceptanceAmmo,
            TEXT("foerste salve -> afvist anden salve -> virkelig genladning -> ny salve")); return;
    }
    if (AcceptanceId == TEXT("SCREEN-SQUARE") && AcceptanceSinceEvent > 8.0f)
    {
        if (AcceptancePhase == 1)
        {
            if (!AcceptanceCompany->SkirmisherComponent->IsDeployed())
            { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: skaerm ikke deployeret foer carre")); return; }
            AcceptanceCompany->FormationComponent->SetFormation(EStrategyFormationType::Square);
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
            return;
        }
        AcceptanceCheck(AcceptanceCompany->SkirmisherComponent->State == EStrategySkirmisherState::Attached &&
            AcceptanceCompany->SkirmisherComponent->DetachedStrength == 0 && AcceptanceCompany->CurrentStrength == 190 &&
            AcceptanceCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square,
            TEXT("carre hjemkalder deployeret skaerm; maend bevares")); return;
    }
    if (AcceptanceId == TEXT("SPREAD-MOVE") && AcceptancePhase == 1 && AcceptanceSinceEvent > 4.0f)
    {
        if (!AcceptanceCompany->FieldOfficerComponent->IsTakingFireCover() || AcceptanceCompany->StanceComponent->Stance != EStrategyStance::Prone)
        { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: SPRED ikke aktiv foer FLYT")); return; }
        Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Move, AcceptanceStart + FVector(5000, 0, 0));
        AcceptancePhase = 2;
        AcceptanceEventAt = AcceptanceElapsed;
        return;
    }
    if (AcceptanceId == TEXT("SPREAD-MOVE") && AcceptancePhase == 2)
    {
        if (AcceptanceSinceEvent < 2.8f && FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceSnapshot) > 1.0f)
        { FinishBehaviourAcceptanceCase(false, TEXT("SPRED marcherede foer 3s rejsning")); return; }
        // Keep the 3s rise; scale the remaining travel allowance by four.
        if (AcceptanceSinceEvent > 71.0f)
        { AcceptanceCheck(!AcceptanceCompany->FieldOfficerComponent->IsTakingFireCover() && AcceptanceCompany->StanceComponent->Stance == EStrategyStance::Standing &&
            AcceptanceCompany->FormationComponent->RankCount == 3 &&
            FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceStart + FVector(5000, 0, 0)) < 100.0f,
            TEXT("leveret FLYT: rejsning foer march, raekker gendannet, ankomst")); return; }
    }
    if (AcceptanceId == TEXT("LOSS-RANKS") && AcceptancePhase == 1)
    {
        // Deterministic, no ticking: a line that has lost men keeps its nominal depth (2 and 3 ranks), so the close-up shortens the front
        // by whole files instead of thinning the depth. Strength is restored before anything ticks. The step-up inside a file is
        // StepUpFiles (visual component, private) and is checked on a screenshot.
        UStrategyFormationComponent* RankFormation = AcceptanceCompany->FormationComponent;
        const EStrategyFormationType RankOldFormation = RankFormation->CurrentFormation;
        const int32 RankOldCount = RankFormation->RankCount;
        const int32 RankOldStrength = AcceptanceCompany->CurrentStrength;
        const int32 RankOldInitial = AcceptanceCompany->InitialStrength;
        bool bRankOk = true;
        for (int32 RankTry = 2; RankTry <= 3; ++RankTry)
        {
            RankFormation->CurrentFormation = EStrategyFormationType::Line;
            RankFormation->RankCount = RankTry;
            AcceptanceCompany->InitialStrength = 150;
            AcceptanceCompany->CurrentStrength = 90;
            const int32 RankMen = 90;
            const TArray<FStrategyFormationSlot> RankSlots = RankFormation->GenerateSoldierSlots(FVector::ZeroVector, 0.0f, RankMen);
            TSet<int32> RankRows;
            TSet<int32> RankFiles;
            for (const FStrategyFormationSlot& RankSlot : RankSlots)
            {
                RankRows.Add(FMath::RoundToInt(RankSlot.WorldLocation.X));
                RankFiles.Add(FMath::RoundToInt(RankSlot.WorldLocation.Y));
            }
            bRankOk &= RankRows.Num() == RankTry && RankFiles.Num() == FMath::DivideAndRoundUp(RankMen, RankTry);
        }
        RankFormation->CurrentFormation = RankOldFormation;
        RankFormation->RankCount = RankOldCount;
        AcceptanceCompany->InitialStrength = RankOldInitial;
        AcceptanceCompany->CurrentStrength = RankOldStrength;
        AcceptanceCheck(bRankOk, TEXT("linje med tab beholder 2 og 3 geleddybde; fronten kortes i hele filer"));
        return;
    }
    if (AcceptanceId == TEXT("ARTILLERY-DEPLOY"))
    {
        if (!IsValid(AcceptanceBattery)) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: batteri forsvandt")); return; }
        if (AcceptancePhase == 1 && AcceptanceSinceEvent < 11.5f && AcceptanceBattery->DeploymentComponent->IsDeployed())
        { FinishBehaviourAcceptanceCase(false, TEXT("afprotsning afsluttet foer 12s")); return; }
        if (AcceptancePhase == 1 && AcceptanceSinceEvent > 14.0f)
        {
            if (!AcceptanceBattery->DeploymentComponent->IsDeployed() || !AcceptanceBattery->DeploymentComponent->RequestLimber())
            { FinishBehaviourAcceptanceCase(false, TEXT("batteri ikke deployeret eller beprotsning afvist")); return; }
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
            return;
        }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent < 14.5f && AcceptanceBattery->DeploymentComponent->IsLimbered())
        { FinishBehaviourAcceptanceCase(false, TEXT("beprotsning afsluttet foer 15s")); return; }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent > 17.0f)
        { AcceptanceCheck(AcceptanceBattery->DeploymentComponent->IsLimbered() && AcceptanceBattery->CanNormalMove(),
            TEXT("virkelig 12s afprotsning, 15s beprotsning, mobilitet gendannet")); return; }
    }
    if (AcceptanceId == TEXT("DRAGOON") && AcceptanceSinceEvent > 3.0f)
    {
        if (!IsValid(AcceptanceCavalry)) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: dragon forsvandt")); return; }
        if (AcceptancePhase == 1)
        {
            if (AcceptanceCavalry->DragoonComponent->MountedState != EStrategyMountedState::Dismounted ||
                AcceptanceCavalry->DragoonComponent->DismountedCombatStrength != 60 || AcceptanceCavalry->DragoonComponent->HorseHolderStrength != 20 ||
                AcceptanceCavalry->FormationComponent->CurrentFormation != EStrategyFormationType::Line)
            { FinishBehaviourAcceptanceCase(false, TEXT("afstigning mistede 60/20 styrkefordeling eller linje")); return; }
            if (!AcceptanceCavalry->DragoonComponent->RequestRemount())
            { FinishBehaviourAcceptanceCase(false, TEXT("opstigning afvist ved hestepark")); return; }
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
        }
        else { AcceptanceCheck(AcceptanceCavalry->DragoonComponent->MountedState == EStrategyMountedState::Mounted &&
            AcceptanceCavalry->DragoonComponent->DismountedCombatStrength == 0 && AcceptanceCavalry->DragoonComponent->HorseHolderStrength == 0 &&
            AcceptanceCavalry->CurrentStrength == 80 && AcceptanceCavalry->FormationComponent->CurrentFormation == EStrategyFormationType::CavalryLine,
            TEXT("dragon ved hestepark: af/opstigning bevarer 80 mand og gendanner rytterlinje; ild/HUD separat")); return; }
    }
    if (AcceptanceId == TEXT("CAVALRY-STILL"))
    {
        if (!IsValid(AcceptanceCavalry)) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: rytteri forsvandt")); return; }
        if (AcceptanceCompany->ThreatReactionComponent->IsRespondingToCavalry() || AcceptanceCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Square)
        { FinishBehaviourAcceptanceCase(false, TEXT("stillestaaende rytteri udloeste automatisk carre")); return; }
        if (AcceptanceSinceEvent > 12.0f)
        { AcceptanceCheck(AcceptanceCompany->ContactComponent->HasCurrentContact(AcceptanceCavalry) &&
            AcceptanceCavalry->MovementExecutor->GetExecutedVelocity().IsNearlyZero(),
            TEXT("observeret stillestaaende rytteri 200m udloeser ingen carre i hele observationsvinduet")); return; }
    }

    if (AcceptanceId == TEXT("MOVE") && AcceptanceSinceEvent > 120.0f)
    {
        AcceptanceCheck(FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceStart + FVector(10000, 0, 0)) < 100.0f &&
            AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial == AcceptanceSerial, TEXT("ren FLYT ankommer uden ny ordre")); return;
    }
    if ((AcceptanceId == TEXT("STOPFIRE") || AcceptanceId == TEXT("NEW-HOLD")) && AcceptancePhase == 1 && AcceptanceSinceEvent > 2.0f)
    {
        AcceptanceCompany->CombatComponent->NotifyIncomingVolley(0);
        const bool bAcceptanceSuspended = AcceptanceCompany->MovementExecutor->GetSuspendedMission().OrderSerial == AcceptanceSerial &&
            AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial == AcceptanceSerial && !AcceptanceCompany->MovementExecutor->HasMovementGoal();
        if (!bAcceptanceSuspended) { FinishBehaviourAcceptanceCase(false, TEXT("indgaaende ild bevarede ikke mission/serial eller standsede ikke")); return; }
        AcceptancePhase = 2;
        AcceptanceEventAt = AcceptanceElapsed;
        if (AcceptanceId == TEXT("NEW-HOLD")) Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Hold, AcceptanceCompany->GetActorLocation());
        return;
    }
    if ((AcceptanceId == TEXT("STOPFIRE") || AcceptanceId == TEXT("NEW-HOLD")) && AcceptancePhase == 2)
    {
        if (AcceptanceId == TEXT("STOPFIRE") && AcceptanceSinceEvent < 9.9f && AcceptanceCompany->MovementExecutor->HasMovementGoal())
        { FinishBehaviourAcceptanceCase(false, TEXT("genoptaget foer 10s ro")); return; }
        if (AcceptanceSinceEvent > (AcceptanceId == TEXT("STOPFIRE") ? 70.0f : 25.0f))
        {
            const bool bAcceptanceHold = AcceptanceCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Hold &&
                !AcceptanceCompany->MovementExecutor->GetSuspendedMission().IsValidOrder() && !AcceptanceCompany->MovementExecutor->HasMovementGoal();
            const bool bAcceptanceResumed = AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial == AcceptanceSerial &&
                !AcceptanceCompany->MovementExecutor->GetSuspendedMission().IsValidOrder() &&
                FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceStart) > 2500.0f;
            AcceptanceCheck(AcceptanceId == TEXT("NEW-HOLD") ? bAcceptanceHold : bAcceptanceResumed, TEXT("suspension/10s hysterese/ny HOLD/resume kontrolleret")); return;
        }
    }
    if (AcceptanceId == TEXT("DELAY"))
    {
        if (AcceptanceSinceEvent < 4.0f && AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial != AcceptanceSerial)
        { FinishBehaviourAcceptanceCase(false, TEXT("forsinket ordre leveret for tidligt")); return; }
        if (AcceptanceSinceEvent > 6.0f) { AcceptanceCheck(!AcceptanceCompany->OrderComponent->HasDelayedOrder() && AcceptanceCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Move, TEXT("5s ordre-latency uden tidlig levering")); return; }
    }
    if (AcceptanceId == TEXT("SPREAD") && AcceptanceSinceEvent > 4.0f)
    {
        if (AcceptancePhase == 1)
        {
            if (AcceptanceCompany->StanceComponent->Stance != EStrategyStance::Prone || AcceptanceCompany->FormationComponent->RankCount != 1 ||
                !AcceptanceCompany->FieldOfficerComponent->IsTakingFireCover()) { FinishBehaviourAcceptanceCase(false, TEXT("SPRED gav ikke liggende enkelt raekke")); return; }
            AcceptanceCompany->FieldOfficerComponent->SetManualSpread(false);
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
        }
        else { AcceptanceCheck(AcceptanceCompany->StanceComponent->Stance == EStrategyStance::Standing && !AcceptanceCompany->FieldOfficerComponent->IsTakingFireCover() && AcceptanceCompany->FormationComponent->RankCount == 3, TEXT("SPRED/SAML genopretter stance og raekker med AI OFF")); return; }
    }
    if (AcceptanceId == TEXT("SCREEN") && AcceptanceSinceEvent > 8.0f)
    {
        if (AcceptancePhase == 1)
        {
            if (!AcceptanceCompany->SkirmisherComponent->IsDeployed() || AcceptanceCompany->SkirmisherComponent->DetachedStrength <= 0)
            { FinishBehaviourAcceptanceCase(false, TEXT("skaerm ikke deployeret")); return; }
            AcceptanceCompany->SkirmisherComponent->RecallSkirmishers();
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
        }
        else { AcceptanceCheck(AcceptanceCompany->SkirmisherComponent->State == EStrategySkirmisherState::Attached && AcceptanceCompany->SkirmisherComponent->DetachedStrength == 0, TEXT("deploy og recall gennem rigtig komponent")); return; }
    }
    if (AcceptanceId == TEXT("WORKS") && AcceptanceSinceEvent > 35.0f)
    { AcceptanceCheck(AcceptanceCompany->FieldworksComponent->HasCompletedFieldworks() && IsValid(AcceptanceCompany->FieldworksComponent->GetCompletedPosition()), TEXT("30s BYG efterlader virkelig stillingsaktoer")); return; }
    if (AcceptanceId == TEXT("WORKS-CANCEL") && AcceptanceSinceEvent > 5.0f && AcceptancePhase == 1)
    { AcceptanceCompany->FieldworksComponent->CancelFieldworks(); AcceptancePhase = 2; }
    if (AcceptanceId == TEXT("WORKS-CANCEL") && AcceptanceSinceEvent > 35.0f)
    { AcceptanceCheck(!AcceptanceCompany->FieldworksComponent->bBuilding && !AcceptanceCompany->FieldworksComponent->HasCompletedFieldworks(), TEXT("annulleret BYG faerdiggoeres ikke senere")); return; }
    if (AcceptanceId == TEXT("WORKS-MOVE") && AcceptancePhase == 1 && AcceptanceSinceEvent > 5.0f)
    {
        Strategy1864Acceptance::Order(AcceptanceCompany, EStrategyOrderType::Move, AcceptanceStart + FVector(5000, 0, 0));
        AcceptancePhase = 2;
    }
    if (AcceptanceId == TEXT("WORKS-MOVE") && AcceptanceSinceEvent > 35.0f)
    { AcceptanceCheck(!AcceptanceCompany->FieldworksComponent->bBuilding && !AcceptanceCompany->FieldworksComponent->HasCompletedFieldworks(), TEXT("leveret FLYT annullerer BYG")); return; }
    if (AcceptanceId == TEXT("LONG-MOVE") && AcceptanceSinceEvent > 220.0f)
    { AcceptanceCheck(AcceptanceCompany->FormationComponent->CurrentFormation == EStrategyFormationType::Line &&
        FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceStart + FVector(25000, 0, 0)) < 100.0f, TEXT("lang march ankommer i kampformation")); return; }
    if (AcceptanceId == TEXT("DISENGAGE") && AcceptanceCompany->OrderComponent->GetCurrentOrder().Type == EStrategyOrderType::Hold)
    { AcceptanceCheck(AcceptanceCompany->CombatComponent->AmmunitionRounds < AcceptanceAmmo && AcceptanceCompany->GetActorLocation().X < AcceptanceStart.X - 3000.0f &&
        FMath::Abs(FMath::FindDeltaAngleDegrees(AcceptanceCompany->GetActorRotation().Yaw, 0.0f)) < 5.0f, TEXT("AFBRYD afgav rigtig salve, gik baglaens ud af LONG, sluttede HOLD med front bevaret")); return; }
    if (AcceptanceId == TEXT("CONTACT") && AcceptanceSinceEvent > 10.0f)
    {
        FStrategyContactRecord AcceptanceAged;
        AcceptanceCheck(AcceptanceCompany->ContactComponent->GetLastKnownContact(AcceptanceOther->StableUnitId, AcceptanceAged) &&
            !AcceptanceAged.bCurrentlyVisible && AcceptanceAged.LastKnownPosition.Equals(AcceptanceSnapshot, 1.0f) &&
            AcceptanceAged.SecondsSinceSeen > 8.0f && AcceptanceAged.Confidence < 1.0f && AcceptanceAged.Confidence > 0.0f && AcceptanceAged.UncertaintyRadiusCm > 0.0f,
            TEXT("tabt syn fryser position; alder/usikkerhed vokser og confidence falder")); return;
    }
    if (AcceptanceId == TEXT("ROE") && AcceptanceSinceEvent > 20.0f)
    {
        const bool bAcceptanceHeld = AcceptanceCompany->CombatComponent->AmmunitionRounds == AcceptanceAmmo && !AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther);
        AcceptanceCompany->FireDisciplineComponent->SetDiscipline(EStrategyFireDiscipline::FireAtWill);
        AcceptanceCheck(bAcceptanceHeld && AcceptanceCompany->CombatComponent->TryFireAt(AcceptanceOther) && AcceptanceCompany->CombatComponent->AmmunitionRounds < AcceptanceAmmo,
            TEXT("HOLD ILD ingen salve; fri ild paa samme maal giver virkelig salve/ammo-forbrug")); return;
    }
    if (AcceptanceId == TEXT("SQUARE-ZONES") && AcceptanceSinceEvent > 30.0f)
    {
        bool bAcceptanceFaces = AcceptanceCompany->EquipmentVisualComponent->bBayonetFixed;
        for (int32 AcceptanceFace = 0; AcceptanceFace < 4; ++AcceptanceFace)
        {
            const FVector AcceptanceDirection = FRotator(0, AcceptanceFace * 90.0f, 0).Vector();
            bAcceptanceFaces &= AcceptanceCompany->FireControlComponent->IsLocationInsideFireField(AcceptanceStart + AcceptanceDirection * 5000.0f, 7000.0f);
        }
        AcceptanceCheck(bAcceptanceFaces && !AcceptanceCompany->FormationTransition->IsReforming(), TEXT("carre fire retninger/bajonetter/faerdig reform; salveandele testes manuelt")); return;
    }
    if (AcceptanceId == TEXT("T3") && AcceptanceSinceEvent > 5.0f)
    {
        if (AcceptancePhase == 1)
        {
            if (AcceptanceCompany->Morale >= AcceptanceSnapshot.X || AcceptanceFar->Morale < AcceptanceSnapshot.Y - 0.1f || AcceptanceCompany->UnitState == EStrategyUnitState::Routed)
            { FinishBehaviourAcceptanceCase(false, TEXT("rout gav ikke lokal 80m moralsmitte uden fjern/kaskadeeffekt")); return; }
            AcceptanceSnapshot.X = AcceptanceCompany->Morale;
            AcceptanceOther->CombatComponent->EvaluateRoutState();
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
        }
        else { AcceptanceCheck(AcceptanceCompany->Morale >= AcceptanceSnapshot.X - 0.1f, TEXT("samme rout smitter ikke igen under cooldown")); return; }
    }
    if (AcceptanceId == TEXT("T7"))
    {
        if (!IsValid(SpawnedRiverBarrier)) { FinishBehaviourAcceptanceCase(false, TEXT("FIXTURE: bro forsvandt")); return; }
        const bool bAcceptanceFirstOwner = SpawnedRiverBarrier->IsCrossingOwner(AcceptanceCompany);
        const bool bAcceptanceSecondOwner = SpawnedRiverBarrier->IsCrossingOwner(AcceptanceOther);
        {
            static float LastT7Log = -10.0f;
            if (AcceptanceElapsed - LastT7Log >= 1.0f || AcceptanceElapsed < LastT7Log)
            {
                LastT7Log = AcceptanceElapsed;
                UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT T7 t=%.0f company dx=%.0f dy=%.0f own=%d wait=%d idx=%d goal=%d | other dx=%.0f dy=%.0f own=%d wait=%d idx=%d goal=%d"), AcceptanceElapsed,
                    AcceptanceCompany->GetActorLocation().X - SpawnedRiverBarrier->GetActorLocation().X, AcceptanceCompany->GetActorLocation().Y - SpawnedRiverBarrier->GetActorLocation().Y,
                    bAcceptanceFirstOwner, AcceptanceCompany->MovementExecutor->IsWaitingForBridge(), AcceptanceCompany->MovementExecutor->GetRoutePointIndexForTest(), AcceptanceCompany->MovementExecutor->HasMovementGoal(),
                    AcceptanceOther->GetActorLocation().X - SpawnedRiverBarrier->GetActorLocation().X, AcceptanceOther->GetActorLocation().Y - SpawnedRiverBarrier->GetActorLocation().Y,
                    bAcceptanceSecondOwner, AcceptanceOther->MovementExecutor->IsWaitingForBridge(), AcceptanceOther->MovementExecutor->GetRoutePointIndexForTest(), AcceptanceOther->MovementExecutor->HasMovementGoal());
            }
        }
        if (bAcceptanceFirstOwner && bAcceptanceSecondOwner) { FinishBehaviourAcceptanceCase(false, TEXT("to samtidige broejere")); return; }
        if (SpawnedRiverBarrier->IsInsideRiver(AcceptanceCompany->GetActorLocation()) && SpawnedRiverBarrier->IsInsideRiver(AcceptanceOther->GetActorLocation()))
        {
            UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT T7 pos: company dx=%.0f dy=%.0f owner=%d state=%d | other dx=%.0f dy=%.0f owner=%d state=%d | bridge x=%.0f"),
                AcceptanceCompany->GetActorLocation().X - SpawnedRiverBarrier->GetActorLocation().X, AcceptanceCompany->GetActorLocation().Y - SpawnedRiverBarrier->GetActorLocation().Y, bAcceptanceFirstOwner, int32(AcceptanceCompany->UnitState),
                AcceptanceOther->GetActorLocation().X - SpawnedRiverBarrier->GetActorLocation().X, AcceptanceOther->GetActorLocation().Y - SpawnedRiverBarrier->GetActorLocation().Y, bAcceptanceSecondOwner, int32(AcceptanceOther->UnitState),
                SpawnedRiverBarrier->GetActorLocation().X);
            FinishBehaviourAcceptanceCase(false, TEXT("to enheder inde i floden samtidigt")); return; }
        if (bAcceptanceFirstOwner || bAcceptanceSecondOwner) AcceptancePhase = 2;
        if (AcceptancePhase == 2 && AcceptanceCompany->GetActorLocation().X > AcceptanceStart.X + 22000 && AcceptanceOther->GetActorLocation().X > AcceptanceStart.X + 22000)
        { AcceptanceCheck(!bAcceptanceFirstOwner && !bAcceptanceSecondOwner, TEXT("begge krydsede; ingen samtidighed; begge frigav broen")); return; }
    }
    if (AcceptanceId == TEXT("T1"))
    {
        if (AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial != AcceptanceSerial)
        { FinishBehaviourAcceptanceCase(false, TEXT("carre erstattede den autoritative mission")); return; }
        if (AcceptancePhase == 1 && AcceptanceCompany->ThreatReactionComponent->IsRespondingToCavalry())
        {
            if (AcceptanceSinceEvent > 5.0f || AcceptanceCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Square ||
                !AcceptanceCompany->EquipmentVisualComponent->bBayonetFixed || AcceptanceCompany->MovementExecutor->HasMovementGoal() ||
                AcceptanceCompany->MovementExecutor->GetSuspendedMission().OrderSerial != AcceptanceSerial)
            { FinishBehaviourAcceptanceCase(false, TEXT("carre-start X=5s/HALT/bajonetter/mission fejlede")); return; }
            // Remove the observed threat, not the contact record or reaction state.
            AcceptanceCavalry->Destroy();
            AcceptancePhase = 2;
            AcceptanceEventAt = AcceptanceElapsed;
            AcceptanceSnapshot = AcceptanceCompany->GetActorLocation();
            return;
        }
        if (AcceptancePhase == 1 && AcceptanceSinceEvent > 5.0f) { FinishBehaviourAcceptanceCase(false, TEXT("ingen carre-start inden X=5s fra 200m")); return; }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent < 19.0f && !AcceptanceCompany->ThreatReactionComponent->IsRespondingToCavalry())
        { FinishBehaviourAcceptanceCase(false, TEXT("carre sluppet foer 20s lav trussel")); return; }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent < 19.0f && FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceSnapshot) > 100.0f)
        { FinishBehaviourAcceptanceCase(false, TEXT("automatisk carre fortsatte marchen under release-pausen")); return; }
        if (AcceptancePhase == 2 && AcceptanceSinceEvent > 35.0f && !AcceptanceCompany->MovementExecutor->GetSuspendedMission().IsValidOrder())
        {
            AcceptanceCheck(AcceptanceCompany->OrderComponent->GetCurrentOrder().OrderSerial == AcceptanceSerial &&
                AcceptanceCompany->FormationComponent->CurrentFormation != EStrategyFormationType::Square && !AcceptanceCompany->FormationTransition->IsReforming() &&
                FVector::Dist2D(AcceptanceCompany->GetActorLocation(), AcceptanceSnapshot) > 100.0f,
                TEXT("200m -> carre-start <=5s; 20s release; reform og samme mission genoptaget")); return;
        }
    }
}

void AStrategyOOBTestScenario::FinishBehaviourAcceptanceCase(bool bPassed, const FString& Reason)
{
    // Fieldworks owns its completed actor outside the scenario's spawn list.
    // Register it before clearing this fixture so later tests do not inherit cover.
    if (IsValid(AcceptanceCompany) && AcceptanceCompany->FieldworksComponent->HasCompletedFieldworks())
        SpawnedDefensivePositions.Add(AcceptanceCompany->FieldworksComponent->GetCompletedPosition());
    const FString AcceptanceId = AcceptanceIds[AcceptanceIndex];
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT %s %s %s (sim=%.2fs)"), *AcceptanceId, bPassed ? TEXT("PASS") : TEXT("FAIL"), *Reason, AcceptanceElapsed);
    AcceptanceOutcomes.Add(AcceptanceId, bPassed);
    AcceptanceResults.Add(FString::Printf(TEXT("| %s | %s | %.2f | %s |"), *AcceptanceId, bPassed ? TEXT("[x] PASS") : TEXT("[ ] FAIL"), AcceptanceElapsed, *Reason));
    if (bPassed) ++AcceptancePassCount; else ++AcceptanceFailCount;
    ++AcceptanceIndex;
    if (AcceptanceIndex < AcceptanceIds.Num()) { StartBehaviourAcceptanceCase(); return; }
    bAcceptanceFinished = true;
    ClearSpawnedUnits();
    WriteBehaviourAcceptanceReport();
    for (const FString& AcceptanceRow : AcceptanceResults) UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT SUMMARY %s"), *AcceptanceRow);
    UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT SUMMARY pass=%d fail=%d"), AcceptancePassCount, AcceptanceFailCount);
    FPlatformMisc::RequestExitWithStatus(false, AcceptanceFailCount == 0 ? 0 : 1);
}

void AStrategyOOBTestScenario::WriteBehaviourAcceptanceReport()
{
    const FDateTime AcceptanceNow = FDateTime::UtcNow();
    FString AcceptanceReport = FString::Printf(TEXT("# Adfaerdsaccept — %s UTC\n\nSimulationstid; speed=%.1f; basis-timeout=%.1fs (marchcases x4); seed=1864. PASS gaelder kun assertionens kontrakt.\n\n| ID | Status | Simulerede sekunder | Evidens / aarsag |\n|---|---|---|---|\n"),
        *AcceptanceNow.ToString(), AcceptanceDilation, AcceptanceTimeout);
    for (const FString& AcceptanceRow : AcceptanceResults) AcceptanceReport += AcceptanceRow + TEXT("\n");
    FString AcceptanceMatrix;
    if (FFileHelper::LoadFileToString(AcceptanceMatrix, *(FPaths::ProjectDir() / TEXT("Docs/Test-Skema-Enhedsadfaerd.md"))))
    {
        TArray<FString> AcceptanceLines;
        AcceptanceMatrix.ParseIntoArrayLines(AcceptanceLines, false);
        for (FString& AcceptanceLine : AcceptanceLines)
        {
            for (const auto& AcceptanceOutcome : AcceptanceOutcomes)
            {
                if (!AcceptanceLine.Contains(TEXT("<!-- accept:") + AcceptanceOutcome.Key + TEXT(" -->"))) continue;
                AcceptanceLine.ReplaceInline(TEXT("| [ ] | — |"), *FString::Printf(TEXT("| %s | %s |"), AcceptanceOutcome.Value ? TEXT("[x]") : TEXT("[ ]"), *AcceptanceNow.ToString(TEXT("%Y-%m-%d"))));
            }
            AcceptanceReport += AcceptanceLine + TEXT("\n");
        }
    }
    else
    {
        ++AcceptanceFailCount;
        AcceptanceReport += TEXT("\nFAIL REPORT: kildechecklisten kunne ikke indlaeses.\n");
        UE_LOG(LogTemp, Error, TEXT("PROJECT1864-ACCEPT REPORT FAIL kildecheckliste mangler"));
    }
    AcceptanceReport += FString::Printf(TEXT("\nPROJECT1864-ACCEPT SUMMARY pass=%d fail=%d\n\n"), AcceptancePassCount, AcceptanceFailCount);
    const FString AcceptanceDirectory = FPaths::ProjectSavedDir() / TEXT("Reports");
    const FString AcceptancePath = AcceptanceDirectory / (TEXT("accept_") + AcceptanceNow.ToString(TEXT("%Y%m%d_%H%M%S_%s")) + TEXT(".md"));
    if (!IFileManager::Get().MakeDirectory(*AcceptanceDirectory, true) ||
        !FFileHelper::SaveStringToFile(AcceptanceReport, *AcceptancePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        ++AcceptanceFailCount;
        UE_LOG(LogTemp, Error, TEXT("PROJECT1864-ACCEPT REPORT FAIL %s"), *AcceptancePath);
    }
    else UE_LOG(LogTemp, Display, TEXT("PROJECT1864-ACCEPT REPORT %s"), *AcceptancePath);
}
