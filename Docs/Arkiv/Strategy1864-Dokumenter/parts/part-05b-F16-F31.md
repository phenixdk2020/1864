# PROJECT 1864 — Designmanual v00.02.08 — Feature Architecture

### F16 – Battle terrain pipeline

Formål. Generere troværdige slagmarker fra strategisk geografi.

Indhold.

- Elevation/landcover/road/stream ingestion.
- Procedural tile composition.
- Historical landmark overrides.
- Battle bounds og deterministic generation.

Afhængigheder. F01, F09

Exit-kriterier.

- Samme encounter coordinate genererer samme basis-slagmark.
- Roads/woods/streams matcher strategiske source data inden for defineret tolerance.

### F17 – After Action, retreat, salvage og persistence

Formål. Gøre taktiske resultater operationelt meningsfulde.

Indhold.

- Casualty categories: killed, wounded, missing, captured og return-to-duty hooks.
- Captured/abandoned equipment med type, condition, ownership og compatible ammunition.
- Battlefield salvage fra overgivne, dræbte/sårede, forladte guns/limbers og supply-vogne.
- Recoverable percentage, så en del våben/materiel er ødelagt, tabt eller ubrugeligt.
- Retreat corridor resolution.
- Fatigue/cohesion aftermath og recovery.
- After Action Report med mandskab, tab, ammunition, captured materiel og remaining supply.

Afhængigheder. F03, F07, F09, F11, F12, F13, F14

Exit-kriterier.

- Tab kan spores helt ned til relevante subunits og vises separat som dræbte/sårede/missing/captured.
- Blocked retreat kan skabe prisoners/captured guns i stedet for generisk attrition.
- Våben fra overgivne/faldne og forladt materiel kan bjærges med en dokumenteret ikke-genvindelig skadeandel.
- Erobrede våben eller ammunition kan ikke automatisk anvendes, hvis type/ammunition/træning ikke er kompatibel.

### F18 – Strategisk AI

Formål. Give alle lande troværdig operationel adfærd.

Indhold.

- War goals, theatre objectives, threat assessment.
- Concentration, reserve og supply-aware planning.
- Engagement/withdrawal thresholds.
- Fog-of-war compliant decision making.

Afhængigheder. F02, F06, F07, F08

Exit-kriterier.

- AI kan gennemføre en simpel kampagne uden scripts.
- AI-planer kan diagnosticeres via reason codes/telemetry.

### F19 – Taktisk hierarkisk AI

Formål. Lade delegation og autonome commanders føre slag.

Indhold.

- Battle commander plan.
- Division/brigade mission decomposition.
- Battalion local execution.
- Officer personality integration.

Afhængigheder. F04, F10, F11, F12

Exit-kriterier.

- En komplet side kan spille et slag uden spillerinput.
- Commander skills/personality ændrer taktiske valg på reproducerbar måde.

### F20 – Flåde og transport

Formål. Gøre den danske maritime dimension relevant.

Indhold.

- Naval task forces, ports, sea zones/routes.
- Blockade, escort, interception og troop transport.
- Embark/disembark og coastal support.
- Senere detailed naval battle hook.

Afhængigheder. F02, F07

Exit-kriterier.

- Danmark kan flytte landstyrker via skib og opretholde en blockade med supply cost.
- Enemy naval presence kan true transport routes.

### F21 – Mobilisering, manpower og økonomi

Formål. Understøtte en flerårig krig og replacements.

Indhold.

- Population/manpower pools.
- Mobilisation/training pipeline.
- Replacement distribution.
- Weapon/ammunition production og budget constraints.

Afhængigheder. F02, F03, F07

Exit-kriterier.

- Enheder kan modtage replacements, men ikke hurtigere end manpower/training/transport tillader.
- National resource shortages påvirker campaign capability.

### F22 – Diplomati, krigsmål og historiske events

Formål. Gøre 1864 til en politisk krise, ikke kun et map conquest-spil.

Indhold.

- War goals, peace willingness, international pressure.
- Sverige-Norge intervention framework.
- Event engine med historical triggers og alternative outcomes.
- Scenario objective scoring.

Afhængigheder. F02, F18, F21

Exit-kriterier.

- En kampagne kan slutte gennem forhandling/krigsmål uden total conquest.
- Events kan trigges af faktisk simulation state, ikke kun dato.

### F23 – UI/UX, rapportering og accessibility

Formål. Gøre den komplekse simulation læsbar og håndterbar.

Indhold.

- OOB tree, order composer, overlays, message center.
- Battle cards med aktuelt mandskab, tab, killed/wounded split, ammunition, weapon/reload, morale/cohesion og command status.
- Range overlays og kort combat feedback som resolved hits.
- After Action reports med captured/salvaged materiel og supply.
- Tooltips med forklaringer.
- Scalable UI, keybinds og accessibility options.

Afhængigheder. Alle kernesystemer leverer data til UI; kan udvikles iterativt fra F01.

Exit-kriterier.

- Alle kritiske simulationstilstande kan aflæses uden debug tools.
- Spilleren kan finde årsagen til supply-, order- eller moraleproblemer via UI.
- Mandskab, casualty split og ammunition kan aflæses uden at åbne Inspector/debug-menu.

### F24 – Save/load, replay og modding

Formål. Sikre holdbarhed, testbarhed og udvidelsesmuligheder.

Indhold.

- Robust save versioning/migration.
- Event/replay log til debugging.
- Externalized data og localisation.
- Scenario/OOB/event mod support.

Afhængigheder. F00 og løbende integration.

Exit-kriterier.

- Save fra længere kampagne kan load’es deterministisk nok til fortsættelse.
- Nyt regiment/officer/event kan tilføjes uden at ændre core code.

### F25 – Performance, QA og balance

Formål. Få slutvisionen til at køre stabilt og kunne balanceres.

Indhold.

- Performance budgets pr. system.
- Battle stress tests med tusindvis af modeller.
- Simulation regression tests.
- AI telemetry, combat analytics og historical benchmark scenarios.

Afhængigheder. Alle systemer; starter tidligt og fortsætter hele projektet.

Exit-kriterier.

- Definerede target-platforme holder frame/sim budgets i representative worst cases.
- Reference-scenarier giver reproducerbare resultater inden for accepterede intervaller.

### F26 – Population & Labour Force

Formål. Gøre befolkning og mobilisering til samme ressourcebase.

Indhold.

- Regional population og workforce pools.
- Civil/military status og mobilisation opportunity cost.
- Demographic effects fra krig, sygdom og migration.

Afhængigheder. F02, F21

Exit-kriterier.

- Mobilisering reducerer konkret civil arbejdskraft, og regional produktion reagerer på ændringen.

### F27 – Agriculture, Food & Horses

Formål. Skabe den primære førindustrielle forsyningsbase.

Indhold.

- Gårde, harvest cycles, grain/food.
- Livestock/horses og forage.
- Local consumption, surplus og military requisition.

Afhængigheder. F26, F07

Exit-kriterier.

- Hære og civilbefolkning kan forsynes fra producerede/lagrede varer, og hestemangel påvirker mobilitet.

### F28 – Resources & Extraction

Formål. Gøre kul, jern, træ og byggematerialer geografiske.

Indhold.

- Resource sites og regional output.
- Workforce/infrastructure constraints.
- Expansion/investment og transport til marked/industri.

Afhængigheder. F26, F01, F07

Exit-kriterier.

- Råstoffer kan spores fra kilde til forbrug, og afskæring giver målbar produktionsmangel.

### F29 – Cities & Regional Development

Formål. Gøre byer til økonomiske, administrative og logistiske knudepunkter.

Indhold.

- Urban population og services.
- Ports, arsenals, hospitals, education, rail terminals.
- Langsigtede investeringer og organisk vækst.

Afhængigheder. F02, F26, F28

Exit-kriterier.

- Bykapaciteter påvirker handel, produktion, sanitet, supply og rekruttering uden at kræve SimCity-mikro.

### F30 – Industry & Production Chains

Formål. Omsætte råvarer og arbejdskraft til civil og militær kapacitet.

Indhold.

- Workshops/factories og machine capacity.
- Input/output production chains.
- Maintenance/repair versus new production.

Afhængigheder. F28, F29, F34

Exit-kriterier.

- Industrien stopper eller drosler ned ved manglende inputs, og output ender som faktiske varer/lagre.

### F31 – Military Production & Stockpiles

Formål. Gøre udrustning til en konkret begrænsning for mobilisering og kamp.

Indhold.

- Rifles, artillery, ammunition, uniforms og transport equipment.
- Weapon models, compatible ammunition og stockpiles.
- Captured/salvaged materiel som en legitim lagerkilde, men med condition, compatibility og repair requirements.
- Arsenal/depot distribution og repair.

Afhængigheder. F30, F07, F17

Exit-kriterier.

- En ny enhed kan kun udrustes med materiel, der faktisk findes, produceres, importeres eller er bjærget/erobret og gjort brugbart.
