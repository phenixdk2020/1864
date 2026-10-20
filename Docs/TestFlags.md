# Testflag og launchere

Nabokort: `-CampaignNeighbourShot=PR` starter et nyt pauset scenarie, fokuserer første naboby, åbner informationskort/kontrolvisning og gemmer `Saved/Screenshots/neighbour_PR.png`. Bynationer: SE, PR, MEC, HAN, OLD, HH, LUB, BRE. Kombinér med `-CampaignScenario=1825` eller `1851`. Log: `CAMPAIGN-1851|neighbours|`. Implementeret, men ikke kørt; flaget giver ikke tilladelse til spilstart. Se `Naboer.md`.

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

Opdatering 2026-10-10: `Strategy1864DebugDecisions` viser også den fælles missionspause (lag `ReactionAI`), carré-cooldownens klarstatus og afvist automatisk SPRED under cooldown. Acceptflag er tilføjet 2026-10-10; se nedenfor. Ingen acceptkørsel er udført.

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
| `Strategy1864FullLighting` | Beholder Lumen og dynamiske skygger i slaget (standard: slået fra, indtil animationerne er på plads; kampagnen får dem tilbage efter slaget). | `-Strategy1864FullLighting` |
| `Strategy1864DrillTest` | Scriptede ordrer til at se kolonneeksercitsen: `sek:enhed:frem:højre[:front]`, `sek:enhed:LINE`, `sek:enhed:KILL:antal` eller `sek:enhed:KILLCOLOURS`, kommasepareret. | `-Strategy1864DrillTest=8:DK-SKIRMISH-C1:4000:13000:0` |
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

## Automatisk enhedsadfærdsaccept (2026-10-10)

**Review 2026-10-11:** Den eksisterende runner har 29 ID'er og matrixen 190 rækker. Windows afviser redigering/erstatning af de tre nye filer fra den afbrudte kørsel (`StrategyBehaviourTests.cpp`, matrixen og launcheren). [Adfaerd-accept-rettelser.patch](../Adfaerd-accept-rettelser.patch) er derfor **forberedt, men ikke anvendt**. Patchen tilføjer ni afgrænsede fixtures: **SUSPEND-WAYPOINT, CLEAR-SUSPEND, RELOAD, SCREEN-SQUARE, SPREAD-MOVE, ARTILLERY-DEPLOY, DRAGOON, CAVALRY-STILL, ADVANCE-FIRE**; i alt 38 ID'er, heraf syv mangelrapporter, og 199 matrixrækker. De nye ID'er kan først bruges efter anvendelse af patch og brugerens eget build. Eksempel efter dette: `& .\Start-Test-Adfaerd.bat -Strategy1864Accept=RELOAD`. Log: `PROJECT1864-ACCEPT RELOAD PASS|FAIL`.

Patchen retter også 9–10 s-hullet i resume-check, en ugyldig broreference, SETUP ved fejlet spawn, rapportens summary efter manglende matrix og launcherens arbejdsmappe. Patchen retter også afsnitsmarkørerne for §1–2 og §10 samt dubleret SETUP-log. Audio-rækken bruger `PROJECT1864-AUDIO`; manglende kontrakter betegnes som mangelrapporter. Scenarioaktørens editor/PIE-vagt er allerede anvendt: acceptflag afvises dér med `PROJECT1864-ACCEPT START FAIL`, og processens exit kaldes ikke. Ingen build eller spil-/editorkørsel er udført.

`Source/Strategy1864/Tests/StrategyBehaviourTests.cpp` udvider den eksisterende scenarioaktør. Accept-mode tager forrang for duel/skirmish/kampagneopsætning og deres afslutnings-/skærmbilleddrivere. Brug Skirmish-kortet med den eksisterende scenarioaktør; ingen ny map-asset kræves. Flad QA anbefales for reproducerbare syns- og bevægelsesforhold.

| Flag | Virkning | Eksempelargument |
|---|---|---|
| `Strategy1864Accept` | Ét ID, uafhængigt af store/små bogstaver; ukendt ID giver FAIL. AcceptAll har forrang, hvis begge angives. | `-Strategy1864Accept=STOPFIRE` |
| `Strategy1864AcceptAll` | 29 ID'er sekventielt i samme verden; nye enheder pr. case og oprydning mellem dem. | `-Strategy1864AcceptAll` |
| `Strategy1864AcceptTimeout` | Timeout pr. case i simulerede verdenssekunder, begrænset 60–120; standard 120. Succes/kontraktfejl afslutter tidligere. | `-Strategy1864AcceptTimeout=120` |
| `Strategy1864AcceptSpeed` | Global time dilation, begrænset 0,5–10; standard 5. Det er ikke slagurets ClockRate. | `-Strategy1864AcceptSpeed=1` |

ID'er: **T1–T10**, **MOVE, STOPFIRE, NEW-HOLD, DELAY, SPREAD, SCREEN, WORKS, WORKS-CANCEL, CONTACT, ROE, AMMO, ROUT, DESTINATION, SQUARE-ZONES, LONG-MOVE, SQUARE-GATES, FIREBLOCK, DISENGAGE, WORKS-MOVE**. Assertionerne bruger rigtige komponenter. T1 kontrollerer carré-start inden X=5 s, HALT/bajonetter/snapshot, mindst20s release, reformering og resume; det er ikke en charge-overlevelsesprøve. T3 kontrollerer lokal moralsmitte/cooldown og forventes at afsløre den manglende smitteregel. T7 marcherer to kompagnier fra samme bred over QA-bro og observerer ejerskab, flodposition og ankomst.

**T2/T4/T5/T6/T8/T9/T10 giver `FAIL NOT_IMPLEMENTED`**, fordi fulde designkontrakter mangler. De er automatiske mangelrapporter, ikke implementerede fulde fixtures. DELAY beviser ikke T4, og DESTINATION/SQUARE-ZONES beviser ikke HUD eller salveandele. AFBRYD, charge, artilleri/morterbeskyttelse, UI, save/load, audio og mange designvariationer kræver stadig manuel test/videre fixtures. Se [Test-Skema-Enhedsadfaerd.md](Test-Skema-Enhedsadfaerd.md) for alle rækker og afgrænsninger.

Eksakt samlet kommando fra projektroden: `& .\Start-Test-Adfaerd.bat`. Enkeltcase: `& .\Start-Test-Adfaerd.bat -Strategy1864Accept=CONTACT -Strategy1864AcceptSpeed=1`. Direkte start:

```powershell
& "I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\Game1864.uproject" /Game/Maps/Strategy1864_Skirmish -game -windowed -nosplash -unattended -Strategy1864FlatQA -Strategy1864AcceptAll -Strategy1864DebugDecisions
```

Launcheren venter på processen og viderefører exitkode: **0** ved alle PASS, **1** ved assertion/timeout/manglende kontrakt/rapportfejl, **2** ved manglende executable/projekt. Accept-mode afslutter kun sin egen proces efter sidste case. Brug ikke acceptflag i brugerens eksisterende editorproces. Log: `PROJECT1864-ACCEPT <id> SETUP`, `PROJECT1864-ACCEPT <id> PASS|FAIL <årsag>`, resultatlinjer og `PROJECT1864-ACCEPT SUMMARY pass=N fail=M`. Rapport: `Saved/Reports/accept_<UTC-timestamp>.md` med resultater og hele matrixen; kun observerede PASS-rækker afkrydses. Fejl tælles, også ved rapportskrivning. Originalmatrixen røres ikke.

Seed1864 bruges til kamp-RNG, men eksisterende variable ticks/andre RNG-veje betyder, at fuld determinisme ikke er bevist. Testinput via offentlige ordre-/skade-/moralmetoder er ikke en simulation af spillerklik. Intet er bygget eller kørt ved denne opdatering.

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
| `Start-Test-Adfaerd.bat` | Skirmish-kort, FlatQA, AcceptAll (eller enkelt Accept), DebugDecisions, nosplash/unattended; venter og afslutter med testens exitkode. |
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

## Vogntog i 3D-slaget (2026-10-11)

`-Strategy1864Train` aktiverer teststandarder for vogne og to statiske trækheste pr. vogn i skirmish og øvrige testscenarier: bataljon 2 vogne/60 m, feltbatteri 1 pr. 2 kanoner/25 m, morterbatteri 1 pr. morter/25 m og brigade/divisionsstab 4 forsyningsvogne/250 m. Kampagnens eksplicitte `wagons`/`horses` fra Units.json har forrang, også nul. Log: `PROJECT1864-TRAIN`. Se [Wagons.md](Wagons.md) for erobring, ammunition, import og manuelle kontrolpunkter. Flaget er ikke kørt og giver ikke i sig selv tilladelse til spilstart.
