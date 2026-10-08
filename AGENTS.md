# AGENTS.md — PROJECT 1864 (strategispil)

Denne fil giver AI-coding-agenter (Claude Code, Codex, m.fl.) delt kontekst om projektet. Læs den før du udfører opgaver i dette repo.

## Projektoversigt

Et 3D real-time strategispil sat i Europa i 1851, med fokus på **krig og diplomati mellem europæiske stormagter**. Genre: RTS (ikke 4X, ikke rent turn-based grand strategy).

- **Engine:** Unreal Engine 5.8 (projekt `Game1864.uproject`)
- **Sprog:** C++ til kernesystemer og data-klasser, Blueprints til gameplay-logik, UI-reaktioner og hurtig iteration
- **Tidsperiode:** Spillet starter i **1825** — en relativt fredelig periode, bevidst valgt som rolig åbning. Spilleren oplever derfra optrapningen frem mod Julirevolutionen (1830), stænderforsamlingerne i hertugdømmerne (1831-34), Christian VIIIs tronbestigelse (1839) og Slesvig-Holsten-spørgsmålets eskalering mod Første Slesvigske Krig (1848–51). "1851" i filnavne og klasser (`Campaign1851*`) refererer til det oprindelige fokusår, ikke længere til spillets startår. Scenarie **1825** er standard, **1851** kan vælges i spilmenuen (`Campaign1851Scenarios.cpp`, `ACampaign1851Map::ActiveScenario()`); al ny data skal kunne vælges pr. scenarie, og 1851 må ikke ændres utilsigtet.
- **Spilbare nationer:** Flere end Danmark — Danmark er første prioritet/MVP, men andre europæiske magter skal også kunne spilles

## Historisk tilgang: udgangspunkt, ikke manuskript

Den virkelige historie (Slesvig-Holsten-spørgsmålet, 1848-krigen, osv.) er **udgangspunkt og inspiration, ikke et fast forløb spilleren skal genspille**. Spillet skal have et event-system (scriptede og/eller tilfældige begivenheder) der kan sende forløbet i andre retninger, så ikke to spil forløber ens. Implementér historiske hændelser som mulige/sandsynlige udfald, ikke hardcodede, uundgåelige checkpoints.

Spillerens valg skal kunne have reel kausal effekt på verdenshistorien — ikke bare ændre *hvornår* noget sker, men kunne forhindre en historisk begivenhed i at ske overhovedet (fx: god diplomatisk håndtering af Slesvig-Holsten-spørgsmålet kan betyde, at Første Slesvigske Krig aldrig bryder ud). Events bør derfor have forudsætninger/triggers, der reelt kan blive falske som følge af spillerens handlinger.

Spillerens valg skal reelt kunne ændre historiens gang — ikke bare give kosmetisk variation. Hvis spilleren fx forhindrer en krig, griber tidligt ind diplomatisk, eller allierer sig anderledes end historisk, skal det kunne aflede senere events og udfald fra deres historiske forløb, med realistiske nye konsekvenser frem for at forløbet "retter sig selv" tilbage mod historien.

## Nuværende scope (MVP)

**Første spilbare nation: Danmark.** Resten af Europa bygges på senere — datastrukturer skal designes så de kan udvides uden omskrivning.

Kerneregioner i MVP: Sjælland, Fyn, Nørrejylland, Hertugdømmet Slesvig, Holsten, Lauenburg. Slesvig/Holsten-spørgsmålet er en central spændingskilde i spillets diplomati — behandl disse som separate regioner med egen befolkningssammensætning (dansk/tysk), ikke som en del af "Danmark" uden videre.

## To-lags spilstruktur

1. **Campaign map** (bygges først) — provins-baseret kort i stil med Hearts of Iron 4 / Grand Tactician: Civil War. Hver provins har ejer, befolkning, ressourcer, infrastruktur (jernbane er ny teknologi i 1851 og findes ikke i 1825 — kræver forskning), og evt. garnison. Diplomati, rekruttering, forsyningslinjer og teknologi foregår her i real-time med pause/speed-kontrol.
2. **3D taktiske slag** — når hære mødes på campaign-kortet, skiftes til et separat 3D taktisk kort (terræn genereret eller håndbygget ud fra provinsens geografi). Enheder er regimenter/brigader med formationer, moral og ammunition — reflekterer tidlig musket-/riffel-taktik fra perioden, i stil med Grand Tactician.

Resultat fra slag (tab, moral, territorial gevinst) føres tilbage til campaign-laget.

## Visuel stil

By-ikoner på campaign-kortet følger "Isometric Miniature Town"-konceptet: små isometriske bygningsklynger der skalerer i antal/størrelse efter bytype:
- **C — Minor Town:** 2–3 bygninger
- **B — Regional Town:** 5–6 bygninger + kirke
- **A — Development City:** 10–15 bygninger, tydeligt urbant centrum

## Koden i dag

- **Kampagne:** modul `Source/Game1864` (`Campaign1851*.cpp`, overlay `SCampaign1851Overlay`, controller `Campaign1851PlayerController`). Kortdata bygges af `Tools/Map1851/build_map.py`.
- **3D-slag:** modul `Source/Strategy1864` (enheder, `AI/` med feltofficerer og autonom AI, `Visual/`, `Player/` med HUD og controller, `Tests/` med testslag). Slaget bor i dette repo; `Tools/Battle/Sync-Battle.ps1` må **ikke** køres.
- **Historik:** `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md` får et dateret afsnit (før `### Næste skridt`) ved hver ændring. `Docs/Backlog.md` er listen over det, der mangler.

## Regler for agenter

- Svar og skriv brugerrettet tekst på **dansk**.
- **Unreal 5.8 API:** kontrollér altid nye/ændrede API-kald og includes mod headerne i `I:/Spil/Epic Games/UE_5.8/Engine/Source` som statisk kompileringskontrol. Antag ikke API fra andre versioner: `FBox2D.bIsValid`, JSON-nøgler `UE::FSharedString`; `Misc/LexFromString.h` findes ikke. Dette giver ikke tilladelse til at bygge.
- **Klik-kæden:** indsæt ikke en selvstændig `if` midt i en eksisterende `else if`-kæde; slutgrenen kan ellers rydde valget. Eksisterende kædegrene bevares som `else if`; nye selvstændige handlere placeres uden for kæden og konsumerer klikket med `Button = EButton::Block`.
- **Spiltests:** kør ikke launchere, automatiske skærmbilledtests eller andre tests, der starter spil/editor, uden udtrykkelig anmodning. Statisk kontrol er tilladt.
- **Commit:** brugerens anmodning om ingen commit går forud. Ellers gælder Co-Authored-By-konventionen nedenfor med agentens navn og noreply-adresse. Ved sandboxbegrænsning efterlades opgavens ændringer uden commit og rapporteres i svaret.
- **Unity builds:** filer samles i store oversættelsesenheder, så lokale navne må ikke støde sammen (et lokalt navn kan skygge for et klassemedlem). Brug præfikser og undgå generiske navne som `Owner`, `Horses`, `Buttons`.
- Nye klik-handlere i `Campaign1851PlayerController.cpp` skal være selvstændige `if`-blokke, der sætter `Button = EButton::Block`, ellers falder klikket igennem til kortet.
- Gem og indlæs: nye felter skal gemmes i `UCampaign1851SaveGame` (hæv versionen, hvis formatet ændres).
- **Byg ikke og start ikke spillet**, medmindre du er bedt om det. Brugerens eget spil kan køre og holde `UnrealEditor-Game1864.dll` låst. Luk aldrig brugerens spil eller editor.
- Commit med linjen `Co-Authored-By: <agent> <noreply@...>`. Hvis sandboxen ikke kan committe, så lad ændringerne ligge rene i arbejdstræet og skriv det i svaret.
- Statskassens testbeløb (5.000.000 rd.) skal ned på 150.000 før rigtigt spil.
- Test: kampagnen kan tage skærmbilleder af vinduer med `-CampaignUiShots=sek:kommando;kommando,...`; slag testes med `Start-Test-1-Kompagni-mod-Kompagni.bat`, `Start-Test-2-Bataillon-mod-Kompagni.bat` og `Start-Test-3-Rytteri.bat` samt flagene `-Strategy1864Shots`, `-Strategy1864HoldReserve`, `-Strategy1864SkirmishDanes=N`. Se `Docs/TestFlags.md` for fuld flagliste og startbegrænsninger.
- Øvrige launchere: `Start-3D-Skirmish-Test.bat`, `Start-Livgarden-Svensk-Test.bat`, `Start-Slagmark-Generer.bat`, `Start-3D-Slag-Test.bat`, `Start-Kampagne.bat`, `Start-Kampagne-NytSpil.bat`, `Start-Kampagne-Test-Felthaer.bat`. Ældre launcherkommentarer og `CampaignNew` svarer ikke længere til startmenuens kode; se TestFlags.md. Listen giver ikke tilladelse til at køre dem.

## Arbejdsdeling mellem agenter

- **Claude Code:** Planlægning, arkitektur, sammenhænge mellem systemer (Province/Nation/DiplomaticRelation), multi-fil refaktorering, integration.
- **Codex:** Afgrænsede, veldefinerede implementeringsopgaver ud fra spec, samt code review/validering af eksisterende kode.

## Fremtidige idéer / muligt scope (ikke besluttet endnu)

Disse er brainstorm-idéer, ikke bekræftet scope. Afklar med projektejer før implementering.

**Campaign-lag:**
- Jernbane-mekanik som kerne-progression (hurtigere troppeforflytning/handel) — passer godt til periodens tidlige jernbane-boom
- Opinion/stabilitet-system efter 1848-revolutionerne (reform vs. undertrykkelse)
- "Delt loyalitet"-mekanik for Slesvig/Holsten: reaktion afhænger af både dansk regering og Forbundsdagen i Frankfurt
- Stormagtsgarantier (fx Storbritannien/Rusland garanterer en mindre nations grænser — inspireret af London-protokollen 1852)

**Taktisk kamp-lag:**
- Våbenteknologi-overgang: glatløbs-musket vs. tidlig riffel (Minié-kugle)
- Vejr/sæson-effekt på slag (fx frosne stræder som taktisk mulighed)
- Flådestøtte til landslag (kystbombardement, troppetransport) — særlig dansk styrke

**Efterretning/spionage (lav prioritet — vent til de andre systemer er på plads):**
- "Fog of war" på andre nationers militærstyrke/treasury/stabilitet som udgangspunkt, afsløret via spionage
- Spion-funktioner: militær rekognoscering, diplomatisk indsigt, sabotage, desinformation
- Kontraspionage: afslørede spioner kan selv udløse diplomatiske krise-events
- Bør koste en begrænset ressource, så det er et strategisk valg, ikke gratis/uendeligt
- Hænger sammen med event-systemet — en afsløret spion-operation kan selv være en trigger

**Meta/replay:**
- Alternativ historie-scoring: sammenlign spillerens udfald med det faktiske historiske forløb
- Potentiel spiller-til-spiller multiplayer-diplomati

## Konventioner

- Datastrukturer (Province, Nation, DiplomaticRelation) skal designes udvidelsesbart til hele Europa, selvom MVP kun dækker Danmark/Slesvig/Holsten
- Historiske fakta (befolkningstal, grænser, alliancer) skal så vidt muligt være baseret på faktisk 1850'er-data — flag det tydeligt i kommentarer, hvis et tal er et estimat frem for verificeret kilde
