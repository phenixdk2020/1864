# Kortlægning af Unity-enhedernes AI til Unreal

Dato: 8. oktober 2026. Statisk kildekodeanalyse; ingen build eller runtime-test. Unity-projekterne er kun læst. Unreal-status betyder implementeret i C++ i denne worktree, ikke dokumenteret fungerende i en bestemt Blueprint eller bane.

## Kilder, afgrænsning og sikkerhed i konklusionerne

**K** = `R:/Onedrive/Strategy-Kamp`, **R** = `R:/Onedrive/Strategy-Kampe-Rebuild`, **T** = `R:/Onedrive/Strategy-Test`. Unity-filnavne nedenfor er relative til `Assets/Scripts/`, medmindre andet står angivet. Unreal-filnavne er relative til `Source/Strategy1864/`. Funktionsnavne er søgeankre; bilaget indeholder linjenumre, klasser, konstanter og grenvariationer.

Alle `.cs`-filer under de tre `Assets`-træer er inventeret. Indholdet af 18 T-filer kunne ikke læses: operativsystemet returnerede adgang nægtet. De er navngivet i bilaget. Det gælder bl.a. `PrototypeCoordinatedAttack09F29E`, `PrototypeCommandSquareHardening09F30A`, `PrototypeSquareCorrections09F29V`, `PrototypeSquareRuntimeHotfix09F29M` og `PrototypeSquareSectorFire09F29S`. Deres regler og tærskler er **uverificerede**; henvisninger fra læsbare scripts er ikke en erstatning for deres kildekode. Kortlægningen er derfor fuldstændig som filinventar, men ikke som analyse af disse utilgængelige implementationer.

**Porteret** betyder en tydelig funktionel pendant, ikke identiske algoritmer. **Delvis** betyder at noget af adfærden findes, mens konkrete regler eller tilstande afviger. **Mangler** betyder at den beskrevne regel ikke er fundet i de gennemgåede C++-veje; det udelukker ikke eksterne Blueprints. **P0** = afgørende autoritet/korrekthed, **P1** = væsentlig taktisk adfærd, **P2** = kalibrering/forbedring, **P3** = historik/QA eller ikke værd at kopiere.

De tidlige Unity-rækker bruger komprimerede regimentsafstande. R/T har senere 1 Unity-enhed = 1 meter, men beholder klassenavnet `Regiment` om ca. 190-mands **kompagnier**. K har egentlige kompagnicentrene under et regiments-OOB. Unreal bruger centimeter: 70 m = 7.000 cm. Derfor må rå tal ikke overføres uden at identificere grenen og det aktive overskrivningslag.

## Arkitektur og faktisk ejer af adfærden

Unity bruger `Update`/`LateUpdate`, `DefaultExecutionOrder`, singleton-managers og reflection til at overskrive private felter i `Regiment`. Det er en kæde af delvist erstattede prototypelag, ikke ét Behaviour Tree. Mange filer opretter sig selv med `RuntimeInitializeOnLoadMethod`; tilstedeværelse i en scene er derfor ikke alene afgørende for aktivitet.

| Gren | Beslutning og ordre | Fysisk udførelse og senere overtagelse |
|---|---|---|
| Fælles kerne | `OfficerAIPrototypeManager` slår legacy `Regiment.IsAI` fra; `OfficerAIController` vælger Hold/Defend/Move/Attack og Line/Column | `Regiment` flytter, drejer, skyder og router. Managers ændrer private destinationer, accuracy, hastighed og skudtimer |
| K | `PrototypeBrigadeCommand09L` → `OfficerAIController` → `PrototypeKampCommandAuthority09M2` | `PrototypeIndependentCompanyMovement09L5` løsner kompagnier fra pivoten; `PrototypeCompanyTacticalEntity09L2` flytter dem; `PrototypeKampCompanyFire09M` skyder fra dem. Regimentspivoten følger centroiden og må ikke selv skyde |
| R | MajorHQ F15/F16 → MajorBattalion F17 → F18 overtager autoritet; F19 justerer reserve/flanke | `TacticalOrders`, manual authority, river og static routing ligger ovenpå regimentskernen. Ældre navigationsstakke deaktiveres af `NavigationV3Authority` |
| T | Division/Brigade F30B → Regiment F28 → `RegimentHierarchy09F27` → kompagnier; eksplicit mission overlever lokale reaktioner | F27 slår gamle Major-behaviours fra. `AttackAIHotfix09F29B` slår den gamle lokale kaptajn-bro fra efter F27-installation. `AttackContact`, under-fire, charge, square, river og lateral manoeuvre har særlige overtagelser |
| Unreal | `OrderComponent` ejer ordre/serial/autoritet; `OfficerAIComponent` arver; HQ's `ParentExecution` og `ParentFormationPlanner` fordeler; `FieldOfficer` og `AutonomousBattleAI` beslutter lokalt | `MovementExecutor`/`RoutePlanner`, `FormationPolicy`/`FormationTransition`, `Combat`/`FireControl` og specialkomponenter udfører. `StrategyUnit` konstruerer begge lokale AI-komponenter; kun kompagnier kører autonom fjende-battle-AI |

K's `NavigationV3Authority` holder V3 som styringsejer og slår V4/09A fra; 09B fjerner dekorative træer/hegn. R/T's version slår den gamle navigationsstak fra til fordel for `RiverBridgeOnly09F3` og `StaticObstacleRouting09F11`. T's `WideBattlefieldNavigation09F29E` udvider bevægelsen og giver efter for lokalt angrebskontakt. V4 findes altså i alle tre, men er ikke den valgte samtidige A*-autoritet.

**Aktive scenarieoverrides:** K's `PrototypeReducedQABattle09L3` fjerner andet regiment på hver side og deaktiverer `PrototypeBrigadeCommand09L`, når company-control-piloten er installeret. Brigade-reservelogikken findes, men kører ikke i denne reducerede QA-gate. R/T's `PrototypeMoraleTest09F8` opretter sig selv og låser moral til **100 i både Update og LateUpdate**; ingen deaktivering er fundet i de læsbare scripts. Moralreglerne nedenfor beskriver derfor kernen, ikke en garanti for akkumuleret moralnedgang i det aktive QA-scenarie. Et enkelt frame kan stadig nå en rout-check før LateUpdate-reset. T's F27 deaktiverer desuden F18/F17/F15, F18 UI/order visuals, F19 reserve, F20 intent/selection handoff, F22 planner, F23 Major-slot-solver, F24 commitment og F25 command-zone. Deres algoritmer er historiske kilder; F27 og senere hardening er det nyere udførelseslag.

## Grundlæggende infanteri- og officers-AI

| Script, gren og formål | Input og beslutningsregler / nøgletal | Tilstande og Unreal-pendant |
|---|---|---|
| `Regiment.cs`, K/R/T: legacy AI, bevægelse, ild, moral | Legacy tænker hver 0,7–1,2 s, waypoint indtil ≤4, nærmeste ikke-routede fjende inden 999, angriber udenfor 0,86 × effektiv rækkevidde. Danmark 3,2, Preussen 3,35 hastighed; ankomst <0,45. Cohesion giver hastighedsfaktor 0,72–1 og tab 0,18/s, bund 35. Uden ildtryk moral +0,7/s, cohesion +1/s | Flags `hasDestination`, `forcedTarget`, `IsRouted`; ikke en enum-FSM. Legacy AI deaktiveres af manageren. Porteret funktionelt i `Units/StrategyUnit`, `Movement/StrategyMovementExecutor`, `Combat/StrategyCombat`, `AI/StrategyRoutRecovery`; tal er forskellige |
| `OfficerProfile.cs`, identisk K/R/T: officerspersonlighed | Ni 0–100-værdier: Leadership, Inspiration, TacticalSkill, Initiative, StaffCommandSkill, Discipline, Aggressiveness, Composure, Experience. Navnebaserede QA-profiler; default 60 på de fleste, aggression/experience 50 | Datamodel, ingen FSM. `AI/StrategyOfficerProfileComponent` har bl.a. Caution, StaffQuality og Impairment, men ingen selvstændig Inspiration. QA-ratings bør ikke kopieres som historiske data, P3 |
| `OfficerAIController.cs`, K og R/T-varianter: lokal mission | Aggression = clamp(0,70 × officer + 0,30 × ordre ±12 for DEF/OFF). Stress = 0,58 × moraltab + 0,42 × cohesiontab. Hold vinder før fjendesøgning. Move ankommer ved 3; disciplin fastholder bevægelse, medmindre fjende inden MaximumRange og effektiv aggression − Discipline ≥12. TacticalSkill ≥55 og langt mål >1,25 × EffectiveRange vælger Column | Mission: `DefendArea`, `Hold`, `MoveToPoint`, `AttackTarget`, `AttackNearest`; status er tekst, ikke ekstra FSM. Delvis: `Doctrine`, `OfficerProfile`, `Autonomy`, `MissionConstraints`, `FieldOfficer` og `AutonomousBattleAI`. Der er ikke samme samlede missionscontroller |
| Samme, forsvar/angreb: `MakeDecision`, `EngageEnemy` | DefendAnchor begrænser lokal fremrykning, når fjenden er udenfor MaximumRange: DEF lerp 2–11, BAL 5–18, OFF 10–28 efter aggression. Stabiliser hvis moral <lerp(46,24, (0,35L+0,25I+0,40C)/100) og aggression <72. Bridge-steering har prioritet før engagement. Skill ≥58 og afstand >1,35 × effektiv giver Column | Defend → Hold/Engage; waypoint → AttackNearest eller Defend efter doctrine; mistet AttackTarget → AttackNearest. Unreal har område-clamp, men FieldOfficer bruger ikke den samme anker/stabiliseringsregel. P1 for bevaret forsvarsanker; P2 for personlighedsdetaljer |
| Samme, ildafstand og beslutningstid | Foretrukken effektiv-range-faktor lerp 0,98–0,68 efter aggression × lerp 0,96–1,04 efter skill × tilfældig relativ støj; clamp 0,58–1,02. Begrænses til 0,96 × valgt ildafstand, eller 0,90 × Close ved HoldFire. Fremryk hvis >preferred+1. Forsinkelse lerp(1,55,0,52,(0,62Initiative+0,38Staff)/100) × stresspenalty × difficulty × random 0,92–1,08. Støj clamp 0,01–0,30, reduceres af erfaring/composure | `ScheduleNextDecision`, `GetDecisionNoise`. Unreal doctrine lerp 0,92–0,52 og inkluderer Caution i aggression. Autonom AI bruger additiv støj på fraktionen, 1,25 s basis og seed fra StableUnitId. FieldOfficer bruger 1,5 s × skill-faktor 1,4–0,7; ingen tilsvarende stress/difficulty i denne tick-vej. Delvis, P2 |
| `OfficerAIPrototypeManager.cs`, K/R/T: installerer fælles officerskerne, UI og difficulty | Deaktiverer `Regiment.IsAI` via backing field og konfigurerer officerscontroller. Easy/Normal/Hard fjende-reaktion 1,28/1/0,84; støj 0,18/0,10/0,055. Ingen direkte combatbonus. I/F6–F8 styrer AI/difficulty | Installation + AI ON/OFF; delvis i `AIDifficulty`, `OfficerAI`, `AITelemetry` og spiller/HUD. T's senere startup-toggle slår fjendens officer-AI fra, så initial Configure-ON er ikke sluttilstanden |
| `BattleManager.cs`, K/R/T | Regimentsregister, kampresultat efter rout, pause og simuleringshastighed; 2,2 spilminutter pr. simuleringssekund. Pointer-over-UI beskytter ordreinput | Infrastruktur, ikke målvalg. Unreal `Player/StrategyGameMode`, controller og telemetry; pause/tid er ikke en enheds-AI-portering |
| `PlayerCommander.cs`, identisk K/R/T | Selection, klik/drag-facing, waypoint-ruter og direkte attack/hold/formation; AI-mission anvendes ved delegeret styring. Ruter og input kan senere overtages af managers | Selection → ordre/rute → waypoint → afslutning; Unreal `Player/StrategyPlayerController` + `Orders/StrategyOrderComponent` + movement. Skal ses sammen med authority-scripts, P0 |

### Unreal's to lokale beslutningsveje

`AI/StrategyAutonomousBattleAIComponent::TickComponent` gælder Prussia/Austria/Enemy og ikke-spillerkontrollerede danske **Company**-enheder. Den kræver AI ON, ordre og fire control, ikke Routed/Destroyed, ingen fysisk ordreudførelse og ingen standing intent; derefter skal `Autonomy::AllowsLocalRetask` tillade handling. Nærmeste mål skal have **CurrentContact** og bestå missionens pursuit-regel. Rækkevidde mindst 10 m; foretrukken fraktion clamp 0,25–0,95 efter noise (default 0,08, officerstabilitet ×1,25–0,65). Den flytter til ildafstand/flankeslot eller udsteder eksplicit Hold med facing. Seed er hash af StableUnitId XOR `0x1864A1`.

Unreal-difficulty er Easy/Normal/Hard reaktion **1,40/1/0,75** og støj **0,15/0,08/0,03**, altså andre tal end Unity. Autonomy default er Normal; med command parent tillades local retask i denne autonome komponent **kun ved Independent**, uden parent altid. FieldOfficer bruger ikke det samme autonomy-gate. Derfor er tilstedeværelsen af begge komponenter ikke ensbetydende med at begge må udstede samme slags handling.

`AI/StrategyFieldOfficerComponent` gælder Company/Cavalry/Artillery på begge sider. `NearestEnemy` søger inden 1.500 m, udelukker neutral, egne, ikke-kampeffektive og HQ/overordnede/supply, men **kræver ikke LOS eller CurrentContact**. Infanteri falder 250 m tilbage ved resterende styrke under lerp(52 %,30 %,officerstabilitet) eller moral under lerp(30,16,stabilitet), med 45 s cooldown. Denne check ligger før `PlayerOrderUnderWay`; `SetOrder` kan dog afvise en svagere autoritet. Danske kompagnier lukker til ildafstand og drejer mod fjende ved yaw-afvigelse >45° inden 500 m; fjendens normale fremrykning overlades til autonom battle-AI. Uvaklende rytteri inden 150 m får dansk infanteri til at holde formationen.

**Vigtig forskel:** `PlayerOrderUnderWay` beskytter kun fysisk aktiv DirectPlayer-ordre; `OrderComponent` beskytter derudover stående Hold/Defend/ArtilleryFireMission mod lavere autoritet. En FieldOfficer kan derfor forsøge lokalt målvalg og skrive telemetry, selv når ordren afvises. Samme autoritet kan erstatte samme autoritet. Det er ikke en generel stack af parent mission → temporary reaction → resume.

## Kommando, reserve, flanke og ordreautoritet

| Unity-script | Formål, input og regler | FSM / Unreal-status og prioritet |
|---|---|---|
| K `PrototypeOfficerObjective09K` | Gemmer DefendArea/AttackCaptureArea, objective, radius og reservefraktion; ingen selvstændig underenhedsbevægelse | None/mission + commitment-data. `MissionAnchor`, `MissionConstraints`, parent orders er pendant; scaffold, P3 |
| K `PrototypeBrigadeCommand09L` | To regimenter pr. brigade, primary + reserve. Tænker hver 1,25 s. Sætter reserve ind ved primary rout, <72 % styrke, moral <52, cohesion <46, eller angreb uden ≥4 m fremskridt i >12 s udenfor objective radius. Danske/preussiske QA-reservefraktioner 0,45/0,35 er lagrede ønsker, ikke en generel solver | Reserve → Support → Engaged ved ≤1,20 × objective radius. Delvis: FieldOfficer har kompagniereserve, HQ parent execution mangler tilsvarende brigade-primary-progress-vurdering. P1 |
| K `PrototypeKampCommandAuthority09M2` | Manual/RegimentOfficer/BrigadeOfficer; direkte dansk kompagnikommando sætter parent manual. Synkroniserer tom pivot med levende kompagnier; udsteder offsets hver 0,70 s. CloseOrHold bruger 0,92 × trigger (fallback 0,84 × effektiv), clamp 8..MaximumRange; hold ved preferred+1,5; Move hold ved 4,5 | Autoritetsniveau + override-flags. `CommandComponent`, `OrderComponent`, `OfficerAI` er struktureret pendant. Kopiér intentionen, ikke snapping/reflection; P0 |
| K `PrototypeIndependentCompanyMovement09L5` | Frigør kompagnitransforms, konverterer regimentsselection til grupper, stopper tom pivots bevægelse uden at dræbe officers-AI | Attached → Detached, group order; porteret konceptuelt med selvstændige `StrategyCompanyUnit` actors. Ingen behov for Unity's workaround, P3 |
| K `PrototypeCompanyTacticalControl09L2` + indlejret `PrototypeCompanyTacticalEntity09L2` | Centret ejer waypointkø, facing, Hold/Line/Column og rolle. Hastighed 3,2/3,35 × parent-cohesion 0,72–1, ankomst 0,55, drej Slerp 4,5. Linje: 3 ranks, 0,52 filafstand; column 8 abreast, 0,72 rækkedybde. Synkroniserer kompagnistyrke fra parent | Moving/idle + roller Engaged/Support/Reserve/Manoeuvre/Withdraw. Unreal flytter selvstændige actors med 50 cm ankomst; individplacering er `FormationComponent`/Visual. Delvis: enhedstal og ejerskab er anderledes, P2 |
| R/T `PrototypeMajorHQ09F15` | Første fysisk Major og to-company missioner; forsvarsfront vender mod relevant fjende fra objective | Pending ordre → company slots → ankomst; erstattes af F17/F18/F27. `Units/StrategyHQUnit`, parent executor/planner, P3 for legacy |
| R/T `PrototypeMajorHQ09F16` | AI toggle + to-company DefendHere/front; første simpel Major-AI uden personlighedsfortolkning | AI ON/OFF/mission; erstattes. Unreal HQ/OfficerAI, P3 |
| R/T `PrototypeMajorBattalion09F17` | Fire-company Major overtager F15/F16; front, reserve 72 m bag, flanke 105 m, spacing 60 m, ankomst 4 m. Vedligeholder missionsliste og reserve | Mission moving/arrived/reserve + `RunAIDecision`; legacy under F18/F27. Parent planner er pendant, P3 for selve legacyfilen |
| R/T `PrototypeMajorBattalion09F18` | Attack/Defend/Withdraw/Advance/Hold/Assemble. Fire kompagnier, kendte fjender inden 700 m; spacing 60, attack standoff 68, reserve 78, flanke 112, ankomst 5, HQ 6,2 m/s. Gemmer assignments ved manuel detach; ThinkMajor og ReviewReserve adskilles | Missionsliste + manualDetached + savedAssignments; `ParentExecution`, `ParentFormationPlanner`, `OfficerAI`, `CommandDelay`. Delvis, P1; T-F27 slår dette gamle styringslag fra |
| R/T `PrototypeMajorReserveDecision09F19` | Review 0,75 s; ved 4 egne mod 1 kendt fjende kan én reserve beholdes, hvis gennemsnitlig styrkeandel ≥0,68, cohesion ≥54, overtal ≥2,65. Reserve vælges efter 70 × styrkeandel +0,30 × cohesion. Flanke kræver gennemsnit ≥0,76/cohesion ≥62, reserve ≥0,75, fjende ≤285 m attack/190 m defend og rejse ≤245 m | Reserve/Flank, ikke obligatorisk samme beslutning. Unreal FieldOfficer's reserve er en anden algoritme og kan ikke erstatte disse tests. P1 |
| T `PrototypeMajorIntentAuthority09F20` | Fastholder eksplicit spillerobjective; doctrine replanner samme mission, må ikke opfinde ny. Uden eksplicit mission tager DEF aktuel forsvarsposition; BAL/OFF frigør ThinkMajor | Explicit intent/free AI; delvis i order standing intent/mission anchor. P0: AttackHere er finite i Unreal, ikke standing intent |
| T `PrototypeMajorFormationPlanner09F22` | Objective-centreret slots, unik front, minimum samlet marchafstand; review 0,20 s, spacing 60, attack 68, reserve 78. Bevarer eksisterende taktisk flankemål | Signature → solve → assignments; delvis i parent planner, der sorterer kompagnier efter indices/ID og ikke løser minimum-distance permutation. P1 |
| T `PrototypeMajorSlotDeconfliction09F23` | Hele slotsættet løses samlet, reserveres obstacle-safe og minimum-distance fordeles. Review 0,25 s; samme 60/68/78 m | Signature/geometri → replan; Unreal har line slots + local separation, ikke samme fælles reserved-endpoint-solver. P1 |
| T `PrototypeMajorMissionCommitment09F24` | Ny Major-ordre rydder gammelt lokalintent og erstatter udførelse. Pauser `OfficerAIController.enabled`, men lader AIEnabled stå ON; terrain-router må styre waypoint. Legacy ankomst 4,5, ikke tidlig completion pga. fjendeafstand | Committed travelling → arrived → controller restored; order serial/execution-state er porteret. Explicit suspend/resume er delvis/mangler, P0 |
| T `PrototypeMajorCommandZone09F25` | AI-HQ følger i bounds: preferred standoff 165 m, relocation >80, company advance >235. InCommand ≤320, Extended ≤450, ellers Out. Reaktionstilæg 0/0,15/0,40 s; timer cap 2,5. Kun command, aldrig våbenaccuracy/range | InCommand/Extended/Out + HQ move. `CommandZone`, `CommandDelay`, `HQFollow` er pendant; tal/reaction implementation afviger, P2 |
| R/T `PrototypeManualOrderAuthority09F18` | Direkte ordre slår AI OFF og frigør kompagniet fra Major; T udfører dette ved order −1200 før input, R havde +250 | Delegated → Manual; `OrderComponent` har DirectPlayer højeste prioritet, `CommandComponent` ejer attachment. Delvis: tjek detach/reclaim, P0 |
| T `PrototypeLocalCaptainAuthority09F25` | AI ON efter manuel detach må ikke genoplive en gammel Major-assignment; kun ny parent-ordre reclaim. En aktiv manuel waypointrute bevares til slut | Manual route → local captain; new parent mission → reclaimed. T-F29B deaktiverer den gamle bro efter F27. Unreal serial beskytter arvegentagelse, men ikke hele denne semantik, P0 |
| T `PrototypeRegimentHierarchy09F27` | To Major-bataljoner; F27 er recurring mover. Company spacing 72, reserve 86, flanke 126, exact arrival 0,50 m. HQ speed 6,2, 155 m bag, flyt ved 230; command 320/450. AwaitHigherMission betyder ON uden egen defaultmission. Arrived AttackHere må ikke starte ny nearest-enemy chase | Await → assigned → travelling → arrived; local under-fire suspenderer. `StrategyHQUnit`/parent executor har decomposed orders, men ikke identisk await/resume/roles. P0/P1 |
| T `PrototypeRegimentalHQ09F28` | Mission til to Majors; battalion lateral 170, reserve/flanke 285 m; regimental HQ 7 m/s, 360 bag, relocation 470; command 800/1100. BAL attack bruger begge bataljoner som front, lokale reserver bliver hos Majors. Cascade-ON kræver frisk mission | AwaitHigherMission/mission/HQ movement. Unreal HQ parent execution + follow + officer cascade; delvis i taktisk bataljonsrollefordeling, P1 |
| T `PrototypeAttackAIHotfix09F29B` | Ved front+reserve/flanke må nærmeste bataljon være assault fremfor at blive sendt baglæns af billig permutation. Kendt range 1200, reserve/flanke 285. Fjerner gammelt 18th-waypoint og deaktiverer F25 legacy-authority | Ny mission-signatur → korrektion; relevante invariants mangler i ID-baseret UE-slotfordeling. QA-namefix er P3; rolleprioritet P1 |
| T `PrototypeRegimentalCommandHardening09F29E` | Retter centerhul i defence (half separation 84), front/reserve/flanke ud fra fremdrift (285), exact slot 0,50. Nyere kommentarer begrænser gammel hardening til 0,50–0,75 stale-visual-vindue. Stopper stale AttackTarget ved arrival | Mission correction + terminal arrival, også visuel del. Unreal arrival 0,50 er porteret; taktisk split og terminal intent kræver P0/P1 |
| T `PrototypeOfficerTacticalDoctrine09F30A` + `PrototypeOfficerIntentCapture09F30A` i samme fil | Fanger eksplicit facing ved drag ≥5; defend frontage vinkelret på facing eller nærmeste relevant fjende inden 2200. Spacing 60, reserve 86, regiment lateral 170. Attack fordeler stærkere/erfarne på vigtige frontroller, svageste egnede som ren reserve | Én korrektion pr. frisk mission, ikke recurring mover. Facing porteret; kvalitetsbaseret rolleallokering/minimum-distance mangler i parent planner. P1 |
| T `PrototypeHigherCommandHQ09F30B` | Division → Brigade → Regiment intention, behold lower physical owners. Midlertidig cavalry attachment under attack, return/reserve bagefter. Brigade follow 120 bag/+65 lateral, Division 145/−75; hastigheder 6,2/5,9. Command 1350/1850 og 2100/2850. Defend cavalry 150 bag/+45 udad | Armed/waiting → mission → temporary attachment → release; `CavalryTasking`, command parent, HQFollow og cascading AI porterer centrale dele. Delvis, P1 |
| T `PrototypeCommandAttachment09F30B` | Permanent OrganicParent adskilt fra taktisk CurrentCommandParent og attachment-type | Data, ingen bevægelses-FSM. Porteret i `Command/StrategyCommandComponent`, P2 for integration |
| T `PrototypeCavalryCommandControl09F30C` | Flytter command parent mellem division/brigade/regiment/Major A/B, OrganicParent ændres ikke; toggle AI | Command ownership, ikke rytter-bevægelse. CommandComponent + OfficerAI, porteret konceptuelt |

### Unreal's eksisterende flankemanøvre og reserve er ikke en manglende feature

`FieldOfficer::AssignFlanks` grupperer kampduelige kompagnier med samme CurrentCommandParent inden 900 m af målet. Leader-skill = (0,50Tactical +0,25Initiative +0,15Staff +0,10Aggression)/100. Under 0,35 går alle direkte; under 0,55 planlægges kun nabopladser, ellers flere. Lateral sortering giver midterkompagniet ildbase, andre flanke; kaptajn Discipline <40 har 35 % chance for at ignorere flankerollen. Planen holdes 90 s (uden plan 60 s; alene 10 s).

Ved mindst fire kompagnier beholdes fjerneste som reserve, hvis 0,40Caution +0,30Tactical −0,20Aggression >32, eller QA-flag `Strategy1864HoldReserve`. Den står 90 m bag ildbasen og frigøres ved manglende/ukampduelig base, base <75 % styrke, moral <55 eller tid >240 s. **Statisk risikofund:** planens 90 s udløb kan udløse ny `AssignFlanks` og nulstille `FlankSince`, før 240 s-grenen nås. Timeout er derfor ikke en sikker garanti for reserveindsættelse.

Flanke-vinkel = min(80°,45° × afstand fra base i sortering) × lerp(0,7,1,1,skill), så høj skill kan nå 88°. Ved skill ≥0,5 samples ruten hvert 0,05 for krydsning af ildbasens ±32° kegle og gives omgående waypoint. Flanke/reserve skal flytte hvis >20 m fra goal, selv når allerede i ildafstand. Kort movement <90 m kan holde facing. Dette er en reel eksisterende UE-manøvre, men erstatter ikke Unity's fresh-parent authority, target hysteresis eller reservation af hele destinationsfootprints.

T-F27's faktisk nyere reservevalg kræver AI ON, mindst fire aktive kompagnier, AttackHere/DefendHere og kendte fjender ≤antal aktive; bedste reserve-score =60×styrkeandel +0,25×cohesion +0,15×moral. Attack kan bruge denne som flank ved ikke-Defensive doctrine og ≤2 kendte fjender. F30A omfordeler derefter nye attack-roller efter `AssaultPower = CurrentStrength ×(0,60+0,40×Experience/100)`; rolleimportance er front1000−0,05×afstand til objective, flank500, ren reserve−1000. Dermed er F19's historiske bedste-reserve-vurdering ikke den endelige F30A-kvalitetsfordeling.

UE's `IsOpenToCharge` har en Routed-gren, men både nearest-enemy og cavalry-target-loop kræver `IsCombatEffective`, som udelukker Routed. Den flygtende-fjende-gren kan derfor ikke nås gennem det normale felt-officersmålvalg. Dette er en konkret forskel mellem kodekommentarens hensigt og de faktiske filtre.

## Lokal kampreaktion, formationer og navigation

| Unity-script | Input, regler, centrale tal og tilstande | Unreal-pendant og vurdering |
|---|---|---|
| K/R/T `PrototypeAttackDeconflictionManager` | Grupper efter fælles target; think 0,20, minimum separation 24, arrival 4. Sekundære står på slots fremfor OrderAttack mod fælles center | Legacy, deaktiveres af frontage V2. Parent planner + FieldOfficer flankering har funktionel pendant; P3 for legacy |
| K/R/T `PrototypeAttackFrontagePlannerV2` | K: sticky slots, think 0,25; arrival 4, separation 22, spacing 24; progress 0,65, stuck 3,5 s, target movement 14, switch advantage 1,18. R er tidligere nonsticky version. T er advisory, **skriver ikke fysisk destination**; think 0,35, spacing 60, separation 55, sticky floor 140; låser AttackNearest til explicit target ved kontakt og respekterer pauset controller | Acquire → stable target/slot → replan ved reel ændring. UE flankeplan har tidsbinding, men ikke samme target/progress-hysterese. P1 for stabilitet; kopier ikke den gamle sekundære writer |
| R/T `PrototypeFormationAttackLanes09F17` | R grupperer 2+ fælles attack. T kræver 2+ **aktuelt valgte AI OFF** kompagnier, lader enkeltmanual være, suspenderer target under lane travel og gendanner det ved ildkontakt/arrival. Spacing 60, standoff 28–88, T arrival 5/reassert 0,45 | Assigned → engaged; ny direct ordre annullerer. UE parent slots er ikke denne manuelle attack-target-restore. P1 |
| R/T `PrototypeGroupLineSpacing09F17` | Drag ≥4, frontage 48/spacing 60. T legaliserer og reserverer endpoints på spillerens linje og minimum-distance matcher kompagnier | Preview → committed route corrections. Delvis med parent formation slots; manuel gruppesolver P1 |
| T `PrototypeFormationSlotSafety09F23` | Shared resolver, minimum center spacing 68 m; først lateral søgning på ønsket linje, så lille depth-offset, så nødsearch. Hele footprint mod river/hard buildings/reserved slots | Pure geometry/query, ingen egen tick-FSM. `RoutePlanner` og formation slots har noget legalisering, men ikke samme batch solver. P1 |
| T `PrototypeObstacleEndpointGuard09F22` | Projekterer illegal final goal ud af Farmhouse/Barn expanded bounds før router; Line halfwidth 25, Column 3,4, pad 1,5 | Requested → corrected endpoint. Navigation obstacle/route validation er pendant; endpoint kontra travel-footprint bør kontrolleres, P1 |
| T `PrototypeAttackContact09F29G` | Kun Major AttackHere; inden valgt Close/Med/Long tager kaptajnen midlertidig authority, **uanset egen facing**. Deployment Line, face 32°/s, release først ved trigger+18. HoldFire forhindrer kontaktfangst; bridge, square, charge tager prioritet. Parent mission gemmes og gendannes; frisk higher mission vinder | Parent travel → LocalContact → resume parent. UE `Contact` er perception, FieldOfficer face/approach er ikke denne FSM. **Mangler explicit suspension/resume, P0** |
| T `PrototypeUnderFireReaction09F26` | Bekræftet volley via underFireTimer: spring ≥0,55 og frisk timer ≥6,20. Moving AI ON, gyldig attacker inden egen Long; stop/Line/face/return fire. Min reaktion 4 s; release efter stille 7,25 s. Bridge kan defer 20 s; original fire policy og mission gendannes, HoldFire respekteres | Travelling → Deferred/Reacting → Resume. UE `NotifyIncomingVolley` pauser faktisk movement med bevaret mål i 2,5 s × officerstress og ændrer morale/cohesion/UnderFire. Den mangler Unity's attacker-/range-vurdering, Line/face-return-fire og quiet/deferred FSM. **Delvis, P0** |
| T `PrototypeLateralManeuver09F29P` | Think 0,30; fysisk conflict <13, clearance 10,5, overlap step 12. Ildlane halfwidth 55, desired 60, step 18–46, speed 1,75, arrival 0,45, max 32 s, settle/cooldown 4 s. Den mindre nyttige AI-formation flytter; behold produktiv front mod samme mål og flyt bagre shooter. Manual, charge, square, bridge, local contact og underfire har undtagelser | Detect → side step → settle → cooldown. `LocalDeconfliction` flytter kun Company med movement goal væk fra nabo-centre; **ingen tilsvarende stationær ildbane-løsning, P1** |
| K/R/T `PrototypeApproachFormationPolicy09H3` | K/R: Column ved lang approach, effective+12 deploybuffer, waypoint minimum 16. T: begynd Line reform udenfor fjendens MaximumRange+35, column reentry hysteresis 24; fungerer også når Major ejer destination | March → deploy/reform; skriver kun formation. UE FormationPolicy har max(egen/fjendes long/maximum)+35, threshold 120 m, scan 0,25. Godt porteret intention, afvigende thresholds; P2 |
| R/T `PrototypeMarchColumn09F6` | Column ved destination >28 kun udenfor hostile long. Bridge er særskilt exception. T charge låser Line; deploybuffer 35. Må ikke konkurrere med parent destination | March/Line/bridge/charge. UE FormationPolicy + defile i MovementExecutor/CavalryUnit; P2 for tuning og charge-invariant |
| K `PrototypeKampCompanyMarch09M1` | Independent company path >18 → Column; deploy ved remaining ≤14, kontakt-buffer 8 eller ildtryk. Manuel F/C lock 14 s | MarchStatus + manual lock; UE FormationPolicy mangler samme timed manual lock. P2 |
| R/T `PrototypeForcedMarch09F7` | Walk 3,20/3,35; forced 4,25/4,45 med ekstra cohesion −0,22/s, bund 20; rout-run 5,20/5,40 | Walk/Forced/Routed; UE Condition har fatigue og speed multiplier, men ikke samme eksplicitte forced-march toggle i komponenten. P2 |
| R/T `PrototypeFightingWithdrawal09F10` | Line mod threat, FirePause ↔ Backstep på 10 m ved 1,70 m/s; firepause max(2,reload+0,65), arrival 0,35. Stop i valgt rangeband; Long uses out-of-range margin 15. Vandstop i stedet for automatisk baglæns bro | Controlled withdrawal → completed/cancelled. UE Withdraw er movement-order, ikke pause/skyd/backstep-cyklus. **P1** |
| R/T `PrototypeFormationMotion09F3` | Soldat-reform 2,15 m/s, turn 150°/s; stationary root-turn 16,8°/s, arrival 0,035 m /0,20°. Root-facing er fysisk påvirkning selv om filen kaldes visuel | Reform/turn/settled. UE FormationTransition + InfantryVisual og MovementExecutor (120°/s default) har pendant; forskellig tempo. P2 |
| R/T `PrototypeFireVisuals09F8` | Er også **fire-readiness-provider**, ikke kun rendering. Reform estimeres med 2,8 m/s +0,30 s; guard spørger `IsFormationFireReady`. T hides cone under charge/melee, enemy QA-cones giver ikke target knowledge | Reform timer → fire ready; UE FormationTransition/FireControl/Combat + Visual er mere eksplicit. Porteret intention; P2 |
| R/T `PrototypeFormationFireGuard09F8` | Midlertidig HoldFire i Update ved Column/ikke-fire-ready, gendanner real policy i LateUpdate | Gate/restore; UE Combat afviser Reforming, men `FireControl::CanEngageTarget` har ikke et generelt Column/movement/Charge-firegate. P0 for samme formation- og charge-invariants, ikke kun reformtimeout |
| R/T `PrototypeFireArcGuard09F7` | Beskytter ±35° visual cone mod kerne ±60°; nearest-in-range udenfor arc udløser transient HoldFire | Gate/restore; UE FireControl har 35° direkte og target sample/bearing, porteret bedre; P3 for reflection workaround |
| K/R/T `PrototypeManualRouteRecovery09H4` | AI OFF, sample 0,25; deploy 14, progress 0,45, stall 3,5 s, cooldown 4, arrival guard 5, højst 4 bypass waypoints. R/T også effective+12 enemy deployment. Undgå nyt river-crossing | Route progressing → stalled → inserted bypass → cooldown. UE RoutePlanner planlægger, men tilsvarende bounded manual-stall FSM ikke fundet. P2 |
| K/R/T `PrototypeBattlefieldNavigationManager` | Gamle local steering/static query APIs; river/bridge, sanitized goals og formation clearance. Bridge z=22, entry 11; river halfwidth 2,10 | Direct/bridge/detour; gammel aktiv steering erstattes, query-kald kan blive brugt. UE Navigation/Movement, P3 |
| K/R/T `PrototypeNavigationRecoveryManager` | Recovery inde i blocker, reinforcement/detour og path-penalty query; samme tidlige bridge-geometri | Recover/route; erstattet aktivt. UE RoutePlanner/obstacles, P3 |
| K/R/T `PrototypeBattlefieldNavigationV3` | Persistent original goal + valgt detour, narrow Column. Line clearance 9, Column 4,4, detour +3, arrival 2,4 | Direct → obstacle/bridge → restore. K aktiv via authority; R/T deaktiveret. UE route-plan adskilt fra order-intent er porteret princip |
| K/R/T `PrototypeBattlefieldNavigationV4` | Persistent A* 89×59 grid, celle 4, travel clearance 3,8/final line 9, point arrival 2,2, stuck replan 0,85, goal tolerance 0,80. Undgår diagonal corner-cutting | Planned path → steering index → replan/escape; **deaktiveret af authority**. UE navsystem + custom river/obstacle route er anden løsning; P3 for gammel grid |
| K/R/T `PrototypeNavigation09AHotfix` | V4 footprint-recovery: line 10,8/column 5,2, lookahead 22, clear 0,75 s, stuck 0,60, avoidance 7,5 | Narrow/avoid/recover/restore; deaktiveret. P3 |
| K/R/T `PrototypeNavigation09BTreePassThrough` | Fjerner dekorative træer/hegn fra navcollections og rydder stale state én gang | Soft/hard policy, ingen destinationsejer. Unreal `NavigationObstacle`/terrain kræver samme klassifikation, P1 for konsistens |
| K/R/T `PrototypeNavigationV3Authority` | Deaktiverer konkurrerende managers; K vælger V3, R/T river+approved static routing | Authority enforcement, ikke route-planner. UE komponentgrænser bør opretholde samme ene steering-owner; P0 |
| K `PrototypeNavigationV3DetourContinuation09I1` | Reached/degenerate waypoint ved samme Farmhouse/Barn udskiftes i **V3's state**. Trigger 2,85, minimum leg 5, cooldown 0,20, højst 10 continuation/obstacle; må ikke opfinde water crossing | Detour → continuation → V3 resume. UE skal undgå identisk no-progress-loop, men kopier ikke reflection, P2 |
| R/T `PrototypeRiverBridgeOnly09F3` | Hård å, halfwidth 2,75, eneste bridge z=22. Staging 32, entry 13, arrival 2, exit-clear 2,5. T samme-bank curved-river chord følger bank med clearance 7 og step 24 fremfor falsk crossing | Direct → staging → entry/cross/exit → original goal; T SameBankFollow. UE RiverBarrier/RoutePlanner/bridge-slot executor er pendant, samme-bank corner case P1 |
| R/T `PrototypeStaticObstacleRouting09F11` | Kun Farmhouse/Barn; compound expanded rectangles hvis overlap. Line halfwidth 25, Column 3,4, corner pad 1,25, arrival 1,35 | Original goal → corner waypoints → restore. UE NavigationObstacle/RoutePlanner; P1 for legal full footprint |
| R/T `PrototypeAttackRouteAuthority09F12` | Suspenderer forcedTarget **kun** ved påkrævet terrain detour; bevarer også nyt target mens routing aktiv. Restore attack når legal approach | Attack → routed approach → attack restored. Unreal location-order følger ikke samme dynamic target-reference. P1 |
| T `PrototypeWideBattlefieldNavigation09F29E` | Stor bane; Farmhouse/Barn hard, træ/hegn soft. Goal tolerance 1,25, detour arrival 2,5, travel 3,8, replan 0,28, stuck 1,2, margin 18. Yield til AttackContact; river tager steering senere | Direct/detour/stuck; UE route planner, P2; authority-invariant P0 |
| T `PrototypeDefenseArrivalGuard09F29L` | Defensive arrival latch ≤0,50 m, release >1,25; behold under-fire authority | Latched/Released. UE 50 cm arrival findes, tilsvarende hysteresis ikke fundet, P2 |
| T `PrototypeDefensiveStability09F29Y` | Defend slots på missionens bred; deadzone 7. Front-only reference, reserve/flanke tæller ikke i centroid; committed Facing styrer bagakse. Major rear 155. Ændrer mål/state, ikke transforms | Stable defend + HQ-settled; UE HQFollow/mission anchor delvis, P1 |
| T `PrototypeHqFollow09F29L`, `PrototypeHqDepthGuard09F29W` | Følger frontens mission-facing og beskytter HQ-dybde; må ikke bruge vandpunkt eller lade reserven trække HQ-centret tilbage | Follow/settled/guard; UE HQFollow rear 65 m, settle 1,5 m er pendant, men Unity-front-only-reference kræver kontrol, P1 |
| T `PrototypeEnemyTestToggle09F29Y` | TEST startup enemy AI OFF; senere knapper ON/OFF, beskytter UI-input | OFF/ON, scenarieoverride; ingen taktisk porteringsværdi, P3 |
| T `PrototypeRuntimeNullGuard09F30B` | Reparerer singleton/reflection før legacy kører; deaktiverer broken charge-targeting ved manglende bindings | Ready/disabled fallback, ingen movement/combat owner. Ikke relevant at kopiere til C++, P3 |

## Skydning, moral og nærkamp

| Unity-script / regel | Input, nøgleparametre og tilstande | Unreal og prioritet |
|---|---|---|
| `Regiment::FireVolley`, R/T kerne | Stillestående, reload klar, policy ≠Hold, gyldig target i arc/range. Forced target foretrækkes, ellers nearest in cone; T preussisk infanteri prioriterer dansk cavalry med LOS i samme reloadcyklus. Firing men 58 %. Infantry cap 16 hits, cavalry cap 14. Reload random 0,90–1,12; experience multiplier 1,20–0,80 | `Combat::TryFireAt` + `FireControl`: CurrentContact/LOS, formation readiness, ammunition, drill og bearing fraction. Porteret kerne, andre våbental og ingen samme hardcap. P2 kalibrering |
| `Regiment::ReceiveVolley` / `Route` | Moral −0,32×hits−shock; cohesion −0,25×hits−0,70×shock; under-fire 7 s. Rout ved zero, moral ≤17 eller ≤24 % initial styrke. Moral cap 15 ved rout; flygt 150 world-units vest/øst efter team med z random ±25. Ingen rally i kerne | `Combat::NotifyIncomingVolley`/EvaluateRout bruger moral ≤20 eller cohesion ≤10; `RoutRecovery` flygter 120 m, safety 180 m, rally ved moral 35/cohesion 30 med +0,75/+1,25/s. Unreal er udbygget, ikke identisk; P2 |
| K/R/T `PrototypeCombatTuningManager` | Runtime reflection kompenserer kerne raw quality og 58 % firing men, så tuning ikke ganges dobbelt. K: base 1,40 %, range 1,75/1/0,30, 60 rounds/man, 8 casualties/body. R/T: 12 %, 2/1,10/0,55, 60 rounds/man, 1 casualty/body. Morale/cohesion floors 0,72/0,68 | Tuning state/sliders. UE Combat default BaseHitChance 0,035, reload 18 s, ammo 1900 total/maxshots 190; Condition/FireDrill ændrer model. Overfør ønskede scenarietests, ikke legacykompensation, P2 |
| R/T `PrototypeCompanyScale09F5` | Sætter ca. 190 mands company og først ranges 200/400; tidlig execution −11500 | Initialization/scale; UE CompanyUnit + data, P2. Disse ranges bliver senere overskrevet |
| R/T `PrototypeRangeTuning09F7` og `PrototypeRangeTuning09F8` | F7 effektiv/long 80/115, F8 **70/100**, Close =half effektiv =35, cone ±35. Orders −11400/−11350 gør F8 til senere writer | Stateless profile override. UE FireControl default **40/70/100** m og ±35; Close afviger 5 m. P2 |
| K `PrototypeCompanyCombatAuthority09L4` | Undertrykker gammel parent-pivot-ild, indtil rigtig company-fire ejer kampen | Guard/suppression. UE selvstændig CompanyUnit og HQ combat-echelons løser arkitektur; P3 workaround |
| K `PrototypeKampCompanyFire09M` | Company clock; init random 0,2–1,4 s. Gyldigt levende enemy company i parent range/arc, nærmeste. Explicit player Hold-lock respekteres; ellers AI/Prussia åbner Medium fra Hold. Firing fraction 0,58, base 0,014, Dreyse ×0,013/0,014; range 1,75→1→0,30; quality floors 0,72/0,68, variance 0,72–1,28. Hits clamped til target-company strength, men `ReceiveVolley` rammer **enemy parent**. Der er ikke et generelt `IsMoving`-firegate i Update | Independent reload per company, parent shared morale/damage. UE company owns strength/morale/reload direkte; mere sammenhængende. P2 for fire policy lock, P3 for parent-damage workaround |
| K/R/T `PrototypeCombatStatusManager` | Registrerer volley via ændring i private nextFireTime, ammunition og hitfeedback. T `RegisterExternalVolley` registrerer rigtig square-face volley; 0,25 average rounds/man, ikke falsk +3600-timer | Ammo/last volley + T fractional carry; UE combat delegates/ammunition er bedre eventbaseret. P2 for face accounting |
| R/T `PrototypeLethalityCalibration09F14` | Supplerer **kun** skud der rammer legacy cap 16 til close QA-region ca. 20–30; ekstra casualty/cohesion, ikke shock to gange | Capped volley → supplemental losses. UE har egen resolver; **ikke værd at portere**, P3 |
| K/R/T `PrototypeMeleeCombatManager` | Pulser 1,25 s, almindelig kontakt 7 m eller collider; stop begge og undertryk musketsalver. T charge må komme til 2,2 før stop. Damage = (2,2+4,2×clamp(attacker strength/600,0,25,1,20)×quality/lerp(0,72,1,12,defender cohesion)) × sector × charge × random(0,72,1,28), clamp 1–12. Quality 0,35Morale+0,35Cohesion+0,30Experience. Sector flank 1,18/rear 1,35; momentum 1,22 (frontal steady line cohesion≥70/moral≥60 reducerer ×0,86). Moral −1,8−0,28×loss, cohesion −1,4−0,22×loss. Rout zero/moral≤17/strength≤24 % | Contact → repeated melee → rout/separation. UE FieldOfficer har **single shock**, ikke tilsvarende parvis pulsed melee. P1 |
| T `PrototypeInfantryCharge09F25` | Explicit charge ejer approach, HoldFire+Line invariant også på bridge. Steer 0,28 s, speed 4,8/5,0, ekstra cohesion −0,30/s bund20, momentum 2,6 s; physical deep contact ≤2,2. Giver melee manager udfaldet og restore ved cancel | Target-pick → charging → contact/melee → finish/cancel. UE FieldOfficer charge speed ×1,9, kontaktdistance 18 m, chase update når mål flyttet >15 m. Differentieret single-shock-model; P1 |
| T `PrototypeChargeTargeting09F29K` | Kontekstbestemt enemy target picking til explicit charge; runtime guard kan deaktivere ved brudte reflectionbindings | Pending target/committed/cancelled; UE player Charge er location-order og FieldOfficer's own ChargeTarget gemmes kun ved `StartCharge`. P0/P1: ikke antage at manual Charge automatisk får samme infantry shock-FSM |

UE `FieldOfficer::ResolveShock` vægter strength × morale/100 ×(0,7+cohesion/300), square ×1,25; ratio clamp 0,4–2,5. Enemy casualties = own strength ×random 5–10 %×ratio, own = enemy strength ×random 3–7 %/ratio. Enemy moral −20×ratio/cohesion −25×ratio. Ratio >1,15 eller enemy moral <30 giver rout +300 m Withdraw og own moral +8; ellers own moral/cohesion −15. Charge slutter i Hold, bajonet beholdes 30 s, cooldown 30 s. **Dette er væsentligt forskelligt fra Unity's langvarige kontaktkamp**, selv om begge kaldes bajonetangreb.

## Kavaleri, afsiddede dragoner og carré

| Unity-script | Input, regler og nøgleparametre / FSM | Unreal-status |
|---|---|---|
| K `PrototypeSpecialArms09K` + `PrototypeSpecialArmIdentity09L` i samme fil | **Visuel/datapilot**, ikke kamp-AI: DK 135 cavalry-personel/120 heste, 8 guns/190 crew; PR QA 160/155 og 6 guns/120. Logger CombatIntegration=Future, Limber=Future, FireModel=Future. MountedEffective=min(personnel,horses) | Ikke en gammel artilleri-AI at portere. `CavalryUnit`/`ArtilleryBatteryUnit` går funktionelt videre. P3 |
| T `PrototypeCavalryUnit09F30` | Kind Gardehusar/Dragon, formation Line/Column, mode Mounted/Dismounted, action Hold/Move/Charge/Falter. Move 8,2 line/9,4 column, bridge 5,6, foot 3,15 m/s; charge 13,2 (4,4 mens reform). Arrival/bridge/reform/remount styres særskilt. AutoColumn ved >140, Line ≤90, manual override; bridge smal 2 abreast først nær 36 m, normal column 4, line 4 ranks | `Units/CavalryUnit`, Dragoon, CavalryCharge, FormationTransition, MountedAnimationSync og CavalryVisual er delvis pendant. UE mounted 9/charge11/foot4,5; manual/bridge invariants og reform-driven start er forskellige. P2 |
| Samme: mounted charge resolve | Kontakt 7,5. Ready square: **0 hits**, own moral −12/cohesion −18, retreat 38 m, Falter. Front/flank/rear rate 0,025/0,045/0,065 og shock 6/10/15; forming square ×1,25. Styrkefaktor 0,55–1, random 0,82–1,18, hits clamp 1–18; own cohesion cost front9/ellers5, bund25 | UE CavalryCharge sweeper 2,5 m radius, fixed 2 casualties, −5 morale/−8 cohesion på target; **ingen ready-square rejection eller sector damage i resolver**. FieldOfficer undgår Square i *valg*, men direkte charge/kollision er stadig ubeskyttet af denne Unity-regel. **P1** |
| Samme: dismount/remount | Dragon hestepark og hver fjerde hesteholder bliver ved anchor; 75 % mobil combat group 18 m frem i 2 ranks. RequestRemount → return til horses → gather ved 3 m → Mounted; preserve world visuals mod snap | UE Dragoon har HorseParkAnchor, 0,75 combat fraction og ReturningToMounts, remount tolerance 1 m; der er ikke samme 18 m root-recenter/transformation. P2 |
| T `PrototypeCavalryOfficerAI09F30C` | Think 1,15, search 1200. Flank point 145 side/50 rear bias, rear 145 back/70 side; rear cost ×0,88, vælges hvis ≤1,18×flank cost. Arrival 24, target replan move42, timeout26 s. Fjendebubble 105, detour138. Stand-off under115 uden opportunity →145+tangent28. Charge commit ≤155 **og** flank/rear (dot-grænser ±0,55) **og** friendly infantry fixing target eller target cohesion≤62/moral≤58. Ready square → Hold. Direkte playerordre deaktiverer lokal AI; højere mission/await går først | SEEK → MANOEUVRE/DETOUR → SCREEN → CHARGE → FALTER/RECOVER; højere ordre før opportunisme. UE ScreenAI søger LOS inden300 og står120 foran parent; FieldOfficer charger open target inden400–700, moral>55/cooldown45. **Mangler Unity opportunity + enemy-relative flank FSM, P1** |
| T `PrototypeCavalrySquareThreat09F30` | Finder truet enemy infantry, committed target prioriteres, ellers nærmeste valid inden220. Sender distance, closing speed, relative cavalry strength og charging til Square API | Threat bridge, ikke eget square-state. UE ThreatReaction reagerer direkte på visible cavalry inden120. Delvis |
| T `PrototypeInfantrySquare09F29` | Automatic score = (charging?38:10)+34×proximity+2,1×clamp(speed,0,12)+9×clamp(relative strength,0,25,2); range≤220 og score≥62, kræver officer AI ON. Form tid lerp(8,4,5,2,cohesion/100); tidlig accuracy ×0,42. Manual square slår AI fra; manual move kan forlade den. Auto release efter7,5 uden trussel; parent mission bevares | NotSquare → Forming → Ready → released/routed. UE ThreatReaction: visible cavalry≤120 → Square uden score og uden `bOfficerAIEnabled`-check; release8. FormationTransition giver reform, men score, manual-AI-semantik og mission restore afviger. **P0/P1** |
| T `PrototypeSquareFireSmoke09F29Z` | Firepower 0,25 pr. face, **fire uafhængige reload clocks** og retningsbestemt smoke kun fra affyret side. Deaktiverer ældre F29V reflected FireVolley, beholder sector-visuals/locked heading | 4 × ready/reload; UE Combat har én `ReloadRemainingSeconds` og én valgt target pr. unit. Square sample/firefront findes, men parallelle face clocks mangler. P1 |
| T `PrototypeSquareOobHardening09F29X` | Suppresser legacy Line fire uden at ændre vedvarende firepolicy, neutraliserer +3600 timer-sentinel, videresender ægte square volleys til feedback; skjuler gammel Line footprint, bevarer command selection | Gate/restore, external volley + selection grace0,30. UE event pipeline er bedre; undgå duplicate square fire når face clocks indføres. P0 invariant/P3 reflection |
| T `PrototypeDismountedDragonFire09F30J` | Mobile 75 % group må være Dismounted/ikke-moving/ikke-reforming og have ammo. Ranges35/70/100, ±35°, reload7×random0,90–1,12, 20 rounds/man. Hit-rate lerp0,040→0,012 over long range, variation0,78–1,22; shock3,4→1; nearest valid **Prussian** infantry | Hold/move/reform suppress → acquire → fire/reload/out-of-ammo. UE Dragoon ændrer MaxShots, men generic Combat reload18/ranges og ammo er ikke samme karabinprofil; FieldOfficer cavalry-vurdering har ikke Dragoon-mounted-state-gate. P1/P2 |
| T `PrototypeCavalryManager09F30` | Spawner/samler Gardehusar/Dragon, selection, direkte formation/move/hold/charge/dismount/remount, HUD og startupdata | Player order/selection, ikke shared autonome decisions. UE playercontroller + CavalryUnit/specialcomponents, delvis UI-port |

## Artilleri: hvad Unity faktisk havde

Der er ingen selvstændig artilleri-targeting-, firing-, morale- eller limber-AI i de læsbare Unity-Assets. K's `PrototypeSpecialArms09K` opretter kanoner/crew og identitetsdata, men melder eksplicit at combat/limber/fire er fremtidigt. R/T's scriptinventar har heller ingen læsbar artillericontroller. Påstanden om en fuld gammel artillerikamp-AI kan derfor **ikke underbygges** af disse kilder.

Unreal har derimod `Artillery/StrategyArtilleryBatteryUnit`, Deployment, Traverse, Positioning, Ammunition, Damage/Capture/Repair, FireMission, crew animation og projectile presentation. `FieldOfficer::ThinkArtillery` slår AutoTarget til én gang og skriver Kardæsk-telemetry under400 m. `FireMission::SetAutoTargetEnabled(true)` sætter også `bAutoSelectAmmunition=true`, selv om feltets default er false, og rydder manual unit/area target. FireMission har manual unit/area, auto-target, target priority, observer checks, ammunition/range-gates, reservefraktion0,20, salvo/time limits og eval0,20 s. Dette er Unreal-udbygning, ikke manglende Unity-port.

P1 er at kontrollere hvordan officers-auto-target møder et allerede eksplicit manuel fire mission/hold; `ThinkArtillery` undersøger ikke `PlayerOrderUnderWay` før første AutoTarget-toggle. P2 er indstilling af auto-ammo og historiske battery-data. Der er ingen begrundelse for at erstatte denne kode med Unity's visuelle scaffold.

Yderligere beslutningsforskel: `Combat::TickComponent` drejer ikke-spillerkontrollerede enheder direkte mod nærmeste globale fjende før fire-cone-evaluering. Den søgning kræver ikke CurrentContact, selv om selve skuddet bagefter gør. Den kan konkurrere med fysisk facing/formation-turn og square locked heading. Dette bør med i P0-kontrollen af knowledge og én facing-ejer.

## Visuel adfærd kontra kampautoritet

| Unity-filer | Adfærd/input og state | Unreal-pendant / vurdering |
|---|---|---|
| K `PrototypeCompanyRenderer09L2`, `PrototypeFullScaleRenderer09K`, `PrototypeCompanyRendererAuthority09L2`, `PrototypeCompanyGrounding09L2` | Company-anchored/GPU soldater, surviving strength → antal; authority fjerner duplikeret regimentsrenderer, grounding sampler terrain. Ikke target AI | `Visual/StrategyInfantryVisualComponent`, crowd/formation bounds og terrain placement. Ingen per-soldier tactical AI at kopiere |
| K `PrototypeSoldierVisualPass09H`, `PrototypeSoldierVisualPass09I`, `PrototypeKampSoldierVisual09M`, `PrototypeReloadAnimation09J` | Poses følger movement/formation/real volley/reload. ReloadAnimation er udtrykkelig presentation-only; muskellader og Dreyse har forskellige sekvenser. Ny company pipeline læser company volley windows | `HumanAnimationState`, `InfantryVisual`, `EquipmentVisual`, `AnimationManifestLibrary`; eventstyret volley/reload eksisterer, P2 assets/tempo |
| R/T `PrototypeBattleVisuals09F4`, `09F5`, `09F7`, `PrototypeFireVisuals09F8` | Soldiers/selection/fire-smoke fra styrke/nextFireTime; F8 er også gameplay readiness (beskrevet ovenfor) | InfantryVisual + Combat event og FormationTransition. Behold opdeling af readiness og rendering |
| T `PrototypeCavalryAnimation09F30I`, `PrototypeCavalryVisualOneToOne09F30E`, `PrototypeCavalryVisualPolish09F30D`, `09F30H`, `PrototypeCavalryVisualFidelity09F30L` | Gait/poses ved Mounted/Dismounted/Move/Charge/Falter, 1:1 ryttere, ranks og horse-holder park. Visuelle deltilstande, ikke enemy acquisition | CavalryVisual, HorseAnimationState, HumanAnimationState, MountedAnimationSync; funktionel pendant, P2 |
| T `PrototypeSquareVisual09F29L`, `PrototypeSquareFireSmoke09F29Z` | Carré footprint/ranks/faces; Z ejer **rigtig** fire resolution, ikke kun smoke | FormationComponent + InfantryVisual + FireControl viser Square; fire-face clocks er separat gap |
| K/R/T `PrototypeCasualtyVisualManager`; K `PrototypeKampTacticalFeedback09M3`; R/T `PrototypeVolleyTelemetryAudio09F13` | Casualties → lig, volley/hits → feedback/sound. Ingen morale/target-beslutning; tuning påvirker body-ratio | InfantryVisual corpses, combat delegates, telemetry/PresentationSnapshot. P2 til præsentation |
| Flag/standard/zoom/HUD/order-preview-filer i bilaget | Kommandoveje, destinationsghosts, facing, reload/AI status og udstyrsudseende. Input-HUD kan **udstede** ordre, men er ikke ny autonom AI | Visual flag/Uniform/Equipment, UI SemanticZoom/PresentationSnapshot, player controller/HUD. Prioriter læsbarhed når ændret adfærd skal afprøves; ikke en tactical-rule-port |

## Prioriteret porteringsplan

| Prioritet | Konkret ændring værd at overføre | Eksisterende Unreal-sted / hvad der mangler | Acceptkriterium uden at kopiere prototypens hacks |
|---|---|---|---|
| **P0** | Eksplicit temporary local reaction med gemt parent mission | Orders + FieldOfficer + Combat/Contact: ingen tilsvarende AttackContact/UnderFire suspend/resume-FSM | Moving AttackHere reagerer på flankekontakt/indgående ild, respekterer HoldFire, genoptager oprindelig mission efter release; ny player/parent serial annullerer gammel reaction |
| **P0** | Samlet manuel/delegeret/standing intent/AI OFF-politik | FieldOfficer/Autonomous/ThreatReaction/OfficerAI/HQ executor må dele regler. ThreatReaction kræver i dag ikke AI ON; fresh cascade await er ikke samme Unity-FSM | AI OFF må ikke autonomt vælge Square eller mission; AI ON efter detach genopliver ikke stale parent mål; frisk parent mission reclaim er eksplicit; manual Charge får korrekt resolver |
| **P0** | Samme knowledge/mission constraints i alle lokale målvalg | Autonomous bruger CurrentContact; FieldOfficer og FormationPolicy søger globalt; FieldOfficer flank goal og Autonomous's early-flank branch går uden område-clamp | Skjult fjende giver ikke chase/flanke/charge; mål og flankewaypoints holder missionområdet; telemetry angiver hvorfor en ordre blev afvist |
| **P1** | Cavalry screen → enemy-relative flank/rear → opportunity → charge | ScreenAI + FieldOfficer + CavalryCharge. Nuværende screening er parent-relative; charge target kan vælges på column/low morale uden friendly fixing | Rytteri rider udenom infantry bubble, venter udenfor ild ved steady line, charger først ved fixing/weak target + passende aspect; playerordre vinder |
| **P1** | Ready-square rejection og aspect-/strength-baseret cavalry shock | CavalryCharge's fixed 2-hit sweep; FieldOfficer's target guard er utilstrækkelig ved manual charge | Klar Square standser cavalry og giver Falter/retreat uden target damage; forming Square er sårbar; rear/flank har dokumenteret større shock end front |
| **P1** | Parvis sustained infantry melee | FieldOfficer single-shock vs Unity pulsed resolver | Deep contact undertrykker musketild; casualties/morale løses over tid med charge momentum, aspect og defender stability; samme collision må ikke give dobbelte resolvers |
| **P1** | Stabile target/slots, kvalitetsroller og reserveprogress | ParentFormationPlanner + FieldOfficer; ID/index-fordeling og 90s reassign er enklere | Nærmest/taktisk rigtig assault fremfor baglæns assignment; front stærke/erfarne; korrekt pladsreserve og route-footprint; reserve timeout kan faktisk udløses uden reset |
| **P1** | Stationær ildbanefrigørelse | LocalDeconfliction + FieldOfficer flankering; lokal separation kører kun ved movement goal | Rear shooter eller mindre nyttig blocker tager sidetrin med facing bevaret; front som allerede skyder effektivt bliver; ingen manual-order override |
| **P1** | Controlled fighting withdrawal | MovementExecutor/Withdraw + Combat | FirePause/Backstep til valgt afstand med korrekt facing, uden at gå baglæns i vand; manuel stop annullerer |
| **P1** | Fire independent square-face clocks | Combat + FireControl + InfantryVisual | Fire sider kan skyde/reloade uafhængigt; kun face's mænd/ammo tælles; smoke på rigtig side; ikke fire fuld-company salvoer |
| **P1/P2** | Dragoon state-gates og karabinprofil | Dragoon + FieldOfficer + Combat | Dismounted/ReturningToMounts kan ikke få mounted charge; 75 % combat/25 % holders, korrekt anchor/remount og særskilt reload/ammo/ranges |
| **P2** | Kalibrering, personality/stress, forced march og bounded recovery | Doctrine/Profile/Difficulty/Condition/RoutePlanner/FormationPolicy | Scenarier sammenligner time-to-contact, cadence, rout/recovery og formationsklarhed; forklar forskelle fra Unity, ikke blind kopiering af QA-tal |
| **P3** | Undlad gamle reflection-/timer-/render-workarounds | V1/V3/V4/09A stacks, extra lethality, empty pivot suppression, visual special-arms scaffold | Ingen ny parallel destination-writer, ingen nextFireTime-sentinel og ingen duplicate unit visuals |

Disse acceptkriterier er forslag til senere validering. De er ikke kørt her, og ingen kode er ændret eller bygget som del af denne kortlægning.

## Bilag: komplet scriptinventar og kildeankre

Bilaget nedenfor dækker alle fundne filnavne, også UI/QA/scenarie/visuelle filer der ikke selv træffer taktiske beslutninger. K/R/T-angivelser fortæller hvor filen findes. SHA-256 over filbytes bruges kun til at skelne faktisk forskellige versioner; samme navn betyder ikke samme indhold. Konstanter er definitioner, ikke garanti for endelig runtimeværdi: senere managers kan overskrive dem. `Update`/`LateUpdate` og public APIs er indgange; tilstandsovergangene for taktiske scripts står i tabellerne ovenfor. Rent visuelle/UI-filer har ingen særskilt kampbeslutnings-FSM.


### Filoversigt

| Script | Gren / byte-hash | Formål og afgrænsning |
|---|---|---|
| `BattleManager.cs` | K: `461fef9d0201`, R: `78e67978c0b9`, T: `de663c9a41d0` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `OfficerAIController.cs` | K: `5bf22d32baf3`, R: `5aa5e1622884`, T: `5aa5e1622884` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `OfficerAIPrototypeManager.cs` | K: `4f16d0c954f9`, R: `44e989fd4daf`, T: `f53b911a07e4` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `OfficerProfile.cs` | K: `bb44b6adac68`, R: `bb44b6adac68`, T: `bb44b6adac68` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PlayerCommander.cs` | K: `6149f0d1dc9f`, R: `6149f0d1dc9f`, T: `6149f0d1dc9f` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeApproachFormationPolicy09H3.cs` | K: `f093ea3271ef`, R: `582cb7206813`, T: `19a382453be1` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeAttackAIHotfix09F29B.cs` | T: `28640dc34b00` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeAttackContact09F29G.cs` | T: `89439e38bf59` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeAttackDeconflictionManager.cs` | K: `3add8e8ba533`, R: `3add8e8ba533`, T: `3add8e8ba533` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeAttackFrontagePlannerV2.cs` | K: `858e1b004b26`, R: `cf69788251ac`, T: `a9055c9fc7e5` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeAttackRouteAuthority09F12.cs` | R: `0f658877128b`, T: `0f658877128b` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattleVisualFix09H.cs` | K: `9be4e984570c` | Visuelle rettelser/oprensning; ingen autonom målbeslutning. |
| `PrototypeBattleVisuals09F4.cs` | R: `bc48257d5c7a`, T: `bc48257d5c7a` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattleVisuals09F5.cs` | R: `1f39ee9a8d54`, T: `1f39ee9a8d54` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattleVisuals09F7.cs` | R: `a4edb9fce420`, T: `a4edb9fce420` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattlefieldNavigationManager.cs` | K: `4e76baa9484b`, R: `9c49ca5d7e68`, T: `9c49ca5d7e68` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattlefieldNavigationV3.cs` | K: `bf4114948840`, R: `5078b44cee44`, T: `5078b44cee44` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattlefieldNavigationV4.cs` | K: `22812858100d`, R: `22812858100d`, T: `22812858100d` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBattlefieldVisualPass09G.cs` | K: `326db215316f` | Terræn-/battlefield-look ud fra scenariegeometri; ingen taktisk FSM. |
| `PrototypeBootstrap.cs` | K: `d2986033a9b9`, R: `913cc08bdaf6`, T: `56dc9d1fcac0` | Scenarieoprettelse, terrain sampling og materials; spawn/input til AI, ikke recurring unit-AI. |
| `PrototypeBottomCommandBar09F2.cs` | R: `eee60ae120b6`, T: `225960c10935` | Manuel kommando-UI → selection/ordre; ingen autonome valg. |
| `PrototypeBoxSelection09H.cs` | K: `19b1adc2defa`, R: `8e914663c25f`, T: `579355ce8380` | Mouse-drag/klik → company/regiment-selection; ingen autonom kampbeslutning. |
| `PrototypeBridgeVisual09F3.cs` | R: `c6f57e67f5f5`, T: `c6f57e67f5f5` | Terrain/bridge-data → bropræsentation; ikke crossing-authority. |
| `PrototypeBrigadeCommand09L.cs` | K: `1dc5bb061d70` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeBuildVersionOverlay.cs` | K: `1538f3ff7181`, R: `b6339c2972fa`, T: `31b9518369cc` | Viser versionsdata; ingen enhedsadfærd. |
| `PrototypeCameraNavigation09F29P.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeCasualtyVisualManager.cs` | K: `c8ff76dbbbe0`, R: `c8ff76dbbbe0`, T: `c8ff76dbbbe0` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryAnimation09F30I.cs` | T: `ba32a9dfcfe5` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryCommandControl09F30C.cs` | T: `1650f9d9ec53` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryManager09F30.cs` | T: `836fe0b15050` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryOfficerAI09F30C.cs` | T: `a63ef0ccf8ee` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryOob09F30A.cs` | T: `c11bfe4154fd` | Cavalry status → OOB rows; enkeltklik selection/dobbeltklik camera-follow, threshold0,34. Ingen target-AI. |
| `PrototypeCavalryOrderVisuals09F30I.cs` | T: `a9efbcabd607` | Mounted/dismounted destinationsfootprint, facing og route → ghosts/line; ingen motion owner. |
| `PrototypeCavalrySquareThreat09F30.cs` | T: `588536a2cb41` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryUnit09F30.cs` | T: `843b809e6a3b` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryVisualFidelity09F30L.cs` | T: `29136de86c15` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryVisualOneToOne09F30E.cs` | T: `9d7392cb9128` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryVisualPolish09F30D.cs` | T: `a5667cfd23ef` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCavalryVisualPolish09F30H.cs` | T: `36fa19eb1fe6` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeChargeTargeting09F29K.cs` | T: `b48e2023d5d6` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCombatQa09F9.cs` | R: `a1cf3ab96f42`, T: `cdbd88c60a36` | Combat/route diagnostics → QA-log; ikke selvstændig officer-plan. |
| `PrototypeCombatStatusManager.cs` | K: `4319cbde902f`, R: `4319cbde902f`, T: `53317998ea7f` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCombatTuningManager.cs` | K: `38f970049693`, R: `bcc6a0f6709c`, T: `bcc6a0f6709c` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCommandAttachment09F30B.cs` | T: `db6b47f491df` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCommandChainVisual09F29G.cs` | T: `2ca043cdf46b` | HQ/company command-tree → terrain links; ingen ændring af command parent. |
| `PrototypeCommandHud09F29C.cs` | T: `2571393f02ac` | Selected command/execution-state → HUD og manual missionknapper; ingen egen autonome regler. |
| `PrototypeCommandSquareHardening09F30A.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeCommandTreeVisibility09F30B.cs` | T: `31578f4501d3` | Enhver dansk selection viser fuldt hierarchy/support-tree; rendering-state, ikke inheritance. |
| `PrototypeCommandTreeVisibility09F30C.cs` | T: `ae155718efc5` | Senere final tree-renderer, support links fra live CurrentCommandParent; ingen motion/AI. |
| `PrototypeCompanyCombatAuthority09L4.cs` | K: `4ff2ea14ae3e` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCompanyGrounding09L2.cs` | K: `cffd72d9f8fb` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCompanyGuidons09L4.cs` | K: `da1b1d8e966d` | Company identity → national/regimental guidons ved eget center; ingen AI-FSM. |
| `PrototypeCompanyRenderer09L2.cs` | K: `1418c43b530c` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCompanyRendererAuthority09L2.cs` | K: `caa1bc8e8acf` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCompanyScale09F5.cs` | R: `1ef133a6c49c`, T: `ced06dd3cfd3` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCompanyScreenSelection09L4.cs` | K: `5a8a1fd55362` | Skærmselection for løsrevne kompagnier; inputautoritet, ikke autonom taktik. |
| `PrototypeCompanyTacticalControl09L2.cs` | K: `dee41db3f404` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeCoordinatedAttack09F29E.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeCropFieldCombat09F29R.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeCropFieldMapOverlay09F29R.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeCropFieldTerrain09F29R.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeDefenseArrivalGuard09F29L.cs` | T: `7551f8139e79` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeDefensiveStability09F29Y.cs` | T: `6d44504fea92` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeDestinationFacing09F13.cs` | R: `8573cefe1c03`, T: `8573cefe1c03` | Route-final-facing/formation-bounds → gul destination-chevron; visuel FSM. |
| `PrototypeDismountedDragonFire09F30J.cs` | T: `d7dbd2ae1856` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeDualStandards09L.cs` | K: `f6ae86d81a89` | Regimentsidentitet → to standards ved HQ; ingen kampbeslutning. |
| `PrototypeDualStandardsAuthority09L.cs` | K: `c085b46a20bc` | Undertrykker konkurrerende standardrendering; visual-owner, ingen movement-AI. |
| `PrototypeEnemyTestPanel09F29S.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeEnemyTestToggle09F29Y.cs` | T: `80e19c726ecd` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeEnemyThreatCones09F29O.cs` | T: `3800b32e34df` | Enemy-range/facing → synlige TEST threat cones; visning giver ikke target knowledge. |
| `PrototypeExpandedOOBManager.cs` | K: `46390a70aaaa`, R: `46390a70aaaa`, T: `46390a70aaaa` | Udvider QA-regimentsroster/spawn, leverer mål til kerne-AI; initialization-state. |
| `PrototypeF9VisualOrders09F4.cs` | R: `ebf5396e5c9f`, T: `ebf5396e5c9f` | F9 companion UI → explicit tactical-order input; afhænger af TacticalOrders09F4, ikke egen tactical AI. |
| `PrototypeFightingWithdrawal09F10.cs` | R: `e9842774f043`, T: `e9842774f043` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFireArcGuard09F7.cs` | R: `9b2f914bb2cf`, T: `9b2f914bb2cf` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFireVisuals09F8.cs` | R: `e4a4649c5f17`, T: `aa850e42d756` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFlagTextCleanup09F5.cs` | R: `771a8e93b7eb`, T: `771a8e93b7eb` | Oprydning af flagtekst; ingen unit-beslutning. |
| `PrototypeFlags09F4.cs` | R: `6af874cacbce`, T: `6af874cacbce` | Unit/team → flag visuals; ingen combat-FSM. |
| `PrototypeForcedMarch09F7.cs` | R: `85f852a17456`, T: `85f852a17456` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFormationAttackLanes09F17.cs` | R: `e9dcaf25c402`, T: `1c943c46e8bf` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFormationFireGuard09F8.cs` | R: `f5be1fcdbd63`, T: `f5be1fcdbd63` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFormationMotion09F3.cs` | R: `1a22206674e5`, T: `1a22206674e5` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFormationSlotSafety09F23.cs` | T: `d95106f6a31d` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeFullScaleOOB09K.cs` | K: `1024057c205e` | OOB med fire companies/battalion og historisk identity; sætter styrke/pose før company-styring. Allocation-data, ikke egne tactical moves. |
| `PrototypeFullScaleRenderer09K.cs` | K: `85fa48a8c285` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeGroupLineSpacing09F17.cs` | R: `3e1e4b5eed21`, T: `61247a841202` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeHierarchyVisualHotfix09F29A.cs` | T: `9efc6779e61c` | Hierarchy/destinations/selection visuals korrigeres; ingen selvstændig recurring unit-AI. |
| `PrototypeHigherCommandHQ09F30B.cs` | T: `21bd0b0c2291` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeHigherCommandOob09F30B.cs` | T: `150831fb815a` | HQ-hierarchy/status → OOB selection rows; UI/input, ikke højere tactical-plan. |
| `PrototypeHigherCommandOob09F30C.cs` | T: `415299cc9b1d` | Udbygget command-OOB/status/selection; ingen særskilt autonomous FSM. |
| `PrototypeHigherCommandOob09F30D.cs` | T: `81bedcb00a53` | Ny OOB-præsentation/navigation; ingen ny battle-unit-resolver. |
| `PrototypeHqDepthGuard09F29W.cs` | T: `aab7187ce6c8` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeHqFollow09F29L.cs` | T: `126847c8e6d2` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeIndependentCompanyMovement09L5.cs` | K: `101e09573054` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeInfantryCharge09F25.cs` | T: `905c526bb4ee` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeInfantrySquare09F29.cs` | T: `41cf654a5453` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeKampCommandAuthority09M2.cs` | K: `90eaac028c17` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeKampCommandQa09M1.cs` | K: `a3574a73edce` | QA-diagnoser for kompagnikommando; verificerer autoritet/bevægelse, ikke ny autonom plan. |
| `PrototypeKampCompanyFire09M.cs` | K: `00b9ff12f0b1` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeKampCompanyHud09M1.cs` | K: `a485ab7ce08b` | Selected company → HUD, fire/formation/authority-ordreinput; ingen autonom beslutning. |
| `PrototypeKampCompanyMarch09M1.cs` | K: `d8efca376f77` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeKampRebuildLook09V.cs` | R: `3ff1569438e5` | Rebuild-look/material/overlay-pipeline; inkluderer visning af tactical ordre, ingen autonom officer. |
| `PrototypeKampRegimentFlags09M8.cs` | K: `38a6d10ec4c6` | HQ-position/identity → standards; visual-state, ikke combat. |
| `PrototypeKampSoldierVisual09M.cs` | K: `d72ca5fc01d4` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeKampTacticalFeedback09M3.cs` | K: `ccee40e61149` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeLateralManeuver09F29P.cs` | T: `d547a95f3475` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeLethalityCalibration09F14.cs` | R: `8159de83ae10`, T: `7f536058faf8` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeLocalCaptainAuthority09F25.cs` | T: `e33a49da1b26` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorBattalion09F17.cs` | R: `80ee8815f81b`, T: `80ee8815f81b` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorBattalion09F18.cs` | R: `795f00740fbb`, T: `795f00740fbb` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorCommandZone09F25.cs` | T: `3011a11567b7` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorFormationPlanner09F22.cs` | T: `b9626d37a4d8` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorHQ09F15.cs` | R: `1f070b6dc71c`, T: `1f070b6dc71c` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorHQ09F16.cs` | R: `2d2d3fac5de1`, T: `2d2d3fac5de1` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorHudStatus09F29Y.cs` | T: `aae5d5e94847` | Major execution/condition → HUD-statustekst; ingen fysisk mission-owner. |
| `PrototypeMajorIntentAuthority09F20.cs` | T: `aea633f9579e` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorMissionCommitment09F24.cs` | T: `01191372c332` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorOrderVisuals09F18.cs` | R: `9d43d2b8535b`, T: `da30c84ce9c1` | Major mission/slot → ghost/path/facing; disabled i T-F27. Ingen egen target-plan. |
| `PrototypeMajorPointerIsolation09F18.cs` | R: `164262a7afda`, T: `164262a7afda` | HUD/world-pointer → input suppression; disabled i T-F27, beskytter mod dobbeltklik-ordrer. |
| `PrototypeMajorReserveDecision09F19.cs` | R: `841585c24513`, T: `841585c24513` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorSelectionBridge09F16.cs` | R: `d371ce8ec9ab`, T: `d371ce8ec9ab` | Major selection ↔ ældre selection/UI; input bridge, ingen autonomous mission. |
| `PrototypeMajorSelectionHandoff09F20.cs` | T: `123fb5a73058` | Efter unit/box-input må company-selection overtage fra Major; disabled i T-F27. Selection-state, P3. |
| `PrototypeMajorSlotDeconfliction09F23.cs` | T: `fdf5e51e6ae6` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMajorUi09F18.cs` | R: `67dfd13e3a84`, T: `facbf6f3b5aa` | Major selected/pending-order → knapper/ordreinput; T-F27 deaktiverer gamle renderer. |
| `PrototypeManualOrderAuthority09F18.cs` | R: `31aa69c69052`, T: `42c3cd37ce90` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeManualRouteRecovery09H4.cs` | K: `71f2df407bde`, R: `512dbaa6fbd4`, T: `512dbaa6fbd4` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMarchColumn09F6.cs` | R: `2c661d6a9609`, T: `12f5f9a520a9` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMeleeCombatManager.cs` | K: `478d4c8d518a`, R: `478d4c8d518a`, T: `ecdc841592d9` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeMoraleTest09F8.cs` | R: `bfc561813a21`, T: `bfc561813a21` | QA_BYPASS: låser alle registrerede regimenters Morale til100 både Update/LateUpdate og logger casualties. Overskriver normal morale-akkumulation; ingen taktisk FSM. |
| `PrototypeMovementStallDiagnostics09H2.cs` | K: `86d5d4fd1525` | Position/destination over tid → stall-log; diagnose, ikke ny destinationsejer. |
| `PrototypeNationalFlags09F2.cs` | R: `f206867ee54a`, T: `f206867ee54a` | National identity → flag visuals; ingen AI. |
| `PrototypeNavigation09AHotfix.cs` | K: `dddeaa651c44`, R: `d522a3631e13`, T: `d522a3631e13` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeNavigation09BTreePassThrough.cs` | K: `a34bc972149b`, R: `15a1f32f1629`, T: `15a1f32f1629` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeNavigationRecoveryManager.cs` | K: `7cafaec16397`, R: `c5f1ff95ab34`, T: `c5f1ff95ab34` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeNavigationV3Authority.cs` | K: `4b04e6469846`, R: `43a9d2a5249a`, T: `43a9d2a5249a` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeNavigationV3DetourContinuation09I1.cs` | K: `2142c4a77779` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeObstacleEndpointGuard09F22.cs` | T: `145baffcbfef` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeOfficerFacingOrder09F29G.cs` | T: `06cef8b61826` | Unified Major/Regiment/higher click-objective + drag-facing≥5; pending→anchor→commit/cancel, cancellerer konfliktende withdrawal. UE player/Order.FacingYaw, porteret intention. |
| `PrototypeOfficerObjective09K.cs` | K: `9f6f3e4c3ce7` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeOfficerTacticalDoctrine09F30A.cs` | T: `9f3e339e8eea` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeOobNavigator09F29Q.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeOobStatus09F29V.cs` | T: `744e689e8712` | Strength/condition/order → OOB-status; ingen selvstændig unit-plan. |
| `PrototypePerformanceOverlay09K.cs` | K: `ee2931f6de05` | Tællere/performance-overlay; ingen unit-AI. |
| `PrototypeProjectStartup.cs` | K: `fe15efabd1dd`, R: `fe15efabd1dd`, T: `adgang nægtet` | Editor-startup; ingen battle-unit-beslutninger. |
| `PrototypeRangeFanVisualEnhancer.cs` | K: `f7177ec8c0b8`, R: `f7177ec8c0b8`, T: `f7177ec8c0b8` | Range/formation → rangefans; præsentation, ingen target-acquisition. |
| `PrototypeRangeTuning09F7.cs` | R: `75cddfcbf738`, T: `75cddfcbf738` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeRangeTuning09F8.cs` | R: `6b50554d1f2a`, T: `6b50554d1f2a` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeReducedQABattle09L3.cs` | K: `f621bb280d35` | Efter company-installation: fjerner 5./18. Regiment, reducerer roster/selection og slår Brigade AI/old renderer fra. One-shot QA-state, se scenarieoverrides. |
| `PrototypeRegimentHierarchy09F27.cs` | T: `ed2d9671011c` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeRegimentalCommandHardening09F29E.cs` | T: `f5991352ea28` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeRegimentalHQ09F28.cs` | T: `e5244b6e6d75` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeRegimentalHud09F29F.cs` | T: `03395331eccf` | Regiment selection/order → kompakt HUD og explicit order input; ingen extra autonomous FSM. |
| `PrototypeRegimentalOrderState09F30A.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeRegimentalStandards.cs` | K: `66e51e830c4d`, R: `ec818016abdc`, T: `ec818016abdc` | Regimentsidentity → standards/bærere; ingen AI-valg. |
| `PrototypeRegimentalStandards09I.cs` | K: `52eef0b3daa6` | Detaljerede standards/identity ved regiment; visuel replacement, ingen combat-FSM. |
| `PrototypeReloadAnimation09J.cs` | K: `a8fb235dc2a4` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeRiverBridgeOnly09F3.cs` | R: `4a9d243ec6bf`, T: `3395f7e5d035` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeRiverVisual09F29G.cs` | T: `3bc4ce8a9d74` | Å/bridge-geometri → water visuals; river-router er separate owner. |
| `PrototypeRiverVisual09F29L.cs` | T: `32ca2dd1658a` | Senere river visual pass; ingen tactical crossing-beslutning. |
| `PrototypeRuntimeNullGuard09F30B.cs` | T: `84e8f7097e21` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeScenario09F2.cs` | R: `188a88586472`, T: `8ad6959ca2bf` | QA-startpositioner/roster/scenarieprofil → initial units; one-shot scenarie, ikke local tactical plan. |
| `PrototypeSelectionRange09F22.cs` | T: `b2b4d844eea5` | Selection → synlige rangefans; ingen range/target-authority. |
| `PrototypeSemanticZoom09F29D.cs` | T: `3fbfe9dfb54f` | Camera-distance/selection → NATO/unit rendering; præsentation, ikke fog/target-knowledge. |
| `PrototypeSemanticZoomPolish09F29Q.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeSemanticZoomUnified09F29V.cs` | T: `a8d5744cf591` | Samlet semantic zoom/hierarchy presentation; ingen combat-FSM. |
| `PrototypeSoldierVisualPass09H.cs` | K: `1238f7da149e` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeSoldierVisualPass09I.cs` | K: `fffe1a92b5f5` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeSpecialArms09K.cs` | K: `717dd67ac1c0` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeSquareCorrections09F29V.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeSquareFireSmoke09F29Z.cs` | T: `9034bcaca9ae` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeSquareOobHardening09F29X.cs` | T: `f8c4072b1fd4` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeSquareRuntimeHotfix09F29M.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeSquareSectorFire09F29S.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeSquareVisual09F29L.cs` | T: `7e8a895ffe56` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeStaticObstacleRouting09F11.cs` | R: `a33565b9905c`, T: `a33565b9905c` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeTacticalMapInteraction09F29T.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeTacticalMapOrderIsolation09F29T.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeTacticalMapUpgrade09F29V.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeTacticalOrders09F4.cs` | R: `9f08dde6d951`, T: `9f08dde6d951` | F9-order selection → ground Defend/Capture eller enemy Attack. Pending→travelling→arrival4,5; Column ved24, deploy14. Reassert original goal før river-steering. UE Order/MissionAnchor/Movement; delvis, P0 for authority. |
| `PrototypeTacticalVisibility09F29I.cs` | T: `67da122f21f9` | Camera/selected-unit/order/under-fire → HQ beacons, objectives/facing/alerts; TAB overlay, ikke LOS-provider. |
| `PrototypeTacticalVisualCleanup09F.cs` | K: `f16ba0853199`, R: `f16ba0853199`, T: `f16ba0853199` | Fjerner gammel tactical rendering; visual authority, ikke movement owner. |
| `PrototypeTacticalVisuals09F6.cs` | R: `74c02ae0c0b2`, T: `74c02ae0c0b2` | Formation/route → tactical selection/preview; visual-state, ikke target-AI. |
| `PrototypeTacticalVisuals09F7.cs` | R: `fb99348b2120`, T: `fb99348b2120` | Ny tactical footprint/range-visual authority; ingen egen combat-resolver. |
| `PrototypeTerrainVisualPolish09F29Y.cs` | T: `74ad15472625` | Terrænpræsentation; ingen hard/soft tactical-regel udledt af look alene. |
| `PrototypeUiTheme09F15.cs` | R: `c54f65e26acb`, T: `1f3ee7bbdf6a` | UI-styles/paletter; ingen unit-input eller autonomous FSM. |
| `PrototypeUnderFireReaction09F26.cs` | T: `fc0d4c825861` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeUnifiedCommandHud09F29G.cs` | T: `45fdf337030a` | Fælles HQ/company/cavalry UI → delegation/doctrine/order-input og execution-status; ingen separat recurring tactical algorithm. |
| `PrototypeUniformDesigner09H.cs` | K: `46cec500a1a8` | Manuelt uniform-designinput → material/appearance; ingen battle-AI. |
| `PrototypeUniformProfile09H.cs` | K: `0efd3131cc0c` | Uniformdata; ingen autonom beslutningsregel eller FSM. |
| `PrototypeUnitFootprint09F5.cs` | R: `994c9696057e`, T: `994c9696057e` | Aktuel formation/styrke → collider/selection/firefan bounds. Påvirker fysisk kontakt/selection selv om visuals; UE Formation/InfantryVisual/collider, P2. |
| `PrototypeVisualPolish09F29P.cs` | T: `adgang nægtet` | **Uverificeret: adgang nægtet.** Ingen beslutningsregler eller tal påstås. |
| `PrototypeVisualReadability09I2.cs` | K: `81f95f6ea84f` | Visuel læsbarhed/appearance; ingen unit target/ordre-AI. |
| `PrototypeVolleyTelemetryAudio09F13.cs` | R: `8689fdf23a68`, T: `8689fdf23a68` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeWideBattlefieldNavigation09F29E.cs` | T: `1df8c98d8288` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |
| `PrototypeWithdrawalButtons09F10.cs` | R: `0c1a1880a8f9`, T: `0c1a1880a8f9` | Manuelle valgte range-knapper → FightingWithdrawal API; pending/cancel, ingen nye withdrawal-regler. |
| `RTSCameraController.cs` | K: `4edcb7f4ef05`, R: `54cd8caad37a`, T: `54cd8caad37a` | Keyboard/mouse → camera pan/zoom; kamera-state, ingen unit-behaviour. |
| `Regiment.cs` | K: `2f92b3e8912e`, R: `2f92b3e8912e`, T: `fa7b8d232a7b` | Adfærds-/præsentationskomponent beskrevet i hovedtabellerne; konkrete klasser, parametre og indgange nedenfor. |

### Utilgængelige filer

Disse filer er kun inventeret ved navn. Læseforsøg gav adgang nægtet; hverken indhold, formål, aktiv status eller porteringsparitet er verificeret.

- T: `Scripts/PrototypeCameraNavigation09F29P.cs`
- T: `Scripts/PrototypeCommandSquareHardening09F30A.cs`
- T: `Scripts/PrototypeCoordinatedAttack09F29E.cs`
- T: `Scripts/PrototypeCropFieldCombat09F29R.cs`
- T: `Scripts/PrototypeCropFieldMapOverlay09F29R.cs`
- T: `Scripts/PrototypeCropFieldTerrain09F29R.cs`
- T: `Scripts/PrototypeEnemyTestPanel09F29S.cs`
- T: `Scripts/PrototypeOobNavigator09F29Q.cs`
- T: `Editor/PrototypeProjectStartup.cs`
- T: `Scripts/PrototypeRegimentalOrderState09F30A.cs`
- T: `Scripts/PrototypeSemanticZoomPolish09F29Q.cs`
- T: `Scripts/PrototypeSquareCorrections09F29V.cs`
- T: `Scripts/PrototypeSquareRuntimeHotfix09F29M.cs`
- T: `Scripts/PrototypeSquareSectorFire09F29S.cs`
- T: `Scripts/PrototypeTacticalMapInteraction09F29T.cs`
- T: `Scripts/PrototypeTacticalMapOrderIsolation09F29T.cs`
- T: `Scripts/PrototypeTacticalMapUpgrade09F29V.cs`
- T: `Scripts/PrototypeVisualPolish09F29P.cs`

### Definitioner og funktionsankre pr. faktisk version

Linjenumrene gælder de undersøgte filer. Ens byteindhold vises samlet. Parameterlisten inkluderer konstanter og simple felt-/property-defaults; interne grafikparametre er ikke kampthresholds. For filer med kun UI/visuals angiver enum/fields lokal præsentations- eller inputstate, ikke en autonome battle-FSM.


#### K `BattleManager.cs` (383 linjer; `461fef9d0201`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/BattleManager.cs).

Klasser: `BattleManager` L5.

Indgange og kildeankre: `Awake` L30; `OnDestroy` L42; `Update` L48; `Register` L82; `NotifyRout` L88; `IsPointerOverSimulationControls` L93; `GetHoveredRegiment` L116; `GetHoverInfoRect` L135; `GetUnitDisplayName` L152; `EvaluateBattleResult` L161; `SetPaused` L192; `SetSpeed` L198; `RestartBattle` L218; `GetTimeControlRect` L227; `DrawTimeControls` L275.

```csharp
L13: private const float GameMinutesPerSimulationSecond = 2.2f;
L14: private const float HoverInfoWidth = 200f;
L15: private const float HoverInfoHeight = 34f;
L18: private float battleMinutes = 10f * 60f + 20f;
L19: private float speed = 1f;
L282: const float h = 24f;
L283: const float gap = 3f;
```


#### R `BattleManager.cs` (375 linjer; `78e67978c0b9`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/BattleManager.cs).

Klasser: `BattleManager` L5.

Indgange og kildeankre: `Awake` L29; `OnDestroy` L41; `Update` L47; `Register` L81; `NotifyRout` L87; `IsPointerOverSimulationControls` L92; `GetHoveredRegiment` L110; `GetHoverInfoRect` L129; `EvaluateBattleResult` L141; `SetPaused` L172; `SetSpeed` L178; `RestartBattle` L198; `GetTimeControlRect` L206; `DrawTimeControls` L249; `GetDisplayName` L288; `GetOrderLabel` L305.

```csharp
L13: private const float GameMinutesPerSimulationSecond = 2.2f;
L14: private const float HoverInfoWidth = 300f;
L15: private const float HoverInfoHeight = 122f;
L18: private float battleMinutes = 10f * 60f + 20f;
L19: private float speed = 1f;
L208: const float width = 318f;
L209: const float height = 28f;
L256: const float h = 22f;
L257: const float gap = 2f;
L258: const float clockWidth = 136f;
L259: const float buttonWidth = 27f;
```


#### T `BattleManager.cs` (492 linjer; `de663c9a41d0`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/BattleManager.cs).

Klasser: `BattleManager` L5.

Indgange og kildeankre: `Awake` L29; `OnDestroy` L41; `Update` L47; `Register` L81; `NotifyRout` L87; `IsPointerOverSimulationControls` L92; `TryGetHoveredBattleUnit` L110; `GetHoverInfoRect` L153; `EvaluateBattleResult` L165; `SetPaused` L196; `SetSpeed` L202; `RestartBattle` L222; `GetTimeControlRect` L230; `DrawTimeControls` L273; `GetDisplayName` L312; `GetOrderLabel` L329.

```csharp
L13: private const float GameMinutesPerSimulationSecond = 2.2f;
L14: private const float HoverInfoWidth = 300f;
L15: private const float HoverInfoHeight = 122f;
L18: private float battleMinutes = 10f * 60f + 20f;
L19: private float speed = 1f;
L232: const float width = 318f;
L233: const float height = 28f;
L280: const float h = 22f;
L281: const float gap = 2f;
L282: const float clockWidth = 136f;
L283: const float buttonWidth = 27f;
```


#### K `OfficerAIController.cs` (553 linjer; `5bf22d32baf3`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/OfficerAIController.cs).

Klasser: `OfficerAIController` L19.

Tilstands-/data-enums: `OfficerAIMission` = DefendArea, Hold, MoveToPoint, AttackTarget, AttackNearest (L3); `OfficerAIDoctrine` = Defensive, Balanced, Offensive (L12).

Indgange og kildeankre: `Configure` L40; `Update` L96; `ToggleAI` L109; `SetAIEnabled` L114; `SetDoctrine` L147; `SetOrderAggressiveness` L176; `GetEffectiveAggressiveness` L185; `SetMoveMission` L205; `SetAttackMission` L220; `SetHoldMission` L237; `MakeDecision` L252; `EngageEnemy` L377; `ScheduleNextDecision` L457; `GetDecisionNoise` L479; `GetStress01` L493; `FindNearestEnemy` L501; `SetStatus` L526.

```csharp
L25: public float OrderAggressiveness { get; private set; } = 50f;
```


#### R/T `OfficerAIController.cs` (569 linjer; `5aa5e1622884`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/OfficerAIController.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/OfficerAIController.cs).

Klasser: `OfficerAIController` L19.

Tilstands-/data-enums: `OfficerAIMission` = DefendArea, Hold, MoveToPoint, AttackTarget, AttackNearest (L3); `OfficerAIDoctrine` = Defensive, Balanced, Offensive (L12).

Indgange og kildeankre: `Configure` L37; `Update` L93; `ToggleAI` L106; `SetAIEnabled` L111; `SetDoctrine` L144; `SetOrderAggressiveness` L173; `GetEffectiveAggressiveness` L182; `SetMoveMission` L202; `SetAttackMission` L217; `SetHoldMission` L234; `MakeDecision` L249; `EngageEnemy` L374; `ScheduleNextDecision` L473; `GetDecisionNoise` L495; `GetStress01` L509; `FindNearestEnemy` L517; `SetStatus` L542.

```csharp
L25: public float OrderAggressiveness { get; private set; } = 50f;
```


#### K `OfficerAIPrototypeManager.cs` (450 linjer; `4f16d0c954f9`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/OfficerAIPrototypeManager.cs).

Klasser: `OfficerAIPrototypeManager` L12; `BattleManagerOfficerAIExtensions` L439.

Tilstands-/data-enums: `OfficerAIDifficulty` = Easy, Normal, Hard (L4).

Indgange og kildeankre: `AutoCreate` L23; `Awake` L32; `OnDestroy` L43; `Update` L49; `TryInstallOfficerAI` L68; `DisableLegacyRegimentAI` L98; `GetReactionMultiplier` L124; `GetDecisionNoise` L140; `SetDifficulty` L156; `IsPointerOverControls` L162; `GetControlPanelRect` L171; `ToggleSelectedAI` L180; `SetSelectedAIEnabled` L189; `SetSelectedDoctrine` L206; `SetSelectedOrderAggressiveness` L223; `SetSelectedFirePolicy` L240; `IsSelectedDanish` L255; `GetFirstSelectedController` L262; `GetSelectedDanishCount` L281; `GetFirstSelectedRegiment` L297; `GetAIReactionMultiplier` L441; `GetAIDecisionNoise` L446.

```csharp
L175: const float height = 58f;
L353: const float gap = 3f;
L354: const float buttonHeight = 23f;
```


#### R `OfficerAIPrototypeManager.cs` (365 linjer; `44e989fd4daf`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/OfficerAIPrototypeManager.cs).

Klasser: `OfficerAIPrototypeManager` L12; `BattleManagerOfficerAIExtensions` L354.

Tilstands-/data-enums: `OfficerAIDifficulty` = Easy, Normal, Hard (L4).

Indgange og kildeankre: `AutoCreate` L26; `Awake` L35; `OnDestroy` L45; `Update` L51; `TryInstallOfficerAI` L64; `DisableLegacyRegimentAI` L92; `GetReactionMultiplier` L118; `GetDecisionNoise` L130; `SetDifficulty` L142; `IsPointerOverControls` L148; `GetControlPanelRect` L156; `ToggleSelectedAI` L163; `SetSelectedAIEnabled` L170; `SetSelectedDoctrine` L182; `SetSelectedOrderAggressiveness` L194; `SetSelectedFirePolicy` L206; `IsSelectedDanish` L216; `GetFirstSelectedController` L221; `GetSelectedDanishCount` L234; `GetFirstSelectedRegiment` L244; `DisplayUnitName` L264; `GetAIReactionMultiplier` L356; `GetAIDecisionNoise` L361.

```csharp
L159: const float height = 104f;
L299: const float leftWidth = 284f;
L315: const float gap = 5f;
L316: const float buttonHeight = 29f;
```


#### T `OfficerAIPrototypeManager.cs` (364 linjer; `f53b911a07e4`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/OfficerAIPrototypeManager.cs).

Klasser: `OfficerAIPrototypeManager` L12; `BattleManagerOfficerAIExtensions` L353.

Tilstands-/data-enums: `OfficerAIDifficulty` = Easy, Normal, Hard (L4).

Indgange og kildeankre: `AutoCreate` L27; `Awake` L36; `OnDestroy` L46; `Update` L52; `TryInstallOfficerAI` L65; `DisableLegacyRegimentAI` L93; `GetReactionMultiplier` L119; `GetDecisionNoise` L131; `SetDifficulty` L143; `IsPointerOverControls` L149; `GetControlPanelRect` L157; `ToggleSelectedAI` L162; `SetSelectedAIEnabled` L169; `SetSelectedDoctrine` L181; `SetSelectedOrderAggressiveness` L193; `SetSelectedFirePolicy` L205; `IsSelectedDanish` L215; `GetFirstSelectedController` L220; `GetSelectedDanishCount` L233; `GetFirstSelectedRegiment` L243; `DisplayUnitName` L263; `GetAIReactionMultiplier` L355; `GetAIDecisionNoise` L360.

```csharp
L24: private const float HudHeight = 90f;
L289: const float pad = 7f;
L315: const float gap = 4f;
L316: const float buttonHeight = 25f;
```


#### K/R/T `OfficerProfile.cs` (100 linjer; `bb44b6adac68`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/OfficerProfile.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/OfficerProfile.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/OfficerProfile.cs).

Klasser: `OfficerProfile` L3.

Indgange og kildeankre: `CreatePrototype` L57.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K/R/T `PlayerCommander.cs` (870 linjer; `6149f0d1dc9f`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PlayerCommander.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PlayerCommander.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PlayerCommander.cs).

Klasser: `PlayerCommander` L5; `MovementRoute` L7; `OrderGhostVisual` L16.

Indgange og kildeankre: `Awake` L44; `OnDestroy` L55; `Start` L61; `Update` L67; `RotateSelectedFacing` L131; `SetSelectedFormation` L161; `HandleSelection` L175; `BeginRightMouseOrder` L212; `UpdateRightMouseOrder` L239; `CompleteRightMouseOrder` L247; `IssueAttackOrder` L272; `IssueSingleMoveOrder` L288; `IssueCenteredGroupLineOrder` L308; `AppendGroupWaypointOrder` L332; `GetAutomaticGroupFacing` L355; `GetSelectedWorldCenter` L379; `GetSelectedInLineOrder` L393; `ReplaceRoute` L413; `AppendRouteWaypoint` L430; `UpdateRoutes` L456; `IssueCurrentRouteLeg` L512; `NormalizedFacing` L525; `CreateOrUpdateOrderGhost` L538; `CreateGhostLine` L560; `UpdateOrderGhosts` L580; `UpdateGhostPath` L603; `UpdateGhostFootprint` L642; `ClearRoute` L651; `ClearOrderGhost` L658; `GetEnemyUnderMouse` L669; `TryGetGroundPoint` L683; `EnsureFormationPreview` L700; `UpdateFormationPreview` L736; `DrawFacingGuide` L775; `ShowPreviewCenter` L788; `SetTerrainFollowingLine` L797; `DrawFootprint` L812; `SetFormationPreviewVisible` L840; `ForEachSelected` L850; `ClearSelection` L863.

```csharp
L38: private const float FormationDragThreshold = 4f;
L39: private const float RegimentLineSpacing = 22f;
L40: private const float FacingStepDegrees = 15f;
L41: private const float RouteArrivalDistance = 3.25f;
L42: private const float RegimentPreviewWidth = 19f;
L619: const int samplesPerLeg = 8;
```


#### K `PrototypeApproachFormationPolicy09H3.cs` (175 linjer; `f093ea3271ef`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeApproachFormationPolicy09H3.cs).

Klasser: `PrototypeApproachFormationPolicy09H3` L10.

Indgange og kildeankre: `AutoCreate` L22; `Awake` L31; `Update` L49; `ApplyPolicy` L64; `SetPolicyState` L121; `FindNearestEnemy` L142; `PlanarDistance` L169.

```csharp
L18: private const float MinimumWaypointColumnDistance = 16f;
L19: private const float DeploymentBuffer = 12f;
```


#### R `PrototypeApproachFormationPolicy09H3.cs` (179 linjer; `582cb7206813`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeApproachFormationPolicy09H3.cs).

Klasser: `PrototypeApproachFormationPolicy09H3` L9.

Indgange og kildeankre: `AutoCreate` L21; `Awake` L30; `Update` L48; `ApplyPolicy` L63; `DeployLine` L119; `SetPolicyState` L125; `FindNearestEnemy` L146; `PlanarDistance` L173.

```csharp
L17: private const float MinimumWaypointColumnDistance = 16f;
L18: private const float DeploymentBuffer = 12f;
```


#### T `PrototypeApproachFormationPolicy09H3.cs` (233 linjer; `19a382453be1`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeApproachFormationPolicy09H3.cs).

Klasser: `PrototypeApproachFormationPolicy09H3` L12.

Indgange og kildeankre: `AutoCreate` L29; `Awake` L38; `Update` L57; `ApplyPolicy` L72; `DeployLine` L169; `SetPolicyState` L179; `FindNearestEnemy` L200; `PlanarDistance` L227.

```csharp
L20: private const float MinimumWaypointColumnDistance = 16f;
L25: private const float EnemyLongRangeDeploymentBuffer = 35f;
L26: private const float ColumnReentryHysteresis = 24f;
```


#### T `PrototypeAttackAIHotfix09F29B.cs` (313 linjer; `28640dc34b00`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeAttackAIHotfix09F29B.cs).

Klasser: `PrototypeAttackAIHotfix09F29B` L15.

Indgange og kildeankre: `AutoCreate` L28; `Awake` L37; `Update` L50; `DisableLegacyAuthorityConflict` L74; `NeutralizeLegacyEnemyWaypoint` L97; `CorrectNewRegimentalAttackRoleAssignment` L127; `KnownEnemies` L228; `FindNearestEnemy` L250; `FindRegiment` L278; `Flat` L293; `Ground` L301; `PlanarDistance` L307.

```csharp
L23: private const float RegimentKnownEnemyRange = 1200f;
L24: private const float ReserveDepth = 285f;
L25: private const float FlankOffset = 285f;
```


#### T `PrototypeAttackContact09F29G.cs` (462 linjer; `89439e38bf59`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeAttackContact09F29G.cs).

Klasser: `PrototypeAttackContact09F29G` L14; `ContactState` L16.

Indgange og kildeankre: `AutoCreate` L41; `Awake` L48; `OnDestroy` L62; `SetPreferredFirePolicy` L68; `IsLocalContact` L77; `GetLocalContactTarget` L82; `Update` L89; `DisableCompetingCoordinator` L110; `CaptureInitialPolicies` L117; `MaintainContacts` L131; `ScanParentAttackMissions` L229; `RestoreParentMission` L338; `GetMissions` L360; `FindNearestEnemyWithinPolicy` L369; `GetPolicyRange` L395; `FaceContact` L408; `ContactBearingDegrees` L425; `ValidUnit` L438; `ValidEnemy` L443; `PlanarDistance` L449; `Add` L456.

```csharp
L28: private const float ContactReleaseBuffer = 18f;
L29: private const float ContactTurnDegreesPerSecond = 32f;
```


#### K/R/T `PrototypeAttackDeconflictionManager.cs` (342 linjer; `3add8e8ba533`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeAttackDeconflictionManager.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeAttackDeconflictionManager.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeAttackDeconflictionManager.cs).

Klasser: `PrototypeAttackDeconflictionManager` L5; `EngagementState` L7.

Indgange og kildeankre: `AutoCreate` L26; `Update` L35; `DeconflictGroup` L100; `ChooseBestSlot` L208; `BuildSlotPosition` L257; `FaceTarget` L271; `GetAttackSlotRadius` L284; `IsAttackAI` L297; `FindNearestEnemy` L310; `PlanarDistance` L336.

```csharp
L21: private const float ThinkInterval = 0.20f;
L22: private const float MinimumFriendlySeparation = 24f;
L23: private const float SlotArrivalDistance = 4.0f;
```


#### K `PrototypeAttackFrontagePlannerV2.cs` (529 linjer; `858e1b004b26`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeAttackFrontagePlannerV2.cs).

Klasser: `PrototypeAttackFrontagePlannerV2` L5; `EngagementState` L7.

Indgange og kildeankre: `AutoCreate` L41; `Update` L50; `DisableLegacyPlanner` L105; `AllocateFrontage` L121; `ChooseStableTarget` L237; `ResetForTarget` L264; `GetReplanReason` L279; `AssignSlot` L318; `TrackProgress` L334; `ChooseBestSlot` L348; `GetAttackRadius` L454; `IsAttackAI` L466; `IsValidEnemy` L479; `FindNearestEnemy` L489; `FaceTarget` L510; `PlanarDistance` L523.

```csharp
L15: public float LastDistanceToSlot = float.PositiveInfinity;
L26: private const float ThinkInterval = 0.25f;
L27: private const float SlotArrivalDistance = 4.0f;
L28: private const float MinimumFriendlySeparation = 22f;
L29: private const float LateralSlotSpacing = 24f;
L35: private const float ProgressEpsilon = 0.65f;
L36: private const float StuckReplanSeconds = 3.5f;
L37: private const float TargetMovementReplanDistance = 14f;
L38: private const float TargetSwitchAdvantage = 1.18f;
```


#### R `PrototypeAttackFrontagePlannerV2.cs` (375 linjer; `cf69788251ac`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeAttackFrontagePlannerV2.cs).

Klasser: `PrototypeAttackFrontagePlannerV2` L5; `EngagementState` L7.

Indgange og kildeankre: `AutoCreate` L28; `Update` L37; `DisableLegacyPlanner` L97; `AllocateFrontage` L113; `ChooseBestSlot` L210; `GetAttackRadius` L305; `IsAttackAI` L317; `FindNearestEnemy` L330; `FaceTarget` L356; `PlanarDistance` L369.

```csharp
L22: private const float ThinkInterval = 0.25f;
L23: private const float SlotArrivalDistance = 4.0f;
L24: private const float MinimumFriendlySeparation = 22f;
L25: private const float LateralSlotSpacing = 24f;
```


#### T `PrototypeAttackFrontagePlannerV2.cs` (471 linjer; `a9055c9fc7e5`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeAttackFrontagePlannerV2.cs).

Klasser: `PrototypeAttackFrontagePlannerV2` L12; `EngagementState` L14.

Indgange og kildeankre: `AutoCreate` L37; `Awake` L46; `Update` L64; `DisableLegacyPlanner` L139; `IsPlannerEligible` L156; `ResolveTarget` L179; `GetOrCreate` L213; `AllocateAdvisoryFrontage` L223; `ChooseBestSlot` L314; `GetAttackRadius` L388; `GetEngagementThreshold` L400; `FindNearestEnemy` L407; `IsValidEnemy` L428; `CleanupStates` L438; `PlanarDistance` L465.

```csharp
L31: private const float ThinkInterval = 0.35f;
L32: private const float MinimumFriendlySeparation = 55f;
L33: private const float LateralSlotSpacing = 60f;
L34: private const float StickyContactFloor = 140f;
```


#### R/T `PrototypeAttackRouteAuthority09F12.cs` (214 linjer; `0f658877128b`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeAttackRouteAuthority09F12.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeAttackRouteAuthority09F12.cs).

Klasser: `PrototypeAttackRouteAuthority09F12` L11; `AttackRouteState` L13.

Indgange og kildeankre: `AutoCreate` L28; `Awake` L37; `Update` L56; `ApplyRouteOwnership` L154; `GetOrCreate` L181; `RestoreAndClear` L192; `AddStale` L208.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeBattleVisualFix09H.cs` (166 linjer; `9be4e984570c`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBattleVisualFix09H.cs).

Klasser: `PrototypeBattleVisualFix09H` L7.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24; `EnsureSmokeMaterial` L64; `TuneSmoke` L94; `CreateSoftSmokeTexture` L132.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeBattleVisuals09F4.cs` (601 linjer; `bc48257d5c7a`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBattleVisuals09F4.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBattleVisuals09F4.cs).

Klasser: `PrototypeBattleVisuals09F4` L10; `FallenRecord` L12; `RegimentVisualState` L18.

Indgange og kildeankre: `AutoCreate` L62; `Awake` L71; `OnDestroy` L88; `CycleSoldierDisplayRatio` L94; `CycleCasualtyDisplayRatio` L99; `SetSoldierDisplayRatio` L104; `SetCasualtyDisplayRatio` L110; `GetVisibleLivingCount` L116; `GetVisibleFallenCount` L124; `NormalizeRatio` L135; `GetNextRatio` L150; `Update` L161; `LateUpdate` L193; `InitializeState` L211; `RecordNewCasualties` L230; `CreateFallenRecord` L252; `UpdateLivingFormation` L272; `GetRepresentedActualIndex` L325; `GetFormationPosition` L334; `BuildLivingRootMatrices` L360; `BuildFallenRootMatrices` L374; `DrawLiving` L388; `DrawFallen` L426; `Part` L446; `DrawPart` L451; `HideLegacyRepresentativeSoldiers` L481; `DisableLegacyCasualtyRenderer` L494; `CreatePrimitiveMeshes` L502; `GetPrimitiveMesh` L509; `CreateMaterials` L520; `CreateInstancedMaterial` L536; `Cleanup` L553; `StableHash` L576; `Hash01` L590.

```csharp
L32: public static int SoldierDisplayRatio { get; private set; } = 1;
L33: public static int CasualtyDisplayRatio { get; private set; } = 1;
L59: private const float FormationMoveSpeed = 3.20f;
L342: const int ranks = 6;
L351: const int columnWidth = 8;
```


#### R/T `PrototypeBattleVisuals09F5.cs` (451 linjer; `1f39ee9a8d54`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBattleVisuals09F5.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBattleVisuals09F5.cs).

Klasser: `PrototypeBattleVisuals09F5` L8; `FallenRecord` L10; `State` L16.

Indgange og kildeankre: `AutoCreate` L45; `Awake` L54; `Update` L63; `LateUpdate` L99; `VisibleLiving` L120; `DisableOlderVisualLayers` L127; `HideLegacySoldiers` L138; `RecordCasualties` L148; `CreateFallen` L165; `RebuildPositions` L184; `UpdateFormation` L197; `GetActualIndex` L230; `GetFormationPosition` L237; `GetLineWidth` L259; `GetFootprint` L265; `BuildLivingRoots` L284; `BuildFallenRoots` L295; `DrawLiving` L305; `DrawFallen` L336; `Part` L351; `Draw` L356; `GetPrimitiveMesh` L374; `CreateMaterials` L384; `Mat` L400; `StableHash` L408; `Hash01` L420; `Cleanup` L434.

```csharp
L36: public const int LineRanks = 3;
L37: public const float LineSpacingX = 0.75f;
L38: public const float RankSpacingZ = 0.90f;
L39: public const int ColumnWidth = 6;
L40: public const float ColumnSpacingX = 0.75f;
L41: public const float ColumnSpacingZ = 0.82f;
L42: private const float ReformSpeed = 2.8f;
```


#### R/T `PrototypeBattleVisuals09F7.cs` (738 linjer; `a4edb9fce420`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBattleVisuals09F7.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBattleVisuals09F7.cs).

Klasser: `PrototypeBattleVisuals09F7` L10; `FallenRecord` L23; `State` L29; `FormationBounds` L460.

Indgange og kildeankre: `AutoCreate` L61; `Awake` L70; `Update` L87; `LateUpdate` L129; `VisibleLiving` L151; `ReadNextFireTime` L158; `IsMoving` L166; `UpdateVolleyDetection` L180; `GetPhase` L194; `HideLegacySoldiersAndSmoke` L222; `RecordCasualties` L242; `RebuildPositions` L286; `UpdateFormation` L300; `GetActualIndex` L347; `BuildLivingRoots` L354; `BuildFallenRoots` L378; `CreateSmoke` L388; `UpdateSmokeShape` L436; `EmitFrontSmoke` L450; `CalculateFormationBounds` L467; `DrawLiving` L490; `DrawWeaponAndArms` L524; `DrawFallen` L563; `GetMovementFrequency` L578; `GetMovementBob` L593; `GetMovementLean` L608; `Part` L621; `Draw` L626; `GetPrimitiveMesh` L655; `CreateMaterials` L665; `Mat` L681; `StableHash` L689; `Hash01` L701; `Cleanup` L715.

```csharp
L35: public float VolleyStartedAt = -100f;
L54: private const float ReformSpeed = 2.8f;
L55: private const float FirePoseSeconds = 0.18f;
L56: private const float AimLeadSeconds = 0.70f;
L57: private const float ReloadFraction = 0.62f;
L58: private const float CasualtyJitter = 0.30f;
```


#### K `PrototypeBattlefieldNavigationManager.cs` (555 linjer; `4e76baa9484b`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBattlefieldNavigationManager.cs).

Klasser: `PrototypeBattlefieldNavigationManager` L7; `Obstacle` L9; `NavigationState` L16.

Indgange og kildeankre: `AutoCreate` L47; `Awake` L56; `OnDestroy` L75; `Update` L81; `TryInstall` L104; `RefreshStaticObstacles` L119; `AddObstacle` L147; `CreateBridgeVisual` L158; `ApplyNavigation` L178; `RestoreFormationAfterBridge` L257; `GetSteeringTarget` L270; `GetBridgeSteeringTarget` L319; `TryFindNearestValidDestination` L344; `IsBlockedDestination` L392; `IsPointNavigable` L400; `FindFirstBlockingObstacle` L417; `DistancePointToSegment` L438; `GetClearance` L457; `StreamCenterX` L462; `BankSide` L467; `IsBridgeZone` L472; `IsRiverWater` L479; `SegmentCrossesRiver` L485; `SanitizePlayerRoutes` L499; `PlanarDistance` L549.

```csharp
L38: private const float RiverHalfWidth = 2.10f;
L39: private const float BridgeZ = 22.0f;
L40: private const float BridgeHalfLengthX = 8.0f;
L41: private const float BridgeHalfWidthZ = 4.0f;
L42: private const float BridgeApproachOffset = 11.0f;
L43: private const float BattlefieldHalfWidth = 176f;
L44: private const float BattlefieldHalfDepth = 116f;
L487: const int samples = 32;
```


#### R/T `PrototypeBattlefieldNavigationManager.cs` (555 linjer; `9c49ca5d7e68`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBattlefieldNavigationManager.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBattlefieldNavigationManager.cs).

Klasser: `PrototypeBattlefieldNavigationManager` L7; `Obstacle` L9; `NavigationState` L16.

Indgange og kildeankre: `AutoCreate` L47; `Awake` L56; `OnDestroy` L75; `Update` L81; `TryInstall` L104; `RefreshStaticObstacles` L119; `AddObstacle` L147; `CreateBridgeVisual` L158; `ApplyNavigation` L178; `RestoreFormationAfterBridge` L257; `GetSteeringTarget` L270; `GetBridgeSteeringTarget` L319; `TryFindNearestValidDestination` L344; `IsBlockedDestination` L392; `IsPointNavigable` L400; `FindFirstBlockingObstacle` L417; `DistancePointToSegment` L438; `GetClearance` L457; `StreamCenterX` L462; `BankSide` L467; `IsBridgeZone` L472; `IsRiverWater` L479; `SegmentCrossesRiver` L485; `SanitizePlayerRoutes` L499; `PlanarDistance` L549.

```csharp
L38: private const float RiverHalfWidth = 2.10f;
L39: private const float BridgeZ = 22.0f;
L40: private const float BridgeHalfLengthX = 8.0f;
L41: private const float BridgeHalfWidthZ = 4.0f;
L42: private const float BridgeApproachOffset = 11.0f;
L43: private const float BattlefieldHalfWidth = 176f;
L44: private const float BattlefieldHalfDepth = 116f;
L487: const int samples = 32;
```


#### K `PrototypeBattlefieldNavigationV3.cs` (928 linjer; `bf4114948840`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBattlefieldNavigationV3.cs).

Klasser: `PrototypeBattlefieldNavigationV3` L12; `Obstacle` L14; `NavigationState` L21.

Indgange og kildeankre: `AutoCreate` L71; `Awake` L80; `OnDestroy` L102; `Update` L108; `Install` L135; `DisableLegacySteering` L151; `RefreshObstacles` L170; `AddObstacle` L201; `EnsureBridgeVisual` L212; `ApplyNavigation` L234; `BeginObstacleDetour` L363; `FinishObstacleDetour` L401; `TryChoosePersistentDetour` L425; `BeginBridgeRouting` L502; `FinishBridgeRouting` L521; `SetSteering` L540; `RecoverIfInsideObstacle` L554; `PathBlockedByObstacle` L589; `FindFirstBlockingObstacle` L613; `TryFindNearestValidDestination` L642; `IsBlockedDestination` L693; `EstimatePathPenalty` L707; `HasObstacleBetween` L749; `IsPointNavigable` L757; `GetBridgeSteeringTarget` L777; `SanitizePlayerRoutes` L802; `DistancePointToSegment` L857; `StreamCenterX` L880; `BankSide` L885; `IsBridgeZone` L890; `IsRiverWater` L897; `SegmentCrossesRiver` L903; `PlanarDistance` L922.

```csharp
L53: private const float BattlefieldHalfWidth = 176f;
L54: private const float BattlefieldHalfDepth = 116f;
L56: private const float RiverHalfWidth = 2.10f;
L57: private const float BridgeZ = 22.0f;
L58: private const float BridgeHalfLengthX = 8.0f;
L59: private const float BridgeHalfWidthZ = 4.0f;
L60: private const float BridgeApproachOffset = 12.0f;
L65: private const float LineClearance = 9.0f;
L66: private const float ColumnClearance = 4.4f;
L67: private const float DetourExtraClearance = 3.0f;
L68: private const float DetourArrivalDistance = 2.4f;
L908: const int samples = 40;
```


#### R/T `PrototypeBattlefieldNavigationV3.cs` (928 linjer; `5078b44cee44`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBattlefieldNavigationV3.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBattlefieldNavigationV3.cs).

Klasser: `PrototypeBattlefieldNavigationV3` L12; `Obstacle` L14; `NavigationState` L21.

Indgange og kildeankre: `AutoCreate` L71; `Awake` L80; `OnDestroy` L102; `Update` L108; `Install` L135; `DisableLegacySteering` L151; `RefreshObstacles` L170; `AddObstacle` L201; `EnsureBridgeVisual` L212; `ApplyNavigation` L234; `BeginObstacleDetour` L363; `FinishObstacleDetour` L401; `TryChoosePersistentDetour` L425; `BeginBridgeRouting` L502; `FinishBridgeRouting` L521; `SetSteering` L540; `RecoverIfInsideObstacle` L554; `PathBlockedByObstacle` L589; `FindFirstBlockingObstacle` L613; `TryFindNearestValidDestination` L642; `IsBlockedDestination` L693; `EstimatePathPenalty` L707; `HasObstacleBetween` L749; `IsPointNavigable` L757; `GetBridgeSteeringTarget` L777; `SanitizePlayerRoutes` L802; `DistancePointToSegment` L857; `StreamCenterX` L880; `BankSide` L885; `IsBridgeZone` L890; `IsRiverWater` L897; `SegmentCrossesRiver` L903; `PlanarDistance` L922.

```csharp
L53: private const float BattlefieldHalfWidth = 176f;
L54: private const float BattlefieldHalfDepth = 116f;
L56: private const float RiverHalfWidth = 2.10f;
L57: private const float BridgeZ = 22.0f;
L58: private const float BridgeHalfLengthX = 8.0f;
L59: private const float BridgeHalfWidthZ = 4.0f;
L60: private const float BridgeApproachOffset = 12.0f;
L65: private const float LineClearance = 9.0f;
L66: private const float ColumnClearance = 4.4f;
L67: private const float DetourExtraClearance = 3.0f;
L68: private const float DetourArrivalDistance = 2.4f;
L908: const int samples = 40;
```


#### K/R/T `PrototypeBattlefieldNavigationV4.cs` (964 linjer; `22812858100d`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBattlefieldNavigationV4.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBattlefieldNavigationV4.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBattlefieldNavigationV4.cs).

Klasser: `PrototypeBattlefieldNavigationV4` L15; `Obstacle` L17; `NavigationState` L24.

Indgange og kildeankre: `AutoCreate` L72; `Awake` L81; `OnDestroy` L102; `Update` L108; `Install` L131; `DisableOlderActiveNavigation` L147; `RefreshObstacles` L171; `AddObstacle` L201; `ApplyNavigation` L212; `UpdateStuckDetection` L310; `PlanPath` L340; `AdvancePathIndex` L390; `TryBuildAStarPath` L424; `ReconstructPath` L540; `SimplifyPath` L563; `TryFindNearestGridNode` L597; `GridNodeWalkable` L664; `GridPosition` L669; `Heuristic` L680; `SegmentWalkable` L692; `IsPointWalkable` L722; `RecoverIfInsideBlockedSpace` L742; `TryEmergencyEscape` L770; `TryFindNearestValidDestination` L828; `ForceColumn` L883; `RestoreFormation` L898; `SetSteering` L910; `DistancePointToSegment` L924; `StreamCenterX` L940; `IsBridgeZone` L945; `IsRiverWater` L952; `PlanarDistance` L958.

```csharp
L50: private const float BattlefieldHalfWidth = 176f;
L51: private const float BattlefieldHalfDepth = 116f;
L53: private const float RiverHalfWidth = 2.10f;
L54: private const float BridgeZ = 22.0f;
L55: private const float BridgeHalfLengthX = 8.0f;
L56: private const float BridgeHalfWidthZ = 4.0f;
L61: private const float TravelClearance = 3.8f;
L62: private const float LineDestinationClearance = 9.0f;
L64: private const float CellSize = 4.0f;
L65: private const int GridWidth = 89;   // -176 .. +176 inclusive
L66: private const int GridHeight = 59;  // -116 .. +116 inclusive
L67: private const float PathPointArrival = 2.2f;
L68: private const float ReplanStuckSeconds = 0.85f;
L69: private const float GoalChangeTolerance = 0.80f;
```


#### K `PrototypeBattlefieldVisualPass09G.cs` (485 linjer; `326db215316f`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBattlefieldVisualPass09G.cs).

Klasser: `PrototypeBattlefieldVisualPass09G` L9.

Indgange og kildeankre: `AutoCreate` L28; `Update` L37; `Install` L49; `CreateMaterials` L66; `CreateOuterTerrain` L96; `CreateFieldMosaic` L161; `CreateField` L186; `CreateSecondaryRoads` L206; `CreateRoadSpline` L233; `CreateHedgerows` L257; `CreateHedge` L276; `CreateScenicWoodland` L283; `CreateTreeCluster` L298; `CreateScenicTree` L311; `CreateHamlets` L333; `CreateHamlet` L342; `CreateScenicHouse` L354; `CreateRuralDetails` L387; `TuneAtmosphere` L433; `GroundRotation` L448; `CreateSegment` L464.

```csharp
L98: const int xSegments = 180;
L99: const int zSegments = 120;
L100: const float width = 720f;
L101: const float depth = 480f;
L450: const float sample = 1.5f;
```


#### K `PrototypeBootstrap.cs` (391 linjer; `d2986033a9b9`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBootstrap.cs).

Klasser: `PrototypeBootstrap` L3.

Indgange og kildeankre: `AutoBootstrap` L6; `Awake` L15; `BuildBattlefield` L20; `CreateLighting` L78; `CreateCamera` L88; `CreateGround` L102; `SampleGroundHeight` L164; `CreateStream` L180; `CreateRoad` L198; `CreateSegment` L216; `CreateFarmstead` L231; `CreateVegetation` L261; `CreateTree` L286; `CreateFences` L309; `CreateFenceLine` L329; `CreateRegiment` L349; `CreateSharedMaterial` L373.

```csharp
L104: const int xSegments = 144;
L105: const int zSegments = 96;
L106: const float width = 360f;
L107: const float depth = 240f;
```


#### R `PrototypeBootstrap.cs` (447 linjer; `913cc08bdaf6`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBootstrap.cs).

Klasser: `PrototypeBootstrap` L4.

Indgange og kildeankre: `AutoBootstrap` L13; `Awake` L22; `BuildBattlefield` L27; `CreateLighting` L87; `CreateCamera` L98; `CreateGround` L114; `SampleGroundHeight` L178; `StreamCenterX` L196; `CreateStream` L201; `CreateRoad` L223; `CreateSegment` L245; `CreateFarmstead` L260; `CreateFarmDetail` L296; `CreateVegetation` L306; `CreateTree` L331; `CreateFences` L365; `CreateFenceLine` L385; `CreateRegiment` L405; `CreateSharedMaterial` L429.

```csharp
L7: public const float BattlefieldWidth = 2880f;
L8: public const float BattlefieldDepth = 1920f;
L9: public const float BattlefieldHalfWidth = BattlefieldWidth * 0.5f;
L10: public const float BattlefieldHalfDepth = BattlefieldDepth * 0.5f;
L117: const int xSegments = 576;
L118: const int zSegments = 384;
L119: const float width = BattlefieldWidth;
L120: const float depth = BattlefieldDepth;
L206: const int points = 477;
L207: const float startZ = -952f;
L208: const float stepZ = 4f;
L228: const int points = 481;
L229: const float startX = -1408f;
L230: const float stepX = 5.87f;
```


#### T `PrototypeBootstrap.cs` (455 linjer; `56dc9d1fcac0`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBootstrap.cs).

Klasser: `PrototypeBootstrap` L4.

Indgange og kildeankre: `AutoBootstrap` L14; `Awake` L23; `BuildBattlefield` L28; `CreateLighting` L88; `CreateCamera` L99; `CreateGround` L115; `SampleGroundHeight` L180; `StreamCenterX` L198; `CreateStream` L203; `CreateRoad` L226; `CreateSegment` L249; `CreateFarmstead` L264; `CreateFarmDetail` L300; `CreateVegetation` L310; `CreateTree` L339; `CreateFences` L373; `CreateFenceLine` L393; `CreateRegiment` L413; `CreateSharedMaterial` L437.

```csharp
L8: public const float BattlefieldWidth = 5760f;
L9: public const float BattlefieldDepth = 3840f;
L10: public const float BattlefieldHalfWidth = BattlefieldWidth * 0.5f;
L11: public const float BattlefieldHalfDepth = BattlefieldDepth * 0.5f;
L119: const int xSegments = 576;
L120: const int zSegments = 384;
L121: const float width = BattlefieldWidth;
L122: const float depth = BattlefieldDepth;
L208: const float margin = 8f;
L209: const float stepZ = 4f;
L231: const float margin = 32f;
L232: const float stepX = 5.87f;
L314: const int treeCount = 1800;
```


#### R `PrototypeBottomCommandBar09F2.cs` (365 linjer; `eee60ae120b6`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBottomCommandBar09F2.cs).

Klasser: `PrototypeBottomCommandBar09F2` L5.

Indgange og kildeankre: `AutoCreate` L18; `UseLookPack` L27; `Update` L32; `OnDisable` L40; `SuppressLegacyOfficerManagerWhenReady` L45; `ControllersInstalled` L66; `ApplyInitialEnemyHold` L85; `HandleHotkeys` L95; `SetEnemyActive` L104; `ToggleSelectedDanishAI` L135; `UpdatePointerIsolation` L145; `RestorePointerComponents` L178; `GetPanelRect` L197; `DrawFireButton` L308; `ForEachSelectedDanish` L316; `GetSelectedDanishCount` L333; `GetFirstSelectedRegiment` L347; `GetFirstSelectedController` L360.

```csharp
L7: private const float PanelHeight = 64f;
L230: const float buttonH = 27f;
```


#### T `PrototypeBottomCommandBar09F2.cs` (274 linjer; `225960c10935`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBottomCommandBar09F2.cs).

Klasser: `PrototypeBottomCommandBar09F2` L7.

Indgange og kildeankre: `AutoCreate` L20; `Update` L29; `OnDisable` L37; `SuppressLegacyOfficerManagerWhenReady` L42; `ControllersInstalled` L63; `ApplyInitialEnemyHold` L82; `HandleHotkeys` L92; `SetEnemyActive` L101; `ToggleSelectedDanishAI` L132; `UpdatePointerIsolation` L142; `RestorePointerComponents` L169; `GetPanelRect` L188; `DrawFireButton` L217; `ForEachSelectedDanish` L225; `GetSelectedDanishCount` L242; `GetFirstSelectedRegiment` L256; `GetFirstSelectedController` L269.

```csharp
L9: private const float PanelHeight = 64f;
```


#### K `PrototypeBoxSelection09H.cs` (286 linjer; `19b1adc2defa`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBoxSelection09H.cs).

Klasser: `PrototypeBoxSelection09H` L10.

Indgange og kildeankre: `AutoCreate` L28; `Start` L37; `Update` L43; `ResolveSelectionList` L68; `BeginPotentialDrag` L98; `CompletePotentialDrag` L128; `ApplyBoxSelection` L140; `RestoreMouseDownSelection` L193; `ClearSelection` L201; `AddSelection` L212; `RemoveSelection` L222; `GetScreenRectBottomLeft` L231; `GetGuiRectTopLeft` L240; `EnsureGuiStyle` L251.

```csharp
L12: private const float DragThresholdPixels = 9f;
L276: const float border = 1.5f;
```


#### R `PrototypeBoxSelection09H.cs` (286 linjer; `8e914663c25f`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBoxSelection09H.cs).

Klasser: `PrototypeBoxSelection09H` L10.

Indgange og kildeankre: `AutoCreate` L28; `Start` L37; `Update` L43; `ResolveSelectionList` L68; `BeginPotentialDrag` L98; `CompletePotentialDrag` L128; `ApplyBoxSelection` L140; `RestoreMouseDownSelection` L193; `ClearSelection` L201; `AddSelection` L212; `RemoveSelection` L222; `GetScreenRectBottomLeft` L231; `GetGuiRectTopLeft` L240; `EnsureGuiStyle` L251.

```csharp
L12: private const float DragThresholdPixels = 9f;
L276: const float border = 1.5f;
```


#### T `PrototypeBoxSelection09H.cs` (385 linjer; `579355ce8380`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBoxSelection09H.cs).

Klasser: `PrototypeBoxSelection09H` L11.

Indgange og kildeankre: `AutoCreate` L29; `Start` L38; `Update` L44; `OfficerOrderOwnsPointer` L78; `CancelPotentialDrag` L84; `ResolveSelectionList` L91; `BeginPotentialDrag` L121; `CompletePotentialDrag` L153; `ApplyBoxSelection` L165; `TrySelectNonInfantry` L229; `RestoreMouseDownSelection` L292; `ClearSelection` L300; `AddSelection` L311; `RemoveSelection` L321; `GetScreenRectBottomLeft` L330; `GetGuiRectTopLeft` L339; `EnsureGuiStyle` L350.

```csharp
L13: private const float DragThresholdPixels = 9f;
L375: const float border = 1.5f;
```


#### R/T `PrototypeBridgeVisual09F3.cs` (67 linjer; `c6f57e67f5f5`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBridgeVisual09F3.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBridgeVisual09F3.cs).

Klasser: `PrototypeBridgeVisual09F3` L5.

Indgange og kildeankre: `AutoCreate` L10; `Start` L19; `CreateRail` L51.

```csharp
L7: private const float BridgeZ = 22.0f;
```


#### K `PrototypeBrigadeCommand09L.cs` (402 linjer; `1dc5bb061d70`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBrigadeCommand09L.cs).

Klasser: `PrototypeBrigadeCommand09L` L21; `BrigadeState` L23.

Tilstands-/data-enums: `PrototypeBrigadeMission09L` = Hold, DefendArea, AttackCaptureArea (L4); `PrototypeBrigadeRegimentRole09L` = Reserve, Support, Engaged, Manoeuvre, Withdraw (L11).

Indgange og kildeankre: `Contains` L47; `SetDanishAIEnabled` L55; `AutoCreate` L82; `Awake` L90; `OnDestroy` L100; `Update` L106; `TryInstall` L121; `Ready` L137; `CreateBrigade` L142; `EvaluateBrigade` L165; `AuthorityAllows` L195; `ApplyMission` L201; `CommitReserve` L244; `GetOfficer` L261; `ReleaseDanishToManualControl` L266; `SetRole` L278; `GetDisplayName` L289; `HorizontalDistance` L297; `BuildBrigadeHQ` L304; `AddHQLabel` L336; `DescribeBrigade` L395.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeBuildVersionOverlay.cs` (43 linjer; `1538f3ff7181`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeBuildVersionOverlay.cs).

Klasser: `PrototypeBuildVersionOverlay` L4.

Indgange og kildeankre: `AutoCreate` L12.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R `PrototypeBuildVersionOverlay.cs` (40 linjer; `b6339c2972fa`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeBuildVersionOverlay.cs).

Klasser: `PrototypeBuildVersionOverlay` L4.

Indgange og kildeankre: `AutoCreate` L12.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeBuildVersionOverlay.cs` (36 linjer; `31b9518369cc`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeBuildVersionOverlay.cs).

Klasser: `PrototypeBuildVersionOverlay` L4.

Indgange og kildeankre: `AutoCreate` L12.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K/R/T `PrototypeCasualtyVisualManager.cs` (113 linjer; `c8ff76dbbbe0`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCasualtyVisualManager.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeCasualtyVisualManager.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCasualtyVisualManager.cs).

Klasser: `PrototypeCasualtyVisualManager` L4.

Indgange og kildeankre: `AutoCreate` L9; `Update` L18; `CreateCasualtyVisual` L49; `CreateMaterial` L101.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeCavalryAnimation09F30I.cs` (407 linjer; `ba32a9dfcfe5`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryAnimation09F30I.cs).

Klasser: `PrototypeCavalryAnimation09F30I` L9; `TransitionState` L15; `MountedRig` L32.

Indgange og kildeankre: `AutoCreate` L53; `GetOrCreate` L58; `IsTransitioning` L70; `BeginDismount` L75; `BeginRemount` L80; `Begin` L85; `Capture` L116; `Update` L164; `UpdateTransitions` L170; `UpdateGait` L240; `ApplyGait` L250; `GetRigs` L317; `FindDescendant` L368; `SetAnimatedRotation` L380; `RestoreRig` L386.

```csharp
L12: private const float DismountDuration = 2.25f;
L13: private const float RemountDuration = 4.50f;
```


#### T `PrototypeCavalryCommandControl09F30C.cs` (115 linjer; `1650f9d9ec53`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryCommandControl09F30C.cs).

Klasser: `PrototypeCavalryCommandControl09F30C` L8.

Indgange og kildeankre: `AutoCreate` L24; `GetParent` L31; `GetParentShort` L41; `SetParent` L51; `CycleParent` L71; `IsPointerOverControlStrip` L81.

```csharp
L16: private const float HudHeight = 90f;
```


#### T `PrototypeCavalryManager09F30.cs` (730 linjer; `836fe0b15050`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryManager09F30.cs).

Klasser: `PrototypeCavalryManager09F30` L11.

Indgange og kildeankre: `BeginChargePick` L61; `CancelChargePick` L67; `AutoCreate` L73; `Awake` L80; `OnDestroy` L94; `Update` L100; `TryInstall` L128; `Spawn` L161; `SelectUnit` L173; `ClearSelection` L191; `HandleWorldSelection` L201; `HandlePendingChargePick` L228; `HandleRightMouseOrder` L250; `HasOtherCommandSelection` L323; `ClearInfantryAndHqSelection` L352; `TryGetGround` L380; `RaycastRegiment` L420; `IsPointerOverUi` L431; `DrawDragonFireButton` L637; `SetSelectedManual` L650; `MakeButtonStyle` L703.

```csharp
L15: private const float HudHeight = 90f;
L32: private const float FacingDragThreshold = 4f;
L526: const float fireGap = 2f;
L574: const float gap = 3f;
```


#### T `PrototypeCavalryOfficerAI09F30C.cs` (894 linjer; `a63ef0ccf8ee`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryOfficerAI09F30C.cs).

Klasser: `PrototypeCavalryOfficerAI09F30C` L8; `State` L10.

Indgange og kildeankre: `AutoCreate` L59; `Awake` L66; `OnDestroy` L76; `Update` L82; `TryInstall` L135; `Register` L147; `IsAIEnabled` L161; `SetAIEnabled` L167; `ToggleAI` L220; `SetHigherMission` L225; `ClearHigherMission` L271; `HasHigherMission` L283; `GetPhase` L290; `GetTargetName` L298; `DetectDirectPlayerOverride` L306; `ParentAllowsDelegatedAI` L324; `ExecuteHigherMission` L359; `HigherMissionLabel` L418; `Think` L432; `PlanManeuver` L558; `PlanStandOffEvasion` L620; `IsTargetEngagedByFriendlyInfantry` L666; `AvoidEnemyBubbleOnRoute` L687; `DetourCost` L750; `DistancePointToSegmentXZ` L776; `AcquireTarget` L799; `ValidTarget` L840; `GetAspect` L845; `PlannedAspectLabel` L857; `SafeMountedPoint` L862; `Flat` L882; `PlanarDistance` L888.

```csharp
L13: public bool Enabled = false;
L21: public int PreferredSide = 1;
L31: private const float ThinkInterval = 1.15f;
L32: private const float SearchRange = 1200f;
L33: private const float RearDepth = 145f;
L34: private const float RearLateral = 70f;
L35: private const float FlankOffset = 145f;
L36: private const float FlankRearBias = 50f;
L37: private const float ChargeCommitRange = 155f;
L38: private const float ArriveTolerance = 24f;
L39: private const float ReplanTargetMove = 42f;
L40: private const float MaxManeuverSeconds = 26f;
L44: private const float EnemyAvoidRadius = 105f;
L45: private const float EnemyDetourRadius = 138f;
L46: private const float StandOffMinDistance = 115f;
L47: private const float StandOffDistance = 145f;
L48: private const float OpportunityCohesionThreshold = 62f;
L49: private const float OpportunityMoraleThreshold = 58f;
L50: private const float BottomHudHeight = 90f;
```


#### T `PrototypeCavalryOob09F30A.cs` (259 linjer; `c11bfe4154fd`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryOob09F30A.cs).

Klasser: `PrototypeCavalryOob09F30A` L9.

Indgange og kildeankre: `AutoCreate` L36; `Awake` L43; `OnDestroy` L51; `Update` L57; `IsPointerOverPanel` L63; `ShouldDraw` L73; `GetPanelRect` L87; `DrawUnitRow` L127; `RegisterClick` L157; `FocusBehind` L166; `ShortStatus` L194.

```csharp
L12: private const float DoubleClickSeconds = 0.34f;
L13: private const float HeaderHeight = 19f;
L14: private const float RowHeight = 22f;
L22: private float lastClickAt = -10f;
```


#### T `PrototypeCavalryOrderVisuals09F30I.cs` (209 linjer; `a9efbcabd607`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryOrderVisuals09F30I.cs).

Klasser: `PrototypeCavalryOrderVisuals09F30I` L7; `State` L9.

Indgange og kildeankre: `AutoCreate` L25; `Update` L32; `Ensure` L44; `CreateLine` L62; `UpdateState` L81; `DrawPath` L136; `IsVisibleFromHigherSelection` L152; `DrawLink` L174; `DrawBox` L181; `Ground` L204.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeCavalrySquareThreat09F30.cs` (136 linjer; `588536a2cb41`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalrySquareThreat09F30.cs).

Klasser: `PrototypeCavalrySquareThreat09F30` L9.

Indgange og kildeankre: `AutoCreate` L14; `Awake` L21; `Update` L28; `FindNearestThreatenedInfantry` L79; `PlanarDistance` L130.

```csharp
L11: private const float MaxThreatDistance = 220f;
```


#### T `PrototypeCavalryUnit09F30.cs` (1465 linjer; `843b809e6a3b`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryUnit09F30.cs).

Klasser: `PrototypeCavalryUnit09F30` L34.

Tilstands-/data-enums: `PrototypeCavalryKind09F30` = Gardehusar, Dragon (L4); `PrototypeCavalryFormation09F30` = Line, Column (L10); `PrototypeCavalryMode09F30` = Mounted, Dismounted (L16); `PrototypeCavalryAction09F30` = Hold, Move, Charge, Falter (L22).

Indgange og kildeankre: `GetCurrentFootprintSize` L93; `GetDestinationFootprintSize` L98; `GetCurrentFootprintCenterWorld` L103; `GetDestinationFootprintCenterWorld` L110; `GetDismountedCombatCenterWorld` L121; `GetDismountedCombatFootprintSize` L126; `GetDismountedHorseHolderCenterWorld` L133; `GetDismountedHorseHolderFootprintSize` L149; `GetDismountedCombatStrength` L159; `GetHorseHolderStrength` L164; `Initialize` L215; `OnDestroy` L252; `Update` L258; `SetSelected` L287; `SnapVisualFormationForInitialization` L294; `SetFormationManual` L302; `ClearManualFormationOverride` L312; `SetFormation` L317; `OrderMove` L337; `OrderMove` L347; `OrderCharge` L370; `OrderHold` L390; `ReceiveInfantryVolley` L404; `RefreshStrengthVisuals` L467; `Dismount` L490; `Remount` L524; `BeginReturnToHorses` L546; `UpdateRemountRequest` L580; `CompleteRemount` L599; `GetStatusLabel` L662; `BeginRoute` L668; `UpdateBridgeApproachState` L706; `UpdateMountedMoveFormationPolicy` L747; `ApplyMountedMoveFormationPolicy` L761; `HasEnemyInsideLongRange` L793; `UpdateMovement` L818; `ResolveSteeringTarget` L874; `AdvanceBridgePhase` L908; `GetMoveSpeed` L941; `ResolveChargeContact` L955; `GetChargeAspect` L1033; `CreateVisuals` L1050; `CreateMountedFigure` L1085; `CreateFootFigure` L1123; `UpdateVisualFormation` L1145; `RefreshFormationInstant` L1206; `MountedPosition` L1228; `IsHorseHolderVisual` L1261; `HorseHolderAnchorSlot` L1268; `HorseHolderAnchoredLocalPosition` L1281; `HorseHolderAnchoredLocalRotation` L1294; `FootPosition` L1303; `CalculateFootprint` L1342; `CalculateFootprintCenterOffsetZ` L1374; `ResizeCollider` L1393; `BridgeExitClearance` L1432; `BankSide` L1438; `Ground` L1447; `Flat` L1453; `PlanarDistance` L1459.

```csharp
L51: public float Morale { get; private set; } = 100f;
L52: public float Cohesion { get; private set; } = 100f;
L60: public float FormationReadyFraction { get; private set; } = 1f;
L193: private const float BridgeZ = 22f;
L194: private const float BridgeBankOffset = 13.5f;
L195: private const float BridgeNarrowApproachDistance = 36f;
L196: private const float ContactDistance = 7.5f;
L197: private const float MountedReformSpeed = 8.0f;
L198: private const float FootReformSpeed = 5.0f;
L199: private const float FormationReadyTolerance = 0.55f;
L200: private const float RemountGatherDistance = 3.0f;
L201: private const float AutoMarchColumnEnterDistance = 140f;
L202: private const float AutoMarchLineDistance = 90f;
```


#### T `PrototypeCavalryVisualFidelity09F30L.cs` (335 linjer; `29136de86c15`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryVisualFidelity09F30L.cs).

Klasser: `PrototypeCavalryVisualFidelity09F30L` L10.

Indgange og kildeankre: `AutoCreate` L22; `Update` L29; `PolishUnit` L62; `CreateHorsePalette` L114; `ReshapeExistingFigure` L127; `BuildCloseDetail` L199; `UpdateDetailLod` L258; `FindDescendant` L275; `SetScale` L287; `SetPosition` L292; `SetRotation` L297; `ApplyMaterial` L302.

```csharp
L13: private const float DetailCameraHeight = 210f;
L17: private bool closeDetailsVisible = true;
```


#### T `PrototypeCavalryVisualOneToOne09F30E.cs` (613 linjer; `9d7392cb9128`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryVisualOneToOne09F30E.cs).

Klasser: `PrototypeCavalryVisualOneToOne09F30E` L10.

Indgange og kildeankre: `AutoCreate` L17; `Update` L24; `EnhanceUnit` L61; `CreateHorseMaterials` L159; `CreateMountedFigure` L175; `EnhanceExistingMountedFigure` L245; `EnhanceExistingDismountedDragon` L288; `BuildRider` L314; `CreateDismountedDragon` L408; `AddHorseLegSet` L447; `EnhanceHigherHq` L469; `MaintainOneToOneCollider` L534; `ResizeOneToOneCollider` L549.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeCavalryVisualPolish09F30D.cs` (197 linjer; `a5667cfd23ef`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryVisualPolish09F30D.cs).

Klasser: `PrototypeCavalryVisualPolish09F30D` L8.

Indgange og kildeankre: `AutoCreate` L15; `Update` L21; `EnhanceUnit` L41; `CreateMountedFigure` L87; `CreateFootFigure` L110; `AddHorseDetails` L120; `AddHorseLegSet` L136; `EnhanceHigherHq` L150.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeCavalryVisualPolish09F30H.cs` (117 linjer; `36fa19eb1fe6`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCavalryVisualPolish09F30H.cs).

Klasser: `PrototypeCavalryVisualPolish09F30H` L9.

Indgange og kildeankre: `AutoCreate` L16; `Update` L23; `Polish` L41; `EnsureLabel` L83.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeChargeTargeting09F29K.cs` (310 linjer; `b48e2023d5d6`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeChargeTargeting09F29K.cs).

Klasser: `PrototypeChargeTargeting09F29K` L10.

Indgange og kildeankre: `AutoCreate` L35; `Awake` L42; `OnDestroy` L62; `Update` L68; `EnsureReflectionBindings` L131; `CaptureLegacyTargetPick` L146; `RemoveInvalidArmedUnits` L186; `CancelArmed` L196; `ClearArmed` L205; `GetEnemyUnderMouse` L211; `PointerOverBottomHud` L231.

```csharp
L16: private const float BottomHudHeight = 100f;
L253: const float gap = 3f;
```


#### R `PrototypeCombatQa09F9.cs` (115 linjer; `a1cf3ab96f42`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeCombatQa09F9.cs).

Klasser: `PrototypeCombatQa09F9` L10.

Indgange og kildeankre: `AutoCreate` L25; `Awake` L34; `Update` L47; `ApplyAccuracyOnce` L53; `RepairParticleVelocityCurves` L85.

```csharp
L12: public const float DanishQaAccuracy = 0.050f;
L13: public const float PrussianQaAccuracy = 0.052f;
```


#### T `PrototypeCombatQa09F9.cs` (116 linjer; `cdbd88c60a36`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCombatQa09F9.cs).

Klasser: `PrototypeCombatQa09F9` L10.

Indgange og kildeankre: `AutoCreate` L23; `Awake` L32; `Update` L45; `ApplyAccuracyOnce` L51; `RepairParticleVelocityCurves` L83.

```csharp
L12: public const float DanishQaAccuracy = 0.050f;
L13: public const float PrussianQaAccuracy = 0.052f;
```


#### K/R `PrototypeCombatStatusManager.cs` (251 linjer; `4319cbde902f`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCombatStatusManager.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeCombatStatusManager.cs).

Klasser: `PrototypeCombatStatusManager` L6; `CombatState` L8.

Indgange og kildeankre: `AutoCreate` L25; `Awake` L34; `OnDestroy` L52; `Update` L58; `ReadNextFireTime` L115; `GetLikelyTarget` L121; `WriteVolleyDiagnostic` L154; `GetAmmunitionRoundsPerMan` L215; `GetStartingAmmunitionRoundsPerMan` L225; `TryGetVolleyFeedback` L235.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeCombatStatusManager.cs` (320 linjer; `53317998ea7f`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCombatStatusManager.cs).

Klasser: `PrototypeCombatStatusManager` L6; `CombatState` L8.

Indgange og kildeankre: `AutoCreate` L26; `Awake` L35; `OnDestroy` L53; `EnsureState` L59; `Update` L79; `RegisterExternalVolley` L133; `ReadNextFireTime` L182; `GetLikelyTarget` L190; `WriteVolleyDiagnostic` L223; `GetAmmunitionRoundsPerMan` L284; `GetStartingAmmunitionRoundsPerMan` L294; `TryGetVolleyFeedback` L304.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeCombatTuningManager.cs` (431 linjer; `38f970049693`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCombatTuningManager.cs).

Klasser: `PrototypeCombatTuningManager` L5.

Indgange og kildeankre: `AutoCreate` L30; `Awake` L39; `OnDestroy` L57; `Update` L63; `TogglePanel` L71; `IsPointerOverControls` L76; `GetStartingAmmoRoundsPerMan` L85; `GetCasualtiesPerBody` L90; `GetConfiguredRangeMultiplier` L95; `GetRangeBandLabel` L119; `GetConfiguredQualityMultiplier` L133; `GetConfiguredFiringMen` L154; `GetExpectedHitsPreview` L165; `ApplyCombatTuning` L183; `GetReferenceTarget` L232; `FindNearestReferenceTarget` L247; `GetKernelRangeMultiplier` L268; `GetPanelRect` L274; `DrawFloatSlider` L397; `ResetDefaults` L419.

```csharp
L9: public static float BaseHitChancePercent { get; private set; } = 1.40f;
L10: public static float CloseRangeMultiplier { get; private set; } = 1.75f;
L11: public static float MediumRangeMultiplier { get; private set; } = 1.00f;
L12: public static float LongRangeMultiplier { get; private set; } = 0.30f;
L13: public static float FiringFractionPercent { get; private set; } = 58f;
L14: public static int StartingAmmoRoundsPerMan { get; private set; } = 60;
L15: public static int CasualtiesPerBody { get; private set; } = 8;
L19: public static float MoraleAccuracyFloor { get; private set; } = 0.72f;
L20: public static float CohesionAccuracyFloor { get; private set; } = 0.68f;
L407: const float labelWidth = 184f;
L408: const float valueWidth = 66f;
```


#### R/T `PrototypeCombatTuningManager.cs` (387 linjer; `bcc6a0f6709c`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeCombatTuningManager.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCombatTuningManager.cs).

Klasser: `PrototypeCombatTuningManager` L9.

Indgange og kildeankre: `AutoCreate` L32; `Awake` L41; `OnDestroy` L64; `Update` L70; `TogglePanel` L79; `IsPointerOverControls` L84; `GetStartingAmmoRoundsPerMan` L93; `GetCasualtiesPerBody` L98; `GetConfiguredRangeMultiplier` L103; `GetRangeBandLabel` L127; `GetConfiguredQualityMultiplier` L140; `GetConfiguredFiringMen` L153; `GetExpectedHitsPreview` L164; `ApplyCombatTuning` L182; `GetReferenceTarget` L227; `FindNearestReferenceTarget` L242; `GetKernelRangeMultiplier` L263; `GetPanelRect` L269; `DrawFloatSlider` L352; `ResetDefaults` L374.

```csharp
L13: public static float BaseHitChancePercent { get; private set; } = 12.00f;
L14: public static float CloseRangeMultiplier { get; private set; } = 2.00f;
L15: public static float MediumRangeMultiplier { get; private set; } = 1.10f;
L16: public static float LongRangeMultiplier { get; private set; } = 0.55f;
L17: public static float FiringFractionPercent { get; private set; } = 58f;
L18: public static int StartingAmmoRoundsPerMan { get; private set; } = 60;
L19: public static int CasualtiesPerBody { get; private set; } = 1;
L21: public static float MoraleAccuracyFloor { get; private set; } = 0.72f;
L22: public static float CohesionAccuracyFloor { get; private set; } = 0.68f;
L362: const float labelWidth = 184f;
L363: const float valueWidth = 66f;
```


#### T `PrototypeCommandAttachment09F30B.cs` (41 linjer; `db6b47f491df`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCommandAttachment09F30B.cs).

Klasser: `PrototypeCommandAttachment09F30B` L15.

Tilstands-/data-enums: `PrototypeAttachmentType09F30B` = Organic, Attached, Detached, Reserve (L3).

Indgange og kildeankre: `Configure` L21; `SetCurrentCommandParent` L28; `GetShortLabel` L34.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeCommandChainVisual09F29G.cs` (134 linjer; `2ca043cdf46b`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCommandChainVisual09F29G.cs).

Klasser: `PrototypeCommandChainVisual09F29G` L10.

Indgange og kildeankre: `AutoCreate` L24; `Awake` L31; `LateUpdate` L41; `GetSelectedMajor` L113; `DrawTerrainLink` L121.

```csharp
L13: private const int LinkSamples = 30;
L14: private const float LinkHeight = 0.96f;
```


#### T `PrototypeCommandHud09F29C.cs` (859 linjer; `2571393f02ac`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCommandHud09F29C.cs).

Klasser: `PrototypeCommandHud09F29C` L14; `AggregateStats` L702; `PrototypeUnitNames09F29C` L836.

Indgange og kildeankre: `AutoCreate` L53; `Awake` L60; `OnDestroy` L92; `Update` L98; `DrawMajorHud` L147; `DrawMajorOrderButton` L240; `DrawCompanyHud` L260; `DrawFireButton` L401; `SetSelectedAi` L416; `SetSelectedDoctrine` L427; `SetSelectedAggression` L437; `BeginWithdrawal` L447; `BeginChargePick` L453; `SetForcedMarch` L460; `StopSelected` L474; `SetSelectedFormation` L499; `FinishCharge` L534; `CancelWithdrawal` L542; `ClearPlayerRoute` L550; `GetSelectedBattalionIndex` L596; `GetPendingMajorOrder` L606; `SetPendingMajorOrder` L616; `GetSelectedDanishCompanies` L623; `ApplyVisibleCompanyNames` L638; `GetFormationLabel` L673; `All` L682; `Any` L692; `CalculateStats` L712; `CalculateStats` L747; `StateStyle` L752; `DrawSectionLabel` L757; `BuildStyles` L762; `MakeButtonStyle` L808; `Get` L841.

```csharp
L18: private const float HudHeight = 90f;
L326: const float gap = 3f;
```


#### T `PrototypeCommandTreeVisibility09F30B.cs` (200 linjer; `31578f4501d3`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCommandTreeVisibility09F30B.cs).

Klasser: `PrototypeCommandTreeVisibility09F30B` L11.

Indgange og kildeankre: `AutoCreate` L26; `Awake` L33; `LateUpdate` L52; `ForceLowerTree` L81; `ForceHigherTree` L118; `DrawSupportLink` L140; `GetSelectedMajor` L158; `AnyCompanySelected` L166; `GetLine` L180; `DrawTerrainLink` L185.

```csharp
L14: private const int LinkSamples = 30;
```


#### T `PrototypeCommandTreeVisibility09F30C.cs` (167 linjer; `ae155718efc5`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCommandTreeVisibility09F30C.cs).

Klasser: `PrototypeCommandTreeVisibility09F30C` L10.

Indgange og kildeankre: `AutoCreate` L24; `Awake` L31; `LateUpdate` L42; `DrawHigher` L69; `DrawLower` L79; `DrawSupport` L105; `GetSelectedMajor` L126; `AnyCompanySelected` L134; `GetLine` L147; `DrawTerrainLink` L152.

```csharp
L13: private const int LinkSamples = 30;
```


#### K `PrototypeCompanyCombatAuthority09L4.cs` (74 linjer; `4ff2ea14ae3e`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyCombatAuthority09L4.cs).

Klasser: `PrototypeCompanyCombatAuthority09L4` L9.

Indgange og kildeankre: `AutoCreate` L16; `Update` L25; `OnDestroy` L63.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeCompanyGrounding09L2.cs` (37 linjer; `cffd72d9f8fb`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyGrounding09L2.cs).

Klasser: `PrototypeCompanyGrounding09L2` L7.

Indgange og kildeankre: `AutoCreate` L10; `LateUpdate` L19.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeCompanyGuidons09L4.cs` (205 linjer; `da1b1d8e966d`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyGuidons09L4.cs).

Klasser: `PrototypeCompanyGuidons09L4` L9.

Indgange og kildeankre: `AutoCreate` L16; `Update` L25; `BuildGuidons` L58; `BuildSmallFlag` L131.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeCompanyRenderer09L2.cs` (438 linjer; `1418c43b530c`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyRenderer09L2.cs).

Klasser: `PrototypeCompanyRenderer09L2` L8; `CompanyRenderState` L13; `RegimentResources` L22.

Indgange og kildeankre: `AutoCreate` L61; `Awake` L70; `OnDestroy` L85; `Update` L91; `GetState` L135; `GetResources` L146; `GetCurrentProfile` L168; `RefreshResources` L181; `RebuildSlots` L194; `DrawCompany` L245; `DrawFootRegimentalStaff` L330; `Flush` L381; `UpdateReloadState` L389; `GetReloadProgress` L407; `CreateInstancedMaterial` L414; `BuildPrimitiveMeshes` L421; `ExtractMesh` L429.

```csharp
L18: public int LastInitialStrength = -1;
L53: private const float LineFileSpacing = 0.52f;
L54: private const float LineRankSpacing = 0.76f;
L55: private const int LineRanks = 3;
L56: private const int ColumnFiles = 8;
L57: private const float ColumnFileSpacing = 0.60f;
L58: private const float ColumnRankSpacing = 0.72f;
```


#### K `PrototypeCompanyRendererAuthority09L2.cs` (66 linjer; `caa1bc8e8acf`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyRendererAuthority09L2.cs).

Klasser: `PrototypeCompanyRendererAuthority09L2` L6.

Indgange og kildeankre: `AutoCreate` L11; `Update` L20; `IsPilot` L59.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R `PrototypeCompanyScale09F5.cs` (91 linjer; `1ef133a6c49c`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeCompanyScale09F5.cs).

Klasser: `PrototypeCompanyScale09F5` L8.

Indgange og kildeankre: `AutoCreate` L22; `Awake` L31; `Update` L48.

```csharp
L17: private const int CompanyStrength = 190;
L18: private const float EffectiveRangeMetres = 200f;
L19: private const float LongRangeMetres = 400f;
```


#### T `PrototypeCompanyScale09F5.cs` (93 linjer; `ced06dd3cfd3`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeCompanyScale09F5.cs).

Klasser: `PrototypeCompanyScale09F5` L9.

Indgange og kildeankre: `AutoCreate` L23; `Awake` L32; `Update` L49.

```csharp
L18: private const int CompanyStrength = 190;
L19: private const float EffectiveRangeMetres = 200f;
L20: private const float LongRangeMetres = 400f;
```


#### K `PrototypeCompanyScreenSelection09L4.cs` (250 linjer; `5a8a1fd55362`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyScreenSelection09L4.cs).

Klasser: `PrototypeCompanyScreenSelection09L4` L11.

Indgange og kildeankre: `AutoCreate` L29; `Update` L38; `ResolveLists` L87; `FindCompanyAtScreenPoint` L108; `TryGetProjectedFootprint` L148; `ApplySelection` L194; `ClearCompanySelection` L222; `ClearRegimentSelection` L237.

```csharp
L13: private const float ClickDragThresholdPixels = 9f;
L14: private const float ScreenPaddingPixels = 7f;
```


#### K `PrototypeCompanyTacticalControl09L2.cs` (1054 linjer; `dee41db3f404`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeCompanyTacticalControl09L2.cs).

Klasser: `PrototypeCompanyTacticalEntity09L2` L17; `CompanyWaypoint` L46; `PrototypeCompanyTacticalControl09L2` L247.

Tilstands-/data-enums: `PrototypeCompanyRole09L2` = Engaged, Support, Reserve, Manoeuvre, Withdraw (L6).

Indgange og kildeankre: `Configure` L58; `SetSelected` L88; `SetFormation` L95; `GetFootprintWidth` L101; `GetFootprintDepth` L111; `IssueMove` L123; `Hold` L141; `RotateFacing` L146; `ReturnToDefaultPosition` L156; `Update` L165; `RefreshFootprint` L209; `BuildSelectionOutline` L229; `AutoCreate` L293; `Awake` L302; `OnDestroy` L313; `Start` L319; `Update` L325; `LateUpdate` L366; `TryInstall` L375; `BuildCompaniesForRegiment` L425; `ResolvePlayerCommanderInternals` L468; `BeginLeftSelection` L487; `UpdateLeftSelection` L501; `CompleteLeftSelection` L508; `ApplyPointSelection` L521; `ApplyCompanyBoxSelection` L571; `BeginCompanyOrder` L608; `UpdateCompanyOrder` L618; `CompleteCompanyOrder` L624; `HandleCompanyHotkeys` L673; `PrepareRegimentForManualCompanyControl` L706; `SyncCompanyStrengths` L726; `ReconcileCompanyStrengths` L749; `SuppressCommanderThisFrame` L821; `ClearCompanySelection` L832; `AddCompanySelection` L840; `RemoveCompanySelection` L850; `ClearRegimentSelection` L858; `SelectRegiment` L873; `GetCompanyUnderMouse` L894; `GetSelectableUnderMouse` L900; `TryGetGroundPoint` L936; `GetAverageCompanyFacing` L963; `GetRegimentDisplayName` L973; `GetScreenRectBottomLeft` L981; `GetGuiRectTopLeft` L990.

```csharp
L115: const int filesAcross = 8;
L255: private const float DragThresholdPixels = 9f;
L256: private const float RightFacingThreshold = 4f;
L257: private const float DoubleClickSeconds = 0.32f;
L1032: const float border = 1.4f;
```


#### T `PrototypeDefenseArrivalGuard09F29L.cs` (211 linjer; `7551f8139e79`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeDefenseArrivalGuard09F29L.cs).

Klasser: `PrototypeDefenseArrivalGuard09F29L` L13.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L34; `Update` L47; `LateUpdate` L139; `SuppressF29EPreciseCorrection` L152; `SetVisualArrived` L164; `CleanupStale` L183; `PlanarDistance` L205.

```csharp
L16: private const float ArrivalLatchDistance = 0.50f;
L17: private const float ArrivalReleaseDistance = 1.25f;
```


#### T `PrototypeDefensiveStability09F29Y.cs` (413 linjer; `6d44504fea92`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeDefensiveStability09F29Y.cs).

Klasser: `PrototypeDefensiveStability09F29Y` L16.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L34; `Update` L44; `GuardBattalion` L59; `EnforceMissionBank` L166; `ResolveDefendBank` L243; `TryGetFrontReference` L275; `TryRearHqGoal` L318; `TrySameBankGoal` L342; `BankSide` L379; `ReadBool` L387; `Flat` L395; `Ground` L401; `PlanarDistance` L407.

```csharp
L19: private const float BankDeadZone = 7.0f;
L20: private const float MajorRearDepth = 155f;
```


#### R/T `PrototypeDestinationFacing09F13.cs` (185 linjer; `8573cefe1c03`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeDestinationFacing09F13.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeDestinationFacing09F13.cs).

Klasser: `PrototypeDestinationFacing09F13` L11; `FormationBounds` L13.

Indgange og kildeankre: `AutoCreate` L29; `Awake` L38; `LateUpdate` L61; `EnsureArrow` L130; `CalculateBounds` L156; `GroundPoint` L180.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeDismountedDragonFire09F30J.cs` (459 linjer; `d7dbd2ae1856`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeDismountedDragonFire09F30J.cs).

Klasser: `PrototypeDismountedDragonFire09F30J` L9; `State` L11.

Indgange og kildeankre: `AutoCreate` L35; `Awake` L42; `OnDestroy` L50; `GetAmmoRoundsPerMan` L55; `GetTargetName` L62; `GetFirePolicy` L69; `SetFirePolicy` L77; `GetFirePolicyLabel` L95; `GetSelectedRange` L100; `Update` L105; `GetState` L140; `FindTarget` L157; `Fire` L201; `CreateLine` L229; `UpdateCones` L249; `SetConeEmphasis` L280; `BuildFan` L300; `CreateSmoke` L371; `EmitSmoke` L395; `GetPolicyRange` L417; `FirePolicyLabel` L432; `Terrain` L447; `PlanarDistance` L453.

```csharp
L14: public float AmmoRoundsPerMan = 20f;
L23: private const float CloseRange = 35f;
L24: private const float MediumRange = 70f;
L25: private const float LongRange = 100f;
L26: private const float HalfArcDegrees = 35f;
L27: private const float ReloadSeconds = 7.0f;
L28: private const int ArcSegments = 32;
```


#### K `PrototypeDualStandards09L.cs` (300 linjer; `f6ae86d81a89`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeDualStandards09L.cs).

Klasser: `PrototypeDualStandards09L` L9; `StandardState` L11.

Indgange og kildeankre: `AutoCreate` L24; `Update` L33; `HideLegacyStandards` L68; `BuildDualStandards` L90; `BuildFlag` L145; `AddText` L241; `Animate` L287.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeDualStandardsAuthority09L.cs` (74 linjer; `c085b46a20bc`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeDualStandardsAuthority09L.cs).

Klasser: `PrototypeDualStandardsAuthority09L` L6.

Indgange og kildeankre: `AutoCreate` L12; `Update` L21.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeEnemyTestToggle09F29Y.cs` (265 linjer; `80e19c726ecd`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeEnemyTestToggle09F29Y.cs).

Klasser: `PrototypeEnemyTestToggle09F29Y` L10.

Indgange og kildeankre: `AutoCreate` L30; `Awake` L37; `Update` L48; `EnsureLegacyPanelSuppressed` L57; `SyncLegacyPointerRect` L66; `TryApplyStartupOff` L75; `GetPanelRect` L143; `DrawEnemyToggle` L156; `ToggleEnemy` L172; `ResolveEnemy` L203; `ConsumePointer` L258.

```csharp
L12: private const float PanelWidth = 188f;
L13: private const float PanelXMargin = 8f;
L14: private const float PanelY = 64f;
L15: private const float HeaderHeight = 25f;
L16: private const float RowHeight = 25f;
L17: private const float Gap = 3f;
```


#### T `PrototypeEnemyThreatCones09F29O.cs` (277 linjer; `3800b32e34df`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeEnemyThreatCones09F29O.cs).

Klasser: `PrototypeEnemyThreatCones09F29O` L11; `FormationBounds` L13.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L35; `LateUpdate` L50; `GetSelectedDanishCompanies` L74; `IsNearAnySelected` L86; `ApplyEnemyFans` L97; `EnsureFanTransform` L135; `BuildFan` L165; `CalculateBounds` L235; `TerrainPoint` L259; `Flat` L265; `PlanarDistance` L271.

```csharp
L20: private const float PreviewDistance = 180f;
L21: private const int ArcSegments = 40;
L22: private const float GroundOffset = 0.43f;
```


#### K/R/T `PrototypeExpandedOOBManager.cs` (128 linjer; `46390a70aaaa`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeExpandedOOBManager.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeExpandedOOBManager.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeExpandedOOBManager.cs).

Klasser: `PrototypeExpandedOOBManager` L4.

Indgange og kildeankre: `AutoCreate` L9; `Update` L18; `EnsureRegiment` L77; `SetPose` L102; `FindRegiment` L114.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeF9VisualOrders09F4.cs` (242 linjer; `ebf5396e5c9f`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeF9VisualOrders09F4.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeF9VisualOrders09F4.cs).

Klasser: `PrototypeF9VisualOrders09F4` L8.

Indgange og kildeankre: `AutoCreate` L19; `Awake` L28; `Update` L37; `OnDisable` L52; `IsCombatPanelVisible` L57; `GetMainCombatRect` L67; `GetExtensionRect` L73; `GetFirstSelectedDanish` L173; `BeginOrder` L193; `SuppressPointerHandlers` L207; `RestorePointerHandlers` L224.

```csharp
L76: const float extensionHeight = 176f;
```


#### R/T `PrototypeFightingWithdrawal09F10.cs` (454 linjer; `e9842774f043`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFightingWithdrawal09F10.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFightingWithdrawal09F10.cs).

Klasser: `PrototypeFightingWithdrawal09F10` L15; `WithdrawalState` L23.

Tilstands-/data-enums: `PrototypeWithdrawalRange09F10` = Medium, Long, OutOfRange (L4).

Indgange og kildeankre: `AutoCreate` L57; `Awake` L66; `OnDestroy` L81; `BeginForSelected` L87; `IsWithdrawing` L115; `StartWithdrawal` L120; `Update` L161; `AddComplete` L228; `BeginBackstep` L235; `UpdateBackstep` L274; `CompleteBackstep` L313; `Finish` L327; `Cancel` L343; `GetTargetDistance` L355; `GetFirePauseSeconds` L371; `FaceThreat` L376; `FindNearestEnemy` L390; `SegmentTouchesOpenWater` L418; `IsInOpenWater` L430; `StreamCenterX` L443; `PlanarDistance` L448.

```csharp
L42: private const float BackstepDistance = 10.0f;
L43: private const float BackwardSpeed = 1.70f;
L44: private const float OutOfRangeMargin = 15.0f;
L45: private const float PositionArrival = 0.35f;
L46: private const float FacingTurnSpeed = 16.8f;
L51: private const float RiverHalfWidth = 2.75f;
L52: private const float BridgeZ = 22.0f;
L53: private const float BridgeHalfLengthX = 9.0f;
L54: private const float BridgeHalfWidthZ = 4.0f;
L420: const int samples = 24;
```


#### R/T `PrototypeFireArcGuard09F7.cs` (101 linjer; `9b2f914bb2cf`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFireArcGuard09F7.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFireArcGuard09F7.cs).

Klasser: `PrototypeFireArcGuard09F7` L8.

Indgange og kildeankre: `AutoCreate` L14; `Update` L23; `LateUpdate` L49; `FindNearestEnemyInTriggerRange` L59; `PlanarAngle` L86; `PlanarDistance` L95.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R `PrototypeFireVisuals09F8.cs` (297 linjer; `e4a4649c5f17`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFireVisuals09F8.cs).

Klasser: `PrototypeFireVisuals09F8` L9; `FormationState` L11; `FormationBounds` L18.

Indgange og kildeankre: `AutoCreate` L38; `Awake` L47; `OnDestroy` L66; `LateUpdate` L72; `IsFormationFireReady` L116; `UpdateAndGetFormationReady` L121; `CalculateReformSeconds` L164; `BuildFan` L184; `ConfigureLine` L241; `CalculateBounds` L255; `TerrainPoint` L279; `PlanarNormalized` L285.

```csharp
L32: private const int ArcSegments = 40;
L33: private const float GroundOffset = 0.42f;
L34: private const float ReformSpeedMetresPerSecond = 2.8f;
L35: private const float ReformSafetySeconds = 0.30f;
```


#### T `PrototypeFireVisuals09F8.cs` (336 linjer; `aa850e42d756`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFireVisuals09F8.cs).

Klasser: `PrototypeFireVisuals09F8` L10; `FormationState` L12; `FormationBounds` L19.

Indgange og kildeankre: `AutoCreate` L39; `Awake` L48; `OnDestroy` L68; `LateUpdate` L74; `IsFormationFireReady` L133; `IsChargeFireSuppressed` L140; `UpdateAndGetFormationReady` L147; `CalculateReformSeconds` L190; `BuildFan` L210; `ConfigureLine` L268; `CalculateBounds` L294; `TerrainPoint` L318; `PlanarNormalized` L324.

```csharp
L33: private const int ArcSegments = 40;
L34: private const float GroundOffset = 0.42f;
L35: private const float ReformSpeedMetresPerSecond = 2.8f;
L36: private const float ReformSafetySeconds = 0.30f;
```


#### R/T `PrototypeFlagTextCleanup09F5.cs` (36 linjer; `771a8e93b7eb`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFlagTextCleanup09F5.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFlagTextCleanup09F5.cs).

Klasser: `PrototypeFlagTextCleanup09F5` L5.

Indgange og kildeankre: `AutoCreate` L8; `LateUpdate` L17.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeFlags09F4.cs` (314 linjer; `6af874cacbce`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFlags09F4.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFlags09F4.cs).

Klasser: `PrototypeFlags09F4` L9.

Indgange og kildeankre: `AutoCreate` L17; `Awake` L26; `Update` L31; `SuppressLegacyFlagSystems` L49; `RemoveLegacyFlagChildren` L67; `CreateFlagSet` L84; `CreateDanishNationalFlag` L110; `CreateDanishRegimentalStandard` L131; `CreateMottoRibbon` L171; `CreateBorder` L184; `CreateLionMark` L192; `CreateRing` L204; `CreatePrussianNationalFlag` L216; `CreatePrussianRegimentalStandard` L232; `Mat` L248; `CreatePole` L253; `CreateFinial` L264; `CreateBox` L275; `CreateText` L291.

```csharp
L206: const int pieces = 16;
L207: const float radius = 0.30f;
```


#### R/T `PrototypeForcedMarch09F7.cs` (247 linjer; `85f852a17456`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeForcedMarch09F7.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeForcedMarch09F7.cs).

Klasser: `PrototypeForcedMarch09F7` L9.

Indgange og kildeankre: `AutoCreate` L28; `Awake` L37; `OnDestroy` L66; `Update` L72; `HasDestination` L118; `IsMoving` L125; `IsForcedMarch` L130; `Toggle` L135; `ToggleSelectedDanish` L152; `GetFirstSelectedDanish` L201; `Cleanup` L214; `GetWalkSpeed` L233; `GetForcedSpeed` L238; `GetRoutRunSpeed` L243.

```csharp
L19: private const float DenmarkWalkSpeed = 3.20f;
L20: private const float PrussiaWalkSpeed = 3.35f;
L21: private const float DenmarkForcedSpeed = 4.25f;
L22: private const float PrussiaForcedSpeed = 4.45f;
L23: private const float DenmarkRoutRunSpeed = 5.20f;
L24: private const float PrussiaRoutRunSpeed = 5.40f;
L25: private const float ForcedMarchExtraCohesionCostPerSecond = 0.22f;
```


#### R `PrototypeFormationAttackLanes09F17.cs` (154 linjer; `e9dcaf25c402`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFormationAttackLanes09F17.cs).

Klasser: `PrototypeFormationAttackLanes09F17` L10.

Indgange og kildeankre: `AutoCreate` L19; `Awake` L28; `Update` L44; `ConvertToFormationAttack` L78.

```csharp
L14: private const float CompanySpacing = 60f;   // ~48 m frontage + ~12 m interval.
L15: private const float MinStandoff = 28f;
L16: private const float MaxStandoff = 88f;
```


#### T `PrototypeFormationAttackLanes09F17.cs` (309 linjer; `1c943c46e8bf`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFormationAttackLanes09F17.cs).

Klasser: `PrototypeFormationAttackLanes09F17` L19; `ManualLaneAssignment` L21.

Indgange og kildeankre: `AutoCreate` L40; `Awake` L49; `Update` L65; `UpdateExistingAssignments` L81; `DetectNewManualAttackGroups` L142; `IsEligibleForNewManualLane` L174; `IsStillManual` L188; `CreateManualFormationAttack` L197; `IsNewDirectPlayerOrderInput` L293; `PlanarDistance` L303.

```csharp
L33: private const float CompanySpacing = 60f;   // ~48 m frontage + ~12 m interval.
L34: private const float MinStandoff = 28f;
L35: private const float MaxStandoff = 88f;
L36: private const float LaneArrival = 5.0f;
L37: private const float ReassertInterval = 0.45f;
```


#### R/T `PrototypeFormationFireGuard09F8.cs` (58 linjer; `f5be1fcdbd63`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFormationFireGuard09F8.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFormationFireGuard09F8.cs).

Klasser: `PrototypeFormationFireGuard09F8` L7.

Indgange og kildeankre: `AutoCreate` L13; `Update` L22; `LateUpdate` L49.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeFormationMotion09F3.cs` (318 linjer; `1a22206674e5`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeFormationMotion09F3.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFormationMotion09F3.cs).

Klasser: `PrototypeFormationMotion09F3` L11; `MotionState` L13.

Indgange og kildeankre: `AutoCreate` L37; `Awake` L46; `LateUpdate` L59; `InitializeState` L91; `UpdateFormationMotion` L114; `UpdateFacingMotion` L174; `IsMoving` L235; `GetFormationPosition` L244; `EnsureSoldierCache` L269; `Cleanup` L296.

```csharp
L30: private const float FormationMoveSpeed = 2.15f;
L31: private const float SoldierTurnSpeed = 150f;
L32: private const float RegimentTurnSpeed = 16.8f;
L33: private const float FormationArrival = 0.035f;
L34: private const float TurnArrivalDegrees = 0.20f;
L251: const int ranks = 3;
L260: const int columnCount = 4;
```


#### T `PrototypeFormationSlotSafety09F23.cs` (309 linjer; `d95106f6a31d`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeFormationSlotSafety09F23.cs).

Klasser: `PrototypeFormationSlotSafety09F23` L11.

Indgange og kildeankre: `ResolveOnFormationLine` L42; `TryEmergencySearch` L126; `IsEmergencyCandidate` L190; `IsLegalFormationEndpoint` L203; `IsFormationFootprintClearOfRiver` L247; `ConflictsWithReserved` L269; `ContainsHardBuilding` L282; `Flat` L289; `Ground` L297; `PlanarDistance` L303.

```csharp
L13: public const float CompanyMinCentreSpacing = 68f;
L135: const float step = 20f;
L136: const float maxRadius = 400f;
```


#### K `PrototypeFullScaleOOB09K.cs` (314 linjer; `1024057c205e`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeFullScaleOOB09K.cs).

Klasser: `PrototypeRegimentOOB09K` L10; `CompanyState` L13; `BattalionState` L21; `PrototypeFullScaleOOBManager09K` L178.

Indgange og kildeankre: `Build` L53; `OverrideHistoricalIdentity` L104; `LogIdentity` L117; `ApplyHistoricalIdentity` L133; `Roman` L165; `AutoCreate` L185; `Awake` L194; `Update` L207; `ApplyStrength` L269; `SetPose` L280; `IsPilotRegiment` L292; `FindRegiment` L300.

```csharp
L65: const int companiesPerBattalion = 4;
```


#### K `PrototypeFullScaleRenderer09K.cs` (585 linjer; `85fa48a8c285`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeFullScaleRenderer09K.cs).

Klasser: `PrototypeFullScaleRenderer09K` L9; `UnitRenderState` L11.

Indgange og kildeankre: `AutoCreate` L57; `Awake` L66; `Start` L75; `Update` L80; `IsFullScalePilotRegiment` L139; `DisableRepresentativeVisualLayers` L147; `BuildState` L164; `GetCurrentProfile` L184; `RefreshUniformColors` L197; `RebuildSlots` L212; `BuildBattalionLineSlots` L258; `BuildMarchColumnSlots` L313; `BuildFallbackSlots` L326; `AddSlot` L337; `DrawRegiment` L343; `FlushBatch` L427; `UpdateReloadState` L435; `GetReloadProgress` L458; `HideRepresentativeSoldierRenderers` L465; `BuildMountedRegimentalHQ` L479; `UpdateHqVisibility` L528; `CreatePrimitive` L538; `CreateInstancedMaterial` L561; `BuildPrimitiveMeshes` L568; `ExtractMesh` L576.

```csharp
L21: public int LastInitialStrength = -1;
L48: private const int BatchSize = 1023;
L49: private const float FileSpacing = 0.64f;
L50: private const float RankSpacing = 0.78f;
L51: private const int CompanyRanks = 3;
L52: private const float CompanyGap = 3.2f;
L53: private const float CompanyRowGap = 4.5f;
L54: private const float BattalionDepthGap = 8.5f;
L315: const int filesAcross = 12;
```


#### R `PrototypeGroupLineSpacing09F17.cs` (284 linjer; `3e1e4b5eed21`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeGroupLineSpacing09F17.cs).

Klasser: `PrototypeGroupLineSpacing09F17` L10.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L36; `Update` L53; `CorrectCreatedRoutes` L106; `CorrectPreview` L179; `GetSelectedDanish` L211; `GetAutomaticFacing` L226; `GetEnemyUnderMouse` L253; `TryGetGroundPoint` L267.

```csharp
L12: private const float FormationDragThreshold = 4f;
L13: private const float CompanyFrontage = 48f;
L14: private const float CompanySpacing = 60f; // 48 m frontage + ~12 m interval.
L200: const int segments = 32;
```


#### T `PrototypeGroupLineSpacing09F17.cs` (356 linjer; `61247a841202`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeGroupLineSpacing09F17.cs).

Klasser: `PrototypeGroupLineSpacing09F17` L13.

Indgange og kildeankre: `AutoCreate` L30; `Awake` L39; `Update` L56; `CorrectCreatedRoutes` L109; `BestAssignment` L200; `SearchAssignment` L211; `CorrectPreview` L251; `GetSelectedDanish` L283; `GetAutomaticFacing` L298; `GetEnemyUnderMouse` L325; `TryGetGroundPoint` L339.

```csharp
L15: private const float FormationDragThreshold = 4f;
L16: private const float CompanyFrontage = 48f;
L17: private const float CompanySpacing = 60f;
L272: const int segments = 32;
```


#### T `PrototypeHierarchyVisualHotfix09F29A.cs` (323 linjer; `9efc6779e61c`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHierarchyVisualHotfix09F29A.cs).

Klasser: `PrototypeHierarchyVisualHotfix09F29A` L14.

Indgange og kildeankre: `AutoCreate` L36; `Awake` L43; `LateUpdate` L55; `ResolveHierarchy` L78; `SelectedBattalionIndex` L86; `RefreshBattalionCommandLinks` L95; `RefreshRegimentalCommandLinks` L140; `RefreshOfficerTargetRing` L171; `EnsureTargetRing` L216; `TryGetGround` L237; `SetTerrainFollowingConnection` L264; `DrawTerrainCircle` L282; `ColorFor` L299; `CreateUnlit` L310.

```csharp
L30: private const float LinkHeight = 0.82f;
L31: private const float TargetHeight = 0.86f;
L32: private const int LinkSamples = 28;
L33: private const int RingSamples = 72;
```


#### T `PrototypeHigherCommandHQ09F30B.cs` (1479 linjer; `21bd0b0c2291`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHigherCommandHQ09F30B.cs).

Klasser: `PrototypeHigherCommandMarker09F30B` L12; `PrototypeHigherCommandHQ09F30B` L22; `PrototypeHigherCommandBillboard09F30B` L1467.

Tilstands-/data-enums: `PrototypeHigherCommandLevel09F30B` = None, Brigade, Division (L5).

Indgange og kildeankre: `AutoCreate` L110; `Awake` L117; `OnDestroy` L132; `Update` L138; `TryInstall` L156; `ConfigureCommandParents` L199; `ConfigureCavalry` L210; `SelectLevel` L220; `GetAIEnabled` L241; `GetDoctrine` L248; `ToggleAI` L255; `CascadeSubordinateAI` L285; `SetDoctrine` L306; `ClearSelectionOnly` L316; `FocusBehindExternal` L323; `GetCavalryCommandParent` L348; `ToggleCavalryAttachment` L356; `ClearLowerSelection` L378; `HandlePendingOrderInput` L403; `IssueHigherOrder` L419; `CommitHigherOrder` L433; `BeginTemporaryAttackAttachments` L485; `SetTemporaryCavalryParent` L566; `UpdateTemporaryAttackAttachments` L580; `IsCavalryInCommittedCharge` L597; `ReleaseTemporaryAttackAttachments` L602; `ReleaseCavalryToReserve` L622; `ResolveCommandParentTransform` L662; `IssueAttachedCavalryMission` L679; `IssueCavalrySupportToUnit` L709; `IsCavalrySubordinateToLevel` L768; `HasHigherOrderActive` L789; `IsHigherHqStillMoving` L820; `IsCavalryExecuting` L859; `HandleWorldSelection` L873; `UpdateHqFollow` L912; `MoveHqToward` L949; `CreateLinks` L971; `UpdateCommandLinks` L992; `UpdateCavalryLink` L1038; `CreateLine` L1054; `SetTerrainLink` L1068; `SetCircle` L1086; `CreateHigherHq` L1106; `CreateMountedStaff` L1145; `TryGetGround` L1187; `IsPointerOverUi` L1221; `HigherOrderLabel` L1345; `AggregateHigherStrength` L1359; `DrawOrderButton` L1385; `DrawAttachmentButton` L1401; `PlanarDistance` L1441; `Flat` L1448; `Ground` L1454; `LateUpdate` L1470.

```csharp
L40: private const float HudHeight = 96f;
L44: private const float BrigadeFollowDistance = 120f;
L45: private const float BrigadeLateralOffset = 65f;
L46: private const float DivisionFollowDistance = 145f;
L47: private const float DivisionLateralOffset = -75f;
L48: private const float BrigadeMoveSpeed = 6.2f;
L49: private const float DivisionMoveSpeed = 5.9f;
L52: private const float BrigadeCommandInner = 1350f;
L53: private const float BrigadeCommandOuter = 1850f;
L54: private const float DivisionCommandInner = 2100f;
L55: private const float DivisionCommandOuter = 2850f;
L58: private const float DefendCavalryRearDepth = 150f;
L59: private const float DefendCavalryOutwardOffset = 45f;
L61: private const int LinkSamples = 28;
L1094: const int segments = 56;
L1306: const float gap = 4f;
L1308: const float commandHeight = 27f;
```


#### T `PrototypeHigherCommandOob09F30B.cs` (430 linjer; `150831fb815a`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHigherCommandOob09F30B.cs).

Klasser: `PrototypeHigherCommandOob09F30B` L10.

Indgange og kildeankre: `AutoCreate` L51; `Awake` L58; `OnDestroy` L71; `Update` L77; `DisableLegacyPanelsOnceReady` L88; `IsPointerOverPanel` L106; `GetPanelRect` L114; `DrawCavalryRow` L264; `DrawRow` L283; `GetSelectedBattalion` L297; `AggregateStrength` L305; `AggregateBattalionStrength` L315; `GetCompanyStatus` L327; `ShortCavalryStatus` L345; `RegisterClick` L354; `ConsumePointer` L363.

```csharp
L13: private const float PanelX = 8f;
L14: private const float PanelY = 39f;
L15: private const float PanelWidth = 318f;
L16: private const float HeaderHeight = 27f;
L17: private const float RowHeight = 22f;
L18: private const float SectionHeight = 18f;
L19: private const float DoubleClickSeconds = 0.34f;
L23: private bool open = true;
L27: private float lastClickAt = -10f;
```


#### T `PrototypeHigherCommandOob09F30C.cs` (384 linjer; `415299cc9b1d`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHigherCommandOob09F30C.cs).

Klasser: `PrototypeHigherCommandOob09F30C` L9.

Indgange og kildeankre: `AutoCreate` L47; `Awake` L54; `OnDestroy` L66; `Update` L72; `DisableOldPanel` L85; `IsPointerOverPanel` L96; `GetPanelRect` L104; `DrawHigherRow` L223; `DrawAttachedForParent` L235; `DrawCavalryIfParent` L243; `DrawRow` L264; `GetSelectedBattalion` L274; `AggregateStrength` L281; `AggregateBattalionStrength` L289; `GetCompanyStatus` L298; `ShortCavalryStatus` L316; `RegisterClick` L324; `ConsumePointer` L333.

```csharp
L12: private const float PanelX = 8f;
L13: private const float PanelY = 39f;
L14: private const float PanelWidth = 326f;
L15: private const float HeaderHeight = 27f;
L16: private const float RowHeight = 22f;
L17: private const float DoubleClickSeconds = 0.34f;
L21: private bool open = true;
L25: private float lastClickAt = -10f;
```


#### T `PrototypeHigherCommandOob09F30D.cs` (587 linjer; `81bedcb00a53`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHigherCommandOob09F30D.cs).

Klasser: `PrototypeHigherCommandOob09F30D` L9.

Indgange og kildeankre: `AutoCreate` L56; `Awake` L63; `OnDestroy` L75; `Update` L81; `DisableLegacyPanels` L97; `IsPointerOverPanel` L118; `GetPanelRect` L126; `ComputeContentHeight` L137; `DrawColumnHeaders` L309; `DrawHigherRow` L319; `DrawAttachedForParent` L336; `DrawCavalryIfParent` L344; `HandleCavalryDrag` L367; `HandleDropTarget` L407; `DrawRow` L432; `DrawRowVisual` L443; `RowRect` L459; `GetSelectedBattalion` L464; `AggregateStrength` L471; `AggregateBattalionStrength` L479; `GetCompanyStatus` L488; `ShortCavalryStatus` L506; `RegisterClick` L515; `ConsumePointer` L524.

```csharp
L12: private const float PanelX = 8f;
L13: private const float PanelY = 39f;
L14: private const float PanelWidth = 535f;
L15: private const float HeaderHeight = 27f;
L16: private const float ColumnHeaderHeight = 16f;
L17: private const float RowHeight = 22f;
L18: private const float DoubleClickSeconds = 0.34f;
L19: private const float DragThreshold = 5f;
L23: private bool open = true;
L28: private float lastClickAt = -10f;
```


#### T `PrototypeHqDepthGuard09F29W.cs` (324 linjer; `aab7187ce6c8`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHqDepthGuard09F29W.cs).

Klasser: `PrototypeHqDepthGuard09F29W` L11.

Indgange og kildeankre: `AutoCreate` L29; `Awake` L35; `Update` L50; `GuardMajors` L64; `GuardRegimentalHq` L130; `ActiveCenter` L182; `FindNearestEnemy` L197; `IsAdequatelyBehind` L220; `RearDepth` L235; `TrySafeRearGoal` L242; `IsSafeHqRelocation` L271; `BankSide` L285; `ReadBool` L294; `ReadVector` L300; `Flat` L306; `Ground` L312; `PlanarDistance` L318.

```csharp
L15: private const float ReviewInterval = 0.50f;
L16: private const float MajorMinBehind = 120f;
L17: private const float MajorPreferredBehind = 175f;
L18: private const float MajorMaxLateral = 240f;
L19: private const float RegimentMinBehindMajors = 220f;
L20: private const float RegimentPreferredBehindMajors = 285f;
L21: private const float RegimentMaxLateral = 320f;
```


#### T `PrototypeHqFollow09F29L.cs` (291 linjer; `126847c8e6d2`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeHqFollow09F29L.cs).

Klasser: `PrototypeHqFollow09F29L` L11.

Indgange og kildeankre: `AutoCreate` L41; `Awake` L47; `Update` L66; `UpdateMajorFollow` L72; `UpdateRegimentalFollow` L148; `ResolveMissionForward` L211; `FindNearestEnemy` L229; `SafeMajorRelocation` L251; `SafeRegimentalRelocation` L259; `ReadBool` L267; `Ground` L273; `Flat` L279; `PlanarDistance` L285.

```csharp
L19: private const float MajorBehind = 80f;
L20: private const float MajorRepositionDelta = 42f;
L21: private const float RegimentalBehind = 160f;
L22: private const float RegimentalRepositionDelta = 68f;
```


#### K `PrototypeIndependentCompanyMovement09L5.cs` (321 linjer; `101e09573054`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeIndependentCompanyMovement09L5.cs).

Klasser: `PrototypeIndependentCompanyMovement09L5` L10; `Snapshot` L12.

Indgange og kildeankre: `AutoCreate` L33; `Awake` L42; `Start` L48; `Update` L53; `EnsureReflection` L89; `DetachCompanies` L108; `ConvertWholeRegimentSelectionToCompanies` L128; `SuppressParentMovementWriters` L176; `BeginGroupOrder` L199; `UpdateGroupOrder` L226; `CompleteGroupOrder` L232; `TryGetGroundPoint` L287.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeInfantryCharge09F25.cs` (434 linjer; `905c526bb4ee`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeInfantryCharge09F25.cs).

Klasser: `PrototypeInfantryCharge09F25` L12; `ChargeState` L14.

Indgange og kildeankre: `AutoCreate` L50; `Awake` L56; `OnDestroy` L85; `Update` L91; `LateUpdate` L119; `IsCharging` L150; `IsChargeMeleeParticipant` L157; `HasChargeMomentum` L175; `BeginTargetPickFromSelection` L182; `IssueCharge` L203; `UpdateCharges` L251; `FinishCharge` L326; `ClearPlayerRoute` L343; `GetEnemyUnderMouse` L350; `PointerOverControls` L368; `HasSelectedDanishCompany` L406; `AddFinish` L417; `PlanarDistance` L428.

```csharp
L42: private const float ContactDistance = 2.2f;
L43: private const float SteerInterval = 0.28f;
L44: private const float DenmarkChargeSpeed = 4.80f;
L45: private const float PrussiaChargeSpeed = 5.00f;
L46: private const float ExtraCohesionCostPerSecond = 0.30f;
L47: private const float ChargeMomentumSeconds = 2.6f;
```


#### T `PrototypeInfantrySquare09F29.cs` (752 linjer; `41cf654a5453`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeInfantrySquare09F29.cs).

Klasser: `PrototypeInfantrySquare09F29` L13; `SquareState` L15.

Indgange og kildeankre: `AutoCreate` L59; `Awake` L66; `OnDestroy` L88; `OnDisable` L95; `Update` L100; `LateUpdate` L181; `HandlePlayerInput` L197; `IsInSquare` L242; `IsSquareReady` L247; `GetMountedChargeDefenseMultiplier` L254; `ReportMountedThreat` L263; `HandleMountedThreat` L283; `EnterSquare` L331; `LeaveSquare` L370; `CaptureAccuracy` L398; `ApplySquareFireFactor` L409; `RestoreAccuracy` L416; `CaptureCollider` L422; `ForceSquareCollider` L431; `RestoreCollider` L438; `HasDestination` L447; `ResolveVisualReflection` L455; `ForceSquareVisual` L470; `GetSquareHalfSide` L508; `SquarePosition` L515; `FindNearestEnemy` L534; `TurnSquareTowardThreat` L555; `SelectedDanishCompanies` L568; `BuildMaterials` L579; `MakeMaterial` L587; `CreateWorldVisual` L595; `MakeLine` L606; `UpdateWorldVisual` L621; `DrawWorldSquare` L637; `DrawWorldCircle` L661; `SuppressLegacyForwardFans` L680; `RestoreLegacyFans` L689; `SetChildLine` L695; `EnsureStatusStyle` L704; `RestoreAll` L737; `PlanarDistance` L746.

```csharp
L52: private const float SquareVisualSpeed = 8.0f;
L53: private const float SquareAccuracyFactor = 0.42f;
L54: private const float ThreatHoldSeconds = 7.5f;
L55: private const float AutoSquareThreshold = 62f;
L56: private const float MaxThreatDistance = 220f;
L669: const int segments = 64;
```


#### K `PrototypeKampCommandAuthority09M2.cs` (394 linjer; `90eaac028c17`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampCommandAuthority09M2.cs).

Klasser: `PrototypeKampCommandAuthority09M2` L13.

Tilstands-/data-enums: `PrototypeKampCommandAuthorityLevel09M2` = Manual, RegimentOfficer, BrigadeOfficer (L4).

Indgange og kildeankre: `AutoCreate` L23; `Awake` L31; `OnDestroy` L41; `Update` L47; `GetLevel` L63; `GetLevelLabel` L79; `SetManual` L92; `SetRegimentOfficer` L103; `SetBrigadeOfficer` L114; `ShouldKeepOfficerAI` L131; `ClearDanishBrigadeOverrides` L144; `BrigadeMayCommand` L152; `CaptureManualOverride` L157; `SyncPivotsAndIssue` L173; `IssueFromOfficer` L203; `ResolveIntent` L234; `CloseOrHold` L280; `FaceToward` L298; `SnapEmptyPivot` L306; `TryGetFormationPose` L316; `FindNearestEnemy` L345; `IsPlayerOverride` L368; `IsLocalOfficer` L374; `IsBrigadeCommanding` L380; `Horizontal` L388.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeKampCommandQa09M1.cs` (421 linjer; `a3574a73edce`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampCommandQa09M1.cs).

Klasser: `PrototypeKampCommandQa09M1` L7; `HqState` L9.

Indgange og kildeankre: `AutoCreate` L28; `Update` L36; `SuppressConflictingLayers` L54; `GroundSelectionBoxes` L87; `GetOrCaptureOutline` L126; `UpdateRegimentalHqs` L138; `GetOrBuildHq` L155; `BuildMountedOfficers` L174; `BuildCarriedFlag` L201; `PlaceHqBehindCompanies` L217; `UpdateRangeFans` L248; `GetSelectedParents` L263; `CreateFan` L279; `RebuildFan` L293; `GetMenuRegiment` L330; `GetPanelRect` L355; `IsPointerOverControls` L362.

```csharp
L297: const int arcSegments = 28;
L298: const float muzzleHalfWidth = 6.5f;
L299: const float muzzleZ = 1.10f;
L300: const float halfAngle = 60f;
L357: const float height = 62f;
L381: const float h = 24f;
L382: const float gap = 4f;
```


#### K `PrototypeKampCompanyFire09M.cs` (246 linjer; `00b9ff12f0b1`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampCompanyFire09M.cs).

Klasser: `PrototypeKampCompanyFire09M` L6; `FireState` L10.

Indgange og kildeankre: `NotifyPlayerFirePolicy` L27; `AutoCreate` L38; `Awake` L46; `OnDestroy` L51; `TryGetLastVolley` L57; `TryGetReloadWindow` L65; `Update` L76; `AllowsCompanyVolley` L107; `GetState` L123; `CreateCompanySmoke` L137; `FindTarget` L165; `FireVolley` L202.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeKampCompanyHud09M1.cs` (180 linjer; `a485ab7ce08b`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampCompanyHud09M1.cs).

Klasser: `PrototypeKampCompanyHud09M1` L6.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24; `DrawWorldChips` L47; `DrawSelectedCard` L76; `ShortName` L139.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeKampCompanyMarch09M1.cs` (279 linjer; `d8efca376f77`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampCompanyMarch09M1.cs).

Klasser: `PrototypeKampCompanyMarch09M1` L11; `State` L24.

Tilstands-/data-enums: `MarchStatus` = Hold, ColumnMarch, Deploying, Contact, Manual (L15).

Indgange og kildeankre: `AutoCreate` L45; `Awake` L54; `OnDestroy` L62; `GetStatus` L68; `GetStatusLabel` L75; `Update` L92; `NoteManualFormationLocks` L119; `ApplyPolicy` L137; `DeployLine` L190; `SetStatus` L202; `GetState` L210; `GetRemainingPathDistance` L220; `GetNearestEnemyDistance` L252.

```csharp
L39: private const float LongMarchMetres = 18f;
L40: private const float DeployBeforeDestinationMetres = 14f;
L41: private const float ContactBufferMetres = 8f;
L42: private const float ManualLockSeconds = 14f;
```


#### R `PrototypeKampRebuildLook09V.cs` (458 linjer; `3ff1569438e5`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeKampRebuildLook09V.cs).

Klasser: `PrototypeKampRebuildLook09V` L5.

Indgange og kildeankre: `AutoCreate` L32; `Awake` L39; `OnDestroy` L46; `LateUpdate` L51; `ContainsPointer` L58; `ApplyFlagVisibility` L64; `UpdateSelectionMarking` L77; `DockWidth` L167; `DockHeight` L172; `DockRect` L177; `UnitCardRect` L184; `CommandRect` L190; `DrawCompass` L208; `DrawMinimap` L219; `DrawUnitCard` L239; `DrawBar` L285; `DrawCommandPanel` L292; `DrawMajorCommands` L305; `DrawCompanyCommands` L328; `Tiny` L373; `Mark` L380; `BeginOrder` L385; `ForEachSelected` L391; `BlockPointerOnHud` L403; `GetSelectedDanish` L415; `Solid` L427; `MakePortrait` L436.

```csharp
L7: public static bool HideLegacyBottomBar = true;
L26: private bool showFlags = true;
L27: private bool showMarking = true;
```


#### K `PrototypeKampRegimentFlags09M8.cs` (162 linjer; `38a6d10ec4c6`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampRegimentFlags09M8.cs).

Klasser: `PrototypeKampRegimentFlags09M8` L6; `FlagPair` L8.

Indgange og kildeankre: `AutoCreate` L19; `Awake` L27; `LateUpdate` L33; `PlacePair` L59; `BuildPair` L100; `BuildFlag` L109.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeKampSoldierVisual09M.cs` (528 linjer; `d72ca5fc01d4`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampSoldierVisual09M.cs).

Klasser: `PrototypeKampSoldierVisual09M` L8; `CompanyRenderState` L12; `Kit` L21.

Indgange og kildeankre: `AutoCreate` L65; `Awake` L73; `OnDestroy` L84; `Update` L89; `SuppressLegacyVisuals` L116; `HideLegacyRegimentSoldiers` L125; `GetState` L147; `GetKit` L158; `GetProfile` L183; `RefreshKit` L194; `RebuildSlots` L209; `DrawCompany` L253; `DrawLiving` L272; `DrawFallen` L410; `DrawPaired` L431; `DrawSingle` L443; `Grounded` L455; `UpdateReload` L463; `GetReloadProgress` L493; `Flush` L499; `Mat` L505; `Darken` L514; `Extract` L519.

```csharp
L17: public int LastInitialStrength = -1;
L57: private const float LineFileSpacing = 0.52f;
L58: private const float LineRankSpacing = 0.76f;
L59: private const int LineRanks = 3;
L60: private const int ColumnFiles = 8;
L61: private const float ColumnFileSpacing = 0.60f;
L62: private const float ColumnRankSpacing = 0.72f;
```


#### K `PrototypeKampTacticalFeedback09M3.cs` (246 linjer; `ccee40e61149`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeKampTacticalFeedback09M3.cs).

Klasser: `PrototypeKampTacticalFeedback09M3` L7.

Indgange og kildeankre: `AutoCreate` L22; `Awake` L29; `LateUpdate` L36; `TryFront` L105; `BuildPath` L132; `SnapOutOfRiver` L155; `SegmentCrossesRiver` L164; `IsRiverWater` L174; `IsBridgeZone` L179; `StreamCenterX` L186; `GetLine` L191; `Hide` L210; `DrawArc` L216; `TryPick` L230; `Ground` L241.

```csharp
L9: public const float RiverHalfWidth = 2.10f;
L10: public const float BridgeZ = 22.0f;
L11: public const float BridgeHalfLengthX = 10.0f;
L12: public const float BridgeHalfWidthZ = 5.5f;
L13: public const float BridgeApproachOffset = 14.0f;
L218: const int segs = 40;
```


#### T `PrototypeLateralManeuver09F29P.cs` (667 linjer; `d547a95f3475`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeLateralManeuver09F29P.cs).

Klasser: `PrototypeLateralManeuver09F29P` L11; `SideStepState` L13.

Indgange og kildeankre: `AutoCreate` L53; `Awake` L60; `Update` L80; `LateUpdate` L165; `SuppressParentMovementWhileStepping` L227; `Eligible` L240; `FindOverlappingFriendly` L280; `TryFindFireLaneConflict` L303; `ChooseFireLaneMover` L375; `IsProductivelyEngaging` L400; `ResolveLiveFireTarget` L420; `IsPotentialShooter` L467; `IsAutoTacticalUnit` L480; `IsValidEnemy` L488; `TryChooseOverlapSideStep` L494; `TryChooseFireLaneSideStep` L517; `FireLaneCandidateScore` L557; `ClearanceScore` L592; `Finish` L617; `Clamp` L636; `Flat` L648; `PlanarDistance` L654; `AddFinish` L661.

```csharp
L24: private const float ThinkInterval = 0.30f;
L25: private const float FriendlyTrigger = 13.0f;
L26: private const float DesiredClearance = 10.5f;
L27: private const float OverlapSideStepDistance = 12.0f;
L31: private const float FireLaneConflictHalfWidth = 55.0f;
L32: private const float FireLaneDesiredClearance = 60.0f;
L33: private const float MinFireLaneStep = 18.0f;
L34: private const float MaxFireLaneStep = 46.0f;
L36: private const float SideStepSpeed = 1.75f;
L37: private const float ArrivalDistance = 0.45f;
L38: private const float MaxDuration = 32.0f;
L39: private const float FireLaneSettleSeconds = 4.0f;
L40: private const float Cooldown = 4.0f;
L638: const float margin = 20f;
```


#### R `PrototypeLethalityCalibration09F14.cs` (206 linjer; `8159de83ae10`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeLethalityCalibration09F14.cs).

Klasser: `PrototypeLethalityCalibration09F14` L11; `State` L13.

Indgange og kildeankre: `AutoCreate` L26; `Awake` L35; `Update` L46; `ResolveAndLog` L77; `FindVolleyTarget` L135; `ReadNextFireTime` L174; `PlanarDistance` L180; `Cleanup` L187.

```csharp
L23: private const int LegacyKernelHitCap = 16;
```


#### T `PrototypeLethalityCalibration09F14.cs` (210 linjer; `7f536058faf8`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeLethalityCalibration09F14.cs).

Klasser: `PrototypeLethalityCalibration09F14` L11; `State` L13.

Indgange og kildeankre: `AutoCreate` L26; `Awake` L35; `Update` L46; `ResolveAndLog` L77; `FindVolleyTarget` L139; `ReadNextFireTime` L178; `PlanarDistance` L184; `Cleanup` L191.

```csharp
L23: private const int LegacyKernelHitCap = 16;
```


#### T `PrototypeLocalCaptainAuthority09F25.cs` (193 linjer; `e33a49da1b26`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeLocalCaptainAuthority09F25.cs).

Klasser: `PrototypeLocalCaptainAuthority09F25` L12.

Indgange og kildeankre: `AutoCreate` L23; `Update` L29; `Resolve` L128; `GetPlayerRoutes` L156; `AddRemove` L164; `OnDisable` L171; `OnDestroy` L176; `RestoreControllers` L181.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeMajorBattalion09F17.cs` (824 linjer; `80ee8815f81b`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorBattalion09F17.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorBattalion09F17.cs).

Klasser: `PrototypeMajorBattalion09F17` L8; `Mission` L21.

Indgange og kildeankre: `AutoCreate` L68; `Awake` L77; `OnDestroy` L87; `Update` L93; `TryTakeOwnership` L118; `CreateMaterials` L158; `CreateUnlit` L165; `CreateSelectionRing` L173; `CreateTargetRing` L186; `RefreshCompanies` L199; `Find` L225; `RebuildLinks` L235; `UpdateLinks` L257; `HandleSelectionAndTargetInput` L280; `RayHitsHQ` L312; `TryGetGround` L327; `IssueOrder` L346; `PrepareCompany` L417; `ApplyMoveForOrder` L425; `UpdateMissions` L435; `RunAIDecision` L500; `EvaluateFlankOpportunity` L545; `ChooseReserve` L551; `HoldAll` L569; `FindNearestEnemy` L584; `CompanyCenter` L602; `AverageStrengthRatio` L615; `AverageCohesion` L628; `FlatDirection` L641; `Ground` L648; `PlanarDistance` L654; `AddCompleted` L661; `UpdateTargetRing` L667; `UpdateSelectionRing` L689; `DrawCircle` L696; `PanelRect` L707; `IsPointerOverPanel` L714; `DisplayCompany` L801; `Label` L811.

```csharp
L62: private const float CompanySpacing = 60f;
L63: private const float ReserveDepth = 72f;
L64: private const float FlankOffset = 105f;
L65: private const float ArrivalDistance = 4.0f;
L710: const float height = 120f;
L765: const float infoWidth = 328f;
L787: const float gap = 5f;
L789: const float buttonHeight = 31f;
```


#### R/T `PrototypeMajorBattalion09F18.cs` (884 linjer; `795f00740fbb`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorBattalion09F18.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorBattalion09F18.cs).

Klasser: `PrototypeMajorBattalion09F18` L9; `Mission` L11.

Tilstands-/data-enums: `MajorOrder09F18` = None, AttackHere, DefendHere, WithdrawHere, AdvanceHere, HoldPosition, AssembleHere (L5).

Indgange og kildeankre: `AutoCreate` L59; `Awake` L65; `OnDestroy` L75; `Update` L83; `TryInstall` L109; `RefreshCompanies` L131; `FindOwn` L155; `SetSelected` L167; `ClearCompanySelection` L179; `ToggleAI` L200; `SetDoctrine` L207; `MoveMajor` L212; `ReleaseCompanyToManual` L220; `SyncManualAiState` L239; `IssueOrder` L281; `NewMission` L367; `Clone` L379; `Assign` L384; `ClearPlayerRoute` L424; `UpdateMissions` L431; `ShouldUseReserve` L482; `TryFlank` L491; `BestReserve` L513; `ReviewReserve` L529; `HoldAll` L579; `ThinkMajor` L614; `KnownEnemies` L653; `Nearest` L686; `Center` L702; `AverageStrength` L716; `AverageCohesion` L730; `SafeCompany` L744; `GoalSafe` L768; `SafeHq` L794; `UpdateHqMove` L803; `Role` L829; `Name` L842; `Label` L852; `Ground` L866; `Flat` L872; `Dist` L878.

```csharp
L50: private const float Spacing = 60f;
L51: private const float AttackStandoff = 68f;
L52: private const float ReserveDepth = 78f;
L53: private const float FlankOffset = 112f;
L54: private const float KnownRange = 700f;
L55: private const float Arrival = 5f;
L56: private const float HqSpeed = 6.2f;
```


#### T `PrototypeMajorCommandZone09F25.cs` (452 linjer; `3011a11567b7`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorCommandZone09F25.cs).

Klasser: `PrototypeMajorCommandZone09F25` L10.

Tilstands-/data-enums: `CommandBand` = InCommand, Extended, OutOfCommand (L12).

Indgange og kildeankre: `AutoCreate` L48; `Awake` L54; `OnDestroy` L64; `Update` L70; `GetBand` L90; `GetBandLabel` L103; `Resolve` L113; `ReviewHqPosition` L144; `HqPathClear` L227; `ApplyCommandDelay` L266; `GetBattalionCenter` L312; `FarthestCompanyDistance` L326; `EnsureRings` L338; `CreateRing` L352; `UpdateRings` L365; `DrawRing` L378; `CreateMaterial` L393; `ContainsCompany` L427; `FirstSelectedCompany` L435; `PlanarDistance` L446.

```csharp
L39: private const float PreferredAttackStandoff = 165f;
L40: private const float RelocationTrigger = 80f;
L41: private const float CompanyAdvanceTrigger = 235f;
L42: private const float InCommandRange = 320f;
L43: private const float ExtendedRange = 450f;
L44: private const float ExtendedReactionDelay = 0.15f;
L45: private const float OutReactionDelay = 0.40f;
```


#### T `PrototypeMajorFormationPlanner09F22.cs` (429 linjer; `b9626d37a4d8`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorFormationPlanner09F22.cs).

Klasser: `PrototypeMajorFormationPlanner09F22` L13.

Indgange og kildeankre: `AutoCreate` L33; `Update` L39; `ResolveMajor` L69; `Replan` L97; `GetObjectiveForward` L175; `FacingForSlot` L215; `NearestKnownEnemy` L226; `ApplyMission` L255; `UpdateSavedAssignment` L287; `SafeCompany` L305; `BestAssignment` L311; `SearchAssignment` L322; `AveragePosition` L358; `BuildSignature` L372; `ReadOrder` L379; `ReadBool` L387; `ReadVector` L393; `WriteVector` L399; `WriteBool` L405; `WriteFloat` L411; `Ground` L417; `PlanarDistance` L423.

```csharp
L27: private const float ReviewInterval = 0.20f;
L28: private const float CompanySpacing = 60f;
L29: private const float AttackStandoff = 68f;
L30: private const float ReserveDepth = 78f;
```


#### R/T `PrototypeMajorHQ09F15.cs` (801 linjer; `1f070b6dc71c`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorHQ09F15.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorHQ09F15.cs).

Klasser: `PrototypeMajorHQ09F15` L8; `UnitMission` L21.

Indgange og kildeankre: `AutoCreate` L65; `Awake` L74; `OnDestroy` L84; `Update` L90; `IsPointerOverControls` L125; `DebugRayHitsHQ` L135; `DebugSetSelected` L140; `DebugGetSubordinateCenter` L145; `DebugAverageStrengthRatio` L159; `DebugAverageCohesion` L173; `DebugFindNearestEnemyToPoint` L187; `DebugIssueHoldOrder` L192; `DebugIssueTargetedOrder` L197; `DebugHandlePendingTargetClick` L212; `TryCreateHQ` L226; `CreateMaterials` L264; `CreateUnlitMaterial` L277; `CreateHorse` L285; `CreatePrimitivePart` L325; `CreateHQFlag` L342; `CreateSelectionRing` L357; `CreateTargetPreview` L371; `RefreshSubordinates` L384; `FindRegiment` L400; `CreateCommandLinks` L411; `UpdateCommandLinks` L434; `SetSelected` L457; `RayHitsHQ` L469; `TryGetGroundPoint` L485; `IssueTargetedOrder` L505; `IssueHoldOrder` L554; `UpdateMissionStates` L570; `AddCompleted` L619; `FindNearestEnemy` L625; `FindNearestEnemyToPoint` L630; `PlanarDistance` L647; `UpdateTargetPreview` L654; `UpdateCircle` L671; `SetTargetPreviewVisible` L684; `GetOrderColor` L689; `GetOrderLabel` L700; `GetPanelRect` L714; `GetTargetHintRect` L721.

```csharp
L717: const float height = 104f;
L723: const float width = 430f;
L724: const float height = 28f;
L758: const float infoWidth = 282f;
L778: const float gap = 5f;
L780: const float buttonHeight = 29f;
```


#### R/T `PrototypeMajorHQ09F16.cs` (166 linjer; `2d2d3fac5de1`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorHQ09F16.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorHQ09F16.cs).

Klasser: `PrototypeMajorHQ09F16` L8.

Indgange og kildeankre: `AutoCreate` L21; `Awake` L30; `OnDestroy` L40; `Update` L46; `SetMajorAIEnabled` L64; `IsMouseOverHQ` L71; `GetHoverText` L76; `RunMajorAIDecision` L88; `FlatDirection` L126; `PlanarDistance` L135.

```csharp
L160: const float w = 250f;
L161: const float h = 76f;
```


#### T `PrototypeMajorHudStatus09F29Y.cs` (158 linjer; `aae5d5e94847`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorHudStatus09F29Y.cs).

Klasser: `PrototypeMajorHudStatus09F29Y` L9.

Indgange og kildeankre: `AutoCreate` L22; `Awake` L29; `Update` L37; `ResolveStatus` L95; `DrawPatch` L110; `StatusColor` L139.

```csharp
L12: private const float HudHeight = 90f;
```


#### T `PrototypeMajorIntentAuthority09F20.cs` (184 linjer; `aea633f9579e`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorIntentAuthority09F20.cs).

Klasser: `PrototypeMajorIntentAuthority09F20` L9.

Indgange og kildeankre: `AutoCreate` L30; `Update` L36; `Resolve` L119; `CaptureExplicitIntent` L144; `CompanyCenter` L160; `SuppressAutonomousReplacement` L178.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeMajorMissionCommitment09F24.cs` (339 linjer; `01191372c332`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorMissionCommitment09F24.cs).

Klasser: `PrototypeMajorMissionCommitment09F24` L21.

Indgange og kildeankre: `AutoCreate` L38; `Update` L44; `Resolve` L136; `HardReplaceOldExecution` L163; `ResetControllerIntent` L204; `PauseController` L219; `RestoreControllersNotUsed` L229; `RestoreAllPausedControllers` L252; `HasDestination` L260; `BuildSignature` L268; `ReadOrder` L284; `ReadBool` L290; `ReadVector` L296; `WriteBool` L302; `WriteFloat` L309; `MissionField` L316; `PlanarDistance` L323; `OnDisable` L330; `OnDestroy` L335.

```csharp
L35: private const float ExactArrival = 4.5f;
```


#### R `PrototypeMajorOrderVisuals09F18.cs` (278 linjer; `9d43d2b8535b`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorOrderVisuals09F18.cs).

Klasser: `PrototypeMajorOrderVisuals09F18` L6; `Visual` L8.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L33; `OnDestroy` L43; `Update` L49; `RebuildLinks` L62; `UpdateLinks` L90; `SetMission` L118; `MarkArrived` L151; `ClearMission` L160; `ClearAllMissions` L169; `UpdateMissionVisuals` L177; `CreateLine` L210; `CreateMaterial` L223; `SetColor` L231; `ColorFor` L237; `DrawFootprint` L248.

```csharp
L258: const float halfWidth = 24f;
L259: const float halfDepth = 3.2f;
```


#### T `PrototypeMajorOrderVisuals09F18.cs` (289 linjer; `da30c84ce9c1`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorOrderVisuals09F18.cs).

Klasser: `PrototypeMajorOrderVisuals09F18` L9; `Visual` L11.

Indgange og kildeankre: `AutoCreate` L30; `Awake` L36; `OnDestroy` L46; `Update` L52; `RebuildLinks` L65; `UpdateLinks` L93; `SetMission` L121; `MarkArrived` L154; `ClearMission` L163; `ClearAllMissions` L172; `UpdateMissionVisuals` L180; `CreateLine` L221; `CreateMaterial` L234; `SetColor` L242; `ColorFor` L248; `DrawFootprint` L259.

```csharp
L269: const float halfWidth = 24f;
L270: const float halfDepth = 3.2f;
```


#### R/T `PrototypeMajorPointerIsolation09F18.cs` (70 linjer; `164262a7afda`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorPointerIsolation09F18.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorPointerIsolation09F18.cs).

Klasser: `PrototypeMajorPointerIsolation09F18` L5.

Indgange og kildeankre: `AutoCreate` L11; `Update` L20; `OnDisable` L47; `Restore` L52.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeMajorReserveDecision09F19.cs` (408 linjer; `841585c24513`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorReserveDecision09F19.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorReserveDecision09F19.cs).

Klasser: `PrototypeMajorReserveDecision09F19` L12.

Indgange og kildeankre: `AutoCreate` L30; `Update` L36; `ResolveMajor` L48; `ReviewFourVersusOne` L68; `ActiveCompanies` L171; `KnownEnemies` L180; `ShouldFlank` L211; `ChooseFlankSign` L245; `BestReserve` L252; `MissionCenter` L269; `MissionFacing` L287; `SetMission` L306; `SetField` L317; `Reassert` L324; `SafeCompany` L352; `SetDecision` L358; `AverageStrength` L364; `AverageCohesion` L372; `TotalStrength` L380; `CompanyCenter` L388; `Dist` L402.

```csharp
L23: private const float ReviewInterval = 0.75f;
L24: private const float KnownRange = 700f;
L25: private const float CompanySpacing = 60f;
L26: private const float ReserveDepth = 78f;
L27: private const float FlankOffset = 112f;
```


#### R/T `PrototypeMajorSelectionBridge09F16.cs` (41 linjer; `d371ce8ec9ab`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorSelectionBridge09F16.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorSelectionBridge09F16.cs).

Klasser: `PrototypeMajorSelectionBridge09F16` L7.

Indgange og kildeankre: `AutoCreate` L10; `Update` L18.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeMajorSelectionHandoff09F20.cs` (40 linjer; `123fb5a73058`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorSelectionHandoff09F20.cs).

Klasser: `PrototypeMajorSelectionHandoff09F20` L7.

Indgange og kildeankre: `AutoCreate` L10; `Update` L16.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeMajorSlotDeconfliction09F23.cs` (475 linjer; `fdf5e51e6ae6`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorSlotDeconfliction09F23.cs).

Klasser: `PrototypeMajorSlotDeconfliction09F23` L13.

Indgange og kildeankre: `AutoCreate` L32; `Update` L38; `ResolveMajor` L69; `ApplyFormation` L95; `DetermineFormationForward` L202; `FacingForSlot` L237; `NearestKnownEnemy` L248; `ApplyMission` L275; `UpdateSavedAssignment` L313; `AllGoalsStillSeparated` L329; `BestAssignment` L354; `SearchAssignment` L365; `BuildSignature` L401; `AveragePosition` L407; `ReadOrder` L420; `ReadBool` L426; `ReadVector` L432; `WriteVector` L438; `WriteBool` L444; `WriteFloat` L450; `MissionField` L456; `Ground` L463; `PlanarDistance` L469.

```csharp
L26: private const float ReviewInterval = 0.25f;
L27: private const float CompanySpacing = 60f;
L28: private const float AttackStandoff = 68f;
L29: private const float ReserveDepth = 78f;
```


#### R `PrototypeMajorUi09F18.cs` (373 linjer; `67dfd13e3a84`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMajorUi09F18.cs).

Klasser: `PrototypeMajorUi09F18` L4.

Indgange og kildeankre: `AutoCreate` L28; `Awake` L34; `OnDestroy` L44; `QueueOrder` L50; `UseLookPack` L55; `Update` L60; `HandleWorldInput` L75; `RayHitsHQ` L141; `TryGetGround` L160; `EnsureWorldVisuals` L184; `CreateRing` L195; `CreateMaterial` L209; `UpdateSelectionRing` L217; `UpdateTargetRing` L224; `DrawCircle` L240; `ColorFor` L251; `IsPointerOverControls` L262; `PanelRect` L274.

```csharp
L25: private const float DragThreshold = 9f;
L277: const float height = 112f;
L342: const float colW = 204f;
L360: const float gap = 4f;
L362: const float buttonHeight = 32f;
```


#### T `PrototypeMajorUi09F18.cs` (359 linjer; `facbf6f3b5aa`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMajorUi09F18.cs).

Klasser: `PrototypeMajorUi09F18` L6.

Indgange og kildeankre: `AutoCreate` L31; `Awake` L37; `OnDestroy` L47; `Update` L53; `HandleWorldInput` L68; `RayHitsHQ` L134; `TryGetGround` L153; `EnsureWorldVisuals` L177; `CreateRing` L188; `CreateMaterial` L202; `UpdateSelectionRing` L210; `UpdateTargetRing` L217; `DrawCircle` L233; `ColorFor` L244; `IsPointerOverControls` L255; `PanelRect` L264.

```csharp
L27: private const float DragThreshold = 9f;
L28: private const float HudHeight = 90f;
L335: const float gap = 4f;
L337: const float commandHeight = 27f;
```


#### R `PrototypeManualOrderAuthority09F18.cs` (61 linjer; `31aa69c69052`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeManualOrderAuthority09F18.cs).

Klasser: `PrototypeManualOrderAuthority09F18` L7.

Indgange og kildeankre: `AutoCreate` L10; `Update` L19.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeManualOrderAuthority09F18.cs` (69 linjer; `42c3cd37ce90`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeManualOrderAuthority09F18.cs).

Klasser: `PrototypeManualOrderAuthority09F18` L12.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeManualRouteRecovery09H4.cs` (382 linjer; `71f2df407bde`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeManualRouteRecovery09H4.cs).

Klasser: `PrototypeManualRouteRecovery09H4` L12; `RouteProgress` L14.

Indgange og kildeankre: `AutoCreate` L42; `Awake` L51; `Update` L69; `ProcessManualRoute` L104; `ApplyManualFormation` L230; `TryChooseBypass` L275; `CleanupInactive` L353; `PlanarDistance` L376.

```csharp
L17: public int CurrentIndex = -1;
L33: private const float SampleInterval = 0.25f;
L34: private const float DeployDistance = 14f;
L35: private const float ProgressDistance = 0.45f;
L36: private const float StallSeconds = 3.5f;
L37: private const float RecoveryCooldown = 4.0f;
L38: private const float RouteArrivalGuard = 5.0f;
L39: private const int MaxRecoveryWaypoints = 4;
```


#### R/T `PrototypeManualRouteRecovery09H4.cs` (424 linjer; `512dbaa6fbd4`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeManualRouteRecovery09H4.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeManualRouteRecovery09H4.cs).

Klasser: `PrototypeManualRouteRecovery09H4` L12; `RouteProgress` L14.

Indgange og kildeankre: `AutoCreate` L43; `Awake` L52; `Update` L70; `ProcessManualRoute` L105; `ApplyManualFormation` L238; `FindNearestEnemyDistance` L291; `TryChooseBypass` L317; `CleanupInactive` L395; `PlanarDistance` L418.

```csharp
L17: public int CurrentIndex = -1;
L33: private const float SampleInterval = 0.25f;
L34: private const float DeployDistance = 14f;
L35: private const float DeploymentBuffer = 12f;
L36: private const float ProgressDistance = 0.45f;
L37: private const float StallSeconds = 3.5f;
L38: private const float RecoveryCooldown = 4.0f;
L39: private const float RouteArrivalGuard = 5.0f;
L40: private const int MaxRecoveryWaypoints = 4;
```


#### R `PrototypeMarchColumn09F6.cs` (176 linjer; `2c661d6a9609`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMarchColumn09F6.cs).

Klasser: `PrototypeMarchColumn09F6` L10.

Indgange og kildeankre: `AutoCreate` L20; `Awake` L29; `Update` L48; `ApplyPolicy` L68; `EnsureLine` L130; `FindNearestEnemyDistance` L142; `PlanarDistance` L165; `FormatDistance` L172.

```csharp
L17: private const float EnterColumnDestinationDistance = 28f;
```


#### T `PrototypeMarchColumn09F6.cs` (223 linjer; `12f5f9a520a9`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMarchColumn09F6.cs).

Klasser: `PrototypeMarchColumn09F6` L11.

Indgange og kildeankre: `AutoCreate` L22; `Awake` L31; `Update` L52; `ApplyPolicy` L72; `EnsureLine` L165; `FindNearestEnemyThreat` L177; `PlanarDistance` L212; `FormatDistance` L219.

```csharp
L18: private const float EnterColumnDestinationDistance = 28f;
L19: private const float EnemyFireDeploymentBuffer = 35f;
```


#### K/R `PrototypeMeleeCombatManager.cs` (253 linjer; `478d4c8d518a`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeMeleeCombatManager.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMeleeCombatManager.cs).

Klasser: `PrototypeMeleeCombatManager` L6.

Indgange og kildeankre: `AutoCreate` L24; `Awake` L33; `Update` L64; `ResolveMeleePulse` L117; `CalculateLosses` L148; `ApplyMeleeLosses` L163; `CanMelee` L194; `AreInContact` L201; `FaceOpponent` L222; `BuildPairKey` L236; `PlanarDistance` L247.

```csharp
L20: private const float PulseInterval = 1.25f;
L21: private const float ContactDistance = 7.0f;
```


#### T `PrototypeMeleeCombatManager.cs` (366 linjer; `ecdc841592d9`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMeleeCombatManager.cs).

Klasser: `PrototypeMeleeCombatManager` L10.

Indgange og kildeankre: `AutoCreate` L29; `Awake` L38; `Update` L70; `ResolveMeleePulse` L138; `CalculateLosses` L177; `ApplyMeleeLosses` L209; `CanMelee` L240; `AreInContact` L247; `HardObstacleBetween` L263; `IsCharging` L298; `HasChargeMomentum` L304; `ContactSector` L310; `FaceOpponent` L335; `BuildPairKey` L349; `PlanarDistance` L360.

```csharp
L24: private const float PulseInterval = 1.25f;
L25: private const float ContactDistance = 7.0f;
L26: private const float ChargeDeepContactDistance = 2.2f;
```


#### R/T `PrototypeMoraleTest09F8.cs` (146 linjer; `bfc561813a21`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeMoraleTest09F8.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeMoraleTest09F8.cs).

Klasser: `PrototypeMoraleTest09F8` L8.

Indgange og kildeankre: `AutoCreate` L15; `Awake` L24; `Update` L42; `LateUpdate` L48; `LockMorale` L54; `LogNewCasualties` L67; `GetBand` L106; `FindNearestEnemy` L119; `PlanarDistance` L140.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeMovementStallDiagnostics09H2.cs` (205 linjer; `86d5d4fd1525`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeMovementStallDiagnostics09H2.cs).

Klasser: `PrototypeMovementStallDiagnostics09H2` L9; `StallState` L11.

Indgange og kildeankre: `AutoCreate` L32; `Awake` L41; `Update` L54; `SampleRegiment` L74; `ResetState` L127; `WriteReport` L134; `FindNearestHardObstacle` L165; `PlanarDistance` L199.

```csharp
L26: private const float SampleInterval = 0.50f;
L27: private const float ProgressDistance = 0.30f;
L28: private const float ReportAfterSeconds = 3.50f;
L29: private const float IgnoreGoalDistance = 4.0f;
```


#### R/T `PrototypeNationalFlags09F2.cs` (113 linjer; `f206867ee54a`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeNationalFlags09F2.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeNationalFlags09F2.cs).

Klasser: `PrototypeNationalFlags09F2` L8.

Indgange og kildeankre: `AutoCreate` L14; `Update` L23; `CreateNationalFlag` L38; `BuildDanishFlag` L70; `BuildPrussianFlag` L84; `CreateCloth` L98.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeNavigation09AHotfix.cs` (623 linjer; `dddeaa651c44`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeNavigation09AHotfix.cs).

Klasser: `PrototypeNavigation09AHotfix` L10; `Obstacle` L12; `UnitState` L19.

Indgange og kildeankre: `AutoCreate` L55; `Awake` L64; `Update` L77; `Install` L103; `RefreshObstacles` L118; `AddObstacle` L146; `HandleRegiment` L157; `ResolveInitialFormationOverlap` L205; `ForceOwnColumn` L242; `ApplyAvoidanceStep` L265; `UpdateStuckRecovery` L324; `UpdateFormationRestore` L384; `LimitLookAhead` L422; `FindClosestOverlap` L434; `FindFirstThreat` L457; `FindNearestObstacle` L479; `TryFindSafePointAroundObstacle` L497; `IsPointSafe` L544; `DistancePointToSegment` L564; `StableSide` L587; `StreamCenterX` L599; `IsBridgeZone` L604; `IsRiverWater` L611; `PlanarDistance` L617.

```csharp
L40: private const float LineFootprintClearance = 10.8f;
L41: private const float ColumnFootprintClearance = 5.2f;
L42: private const float LookAheadDistance = 22f;
L43: private const float ClearToRestoreSeconds = 0.75f;
L44: private const float StuckSeconds = 0.60f;
L45: private const float AvoidanceSpeed = 7.5f;
L47: private const float BattlefieldHalfWidth = 176f;
L48: private const float BattlefieldHalfDepth = 116f;
L49: private const float RiverHalfWidth = 2.10f;
L50: private const float BridgeZ = 22.0f;
L51: private const float BridgeHalfLengthX = 8.0f;
L52: private const float BridgeHalfWidthZ = 4.0f;
```


#### R/T `PrototypeNavigation09AHotfix.cs` (623 linjer; `d522a3631e13`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeNavigation09AHotfix.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeNavigation09AHotfix.cs).

Klasser: `PrototypeNavigation09AHotfix` L10; `Obstacle` L12; `UnitState` L19.

Indgange og kildeankre: `AutoCreate` L55; `Awake` L64; `Update` L77; `Install` L103; `RefreshObstacles` L118; `AddObstacle` L146; `HandleRegiment` L157; `ResolveInitialFormationOverlap` L205; `ForceOwnColumn` L242; `ApplyAvoidanceStep` L265; `UpdateStuckRecovery` L324; `UpdateFormationRestore` L384; `LimitLookAhead` L422; `FindClosestOverlap` L434; `FindFirstThreat` L457; `FindNearestObstacle` L479; `TryFindSafePointAroundObstacle` L497; `IsPointSafe` L544; `DistancePointToSegment` L564; `StableSide` L587; `StreamCenterX` L599; `IsBridgeZone` L604; `IsRiverWater` L611; `PlanarDistance` L617.

```csharp
L40: private const float LineFootprintClearance = 10.8f;
L41: private const float ColumnFootprintClearance = 5.2f;
L42: private const float LookAheadDistance = 22f;
L43: private const float ClearToRestoreSeconds = 0.75f;
L44: private const float StuckSeconds = 0.60f;
L45: private const float AvoidanceSpeed = 7.5f;
L47: private const float BattlefieldHalfWidth = 176f;
L48: private const float BattlefieldHalfDepth = 116f;
L49: private const float RiverHalfWidth = 2.10f;
L50: private const float BridgeZ = 22.0f;
L51: private const float BridgeHalfLengthX = 8.0f;
L52: private const float BridgeHalfWidthZ = 4.0f;
```


#### K `PrototypeNavigation09BTreePassThrough.cs` (127 linjer; `a34bc972149b`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeNavigation09BTreePassThrough.cs).

Klasser: `PrototypeNavigation09BTreePassThrough` L10.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24; `RemoveSoftObstacles` L70; `ClearPrivateCollection` L113.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeNavigation09BTreePassThrough.cs` (127 linjer; `15a1f32f1629`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeNavigation09BTreePassThrough.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeNavigation09BTreePassThrough.cs).

Klasser: `PrototypeNavigation09BTreePassThrough` L10.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24; `RemoveSoftObstacles` L70; `ClearPrivateCollection` L113.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeNavigationRecoveryManager.cs` (403 linjer; `7cafaec16397`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeNavigationRecoveryManager.cs).

Klasser: `PrototypeNavigationRecoveryManager` L6; `Obstacle` L8.

Indgange og kildeankre: `AutoCreate` L33; `Awake` L42; `OnDestroy` L57; `Update` L63; `TryInstall` L81; `RefreshObstacles` L94; `AddObstacle` L122; `RecoverIfInsideBlockedTerrain` L133; `ReinforceCurrentSteering` L181; `TryFindBestDetour` L208; `EstimatePathPenalty` L250; `EstimatePathPenaltyInternal` L264; `HasObstacleBetween` L294; `FindFirstBlockingObstacle` L302; `IsPointNavigableForRecovery` L324; `DistancePointToSegment` L341; `GetClearance` L360; `StreamCenterX` L365; `IsBridgeZone` L370; `IsRiverWater` L377; `SegmentCrossesRiver` L383; `PlanarDistance` L397.

```csharp
L25: private const float BattlefieldHalfWidth = 176f;
L26: private const float BattlefieldHalfDepth = 116f;
L27: private const float RiverHalfWidth = 2.10f;
L28: private const float BridgeZ = 22.0f;
L29: private const float BridgeHalfLengthX = 8.0f;
L30: private const float BridgeHalfWidthZ = 4.0f;
L385: const int samples = 32;
```


#### R/T `PrototypeNavigationRecoveryManager.cs` (403 linjer; `c5f1ff95ab34`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeNavigationRecoveryManager.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeNavigationRecoveryManager.cs).

Klasser: `PrototypeNavigationRecoveryManager` L6; `Obstacle` L8.

Indgange og kildeankre: `AutoCreate` L33; `Awake` L42; `OnDestroy` L57; `Update` L63; `TryInstall` L81; `RefreshObstacles` L94; `AddObstacle` L122; `RecoverIfInsideBlockedTerrain` L133; `ReinforceCurrentSteering` L181; `TryFindBestDetour` L208; `EstimatePathPenalty` L250; `EstimatePathPenaltyInternal` L264; `HasObstacleBetween` L294; `FindFirstBlockingObstacle` L302; `IsPointNavigableForRecovery` L324; `DistancePointToSegment` L341; `GetClearance` L360; `StreamCenterX` L365; `IsBridgeZone` L370; `IsRiverWater` L377; `SegmentCrossesRiver` L383; `PlanarDistance` L397.

```csharp
L25: private const float BattlefieldHalfWidth = 176f;
L26: private const float BattlefieldHalfDepth = 116f;
L27: private const float RiverHalfWidth = 2.10f;
L28: private const float BridgeZ = 22.0f;
L29: private const float BridgeHalfLengthX = 8.0f;
L30: private const float BridgeHalfWidthZ = 4.0f;
L385: const int samples = 32;
```


#### K `PrototypeNavigationV3Authority.cs` (72 linjer; `4b04e6469846`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeNavigationV3Authority.cs).

Klasser: `PrototypeNavigationV3Authority` L8.

Indgange og kildeankre: `AutoCreate` L13; `Update` L23.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeNavigationV3Authority.cs` (74 linjer; `43a9d2a5249a`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeNavigationV3Authority.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeNavigationV3Authority.cs).

Klasser: `PrototypeNavigationV3Authority` L9.

Indgange og kildeankre: `AutoCreate` L14; `Update` L24.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeNavigationV3DetourContinuation09I1.cs` (424 linjer; `2142c4a77779`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeNavigationV3DetourContinuation09I1.cs).

Klasser: `PrototypeNavigationV3DetourContinuation09I1` L18; `ContinuationState` L20.

Indgange og kildeankre: `AutoCreate` L44; `Awake` L53; `Update` L74; `ProcessRegiment` L114; `TryChooseContinuation` L263; `DistancePointToSegment` L376; `PlanarDistance` L395; `CleanupDestroyed` L402.

```csharp
L37: private const float ColumnClearance = 4.4f;
L38: private const float TriggerDistance = 2.85f;
L39: private const float MinimumLegDistance = 5.0f;
L40: private const float ContinuationCooldown = 0.20f;
L41: private const int MaxContinuationsPerObstacle = 10;
```


#### T `PrototypeObstacleEndpointGuard09F22.cs` (129 linjer; `145baffcbfef`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeObstacleEndpointGuard09F22.cs).

Klasser: `PrototypeObstacleEndpointGuard09F22` L10.

Indgange og kildeankre: `AutoCreate` L22; `Awake` L28; `Update` L36; `Scan` L72; `CorrectEndpoint` L87; `PlanarDistance` L123.

```csharp
L17: private const float LineHalfWidth = 25f;
L18: private const float ColumnHalfWidth = 3.4f;
L19: private const float Pad = 1.5f;
```


#### T `PrototypeOfficerFacingOrder09F29G.cs` (535 linjer; `06cef8b61826`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeOfficerFacingOrder09F29G.cs).

Klasser: `PrototypeOfficerFacingOrder09F29G` L12.

Indgange og kildeankre: `AutoCreate` L54; `Awake` L61; `OnDestroy` L82; `BeginBattalionOrder` L88; `BeginHigherOrder` L111; `BeginRegimentalOrder` L147; `IsPendingHigher` L172; `IsPendingRegimental` L181; `IsPendingBattalion` L187; `CancelPending` L194; `Update` L205; `CommitOrder` L278; `ApplyFacingToBattalion` L333; `CancelWithdrawalsForBattalion` L360; `ClearLegacyPending` L380; `SuppressLegacySelectionDrag` L391; `TryGetGround` L408; `PointerOverBottomHud` L427; `CreateVisuals` L432; `CreateLine` L446; `GetPendingCircleRadius` L461; `DrawCircle` L470; `DrawArrow` L482; `SetTwoPointLine` L506; `SetVisualColor` L515; `SetVisuals` L528.

```csharp
L26: private const float DragFacingThreshold = 5f;
L27: private const int CircleSamples = 72;
L30: private int battalionIndex = -1;
```


#### K `PrototypeOfficerObjective09K.cs` (62 linjer; `9f6f3e4c3ce7`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeOfficerObjective09K.cs).

Klasser: `PrototypeOfficerObjective09K` L21.

Tilstands-/data-enums: `PrototypeOfficerMission09K` = None, DefendArea, AttackCaptureArea (L3); `PrototypeSubunitCommitment09K` = Engaged, Support, Reserve (L10).

Indgange og kildeankre: `SetDefendArea` L28; `SetAttackCaptureArea` L37; `ClearMission` L46; `LogMission` L51.

```csharp
L25: public float ObjectiveRadius { get; private set; } = 24f;
L26: public float RequestedReserveFraction { get; private set; } = 0.25f;
```


#### T `PrototypeOfficerTacticalDoctrine09F30A.cs` (582 linjer; `9f3e339e8eea`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeOfficerTacticalDoctrine09F30A.cs).

Klasser: `PrototypeOfficerIntentCapture09F30A` L17; `PrototypeOfficerTacticalDoctrine09F30A` L117; `MissionRole` L119.

Indgange og kildeankre: `AutoCreate` L38; `Awake` L45; `Update` L56; `ReadBool` L99; `ReadInt` L107; `AutoCreate` L139; `Awake` L146; `Update` L155; `ReframeRegimentalDefend` L190; `ReframeBattalionDefend` L223; `ApplyMission` L309; `ProcessAttackAllocation` L338; `ResolveAutoFacing` L465; `AddCurrentFacing` L491; `FindNearestEnemy` L509; `ResolveLegalSlot` L531; `AssaultPower` L542; `ReadBool` L550; `SetField` L558; `Flat` L564; `Ground` L570; `PlanarDistance` L576.

```csharp
L20: private const float DragFacingThreshold = 5f;
L29: public static int LastCommitFrame { get; private set; } = -1;
L31: public static int LastBattalionIndex { get; private set; } = -1;
L129: private const float CompanySpacing = 60f;
L130: private const float ReserveDepth = 86f;
L131: private const float RegimentBattalionLateral = 170f;
L132: private const float AutoFacingEnemyRange = 2200f;
```


#### T `PrototypeOobStatus09F29V.cs` (211 linjer; `744e689e8712`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeOobStatus09F29V.cs).

Klasser: `PrototypeOobStatus09F29V` L12.

Indgange og kildeankre: `AutoCreate` L37; `Awake` L43; `GetStatus` L101; `TryGetMission` L132; `HasDestination` L166; `IsOobOpen` L173; `GetBattalionOpen` L180; `StatusColor` L185.

```csharp
L15: private const float PanelX = 8f;
L16: private const float PanelY = 39f;
L17: private const float PanelWidth = 292f;
L18: private const float HeaderHeight = 27f;
L19: private const float RegimentRowHeight = 25f;
L20: private const float MajorRowHeight = 23f;
L21: private const float CompanyRowHeight = 21f;
```


#### K `PrototypePerformanceOverlay09K.cs` (73 linjer; `ee2931f6de05`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypePerformanceOverlay09K.cs).

Klasser: `PrototypePerformanceOverlay09K` L7.

Indgange og kildeankre: `AutoCreate` L13; `Update` L22.

```csharp
L9: private float smoothedDelta = 1f / 60f;
```


#### K/R `PrototypeProjectStartup.cs` (49 linjer; `fe15efabd1dd`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Editor/PrototypeProjectStartup.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Editor/PrototypeProjectStartup.cs).

Klasser: `PrototypeProjectStartup` L9.

Indgange og kildeankre: `EnsurePrototypeScene` L19.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K/R/T `PrototypeRangeFanVisualEnhancer.cs` (223 linjer; `f7177ec8c0b8`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeRangeFanVisualEnhancer.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeRangeFanVisualEnhancer.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRangeFanVisualEnhancer.cs).

Klasser: `PrototypeRangeFanVisualEnhancer` L11; `FanSet` L13.

Indgange og kildeankre: `AutoCreate` L24; `LateUpdate` L33; `GetOrCreateFanSet` L62; `PrepareFans` L86; `PrepareLine` L109; `UpdateArcOnly` L143; `UpdateOuterFan` L161; `GetFlatForward` L207; `GroundVisiblePoint` L218.

```csharp
L145: const int arcSegments = 48;
L163: const int sideSegments = 12;
L164: const int arcSegments = 48;
L165: const float muzzleHalfWidth = 8.5f;
L166: const float muzzleForward = 1.30f;
```


#### R/T `PrototypeRangeTuning09F7.cs` (63 linjer; `75cddfcbf738`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeRangeTuning09F7.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRangeTuning09F7.cs).

Klasser: `PrototypeRangeTuning09F7` L7.

Indgange og kildeankre: `AutoCreate` L17; `Awake` L26; `Update` L39; `Start` L55.

```csharp
L9: public const float EffectiveRangeMetres = 80f;
L10: public const float MaximumRangeMetres = 115f;
L11: public const float FireArcHalfAngleDegrees = 35f;
```


#### R/T `PrototypeRangeTuning09F8.cs` (59 linjer; `6b50554d1f2a`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeRangeTuning09F8.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRangeTuning09F8.cs).

Klasser: `PrototypeRangeTuning09F8` L7.

Indgange og kildeankre: `AutoCreate` L17; `Awake` L26; `Update` L44.

```csharp
L9: public const float EffectiveRangeMetres = 70f;
L10: public const float MaximumRangeMetres = 100f;
L11: public const float FireArcHalfAngleDegrees = 35f;
```


#### K `PrototypeReducedQABattle09L3.cs` (221 linjer; `f621bb280d35`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeReducedQABattle09L3.cs).

Klasser: `PrototypeReducedQABattle09L3` L10.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24; `FindActiveRegiment` L88; `ReduceCompanyLists` L102; `IsRemovedParent` L142; `ClearPlayerCommanderReferences` L150; `RemoveFromList` L180; `RemoveFromBattleRoster` L189; `CountActiveCompanies` L207.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeRegimentHierarchy09F27.cs` (1701 linjer; `ed2d9671011c`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRegimentHierarchy09F27.cs).

Klasser: `PrototypeRegimentHierarchy09F27` L11; `CompanyMission` L13; `MissionVisual` L24; `Battalion` L36.

Indgange og kildeankre: `AutoCreate` L118; `Awake` L124; `OnDestroy` L140; `Update` L146; `TryInstall` L169; `SpawnSecondBattalionCompanies` L218; `EnsureNewCompaniesHaveOfficerAI` L248; `PositionEightCompanies` L264; `SetPose` L289; `DisableLegacyMajorSystems` L300; `DisableComponent` L326; `UpdateAuthority` L332; `UpdateMissions` L366; `UpdateDynamicHq` L466; `MoveHqToward` L503; `UpdateBattalionAI` L527; `IssueBattalionOrderFromRegiment` L544; `IssueBattalionOrder` L561; `NewMission` L716; `AssignMission` L730; `ToggleBattalionAI` L768; `SetBattalionAIEnabled` L775; `HasActiveMissionExecutors` L813; `IsBattalionOrderActive` L822; `SetBattalionDoctrine` L865; `MoveMajor` L871; `GetBattalionCenter` L891; `GetMajorHq` L909; `GetBattalionLastOrderPoint` L914; `GetBattalionLastOrder` L925; `GetCompanies` L936; `GetBattalionAIEnabled` L941; `GetBattalionDoctrine` L946; `ClearMajorSelection` L951; `IsPointerOverControls` L959; `HandleWorldInput` L967; `SelectMajor` L1052; `ClearCompanySelection` L1063; `RayHitMajor` L1079; `TryGetGround` L1108; `EnsureWorldVisualInfrastructure` L1134; `UpdateWorldVisuals` L1163; `SetMissionVisual` L1223; `MarkMissionVisualArrived` L1246; `ClearMissionVisual` L1252; `CreateLine` L1261; `CreateRing` L1273; `DrawCircle` L1287; `DrawFootprint` L1298; `CreateMajorHq` L1316; `CreateHorse` L1337; `ClearPlayerRoute` L1377; `HasDestination` L1384; `AssignRecursive` L1410; `KnownEnemies` L1438; `NearestEnemyToPoint` L1455; `FindNearestEnemy` L1471; `BestReserve` L1476; `FaceDirection` L1495; `IsSafeHqRelocation` L1506; `BankSide` L1515; `ValidBattalion` L1524; `FindRegiment` L1529; `CreateUnlit` L1540; `SetLineColor` L1548; `ColorFor` L1554; `Label` L1565; `Flat` L1579; `Ground` L1587; `GroundWithOffset` L1593; `PlanarDistance` L1599.

```csharp
L68: private int selectedBattalion = -1;
L95: private const float HudHeight = 90f;
L96: private const float DragThreshold = 9f;
L97: private const float CompanySpacing = 72f;
L98: private const float ReserveDepth = 86f;
L99: private const float FlankOffset = 126f;
L100: private const float ExactArrival = 0.50f;
L101: private const float HqSpeed = 6.2f;
L102: private const float HqPreferredBehind = 155f;
L103: private const float HqRelocateThreshold = 230f;
L104: private const float CommandInner = 320f;
L105: private const float CommandOuter = 450f;
L1683: const float gap = 4f;
L1685: const float commandHeight = 27f;
```


#### T `PrototypeRegimentalCommandHardening09F29E.cs` (1126 linjer; `f5991352ea28`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRegimentalCommandHardening09F29E.cs).

Klasser: `PrototypeRegimentalHud09F29E` L20; `AggregateStats` L249; `PrototypeRegimentalCommandHardening09F29E` L378.

Indgange og kildeankre: `AutoCreate` L45; `Awake` L52; `DrawOrderButton` L185; `GetPendingOrder` L199; `SetPendingOrder` L207; `ReadString` L213; `DoctrineShort` L221; `GetAllCompanies` L231; `CalculateStats` L259; `CalculateStats` L292; `StateStyle` L297; `DrawSectionLabel` L302; `BuildStyles` L307; `MakeButtonStyle` L349; `AutoCreate` L412; `Awake` L419; `Update` L436; `LateUpdate` L446; `IsRegimentalOrHigherSelected` L490; `SelectedCommandLevelLabel` L500; `Resolve` L513; `ProcessNewRegimentalMission` L523; `ApplyCompactDefense` L543; `ApplyAttackRoleCorrection` L562; `AssignTwoBattalionGoals` L609; `EnforcePreciseSlotArrival` L631; `ShowCompleteCommandChain` L735; `ShowAllCompanyMissionVisuals` L766; `RefreshRegimentalObjective` L803; `TryReadMission` L834; `TryReadVisual` L857; `EnsureObjectiveVisuals` L905; `CreateLine` L920; `SetObjectiveVisible` L935; `DrawTerrainCircle` L942; `DrawTerrainCross` L956; `SetTerrainFollowingConnection` L980; `DrawFootprint` L994; `DirectionTowardNearestEnemy` L1015; `CountEnemies` L1038; `Face` L1055; `Add` L1065; `ColorFor` L1072; `Label` L1083; `CreateUnlit` L1097; `Ground` L1106; `Flat` L1112; `PlanarDistance` L1120.

```csharp
L22: private const float HudHeight = 90f;
L102: const float stateGap = 3f;
L119: const float gap = 4f;
L383: private const float DefendBattalionHalfSeparation = 84f;
L384: private const float BattalionReserveDepth = 285f;
L385: private const float BattalionFlankOffset = 285f;
L386: private const float ExactSlotArrival = 0.50f;
L387: private const float OriginalArrivalWindow = 0.75f;
L388: private const float LinkHeight = 0.88f;
L389: private const int LinkSamples = 28;
L390: private const int ObjectiveRingSamples = 96;
```


#### T `PrototypeRegimentalHQ09F28.cs` (941 linjer; `e5244b6e6d75`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRegimentalHQ09F28.cs).

Klasser: `PrototypeRegimentalHQ09F28` L10; `PendingRegimentalOrder` L12.

Indgange og kildeankre: `AutoCreate` L70; `Awake` L76; `OnDestroy` L89; `Update` L95; `TryInstall` L112; `ToggleAI` L127; `SetAIEnabled` L132; `HasActiveMissionExecutors` L189; `IsMissionActive` L194; `SetDoctrine` L205; `IssueRegimentalOrder` L211; `UpdateRegimentalAI` L348; `UpdateDynamicHq` L372; `MoveHqToward` L417; `MoveRegimentalHq` L441; `IsPointerOverControls` L456; `HandleWorldInput` L464; `SetSelected` L539; `ClearCompanySelection` L552; `RayHitsHq` L568; `TryGetGround` L588; `CreateWorldVisuals` L612; `UpdateWorldVisuals` L636; `CreateRegimentalHq` L665; `CreateHorse` L688; `GetRegimentCenter` L728; `KnownEnemies` L733; `FindNearestEnemy` L750; `IsSafeHqRelocation` L766; `BankSide` L775; `CreateRing` L784; `DrawCircle` L798; `CreateUnlit` L809; `Label` L817; `Flat` L831; `Ground` L839; `GroundWithOffset` L845; `PlanarDistance` L851.

```csharp
L58: private const float HudHeight = 90f;
L59: private const float DragThreshold = 9f;
L60: private const float BattalionLateral = 170f;
L61: private const float BattalionReserveDepth = 285f;
L62: private const float BattalionFlankOffset = 285f;
L63: private const float RegimentalHqSpeed = 7.0f;
L64: private const float RegimentalHqBehind = 360f;
L65: private const float RegimentalRelocateThreshold = 470f;
L66: private const float RegimentCommandInner = 800f;
L67: private const float RegimentCommandOuter = 1100f;
L922: const float gap = 4f;
L924: const float commandHeight = 27f;
```


#### T `PrototypeRegimentalHud09F29F.cs` (387 linjer; `03395331eccf`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRegimentalHud09F29F.cs).

Klasser: `PrototypeRegimentalHud09F29F` L15; `AggregateStats` L265.

Indgange og kildeankre: `AutoCreate` L39; `Awake` L46; `DrawOrderButton` L202; `GetPendingOrder` L216; `SetPendingOrder` L224; `ReadString` L230; `DoctrineShort` L237; `GetAllCompanies` L247; `CalculateStats` L276; `CalculateStats` L308; `StateStyle` L313; `DrawSectionLabel` L318; `BuildStyles` L323; `MakeButtonStyle` L361.

```csharp
L17: private const float HudHeight = 90f;
L113: const float stateGap = 3f;
L130: const float gap = 4f;
```


#### K `PrototypeRegimentalStandards.cs` (260 linjer; `66e51e830c4d`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeRegimentalStandards.cs).

Klasser: `PrototypeRegimentalStandards` L8.

Indgange og kildeankre: `AutoCreate` L14; `Update` L23; `CleanupDestroyedRegiments` L40; `CreateStandard` L63; `CreateDeviceBar` L198; `CreateCord` L214; `AddRegimentRibbon` L236.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeRegimentalStandards.cs` (192 linjer; `ec818016abdc`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeRegimentalStandards.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRegimentalStandards.cs).

Klasser: `PrototypeRegimentalStandards` L7.

Indgange og kildeankre: `AutoCreate` L13; `Update` L22; `CleanupDestroyedRegiments` L39; `CreateStandard` L62; `CreateDeviceBar` L153; `AddRegimentRibbon` L169.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeRegimentalStandards09I.cs` (535 linjer; `52eef0b3daa6`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeRegimentalStandards09I.cs).

Klasser: `PrototypeRegimentalStandards09I` L9; `StandardRig` L11.

Indgange og kildeankre: `AutoCreate` L40; `Start` L49; `Update` L56; `CreateStandard` L86; `CreateCloth` L241; `AnimateStandard` L301; `ApplyProfileColors` L338; `GetProfile` L352; `HideLegacyStandard` L365; `CreateUnlitMaterial` L379; `CreateSpearFinial` L395; `CreateRibbon` L421; `AddFringe` L444; `CreateCord` L464; `CreatePrimitivePart` L487; `CleanupDestroyedRegiments` L513.

```csharp
L34: private const int ClothColumns = 8;
L35: private const int ClothRows = 5;
L36: private const float ClothWidth = 2.35f;
L37: private const float ClothHeight = 1.38f;
```


#### K `PrototypeReloadAnimation09J.cs` (417 linjer; `a8fb235dc2a4`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeReloadAnimation09J.cs).

Klasser: `PrototypeReloadAnimation09J` L10; `SoldierPoseRig` L12; `UnitState` L20.

Indgange og kildeankre: `AutoCreate` L39; `Awake` L48; `Update` L66; `CreateState` L93; `ReadNextFireTime` L106; `UpdateMovementState` L112; `UpdateVolleyDetection` L124; `AnimateUnit` L160; `ApplyFirePose` L201; `ApplyReadyPose` L210; `ApplyMarchPose` L219; `ApplyReloadPose` L232; `ApplyMuzzleLoaderReload` L247; `ApplyDreyseReload` L288; `BlendPose` L320; `ApplyPose` L334; `Smooth01` L354; `RefreshRigReferencesIfNeeded` L360; `RefreshRigReferences` L366; `CleanupDestroyedRegiments` L395.

```csharp
L36: private const float MoveEpsilon = 0.0025f;
```


#### R `PrototypeRiverBridgeOnly09F3.cs` (504 linjer; `4a9d243ec6bf`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeRiverBridgeOnly09F3.cs).

Klasser: `PrototypeRiverBridgeOnly09F3` L9; `RiverState` L11.

Indgange og kildeankre: `AutoCreate` L43; `Awake` L52; `OnDestroy` L80; `TryGetAttackSteering` L86; `IsBridgeRouteActive` L106; `GetOrCreateState` L116; `Update` L126; `LateUpdate` L147; `ApplyRiverConstraint` L184; `ResolveBridgeSteering` L231; `UpdateFinalGoal` L344; `WriteSteering` L378; `WithGroundHeight` L386; `SanitizeGoal` L392; `SegmentTouchesOpenWater` L407; `IsInOpenWater` L422; `IsBridgeZone` L431; `GetBankSide` L438; `StreamCenterX` L448; `NearlySame` L453; `PlanarDistance` L458; `SetPhase` L465; `SetPhaseSilently` L477; `Cleanup` L482.

```csharp
L21: public float LastWaterWarningAt = -100f;
L32: private const float RiverHalfWidth = 2.75f;
L33: private const float BridgeZ = 22.0f;
L34: private const float BridgeHalfLengthX = 9.0f;
L35: private const float BridgeHalfWidthZ = 4.0f;
L37: private const float BridgeStagingOffset = 32.0f;
L38: private const float BridgeEntryOffset = 13.0f;
L39: private const float PointArrival = 2.0f;
L40: private const float ExitClearDistance = 2.5f;
```


#### T `PrototypeRiverBridgeOnly09F3.cs` (591 linjer; `3395f7e5d035`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRiverBridgeOnly09F3.cs).

Klasser: `PrototypeRiverBridgeOnly09F3` L9; `RiverState` L11.

Indgange og kildeankre: `AutoCreate` L48; `Awake` L57; `OnDestroy` L85; `TryGetAttackSteering` L91; `IsBridgeRouteActive` L111; `GetOrCreateState` L121; `Update` L131; `LateUpdate` L152; `ApplyRiverConstraint` L189; `ResolveBridgeSteering` L236; `UpdateFinalGoal` L381; `WriteSteering` L415; `WithGroundHeight` L423; `SanitizeGoal` L429; `TryGetSameBankSteering` L444; `SegmentTouchesOpenWater` L494; `IsInOpenWater` L509; `IsBridgeZone` L518; `GetBankSide` L525; `StreamCenterX` L535; `NearlySame` L540; `PlanarDistance` L545; `SetPhase` L552; `SetPhaseSilently` L564; `Cleanup` L569.

```csharp
L21: public float LastWaterWarningAt = -100f;
L32: private const float RiverHalfWidth = 2.75f;
L33: private const float BridgeZ = 22.0f;
L34: private const float BridgeHalfLengthX = 9.0f;
L35: private const float BridgeHalfWidthZ = 4.0f;
L37: private const float BridgeStagingOffset = 32.0f;
L38: private const float BridgeEntryOffset = 13.0f;
L39: private const float PointArrival = 2.0f;
L40: private const float ExitClearDistance = 2.5f;
L44: private const float SameBankClearance = 7.0f;
L45: private const float SameBankStep = 24.0f;
```


#### T `PrototypeRiverVisual09F29G.cs` (89 linjer; `3bc4ce8a9d74`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRiverVisual09F29G.cs).

Klasser: `PrototypeRiverVisual09F29G` L10.

Indgange og kildeankre: `AutoCreate` L13; `Start` L20.

```csharp
L25: const float margin = 8f;
L26: const float step = 2.5f;
L27: const float halfWidth = 3.15f;
L28: const float waterOffset = 0.075f;
```


#### T `PrototypeRiverVisual09F29L.cs` (151 linjer; `32ca2dd1658a`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRiverVisual09F29L.cs).

Klasser: `PrototypeRiverVisual09F29L` L11.

Indgange og kildeankre: `AutoCreate` L19; `Start` L25; `DisableOlderRiverRendering` L35; `BuildContinuousRiver` L50; `GetRenderedGroundHeight` L140.

```csharp
L13: private const float Margin = 8f;
L14: private const float Step = 1.25f;
L15: private const float HalfWidth = 3.75f;
L16: private const float WaterLift = 0.24f;
```


#### T `PrototypeRuntimeNullGuard09F30B.cs` (138 linjer; `84e8f7097e21`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeRuntimeNullGuard09F30B.cs).

Klasser: `PrototypeRuntimeNullGuard09F30B` L9.

Indgange og kildeankre: `AutoCreate` L24; `Awake` L31; `Update` L42; `RepairBattleManagerReference` L48; `RepairChargeTargetingReflection` L65; `DisableBrokenTargeting` L125.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R `PrototypeScenario09F2.cs` (164 linjer; `188a88586472`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeScenario09F2.cs).

Klasser: `PrototypeScenario09F2` L9.

Indgange og kildeankre: `AutoCreate` L14; `Update` L23; `EnsureDanishCompany` L123; `FindRegiment` L143; `SetPose` L153.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeScenario09F2.cs` (167 linjer; `8ad6959ca2bf`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeScenario09F2.cs).

Klasser: `PrototypeScenario09F2` L10.

Indgange og kildeankre: `AutoCreate` L15; `Update` L24; `EnsureDanishCompany` L128; `FindRegiment` L146; `SetPose` L156.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeSelectionRange09F22.cs` (99 linjer; `b2b4d844eea5`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeSelectionRange09F22.cs).

Klasser: `PrototypeSelectionRange09F22` L10.

Indgange og kildeankre: `AutoCreate` L17; `Update` L23; `Resolve` L84.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeSemanticZoom09F29D.cs` (745 linjer; `3fbfe9dfb54f`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeSemanticZoom09F29D.cs).

Klasser: `PrototypeSemanticZoom09F29D` L10; `AggregateStats` L572.

Indgange og kildeankre: `AutoCreate` L58; `Awake` L65; `OnDestroy` L83; `OnDisable` L90; `Update` L95; `DetermineLevel` L124; `DrawCompanyCounters` L161; `DrawCompanyCounter` L182; `DrawCavalryCounters` L236; `DrawCavalryCounter` L246; `DrawHqCounters` L293; `DrawHqCounter` L364; `DrawHqBeacon` L407; `DrawFacingArrow` L422; `DrawStrengthBar` L443; `DrawViewIndicator` L453; `ApplyStrategicRendering` L461; `RestoreMeshes` L508; `SetMeshVisibility` L513; `TryProject` L529; `AnchoredRect` L541; `IsUsefulRect` L548; `DisplayCompanyName` L554; `Aggregate` L581; `AggregateRegiment` L610; `BuildStyles` L646; `Flat` L728.

```csharp
L22: private const float HqMarkerStartHeight = 72f;
L23: private const float MediumStartHeight = 135f;
L24: private const float OperationalStartHeight = 235f;
L25: private const float StrategicStartHeight = 390f;
L26: private const float VeryFarHeight = 525f;
L27: private const float RenderRefreshSeconds = 0.80f;
L28: private const float BottomHudGuard = 96f;
```


#### T `PrototypeSemanticZoomUnified09F29V.cs` (359 linjer; `a8d5744cf591`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeSemanticZoomUnified09F29V.cs).

Klasser: `PrototypeSemanticZoomUnified09F29V` L11.

Indgange og kildeankre: `AutoCreate` L39; `Awake` L45; `Update` L51; `DisableLegacyLayers` L66; `DrawCompanyCounters` L103; `DrawCavalryCounters` L128; `DrawCavalryCounter` L138; `DrawHqCounters` L161; `DrawHqCounter` L198; `DrawViewIndicator` L214; `ApplyStrategicSuppression` L221; `RestoreStrategicMeshes` L235; `SetAllMeshes` L245; `SetMeshVisibility` L276; `GetSelectedMajor` L285; `TryProject` L292; `IsUseful` L301; `EnemyName` L306; `OnDisable` L354.

```csharp
L13: private const float HqStart = 72f;
L14: private const float CompanyNatoStart = 175f;
L15: private const float StrategicStart = 315f;
L16: private const float BottomGuard = 104f;
```


#### K `PrototypeSoldierVisualPass09H.cs` (507 linjer; `1238f7da149e`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeSoldierVisualPass09H.cs).

Klasser: `PrototypeSoldierVisualPass09H` L9; `UnitVisual` L13.

Indgange og kildeankre: `AutoCreate` L31; `Awake` L40; `OnDestroy` L51; `Update` L57; `GetProfile` L85; `GetProfileCopy` L97; `ApplyProfile` L103; `SaveProfile` L121; `LoadSavedProfile` L149; `ResetRegimentDefault` L183; `ResetFactionDefault` L191; `InstallRegiment` L199; `LoadOrCreateProfile` L233; `GetSaveKey` L257; `SaveColor` L262; `LoadColor` L270; `CreateMaterials` L280; `UpgradeSoldier` L296; `FindLegacyChild` L393; `SetRendererMaterial` L429; `ApplyMaterialColors` L439; `ApplyStandardColors` L453; `SetChildColor` L474; `CleanupDestroyedRegiments` L485.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeSoldierVisualPass09I.cs` (908 linjer; `fffe1a92b5f5`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeSoldierVisualPass09I.cs).

Klasser: `PrototypeSoldierVisualPass09I` L11; `SoldierRig` L13; `UnitVisual` L28.

Indgange og kildeankre: `AutoCreate` L68; `Start` L77; `Update` L86; `InstallRegiment` L114; `UpdateUnitVisual` L166; `UpdateDetailLodAndReadability` L197; `AnimateSoldier` L239; `BuildSoldier` L309; `BuildHeadgear` L522; `BuildRifle` L592; `AddOfficerVisuals` L627; `AddStandardBearerVisuals` L667; `CreateLimb` L695; `CreateTaperedBox` L720; `CreatePrimitivePart` L776; `HideLegacySoldierRenderers` L800; `GetCurrentProfile` L834; `CreateMaterials` L849; `ApplyProfileColors` L864; `CleanupDestroyedRegiments` L888.

```csharp
L47: public bool DetailsVisible = true;
L48: public float CurrentReadabilityScale = 1f;
L60: private const float DetailDistance = 285f;
L61: private const float ReadabilityNearDistance = 70f;
L62: private const float ReadabilityFarDistance = 260f;
L63: private const float ReadabilityNearScale = 1.04f;
L64: private const float ReadabilityFarScale = 1.24f;
L65: private const float MoveEpsilon = 0.0025f;
```


#### K `PrototypeSpecialArms09K.cs` (327 linjer; `717dd67ac1c0`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeSpecialArms09K.cs).

Klasser: `PrototypeSpecialArmIdentity09L` L11; `PrototypeSpecialArms09K` L51; `SpecialUnit` L53.

Tilstands-/data-enums: `PrototypeSpecialArmType09K` = Dragoons, FieldArtillery (L4).

Indgange og kildeankre: `Configure` L23; `AutoCreate` L75; `Update` L84; `CreateMaterials` L136; `CreateMountedSquadron` L146; `CreateFieldBattery` L227.

```csharp
L174: const int filesAcross = 8;
```


#### T `PrototypeSquareFireSmoke09F29Z.cs` (371 linjer; `9034bcaca9ae`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeSquareFireSmoke09F29Z.cs).

Klasser: `PrototypeSquareFireSmoke09F29Z` L12; `UnitState` L14.

Indgange og kildeankre: `AutoCreate` L31; `Awake` L38; `Update` L51; `SuppressF29VFireOnly` L94; `ReadF29VLockedRotation` L112; `GetState` L135; `ProcessFaces` L155; `FindTarget` L193; `ResolveFaceVolley` L224; `CreateSmoke` L247; `EmitFaceSmoke` L278; `GetFaceIndex` L300; `RotateFlat` L313; `GetSquareHalfSide` L321; `CleanupExited` L328; `FaceName` L354; `PlanarDistance` L365.

```csharp
L21: private const float FaceFirepowerFraction = 0.25f;
```


#### T `PrototypeSquareOobHardening09F29X.cs` (474 linjer; `f8c4072b1fd4`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeSquareOobHardening09F29X.cs).

Klasser: `PrototypeSquareGateState09F29X` L16; `UnitState` L18; `PrototypeSquareLegacyFireGate09F29X` L143; `PrototypeSquarePreSectorRestore09F29X` L215; `PrototypeSquarePostSectorRestore09F29X` L237; `PrototypeOobSelectionPersistence09F29X` L292.

Indgange og kildeankre: `GetF29VState` L67; `AutoCreate` L146; `Awake` L158; `Update` L166; `Update` L217; `Update` L239; `LateUpdate` L267; `Awake` L309; `Update` L317; `LateUpdate` L337; `CaptureLiveSelection` L347; `RestoreCachedSelectionIfNeeded` L391; `ReadSelectedMajor` L459; `ReadPlayerSelection` L467.

```csharp
L22: public float PreF29VLastShotAt = -999f;
L295: private const float BottomHudHeight = 100f;
L296: private const float CommitGraceSeconds = 0.30f;
L304: private int cachedMajor = -1;
L307: private float preserveUntil = -1f;
```


#### T `PrototypeSquareVisual09F29L.cs` (228 linjer; `7e8a895ffe56`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeSquareVisual09F29L.cs).

Klasser: `PrototypeSquareVisual09F29L` L14; `Transition` L16.

Indgange og kildeankre: `AutoCreate` L36; `Awake` L42; `Update` L52; `ResolveManagers` L111; `CreateTransition` L119; `ResetTransition` L127; `ApplyTransition` L142; `GetSquareHalfSide` L169; `SquarePosition` L176; `ScaleCollider` L195; `CleanupTransitions` L208.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeStaticObstacleRouting09F11.cs` (399 linjer; `a33565b9905c`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeStaticObstacleRouting09F11.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeStaticObstacleRouting09F11.cs).

Klasser: `PrototypeStaticObstacleRouting09F11` L10; `Obstacle` L12; `RouteState` L18.

Indgange og kildeankre: `AutoCreate` L42; `Awake` L51; `OnDestroy` L67; `RequiresDetour` L73; `Update` L87; `ScanApprovedBlockers` L108; `ApplyRouting` L137; `FindFirstBlockingObstacle` L209; `BuildDetour` L239; `ExpandedRect` L319; `UnionRect` L328; `RectsOverlap` L337; `ApproximatelySameRect` L343; `SegmentIntersectsRect` L351; `SegmentsIntersect` L369; `Cross` L380; `GroundPoint` L385; `PlanarDistance` L393.

```csharp
L36: private const float WaypointArrival = 1.35f;
L37: private const float CornerPad = 1.25f;
L38: private const float ColumnHalfWidth = 3.4f;
L39: private const float LineHalfWidth = 25.0f;
```


#### R/T `PrototypeTacticalOrders09F4.cs` (526 linjer; `9f08dde6d951`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeTacticalOrders09F4.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeTacticalOrders09F4.cs).

Klasser: `PrototypeTacticalOrders09F4` L16; `GroundOrder` L18.

Tilstands-/data-enums: `PrototypeTacticalOrderMode09F4` = None, DefendHere, Attack, CaptureHere (L4).

Indgange og kildeankre: `AutoCreate` L44; `Awake` L53; `OnDestroy` L65; `Update` L72; `BeginOrder` L100; `CancelPendingOrder` L129; `TryResolvePendingOrder` L136; `IssueAttackOrder` L208; `IssueGroundOrder` L230; `UpdateGroundOrders` L255; `GetEnemyUnderMouse` L329; `TryGetGroundPoint` L345; `GetSelectedDanish` L368; `GetAverageFacing` L390; `PlanarDistance` L408; `IsPointerOverExistingUI` L415; `SuppressPointerHandlers` L421; `RestorePointerHandlers` L438; `CreateOrderMarker` L457; `SetStatus` L496.

```csharp
L39: private const float ArrivalDistance = 4.5f;
L40: private const float DeployDistance = 14f;
L41: private const float MarchColumnDistance = 24f;
```


#### T `PrototypeTacticalVisibility09F29I.cs` (493 linjer; `67da122f21f9`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeTacticalVisibility09F29I.cs).

Klasser: `PrototypeTacticalVisibility09F29I` L15.

Indgange og kildeankre: `AutoCreate` L43; `Awake` L50; `Update` L66; `DrawOverlayToggle` L142; `DrawCloseHq` L149; `DrawOrderTag` L178; `DrawObjectiveTag` L193; `DrawSelectedFacing` L208; `DrawCombatAlerts` L232; `HasFireContact` L262; `GetSelectedBattalion` L279; `TryGetBattalionOrder` L287; `TryGetRegimentalOrder` L327; `OrderLabel` L357; `TryProject` L371; `IsUseful` L383; `DrawArrow` L389; `Flat` L435; `MeasureText` L443; `BuildStyles` L450.

```csharp
L17: private const float SemanticHqStartHeight = 72f;
L18: private const float BottomHudGuard = 96f;
L28: private bool tacticalOverlay = true;
L144: const float width = 188f;
```


#### K/R/T `PrototypeTacticalVisualCleanup09F.cs` (159 linjer; `f16ba0853199`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeTacticalVisualCleanup09F.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeTacticalVisualCleanup09F.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeTacticalVisualCleanup09F.cs).

Klasser: `PrototypeTacticalVisualCleanup09F` L10.

Indgange og kildeankre: `AutoCreate` L16; `Update` L25; `LateUpdate` L30; `HideSelectedSummaryPanel` L36; `RefreshRangeFans` L76; `EnsureGhostMaterial` L111; `StyleFan` L132.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeTacticalVisuals09F6.cs` (532 linjer; `74c02ae0c0b2`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeTacticalVisuals09F6.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeTacticalVisuals09F6.cs).

Klasser: `PrototypeTacticalVisuals09F6` L11; `UnitVisual` L13; `TypeInfoCache` L504.

Indgange og kildeankre: `AutoCreate` L39; `Awake` L48; `LateUpdate` L65; `SuppressOlderVisualControllers` L90; `CreateMaterials` L103; `EnsureUnitVisual` L128; `UpdateSelection` L167; `DisableOldSelectionOutline` L196; `UpdateRangeFans` L207; `StyleAndBuildFan` L244; `UpdateDestinationGhosts` L288; `UpdateLiveDragPreview` L344; `ConfigureLine` L400; `GetFootprint` L417; `DrawTerrainRectangle` L441; `TerrainPoint` L480; `PlanarNormalized` L486; `SetPositions` L497; `For` L513.

```csharp
L32: private const int EdgeSamples = 8;
L33: private const int ArcSegments = 48;
L34: private const float GroundOffset = 0.42f;
L35: private const float SelectionMargin = 0.65f;
L36: private const float FireArcHalfAngle = 60f;
```


#### R/T `PrototypeTacticalVisuals09F7.cs` (522 linjer; `fb99348b2120`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeTacticalVisuals09F7.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeTacticalVisuals09F7.cs).

Klasser: `PrototypeTacticalVisuals09F7` L11; `UnitVisual` L13; `FormationBounds` L19.

Indgange og kildeankre: `AutoCreate` L50; `Awake` L59; `LateUpdate` L77; `SuppressOlderVisualControllers` L100; `CreateMaterials` L111; `EnsureUnitVisual` L136; `UpdateSelection` L173; `UpdateRangeFans` L208; `StyleAndBuildFan` L240; `UpdateDestinationGhosts` L289; `UpdateLiveDragPreview` L350; `CalculateBounds` L407; `FormationCenterWorld` L430; `ConfigureLine` L437; `DrawTerrainRectangle` L454; `TerrainPoint` L497; `SetPositions` L503; `PlanarNormalized` L510.

```csharp
L44: private const int EdgeSamples = 10;
L45: private const int ArcSegments = 40;
L46: private const float GroundOffset = 0.40f;
L47: private const float SelectionMargin = 0.70f;
```


#### T `PrototypeTerrainVisualPolish09F29Y.cs` (468 linjer; `74ad15472625`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeTerrainVisualPolish09F29Y.cs).

Klasser: `PrototypeTerrainVisualPolish09F29Y` L11.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L34; `Start` L44; `Update` L49; `TryBuildDryBridgeRiver` L57; `BuildRiverSegment` L93; `TryBuildDenseCrops` L168; `BuildField` L207; `BuildConformingBase` L235; `BuildCropRows` L292; `AddCrossTuft` L398; `AddDoubleSidedQuad` L426; `GetRenderedGroundHeight` L457.

```csharp
L13: private const float RiverMargin = 8f;
L14: private const float RiverStep = 1.25f;
L15: private const float RiverHalfWidth = 3.75f;
L16: private const float RiverWaterLift = 0.24f;
L17: private const float BridgeZ = 22.0f;
L18: private const float BridgeDryHalfZ = 4.8f;
L240: const float cell = 8f;
```


#### R `PrototypeUiTheme09F15.cs` (117 linjer; `c54f65e26acb`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeUiTheme09F15.cs).

Klasser: `PrototypeUiTheme09F15` L5.

Indgange og kildeankre: `Panel` L22; `Header` L34; `Label` L47; `MutedLabel` L57; `Button` L64; `AccentBox` L82; `EnsureTextures` L95.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeUiTheme09F15.cs` (146 linjer; `1f3ee7bbdf6a`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeUiTheme09F15.cs).

Klasser: `PrototypeUiTheme09F15` L5.

Indgange og kildeankre: `Panel` L24; `Header` L36; `Label` L49; `MutedLabel` L59; `Section` L66; `Button` L74; `AccentBox` L92; `DangerBox` L105; `EnsureTextures` L122.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### T `PrototypeUnderFireReaction09F26.cs` (524 linjer; `fc0d4c825861`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeUnderFireReaction09F26.cs).

Klasser: `PrototypeUnderFireReaction09F26` L12; `ReactionState` L14; `DeferredState` L25.

Indgange og kildeankre: `AutoCreate` L51; `Awake` L57; `OnDestroy` L83; `OnDisable` L90; `Update` L95; `IsReacting` L133; `IsFreshVolleyEvent` L138; `TryStartReaction` L149; `TryStartDeferred` L189; `StartReaction` L227; `UpdateReaction` L263; `ApplyReturnFire` L318; `FinishReaction` L351; `FindNearestEnemyInsideLong` L375; `ReadUnderFire` L400; `HasDestination` L406; `TurnToward` L412; `Cleanup` L426; `RestoreAll` L469; `FirstSelectedReactingCompany` L505; `PlanarDistance` L518.

```csharp
L44: private const float VolleyJumpThreshold = 0.55f;
L45: private const float FreshUnderFireThreshold = 6.20f;
L46: private const float MinimumReactionSeconds = 4.0f;
L47: private const float QuietReleaseSeconds = 7.25f;
L48: private const float DeferredBridgeSeconds = 20.0f;
```


#### T `PrototypeUnifiedCommandHud09F29G.cs` (848 linjer; `45fdf337030a`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeUnifiedCommandHud09F29G.cs).

Klasser: `PrototypeUnifiedCommandHud09F29G` L16; `AggregateStats` L719.

Indgange og kildeankre: `AutoCreate` L52; `Awake` L59; `OnDestroy` L79; `Update` L85; `DisableSupersededHudComponents` L97; `DrawHigherHud` L157; `DrawRegimentalHud` L279; `DrawMajorHud` L325; `DrawCompanyHud` L377; `DrawInfoAiBlock` L460; `DrawOfficerOrderGrid` L477; `DrawOfficerOrderButton` L502; `BeginHigherOrder` L518; `BeginRegimentalOrder` L526; `BeginBattalionOrder` L532; `DrawFireButton` L538; `SetSelectedAi` L546; `SetSelectedDoctrine` L555; `BeginWithdrawal` L564; `SetForcedMarch` L570; `BeginChargePick` L579; `StopSelected` L586; `SetFormation` L602; `FinishCharge` L627; `CancelWithdrawal` L634; `ClearRoute` L641; `GetSelectedBattalionIndex` L669; `GetSelectedCompanies` L677; `GetAllCompanies` L688; `ApplyVisibleCompanyNames` L701; `CalculateStats` L725; `CalculateStats` L750; `All` L755; `Any` L762; `FormationLabel` L769; `ShortDoctrine` L776; `State` L781; `DrawSection` L782; `BuildStyles` L784; `MakeButtonStyle` L824.

```csharp
L20: private const float HudHeight = 96f;
L485: const float gap = 4f;
```


#### K `PrototypeUniformDesigner09H.cs` (276 linjer; `46cec500a1a8`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeUniformDesigner09H.cs).

Klasser: `PrototypeUniformDesigner09H` L7.

Indgange og kildeankre: `AutoCreate` L27; `Awake` L36; `OnDestroy` L47; `Update` L53; `IsPointerOverControls` L65; `RefreshSelection` L74; `FindSingleSelectedRegiment` L87; `DrawWindow` L137; `DrawChannel` L228; `ApplyWorking` L238; `GetSelectedColor` L245; `SetSelectedColor` L261.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### K `PrototypeUniformProfile09H.cs` (132 linjer; `0efd3131cc0c`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeUniformProfile09H.cs).

Klasser: `PrototypeUniformProfile09H` L5.

Indgange og kildeankre: `Clone` L18; `CreateFactionDefault` L35; `CreateDenmarkDefault` L71; `CreatePrussiaDefault` L76; `CreateRegimentDefault` L81.

Ingen simple numeriske/bool-defaultdefinitioner i dette script; regler/inputs findes ved ovenstående funktioner.


#### R/T `PrototypeUnitFootprint09F5.cs` (134 linjer; `994c9696057e`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeUnitFootprint09F5.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeUnitFootprint09F5.cs).

Klasser: `PrototypeUnitFootprint09F5` L6.

Indgange og kildeankre: `AutoCreate` L12; `LateUpdate` L21; `UpdateCollider` L40; `HideLegacySelectionDisc` L53; `UpdateSelectionOutline` L67; `UpdateRangeFans` L97; `RebuildFan` L107.

```csharp
L116: const int arcSegments = 40;
L133: private const float regimentHalfArc = 60f;
```


#### K `PrototypeVisualReadability09I2.cs` (141 linjer; `81f95f6ea84f`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/PrototypeVisualReadability09I2.cs).

Klasser: `PrototypeVisualReadability09I2` L8; `FlagState` L10.

Indgange og kildeankre: `AutoCreate` L31; `Start` L40; `Update` L45; `CleanupDestroyed` L120.

```csharp
L14: public float AppliedScale = -1f;
L25: private const float NearDistance = 70f;
L26: private const float FarDistance = 300f;
L27: private const float NearScale = 1.18f;
L28: private const float FarScale = 1.52f;
```


#### R/T `PrototypeVolleyTelemetryAudio09F13.cs` (240 linjer; `8689fdf23a68`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeVolleyTelemetryAudio09F13.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeVolleyTelemetryAudio09F13.cs).

Klasser: `PrototypeVolleyTelemetryAudio09F13` L10; `State` L12.

Indgange og kildeankre: `AutoCreate` L25; `Awake` L34; `Update` L46; `OnVolley` L82; `FindVolleyTarget` L122; `ReadNextFireTime` L161; `EnsureAudioSource` L167; `CreateProceduralVolleyClip` L184; `PlanarDistance` L211; `Cleanup` L218.

```csharp
L186: const int sampleRate = 44100;
L187: const float duration = 0.72f;
```


#### T `PrototypeWideBattlefieldNavigation09F29E.cs` (486 linjer; `1df8c98d8288`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeWideBattlefieldNavigation09F29E.cs).

Klasser: `PrototypeWideBattlefieldNavigation09F29E` L11; `Obstacle` L13; `NavigationState` L20.

Indgange og kildeankre: `AutoCreate` L53; `Awake` L60; `OnDestroy` L88; `Update` L95; `DisableObsoleteNavigation` L117; `EnsureObstacles` L146; `AddObstacle` L172; `UpdateUnit` L178; `PlanSteering` L276; `WriteTarget` L366; `FindFirstBlockingObstacle` L375; `IsPointBlocked` L396; `ClampToBattlefield` L411; `DistancePointToSegment` L421; `Flat` L438; `PlanarDistance` L446; `RestoreFormation` L453; `RestoreAllFormations` L462; `Cleanup` L468.

```csharp
L45: private const float GoalChangeTolerance = 1.25f;
L46: private const float DetourArrival = 2.5f;
L47: private const float TravelClearance = 3.8f;
L48: private const float ReplanInterval = 0.28f;
L49: private const float StuckSeconds = 1.2f;
L50: private const float EdgeMargin = 18f;
```


#### R/T `PrototypeWithdrawalButtons09F10.cs` (80 linjer; `0c1a1880a8f9`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/PrototypeWithdrawalButtons09F10.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/PrototypeWithdrawalButtons09F10.cs).

Klasser: `PrototypeWithdrawalButtons09F10` L6.

Indgange og kildeankre: `AutoCreate` L11; `GetSelectedDanishCount` L61.

```csharp
L44: const float h = 27f;
```


#### K `RTSCameraController.cs` (104 linjer; `4edcb7f4ef05`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/RTSCameraController.cs).

Klasser: `RTSCameraController` L3.

Indgange og kildeankre: `Update` L14; `HandleCameraInput` L19.

```csharp
L5: [SerializeField] private float moveSpeed = 72f;
L6: [SerializeField] private float rotateSpeed = 82f;
L7: [SerializeField] private float zoomStep = 4.8f;
L8: [SerializeField] private float minGroundClearance = 3.8f;
L9: [SerializeField] private float maxHeight = 300f;
L11: private const float MapHalfWidth = 350f;
L12: private const float MapHalfDepth = 230f;
```


#### R/T `RTSCameraController.cs` (78 linjer; `54cd8caad37a`)

Kilde: [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/RTSCameraController.cs), [T](R:/Onedrive/Strategy-Test/Assets/Scripts/RTSCameraController.cs).

Klasser: `RTSCameraController` L3.

Indgange og kildeankre: `Update` L11; `HandleCameraInput` L16.

```csharp
L5: [SerializeField] private float moveSpeed = 150f;
L6: [SerializeField] private float rotateSpeed = 75f;
L7: [SerializeField] private float zoomStep = 9.5f;
L8: [SerializeField] private float minHeight = 9.5f;
L9: [SerializeField] private float maxHeight = 600f;
```


#### K/R `Regiment.cs` (720 linjer; `2f92b3e8912e`)

Kilde: [K](R:/Onedrive/Strategy-Kamp/Assets/Scripts/Regiment.cs), [R](R:/Onedrive/Strategy-Kampe-Rebuild/Assets/Scripts/Regiment.cs).

Klasser: `Regiment` L30.

Tilstands-/data-enums: `BattleTeam` = Denmark, Prussia (L4); `RegimentFormation` = Line, Column (L10); `InfantryWeaponType` = RifledMuzzleLoader, DreyseNeedleRifle (L16); `RegimentFirePolicy` = HoldFire, CloseRange, MediumRange, LongRange (L22).

Indgange og kildeankre: `Initialize` L78; `ApplyWeaponProfile` L114; `GetPrototypeExperience` L140; `GetExperienceReloadMultiplier` L159; `CreateVisuals` L166; `CreateRangeFans` L239; `CreateRangeFan` L262; `Update` L294; `UpdateAI` L347; `UpdateMovement` L382; `UpdateSoldierFormation` L417; `GetFormationPosition` L431; `FindNearestEnemy` L450; `FindNearestEnemyInFireArc` L471; `GetFireTriggerRange` L492; `GetFirePolicyLabel` L507; `IsTargetInFireArc` L522; `CanFireAt` L540; `SetFirePolicy` L549; `GetAttackStopRange` L554; `GetRangeAccuracyMultiplier` L562; `FireVolley` L570; `FaceTarget` L591; `ReceiveVolley` L605; `Route` L622; `RefreshVisualStrength` L639; `SetSelected` L646; `RefreshRangeVisibility` L656; `SetRangeVisualEnabled` L661; `OrderMove` L671; `OrderAttack` L682; `OrderHold` L701; `SetFormation` L710.

```csharp
L36: public float Morale { get; private set; } = 100f;
L37: public float Cohesion { get; private set; } = 100f;
L38: public float Experience { get; private set; } = 50f;
L54: public bool ShowRange { get; set; } = true;
L273: const int arcSegments = 28;
L274: const float muzzleHalfWidth = 8.5f;
L275: const float muzzleZ = 1.30f;
```


#### T `Regiment.cs` (995 linjer; `fa7b8d232a7b`)

Kilde: [T](R:/Onedrive/Strategy-Test/Assets/Scripts/Regiment.cs).

Klasser: `Regiment` L30.

Tilstands-/data-enums: `BattleTeam` = Denmark, Prussia (L4); `RegimentFormation` = Line, Column (L10); `InfantryWeaponType` = RifledMuzzleLoader, DreyseNeedleRifle (L16); `RegimentFirePolicy` = HoldFire, CloseRange, MediumRange, LongRange (L22).

Indgange og kildeankre: `Initialize` L85; `ApplyWeaponProfile` L121; `GetPrototypeExperience` L147; `GetExperienceReloadMultiplier` L166; `CreateVisuals` L173; `CreateRangeFans` L246; `CreateRangeFan` L269; `Update` L301; `UpdateAI` L370; `UpdateMovement` L405; `UpdateSoldierFormation` L440; `GetFormationPosition` L454; `FindNearestEnemy` L473; `FindNearestEnemyInFireArc` L494; `GetFireTriggerRange` L515; `GetFirePolicyLabel` L530; `IsTargetInFireArc` L545; `CanFireAt` L563; `IsTargetInFireArc` L572; `CanFireAt` L592; `CanSee` L609; `FindNearestEnemyCavalryInFireArc` L656; `SetFirePolicy` L692; `GetAttackStopRange` L698; `GetRangeAccuracyMultiplier` L709; `FireVolley` L717; `FireVolley` L738; `FaceTarget` L812; `ReceiveVolley` L826; `Route` L843; `RefreshVisualStrength` L861; `SetSelected` L868; `RefreshRangeVisibility` L878; `ShouldShowRangeVisuals` L883; `SetRangeVisualEnabled` L897; `SetRangeFanEmphasis` L926; `OrderMove` L946; `OrderAttack` L957; `OrderHold` L976; `SetFormation` L985.

```csharp
L35: private const bool TestShowEnemyRangeCones = true;
L43: public float Morale { get; private set; } = 100f;
L44: public float Cohesion { get; private set; } = 100f;
L45: public float Experience { get; private set; } = 50f;
L61: public bool ShowRange { get; set; } = true;
L280: const int arcSegments = 28;
L281: const float muzzleHalfWidth = 8.5f;
L282: const float muzzleZ = 1.30f;
```


### Unreal-kildeankre

Disse links er implementationer, ikke runtimebevis.

- [AI/StrategyFieldOfficerComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyFieldOfficerComponent.cpp:215): `ThinkInfantry`.
- [AI/StrategyAutonomousBattleAIComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyAutonomousBattleAIComponent.cpp:35): `TickComponent`.
- [AI/StrategyOfficerAIComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyOfficerAIComponent.cpp:39): `EvaluateInheritedMission`.
- [AI/StrategyDoctrineComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyDoctrineComponent.cpp:10): `GetEffectiveAggression`.
- [AI/StrategyOfficerProfileComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyOfficerProfileComponent.cpp:31): `GetDecisionStability`.
- [Orders/StrategyOrderComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Orders/StrategyOrderComponent.cpp:40): `CanReplaceCurrentOrder`.
- [Orders/StrategyOrderTypes.h](R:/Onedrive/cx/mapai/Source/Strategy1864/Orders/StrategyOrderTypes.h:78): `IsStandingIntent`.
- [Orders/StrategyParentExecutionComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Orders/StrategyParentExecutionComponent.cpp:1).
- [Formations/StrategyParentFormationPlannerComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Formations/StrategyParentFormationPlannerComponent.cpp:54): `IssueCompanySlotsForOrder`.
- [Formations/StrategyFormationPolicyComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Formations/StrategyFormationPolicyComponent.cpp:124): `DeployDistanceCm`.
- [Formations/StrategyFormationTransitionComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Formations/StrategyFormationTransitionComponent.cpp:28): `HandleFormationChanged`.
- [Combat/StrategyCombatComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Combat/StrategyCombatComponent.cpp:270): `NotifyIncomingVolley`.
- [Combat/StrategyFireControlComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Combat/StrategyFireControlComponent.cpp:263): `CanEngageTarget`.
- [Combat/StrategyThreatReactionComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Combat/StrategyThreatReactionComponent.cpp:19): `TickComponent`.
- [Combat/StrategyCavalryChargeComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Combat/StrategyCavalryChargeComponent.cpp:79): `TickComponent`.
- [Movement/StrategyLocalDeconflictionComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Movement/StrategyLocalDeconflictionComponent.cpp:18): `TickComponent`.
- [Movement/StrategyMovementExecutorComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Movement/StrategyMovementExecutorComponent.cpp:1).
- [Navigation/StrategyRoutePlannerComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Navigation/StrategyRoutePlannerComponent.cpp:1).
- [AI/StrategyCavalryScreenAIComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyCavalryScreenAIComponent.cpp:21): `TickComponent`.
- [AI/StrategyCavalryTaskingComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyCavalryTaskingComponent.cpp:46): `AssignDefensiveReserve`.
- [AI/StrategyHQFollowComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyHQFollowComponent.cpp:1).
- [AI/StrategyRoutRecoveryComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/AI/StrategyRoutRecoveryComponent.cpp:1).
- [Units/StrategyUnit.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Units/StrategyUnit.cpp:62): `AStrategyUnit::AStrategyUnit`.
- [Units/StrategyCompanyUnit.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Units/StrategyCompanyUnit.cpp:1).
- [Units/CavalryUnit.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Units/CavalryUnit.cpp:1).
- [Units/StrategyDragoonComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Units/StrategyDragoonComponent.cpp:55): `DismountAtCurrentPosition`.
- [Artillery/StrategyArtilleryFireMissionComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Artillery/StrategyArtilleryFireMissionComponent.cpp:224): `SetAutoTargetEnabled`.
- [Visual/StrategyInfantryVisualComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Visual/StrategyInfantryVisualComponent.cpp:122): `HandleVolleyVisualEvent`.
- [Visual/StrategyCavalryVisualComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Visual/StrategyCavalryVisualComponent.cpp:1).
- [Visual/StrategyHumanAnimationStateComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Visual/StrategyHumanAnimationStateComponent.cpp:1).
- [Visual/StrategyMountedAnimationSyncComponent.cpp](R:/Onedrive/cx/mapai/Source/Strategy1864/Visual/StrategyMountedAnimationSyncComponent.cpp:1).
