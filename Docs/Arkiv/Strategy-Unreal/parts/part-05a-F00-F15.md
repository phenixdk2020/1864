# PROJECT 1864 — Designmanual v00.02.08 — Feature Architecture

## 28. Feature-arkitektur – udviklingsklumper

Nedenstående chunks beskriver funktionelle leverancer, ikke kalenderestimater. Hver chunk skal kunne testes isoleret og har exit-kriterier. Nummereringen er foreslået dependency-order, men flere spor kan udvikles parallelt når fundamentet er stabilt.

### F00 – Projektfundament og simulationskerne

Formål. Skabe en stabil, data-drevet Unity-arkitektur, hvor simulation state er adskilt fra visuelle GameObjects.

Indhold.

- Stable IDs og registries for nationer, officers, formations, locations og equipment.
- Simulation clock, event queue, deterministic/random service og saveable state.
- Domain assemblies: Strategic, Military, Tactical, AI, UI, Data.
- Automated tests for state transitions og serialization.

Afhængigheder. Ingen. Dette er fundamentet.

Exit-kriterier.

- En headless simulation kan starte, køre 30 simulerede dage og gemme/indlæse uden Unity-sceneafhængighed.
- Samme unit state kan bindes til og afkobles fra en visual representation uden datatab.

Slutversionsnote. Alt andet bør bygge på dette lag.

### F01 – Strategisk kort, kamera og tid

Formål. Gøre kampagnen navigerbar og levende i realtid.

Indhold.

- 3D-reliefkort prototype.
- Pan/zoom/rotate, map selection og overlays.
- Pause + speed steps.
- World locations, roads og basic path graph.

Afhængigheder. F00

Exit-kriterier.

- To testformationer kan marchere samtidigt mens tiden kører.
- Pause/resume og speed ændrer ikke simuleringsresultatet ud over timing.

### F02 – Nationer, regioner og politisk kontrol

Formål. Etablere world-state for Danmark, Sverige-Norge, Preussen og Østrig.

Indhold.

- Nation definitions, ownership/control, regions, cities, ports og strategic objectives.
- War/peace relation og simple war goals.
- Basic occupation/control transition.

Afhængigheder. F00, F01

Exit-kriterier.

- Scenario kan indlæse alle kernemagter og regioner med stable state.
- Territorial control kan ændres og gemmes.

### F03 – OOB og enhedsdata

Formål. Implementere den historiske kommandostruktur som generisk tree/graph.

Indhold.

- CommandNode med valgfrie niveauer.
- Parent, attached-to, subunits og commander assignments.
- Strength, current effective men, killed, wounded, missing/captured, morale, cohesion og fatigue.
- Weapon profiles, compatible ammunition, ammunition carried og supply fields.
- Equipment ownership/state for guns, wagons, horses og øvrigt materiel.
- OOB browser/editor for debug.

Afhængigheder. F00, F02

Exit-kriterier.

- Dansk og preussisk eksempel-OOB kan eksistere med forskellige hierarkier.
- En unit kan detach/reattach uden at miste identitet eller historik.
- En unit kan rapportere fx aktuelt mandskab, dræbte/sårede og ammunition uden at disse værdier udledes af visuelle modeller.

### F04 – Officer-system og autonomi

Formål. Gøre officerer til aktive beslutningstagere.

Indhold.

- Skills, traits, personality og reputation.
- Commander intent evaluation.
- Autonomy levels og local reaction scoring.
- Officer succession ved casualty/removal.

Afhængigheder. F03

Exit-kriterier.

- To officerer med forskellige profiler reagerer dokumenterbart forskelligt på samme threat.
- AI-handling kan forklares via debug reason codes.

### F05 – Ordrer, couriers og kommunikation

Formål. Fjerne telepatisk kontrol og skabe command friction.

Indhold.

- Order object lifecycle.
- Courier/telegraph transmission model.
- Delay, delivery, acknowledgement og supersede.
- Order composer UI prototype.

Afhængigheder. F03, F04, F01

Exit-kriterier.

- En ordre kan sendes, forsinkes, leveres og udføres efter simuleret tid.
- En officer kan reagere lokalt før en ny ordre ankommer.

### F06 – Operationel march, congestion og contact

Formål. Skabe meningsfuld bevægelse mellem strategiske objectives.

Indhold.

- Road-aware movement, formation speed og fatigue.
- March column length og chokepoint congestion.
- Screening/recon radius.
- Encounter detection og deployment state.

Afhængigheder. F01, F03, F05

Exit-kriterier.

- To brigader kan marchere på samme vej og skabe realistisk kø.
- Enemy contact kan føre til stop, withdrawal eller battle trigger afhængigt af orders/AI.

### F07 – Logistik og supply network

Formål. Gøre jernbaner, veje, depoter og transport strategisk relevante.

Indhold.

- Depots, supply nodes og throughput.
- Food, ammunition, forage og transport capacity.
- Consumption og resupply priority.
- Supply wagons/convoys som konkrete inventory-holdere med crew, trækdyr og ownership-state.
- Capture, abandonment og destruction af supply-elementer med delvist tab af lasten.
- Supply overlay og alerts.

Afhængigheder. F02, F03, F06

Exit-kriterier.

- En formation afskåret fra depot mister supply over tid og får gameplay-effekter.
- En alternativ vej/depot kan reetablere supply.
- En supply-vogn kan bringe ammunition frem, mistes eller erobres, og beholdningen følger den faktiske ownership/state.

### F08 – Fog of war og reconnaissance

Formål. Erstatte omniscience med tidsstemplet information.

Indhold.

- Observation records med confidence og age.
- Cavalry/scout spotting.
- Last-known-position og estimated strength.
- Intel map overlay.

Afhængigheder. F03, F06

Exit-kriterier.

- AI og spiller kan have forskellige knowledge states om samme unit.
- Gammel observation bliver tydeligt stale og kan være forkert.

### F09 – Tactical battle bootstrap

Formål. Kunne åbne en 3D-slagscene med de faktiske kampagneenheder.

Indhold.

- Battle instance generator.
- Transfer af units/commanders/time/weather fra strategic state.
- Deployment zones fra approach vectors.
- Return af resultat til campaign state.

Afhængigheder. F03, F06

Exit-kriterier.

- Et encounter kan starte et 3D-slag og returnere ændrede styrker uden duplikation af units.
- Campaign clock fremrykkes med battle duration.

### F10 – Formation rendering 1:10, stance og tactical movement

Formål. Vise regimenter/bataljoner som store levende formationer uden individual-agent overhead.

Indhold.

- Formation slots og render ratio.
- GPU/animation instancing baseline.
- Line/column/skirmish formations.
- Formation-level pathfinding + local avoidance.
- Stance-state mindst standing og prone; senere kneeling/skirmish variationer.
- Occupy-cover behavior, hvor formationen kobles til fysisk cover geometry og retning i stedet for en abstrakt buff.

Afhængigheder. F09

Exit-kriterier.

- 1.000 simulerede mænd kan vises som ca. 100 modeller og skifte formation stabilt.
- Render ratio kan ændres uden at combat state ændres.
- En formation kan gå fra standing til prone og tilbage med tids-/movement-/cohesion-konsekvens.
- En enhed kan besætte et hegn, en grøft eller et brystværn og bevare korrekt facing mod coveret.

### F11 – Infanteri-ild, range, ammunition, cover og smoke

Formål. Skabe den centrale synlige black-powder ildkamp.

Indhold.

- Weapon profiles, loading method, reload cadence og range curves.
- Experience/training som bounded modifier på reload og fire discipline.
- Stance-afhængig reload; muzzle-loaders får større penalty i prone end breech-loaders.
- Volley/independent fire og konkret ammunition consumption.
- Casualty resolution med killed/wounded/missing hooks.
- Directional cover og concealment fra hegn, mure, grøfter, bygninger, vegetation og terrain folds.
- Prone reducerer target profile; effekt beregnes separat for small arms og forskellige artillery/ammunitionstyper.
- Muzzle effects, representative projectiles og smoke field.
- Range overlay og combat feedback.

Afhængigheder. F10, F03, F07

Exit-kriterier.

- Effekten af 100 m og 400 m ild er forskellig efter weapon curve.
- To enheder med samme våben men forskellig experience har dokumenterbart forskellig reload uden at teknologiforskellen udviskes.
- Enheden kan løbe tør for ammunition og genforsynes fra kompatibel supply.
- En prone formation er målbar sværere at ramme end en tilsvarende stående formation, uden at være immun mod artilleri.
- Cover virker kun fra relevante retninger og ophører, når enheden forlader positionen.
- Smoke reducerer faktisk LOS/accuracy, ikke kun grafik.

### F12 – Morale, cohesion, fatigue og rout

Formål. Gøre slag til kamp om organisation og vilje, ikke HP.

Indhold.

- Continuous morale/cohesion/fatigue values.
- State thresholds: shaken/wavering/rout.
- Rally, officer influence og contagion.
- Stragglers, prisoners og pursuit hooks.

Afhængigheder. F11, F04

Exit-kriterier.

- En formation kan rout’e før udslettelse og senere rally.
- Flank/rear threat og officer quality kan ændre udfald mærkbart.

### F13 – Artilleri

Formål. Tilføje feltartilleri som fuldt taktisk system.

Indhold.

- Gun/battery data, limber/unlimber, ammunition.
- Targeting, LOS, trajectory visuals, counter-battery.
- Ammunitionstyper får forskellige effektmodeller mod standing/prone, open ground og cover/fieldworks.
- Crew/horses og explicit equipment state: operational, abandoned, disabled/destroyed, captured.
- Capture kræver fysisk kontrol; genbrug kræver egnet crew, ammunition og klargøringstid.

Afhængigheder. F09, F11, F12

Exit-kriterier.

- Et batteri kan deploye, skyde, bruge ammunition, flytte, blive forladt og blive erobret.
- Piece count, condition og ownership returneres korrekt til kampagnen.
- Prone/cover påvirker artilleriets casualty probability efter ammunitionstype uden at gøre beskyttelsen absolut.

### F14 – Kavaleri, dragoner og reconnaissance

Formål. Forbinde operationel information med taktisk mobility.

Indhold.

- Screening, reconnaissance og courier modifiers på strategic layer.
- Tactical charge, pursuit og dismounted options efter type.
- Dragoner har mounted/dismounted state og kan føre ildkamp til fods.
- Horse-holder/remount state: heste står separat under afsiddet kamp, kan lide tab og remount tager tid.
- Horse fatigue/casualties/forage.

Afhængigheder. F06, F08, F10, F11, F12

Exit-kriterier.

- Kavaleri kan forbedre contact information og påvirke retreat/pursuit.
- Dragoner kan sidde af, skyde og senere remount uden at miste enhedsidentitet/state.
- Horse losses påvirker efterfølgende mobility.

### F15 – Ingeniører, hasty fieldworks, broer og befæstning

Formål. Gøre terrænændringer og fortifikationer til vedvarende state.

Indhold.

- Field works, bridge tasks, demolition, repair.
- Almindeligt infanteri kan bygge hasty cover: skyttehuller, lave jordvolde/brystværn og improviserede barrikader, når tid, terræn og værktøj tillader det.
- Ingeniører giver højere build-rate og kan konstruere/forbedre mere avancerede stillinger.
- Field works har position, orientation, construction progress, condition og cover class/value.
- Artilleri kan beskadige fieldworks; eksisterende stillinger kan forbedres eller repareres.
- Task progress og material/tool requirements.
- Transfer mellem strategic improvements og tactical objects.

Afhængigheder. F06, F07, F09, F10, F11

Exit-kriterier.

- En bygget/ødelagt bro påvirker både strategic pathing og battle map.
- Infanteri kan etablere en simpel hasty position over tid og få directional cover, når den fysisk besætter stillingen.
- Ingeniører bygger samme type stilling hurtigere eller stærkere end almindeligt infanteri.
- Entrenchment kan forbedres over tid og følge unit/position data.
