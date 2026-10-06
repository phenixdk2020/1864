# PROJECT 1864 — Designmanual

**Aktuel designbaseline: v00.02.22**  
**Aktuel prototype-workbranch: P0A v00.00.09f30f IMPLEMENTATION + FIX HISTORY CONSOLIDATION TEST**

Grand Strategy i realtid + taktiske 3D-slag. Denne GitHub-udgave er opdelt i dele for overskuelig versionsstyring. Den layoutede Word-master opdateres parallelt som projektartefakt, mens GitHub-Markdown er den løbende designmæssige source of truth.

Projektets centrale intake-log for besluttede men endnu ikke implementerede funktioner, planlagte opgaver, research-emner og løse idéer ligger i [PROJECT-BACKLOG.md](PROJECT-BACKLOG.md). Større emner kan have detaljerede backlog-supplementer, som senere konsolideres ind i hovedbackloggen.

Den aktuelle **gameplay-baseline** ligger på **v00.00.09f30e**. Den aktuelle dokumentations-/buildbaseline er **v00.00.09f30f**, som ikke ændrer tactical gameplay men konsoliderer hele implementation- og fix-historikken. Det centrale design for regimentschef, cavalry, charge shock og infantry square er samlet i [Designsupplement v00.02.09 — Regimentskommando, cavalry og square](design-supplements/v00.02.09-regimental-command-cavalry-square.md). Map/attack-AI-hardening er dokumenteret i [v00.00.09f29b — Battlefield + Attack AI Hardening](design-supplements/v00.00.09f29b-battlefield-attack-ai-hardening.md), den fælles Major/Company command UI i [v00.00.09f29c — Unified Command HUD](design-supplements/v00.00.09f29c-unified-command-hud.md), semantic zoom/HQ visibility/NATO-symboler i [v00.00.09f29d — Semantic Zoom + NATO Symbols](design-supplements/v00.00.09f29d-semantic-zoom-nato-symbols.md), navigation/sidestep/visual polish i [v00.00.09f29p — Navigation, Side Step & Visual Polish](design-supplements/v00.00.09f29p-navigation-sidestep-visual-polish.md), OOB/HQ-discoverability i [v00.00.09f29q — OOB, HQ Discoverability & Semantic Zoom](design-supplements/v00.00.09f29q-oob-hq-semantic-zoom.md), crop-field concealment/map polish i [v00.00.09f29r — Crop Field Concealment & Map Polish](design-supplements/v00.00.09f29r-crop-field-concealment.md), defensive/UI/terrain-hardening i [v00.00.09f29y — Defensive Stability, TEST AI, HUD Status & Terrain Polish](design-supplements/v00.00.09f29y-defensive-stability-ui-terrain-polish.md), Square face-fire i [v00.00.09f29z — Square Face Fire & Directional Smoke](design-supplements/v00.00.09f29z-square-face-fire-smoke.md), første mounted cavalry implementation i [v00.00.09f30 — First Cavalry Core](design-supplements/v00.00.09f30-cavalry-core.md), command/officer-hardening i [v00.00.09f30a — Command, Square and Officer AI Hardening](design-supplements/v00.00.09f30a-command-officer-hardening.md), og den højere kommandokæde i [v00.00.09f30b — Higher Command HQ](design-supplements/v00.00.09f30b-higher-command-hq.md). Cavalry Officer AI/dynamic attachment ligger i [F30C](design-supplements/v00.00.09f30c-cavalry-ai-dynamic-attachment.md), OOB scroll/drag-drop i [F30D](design-supplements/v00.00.09f30d-oob-scroll-dnd-cavalry-visuals.md), 1:1 cavalry/visual fidelity i [F30E](design-supplements/v00.00.09f30e-1to1-cavalry-visuals.md), og den samlede historik i [Implementation & Fix History](IMPLEMENTATION-AND-FIX-HISTORY.md) samt [F30F-supplementet](design-supplements/v00.00.09f30f-history-consolidation.md).

## Indhold

- [Del 1: 1–6 — Executive summary, slutvision, strategisk realtid, kort, nationer og OOB](parts/part-01-01-06.md)
- [Del 2: 7–12 — Enheder, officerer, ordrer, march, logistik og fog of war](parts/part-02-07-12.md)
- [Del 3A: 13–19 — Taktiske 3D-slag, kamp, våbenarter, casualties, retreat og flåde](parts/part-03a-13-19.md)
- [Del 3B: 20 — Befolkning, økonomi, byudvikling, industri, handel, forskning, rekruttering, træning, sanitet/fanger, regimentshistorik og perks](parts/part-03b-20-20.md)
- [Del 3C: 20.16 — Strategisk landudvikling: veje, jernbane, gårde, hesteopdræt, våbenindustri og regionale projekter](parts/part-03c-20-16-strategic-development.md)
- [Del 3D: Battle supply, skumring/nat, overnight resupply, kavaleri og dragoner](parts/part-03d-night-supply-cavalry.md)
- [Del 4: 21–27 — Strategisk/taktisk AI, terræn, performance, UI, save/modding og historisk datamodel](parts/part-04-21-27.md)
- [Del 5A: Feature-arkitektur F00–F15](parts/part-05a-F00-F15.md)
- [Del 5B: Feature-arkitektur F16–F31](parts/part-05b-F16-F31.md)
- [Del 5C: Feature-arkitektur F32–F47](parts/part-05c-F32-F47.md)
- [Del 6: 29–36 — Milepæle, immediate prototype sequence, vertical slice, risici, datarelationer, historisk grounding og designbeslutninger](parts/part-06-29-36.md)
- [Del 7: 37 — Implementeringsstatus P0A Unity 3D Battle Prototype](parts/part-07-37-P0A.md)
- [Del 8: 38 — P0A v00.00.08 reload, experience, salve-feedback og enkel casualty-visual](parts/part-08-38-P0A-v08.md)
- [Implementation & Fix History — samlet historik over implementeringer og fejlrettelser](IMPLEMENTATION-AND-FIX-HISTORY.md)
- [Designsupplement v00.00.09f30f — Implementation & Fix History Consolidation](design-supplements/v00.00.09f30f-history-consolidation.md)
- [Designsupplement v00.02.09 — Regimentskommando, cavalry, charge shock og infantry square](design-supplements/v00.02.09-regimental-command-cavalry-square.md)
- [Designsupplement v00.00.09f29b — Battlefield + Attack AI Hardening](design-supplements/v00.00.09f29b-battlefield-attack-ai-hardening.md)
- [Designsupplement v00.00.09f29c — Unified Command HUD](design-supplements/v00.00.09f29c-unified-command-hud.md)
- [Designsupplement v00.00.09f29d — Semantic Zoom + NATO Symbols](design-supplements/v00.00.09f29d-semantic-zoom-nato-symbols.md)
- [Designsupplement v00.00.09f29e — Regimental Defense, Objective Visuals & Coordinated Attack](design-supplements/v00.00.09f29e-regimental-defense-attack-coordination.md)
- [Designsupplement v00.00.09f29p — Navigation, Side Step & Visual Polish](design-supplements/v00.00.09f29p-navigation-sidestep-visual-polish.md)
- [Designsupplement v00.00.09f29q — OOB, HQ Discoverability & Semantic Zoom](design-supplements/v00.00.09f29q-oob-hq-semantic-zoom.md)
- [Designsupplement v00.00.09f29r — Crop Field Concealment & Map Polish](design-supplements/v00.00.09f29r-crop-field-concealment.md)
- [Designsupplement v00.00.09f29y — Defensive Stability, TEST AI, HUD Status & Terrain Polish](design-supplements/v00.00.09f29y-defensive-stability-ui-terrain-polish.md)
- [Designsupplement v00.00.09f29z — Square Face Fire & Directional Smoke](design-supplements/v00.00.09f29z-square-face-fire-smoke.md)
- [Designsupplement v00.00.09f30 — First Cavalry Core](design-supplements/v00.00.09f30-cavalry-core.md)
- [Designsupplement v00.00.09f30a — Command, Square and Officer AI Hardening](design-supplements/v00.00.09f30a-command-officer-hardening.md)
- [Designsupplement v00.00.09f30b — Higher Command HQ](design-supplements/v00.00.09f30b-higher-command-hq.md)
- [Designsupplement v00.00.09f30c — Cavalry Officer AI + Dynamic Attachment](design-supplements/v00.00.09f30c-cavalry-ai-dynamic-attachment.md)
- [Designsupplement v00.00.09f30d — OOB Scroll, Drag/Drop & Cavalry/HQ Visual Polish](design-supplements/v00.00.09f30d-oob-scroll-dnd-cavalry-visuals.md)
- [Designsupplement v00.00.09f30e — 1:1 Cavalry & Historical Visual Fidelity](design-supplements/v00.00.09f30e-1to1-cavalry-visuals.md)
- [Release Notes — P0A v00.00.08 TEST](releases/P0A-v00.00.08-RELEASE-NOTES.md)
- [Release Notes — P0A v00.00.09 TACTICAL COMMAND TEST](releases/P0A-v00.00.09-RELEASE-NOTES.md)
- [Projekt-backlog — beslutninger, planlagte funktioner, research og idéer](PROJECT-BACKLOG.md)
- [Backlog B-160–B-169 — strategisk landudvikling](backlog/B-160-STRATEGIC-DEVELOPMENT.md)
- [Backlog B-170–B-179 — Officer AI, delegeret kommando og AI Unit ON/OFF](backlog/B-170-OFFICER-AI-DELEGATION.md)
- [Backlog B-180–B-189 — symmetrisk fjende-AI, sværhedsgrad og v00.00.09-prioritet](backlog/B-180-AI-DIFFICULTY-AND-V009.md)
- [Backlog B-190–B-199 — officerstats, Composure/Nerve og AI decision model](backlog/B-190-OFFICER-STATS-MODEL.md)
- [Backlog B-200–B-209 — Close/Medium/Long range bands, HQ hierarchy, semantic zoom og couriers](backlog/B-200-COMMAND-VISUALS-RANGE-HQ-COURIERS.md)
- [Backlog B-210–B-219 — fire eligibility, skudkegle og højere formation templates](backlog/B-210-FIRE-ELIGIBILITY-AND-HIGHER-FORMATIONS.md)
- [Backlog B-220–B-229 — Pause/x0,5/x1/x2/x5/x20, simulationstid og klokke](backlog/B-220-SIMULATION-TIME-CONTROLS.md)
- [Backlog B-230–B-239 — battle supply, skumring/nat og overnight resupply](backlog/B-230-BATTLE-SUPPLY-NIGHT-OPERATIONS.md)
- [Backlog B-240–B-249 — udvidet kavaleri-, Gardehusar- og dragonmodel](backlog/B-240-CAVALRY-DRAGOONS-EXPANDED.md)
- [Backlog B-250–B-259 — fog of war, scouts og HQ command effectiveness](backlog/B-250-FOG-SCOUTS-COMMAND-EFFECTIVENESS.md)
- [Backlog B-280–B-291 — Charge, melee, Regiments-HQ, Dragoons og artilleri](backlog/B-280-CHARGE-REGIMENT-HQ-HIERARCHY.md)

## v00.00.09f28 Regiment Control Test — implementeringsstatus

F28 er første samlede regiment-control MVP. Den danske prototypekæde var først designmæssigt og runtime-mæssigt:

`Oberstløjtnant → Major A / Major B → 4 kompagnier pr. Major → 8 kompagnier i regimentet`

**Kaptajn** er company-chef, **Major** bataljonschef og **Oberstløjtnant** regimentschef i den aktuelle test-OOB. Fra F30B er **Brigadechef** og **Divisionschef** selvstændige command roles over regimentet; den konkrete historiske officersgrad/navn er scenario/OOB-data og må ikke hardcodes som identisk med command role.

Oberstløjtnanten giver missionsordrer til Majorerne — ikke direkte til kompagnierne. Regiments-AI kan i første MVP disponere de to bataljoner side om side, med én bataljon i reserve eller med én bataljon på flankemission. Majorerne omsætter regimentsintentionen til company-slots og lokal udførelse.

Authority er hierarkisk. Direkte manuel company-ordre giver spiller-authority. AI ON bagefter betyder lokal Kaptajn-AI; gammel Major-ordre må ikke genopstå. En ny Major-ordre kan reclaim'e kompagniet. Manuel flytning af Major giver spiller-authority over HQ, mens en ny Oberstløjtnant-ordre kan reclaim'e Majoren og give en frisk bataljonsmission.

HQ'er flytter frem i bounds under angreb i stedet for at stå permanent på startpositionen. Command reach påvirker primært order/reaction delay, coordination, reserve/flank reassignment, recovery og senere information quality — ikke direkte musket accuracy eller damage.

## v00.00.09f29b Battlefield og Attack AI — hardening

Det taktiske battlemap er nu **5760 × 3840 m**, dvs. dobbelt bredde og dobbelt dybde i forhold til den foregående 2880 × 1920 m testflade. Road, river og vegetation fortsætter over den større flade, mens den centrale bridge/farm testzone bevares til regressionstest.

Det aktive QA-scenario beholder nu to fjendtlige Prussian company-scale formationer, `8th Regiment` og `18th Regiment`, så target-selection og fler-target angreb kan testes i samme slag. Begge behandles som company-scale testformationer.

Attack-authority følger den eksplicitte regel **én fysisk movement owner ad gangen**. Når en Major placerer et company og derfor har disabled den lokale `OfficerAIController`, må frontage-, approach- eller andre hjælpe-AI-systemer ikke skrive ny route eller formation. Under-fire reaction har tilsvarende midlertidig authority og må ikke bekæmpes af andre movement layers.

Et eksplicit `AttackTarget` er sticky og må ikke overskrives af "nearest enemy". `AttackNearest` kan revurdere under approach, men låses til et konkret target ved contact, så formationen ikke oscillerer mellem to næsten lige nære fjender.

F29a-visualiseringen bevares: selected company viser connection til egen Major, selected Major viser connection til Oberstløjtnanten, command-lines følger terrænet, og officer target-area cirklen vises under point-orders. Formation endpoint safety kontrollerer hele company-footprint mod åen, så en formation ikke kan godkendes med dele af linjen i åbent vand.

## v00.00.09f29c Unified Command HUD — UI-baseline

Major/Bataljon og Kaptajn/Kompagni bruger samme compact bottom-HUD visual language. HUD'en opdeles efter command level i **ENHEDSINFO**, **AI/DOKTRIN**, relevante ordregrupper og ved Major-selection en separat oversigt over subordinate companies.

State-farver er standardiseret: **grøn = aktiv/valgt**, **rød = inaktiv/ikke valgt**. Det bruges på AI ON/OFF, DEF/BAL/OFF doctrine, company fire policy, formationer og relevante movement states.

Major HUD viser aggregate mænd, tab, morale, cohesion og ammunition samt fire subordinate company rows med mænd/tab/morale/ammo. Fra F29Y viser hver row desuden samme taktiske status som OOB, så Major-HUD og OOB ikke kan fortælle to forskellige historier om samme kompagni. Bataljonsordrerne er samlet som `ANGRIB HER / FORSVAR HER / RYK FREM / TILBAGETRÆK / SAML / STOP-HOLD`.

Company HUD viser de samme statusprincipper og grupperer `HOLD / CLOSE / MEDIUM / LONG` under skydning, `→MED / →LONG / →UD / TVANG / CHARGE / STOP` under movement/orders og `LINJE / KOLONNE / SQUARE` under formation. **SQUARE er synlig i HUD'en og bruger direkte F29 square-state**, ikke en parallel implementering.

`STOP` er defineret som en direkte company-order: local AI OFF, manual route ryddes, withdrawal/charge afsluttes, forced march slås fra og company holder position. Formation-knapper er tilsvarende direct company authority, så AI ikke straks kan overskrive spillerens formation.

De otte danske company-scale enheder skal synligt hedde **1. KOMPAGNI–8. KOMPAGNI**. De ældre strings `1. Regiment`, `5. Regiment`, `2. Regiment` og `3. Regiment` bevares midlertidigt som interne compatibility-id'er, fordi ældre prototype-lag fortsat bruger dem til lookup; display/GameObject naming korrigeres i F29c. Den langsigtede datamodel skal adskille permanent unit-id, display name, parent regiment og battalion/company index.

## v00.00.09f29d Semantic Zoom og NATO-symboler — designforslag + første implementation

Taktisk visualisering skifter informationsform med zoomniveauet i stedet for kun at gøre de samme 3D-modeller mindre. Målet er at bevare fysisk nærkamp i close view og samtidig gøre command structure og formationer læsbare i operational/strategic view.

Semantic zoom bruger fire niveauer: **CLOSE → MEDIUM → OPERATIONAL → STRATEGIC**. HQ'er har højere informationsprioritet end companies og får derfor screen-space markører allerede ved moderat zoom. Major/Battalion vises som `II + HQ`, Oberstløjtnant/Regiment som `III + HQ`, mens company-scale infantry vises som `I` med NATO-style infantry `X`.

I **OPERATIONAL** beholdes 3D-modellerne, mens alle companies får 2D tactical counters med affiliation, strength, morale, facing og for danske enheder ammunition. Friendly er blå, enemy rød, selected gul/guld og routed grå. HQ-counters har fast minimumsstørrelse i screen-space og en beacon/stem ned til den faktiske world-position, så officererne kan lokaliseres selv over terræn og formationer.

I **STRATEGIC** skjules MeshRenderers og ParticleSystemRenderers under companies og HQ'er. Terrain, simulation, colliders, AI, LineRenderers, command links og ordre-state fortsætter uændret. Dermed bliver zoomet reelt en ren taktisk 2D-symbolvisning oven på det eksisterende 3D-terrain, og command-chain lines kan stadig læses.

F29Q flytter skiftene tidligere, fordi NATO-symbolerne skal være et aktivt navigationsværktøj og ikke først dukke op ved ekstrem højde. QA-thresholds er nu: HQ marker ca. **55 m**, Medium ca. **95 m**, Operational ca. **175 m** og Strategic ca. **315 m**. Den langsigtede model bør stadig kunne bruge projected screen footprint i pixels, fordi det er mere robust over for FOV, opløsning og kameraændringer. Smooth 3D↔2D crossfade, overlap avoidance og very-far battalion aggregation er fortsat relevante; Brigade/Division-echelonerne `X`/`XX` er fra F30B en aktiv del af OOB/HQ-strukturen.

## v00.00.09f29e Regimental defense, objective visuals og coordinated attack

Oberstløjtnanten bruger nu samme kompakte 90 px HUD-sprog som Major og Company: **ENHEDSINFO / AI**, **REGIMENTSORDRER** og **BATALJONER UNDER OBERSTLØJTNANT**. AI ON/OFF og DEF/BAL/OFF følger samme grøn-aktiv / rød-inaktiv regel. HUD'en viser total styrke/tab/morale/cohesion/ammunition samt status for begge Majorer/bataljoner.

En committed Regimental point-order forsvinder ikke længere efter klikket. Mens Oberstløjtnanten er valgt, vises en terrain-following objective circle med centre cross og ordrelabel ved det faktiske `ANGRIB HER`, `FORSVAR HER`, `RYK FREM`, `TILBAGETRÆK` eller `SAML` objective. `FORSVAR HER` har den største første QA-radius, fordi markøren repræsenterer en defensive area og ikke kun et punkt.

Valg af Oberstløjtnanten viser hele command chain: `Oberstløjtnant → begge Majorer → alle otte kompagnier`. Samtidig vises alle aktuelle company destination footprints samt aktive routes, så spilleren kan se hvordan regimentsordren er dekomponeret til bataljons- og company-slots uden at klikke hver Major separat.

Den gamle F27 arrival tolerance på ca. 4.5 m kunne få en route til at forsvinde, mens company-centret stadig stod uden for det viste destination rectangle. F29E har en snæver final-arrival correction, der kun arbejder i den gamle tolerance-zone, respekterer F26 under-fire authority og først accepterer slotten omkring 0.85 m fra centre. Der teleporteres ikke.

`FORSVAR HER` betyder nu, at det klikkede point faktisk skal dækkes. F28's generiske to-battalion split på omtrent ±170 m kunne efterlade en stor corridor gennem centrum. F29E bruger som første QA omtrent **±84 m** battalion half-separation omkring det defensive objective. Med lokal reserve og tre front companies pr. battalion mødes de indre frontage-slots omkring regimentscentrum; terrain/river legality kan stadig flytte individuelle slots.

Ved `ANGRIB HER` er tactical role assignment runtime-korrigeret: ved FRONT+RESERVE eller FRONT+FLANK er den battalion, der allerede er nærmest objective, FRONT. Den anden får reserve/flank-rollen. Rollen afgøres ikke længere af laveste samlede marchafstand mellem Major-HQ og slots.

Når flere companies har samme eksplicitte `AttackTarget`, får de separate formation-safe assault slots omkring target i stedet for alle at lukke direkte mod target-centret. 2–4 companies fordeles på en frontal arc med bevaret left/right ordering; yderligere companies lægges som support. `OfficerAIController` er stadig den fysiske movement owner. F29E må ikke skrive gennem en disabled controller og suspenderes under F26 under-fire reaction.

Map enlargement fra F29B kræver også navigation, der faktisk arbejder på **5760 × 3840 m**. Det gamle V4-lag var stadig bygget omkring den tidligere ±176 × ±116 m QA-grid og kunne derfor behandle gyldige enheder på det større kort som værende uden for navigation-bounds. F29E deaktiverer de obsolete small-grid navigation writers på det store map og bruger dynamic `PrototypeBootstrap` battlefield bounds med persistent goals og let obstacle-detour; det eksisterende bridge-only phase system forbliver autoritet for river crossing.

## v00.00.09f29q OOB navigation, HQ discoverability og semantic zoom

Spilleren skal kunne finde enhver egen enhed eller HQ uden først at lokalisere den visuelt på 3D-slagmarken. F29Q introducerer derfor et sammenklappeligt **ORDER OF BATTLE**-panel som navigationsindeks for den eksisterende fysiske kommandokæde. Panelet er et UI-/navigationlag og må ikke omgå command authority eller ordre-delay; selection og kamera-navigation ændrer ikke enhedens mission.

OOB-hierarkiet følger den faktiske danske command chain og bruger samme NATO-echelon-sprog som semantic zoom: `III` = regiment/Oberstløjtnant, `II` = bataljon/Major og `I` = kompagni. Major A og Major B kan foldes ud/ind, og deres fire kompagnier vises direkte under dem. Company-rows viser kompakt styrke og en taktisk state-markør for fx bevægelse, kamp, under ild, Square, Charge/melee eller Rout.

**Ét klik på en OOB-row** ændrer command selection uden at flytte kameraet. **Dobbeltklik** er den bevidste navigation og placerer kameraet taktisk bag den valgte company/HQ i dens facing-retning. Den valgte OOB-row fremhæves gul/guld, så det er tydeligt hvilken entity der er aktiv, mens spilleren kan blive med kameraet på et andet kritisk område af slaget.

HQ-discoverability skal være stærkere end almindelig company-readability. Major- og Oberstløjtnant-counters har derfor fast screen-space minimumsstørrelse, tydelige `II HQ`/`III HQ`-symboler og beacon/stem til world-position. Når kameraet zoomes ud skifter presentationen tidligere til NATO-symboler; fysisk 3D er stadig primær tæt på, mens operational/strategic view prioriterer command structure og counters.

Den fælles navigation består nu af tre komplementære lag: **OOB → command selection og eksplicit dobbeltklik-navigation**, **taktisk minimap → geografisk orientering/navigation**, og **semantic zoom/NATO counters → direkte battlefield-identifikation ved udzoomning**. Samme echelon- og affiliation-sprog bruges på tværs af lagene, så spilleren ikke skal lære tre forskellige symbolsystemer. Fra F30B er OOB-kæden udvidet til `XX Division → X Brigade → III Regiment → II Battalion → I unit`, og cavalry/support ligger som attached assets frem for ekstra infanterikompagnier.

## v00.00.09f29r Crop fields, concealment og map polish

Gule afgrødemarker er nu et taktisk terrænlag og ikke kun dekoration. De giver **concealment, ikke fysisk cover**: afgrøder stopper ikke projektiler, men gør en infantry formation sværere at identificere og sigte præcist på. Effekten er derfor stærkest på LONG, mindre på MEDIUM og næsten væk på CLOSE.

Første QA-tuning er ca. **LONG -10 % hit chance, MEDIUM -5 %, CLOSE -2 %**, kombineret med en moderat spotting/engagement-range reduktion for mål inde i høj afgrøde. En enhed der selv åbner ild afslører sig midlertidigt gennem bevægelse og sortkrudtsrøg, så concealment reduceres i nogle sekunder efter volley. Det er bevidst ikke hard invisibility eller fog-of-war endnu.

Infantry bevæger sig ca. **8 % langsommere** gennem høj afgrøde. Det repræsenterer formation friction og dårligere fodfæste, ikke et pathfinding-block. Markerne har ingen hårde colliders og må ikke skabe nye navigation-detours. Senere mounted units skal have mindre concealment end infantry og en separat movement/cohesion-model i tæt afgrøde.

Det visuelle battlemap får tydelige gul/gyldne markflader med lave crop-rækker, så soldaternes ben og nederste silhuet delvist forsvinder i afgrøden ved close zoom. Samme markflader tegnes i det taktiske minimap, så terrain-reading er konsistent mellem 3D-view og overview. Markernes gameplay-volumener og visuelle polygoner bruger samme koordinater og rotation.

På længere sigt skal afgrøder kunne blive trampet ned af store formationer, så concealment reduceres efter gentagen passage, og kaptajn-AI skal kunne bruge marker som en faktor i TacticalOpportunity: dygtige officerer kan søge skjult lateral approach mod en engageret fjende, mens dårligere officerer kan overvurdere markens beskyttelse eller fejlbedømme side-/rear threat. F29R implementerer terrain/concealment-fundamentet; den fulde stat-drevne captain-opportunity beslutning forbliver efterfølgende AI-arbejde.

## v00.00.09f29y Defensive stability, TEST AI, HUD-status og terrain polish

`FORSVAR HER` skal være stabilt ved broer og floder. Hvis objective ligger på en tydelig bred, bruges denne bred. Hvis objective ligger i selve bridge/river-zonen, arver den defensive formation den bred som den ikke-reserve front allerede står på. Reserve- og flankekompagnier må ikke trække Major-HQ'ets referencepunkt væk fra fronten, og HQ rear-direction bruger den committed mission-facing i stedet for en vektor der kan vende 180° når fronten passerer objective.

F29Y ændrer ikke den fysiske movement owner: F27 flytter stadig companies og Major-HQ. Guard-laget retter kun mission/HQ goals og annullerer stale wrong-bank destinations, så F27 kan udstede det korrigerede move. F29W forbliver generelt rear-depth safety-net.

De to Prussian TEST-companies starter nu med Officer AI **OFF**. TEST-panelet er fortsat midlertidigt og foldet sammen som default, men hver række er nu en reel toggle: `Start Fjende [AI OFF]` aktiverer offensiv AI, og `Stop Fjende [AI ON]` slår den fra og giver HOLD.

Major-HUD'ens fire subordinate company rows viser fra F29Y samme taktiske runtime-status som OOB. `KLAR`, `RYKKER`, `FORSVAR`, `RESERVE`, `FLANKE`, `SQUARE`, `KAMP`, `UNDER ILD`, `CHARGE`, `MELEE`, `ROUT` m.fl. er presentation af eksisterende mission/combat-state og ikke en ny parallel state-maskine.

Terrain-polish er visuelt: F29L's kontinuerlige river-mesh erstattes i renderingen af to segmenter med et reelt tørt hul under brodækket, mens river blocker og bridge-only navigation forbliver uændret. Crop fields får terrain-conforming gyldent underlag og langt tættere crop rows; F29R's concealment, movement multiplier, volley reveal og minimap-field geometry ændres ikke.

## v00.00.09f29z Square face fire og directional black-powder smoke

Square har nu fire reelle 90° fire faces: `FRONT / RIGHT / REAR / LEFT`. Hver side har egen reload-clock og disponerer ca. **25 % af normal company volley-firepower**. Et mål kan kun beskydes, hvis det er gyldigt i den konkrete side og inden for den valgte `HOLD / CLOSE / MEDIUM / LONG` policy.

Sortkrudtsrøg emitteres fysisk langs den side, der faktisk affyrer salven. FRONT-røg kommer fra fronten, RIGHT fra højre side osv. Flere sider kan skyde uafhængigt, hvis der står gyldige mål i flere sektorer. Ingen gyldig fjende betyder ingen volley, ingen røg og ingen falsk `RAMMER 0` feedback.

Square-face ammunition registreres fractionally: fire 25 %-face volleys svarer til omtrent én gennemsnitlig patron pr. mand i den eksisterende ammo-model. F29V beholder outline/range-sector visuals, F29X beholder legacy auto-fire gating, og F29Z er den autoritative face-fire resolver.

## v00.00.09f30 First cavalry core

F30 introducerer den første fælles mounted cavalry runtime for **Gardehusarer og Dragoner**. Begge bruger samme movement-, formation- og charge-core med mounted `LINE / COLUMN`, direkte move/hold og directional charge-contact. Første styrker og hastigheder er eksplicit QA-værdier, ikke endelige historiske stats.

Gardehusar og Dragon har sabel, karabin og pistol som capability-metadata. Mounted firearms er endnu ikke aktiv combat resolution. Dragonen kan `SID AF`, bevæge sig til fod og senere `STIG OP`; hestene bliver fysisk stående ved dismount-positionen, og dragonen skal tilbage til hestene for at remounte.

Cavalry charge klassificeres relativt til infantry facing som `FRONT / FLANK / REAR`. FRONT er mindst effektiv, FLANK stærkere og REAR stærkest i første QA-model. En færdigdannet Infantry Square stopper første F30 contact og giver cavalry `FALTER`, mens en Square der stadig er under dannelse fortsat er sårbar.

Open water forbliver hard blocker. F30 cavalry ruter til bridge-zonen ved opposite-bank movement og tvinger Column under crossing. Dette er en separat cavalry movement owner; infantry F3/F27 river/navigation writers ændres ikke.

Det oprindelige F30 `CAVALRY TEST [F30]`/F10-panel var kun en bootstrap til den første mounted runtime. F30A erstatter dette parallelle kontrolsystem med normal tactical selection: Gardehusar/Dragon vælges i 3D eller OOB, bruger bund-HUD'en, højreklik terræn = move og højreklik fjende = mounted charge. OOB single-click vælger uden kameraflytning; double-click går bag enheden. Kavaleri og infantry/HQ selection er gensidigt eksklusive, så kun ét command HUD er aktivt.

Endelig cavalry Officer AI, mounted/dismounted fire, cavalry casualties fra infantry volley, Charge Confidence/Momentum, horse casualties og persistent melee ligger i efterfølgende F30-passes.

## v00.00.09f30a Command, Square og Officer AI hardening

F30A gør infantry-commandlaget stabilt nok til videre cavalry/combined-arms QA. OOB-selection behandles som reel command selection og bevares efter HUD-knapper og point-order commits uden automatisk kameraflytning. Regimentsordrer har vedvarende blå pending/active-state så længe underenheder stadig udfører ordren.

Square-transitioner må ikke læses som skud. Legacy Line/mission footprint skjules allerede under `FORMING`, Square-sector linjer løftes/terrain-conformes, og LINE↔SQUARE overgang synkroniserer ældre volley-detektorer, så kun en faktisk volley kan skabe sortkrudtsrøg.

`FORSVAR HER` bruger det klikkede objective som autoritativt centrum. En trukket pil er den autoritative facing; uden pil vælges nærmeste relevante fjende som AUTO-facing og ellers den eksisterende frontretning. River/slot-safety må korrigere ulovlige enkelt-slots, men må ikke omskrive selve missionens objective eller vende fronten vilkårligt.

`ANGRIB HER` fordeler roller efter **mænd + erfaring**. Første QA-score er `CurrentStrength × (0,60 + 0,40 × Experience/100)`: stærkeste egnede companies får assault/front, næste stærke kan få flank/support, og svageste egnede company bruges normalt som ren reserve. Terrain/bridge/charge safety ligger stadig højere end denne prioritering.

Column→Line deployment starter nu omkring **35 m uden for fjendens MaximumRange**, så den fysiske 3-rank Line-reform kan være færdig inden enheden går ind i fjendtligt skudhold. Bridge crossing er fortsat undtagelsen der kan tvinge Column gennem en smal passage.

Side Step er udvidet fra ren overlap-korrektion til **friendly fire-lane deconfliction**. Hvis et friendly company står mellem et skydende company og dets live target, skifter den taktisk mindst nyttige formation sidelæns uden 90° rotation og uden Column. Hvis front-company allerede selv har en god skudposition mod samme mål, bevares den og det bageste company stepper til en fri lane. Square, charge, melee, bridge routing, under-fire emergency reaction og direct manual authority har fortsat højere prioritet.

AI-officer fighting withdrawal er samtidig fastlagt som designregel: Kaptajn/Major skal senere kunne vælge kontrolleret withdrawal ud fra tab, lokal styrkebalance, morale/cohesion, ammunition, flank/rear threat, støtte, terræn og officerstats. Det må ikke være en automatisk panic-trigger; den eksisterende fighting-withdrawal executor genbruges når decision-laget implementeres.

Kavaleriet følger fra F30A samme **unit-selection/command UX** som øvrige taktiske enheder i stedet for et separat testpanel. Gardehusar og Dragon er company-scale OOB-rækker (`I`), single-click selecter uden kamera, double-click placerer kameraet bag enheden, og selection viser den normale 90 px bund-HUD. Mounted højreklik på fjende udfører charge; højreklik på terræn flytter. Dragon beholder `SID AF / STIG OP`. Dette er UI/command-flow integration; cavalry Officer AI og skydevåben er fortsat næste F30-arbejde.

## v00.00.09f30b Higher Command HQ og attachments — aktuel baseline

F30B gør **Brigadechef** og **Divisionschef** til fysiske, selectable HQ-lag i taktiske slag. Den aktive command chain er nu `XX Division → X Brigade → III Regiment → II Bataljon → I Kompagni/enhed`. Higher HQ giver mission intent; eksisterende Regiment/Major-lag beholder mission-decomposition og fysisk company movement, så én movement owner-reglen ikke brydes.

F30B adskiller permanent OOB fra midlertidig taktisk kommando med `OrganicParent`, `CurrentCommandParent` og `AttachmentType = Organic / Attached / Detached / Reserve`. 1. Regiment er organic under 1. Brigade. Gardehusar og Dragon er independent higher-command cavalry-assets og starter attached til 1. Brigade; Brigade-HUD kan midlertidigt attach'e dem til 1. Regiment uden at gøre dem til infanterikompagnier eller ændre deres egen movement core. Det kommende Kanonbatteri skal bruge samme model.

Det gamle separate infantry-OOB og cavalry-OOB erstattes i F30B af ét samlet træ med `XX/X/III/II/I`, hvor cavalry står under `ATTACHED / SUPPORT`. Single-click vælger uden kameraflytning og double-click går bag valgt HQ/enhed. Division/Brigade kan også vælges fysisk på battlefield og bruger en højere 90 px command-HUD med `ANGRIB HER / FORSVAR HER / RYK FREM / TILBAGETRÆK / SAML / STOP-HOLD`.

I første single-brigade/single-regiment QA delegeres en Division-/Brigadeordre gennem det eksisterende Regimental HQ-system. Command-links viser Division→Brigade→Regiment samt Brigade/Regiment→cavalry ud fra aktiv `CurrentCommandParent`. Higher Officer AI er endnu ikke autonom; F30B etablerer strukturen og den manuelle mission chain før artilleri og combined-arms AI.

F30B hardener samtidig de runtime-fejl der blev set efter F30A: charge-targeting revaliderer F25 reflection bindings før capture, et tidligt runtime safety-net reparerer transient manglende BattleManager/charge references, og den løbende ParticleSystem-repair fortsætter uden at spamme en Console-linje for hver ny sortkrudtsrøg-emitter.

## v00.00.09f30f Implementation & Fix History Consolidation — aktuel dokumentationsbaseline

F30F ændrer **ikke** tactical gameplay fra F30E. Versionen etablerer én autoritativ, kronologisk registrering af hvad der faktisk er implementeret fra projektets fundament frem til F30E, inklusive kendte compile-fixes, authority/movement-hardening, UI/HUD-regressioner, Square/Charge/terrain fixes, OOB/attachment fixes og F30E 1:1 cavalry-hardening.

Den fulde registrering ligger i [docs/IMPLEMENTATION-AND-FIX-HISTORY.md](IMPLEMENTATION-AND-FIX-HISTORY.md). Historikken skelner mellem **implementeret funktion**, **hardening/fejlrettelse** og **runtime-verificeret status**. At en rettelse er committed betyder derfor ikke automatisk, at den seneste TEST-build er runtime-verificeret i Unity.

F30F fastlåser samtidig følgende dokumentationsregel: `VERSION.txt` er den korte aktive buildstatus, mens `IMPLEMENTATION-AND-FIX-HISTORY.md` er den samlede lineage. Ældre detaljer gennem F29R bevares desuden i `docs/archive/VERSION-through-v00.00.09f29r.txt` og i de versionsspecifikke designsupplementer.

## v00.00.09f30e 1:1 Cavalry og visual fidelity — aktuel baseline

F30E gør cavalry-visningen konsistent med projektets 1:1-regel: én simuleret soldat = én synlig figur i tactical view. Gardehusar Eskadron vises derfor med 120/120 mounted riders og Dragon Eskadron med 140/140. Dragonens dismounted state fyldes tilsvarende til 140 fodfigurer, mens de 140 heste bliver stående ved horse-holder positionen med riders skjult.

1:1 implementeres i de eksisterende F30 formation-lister, så Line, Column, charge, bridge routing og Dragon dismount/remount fortsat har samme movement authority. Den større visuelle formation får et tilsvarende større collider footprint. Semantic zoom/LOD må senere optimere rendering på afstand, men må ikke genindføre 1:5/1:10 som tactical-strength proxy.

Gardehusar og Dragon får forskellige visuelle identiteter inspireret af de godkendte referencebilleder: Gardehusar læses med lys blå hussar-silhuet, sølvbraid, mørk fur-hovedbeklædning, røde accenter og sabel; Dragon med mørk blå coat, røde facings, crested helmet, carbine og sabel. Cavalry-/HQ-heste får komplet low-poly hestesilhuet med fire ben/hove, hals, hoved/mule, man/hale, ører, saddle og bridle/reins. Higher HQ forbliver mounted staff; officererne sidder på hestene og får tydeligere arme/ben/støvler/headgear/sabre.

Mounted formations forbliver i F30E LINE / COLUMN. En trekantet WEDGE implementeres ikke som standard 1851/1864-formation. ECHELON LEFT / ECHELON RIGHT er næste historisk plausible cavalry-formation til flank/rear maneuver før deployment til Line-charge.

## Directional fire, fire discipline og accuracy

Infantry fire policy bruger `HOLD / CLOSE / MEDIUM / LONG`. Et mål skal være inden for fire-policy-afstanden og relevant fire arc. Close/Medium/Long er ordre-/UI-grænser; accuracy ændres kontinuerligt med faktisk afstand.

Line/Column bruger den almindelige forward fire model. Square er fra F29Z formation-segmenteret i fire selvstændige 90° faces med ca. 25 % firepower pr. side. Den langsigtede combat-fase fortsætter med mere generel `eligibleFiringFraction`, LOS, friendly obstruction, terrain/smoke og target exposure.

## Command-visualisation og order lifecycle

Fysiske HQ-entities, command-links, courier/order lifecycle, fog-of-war reports og semantic zoom er den fælles retning. Valg af et HQ viser relationer til direkte underenheder. Valg af et company viser desuden relationen tilbage til dets direkte Major, og valg af en Major viser relationen op til den aktuelle Oberstløjtnant, så command chain kan læses begge veje under QA og senere semantic zoom.

Ved Regimental HQ selection vises i F29E også det fulde downstream hierarchy og aktuelle company mission destinations. Fra F30B fortsætter hierarchy visibility op gennem Brigade og Division og ud til attached support-assets. Command relationship lines, persistent HQ objective markering og konkrete future courier/order routes er tre forskellige overlays og må ikke semantisk blandes sammen.

Ordrer transporteres senere gennem et egentligt courier/order-lifecycle-system. En aktiv ordre kan vises som en route fra afsender-HQ til modtager med en bevægelig courier-markør, hvis position svarer til faktisk simulation progress.

Courier-interception håndteres primært som område-/risikomodel. Enemy presence, cavalry/scouts, screening, roads, terrain, mørke og command quality kan føre til reroute, delay, searching eller lost/intercepted. Fjendens couriers er selv underlagt fog of war.

## Fire eligibility og højere formationer

Combat resolution skal senere beregne hvor stor en del af formationens frontage der faktisk kan skyde på målet. Formationens frontage opdeles i fire groups/segmenter, som testes mod fire arc, LOS, range, friendly obstruction, terrain/smoke og target exposure.

Højere HQ'er opstiller underenheder efter data-drevne formation templates, fx 4 abreast, 3 + 1 reserve, 2 + 2, echelon og march column. Reserve er en faktisk rolle/state, ikke en fast bonus.

## Battle supply, nat og flerdagsslag

Forsyning under taktiske slag er fysisk og begrænset. Enheder forbruger konkret ammunition og kan kun genforsynes fra kompatible wagons/caissons/field trains/depots med beholdning, transportkapacitet og brugbar rute.

Skumring er en phase transition, ikke et universelt hard stop. Battle state kan fortsætte som `DAYLIGHT -> DUSK -> NIGHT -> DAWN`. Natten er et naturligt resupply-/reorganisation-vindue, men afskårne enheder får ingen magisk ammunition.

## Kavaleri, Gardehusarer, dragoner og infantry square

Danske **Gardehusarer og dragoner er separate cavalry-typer**, men bygges på en shared cavalry core. Dansk cavalry modelleres ikke som kun sabel; data-driven weapon profiles kan omfatte **sabel, karabin og pistol**, mens de konkrete regimentsspecifikke 1864-profiler research-verificeres før endelige stats låses.

Dragoner kan bevæge sig mounted og sidde af til sustained fire/combat. Ved dismount efterlades horses og horse holders som tactical state; remount tager tid og afhænger senere også af hestenes tilstand.

Cavalry charge er directional. FRONT mod steady, facing infantry er den mindst fordelagtige contact; FLANK giver større shock; REAR kan give severe casualties/stragglers, cohesion- og morale shock og høj break/rout probability, især hvis infantry allerede er engaged forfra.

Et mounted charge er ikke garanteret at nå melee. `Charge Confidence / Charge Momentum` skal påvirkes af casualties, horse hesitation, cohesion, morale, terrain og defensive fire. En vel-timet **close-range volley** fra steady infantry skal kunne få et frontalt charge til at `FALTER`, `ABORT` eller i ekstreme tilfælde `ROUT` før fysisk kontakt. F30-baseline implementerer endnu kun Square-contact FALTER; defensive volley-interception kommer i næste passes.

Infantry får formationerne:

`LINE / COLUMN / SQUARE`

**SQUARE / KARRÉ** er en specifik anti-cavalry formation. Den tager tid at danne, har lav mobility og er stærk mod mounted charge fra flere retninger, men bliver et tættere og mere sårbart mål for artilleri og koncentreret infantry fire. Square må ikke give 360° full-strength volley; F29Z fordeler den faktiske firepower på fire konkrete sider. Cavalry der rammer mens square stadig dannes kan skabe kraftig disorder og høj break/rout risk.

Cavalry kan også skabe combined-arms pressure uden at charge: en troværdig mounted threat kan tvinge infantry i square og dermed gøre det mere sårbart for artilleri eller infantry manoeuvre.

Efter F30B higher-command fundamentet er næste systemrækkefølge: **mounted/dismounted cavalry AI + defensive-fire/charge-momentum → Kanonbatteri/Artilleri på attachment-modellen → Combined Arms QA → flere Regimenter/Brigader og fuld higher Officer AI**.

## Fog of war, scouts og command effectiveness

Fog of war er en knowledge-state model. En fjendtlig formation kan være `Unknown`, `Suspected`, `Contact`, `Identified`, `Fresh observation` eller `Stale`. Når kontakt mistes, bevares last known position med faldende confidence i stedet for perfekt live-tracking.

Reconnaissance kommer fra faktiske kilder som cavalry patrols, dragoner, skirmishers/scout detachments, line units, HQ og observation points. Information rapporteres gennem command-nettet, så lokal Officer AI kan reagere på frisk information før overordnet HQ har modtaget rapporten.

HQ får et visuelt command effectiveness envelope, men ikke en hård magisk radius. Dårlig connectivity påvirker order delay, acknowledgement, reporting, coordination, reserve/support reaction og afhængighed af lokal Initiative/Tactical Skill/Composure. Den giver ikke en vilkårlig direkte accuracy- eller damage-penalty.

## Versionshistorik

- **v00.02.22 / P0A v00.00.09f30f IMPLEMENTATION + FIX HISTORY CONSOLIDATION TEST** — Dokumentationsbaseline uden gameplayændring: fuld kronologisk implementation/fix-history fra projektstart til F30E samlet i `docs/IMPLEMENTATION-AND-FIX-HISTORY.md`; `VERSION.txt` gjort til kort aktiv buildstatus; fejlrettelsesregister samler compile-, movement/authority-, HUD/OOB-, Square/Charge-, terrain/pathfinding-, NullReference- og 1:1 cavalry hardening; gameplay-baseline forbliver F30E.

- **v00.02.21 / P0A v00.00.09f30e 1:1 CAVALRY + HISTORICAL VISUAL FIDELITY TEST** — cavalry skifter fra F30D 1:5 proxy til tactical 1:1: Gardehusar 120/120, Dragon 140/140 og Dragon dismounted 140/140; Gardehusar/Dragon får særskilt uniform/headgear/equipment-silhuet; horse/HQ mounted detail løftes; collider footprint følger den større 1:1 formation; Line/Column bevares og Echelon Left/Right registreres som næste cavalry-formation; Wedge ikke standardiseret.

- **v00.02.20 / P0A v00.00.09f30b HIGHER COMMAND HQ + ATTACHMENT TEST** — Fysisk Division-/Brigade-HQ; samlet XX/X/III/II/I OOB; higher mission delegation til eksisterende Regimental HQ pipeline; OrganicParent/CurrentCommandParent/AttachmentType; Gardehusar/Dragon default under Brigade med mulighed for midlertidig Regiment attachment; terrain-following higher command-links; F30A runtime NullReference hardening og reduceret particle-repair logspam; Kanonbatteri klargjort til samme attachment-model.
- **v00.02.19 / P0A v00.00.09f30a COMMAND + SQUARE + OFFICER AI HARDENING TEST** — OOB/HUD selection persistence; Square footprint/cone/smoke transition hardening; vedvarende blå regimentsordre-state; autoritativt FORSVAR HER objective/facing med threat-based AUTO-facing; attack-role prioritering efter mænd + erfaring; tidligere Column→Line deployment; friendly fire-lane Side Step; AI-officer fighting withdrawal fastlagt som designregel; F30 Cavalry TEST/F10-panel erstattet af normal battlefield/OOB selection og bottom-HUD command flow for Gardehusar/Dragon.
- **v00.02.18 / P0A v00.00.09f30 FIRST CAVALRY CORE TEST** — Shared mounted cavalry core med Gardehusar + Dragon; Line/Column, move/hold/charge; FRONT/FLANK/REAR contact; ready Square giver cavalry FALTER; Dragon dismount/remount med fysisk horse-holder position; bridge-only cavalry crossing; midlertidigt F10 TEST-panel; mounted firearms, defensive volley interception, cavalry AI og persistent melee udestår.
- **v00.02.17 / P0A v00.00.09f29z SQUARE FACE FIRE + DIRECTIONAL SMOKE TEST** — Fire selvstændige 90° Square faces med ca. 25 % firepower pr. side, independent reload, sidekorrekt black-powder smoke, fractional ammo consumption og ingen falsk RAMMER 0 uden gyldigt mål.
- **v00.02.16 / P0A v00.00.09f29y DEFENSIVE STABILITY + TEST AI + HUD STATUS + TERRAIN POLISH TEST** — DefendHere bank-lock ved river/bridge; reserve/flank fjernet fra Major-HQ anchor; stable mission-facing for HQ rear axis; TEST Prussian AI default OFF med reel ON/OFF-toggle; Major-HUD bruger samme company-status som OOB; dry bridge river-render gap; terrain-conforming gyldne crop fields med langt tættere rows; F29R gameplay-terrain uændret.
- **v00.02.15 / P0A v00.00.09f29r CROP FIELD CONCEALMENT + MAP POLISH TEST** — Gule/gyldne crop fields gjort til gameplay-terrain: concealment uden ballistic cover; ca. -2/-5/-10 % target hit chance ved CLOSE/MEDIUM/LONG; moderat spotting-range reduktion; volley/sortkrudtsrøg reducerer concealment midlertidigt; ca. 8 % infantry movement penalty; crop rows og minimap-markering bruger samme field geometry; ingen hard colliders/path blockers; senere trample og stat-drevet Captain TacticalOpportunity dokumenteret.
- **v00.02.14 / P0A v00.00.09f29q OOB + HQ DISCOVERABILITY + SEMANTIC ZOOM TEST** — Sammenklappeligt OOB-panel med III/II/I-hierarki; company/HQ selection og camera navigation fra OOB; dobbeltklik til taktisk kamera bag valgt entity; kompakt strength/state i OOB; selected row fremhæves; semantic zoom thresholds ca. 55/95/175/315 m; HQ NATO-counters prioriteres visuelt; OOB, minimap og battlefield NATO counters bruger samme echelon-/affiliation-sprog.
- **v00.02.13 / P0A v00.00.09f29e REGIMENTAL DEFENSE + OBJECTIVE VISUALS + COORDINATED ATTACK TEST** — Unified Oberstløjtnant HUD; persistent regimental objective circle/cross; full Oberstløjtnant→Major→Company hierarchy and company destination visibility; precise final-slot arrival; continuous defensive frontage through ordered centre; nearest-battalion FRONT role enforcement; coordinated explicit AttackTarget deployment; enlarged-map navigation guard replacing obsolete ±176/±116 navigation bounds while retaining bridge-only river authority.
- **v00.02.12 / P0A v00.00.09f29d SEMANTIC ZOOM + NATO TACTICAL OVERLAY TEST** — Fire semantic zoom levels; HQ-prioriterede screen-space counters/beacons; NATO-echelon I/II/III; operational company overlays med strength/morale/ammo/facing; strategic view skjuler company/HQ meshes men bevarer simulation, colliders og LineRenderers; designet udvideligt til Brigade X, Division XX, screen-footprint thresholds, clickable counters og fog-of-war symbol states.
- **v00.02.11 / P0A v00.00.09f29c UNIFIED COMMAND HUD TEST** — Fælles kompakt Major/Company bottom HUD; rød/grøn state coding; Major subordinate overview med mænd/tab/morale/ammo; grouped company shooting, withdrawal, forced march, charge, STOP og LINJE/KOLONNE/SQUARE; F29 SQUARE gjort synlig i company HUD; danske company display names normaliseret til 1.-8. KOMPAGNI mens gamle Regiment strings bevares som interne compatibility-id'er.
- **v00.02.10 / P0A v00.00.09f29b BATTLEFIELD + ATTACK AI TEST** — Tactical battlefield 5760 × 3840 m; to Prussian company-scale QA-enheder; single physical movement owner-regel; explicit `AttackTarget` sticky ved contact; frontage-planner gjort movement-write-free; approach formation respekterer Major/under-fire authority; F29a command-chain visuals og river footprint safety retained.
- **v00.02.09 / P0A v00.00.09f28 REGIMENT CONTROL TEST** — Dansk command chain fastlagt som Kaptajn → Major → Oberstløjtnant, senere udvidet med Brigadechef/Divisionschef i F30B; to bataljoner / to Majorer / otte kompagnier under fysisk Oberstløjtnant-HQ; hierarchical authority/reclaim og dynamic HQ command zones; Gardehusarer og dragoner separeret som cavalry-typer på shared core; sabel/karabin/pistol som data-driven cavalry weapon model; FRONT/FLANK/REAR charge shock; close-range volley kan FALTER/ABORT cavalry charge; infantry `SQUARE/KARRÉ` besluttet med reel formation time, multi-side defense og artilleri-trade-off.
- **v00.02.08 / P0A v00.00.09 TACTICAL COMMAND TEST work branch** — Shared `OfficerAIController`/`OfficerProfile`; delegation, doctrine, commander aggression intent, directional infantry fire, HOLD/CLOSE/MEDIUM/LONG, time control, battle supply/night/cavalry/fog-of-war retning.
- **v00.02.08 / P0A v00.00.08** — Våbenprofil/reload, experience, volley feedback, ammunition/casualty model og første expanded systems baseline.
- **v00.02.07** — P0A v00.00.07: statisk Unity 6.6 QA-hardening før runtime-validering.
- **v00.02.06** — Unity compile-gate: `CS0136` rettet, obsolete object lookup erstattet og unused state fjernet.
- **v00.02.05** — Built-in IMGUI, Particle System, Physics og Audio moduler aktiveret.
- **v00.02.04** — Unity baseline flyttet til 6000.6.0f1.
- **v00.02.03** — Repository-roden fastlåst som Unity project root.
- **v00.02.02** — Unity Editor metadata/version rettet.
- **v00.02.01** — Første konkrete P0A implementation koblet til roadmap.
- **v00.02.00** — Expanded systems baseline: økonomi, udvikling, handel, forskning, rekruttering, træning, sanitet/fanger, regimentshistorik, faner og traits.
- **v00.01.00** — Første samlede designbaseline.

## Projektregel

Designmanualen skal opdateres både som layoutet Word-master og her i GitHub, når designbeslutninger eller implementeringsbaselines ændres. Git-historikken bevarer tidligere udgaver af Markdown-delene. Nye beslutninger og idéer registreres desuden i backloggen. Hver testbuild skal have tydelig release-dokumentation, der adskiller **implementeret nu** fra **besluttet senere**.
