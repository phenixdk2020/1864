# PROJECT 1864 — Backlog, beslutninger og idéer

**Baseline: v00.02.79 — researched infantry firing-drill progression**

Dette dokument er projektets centrale intake-log for beslutninger og idéer, der ikke nødvendigvis implementeres i den aktive prototype med det samme. Formålet er at sikre, at gameplay-idéer og designbeslutninger fra projektarbejdet ikke kun findes i chatten.

**Unreal-migration:** Feature-for-feature parity mod den eksisterende Unity battle-prototype spores separat i [UNREAL-PARITY-BACKLOG.md](UNREAL-PARITY-BACKLOG.md). Denne hovedbacklog beholder langsigtet game-design scope; Unreal parity-backloggen bruges til selve engine-migreringen.

**Unreal gameplay checkpoint v00.02.57:** Grafik/asset-passet er bevidst udskudt. Den aktive Unreal-port har nu funktionel mission-anchor for Attack/Defend, explicit obstacles + slope/elevation movement, officer stats + command delay, fatigue/experience gameplay, contact memory/FOG-fire authority, SPEJD HER recon state machine, cavalry screen/reform-gated charge, OOB tactical attachment API, bridge queue, autonom opposition company AI, routed fallback/rally og ammunition exhaustion/resupply. Næste gate er lokal UE 5.8.3 compile/runtime QA og fejlrettelser før flere visuelle systemer.

**Unreal gameplay checkpoint v00.02.58:** Fire discipline/conservation, stance, directional cover, hasty fieldworks, black-powder smoke simulation, skirmishers, ammunition supply, doctrine/OrderAgg, autonomy, no-cheat AI difficulty, AI-DIAG telemetry, expanded officer model og mission constraints er implementeret som code core. Grafik er fortsat bevidst udskudt. Næste funktionelle hovedmål er artilleribatteriet ovenfor.

**Unreal gameplay checkpoint v00.02.59:** Første funktionelle artilleribatteri er nu implementeret som rigtig tactical unit: Artillery-echelon, configurable gun/crew/driver/horse state, timed limber/unlimber, horse/driver-limited towing, short manhandling, fire mission/manual target som standard, Hold Fire, optional Auto Target, traverse, LOS/contact/range, reload, fire resolution, Round/Shell/Shrapnel/Canister inventory/effects, shared tactical resupply, artillery-specific incoming damage, disabled/abandoned/capture/reuse samt OOB/outcome/regression integration. QA-fixturen har ét dansk 6-kanoners testbatteri under Division; seks kanoner er kun QA-data, ikke låst historisk organisation. UE 5.8.3 build/runtime QA og historisk ballistik/drill tuning udestår.

**Unreal gameplay checkpoint v00.02.60:** Artilleri er udvidet med visible ground/area fire, salvo-/tidsbegrænsede fire missions, target priority, auto ammo selection, reserve-conservation, deploy-slope, firing/manhandling fatigue, disabled-gun repair og emergency abandonment. Første fysiske supply-wagon/caisson er implementeret med separate small-arms/artillery cargo stores, drivers, horses, wagon condition, mobility, damage, abandonment/capture, proximity/state-gated transfer og artillery ammunition-family compatibility. QA bruger nu et ammo-lavt dansk batteri + fysisk ammunitionsvogn; Divisionens generiske 50k ammo-pulje er slået fra, når wagon-fixturen er aktiv. UE 5.8.3 build/runtime QA udestår.

**Unreal gameplay checkpoint v00.02.61:** Tactical terrain authority er implementeret som gameplay core: Hill/Ridge/Depression features, physical+analytic effective elevation, shared slope, crest/dead-ground, reverse slope/high-ground awareness, terrain-aware unit/ground LOS, elevation observation, terrain fire masking, route projection/sampling, shared artillery deploy slope og terrain-aware artillery position scoring/candidate search. QA har battery hill + central ridge + depression, units projiceres til terrain-Z, og regressionen kræver både dead-ground lane, clear lane, crest detection og gyldig artillery direct-fire position. Fysisk/visuel battlefield expansion er stadig separat; UE 5.8.3 build/runtime QA udestår.

**Unreal gameplay checkpoint v00.02.62:** Artillery projectile presentation er nu bundet til fire-missionens resolvede per-gun impactpunkter: ammo-specifik trajectory, Round Shot ricochet, Shell/Shrapnel/Canister presentation, virtual muzzle origins, bounded shot history, Follow Shot camera, impact hold/restore, P follow-toggle og F9 trajectory-debug. Projectile actors er presentation-only og kan aldrig skabe skade selv. QA validerer Round Shot/Shell arcs, ricochet og projectile component. Derudover er campaign→battlefield data-kontrakten etableret som foundation til senere generering fra campaign map.

**Unreal gameplay checkpoint v00.02.63:** Shared human visual architecture er implementeret som code core: fælles skeleton/animation-set contract, human animation intents, canonical animation manifests, uniform colour zones/presets/runtime overrides, dynamic material parameters, equipment sockets, rig compatibility, horse gait + rider sync, Dragoon Mount/Dismount intents samt artillery crew roles/stations og reload/push/traverse/limber/unlimber/repair animation states. QA validerer rig, manifests, farveoverride, cavalry sync og artillery crew stations. Final meshes/AnimBP/montages/material assets er fortsat senere grafikarbejde. Git sync-værktøjet er samtidig opdateret til v1.0.2 uden falsk NativeCommandError.

**Unreal gameplay checkpoint v00.02.68–v00.02.77 — 10 × 10 funktionelle blokke:** 100 nye gameplay-/simulationblokke er lagt oven på den eksisterende Unreal-core uden final grafik. Sekvensen dækker temporary specialist detachments med strength/ammo reconciliation, NCO command continuity, weapon/drill/doctrine-baseret firing drill og Kneeling stance, fysiske defensive positions med ownership/condition/capture, fortification assault equipment og breach-state, light/heavy mortar deployment med `TRANSPORT → EMPLACING → DEPLOYED → PACKING`, mortar unit/area/fortification fire, position occupancy og directional protection, working parties til ammunition/sårede/dig/breach/repair samt specialist-state capture/restore/validation. Disse blokke er **code implemented / UE 5.8.3 build+runtime QA pending**; de er ikke parity-verified endnu. Final meshes, animationer, Niagara og UMG-polish er fortsat separat senere grafikarbejde.

**Unreal gameplay checkpoint v00.02.79 — researched infantry firing drill:** Firing drill progression er ændret efter godkendt gameplay-beslutning. Campaign-start kan bevidst være simplere end den historiske 1864-standard: Front Rank Fire er baseline, hvorefter Two-Rank Fire, Fire by Rank, Controlled Volley, Independent Fire og Advanced Fire Drill oplåses gennem research/doctrine adoption. Oplåsning er separat fra unit skill; DrillTraining/FireDiscipline/NCO/loading method afgør fortsat udførelseskvalitet. Unreal-core har nu research-level gating, rank-aware firing fraction, FireByRank cycle/cadence, legacy-mode mapping, Advanced automatic-selection hook og snapshot/QA persistence. Den fulde campaign research/adoption owner samt rank-specifik 1:1 animation presentation er fortsat næste integrationstrin. **Code implemented / UE 5.8.3 build+runtime QA pending.**

**Unreal visual checkpoint v00.02.78 — Livgarden 1864 animated infantry:** Første brugerleverede skeletal infantry mesh, 65 FBX-animationer samt rifle/rifle+bajonet er koblet til en automatiseret Unreal import pipeline og et company-level runtime visual layer. 1:1 er default og 1:2/1:5/1:10 reducerer kun rendering ved at sample over hele formationens authoritative footprint. Gameplay strength, formation, fire/reload og casualties forbliver simulation authority. Kilde-preflight viser 41 fælles centrale Mixamo motion bones på tværs af body og alle clips; Unreal-import er den endelige kompatibilitetskontrol. **Source/runtime code implemented; UE 5.8.3 asset import + compile/runtime QA og weapon-hand offset tuning pending.**

## Statusdefinitioner

- **AKTIV** — implementeres/testes i den aktuelle build.
- **BESLUTTET** — fastlagt designretning; implementering ligger senere.
- **PLANLAGT** — konkret backlogpunkt med kendte afhængigheder, men designet kan stadig finjusteres.
- **IDÉ** — relevant mulighed, som skal undersøges eller besluttes før den bliver bindende design.
- **RESEARCH** — historiske detaljer skal verificeres før endelige værdier/organisation låses.

## Aktiv gate — P0A v00.00.08

| ID | Status | Emne | Exit-kriterium |
| --- | --- | --- | --- |
| B-001 | AKTIV | Weapon profile + experience-modificeret reload | Samme våben + højere experience giver lavere reload; våbentypen er stadig dominerende faktor. |
| B-002 | AKTIV | Salver kan give 0 hits | 0-hit salve reducerer ikke strength, men kan stadig give shock. |
| B-003 | AKTIV | `Ramte N` combat feedback | Positive resolved hits vises kort over målet. |
| B-004 | AKTIV | P0A regressionstest | Kamera, selection/orders, formationer, range, pause/1x/2x/3x, rout, victory/defeat og restart må ikke regressere. |
| B-005 | PLANLAGT | Reproducerbar Unity repository-baseline | Efter Play-test klassificeres scene, `.meta`, `packages-lock.json` og relevante `ProjectSettings` til permanent versionsstyring. |
| B-006 | AKTIV | Enkel casualty-visual | Første strength loss pr. regiment opretter én repræsentativ liggende casualty ved tabsstedet; maksimalt én figur pr. regiment i v00.00.08. |

## Næste aktive gameplay-build — P0A v00.00.09 Officer AI

**HØJESTE PRIORITET EFTER v00.00.08-GATEN.** Ingen anden større gameplay-feature må implementeres mellem v00.00.08 og den første testbare officer-AI/delegationsbuild.

Detaljeret scope ligger i:

- `docs/backlog/B-170-OFFICER-AI-DELEGATION.md`
- `docs/backlog/B-180-AI-DIFFICULTY-AND-V009.md`

v00.00.09 skal mindst bevise:

- `AI UNIT ON/OFF` på egne regimenter,
- prototype officer stats, der mærkbart påvirker reaction/valg,
- samme officer decision core for delegerede spillerformationer og fjenden,
- Easy/Normal/Hard som AI-beslutningsniveauer uden skjulte combat buffs eller omniscience,
- current AI task + reason code/telemetry,
- fuld regression af v00.00.08.

## Tactical combat — besluttet

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-010 | BESLUTTET | Ammunition som konkret beholdning | Enheder har faktisk ammunition pr. kompatibel våbenprofil; skydning forbruger ammunition og supply kan genforsyne. |
| B-011 | BESLUTTET | Unit-status | Battle UI skal kunne vise fx `566 mænd | Tab 30 (8 dræbte / 22 sårede) | Ammunition 18.420` samt morale/cohesion/fatigue/experience. |
| B-012 | BESLUTTET | Casualty pipeline | Hits splittes senere i killed, wounded, missing/captured m.m.; `Ramte N` er ikke lig med dræbte. |
| B-013 | IMPLEMENTERET CORE / UE QA PENDING | Fire discipline | Hold Fire, Fire at Will, volley/independent fire og ammunition conservation skal være separate ordrer/states. |
| B-014 | IMPLEMENTERET CORE / UE QA PENDING | Directional cover | Hegn, mure, grøfter, bygninger, skovkanter, terrænfolder og fieldworks beskytter kun fra relevante retninger. |
| B-015 | IMPLEMENTERET CORE / UE QA PENDING | Stance | Mindst standing og prone. Prone reducerer target profile, men påvirker movement/cohesion og reload afhængigt af loading method. |
| B-016 | IMPLEMENTERET CORE / UE QA PENDING | Hasty fieldworks | Almindeligt infanteri kan etablere simple skyttehuller/jordvolde/barrikader; ingeniører gør det hurtigere og bedre. |
| B-017 | IMPLEMENTERET CORE / UE QA PENDING | LOS og sortkrudtsrøg | Røg er simulation og skal påvirke LOS/accuracy; terræn/objekter må blokere ild. |
| B-018 | BESLUTTET | Formation-level pathfinding | Regimenter/bataljoner skal gå rundt om obstacles og bevare en brugbar formation uden individuel NavMesh-agent pr. soldat. |
| B-019 | BESLUTTET | Avanceret casualty visual senere | Den simple v00.00.08-figur udvides senere til flere repræsentative casualties og killed/wounded/medical collection, men authoritative casualty-state forbliver separat fra grafikken. |

## Skirmishers, marksmen og let infanteri

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-020 | IMPLEMENTERET CORE / UE QA PENDING | Deploy skirmishers | Infanteri skal kunne sende en del af styrken frem i løs skyttekæde foran eller på flanken af hovedformationen. |
| B-021 | IMPLEMENTERET CORE / UE QA PENDING | Skirmisher-roller | Screen, reconnaissance, harassment, contest cover, beskytte advance/retreat og skabe contact før hovedformationen. |
| B-022 | IMPLEMENTERET CORE / UE QA PENDING | Skirmisher combat model | Lavere formation density gør dem sværere at ramme; de har lavere samlet volley density, mere autonomi og højere afhængighed af cover/terrain. |
| B-023 | IMPLEMENTERET CORE / UE QA PENDING | Reform | Skirmishers skal kunne kaldes tilbage og reformere i moderformationen med tids-/cohesion-konsekvens. |
| B-024 | RESEARCH | Skarpskytter/marksmen | Udvalgte gode skytter kan være en mindre specialty/ability eller subunit; de skal ikke modelleres som moderne sniper teams uden historisk dokumentation. |
| B-025 | RESEARCH | Dansk organisation/terminologi | Fastlæg konkrete danske 1864-termer, andele og hvilke regimenter/jægertraditioner der havde særlige skirmisher-egenskaber. |

## Artilleri

> **UNREAL STATUS v00.02.60:** Artilleribatteri + første fysiske ammo-wagon/caisson er implementeret som funktionel core. Næste artilleri-gate er UE 5.8.3 runtime QA på udvidet battlefield/hills/dead-ground samt historisk tuning af gun profiles, drill og ammunition.


| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-030 | IMPLEMENTERET CORE / UE QA PENDING | Limber/unlimber | Kanoner skifter mellem transport- og skydestilling; klargøring tager tid. |
| B-031 | IMPLEMENTERET CORE / UE QA PENDING | Hestetrukket artilleri | Feltkanoner, limbers og caissons transporteres normalt med hestespand; antal/tilstand af heste påvirker mobility. |
| B-032 | IMPLEMENTERET CORE / UE QA PENDING | Manhandling | Crew kan skubbe/trække et unlimbered stykke korte afstande uden heste. Det er langsomt, terrænafhængigt og giver fatigue; tunge stykker begrænses mere end lette. |
| B-033 | IMPLEMENTERET CORE / UE QA PENDING | Artilleriets crew/heste | Crew, gunners, drivers og horses er rigtige state-værdier og kan lide tab. Manglende heste kan gøre et ellers intakt batteri taktisk immobilt. |
| B-034 | IMPLEMENTERET CORE / UE QA PENDING | Erobring af kanoner | Guns kan være operational, abandoned, disabled/destroyed eller captured; capture kræver fysisk kontrol. |
| B-035 | IMPLEMENTERET CORE / UE QA PENDING | Genbrug af erobret artilleri | In-battle reuse kræver egnet crew, ammunition, træning og klargøringstid; ellers bjærges stykket efter slaget. |
| B-036 | IMPLEMENTERET PROTOTYPE / HISTORISK TUNING PENDING | Ammunitionstyper | Round shot/shell/shrapnel/canister m.m. får forskellige effekter mod open, prone, cover og fieldworks. |
| B-037 | IMPLEMENTERET CORE / UE QA PENDING | Artilleri fire control | **Manuel måludpegning er standard for spillerstyret artilleri.** Spilleren vælger mål/område, hvorefter batteriet fortsætter efter ordren, indtil mål/ordre ændres eller ikke længere kan udføres. Automatisk målvalg er en separat mode, som spilleren aktivt slår til. AI-kontrollerede batterier kan bruge automatisk målvalg gennem commander AI. |
| B-038 | IMPLEMENTERET PROTOTYPE / DOCTRINE-TUNING PENDING | Auto-target prioritering | Automatisk artilleriild skal vælge mål efter synlighed/LOS, range, trussel, formation density, target type, ammunitionstype, commander/fire-control doctrine og evt. ammunition conservation — ikke blot nærmeste fjende. |
| B-039 | IMPLEMENTERET CORE / UE QA PENDING | Hold Fire for artilleri | Et batteri skal kunne være deployed og klar uden at skyde. Hold Fire overstyrer både manuelt og automatisk målvalg. |
| B-039A | BESLUTTET / RESEARCH TUNING | Mortarer | Mortarer er artillery support/siege capability med høj krum bane, særskilt ammo/crew/reload og area-fire. De kan bekæmpe skanser, løbegrave, reverse-slope/dead-ground og mål bag brystværn, hvor direct-fire guns ikke har normal skudlinje. |
| B-039B | BESLUTTET | Mortar fire mission | Mortarer skyder primært mod observeret område/fortification via indirect fire. Spotter/observation, dispersion, ammunition conservation og correction between salvos skal indgå; fri omniscient targeting er ikke tilladt. |
| B-039C | BESLUTTET | Siege artillery grouping | Mortarer og tunge belejringsstykker organiseres som støtte under højere HQ/siege artillery group. Vi undgår én permanent OOB-enhedstype pr. kaliber og bruger data-driven weapon profiles. |
| B-039D | RESEARCH | Historiske mortar-profiler 1864 | Fastlæg preussiske/danske typer, kalibre, ammunition, antal, deploy/reload, range, accuracy/dispersion og organisation. Dybbøl-dokumentation bekræfter preussiske morterer og nævner bl.a. 16 stk. 25-pundige morterer. |

## Kavaleri og dragoner

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-040 | BESLUTTET | Dragoner mounted/dismounted | Dragoner kan sidde af og føre ildkamp som fodtropper samt senere remount. |
| B-041 | BESLUTTET | Horse-holder state | Heste bliver samlet bag/ved enheden under afsiddet kamp; remount tager tid, og heste/holdere kan lide tab. |
| B-042 | BESLUTTET | Kavaleri roller | Reconnaissance, screening, courier support, flank security og pursuit er mindst lige så centrale som charge. |
| B-043 | BESLUTTET | Horse fatigue/forage/casualties | Hestebestanden påvirker både taktisk og strategisk mobilitet. |

## Supply, capture og battlefield salvage

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-050 | IMPLEMENTERET CORE / UE QA PENDING | Supply-vogne | Fysiske enheder med inventory, vogn, trækdyr og crew/driver. Kan bære ammunition, mad og andre forsyninger. |
| B-051 | IMPLEMENTERET CORE / UE QA PENDING | Capture/destruction | Supply-vogne kan erobres, forlades eller ødelægges; crew/heste og last kan gå tabt helt eller delvist. |
| B-052 | BESLUTTET | Battlefield salvage | Våben og materiel fra overgivne, faldne, forladte guns/limbers og wagons kan indsamles efter slaget. |
| B-053 | DELVIST IMPLEMENTERET / AFTERMATH RECOVERY PENDING | Damage/recovery percentage | Ikke alt materiel kan reddes; en dokumenteret andel er ødelagt, mistet eller kræver repair. |
| B-054 | IMPLEMENTERET ARTILLERY-AMMO CORE / HISTORICAL DATA PENDING | Compatibility | Erobrede våben/ammunition kan kun bruges, hvis ammunition, træning og condition tillader det. |

## Command, officers og organisation

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-060 | BESLUTTET | Bataljoner som underformationer | Regimenter skal senere kunne bestå af operative bataljoner/underformationer. |
| B-061 | IMPLEMENTERET CORE / UE QA PENDING | Order delay | Ordrer har transmission, delivery og acknowledgement; ingen telepatisk kontrol. |
| B-062 | IMPLEMENTERET CORE / UE QA PENDING | Commander AI | Officer skill, personality, initiative og commander intent påvirker lokal udførelse. |
| B-063 | BESLUTTET | Persistent officers | Officerer har historik, performance, casualties, succession og reputation. |
| B-064 | DELVIST IMPLEMENTERET / UE QA PENDING | Training/readiness/experience adskilt | Experience, training, morale, cohesion, fatigue og readiness må ikke være én samlet magisk stat. |
| B-065 | IMPLEMENTERET CORE / UE QA PENDING | Commander autonomy | Strict/Normal/Independent autonomy påvirker, hvor meget en underordnet officer må ændre lokal udførelse inden for intent. |
| B-066 | BESLUTTET | Order lifecycle | Drafted → Sent → In transit → Delivered → Acknowledged → Executing → Superseded/Failed er eksplicit state. |
| B-067 | BESLUTTET | HQ-perspektiv | Spilleren ser i høj realisme det, eget HQ ved; “ordre sendt” er ikke det samme som “ordre forstået”. Assistanceindstillinger kan reducere friktion. |

## Battle UI/UX

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-070 | BESLUTTET | Kontekstuel bundmenu | Når en enhed vælges, kommer en lav command bar frem nederst i skærmen. Den skjules igen, når intet relevant er valgt. |
| B-071 | BESLUTTET | Unit-type-specific actions | Menuens knapper afhænger af enhedstype og state: infanteri, skirmishers, artilleri, dragoner, supply, ingeniører osv. |
| B-072 | BESLUTTET | Status i bundmenu | Samme panel viser nøgledata som mandskab, casualties, ammo, weapon/reload, morale/cohesion/fatigue/experience og aktiv ordre. |
| B-073 | PLANLAGT | Infanteriknapper | Formation, Hold/Move/Attack, Hold Fire/Fire at Will, Deploy/Reform Skirmishers, Standing/Prone, Use Cover, Dig In, Range overlay. |
| B-074 | PLANLAGT | Artilleriknapper | Limber/Unlimber, Move, Manhandle, **Select Target (standard/manual)**, Auto Target on/off, Hold Fire, ammunitionstype og evt. abandon/capture context. |
| B-075 | PLANLAGT | Dragon-knapper | Mounted/Dismounted, remount, screen/recon, charge/pursuit og fire control efter type. |
| B-076 | PLANLAGT | Multi-select UI | Fælles kompatible ordrer vises ved flere valgte enheder; blandede typer må ikke få ugyldige fælles commands. |
| B-077 | IDÉ | Layout/iconografi | Endelig grupperingsrækkefølge, ikoner, tooltips, hotkeys og om advanced actions ligger i submenus skal prototypetestes. |
| B-078 | BESLUTTET | Aktiv fire-control-state synlig | Bundmenu/formation card skal tydeligt vise fx `MANUELT MÅL`, `AUTO TARGET` eller `HOLD FIRE`, så artilleriets adfærd aldrig er skjult for spilleren. |

## Operationel bevægelse, kontakt og fog of war

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-080 | BESLUTTET | Road-aware march | Strategisk marchhastighed afhænger af vej, terræn, formationstype, fatigue, weather, transport og congestion. |
| B-081 | BESLUTTET | Forced march | Hurtigere march købes med fatigue, stragglers, hestetab og dårligere battle readiness. |
| B-082 | BESLUTTET | March columns og congestion | Store formationer fylder fysisk på veje; smalle broer/chokepoints kan skabe kø og forsinkelser. |
| B-083 | BESLUTTET | Encounter/contact model | Contact kan udvikle sig til spotting, skirmish, withdrawal, deployment eller tactical battle afhængigt af orders, information og AI. |
| B-084 | BESLUTTET | Knowledge-state fog of war | Fjendtlige observationer har timestamp, confidence, last known position og strength/type estimates. AI må kun bruge sin egen knowledge state. |
| B-085 | BESLUTTET | Recon sources | Cavalry, scouts/skirmishers, observationspunkter, civilians, telegraph reports og naval spotting bidrager forskelligt til intel. |
| B-086 | IDÉ | Deception/demonstrations | Informationsmodellen skal kunne understøtte vildledning og demonstrations senere; konkrete mechanics fastlægges senere. |
| B-087 | BESLUTTET | CAV `SPEJD HER` tactical recon order | Når true FOG/LOS er aktiv, kan mounted cavalry få et område-/punktmål til SEEK/RECON → CONTACT → SCREEN. CAV holder våbenafhængig stand-off, rapporterer observationer via CurrentCommandParent/command delay og skaber last-known contacts; ordren giver ikke automatisk CHARGE-authority og er skjult/disabled uden FOG. |

## Strategisk logistik og forsyning

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-090 | BESLUTTET | Supply chain | Depot → rail/ship/road → field depot/train → unit er den grundlæggende forsyningskæde. |
| B-091 | BESLUTTET | Throughput og interdiction | Hver node/rute har kapacitet og risiko; fjenden kan afskære supply uden at erobre en hel provins. |
| B-092 | BESLUTTET | Food/forage/ammo/medical/transport | Disse er adskilte logistiske behov og påvirker forskellige dele af kampkraften. |
| B-093 | BESLUTTET | Local forage | Enheder kan delvist leve af lokal forage afhængigt af region, årstid, politisk kontrol og tidligere forbrug. |
| B-094 | BESLUTTET | Infrastruktur som objectives | Broer, stationer, havne, depoter og vejknudepunkter bliver reelle operationelle mål pga. supply/throughput. |

## Persistence, sanitet, POW og regimentshistorik

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-100 | BESLUTTET | Persistent wounded | Wounded har severity/recovery og kan returnere, blive invalid eller dø senere; battle end må ikke konvertere dem direkte til permanent manpower loss. |
| B-101 | BESLUTTET | Medical service | Battlefield collection, ambulance/transport, casualty stations, field hospitals, doctors, supplies og capacity påvirker outcomes. |
| B-102 | BESLUTTET | POW/missing | Missing kan senere resolve til POW, hospitalised, returned eller killed. POW er persistent personelstate. |
| B-103 | BESLUTTET | Regimental lineage | Navngivne enheder bevarer lineage, commanders, battles, casualties, reorganizations og campaign chronicle. |
| B-104 | BESLUTTET | Colours/standards | Faner er data + fysisk 3D-object med colour guard, capture/loss, damage og battle honours. |
| B-105 | BESLUTTET | Officer service records | Promotions, wounds, captivity, commands, battles, decorations, retirement/death gemmes persistent. |
| B-106 | BESLUTTET | Earned traits | Unit traits optjenes gennem training/experience/historik og kan fortyndes ved massive replacements. |
| B-107 | BESLUTTET | Formation specialisations | Brigader/divisioner/korps kan få organisatoriske capabilities som ambulance service, pontoon train, sappers, recon/logistics/artillery specialisations, forankret i faktiske assets/personel. |

## National udvikling, økonomi, mobilisering og træning

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-110 | BESLUTTET | Population/labour | Mobilisering og krig trækker på samme regionale befolknings-/arbejdskraftbase som civil økonomi. |
| B-111 | BESLUTTET | Agriculture/food/horses | Gårde, food/grain, livestock/horses og forage er den primære førindustrielle forsyningsbase. |
| B-112 | BESLUTTET | Resources/extraction | Kul, jern, træ og building materials er geografiske ressourcer med workforce/infrastructure constraints. |
| B-113 | BESLUTTET | Cities/regional development | Byer er økonomiske, administrative og logistiske knudepunkter med ports, arsenals, hospitals, education og transport. |
| B-114 | BESLUTTET | Industry/production chains | Workshops/factories omsætter konkrete inputs til varer; mangel på inputs skal kunne stoppe produktion. |
| B-115 | BESLUTTET | Military production/stockpiles | Rifles, artillery, ammunition, uniforms og transportmateriel produceres/importeres/bjærges og eksisterer i lagre. |
| B-116 | BESLUTTET | Trade/foreign procurement | Import/eksport, contracts, tariffs og blockade påvirker priser, lagre og adgang til våben/råvarer. |
| B-117 | BESLUTTET | Research vs adoption | En teknologi kan være kendt uden at være masseproduceret; tooling, doctrine, training og unit-level adoption er separate trin. |
| B-118 | BESLUTTET | Budget/debt | Taxes, tariffs, loans, interest, war finance og military/civil priorities giver langsigtede trade-offs. |
| B-119 | BESLUTTET | Recruitment/mobilisation | Nation-specifik volunteer/conscription/reserve/cadre-proces; ingen universel instant draft-knap. |
| B-120 | BESLUTTET | Training/readiness/experience | Drill, marksmanship, fire discipline, fieldcraft m.m. er træningsdimensioner; readiness og combat experience er separate states. |
| B-121 | BESLUTTET | Leader development | Academies, staff courses, assignments, mentorship, manoeuvres og promotion påvirker officerskorpsets kvalitet over tid. |

## Teknisk arkitektur og persistence-regler

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-130 | BESLUTTET | Én authoritative simulation state | Tactical og strategic lag bruger samme unit-identiteter/state; visuelle GameObjects er ikke authoritative data. |
| B-131 | BESLUTTET | Stable IDs | Nationer, officers, formations, locations og equipment bruger stabile IDs, så save/load og tactical transfer ikke duplikerer entities. |
| B-132 | BESLUTTET | Save/version migration | Savegames skal versionsstyres og kunne migreres robust mellem compatible builds. |
| B-133 | BESLUTTET | Replay/debug event log | Deterministic seed/event log skal understøtte regression, replay/debug af større hændelser. |
| B-134 | BESLUTTET | Scalable rendering | Standard ca. 1:10, men render ratio 1:5/1:10/1:20/1:50 kan skaleres efter hardware uden at ændre simulationen. |
| B-135 | BESLUTTET | Formation-level performance | Visual soldiers er primært slots/animation/local avoidance; combat og movement ligger på formationsniveau. |
| B-136 | BESLUTTET | Data-driven/modding | OOB, officers, units, weapons, events, localisation og scenarios externaliseres så langt som praktisk uden core recompilation. |
| B-137 | BESLUTTET | Automated/headless tests | Simulation state transitions, serialization og centrale regressions skal kunne testes uden fuld 3D-scene. |

## Flåde, diplomati og politisk krig

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-140 | BESLUTTET | Naval operations | Blockade, escort, transport, patrol, interception, bombardment og amphibious support er strategisk/operationelt scope. |
| B-141 | BESLUTTET | Troop transport | Skibe, embark/disembark time, weather, port/destination capacity og enemy naval presence påvirker transport. |
| B-142 | PLANLAGT | Detailed naval combat | Arkitekturen reserverer hook til mere detaljeret/3D naval combat; det er ikke nødvendigt for den første landkrigs-vertical slice. |
| B-150 | BESLUTTET | War goals/peace | Kampagnen skal kunne slutte gennem krigsmål/forhandling og internationalt pres, ikke kun total conquest. |
| B-151 | BESLUTTET | Historical event engine | Events trigges af faktisk simulation state og dato/conditions og skal understøtte alternative outcomes. |
| B-152 | BESLUTTET | Sverige-Norge/intervention framework | International intervention skal være systemisk mulig og ikke kun en scripted cutscene. |

## Tactical specialisation, fieldworks og siege systems

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-153 | AKTIV | Unit / detachment / capability model | Permanente OOB-enheder holdes relativt få. Capabilities kan oprette midlertidige skirmisher-, marksman/sharpshooter-, pioneer/working-, ammunition-, stretcher- og picket/scout-detachments. `OrganicParent` bevares; detached strength/ammo må ikke duplikeres ved reform/reattach. |
| B-154 | AKTIV | NCO command continuity | Underformationer har mindst `NCOStrength` og `NCOQuality`. Ved officer unavailable/disrupted kan NCO-kadren fortsætte lokal kommando med reduceret kvalitet; NCO-state påvirker reform, reload discipline, rally, response delay og detachment control. |
| B-155 | AKTIV | Defensive position object model | Rifle pits, breastworks, trenches, gun emplacements, barricades, abatis/obstacles og redoubts/skanser er fysiske objekter med orientation, footprint, construction progress, condition, capacity, cover og ownership/occupier. De kan beskadiges, repareres, forlades, besættes og erobres. |
| B-156 | AKTIV | Fortification assault equipment | Pioneer-/working parties kan bruge stiger, planker/stormbrædder, økser, brækstænger og sprængmidler. Udstyr og working-party strength påvirker preparation, breach time, exposure og passage gennem hindringer. |
| B-157 | AKTIV | Firing drill, geledder og stance | Godkendt v00.02.79: bevidst gameplay-progression **Front Rank Fire → Two-Rank Fire → Fire by Rank → Controlled Volley → Independent Fire → Advanced Fire Drill**. Research/adoption oplåser drills; DrillTraining, FireDiscipline, NCO, loading method, stance og formation afgør udførelseskvalitet. Gamle drills forbliver valgbare efter senere unlocks. Geledder bytter ikke fysisk plads ved Fire by Rank. Unreal unlock/rank-cycle core er implementeret; campaign research-owner og rank-specifik 1:1 animation presentation afventer. |
| B-158 | AKTIV | Mortar classes | Morterer opdeles mindst i `LightHand` og `HeavySiege`. Krum ild, dispersion, ammunition, mobility og tactical role kommer fra weapon/equipment data. |
| B-159 | AKTIV | Heavy mortar transport/deployment | Tunge morterer bruger `TRANSPORT → EMPLACING → DEPLOYED → PACKING → TRANSPORT` med working party, wagons/horses, piece weight og terrain preparation som konkrete state-faktorer. B-153–B-159 har Unreal code core i v00.02.68–v00.02.77 og afventer UE 5.8.3 compile/runtime QA. |
| B-160 | AKTIV | Livgarden 1864 real 3D infantry visual | Brugerleveret skeletal mesh + shared skeleton animations + rifle/rifle+bajonet importeres til canonical Unreal paths. Company visual renderer følger authoritative strength og formation slots, bruger 1:1 som standard og må kun skalere rendering ved 1:2/1:5/1:10. Første QA-company aktiverer laget; import/build/runtime/weapon-offset QA udestår. |

## Åbne designspørgsmål fra tidligere baseline

| ID | Status | Emne | Spørgsmål / nuværende retning |
| --- | --- | --- | --- |
| I-010 | IDÉ | Kampagnestart | Januar 1864, 1863-opbygning eller længere 1848–1871 grand campaign som slutmål? |
| I-011 | IDÉ | Laveste direkte command level | Nuværende retning: bataljon som laveste normale direkte niveau; kompagnier simuleres mest automatisk. |
| I-012 | BESLUTTET | Tactical pause | Singleplayer tactical battles kan pauses; det matcher command/grand-strategy-stilen. |
| I-013 | IDÉ | HQ realism | Assistanceindstillinger kan reducere order/information delay; højere realisme bruger HQ-perspektivet fuldt. |
| I-014 | IDÉ | Economic granularity | War-support economy frem for fuld Victoria-lignende markedssimulation; konkret varegranularitet skal tunes. |
| I-015 | BESLUTTET | Hybrid battle maps | Historiske nøgleslag håndverificeres; øvrige encounters kan data-/proceduralt genereres. |
| I-016 | IDÉ | Multiplayer | Ikke arkitekturdriver i første fase; core state bør dog undgå unødigt singleplayer-hardcode. |

## Øvrige åbne idéer / research

| ID | Status | Emne | Spørgsmål |
| --- | --- | --- | --- |
| I-001 | IDÉ | Marksmen target priority | Skal udvalgte skytter kunne prioritere officerer/gunners, og hvordan begrænses dette af observation/FoW og historisk praksis? |
| I-002 | IDÉ | Skirmisher detached strength | Fast procent, company-based detachments eller doctrine/OOB-data pr. nation/periode? |
| I-003 | IDÉ | In-battle salvage | Skal små mængder ammunition/våben kunne samles under selve slaget, eller primært i aftermath? |
| I-004 | IDÉ | Captured gun immediate use | Hvor ofte bør et erobret stykke realistisk kunne vendes mod fjenden under samme slag? |
| I-005 | IDÉ | Command bar density | Hvor mange primære actions kan vises uden at gøre bundmenuen til et cockpit? Testes med prototype-UI. |
| I-006 | IDÉ | Auto-artillery doctrine | Skal Auto Target have spillerdefinerede priorities som `Counter-battery`, `Infantry`, `Closest threat`, `Conserve ammo`, eller primært styres af battery commander/ordre? |
| I-007 | RESEARCH | Historiske artillery drills | Endelige limber/unlimber-, movement-, crew- og rate-of-fire-værdier fastlægges pr. nation, gun type og periode via research. |
| I-008 | RESEARCH | Historiske weapon/OOB-data | Endelige weapon models, regiment equipment, strengths og organisation er date-valid data med provenance, ikke hardcodede prototypeantagelser. |

## Arbejdsregel

Når en ny idé opstår:

1. Hvis den er besluttet, opdateres relevant designmanual/feature-arkitektur og registreres her som **BESLUTTET** eller **PLANLAGT**.
2. Hvis den endnu ikke er afgjort, registreres den som **IDÉ** eller **RESEARCH**.
3. Når implementeringen starter, skifter den til **AKTIV** med konkrete exit-/testkriterier.
4. Når funktionen er implementeret og QA-valideret, flyttes den til versionshistorik/implementeringsstatus og backlogpunktet markeres afsluttet i en senere backlog-revision.
5. Nye chatsamtaler om PROJECT 1864 skal konsolideres her og i relevante designafsnit, så beslutninger ikke kun eksisterer i samtalehistorikken.


## B-290 — Battlefield Expansion + Hills before Artillery

**Status:** DELVIST IMPLEMENTERET CORE / UE QA + FYSISK MAP-EXPANSION PENDING  
**Prioritet:** Høj terrain authority er implementeret; fysisk/visuel battlefield expansion følger senere

### Beslutning
Før artilleri udbygges til fuld gameplay-test skal tactical battlefield udvides og have tydelige højdedrag/bakker, så kanoner kan testes fra højere stillinger og med reelle skudfelter.

### Retning
- større tactical map end den nuværende QA-flade;
- mindst 1–2 større højdedrag og flere bløde terrænbølger;
- eksisterende river/bridge beholdes som combined-arms testelement;
- åbne marker, veje og skov-/coverzoner skal stadig være læselige;
- artilleripositioner på højder skal kunne testes mod dead ground og skjult approach;
- senere terrain/LOS-pass skal håndtere crest, dead ground, slope og deployability;
- artilleri må senere ikke deployere på for stejle skråninger eller i ulovligt terrain.

### Implementeringsstatus v00.02.61
- Terrain + LOS gameplay baseline: **IMPLEMENTERET CORE**.
- Crest/dead ground/reverse slope/high-ground observation: **IMPLEMENTERET CORE**.
- Artillery deployability + position scoring på terrain: **IMPLEMENTERET CORE**.
- QA-hill/ridge/depression fixtures: **IMPLEMENTERET QA CORE**.
- Større fysisk battlefield, endelige hills/landscape assets, roads/fields/forest visual pass: **PENDING**.
- Lokal UE 5.8.3 compile/runtime test: **PENDING**.


## B-291 — Campaign Map → Generated Tactical Battlefield

**Status:** BESLUTTET / DATA-KONTRAKT IMPLEMENTERET / GENERATOR PENDING  
**Prioritet:** Høj før campaign→battle vertical slice

### Beslutning
Et tactical battlefield skal senere **genereres ud fra det sted på campaign map, hvor slaget opstår**. Tactical map må ikke være et løsrevet tilfældigt level, hvis campaign map allerede indeholder relevant geografi.

### Campaign-data der skal kunne føres ind
- battle center / campaign koordinat;
- deterministic generation seed;
- attacker approach direction;
- defender approach direction;
- campaign height/elevation;
- hills, ridges og depressions;
- rivers/water;
- roads;
- bridges;
- settlements/byer;
- forest;
- fields/agricultural land;
- marsh/wet ground;
- feature importance/scale;
- source feature IDs, så tactical features kan spores tilbage til campaign map.

### Genereringsregler
- tactical battlefield skal være et lokalt udsnit omkring battle center;
- overordnet højdeprofil og større terrænformer skal bevares;
- en campaign-flod skal fortsætte som samme topologiske barriere i tactical battle;
- campaign-veje og broer skal lande på meningsfulde tactical forbindelser;
- større byer/landsbyer skal placeres i korrekt relativ retning og afstand;
- attacker/defender deployment edges skal afspejle deres campaign approach;
- samme battle request + seed skal generere samme gameplay-terrain;
- tactical simplification er tilladt, men må ikke vende eller flytte afgørende geografi vilkårligt;
- den eksisterende tactical terrain authority (v00.02.61) skal være output-/runtime-laget for generated terrain;
- final landscape/meshes/materialer skal være presentation oven på samme generated gameplay data.

### Foundation v00.02.62
`StrategyBattlefieldGenerationTypes.h` definerer nu request/result + source-feature dataformat for HeightSample/Hill/Ridge/Depression/River/Road/Bridge/Settlement/Forest/Field/Marsh/Water. **Selve generatoren er ikke implementeret endnu.**

### Exit-kriterium
Et slag, som opstår ved samme position på campaign map, skal deterministisk kunne starte et tactical battlefield med genkendelig lokal topografi, veje, vandløb, broer, land cover og korrekte attacker/defender approach-retninger.


## B-300 — Infantry Visual Fidelity + Soldier Silhouette Polish

**Status:** BESLUTTET / NÆSTE VISUAL-SPOR  
**Prioritet:** Mellem/høj efter F30M command-chain QA

### Beslutning
1:1 infantry skal have et målrettet visual-polish pass tilsvarende cavalry F30L. De nuværende soldater læses stadig for meget som primitive blokfigurer ved tactical zoom.

### Retning
- bedre torso-/skulder-/benproportioner;
- tydeligere hoved/shako/hovedbeklædning;
- mere naturlig arm- og geværpose;
- tydeligere krydsremme, krave/manchetter og uniformslag;
- små deterministic variationer uden at bryde formation-readability;
- marching/firing/hold silhouettes skal kunne skelnes på afstand;
- visual LOD må aldrig ændre 1:1 simulated strength eller formation slots;
- performance skal måles med fulde 190-mands kompagnier.

### Exit-kriterium
Ved normal tactical zoom skal en enkelt figur tydeligt læses som en bevæbnet infanterist og ikke som en abstrakt søjle/blok, mens 190-mands 1:1 formationer fortsat kører acceptabelt.


## B-292 — Uniform colour customization

**Status:** IMPLEMENTERET DATA/MATERIAL CORE / FINAL MATERIAL ASSETS PENDING  
**Prioritet:** Høj når soldier visual pipeline bygges

### Krav
- Soldater deler fælles human skeleton og animationer.
- Farveændringer må ske via material instances / runtime material parameters, ikke via separate animation assets.
- Minimum parametre hvor mesh/material tillader det:
  - coat/tunic;
  - trousers;
  - collar/facings/cuffs;
  - headgear detail;
  - leather equipment;
  - regiment/unit accent.
- Historical presets kan begrænse eller låse farvevalg pr. nation/regiment.
- Samme soldier mesh kan genbruges med flere farvevarianter uden at duplikere animation blueprint eller animation clips.
- Future regiment/unit data skal kunne pege på et uniform preset og overrides.


## B-293 — Shared human animation architecture

**Status:** IMPLEMENTERET CORE / FINAL ANIMATION ASSETS PENDING  
**Prioritet:** Høj ved næste character graphics pass

### Fast arkitektur
- Kompatible menneskelige modeller bruger samme `SK_Human_1864` skeleton.
- Fælles animation set/Animation Blueprint contract er `ABP_Human_1864`.
- Walk/Run/Aim/Fire/Reload/Die/Mount/Dismount og øvrige human animations laves én gang og genbruges på tværs af uniformer/nationer.
- Uniform mesh/materiale må ikke eje gameplay animation authority.
- Rifle, bayonet, sabre, artillery tools og øvrigt udstyr placeres via fælles sockets.
- Cavalry rider og horse er separate skeletal meshes; rider gait synkroniseres til horse gait.
- Artillery crew bruger samme human skeleton, men har specialiserede crew-role montages.
- Battery gameplay state driver Push/Traverse/Reload/Limber/Unlimber/Repair animation intent.
- Animation må aldrig selv afgøre, om gameplay-state er loaded/deployed/fired/mounted.
- Rig compatibility valideres mod shared skeleton + animation set.
- Canonical animation manifest findes i `StrategyAnimationManifestLibrary`.

### Exit-kriterium for asset-pass
Mindst én dansk og én preussisk human mesh samt artillery crew og cavalry rider skal kunne skifte mesh/uniformpreset uden at duplikere locomotion/fire/reload animationer.


## B-294 — Unreal QA startup visibility

**Status:** IMPLEMENTERET QA CORE / UE 5.8.3 RUNTIME VERIFICATION PENDING  
**Prioritet:** Høj compile/runtime gate

### Problem
Unreal-projektet kan åbne på en `Untitled` editor-world, fordi repoet endnu ikke indeholder et committed tactical map asset. Det gjorde den kodegenererede battle-slice vanskelig at opdage visuelt.

### Implementeret v00.02.64
- OOB QA-scenariet oprettes fortsat autoritativt ved Play/PIE.
- Strategy-kameraet fokuseres automatisk omkring battlefield-centret.
- Danmark/Prussia/andre sider får tydelige runtime QA-farver.
- Alle units får synlige debug-boxes, facing arrows og labels.
- River banks + bridge crossing visualiseres.
- Navigation obstacle visualiseres.
- Hill/Ridge/Depression fixtures får footprint og højde-markør.
- QA-overlay er presentation-only og må ikke påvirke gameplay authority.

### Exit-kriterium
Efter clean sync + BUILD PASS skal et tryk på **Play** vise den samlede QA battlefield-slice uden krav om eksterne meshes/materials/Blender-assets.


## B-294 — Unreal QA runtime visibility

**Status:** IMPLEMENTERET CORE / UE 5.8.3 RUNTIME QA PENDING  
**Prioritet:** P0 testability

- QA battle must be visible and startable from an Untitled/empty editor level.
- Runtime bootstrap owns fallback scenario/camera creation independent of manual map wiring.
- Placeholder boxes/arrows/labels are temporary presentation until final 3D assets exist.
- River, bridge, obstacle and analytic terrain fixtures must remain visibly inspectable during QA.


## B-295 — Flat Unreal QA battlefield

**Status:** IMPLEMENTERET CORE / UE 5.8.3 RUNTIME QA PENDING  
**Prioritet:** P0 testability

- Default tactical QA map is a flat physical 600 × 600 m WorldStatic surface.
- Hill/Ridge/Depression fixtures are disabled by default and remain opt-in terrain QA.
- River/bridge, OOB, formations, movement, cavalry, artillery, supply and enemy AI remain testable.
- Final soldier/horse/artillery models are deliberately postponed to the later asset pass.