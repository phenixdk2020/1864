# Testflag og launchere

Status 2026-10-09. Oversigten er kontrolleret mod `FParse::Param`/`FParse::Value` i `Source/Game1864` og `Source/Strategy1864`, inklusive ryttervisningens `Flag`-hjælper. Dette er dokumentation, ikke tilladelse til at starte spillet.

Eksemplerne er argumenter, som kan tilføjes en relevant launcher (alle videresender `%*`). ID'er og indeks skal findes i det valgte scenarie/testslag; eksempler viser syntaks, ikke garanteret succes. Sæt hele argumentet i dobbelte anførselstegn ved semikolon/komma. Flag uden værdi aktiveres ved tilstedeværelse; `=0` er ikke en generel deaktivering.

## Kampagne

**Startbegrænsning:** normal opstart viser startmenuen og returnerer før mange af initialiseringsflagene. `CampaignBuild=<by>` åbner teststart; efter nyspilsmarkøren fra menuen kan initialiseringen også fortsætte. `CampaignSeed`, `CampaignDeviation` og `CampaignNation` læses inde i nyt-spil-funktionen. `CampaignNew` forekommer i ældre launchere, men har ingen parser i den aktuelle kildekode og starter derfor ikke selv nyt spil.

| Flag | Virkning | Eksempelargument |
|---|---|---|
| `CampaignAutoBuild` | Udbygger garnisonskomplekser løbende, betalt normalt. | `-CampaignAutoBuild` |
| `CampaignBuild` | Starter byens garnisonsprojekt og åbner teststart uden startmenu. | `-CampaignBuild=Odense` |
| `CampaignSpeed` | Sætter kampagnens hastighedstrin (0=pause). | `-CampaignSpeed=1` |
| `CampaignDate` | Sætter kampagnedag fra ISO-dato. | `-CampaignDate=1852-01-20` |
| `CampaignBuildTown` | Bestiller bybygninger; semikolon mellem ordrer. | `-CampaignBuildTown=Odense:Textile_Mill` |
| `CampaignFocusBuilding` | Fokuserer kamera på bybygning. | `-CampaignFocusBuilding=Odense:Textile_Mill` |
| `CampaignBuildLink` | Bestiller bane (b...) eller chaussé på eksisterende forbindelse. | `-CampaignBuildLink=Odense:Nyborg:chaussee` |
| `CampaignMarch` | Sender angivne regimenter til by eller @lat,lon; valgfri :road/roadonly/direct. | `-CampaignMarch=B9,D2:Randers:road` |
| `CampaignSelectRegiments` | Vælger regiment-id'er, kommasepareret. | `-CampaignSelectRegiments=B9,D2` |
| `CampaignOpenPicker` | Åbner officersvalg: chief/general. | `-CampaignOpenPicker=chief` |
| `CampaignOpenTraining` | Åbner træningsmenu. | `-CampaignOpenTraining` |
| `CampaignDiplomacy` | Udfører nation:handling; 0 gesandt, 1 handel, 2 alliance, 3 garanti. | `-CampaignDiplomacy=SE:0;GB:3` |
| `CampaignResearchDone` | Tildeler forsknings-id'er straks. | `-CampaignResearchDone=conserves,sanitation` |
| `CampaignResearch` | Starter et forskningsemne. | `-CampaignResearch=staff` |
| `CampaignDoctrine` | Sætter niveau:valg, kommasepareret. | `-CampaignDoctrine=0:1,2:2` |
| `CampaignWorks` | Bestiller historiske værker: 0 Dannevirke, 1 Dybbøl, 2 Fredericia. | `-CampaignWorks=0,1` |
| `CampaignFortsComplete` | Færdiggør værker sammen med Works eller BuildFort. | `-CampaignFortsComplete` |
| `CampaignFormCommand` | Danner felthærsformation fra generalkommandoens indeks. | `-CampaignFormCommand=0` |
| `CampaignYearlyOfficers` | Kører årlig officersopdatering straks. | `-CampaignYearlyOfficers` |
| `CampaignOpenWindow` | Åbner army/officers/budget/trains/chart/council/supply/foreign/research/navy/battlefield/materiel/nations/gazette/end; ellers towns. | `-CampaignOpenWindow=research` |
| `CampaignBattlefield` | Genererer slagmark ved lat,lon,km (km standard 8). | `-CampaignBattlefield=54.304,9.663,8` |
| `CampaignBattleView` | Viser genereret 3D-model sammen med Battlefield. | `-CampaignBattleView` |
| `CampaignResearchPick` | Sætter valgt forskningsindeks. | `-CampaignResearchPick=0` |
| `CampaignForeignTab` | Sætter udenrigsfaneindeks. | `-CampaignForeignTab=0` |
| `CampaignGazetteTab` | Sætter avisfaneindeks. | `-CampaignGazetteTab=0` |
| `CampaignSupplyMap` | Slår forsyningskort til/fra. | `-CampaignSupplyMap` |
| `CampaignBuildFort` | Bestiller lat,lon,stor/lille,frontretning; semikolon mellem ordrer. | `-CampaignBuildFort=54.91,9.75,stor,135` |
| `CampaignDemolishFort` | Nedlægger fort-id; læses kun sammen med BuildFort. | `-CampaignDemolishFort=1` |
| `CampaignFortGarrison` | Tilføjer fort:regiment:kompagni; læses kun sammen med BuildFort. | `-CampaignFortGarrison=1:B12:0,B12:1` |
| `CampaignWarTest` | Fremtvinger krig; springes over ved retur fra slag. | `-CampaignWarTest` |
| `CampaignAutoBattles` | Afgør kampagneslag automatisk. | `-CampaignAutoBattles` |
| `CampaignMobilise` | Mobiliserer; springes over ved retur fra slag. | `-CampaignMobilise` |
| `CampaignDelegate` | Sætter alle ministerområder til auto eller advisory. | `-CampaignDelegate=advisory` |
| `CampaignTestFieldArmy` | Opretter testfelthær. | `-CampaignTestFieldArmy` |
| `CampaignOpenOOB` | Åbner/lukker kamporden. | `-CampaignOpenOOB` |
| `CampaignSplitTo` | Åbner og udfører marchdialog: by:regimenter der bliver. | `-CampaignSplitTo=Vejle:B7` |
| `CampaignSplitShow` | Viser kun dialogen fra SplitTo. | `-CampaignSplitShow` |
| `CampaignInspectOfficer` | Åbner officerskort efter officer-id. | `-CampaignInspectOfficer=O1` |
| `CampaignSelectCity` | Vælger by. | `-CampaignSelectCity=Odense` |
| `CampaignSelectAmt` | Vælger amt efter indeks. | `-CampaignSelectAmt=0` |
| `CampaignOpenLedger` | Åbner regnskab. | `-CampaignOpenLedger` |
| `CampaignOpenMenu` | Åbner spilmenu. | `-CampaignOpenMenu` |
| `CampaignView` | Sætter kamera: lat,lon,afstand-km, valgfri yaw i grader. | `-CampaignView=55.4,10.4,20,0` |
| `CampaignMenuShot` | Tager startmenu-billede efter 10 realtidssekunder og afslutter processen 3 sekunder senere. | `-CampaignMenuShot` |
| `CampaignDebugClicks` | Logger klik og valgændringer. | `-CampaignDebugClicks` |
| `CampaignShotAt` | Skærmbilleder på kommaseparerede realtidspunkter. | `-CampaignShotAt=10,20` |
| `CampaignUiShots` | UI-handlinger og billeder: sek:kommando;kommando,sek:kommando. | `-CampaignUiShots=10:window=research;rtab=1,20:clear` |
| `CampaignUiShotsQuit` | Afslutter processen efter sidste UI-billede. | `-CampaignUiShotsQuit` |
| `CampaignFight3D` | Sender første ikke-ventende kampagneslag til 3D og gemmer Autosave. | `-CampaignFight3D` |
| `CampaignAutoClick` | Simulerer venstreklik: realtidssek:x:y;... | `-CampaignAutoClick=10:500:300;20:600:400` |
| `CampaignSeed` | Seed ved nyt spil; starter ikke selv nyt spil. | `-CampaignSeed=42` |
| `CampaignDeviation` | Historisk afvigelse i procent ved nyt spil. | `-CampaignDeviation=20` |
| `CampaignNation` | Spillernation ved nyt spil (DK/SE); SE har ikke eget detaljeret kort. | `-CampaignNation=DK` |
| `CampaignScenario` | Vælger scenarie-id ved initialisering; standard 1825, også 1851. | `-CampaignScenario=1825` |
| `CampaignEvents` | Tvinger uafgjorte events trods dato, betingelser og chance ved næste evaluering. | `-CampaignEvents=abent,kiel` |
| `CampaignEventWhy` | Logger daglig forklaring af et event uden at udløse det. | `-CampaignEventWhy=kiel` |
| `CampaignEventLog` | Logger månedlige eventevalueringer, også ikke-aktuelle/afgjorte. | `-CampaignEventLog` |

`CampaignUiShots` bruger realtid og tager billedet 1,5 sek. efter handlingerne. Kommandoer: `window`, `minister`, `officer`, `select`, `focus`, `kamporden`, `selectmany` (indeks med +), `genfield` (lat+lon), `split`, `unitcard`, `rtab`, `pool`, `unitco`, `oob`, `city`, `tab`, `civil`, `info`, `clear`. Vinduer her: none/army/officers/budget/towns/trains/chart/council/supply/foreign/research/navy/status. Listen er forskellig fra `CampaignOpenWindow`.

## Slag

Opdatering 2026-10-10: `Strategy1864DebugDecisions` viser også den fælles missionspause (lag `ReactionAI`), carré-cooldownens klarstatus og afvist automatisk SPRED under cooldown. Ingen nye T1/T2/T3-acceptflag er implementeret endnu.

Manuel regression til senere godkendt kørsel: giv FLYT med flere vejpunkter i ryttertesten og kontroller, at automatisk carré stopper bevægelsen, bevarer ordre/serial/rute og først genoptager efter 20 s uden trussel, færdig reformering og den fælles pauses 10 s ro. Gentag med ny HOLD/FLYT, formationsordre og AI OFF under pausen; den gamle ordre må ikke genopstå. Kontroller også stop-og-ild efterfulgt af automatisk SPRED, mindst 15 s mellem normale automatiske formationsskift, manuel SPRED/SAML, AFBRYD, BYG og rout. Akut carré må omgå cooldown; manuel carré beholder sin eksisterende marchregel. Dette er en testprocedure, ikke et bestået resultat.

Kortvalg og scenarie er afgørende: duel-, Skirmish- og kampagneslag har forskellige opsætninger. Et indlæst kampagneslag returnerer før den lille testopsætning. URL-valgene `?Battle=N` og `?Field=fil` anvendes også og markerer retur til kampagnen; de er ikke FParse-flag. Tider nedenfor er simulationstid, medmindre andet er angivet; pause/hastighed påvirker dem.

| Flag | Virkning | Eksempelargument |
|---|---|---|
| `Strategy1864DebugDecisions` | Logger officer-/reaktionsvurderinger, inputs, årsager og afviste alternativer med PROJECT1864-DECISION; også gentagne evalueringer. Kontaktlaget logger sidste position, heading, alder, observationstid, confidence, usikkerhedsradius og kilde ved hver kontaktscanning. Ingen taktisk effekt. | `-Strategy1864DebugDecisions` |
| `Strategy1864DebugOfficer` | Logger officerens ordre/fjende/flankerolle og spredt orden. | `-Strategy1864DebugOfficer` |
| `Strategy1864DebugDisengage` | Logger AFBRYD-start, faktiske 9 m-bagtrin, udmarch og afslutning med enheds-id, afstand og seneste salvetid. Starter ikke et slag. | `-Strategy1864DebugDisengage` |
| `Strategy1864HoldReserve` | Tilskynder bataljonschefen til reserve (kræver gruppestørrelse/aktiv AI). | `-Strategy1864HoldReserve` |
| `Strategy1864Grass` | 0 udelader græs ved kampagneslagmarkens opbygning. | `-Strategy1864Grass=0` |
| `Strategy1864FieldLOD` | Mænd pr. figur; mindst 1; tilsidesætter gemt figurskala ved start. | `-Strategy1864FieldLOD=5` |
| `Strategy1864NoHud` | Springer HUD-tegning over. | `-Strategy1864NoHud` |
| `Strategy1864ShowEnemyRange` | Viser fjendens ildrækkevidde. | `-Strategy1864ShowEnemyRange` |
| `Strategy1864TestCourier` | Efter 25 simulationssekunder får dansk kompagni længst fra staben en 200 m flytteordre. | `-Strategy1864TestCourier` |
| `Strategy1864NoCouriers` | Deaktiverer ordonnanslevering. | `-Strategy1864NoCouriers` |
| `Strategy1864StartHour` | Starttid på dagen i timer. | `-Strategy1864StartHour=8` |
| `Strategy1864NoFpsCap` | Undlader standardloftet på 60 FPS. | `-Strategy1864NoFpsCap` |
| `Strategy1864ClockRate` | Slagsekunder pr. simulationssekund. | `-Strategy1864ClockRate=6` |
| `Strategy1864FlatQA` | Beholder den flade QA-boks frem for engen. | `-Strategy1864FlatQA` |
| `Strategy1864Field` | Indlæser slagmarksfil fra Saved/Battle. | `-Strategy1864Field=Battlefield_Test.json` |
| `Strategy1864Duel` | Livgarden mod svensk infanteri; Duel-kort aktiverer også dette. | `-Strategy1864Duel` |
| `Strategy1864FullOOB` | Vælger fuld kamporden frem for duel; OOB-kort aktiverer også dette. | `-Strategy1864FullOOB` |
| `Strategy1864Battle` | Kampagneslag-id; uden Field vælges Battlefield_Battle_N.json. | `-Strategy1864Battle=1` |
| `Strategy1864Skirmish` | Antal fjendtlige kompagnier (1–4); Skirmish-kort har standard 2. | `-Strategy1864Skirmish=1` |
| `Strategy1864SkirmishCavalry` | Tilføjer fjendtlig husareskadron ca. 450 m fra danskerne, med fremrykningsordre; gør dansk start passiv. | `-Strategy1864SkirmishCavalry` |
| `Strategy1864SkirmishPassive` | Slår HQ-AI fra, defensiv doktrin og ingen indledende dansk angrebsordre; reaktive kompagniofficerer bevares. | `-Strategy1864SkirmishPassive` |
| `Strategy1864SkirmishDanes` | Antal danske kompagnier i lille test. | `-Strategy1864SkirmishDanes=4` |
| `Strategy1864SkirmishSwedes` | Bruger svenske infanterifigurer hos fjenden. | `-Strategy1864SkirmishSwedes` |
| `Strategy1864SkirmishArms` | Tilføjer batteri, morter og eskadron på begge sider. | `-Strategy1864SkirmishArms` |
| `Strategy1864EnemyDefends` | Fjenden forsvarer frem for indledende angreb. | `-Strategy1864EnemyDefends` |
| `Strategy1864SkirmishAttack` | HQ får angrebsordre med officerautoritet; ignoreres ved passiv start. | `-Strategy1864SkirmishAttack` |
| `Strategy1864DebugClock` | Logger realtid, simulationstid og slagur. | `-Strategy1864DebugClock` |
| `Strategy1864TestFlank` | Skifter danske kompagnier til offensiv doktrin efter 10 simulationssekunder; doktrin alene giver ikke længere angrebsordre. | `-Strategy1864TestFlank` |
| `Strategy1864TestFormation` | Kolonne 25 s, linje 55 s, knæ 85 s, liggende 100 s (simulationstid). | `-Strategy1864TestFormation` |
| `Strategy1864TestOfficers` | Sårer dansk officer/fanger fjendtlig ved 20 s; afslutter/skriver resultat ved 30 s. | `-Strategy1864TestOfficers` |
| `Strategy1864Pontoon` | Aktiverer pionerbroer i kampagneslagets opsætning. | `-Strategy1864Pontoon` |
| `Strategy1864AutoFinish` | Afslutter kampagneslag efter simulationssekunder og skriver resultat. | `-Strategy1864AutoFinish=60` |
| `Strategy1864Shots` | Billedplan: sek:del-af-enheds-id:kamerafstand-cm:sidefaktor; komma mellem trin; RIDER vælger ordonnans. | `-Strategy1864Shots=30:RIDER:4000:0.5` |
| `Strategy1864ShotsQuit` | Afslutter processen efter sidste billede; ventetid 2 simulationssekunder. | `-Strategy1864ShotsQuit` |
| `Strategy1864DuelCamera` | Duelkameraets afstand i cm. | `-Strategy1864DuelCamera=2500` |
| `Strategy1864DuelFocus` | Følger duelkompagni 0/1; uden gyldigt indeks bruges midten. | `-Strategy1864DuelFocus=0` |
| `Strategy1864DuelPitch` | Duelkameraets hældning i grader, begrænset til −85..−5. | `-Strategy1864DuelPitch=-35` |
| `Strategy1864DuelYaw` | Duelkameraets drejning i grader. | `-Strategy1864DuelYaw=0` |
| `Strategy1864DuelOffset` | Svensk kompagnis sideforskydning i cm. | `-Strategy1864DuelOffset=5000` |
| `Strategy1864DuelDanish` | Dansk duelmodel: Infantry eller Jager. | `-Strategy1864DuelDanish=Jager` |
| `Strategy1864Sun` | Solhøjde i grader ved atmosfærens initialisering; slagurets lysopdatering kan senere ændre den. | `-Strategy1864Sun=25` |
| `Strategy1864SunYaw` | Solens azimut i grader. | `-Strategy1864SunYaw=-45` |
| `Strategy1864Haze` | Tågetæthed ved initialisering. | `-Strategy1864Haze=0.01` |
| `Strategy1864Exposure` | Eksponeringskorrektion. | `-Strategy1864Exposure=-0.4` |
| `Strategy1864Horsemen` | 0 deaktiverer eskadronens rytterfigurer. | `-Strategy1864Horsemen=0` |
| `Strategy1864HorseYaw` | Hestemodellens lokale drejning i grader. | `-Strategy1864HorseYaw=-90` |
| `Strategy1864Saddle` | Sadelhøjde i cm. | `-Strategy1864Saddle=160` |
| `Strategy1864Seat` | Rytterens sædeparameter i cm. | `-Strategy1864Seat=48` |
| `Strategy1864AnimationAudit` | Logger animationskontrol for infanteri. | `-Strategy1864AnimationAudit` |
| `Strategy1864CrowdFar` | Afstand i cm til fjernfigurer/VAT. | `-Strategy1864CrowdFar=30000` |
| `Strategy1864Crowd` | 0 deaktiverer infanteriets crowd-visning. | `-Strategy1864Crowd=0` |
| `Strategy1864DebugForest` | Logger skovdybde, afstand, skjul og synlighedsafgørelse. | `-Strategy1864DebugForest` |

`Strategy1864Shots` tager billede 0,8 simulationssekunder efter kameraskift og gemmer `Saved/Screenshots/battle_shot_XX.png` (uden UI i screenshot-request). Ukendt mål springes over. Kampagnebilleder gemmes samme sted som `ui_shot_XX.png`, `timed_shot_N.png` og `menu_shot.png`. Quit-flag afslutter den proces, testen kører i.

## Launchere i projektroden

Launcherens indhold er autoritativt; flere ældre `rem`-kommentarer beskriver automatisk angreb, 1851-start eller fortsættelse fra autogem, som den aktuelle kode ikke længere garanterer.

| Fil | Faktisk opsætning |
|---|---|
| `Start-Kampagne.bat` | Campaign1851-kort; aktuelt startmenu. |
| `Start-Kampagne-NytSpil.bat` | Samme kort plus uvirksomt `CampaignNew`; vælg nyt spil i menuen. |
| `Start-Kampagne-Test-Felthaer.bat` | CampaignNew, TestFieldArmy, OpenOOB, OpenWindow=chart; menuens tidlige retur kan hindre testinitialisering. |
| `Start-Slagmark-Generer.bat` | CampaignNew, Seed=42, UiShots-plan med genfield ved 40 s og clear ved 50 s, UiShotsQuit. Planen kræver adgang til kampagnens UI-tick efter startmenuen. |
| `Start-3D-Slag-Test.bat` | Field-kort med Field=Battlefield_Test.json; kræver filen i Saved/Battle. |
| `Start-3D-Skirmish-Test.bat` | Skirmish-kort, Arms, Attack, ShowEnemyRange; standard 2 mod 2. |
| `Start-Livgarden-Svensk-Test.bat` | QA-kort, Duel, kamera 2500 cm, fokus 0, pitch −35°. |
| `Start-Test-1-Kompagni-mod-Kompagni.bat` | Skirmish=1, Danes=1, Swedes, EnemyDefends, Passive. |
| `Start-Test-2-Bataillon-mod-Kompagni.bat` | Skirmish=1, Danes=4, HoldReserve, Swedes, EnemyDefends, Passive. Bemærk stavningen Bataillon i filnavnet. |
| `Start-Test-3-Rytteri.bat` | Skirmish=1, Danes=2, Cavalry, Swedes, EnemyDefends, Passive. |

Eksempel i PowerShell: `& .\Start-Test-3-Rytteri.bat -Strategy1864DebugOfficer "-Strategy1864Shots=30:RIDER:4000:0.5"`. Dette er et eksempel; ikke kørt ved dokumentopdateringen.

## Midlertidige testknapper i startmenuen

- **TEST 1 MOD 1:** samme passive opsætning som Test-1.
- **TEST 4 MOD 1:** samme passive opsætning som Test-2, inklusive HoldReserve. Reserve-/flankeangreb kræver aktiv ledelse og ordre; passiv start giver ingen dansk angrebsordre.
- **TEST RYTTERI:** samme opsætning som Test-3; husarerne får en fremrykningsordre, mens danske kompagnier afventer spilleren. Charge er observeret i log, men rytteri mod karré er ikke testet til ende.

Knapperne tilføjer flag til den eksisterende kommandolinje og åbner Strategy1864_Skirmish. Baggrunden er det levende Danmarkskort. Knapperne skal fjernes efter testperioden; større regiment-/brigade-/divisionstests er stadig et senere punkt.

## Slaglyd

- `-Strategy1864NoSound`: deaktiverer slaglyd og lydindlæsning.
- `-Strategy1864DebugAudio`: `PROJECT1864-AUDIO`-log med clip, afstand og volumen.

Se [Audio.md](Audio.md) for WAV-generering, editorimport, HUD-indstillinger og manuel kontrol. Flagene giver ikke tilladelse til at starte spil eller editor.

## Rapport efter slaget (2026-10-10)

- `-Strategy1864DebugReport`: logger hele den færdige rapport med PROJECT1864-REPORT.
- `-Strategy1864TestReport`: kort skirmish med syntetiske tab, såret officer og fanger efter 15 simulerede sekunder; vælger selv lille skirmish, hvis en kampagneanmodning ikke har forrang.
- Se `Docs/AfterAction.md` for forventninger og kampagnekontrol. Flagene giver ikke tilladelse til at starte spil/editor.
