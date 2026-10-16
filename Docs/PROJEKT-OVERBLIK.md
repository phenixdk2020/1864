# PROJECT 1864 - samlet overblik

Dette dokument forklarer hele projektet på ét sted: hvad spillet er, hvor tingene ligger, hvordan slaget hænger sammen, hvordan animationer og mængdetegning virker, hvordan man bygger og tester, og hvad der mangler. Det supplerer `README.md` (kanaler og kom-i-gang), `AGENTS.md` (regler for AI-agenter og historisk tilgang), `Docs/PROJECT-1864-Designmanual.md` (designet) og `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md` (alt, der er lavet, med datoer). Overdragelsesnotatet til den næste agent er `Docs/HANDOVER-2026-10-16.md`.

## 1. Hvad spillet er

Et 3D-realtidsstrategispil i Unreal Engine 5.8 om europæisk krig og diplomati i 1800-tallet. Spillet starter 1. juli 1825 (rolig åbning), scenariet 1851 kan vælges, og 1864 er referencen for slaget. Danmark er første spilbare nation, flere skal kunne spilles. Historien er udgangspunkt, ikke manuskript: spillerens valg skal kunne ændre forløbet.

Spillet har to dele, der hænger sammen:
- **Kampagnen** (`Source/Game1864`, `Content/Campaign1851`, `Data/Campaign1851`): det malede kort over Danmark (LAEA-projektion), byer, økonomi, ministre, diplomati, begivenheder, hær og forsyning.
- **Slaget** (`Source/Strategy1864`): 3D-kamp med kompagnier, batterier og eskadroner, taktisk kamera og efterfølgende kamprapport (AAR).

## 2. Mapper

| Mappe | Indhold |
|---|---|
| `Source/Game1864` | Kampagnemodulet (kort, overlay, controller) |
| `Source/Strategy1864` | Slagmodulet (se afsnit 3) |
| `Content/Battle`, `Content/Units` | Bagte mængdefigurer (`Battle/VAT`), materialer, enhedsmodeller og animationer (`Units/Human/CombatAnimations`) |
| `Content/Python` | Import- og bagescripts (`bake_all_vat.py`, `import_infantry_combat_set.py`, `make_crowd_material.py` m.fl.) |
| `Data/Campaign1851` | Spildata som JSON/CSV: hære, økonomi, nationer, begivenheder, kort |
| `SourceAssets/InfantryCombat` | Kilde-FBX og manifester for de importerede kampklip |
| `Tools/Animation` | Blender-scripts, der bygger klippene (skulder, sabel, genladning, Mixamo-extras) |
| `Tools/*` | Kort-, økonomi-, rapport- og MCP-værktøjer |
| `Reference` | Referencebilleder (ikoner, enheder, animation) |
| `Docs` | Alle designdokumenter; `Docs/Design`, `Docs/Arkiv` |
| `Start-*.bat` | Genveje til spil og tests (afsnit 6) |

Kanaler (git): `channel-campaign` er `R:\Onedrive\1864-Campaign` (her arbejdes der; slagkoden ligger nu også her), `channel-battle` er `R:\Onedrive\1864-Battle` (udfaset som arbejdsmappe), `main` er det fælles grundlag.

## 3. Slagmodulet i korte træk

`Source/Strategy1864` er delt i mapper efter ansvar. Hver enhed er en actor (`AStrategyUnit` og underklasser) med mange komponenter.

- **Units**: `StrategyUnit` (basis), `StrategyCompanyUnit` (kompagni, har blandt andet `InfantryVisualComponent`), `CavalryUnit`, `StrategyHQUnit`, artilleribatterier (`Artillery/`).
- **Orders**: `StrategyOrderComponent` tager imod ordrer (`StrategyOrderTypes`: Hold, Move, AttackHere, DefendHere, Disengage osv.) med autoritet (spiller, officer-AI), forsinkelse og udførelsestilstand.
- **AI**: `StrategyFieldOfficerComponent` (kaptajnens lokale beslutninger: fald tilbage, charge, svinge front), `StrategyAutonomousBattleAIComponent` (fjendens overordnede AI), doktrin, NCO, HQ-følge og rytteri-AI. Beslutninger logges med `STRATEGY1864_DECISION`.
- **Combat**: `StrategyFireControlComponent` (hvem der må skyde: skydekegle, fjendens fjerneste hjørne, rækkevidde), ildrytme (`FireDrill`, `FireDiscipline`), kontakt, tab (`ResolveCasualties`), skyttekæde og feltværker.
- **Formations**: `StrategyFormationComponent` (slots, geledder, frontbredde, `RankCount` = 3), overgange mellem formationer, forældreplanlægger.
- **Movement**: `StrategyMovementExecutorComponent` flytter enheden; lokal deconfliction.
- **Artillery**: ammunition, ildmission, rækkevidde (`GetEffectiveRangeCm`), nedslag, bemanding, afprotsning og beprotsning.
- **Player**: `StrategyPlayerController` (markering, højreklik, ordrer), `StrategyHUD` (alt UI: panelerne, ordreknapper, skydekegler, AAR), `StrategyCameraPawn` (kamera, kan køre under pause).
- **Visual**: se afsnit 4.
- **Tests**: `StrategyOOBTestScenario` (scenarier til tests, skærmbilleder og testflag), `StrategyBehaviourTests` (acceptance), `StrategyClipViewer`.

Slagets tid: `UGameplayStatics::SetGlobalTimeDilation` styrer hastigheden. Pause er `SetGamePaused`; kontrolleren og kameraet ticker under pause, og infanterikomponentens tid står stille.

## 4. Figurer, animation og mængdetegning

- **Skelet og modeller**: alle fodfolk deler skelettet `SK_Human_1864` (41 Mixamo-knogler). Hver enhedstype er en skeletal mesh (`SK_DK_Infantry_1864`, `SK_DK_Livgarden_1864`, `SK_SE_Infantry_1864` m.fl.).
- **Klip**: Mixamo-klip og egne klip (Blender) ligger som `A_Combat_*` i `Content/Units/Human/CombatAnimations` og i enhedernes egne animationsmapper. De oprindelige otte klip må aldrig genimporteres.
- **Bagt mængde (VAT)**: `UStrategyCrowdModel` bager hver soldat plus gevær til en statisk mesh og en bone-tekstur (30 FPS, tre matrixtexler pr. knogle, ca. 7.500 rækker). Materialet `M_CrowdVAT_All` læser tekstursrækken ud fra per-instans-data (klippets første række, antal billeder, starttid, hastighed). Geværets position pr. billede beregnes i bagningen (mellem hænderne, eller på skulderen for klip med "Shoulder" i navnet).
- **Tegning i slaget**: `UStrategyInfantryVisualComponent` styrer hver mand: slot i formationen, klip (gå, løb, sigt, skyd, lad, dø, skulder), personlige skydefaser, omformning (`PlanOrderlyReform`), tab (`KillSoldiers`, `StepUpFiles`, lukning af geledder). Nærmeste figurer kan tegnes som rigtige skeletal meshes, resten som bagte instanser.
- **Skuldergevær i kolonne**: marchen vælger WalkShoulder/ShoulderUp/ShoulderDown i `UpdateSettling` ved formationen Kolonne; `ResolveAnimation` bruges til stående tilstande.
- **Eksport fra Blender**: brug Mixamos egne akser (`axis_forward='-Z', axis_up='Y'`), ellers ligger mændene ned i spillet.
- **Bagning og import**: `UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Content/Python/bake_all_vat.py -unattended -nosplash -AllowCommandletRendering`. Efter hver bagning: tag et skærmbillede i kolonne og linje, og se klippene i klip-viseren.

## 5. Kampagnen

Data ligger i `Data/Campaign1851` (hære og naboer for 1825 og 1851, økonomi, nationer, ministre, begivenheder, kort og højdedata). Kampagnekortet tegnes af `ACampaign1851Map`; `Campaign1851PlayerController` styrer kamera og klik. Scenarier vælges i `Campaign1851Scenarios.cpp`. Al ny data skal kunne vælges pr. scenarie, og 1851 må ikke ændres utilsigtet. Landeskabelonen (`Docs/Landeskabelon.md`) beskriver, hvad der skal indsamles for et nyt land, og `Docs/Uniformer-Lande.md` planen for uniformer og figurer.

## 6. Start, byg og test

- **Spil**: `Start-Kampagne-NytSpil.bat` (kampagne), `Start-Test-1-Kompagni-mod-Kompagni.bat`, `-2-Bataillon-mod-Kompagni`, `-3-Rytteri`, `-4-Kanoner`, `Start-Test-Adfaerd.bat`, `Start-Klipviser.bat` (alle bagte klip, filter med `-Strategy1864ClipViewer=<navn>`).
- **Byg**: `I:\Spil\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat Game1864Editor Win64 Development R:\Onedrive\1864-Campaign\Game1864.uproject -waitmutex`. Editoren og spillet skal være lukket. Fuld build tager 1-10 minutter.
- **Acceptance**: `UnrealEditor.exe` med `/Game/Maps/Strategy1864_Skirmish -game -windowed -nosplash -unattended -Strategy1864FlatQA -Strategy1864AcceptAll -ABSLOG=<fil>`; forventet `pass=31 fail=7` (T2, T4, T5, T6, T8, T9, T10 er endnu ikke implementeret). Enkelt test: `-Strategy1864Accept=<ID>`.
- **Nyttige flag**: `-Strategy1864Shots=sek:enhed:afstand:pitch` med `-Strategy1864ShotsQuit` (skærmbilleder til `Saved/Screenshots`), `-Strategy1864AnimationAudit`, `-Strategy1864DebugDecisions`, `-Strategy1864DebugGpu`, `-Strategy1864TimeScale=n`, `-Strategy1864PauseCameraTest`, `-Strategy1864EnemyTurn=<grader>`, `-Strategy1864NoFarCorner`, `-Strategy1864SkirmishArms`.
- **Git Bash** ændrer `/Game/...`-stier; start spillet fra PowerShell.

## 7. Arbejdsgang og regler

1. Ret, byg, test (acceptance og skærmbillede), commit, push (`git push origin HEAD:channel-campaign`).
2. Hver commit får en dateret post i `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md` (før `### Næste skridt`) og commit-trailer med modelnavn.
3. Dræb aldrig brugerens egen editor eller spil. Kør aldrig `Tools/Battle/Sync-Battle.ps1`.
4. Codex-prompter skal sige, at UE 5.8-headerne findes i `I:\Spil\Epic Games\UE_5.8\Engine\Source`.
5. Officerer dør ikke i slag (kun sårede eller fangne).
6. Rapportér ærligt, hvad der er set på skærmbillede, og hvad der kun er testet i kode.

## 8. Lande, Codex og data

Codex-agenter indsamler historiske data og uniformer i `C:\Users\allan\Documents\Codex\2026-10-12\l\outputs\` (en mappe pr. land plus `Uniformer-agent`). Intet derfra er importeret i spillet endnu. Sverige-Norge er længst (runde 7 færdig, 30 JSON-overlays i `Spilimport/GameReady`, dokumenteret ikke spilbart endnu). Codex køres med `codex.exe exec --skip-git-repo-check -s workspace-write -c sandbox_workspace_write.network_access=true -C <mappe> "<prompt>"`; ved brugsgrænse skal man vente til det angivne klokkeslæt.

## 9. Kendte mangler (kort)

Animationer der "hopper", mænd der ikke når på plads ved formationsskift, svagere grafik før formation, nedliggende mænd under pause (ikke bekræftet løst), de andre nye klip (sabel, genladning V03) er ikke genimporteret med de rettede akser, første salve føles forsinket, gevær-/ladestok-rekvisitter og skalering af ladetiden, sabelsæt og til- og afstigning for rytteri, tests T2, T4, T5, T6, T8, T9 og T10, kort og uniformer for lande ud over Danmark og den svenske stand-in. Den fulde liste står i `Docs/HANDOVER-2026-10-16.md`, afsnit 4.
