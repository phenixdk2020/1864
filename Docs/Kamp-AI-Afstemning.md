# Kamp-AI – afstemning mod design v2.0

Dato: **2026-10-09**. Grundlag: `Kamp-AI-Design-v2.0.md`, C++ i denne arbejdskopi, `Enhedsadfaerd1864.md`, `Unity-AI-Mapping.md` og `TestFlags.md`. Ingen build, editorstart eller kamptest er udført. **Bygget** betyder implementeret og tilsluttet i C++; det betyder ikke bestået accepttest. Blueprints/banernes overrides er ikke runtimeverificeret. Filhenvisninger nedenfor er relative til `Source/Strategy1864/`; funktionsnavne er søgeankre.

Den eksisterende kode er en brugbar prototype med langt flere specialenheder end fase 2 kræver. Den er ikke endnu designets samlede MissionAI/ReactionAI med lokal informationskontrakt. Feltofficerens nærmeste fjende og rytterens nye charge-kandidater vælges nu fra lokale kontakter med confidence, alder og kilde. De vigtigste resterende huller er skjulte data i længerevarende flankering/charge, rapporttransport, generel missions-/reaktionsarbitrering, lanes med bredde og en Auto-overgang uden kaskade.

## Afstemning af afsnit 1–29

### 1. Grundprincip

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyFieldOfficerComponent.cpp`: `IsOffensive`, `ThinkInfantry`, `StartCharge`, `NearestEnemy`; `Orders/StrategyOrderComponent.cpp`: `CanReplaceCurrentOrder`. Offensiv ordre kræves til almindeligt charge/fremrykning; spillerautoritet beskyttes. Nærmeste fjende vælges fra aktuelle kontakter. | `FallBack` kan erstatte en AI-mission. Vedvarende flankering/charge læser stadig aktører. Ingen generel regel om, at kun bataljon og opefter skriver missioner. |

### 2. Epoke-profiler

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyFireControlComponent.cpp`: `GetActiveRangeCm`; `Combat/StrategyFireDrillComponent.cpp`: `SetLoadingMethod`, `SupportsProneReload`; `AI/StrategyDoctrineComponent.cpp`: `GetPreferredEngagementRangeFraction`. Rækkevidde/genladning/drill findes som enhedsdata. | Ingen `FEraProfile`/`FEquipmentProfile` med 1825/1851/1864-valg fundet. Feltofficeren har stadig absolutte afstande, fx 150 m ved dækning og 400–700 m ved rytter-målvalg. Ingen fælles epoke-swap-fixture. |

### 3. Hierarki og spiller

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Command/StrategyCommandComponent.cpp`: `SetCurrentCommandParent`, `AddCurrentSubordinate`; `AI/StrategyOfficerAIComponent.cpp`: `SetAIEnabled`, `EvaluateInheritedMission`; `Player/StrategyPlayerController.cpp`: `ToggleSelectedOfficerAI`, `GetPendingCouriers`. Hierarki, AI-toggle og ordonnansstatus findes. | Toggle kalder `SetAIEnabled(..., true)` og ændrer underordnede; ikke uafhængig `ControlMode` pr. node. Ingen garanteret ventetid ved overtagelse, seneste-plan-visning eller fælles HQ-kontaktbillede. Direkte kompagnikommando er mulig. |

### 4. Mission og reaktion

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Orders/StrategyOrderComponent.cpp`: `SetOrder`; `Combat/StrategyThreatReactionComponent.cpp`: `EnterSquare`, `TryLeaveSquare`; `AI/StrategyFieldOfficerComponent.cpp`: `UpdateAutomaticLooseOrderUnderFire`, `LeaveAutomaticFireCover`; `Movement/StrategyMovementExecutorComponent.cpp`: `HaltForFire`, `TickComponent`. Stop-og-ild har efter denne rettelse en `SuspendedMission` og genoptagelse. | Ingen fælles blackboard eller særskilt missions-/reaktionstilstand. Ny ordre leveres normalt, ikke som designets ventende ordre under reaktion. Carré pauser formering og bevarer bevægelsesordren, men marcherer efterfølgende meget langsomt; det er ikke designets fulde HALT/HOLD_CARRE-kontrakt. |

### 5. Information

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyContactComponent.h`: `FStrategyContactRecord` er enhedens lokale kontakt-blackboard med `LastKnownPosition`, `Heading`, `ObservedVelocity`, `LastSeenTime`, `Confidence`, `Source` og `UncertaintyRadiusCm`. `RefreshContacts` opdaterer kun observationer efter `CanDetectTarget`; scanning 0,35 s, lineær confidence-decay og glemsel 120 s. `FieldOfficer::NearestEnemy`, `ThinkCavalry` og `ThreatReaction::FindVisibleEnemyCavalry` bruger aktuelle kontakter; afstand/hastighed kommer fra snapshots. | Report/HQ er kildetyper, men ingen rapporttransport opad. Ingen samlet company-blackboard for mission/reaktion. Vedvarende flankering/charge og synlige kandidaters tilstand læses fortsat på aktøren. `Artillery/StrategyMortarFireComponent.cpp`: `FireOneBomb` følger `UnitTarget` direkte uden frisk spotterrapport. |

### 6. Ordreobjekt, latency og betingelser

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Orders/StrategyOrderTypes.h`: `FStrategyOrder`, `EStrategyOrderExecutionState`; `Orders/StrategyOrderComponent.cpp`: `QueueDelayedOrder`, `TickComponent`; `AI/StrategyCommandDelayComponent.cpp`: `CalculateDelayFromCurrentParent`; `Player/StrategyPlayerController.cpp`: `TickCouriers`, `DeliverOrder`. Forsinkelse, autoritet, serial, supersede og officersfortolkning findes. | Ingen samlet Issued/InTransit/Received-kontrakt med issuer/prioritet/tidsstempler. Ordonnans er visuel og har ingen kampdødsregel; ugyldig rytter giver fallback til rejsetid, ikke tabt ordre. Ingen `Conditions`/`ThenOrder`. |

### 7. Slots og lanes

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Formations/StrategyParentFormationPlannerComponent.cpp`: `GenerateCompanyLineSlots`, `IssueCompanySlotsForOrder`, `IssueDirectSubordinateSlots`; `AI/StrategyFieldOfficerComponent.cpp`: `AssignFlanks`, `ApproachGoalAt`. Frontpladser, ildbase, flanker, reserve og omvej uden om ildbasens kegle findes. | Ingen bataljonsejet eventstyret FrontAllocator med hullukning; flankernes omvej er geometrisk waypoint, ikke breddeangivet lane. Ingen garanti mod alle overlap eller alle aktive ildfelter. |

### 8. Ildfelt og friendly fire

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyFireControlComponent.cpp`: `IsLocationInsideFireField`, `CanEngageTarget`, `GetBearingFraction`; `Combat/StrategyVisibilityComponent.cpp`: `CanDetectTarget`; `Combat/StrategySmokeField.cpp`: `Tick`. Kegler, formationsfront, LOS og røg findes. | Ingen samlet `FriendlyUnitsInsideCone`/`FireBlocked`-arbitrering fundet i `CanEngageTarget`/`Combat::FindBestTarget`. Omvej omkring ildbase er ikke en generel ildstopregel. 2D-keglegeometri kombineres med LOS; ikke en samlet 3D-sektor med venlige enheder. |

### 9. Leash

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyMissionConstraintsComponent.cpp`: `SetMissionArea`, `ClampGoalToMissionArea`, `CanPursueTarget`; `Orders/StrategyMissionAnchorComponent.cpp`: `HandleOrderChanged`; `AI/StrategyAutonomousBattleAIComponent.cpp`: `FindCurrentVisibleEnemy`, `TickComponent`. Område og do-not-pursue findes. | Feltofficerens `UpdateBayonetCharge` retargeter efter levende mål uden fuld ChargeLeash/Return/Resume. `ThinkInfantry` bruger en anden 400 m mål-clamp. Ingen generel pursuit-leash og eksplicit retursekvens. |

### 10. Threat-system

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyThreatReactionComponent.cpp`: `FindVisibleEnemyCavalry` bruger aktuelle kontakter med snapshot-position/-hastighed, lukningsretning, korridor og tid til formering. `AI/StrategyFieldOfficerComponent.cpp`: `IsWavering`, `ThinkInfantry`. | Confidence er endnu kun et positivt kontaktgate; ingen normaliseret `ThreatScore`, responskurver/vægtdata, gradueret confidence-/aldersfaktor eller ensartet artilleri-/flanketrussel. Kavaleri-confidence i `StrategyCavalryChargeComponent` er kampkraft fra moral/cohesion, ikke kontakt-confidence. |

### 11. Infanteri og countercharge

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyFieldOfficerComponent.cpp`: `ThinkInfantry`, `StartCharge`, `UpdateBayonetCharge`, `ResolveShock`; `Visual/StrategyEquipmentVisualComponent.h`: `SetBayonetFixed`. Ild, frontskifte, bajonetcharge og shock findes. | Charge kræver offensiv ordre og er ikke særskilt LOCAL_COUNTERCHARGE. Ingen samlet opportunitiescore, hårdt terrængate eller reform/retur til assigned position. `ResolveShock` kan afslutte i HOLD; dette er ikke ændret i den lille stop-og-ild-rettelse. |

### 12. Carré

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyThreatReactionComponent.cpp`: `FindVisibleEnemyCavalry`, `EnterSquare`, `TryLeaveSquare`; `Formations/StrategyFormationTransitionComponent.cpp`: `HandleFormationChanged`, `CompleteReform`. Synlig, rettet og hurtig kavaleritrussel; tidligere formation genoprettes. | Release er **8 s**, ikke designets 20 s under score 30; ingen score 70/30, generel cooldown eller afvisning af carré i skov/på bro/dæmning. Carré-formering bevarer ordren gennem `PauseMovementForSeconds`; ikke tabt mission, men manglende fuld holdreaktion. |

### 13. Artilleri som trussel/mål

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyFieldOfficerComponent.cpp`: `NotifyIncomingFire`, `UpdateAutomaticLooseOrderUnderFire`, `IsOpenToCharge`; `Artillery/StrategyArtilleryFireMissionComponent.cpp`: `FireAt`. Fjernild kan udløse liggende/spredt orden; batteri regnes som charge-mål. | `IsOpenToCharge` antager, at artilleri er udækket uden at undersøge skærm. Ingen lokal ENGAGE_BATTERY med kontaktalder/confidence/leash eller rapport/anmodning om counter-battery. |

### 14. Beskyt eget artilleri

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Mangler | `Artillery/StrategyArtilleryCaptureComponent.cpp`: `HasFriendlyProtection` stopper erobring ved venlige enheder i nærheden. Det er en beslægtet erobringsregel. | Ingen `EBatteryProtection`, GUARD_BATTERY, COVER_BATTERY, alarmrapport eller slot-/skærmopgave fundet. Beskyttelse ved erobring er ikke den ønskede aktive AI-beskyttelse. |

### 15. Ilddisciplin/drill

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyFireDisciplineComponent.cpp`: `CalculateShotBudget`, `GetReloadMultiplier`; `Combat/StrategyFireDrillComponent.cpp`: `SelectAutomaticDrillMode`, `AdvanceFireByRankCycle`, `SupportsProneReload`; `Combat/StrategyCombatComponent.cpp`: `TryFireAt`; `Visual/StrategyInfantryVisualComponent.cpp`: `HandleVolleyVisualEvent`. | Kontrolleret ild, hold-fire, ammoøkonomi, rangvis ild og visuel salve findes. Mangler fælles udstyrsprofil, trusselsstyret intensitet og den generelle venlig-ild-gate fra afsnit 8. |

### 16. Artilleri-AI

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Artillery/StrategyArtilleryDeploymentComponent.cpp`: `RequestLimber`, `RequestDeploy`; `Artillery/StrategyArtilleryPositioningComponent.cpp`: `EvaluatePosition`, `FindBestDirectFirePosition`; `Artillery/StrategyArtilleryFireMissionComponent.cpp`: `SetManualAreaTarget`, `SelectBestAmmoForTarget`, `CalculateTargetScore`; `Artillery/StrategyArtilleryCaptureComponent.cpp`: `AttemptReuse`, `CompleteCapture`. | Mobilitet med tider, terræn/LOS-score, ammunition, ildmissioner og erobring findes. Ingen samlet batterichefkontrakt med ProtectionState, screen/escape-route, counter-battery-ordreprioritet eller tidsvurderet limber/abandon under nærtrussel. |

### 17. Morter

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Artillery/StrategyMortarDeploymentComponent.cpp`: `RequestEmplace`, `RequestPack`; `Artillery/StrategyMortarFireComponent.cpp`: `SetUnitTarget`, `SetAreaTarget`, `FireOneBomb`, `IsTargetInRange`, `BuildHighArcApex`. Opsætning, ammunition, min/max-range og indirekte projektilvisning findes. | Ingen målkontrakt med observatør-id og rapportalder; ingen stop ved spottertab. Enhedsmål følger aktørens aktuelle position. |

### 18. Kavaleri

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyCavalryScreenAIComponent.cpp`: `TickComponent`; `AI/StrategyCavalryTaskingComponent.cpp`: `AssignDefensiveReserve`; `Combat/StrategyCavalryChargeComponent.cpp`: `IsSteadySquare`, `TickComponent`; `AI/StrategyFieldOfficerComponent.cpp`: `ThinkCavalry`, `IsOpenToCharge`; `Units/StrategyDragoonComponent.cpp`: `DismountAtCurrentPosition`. | Screen, charge, carré-afvisning, reserve og dragoner findes. En klar linje kan stadig charges under offensiv ordre; batteriets skærm kontrolleres ikke. Ingen samlet charge-score/leash/horse-fatigue-kontrakt, circling/call-artillery eller rapportbaseret pursuit. |

### 19. Bataljon/brigade/division

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Orders/StrategyParentExecutionComponent.cpp`: `TickComponent`; `Formations/StrategyParentFormationPlannerComponent.cpp`: `IssueCompanySlotsForOrder`, `IssueDirectSubordinateSlots`; `AI/StrategyOfficerAIComponent.cpp`: `EvaluateInheritedMission`; `AI/StrategyFieldOfficerComponent.cpp`: `AssignFlanks`, `ApproachGoalAt`. | Hierarkisk eksekvering og kompagniereserve findes. Ikke fulde FBattalionPlan/FBrigadePlan/FDivisionPlan baseret på forsinkede rapporter, GuardAssignments, eventstyret frontreparation, morterobservatør eller counter-battery. |

### 20. Tilstand, moral og ammunition

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Combat/StrategyCombatComponent.cpp`: `NotifyIncomingVolley`, `EvaluateRoutState`, `ConfigureCartridgesPerMan`; `Combat/StrategyConditionComponent.cpp`: `TickComponent`, `GetMovementSpeedMultiplier`, `GetAccuracyMultiplier`; `AI/StrategyRoutRecoveryComponent.cpp`: `TickComponent`. | Moral, cohesion, fatigue, ammunition og rout/recovery findes. Ingen særskilt 0–100 suppression eller lokal moralsmitte med afstand/cooldown for rout, fane/officer/kanontab fundet. HorseFatigue er ikke den fulde model fra designet. |

### 21. Terræn og passager

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Navigation/StrategyRiverBarrier.cpp`: `TryAcquireCrossing`, `ReleaseCrossing`; `Movement/StrategyMovementExecutorComponent.cpp`: `UpdateBridgeQueueState`, `UpdateBridgeFormationState`; `Navigation/StrategyRoutePlannerComponent.cpp`: `FindRelevantRiverBarrier`; `Formations/StrategyFormationPolicyComponent.cpp`: `ColumnFormation`, `EvaluateEarlyDeployment`. Broen har kapacitet, default ét kompagni. | Ikke generel passagekø for gader/vad/skovbryn eller lane-breddekontrakt. Ingen konsekvent hård terrænafvisning af carré/charge og kontrol af linjens fulde frontage. |

### 22. Prioritet/arbitrering

| Status | Bygget og evidens | Forskel til designet |
|---|---|---| 
| Delvist bygget | `AI/StrategyFieldOfficerComponent.cpp`: `TickComponent`, `ThinkInfantry`; `Combat/StrategyThreatReactionComponent.cpp`: `EnterSquare`; `Orders/StrategyOrderComponent.cpp`: `CanReplaceCurrentOrder`. AFBRYD/dækning køres før officerens normale tick; carré rydder automatisk dækning. | Grenrækkefølge og autoritet er ikke en fælles firelags arbitrator. Ingen generel SupersededMission-kø under reaktion; rutine-HOLD/withdraw/charge kan stadig erstatte AI-missioner. |

### 23. Officerkvalitet

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyOfficerProfileComponent.cpp`: `GetDecisionStability`, `GetCommandEfficiency`, `GetStressReactionMultiplier`; `AI/StrategyFieldOfficerComponent.cpp`: `TickComponent`, `UpdateAutomaticLooseOrderUnderFire`; `Player/StrategyPlayerController.cpp`: `DeliverOrder`; `Tests/StrategyOOBTestScenario.cpp`: `TickOfficers`. | Kvalitet påvirker kadence, tabstærskel, dækning og ordreusikkerhed; officersfald findes i testopsætningen. Ikke samlet OfficerPresent/efterfølger-kontrakt med ThreatAccuracy, ChargeJudgement og reorganisering på alle AI-veje. |

### 24. Doktrin som data

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyDoctrineComponent.cpp`: `GetEffectiveAggression`, `GetPreferredEngagementRangeFraction`; `AI/StrategyFieldOfficerComponent.cpp`: `PreferredFraction`, `IsOffensive`. Doktrin påvirker udførelse uden i sig selv at give offensiv ordre. | Ingen samlet nationsprofil kombineret med epoke-/udstyrsprofil, battery-protection/hunt-bias eller data-kurver for alle reaktioner. |

### 25. Sværhedsgrad

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyAIDifficultyComponent.cpp`: `GetReactionTimeMultiplier`, `GetDecisionNoiseAmplitude`; `AI/StrategyAutonomousBattleAIComponent.cpp`: `TickComponent`. Easy/Normal/Hard ændrer reaktion og støj. | Ingen fælles rapportlatency/betinget-ordre/reserve-tuning. Feltofficerens verdensopslag betyder, at princippet om ingen skjult viden endnu ikke gælder hele AI'en. |

### 26. Ydelsesbudget

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `AI/StrategyFieldOfficerComponent.cpp`: `SetDeterministicRandomSeed`, `TickComponent` forskyder tænkning; `Combat/StrategyContactComponent.cpp`: `TickComponent`; `Combat/StrategyThreatReactionComponent.cpp`: `TickComponent` (0,25 s). `Visual/StrategyInfantryVisualComponent.cpp`: `HandleVolleyVisualEvent` er præsentation, ikke soldat-MissionAI. | Ingen fælles scheduler/frame-budget eller budgetteret asynkron LOS. Kavaleritrussel har indlejrede verdensiterationer; kontakt/AI bruger egne akkumulatorer. Log med flag kan være meget omfattende og bør ikke bruges til performance-måling. |

### 27. Debug, determinisme og test

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `UI/StrategyWorldDebugComponent.cpp`: `DrawMissionForUnit`, `DrawRouteForUnit`; `Combat/StrategyFireControlComponent.cpp`: `DrawQARangeCones`; `AI/StrategyAITelemetryComponent.cpp`: `SetDecision`; ny `AI/StrategyDecisionLog.h`: `Strategy1864LogDecision`, kaldt fra officer, ordrearv, trussel og stop-og-ild. | Geometri, seneste beslutning og opt-in beslutningslog findes. Overlay mangler kontaktalder/confidence, alle reaktions-/missionslag, lanes/leash og threat-bidrag. RNG er delvis seedet pr. enhed; `PlayerController::DeliverOrder` bruger `FMath::FRand`, og tidssteg er ikke fastlåste. T1–T10 er ikke bestået af dette arbejde. |

### 28. Samlet angreb

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Tests/StrategyOOBTestScenario.cpp`: `BuildSkirmish` med Arms/Attack; `AI/StrategyFieldOfficerComponent.cpp`: `AssignFlanks`; `Artillery/StrategyArtilleryFireMissionComponent.cpp`: `SetManualAreaTarget`. Infanteri, batteri, morter og eskadron kan optræde sammen. | Samtidig tilstedeværelse er ikke den beskrevne kausale recon→rapport→ildforberedelse→guard→lanes→betinget pursuit-kæde. |

### 29. Implementeringsrækkefølge

| Status | Bygget og evidens | Forskel til designet |
|---|---|---|
| Delvist bygget | `Tests/StrategyOOBTestScenario.cpp`: `BuildSkirmish`; `Units/StrategyUnit.cpp`: konstruktør tilslutter order, movement, officer, contacts, threat og debug. Ét-mod-ét og bataljonsfixture findes allerede. | Fase 1-kontrakter og fase 2-gates er ikke afsluttet, selv om specialenheder/højere niveauer findes. Fortsæt med den eksisterende kompagnifixture; undgå ny specialenhedsudbygning før fase 2 er testet. |

## Sammenhold med de to adfærdsdokumenter

`Enhedsadfaerd1864.md` beskriver flere eksisterende balancevalg korrekt: kvalitetsstyret liggende/spredt orden, retningsbestemt kavalerivarsling og AFBRYD som spillerordre. Dets hidtidige permanente annullering ved stop-og-ild var faktisk implementeret i `HaltForFire`, men strider mod design v2.0's suspendering/genoptagelse. Den regel er rettet og dokumentet opdateret i denne ændring. Den afsluttede, almindelige spillerordre bliver fortsat ikke til en selvvalgt offensiv mission.

`Unity-AI-Mapping.md` er en sammenligning af prototyper og ikke en v2.0-acceptspecifikation. To lokale AI-veje findes stadig. Den autonome komponent bruger current contact og mission constraints; feltofficerens nærmeste-fjende-/ryttervalg bruger nu lokale kontakter, men vedvarende flankering/charge læser fortsat aktører. Den generelle parent→reaction→resume-stack mangler stadig; kun stop-og-ild-hullet er lukket. Kortlægningens gamle `R:/Onedrive/cx/mapai`-links er ikke evidens for denne arbejdskopi; brug fil-/funktionsankrene ovenfor.

## T1–T10: hvad kan testes nu?

**Testbar nu** betyder, at den fulde beskrevne regel kan undersøges med eksisterende kode og en manuel opsætning. **Kræver arbejde** betyder, at en essentiel regel eller reproducerbar fixture mangler. Delprøver nedenfor er ikke beståede acceptscenarier. Kommandoerne er forslag til senere, udtrykkeligt godkendt kørsel; ingen er kørt. Ingen opdigtede T1–T10-flag.

| Test | Status | Eksakt kommando hvor en relevant start findes | Mangler / manuel procedure og kriterium |
|---|---|---|---|
| T1: rytteri 200 m, carré, 20 s ro, resume | **Kræver arbejde** | `& .\Start-Test-3-Rytteri.bat -Strategy1864DebugOfficer -Strategy1864DebugDecisions` | Starter rytteri ca. 450 m fra danskerne, ikke 200 m. Manuel marchordre før kontakt kan undersøge bevaret mission. Release er 8 s, ikke 20; fuldt carré-HALT og score/hysterese mangler. X skal fastlægges; formeringsparametre er ikke en bestået tidsmåling. |
| T2: tre kompagnier/300 m, slots og ildkegler | **Kræver arbejde** | `& .\Start-3D-Skirmish-Test.bat -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=3 -Strategy1864EnemyDefends -Strategy1864DebugDecisions` | Ikke en præcis 300 m-frontfixture. Standardstart har Arms/Attack; ingen Passive. Undersøg flanker manuelt; brede lanes, overlapkontrol og alle venners ildfelt mangler. `Start-Test-2` er passiv og fire kompagnier; et ekstra Attack-flag ophæver ikke Passive. |
| T3: rout-smitte med cooldown | **Kræver arbejde** | `& .\Start-Test-2-Bataillon-mod-Kompagni.bat -Strategy1864DebugDecisions` (kun observation af eksisterende moral/rout) | Ingen flag til at route ét bestemt kompagni; den lokale smitte/cooldown-regel er ikke fundet. Skal have injektion med to naboafstande og gentaget rout-event. |
| T4: død ordonnans / faldet officer | **Kræver arbejde** | `& .\Start-Test-2-Bataillon-mod-Kompagni.bat -Strategy1864TestCourier -Strategy1864DebugDecisions` ; separat: `& .\Start-Test-2-Bataillon-mod-Kompagni.bat -Strategy1864TestOfficers -Strategy1864DebugDecisions` | Courier-test sender efter 25 s; officers-test ændrer officer ved 20 s og afslutter/skriver resultat ved 30 s. Ikke samme acceptfixture. Rytterdød taber ikke ordren; ugyldig rytter leveres efter tidsfallback. Fald/efterfølger-forsinkelse skal måles og have fælles kontrakt. |
| T5: batteri med/uden skærm | **Kræver arbejde** | `& .\Start-3D-Skirmish-Test.bat -Strategy1864SkirmishArms -Strategy1864DebugDecisions` | Arms giver specialenheder, ikke et skærmet/uskærmet A/B-scenarie. ProtectionState, COVER_BATTERY og nærtrussel-limber/abandon-beslutning mangler. |
| T6: udsat/skærmet fjendtligt batteri | **Kræver arbejde** | Samme Arms-kommando som T5 er kun en delprøve. | Ingen fixture eller flag for leash/skærm. `IsOpenToCharge` kalder batterier udækkede uden skærmcheck; rapport/counter-battery-request mangler. |
| T7: brokø, én ad gangen | **Testbar nu – manuelt i QA** | `& "I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "R:\Onedrive\cx2\aidesign\Game1864.uproject" /Game/Maps/Strategy1864_QA -game -windowed -Strategy1864FullOOB -Strategy1864NoCouriers -Strategy1864DebugDecisions` | QA's `BuildTestOOB` har `bSpawnRiverQA=true` som C++-default; ikke Duel/Skirmish. Vælg mindst to kompagnier på samme bred, giv ruter over samme gule QA-bro. Kontroller, at nr. 2 venter til nr. 1 har ryddet udgangen, også modsatgående. `TryAcquireCrossing` defaultkapacitet=1. Banens evt. override skal kontrolleres ved kørsel; der er intet automatisk T7-assertion-flag. Gælder QA-barrieren, ikke garanti for alle kampagnebroer. |
| T8: samme scenario 1825/1864 | **Kræver arbejde** | Ingen battle-era-kommando findes. | `-CampaignScenario=1825/1851` vælger kampagne og er ikke et 1825/1864-battle-equipment-swap. Profiler og identisk fixture skal bygges. |
| T9: Player/Auto midt i slag | **Kræver arbejde** | `& .\Start-Test-2-Bataillon-mod-Kompagni.bat -Strategy1864TestCourier -Strategy1864DebugDecisions` | Brug AI-knappen under ordonnansrejse som delprøve. Toggle findes, men kaskaderer og har ingen garanteret overtagelsesforsinkelse/baseline-kontrakt. Fuld T9 er derfor ikke testbar som designet endnu. |
| T10: betinget forfølgelse | **Kræver arbejde** | Ingen eksisterende kommando til betinget ordre. | `FStrategyOrder` har ingen Conditions/ThenOrder; ingen lokal condition-evaluator. |

Ekstra regression for den ændrede, lille skive: `& .\Start-Test-1-Kompagni-mod-Kompagni.bat -Strategy1864DebugDecisions`. Giv manuelt FLYT med flere vejpunkter ind i ild; kontroller `Suspender udførelse`, samme serial/autoritet/waypoint og `Genoptag mission` efter 10 s uden gyldigt ildmål/indgående ild. Gentag med AI-Advance, med ny HOLD under pause, en forsinket ny ordre, AI OFF, carré, reformering og rout. HOLD/ny ordre må ikke senere erstattes af gammel mission; rout må ikke genoptage. Dette er en regressionsprocedure, ikke noget der er udført.

## Prioriterede små vertikale skridt til fase 2

Størrelser er grove implementerings-/reviewestimater, ikke tilsagn: XS <½ dag, S ½–1½ dag, M 2–4 dage, L 5–8 dage. Runtime-test kræver særskilt tilladelse. **Codex alene** betyder afgrænset implementation med fast kontrakt; arkitektur- og integrationsvalg skal afklares først.

| Pri. | Mindste leverance og gate | Størrelse | Risiko | Sikker for Codex alene? |
|---|---|---|---|---|
| P0.1 | Opt-in beslutningslog i eksisterende officer/trussel; log også afvisninger og inputs. Leveret her. | S | Lav; store logmængder med flag | Ja. Ingen nye taktiske valg. |
| P0.2 | Bevar mission/rute ved stop-og-ild, genoptag kun samme ordre efter ro; ny ordre/rout invaliderer. Leveret her; manuel regression ovenfor mangler. | S | Lav–middel; ordretick, formation og waypoint-levering mødes her | Ja for denne afgrænsede fejl; ingen generel reaction-stack i samme patch. |
| P0.3 | Ét-mod-ét-fixture med fast start/mål og injicerbar kontakt/tab af kontakt, ny ordre under reaktion samt log-assertions. Definér forventning for march, Advance og AttackHere først. | S–M | Lav | Ja efter aftalte kriterier; byg ikke et nyt generisk testframework. |
| P0.4 | **Leveret 2026-10-09:** kontaktfelter, decay/usikkerhed og snapshot-input i nærmeste-fjende-/ryttervalg og kavalerireaktion. Manuel kontaktregression nedenfor mangler. | M | Middel; vedvarende flankering/charge og tilstandsvurdering mangler fuld snapshot-kontrakt | Lokal skive implementeret. Fuld HQ-rapporttransport er næste separate skridt. |
| P0.5 | Lås lille mission/reaktionskontrakt for kompagni: missionens ejer, reaktionsprioritet, suspendering, ny ordre under reaktion og release. Saml kun move/hold/attack + stop/cover/carré i første skive. | M–L | Høj; flere samtidige skriveejere | Nej som selvstændig arkitekturopgave. Codex kan implementere en aftalt handler ad gangen og validere autoriteten. |
| P0.6 | Carré-skive: behold mission, stå under trussel, reform/20 s release og resume; kendt kontakt, tids-til-kollision og ét hårdt brogate. T1 med 200 m-fixture. | M | Middel–høj; formation og movement tick | Ja efter kontrakten i P0.5; ikke samtidig retune alle kavaleriregler. |
| P1.1 | Fælles ordrestatistik i ét-mod-ét: issuer/issued/arrive/status, vis én ordre under vejs og lever med eksisterende latency. Invalider ved tabt ordonnans; efterfølgerens forsinkelse separat. | M | Middel; direkte/inherited/courier-veje | Ja i to små patches efter aftalt leveringskontrakt. Ikke fuldt latency-refaktor alene. |
| P1.2 | Udvid eksisterende selected-unit overlay med mission, `GetSuspendedMission()`, reaktion, kontaktalder/confidence og ordrestatus. Genbrug debug-geometri. | S–M | Lav | Ja efter kontakt-/reaktionsfelterne er låst. Ingen taktisk kodeændring. |
| P1.3 | Auto på **ét** HQ-niveau uden kaskade: behold ordren/ordonnanser, første vurdering efter reaktionstid, vis status/seneste plan. T9: toggle under levering og under lokal reaktion. | M | Middel; påvirker arve- og planlæggerticks | Ja efter baseline-/toggle-kontrakt; udvid først derefter til alle niveauer. |
| P1.4 | Bataljon med tre kompagnier: eksplicit slot-frontage og én lane pr. kompagni; én venlig-ild-gate for krydsende enhed. T2 på 300 m. | L | Høj; koordinering mellem bevægelse og ild | Nej alene uden FrontAllocator/FireSector-spec; afgrænsede overlap- og ildstopchecks er egnede til Codex. |
| P1.5 | Rout-event med afgrænset 80 m moral-hit og cooldown, højst én anvendelse pr. nabo/event. T3 uden gentagen smitte fra samme rout. | M | Middel; kan ændre kampbalance kraftigt | Ja efter tal og event-ejerskab er låst. |
| P1.6 | Reproducerbar QA-brofixture med to/modsatrettede ruter og log af køejer/indgang/ryddet udgang. T7, inklusive ny ordre under passage. | S–M | Lav–middel | Ja. Bevar eksisterende brotransaktion; generalisér ikke til alle passager endnu. |

Stop efter fase 2 og godkendte T1–T3/T7/T9-regressioner. Fuld epokeprofil, betingede ordrer, batteriskærm og morterobservatører er efterfølgende skiver, ikke små log-/bookkeeping-rettelser.

## Implementeret i denne ændring og grænser

- `-Strategy1864DebugDecisions` skriver `PROJECT1864-DECISION` med simulationstid, enhed, lag, valgt handling, årsag, ordre/serial/autoritet, styrke, moral, cohesion, fatigue og grenens ekstra inputs/afviste alternativer. Gentagne evalueringer logges også; ikke kun ændringer i telemetry. Kandidatafvisninger angiver fart, korridor/modtager, afstand eller synlighed. Der er ingen nye scores: ikke-evaluerede alternativer markeres som sådan. Dækning, infanteri-/ryttervurdering, accepterede/afviste lokale ordrer, ordrearv, carré og stop/resume er instrumenteret. Loggen er ikke et fuldt beslutningstræ for alle artilleri-/navigationskomponenter.
- `HaltForFire` mistede før ordren og rutens ventende udvidelser. Den gemmer nu `SuspendedMission`, stopper den fysiske bevægelse og bevarer **den autoritative ordre uændret**, inklusive serial, authority, target og waypoint-id/indeks. Release er et justerbart **10 s balanceestimat**, ikke en historisk måling. Genoptagelse bygger en fysisk rute fra den aktuelle position uden ny ordre/serial. Ingen genoptagelse under indgående ild, gyldigt ildmål, carré, reformering, ventende delayed order eller tab af kampdygtighed. Ny leveret ordre/clear/stop rydder den suspenderede mission.
- Carré bevarer allerede missionen via formationspausen; dækning ændrer stance/spacing og genopretter dem. `UpdateDisengage` håndterer en eksplicit Disengage-mission, hvis slutresultat HOLD er tilsigtet; den er ikke en midlertidig automatisk erstatning af en anden mission. Disse veje er ikke omskrevet. Generel fallback/charge/ny-ordre-under-carré-kontrakt står tilbage som større designarbejde.
- Suspensionen er transient slagtilstand. Der er ingen nye kampagne-savefelter eller scenarieændringer. 1851-data er uberørt.

Statisk validering: nye include-/API-brug er kontrolleret mod UE 5.8-headerne `Runtime/Core/Public/Misc/CommandLine.h`, `Misc/Parse.h`, `Logging/LogMacros.h` samt `Runtime/Engine/Classes/Engine/World.h`. Diff/whitespace, lokale funktionshenvisninger og order/route/authority-kæden er gennemgået. Det erstatter ikke C++-kompilering eller runtime-regression. Ingen commit.

## Fase 2-kørsel 2026-10-09: lokal kontaktskive

Opgavens punkt **1** er implementeret som en udvidelse af det eksisterende `FStrategyContactRecord`, ikke en parallel kontaktliste. `Contacts` i hver `UStrategyContactComponent` er det lokale kontakt-blackboard. Den gamle `LastKnownLocation` bevares og opdateres sammen med `LastKnownPosition` af hensyn til eksisterende C++/Blueprint-forbrugere. Egen observation nulstiller alder/usikkerhed og sætter confidence=1 og Source=OwnEyes. Uden observation falder confidence lineært til 0 over `ForgetAfterSeconds` (default 120 s); usikkerhed vokser med 1200 cm/s, et justerbart balanceestimat. Position, retning, hastighed og observationstid forbliver frosne. Report/HQ er kontraktværdier; der opfindes ingen rapporter eller fælles HQ-viden.

`NearestEnemy`, rytterens nye charge-kandidatvalg og kavaleritruslens fjendeinput bruger lokale aktuelle observationer og snapshot-afstand/-hastighed. Aktørreferencen er svag og valideres inden brug. Der er ingen fallback til verdensopslag, hvis kontaktkomponenten mangler. Kavaleriets eksisterende fart-/retning-/korridor-/varsling-/spillerformationsregler er bevaret; den venlige angrebskorridor bruger fortsat egne kompagnier fra verden. Kadencen giver op til én kontaktscanning (default 0,35 s) ekstra observationstid. Synlige kandidaters formation/moral læses fortsat på aktøren, og vedvarende flankering/charge er ikke fuldt omlagt. Derfor er ingen-skjult-viden-kontrakten endnu ikke komplet.

Manuel regression til senere godkendt kørsel: brug eksisterende ét-mod-ét-/rytterfixture med `-Strategy1864DebugDecisions`. Kontaktloggen skal vise OwnEyes, confidence=1 og radius=0 ved observation. Ved tab af syn skal position/heading/LastSeenTime forblive uændrede, alder og radius vokse, og confidence falde (omkring 0,5 efter 60 s med standardtal). Efter mere end 120 s uden syn skal recorden forsvinde. Gensyn skal nulstille alder/radius. Skjulte kontakter må ikke blive nye officer-/rytterkandidater eller udløse carré; kontroller også AI OFF, stop-og-ild med ny HOLD, AFBRYD og uændrede kavalerigates. Dette er en procedure, **ikke en bestået test**.

**Næste kørsel, i opgavens rækkefølge:** (2) fælles ReactionAI/SuspendedMission inklusive fallback og nye spillerordrer, med release-hysterese/formation-cooldown; (3) lagdelt data-arbiter og beslutningslog; (4) FireBlocked; (5) lokale Conditions/ThenOrder; (6) era-/equipmentprofiler og relativ afstand; (7) reproducerbare T1–T3-flag og PROJECT1864-ACCEPT. Ingen af punkt 2–7 er tilføjet i denne kørsel. Den eksisterende stop-og-ild-suspension er fortsat den eneste implementerede SuspendedMission; carré-release er fortsat 8 s.

Statisk kontrol af nye API/includes mod UE 5.8: `Engine/World.h`, `GameFramework/Actor.h`, `UObject/WeakObjectPtrTemplates.h`, `Containers/Array.h`, `Math/Vector.h` og `Math/UnrealMathUtility.h`; lokale komponentmetoder og hele diffen er gennemgået. Ingen build, editor/spil, runtime-test eller commit. Kontaktfelterne er transient slagtilstand; kampagne-save og 1825/1851-data er ikke ændret.
