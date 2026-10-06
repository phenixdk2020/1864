# PROJECT 1864 — Designmanual

**Aktuel designbaseline: v00.02.79**  
**Aktuel prototype-workbranch: P0A v00.00.09f30x EARLY INFANTRY DEPLOY + COMPANY SPACING + CAV BRIDGE APPROACH + STARTUP ENEMY CONES TEST**

**Unreal runtime QA visibility bootstrap (v00.02.64):** Unreal-porten har endnu ingen committed `.umap`/`.uasset` battlefield assets, så editoren kan fortsat åbne på en `Untitled` world. Ved **Play/PIE** auto-spawner `AStrategyGameMode` det eksisterende OOB QA-scenarie og fokuserer nu strategy-kameraet på battlefield-centret. `AStrategyOOBTestScenario` tegner et kontinuerligt runtime QA-overlay med tydelige sidefarvede unit-boxes og facing arrows, labels, river banks + bridge crossing, navigation obstacle samt hill/ridge/depression footprints og højde-markører. Dette lag er presentation/debug-only og ændrer ingen gameplay authority. Det er bevidst asset-uafhængigt, så battle-kernen kan testes før de endelige soldier/horse/artillery meshes og animation assets er produceret.

Grand Strategy i realtid + taktiske 3D-slag. Denne GitHub-udgave er opdelt i dele for overskuelig versionsstyring. Den layoutede Word-master opdateres parallelt som projektartefakt, mens GitHub-Markdown er den løbende designmæssige source of truth.

Projektets centrale intake-log for besluttede men endnu ikke implementerede funktioner, planlagte opgaver, research-emner og løse idéer ligger i [PROJECT-BACKLOG.md](PROJECT-BACKLOG.md). Større emner kan have detaljerede backlog-supplementer, som senere konsolideres ind i hovedbackloggen.

## Engine-port beslutning — Unreal Engine

Projektet har fra 2026-09-27 et parallelt Unreal Engine-portspor på branchen `unreal-port`. Unity `channel-test` forbliver den operative reference for den eksisterende battle-prototype, indtil Unreal-versionen har opnået dokumenteret feature/QA-paritet. Porten er en implementeringsændring, ikke et redesign: eksisterende command hierarchy, order authority, formationsregler, LOS/range/cones, river/bridge-routing, cavalry behaviour, OOB/HUD-semantik og historiske data skal bevares. Permanent Unreal-kode skal bruge stabile domænenavne frem for Unity-prototypens revisionssuffixer. Se [UNREAL-MIGRATION-PLAN.md](UNREAL-MIGRATION-PLAN.md) og [UNREAL-PORTING-INVENTORY.md](UNREAL-PORTING-INVENTORY.md).

**Uniform customization rule:** Human soldier meshes must support data-driven uniform colour variation without requiring duplicate animations or separate skeletons. All compatible soldier meshes use the shared human skeleton/animation set, while uniform appearance is controlled through material instances/dynamic material parameters. At minimum, Tunic/Coat, Trousers, Facings/Cuffs/Collar, Headgear detail, Leather equipment and optional Regiment/Unit accent colour must be independently addressable where the source mesh/material supports it. Historical presets may lock or constrain allowed colours per nation/regiment, but the technical architecture must allow colour changes at runtime/editor level. Colour variation must never require a duplicated walk/fire/reload/mount/dismount animation set.

**Flat Unreal QA battlefield (v00.02.66):** Den operative Unreal QA-slice bruger nu som standard et fysisk fladt 600 × 600 m runtime battlefield baseret på Engine BasicShapes cube + WorldGridMaterial. Topfladen ligger på Z=0 og har WorldStatic collision, så movement/selection/terrain traces kan testes uden et manuelt level asset. Hill/Ridge/Depression QA-fixtures er slået fra som standard (`bSpawnTerrainQA=false`) men kan aktiveres igen separat til terrain/LOS-artilleri-tests. Flod/bro, navigation obstacle, OOB, cavalry, artillery, supply og enemy QA-enheder kan fortsat testes på den flade flade. Soldier/horse/artillery production meshes og animation assets er eksplicit udsat til senere asset-pass.

**Unreal physical QA placeholders (v00.02.65):** Alle `AStrategyUnit`-aktører har nu en asset-uafhængig fysisk QA-placeholder baseret på Unreal Engine BasicShapes cube mesh. Placeholderen skaleres efter echelon/type: company, Battalion/Major, Regiment, Brigade, Division, cavalry, artillery og supply får forskellige footprints/højder. Runtime QA-overlayet kalder `RefreshQAPlaceholderVisual()`, så de faktiske unit actors nu kan ses som solide objekter på battlefield i stedet for kun tekst/linjer. Sidefarve forsøges via dynamic material-parametre, mens farvede outlines/labels fortsat er den autoritative QA-affiliation presentation hvis Engine BasicShapeMaterial ikke eksponerer tint-parametre. Company labels er forkortet yderligere, og QA-kameraet starter tættere (ca. 180 m spring-arm) for bedre taktisk læsbarhed. Placeholder-meshes er kun presentation og ændrer ikke collision, movement, formation eller combat authority.

**Unreal QA visual/runtime bootstrap (v00.02.64):** Unreal-porten kan nu startes fra et tomt/Untitled level uden manuel level-wiring. `UStrategyQARuntimeSubsystem` oprettes kun i PIE/Game/GamePreview, sikrer at ét `AStrategyOOBTestScenario` eksisterer, sikrer/possesser `AStrategyCameraPawn` og fokuserer kameraet på QA-slagmarkens centrum. GameMode genbruger et eksisterende QA-scenarie for at undgå duplicate spawn. Runtime QA-visualisering tegner tydelige farvekodede unit boxes/facing arrows, labels, river banks/bridge, navigation obstacle samt hill/ridge/depression footprints/height markers. Dette er placeholder presentation til funktionel QA og erstatter ikke de senere soldier/horse/artillery assets. UE 5.8.3 compile/runtime QA er fortsat påkrævet efter sync/build.

**v00.02.64 QA visual hotfix:** Runtime QA labels are now single-source: the legacy TextRender unit label is hidden during QA and the overlay uses one compact DrawDebugString label per unit. Company labels are shortened to side/company/echelon/strength to avoid operational-zoom overlap, while HQ/cavalry/artillery/logistics retain fuller names. The QA HUD build marker is synchronized to v00.02.64-dev.

**Unreal shared animation/uniform 20-block batch (v00.02.63):** Human visuals er nu arkitektonisk adskilt fra uniformidentitet. Kompatible soldier meshes bruger shared `SK_Human_1864` skeleton + `ABP_Human_1864` animation-set contract, så Walk/Run/Aim/Fire/Reload/Die/Mount/Dismount m.fl. kun skal produceres én gang. Canonical manifests dækker human, mounted rider, horse og artillery crew animations. UniformAppearance bruger preset + runtime overrides med selvstændige Coat/Trousers/Facings/Headgear/Leather/Accent/Metal material-parametre; historical palette lock understøttes, men nuværende QA-paletter er kun redigerbare placeholders og ikke endelig historisk farvefacit. EquipmentVisual definerer fælles rifle/bayonet/sabre/tool/backpack/cartridge sockets. Rig compatibility validerer skeleton/animation-set før visual asset regnes kompatibelt. Cavalry har horse gait Idle/Walk/Trot/Canter/Gallop og rider sync; Dragoon mounted-state udsender Mount/Dismount intent. Artillery har crew stations/roller (Gunner/Loader/Rammer/Sponger/Ammunition/Wheels/Driver/HorseHandler/Reserve) og drill-state Recoil→Sponge→Load Charge→Load Projectile→Ram→Prime→Clear→Return, plus Push/Traverse/Limber/Unlimber/Repair. Disse visuals følger authoritative gameplay-state og styrer aldrig gameplay. Final meshes, Animation Blueprints, montages og material assets kommer i det senere grafikpass.

**Build diagnostic tooling (v00.02.63):** `Tools/Build-Strategy1864.ps1` v1.1.1 performs a manual UE 5.8 `Strategy1864Editor Win64 Development` build, stores the full UnrealBuildTool log under `Tools/BuildLogs`, and extracts relevant C++/UHT/UBT errors. This is the required diagnostic path when Unreal only shows the generic “could not be compiled” popup.

**Unreal projectile/follow-camera 20-block batch (v00.02.62):** Artillery fire udsender nu presentation fra de samme resolvede per-gun impactpunkter, som fire-missionen allerede har bestemt. Presentation kan derfor ikke skabe en separat skade-authority. Ammo har forskellige trajectory-profiler: Round Shot lav bane + aftagende presentation-ricochets, Shell højere arc + impact placeholder, Shrapnel høj arc + burst-fragment placeholder, Canister kort pellet-cone og er ikke follow-camera eligible. Hvert batteri har ProjectilePresentationComponent med virtuelle gun muzzle positions, active projectile tracking og bounded shot history. Projectile actor viser et tydeligt QA-punkt under flight; F9 viser hele banen. P følger seneste levende projectile; kameraet holder ved impact og vender tilbage til præcis pre-follow battle view. WASD/rotation/zoom afbryder follow. Direct fire og area fire udsender begge deres resolvede per-gun impactpunkter til dette lag. QA validerer Shell>RoundShot arc og Round Shot ricochet. Final meshes/Niagara/audio er stadig senere grafikarbejde.

**Campaign → generated battlefield (B-291, foundation v00.02.62):** Tactical battlefield skal senere genereres fra selve campaign battle-positionen. Det er nu en fast designregel, at campaign height/topography, hills/ridges/depressions, rivers, roads, bridges, settlements, forest, fields, marsh/water samt attacker/defender approach directions føres gennem en deterministic battle-generation request. Samme request+seed skal give samme gameplay-terrain. Tactical simplification må bevare den afgørende geografiske topologi. `Campaign/StrategyBattlefieldGenerationTypes.h` er etableret som data-kontrakt; generatoren er bevidst ikke implementeret endnu. Output skal senere føde det terrain authority-lag, der allerede findes fra v00.02.61, så campaign- og battle-map ikke udvikler to forskellige terrænsandheder.

**Unreal tactical terrain 20-block batch (v00.02.61):** Tactical battlefield har nu et fælles authoritative terrainlag. Hill/Ridge/Depression actors kan lægge gameplay-elevation oven på fysisk WorldStatic-ground, så funktionelle højdedrag kan testes før final landscape-art. Shared terrain query leverer effective ground Z, local slope, terrain-profile occlusion, crest point, dead ground og elevation advantage. Alle units har TerrainAwareness med high-ground observation, reverse-slope og partial crest masking. Normal LOS og ground/area LOS bruger samme terrain authority; elevation kan forbedre observation range men kan aldrig bypass'e LOS. Infantry og artillery fire bruger terrain exposure i hit-resolution. Route planner projicerer waypoints til terrain og subdividerer lange segmenter, også uden NavMesh, så en bakke midt på ruten ikke springes over af slope-checket. Artillery deploy slope bruger samme model. ArtilleryPositioning vurderer candidate positions efter LOS, slope, elevation, crest/dead ground og movement cost og kan finde bedste direct-fire position. QA har battery hill, central ridge og depression; alle spawned units placeres på effective terrain Z. Regression kræver én forventet dead-ground lane, én clear lane, crest detection, high-ground observation advantage og en gyldig artillery direct-fire candidate. Dette er gameplay terrain authority — fysisk/visuel battlefield expansion er stadig senere arbejde. Hele slicen er code-implemented / UE 5.8.3 build+runtime QA pending.

**Mortarer og belejringsartilleri (v00.02.67 designbeslutning):** Mortarer er en særskilt artillery capability/type og må ikke reduceres til en almindelig kanon med højere damage. Historisk dokumentation fra Dybbøl viser, at preussisk belejringsartilleri anvendte morterer sammen med tunge riflede kanoner; en dansk fremstilling anfører specifikt 16 stk. 25-pundige morterer, og Historiecenter Dybbøl Banke beskriver 126 kanoner og morterer fordelt på ca. 30 batterier under den afsluttende ødelæggelsesild. Gameplaymæssigt bruger morterer høj, krum bane og kan derfor bekæmpe mål bag brystværn, i skanser, løbegrave, reverse slope og dead ground, hvor direct-fire guns ikke har normal LOS. De skal have lavere mobilitet, længere deploy/reload, lavere direkte præcision mod bevægelige mål og særlig ammunition/crew/logistik. Fire-missionen målrettes primært mod observerede områder/fortifications frem for direkte unit-LOS. Forward observation/spotting, area dispersion og ammunition conservation er centrale. Morterer indgår normalt som støtte/belejringskapacitet under højere HQ eller siege artillery group og skal ikke skabe en ny permanent OOB-type pr. kaliber.

**Unreal specialist/fortification/mortar 10 × 10 sequence (v00.02.68–v00.02.77):** Projektet er udvidet med **100 funktionelle blokke fordelt på 10 batches á 10**, uden final grafik. Sekvensen implementerer unit/capability/temporary-detachment modellen, NCO command continuity, `Standing/Kneeling/Prone` og loading-method/drill-baseret firing, fysiske defensive positions, fortification assault equipment, light/heavy mortar deployment og krum ild, position occupancy, working parties samt specialist-state persistence/QA. Detached men og ammunition kan ikke bruges dobbelt af parent formation; NCO-state påvirker reform/reload/rally/response; defensive positions har orientation, condition, capacity, owner/occupier og breach/capture-state; heavy mortars bruger `TRANSPORT → EMPLACING → DEPLOYED → PACKING → TRANSPORT`. Den operative QA-fixture har nu marksman/NCO/fire-drill state på et dansk kompagni, et gun-emplacement og et tungt morterbatteri. **Status: code implemented / UE 5.8.3 build+runtime QA pending; ikke parity-verified.** Final soldier/engineer/mortar meshes, animation, Niagara og UMG-polish er fortsat et separat senere grafikpass. De præcise blokke spores som UE-P412–UE-P511 i `UNREAL-PARITY-BACKLOG.md`, mens designbeslutningerne B-153–B-159 ligger i hovedbackloggen.

**Infantry fire-drill research progression (v00.02.79):** Firing drill er nu en eksplicit research/adoption-progression: **Front Rank Fire → Two-Rank Fire → Fire by Rank → Controlled Volley → Independent Fire → Advanced Fire Drill**. Dette er en bevidst gameplay-abstraktion og ikke et historisk udsagn om, at periodens hære kun kendte front-rank fire ved campaign-start. UStrategyFireDrillComponent starter i FrontRank, afviser valg af endnu ikke oplåste modes og bruger formationens faktiske RankCount: FrontRank bruger ét geled, TwoRankFire op til to, FireByRank ét geled pr. puls, Volley alle relevante geledder og Independent en høj kontinuerlig deltagelsesgrad. FireByRank holder en deterministisk aktiv-geled-index og skifter først efter en resolved volley; cadence deles over antal geledder, så et geled omtrent får en fuld reload-periode før dets næste tur. Legacy AlternatingSections og KneelingFrontRank bevares som skjulte serialized aliases til henholdsvis FireByRank og TwoRankFire. Advanced Fire Drill giver en automatic-selection capability mellem volley/continuous/rank fire; campaign research/adoption-systemet skal senere drive ResearchLevel. Research unlock og unit skill er adskilt: DrillTraining, FireDiscipline, NCO, stance/loading method og øvrige combat modifiers bestemmer fortsat udførelseskvaliteten. Den eksisterende tre-geleds QA-Line ændres ikke i dette checkpoint; derfor er FrontRank aktuelt 1/3 og TwoRankFire 2/3 i denne formation. **Status: code implemented; UE 5.8.3 build/runtime QA og rank-specifik 1:1 firing/reload-animation presentation pending.**

**Livgarden 1864 real 3D infantry visual/import baseline (v00.02.78):** Første brugerleverede production-style infantry asset er nu koblet til Unreal-porten som en sikker import/runtime-slice. Kildesættet består af `DK_Livgarden_1864_Apose_textured_skeleton.fbx`, `Rifle_1_textured.glb`, `Rifle_Bayonet_1_textured.glb` og 65 FBX-animationer. En statisk preflight fandt de samme 41 centrale `mixamorig:` motion-bones i body mesh og alle animationsfiler; FBX-metadataens root-label varierer (`Hips_skin` vs. `Hipsf`), så Unreal-importen er fortsat den autoritative kompatibilitetstest. `import_livgarden_1864.py` importerer body mesh som shared Skeletal Mesh/Skeleton, binder samtlige animationsklip til samme Skeleton og importerer rifle/rifle+bajonet som våbenmeshes. `UStrategyInfantryVisualComponent` kan rendere et company 1:1 fra authoritative `CurrentStrength`, bruger eksisterende formation slots og bevarer fuldt footprint ved 1:2/1:5/1:10 visuel sampling. Første danske QA-company kan aktivere dette lag uden at ændre simulation, combat authority eller formation authority. Standing/Kneeling/Prone, walk/run/aim/fire/reload, bayonet charge og death har første runtime mapping; stående reload mangler et tydeligt dedikeret klip i den leverede pakke og falder derfor midlertidigt tilbage til standing aim. **Status: source/runtime code implemented; binary assets skal importeres i UE 5.8.3 og build/runtime/weapon-offset QA er stadig pending.**

**Unreal artillery/logistics 20-block batch (v00.02.60):** Artillery fire mission kan nu målrettes mod et synligt ground/area point og begrænses af antal salver og/eller missionstid. AUTO TARGET har target priority (Balanced/CounterBattery/Infantry/ClosestThreat/ConserveAmmo), kan vælge ammunitionstype efter target/range og kan bevare en reserve via conservation-policy. Artilleri kan kun deploye under den konfigurerede lokale slope, firing/manhandling giver ekstra fatigue, disabled guns kan repareres i felten uden at resurrecte destroyed guns, og emergency abandon efterlader fysisk materiel til capture. Første fysiske logistics unit er `AStrategySupplyWagonUnit`: separat small-arms/artillery cargo, drivers, horses, wagon condition, horse/driver/condition movement scaling, damage/cargo loss, abandonment og fysisk capture. Resupply kræver radius, kompatibel cargo og rolig state; transfer stopper under movement eller under fire. Artillery ammo bruger ammunition-family tag, så forkert/captured ammunition ikke automatisk passer. QA-fixturen består nu af et bevidst ammo-lavt dansk batteri + en fysisk dansk ammunitionsvogn i transferafstand; Divisionens gamle generiske ammo-pool deaktiveres, når denne fixture er aktiv. Alle blokke er code-implemented / UE 5.8.3 build+runtime QA pending.

**Unreal artillery battery 20-block batch (v00.02.59):** Første funktionelle artilleribatteri er nu en selvstændig tactical unit i Unreal. Batteriet har Artillery-echelon, data-driven gun profile og gun count, crew, drivers og horses; timed Limbered→Deploying→Deployed→Limbering samt kort Manhandling; towing kræver limbered state og mobility skalerer med faktiske horses/drivers. Artillery ammunition ligger separat som RoundShot/Shell/Shrapnel/Canister. Spillerstyret batteri bruger MANUAL TARGET som standard; AUTO TARGET er eksplicit opt-in og HOLD FIRE overstyrer begge uden at slette gemt manual target. Skydning kræver deployed/operational guns, crew, ammunition, ammo-specific min/max range, current contact, LOS og korrekt traverse; batteriet traverserer fysisk før ild. Reload påvirkes af crew availability, fatigue og experience. Prototype-ammunition har forskellige range/hit/casualty-kurver, men endelige 1864-ballistikværdier er research/data tuning. Incoming infantry/counter-battery fire fordeles til personnel, horses og guns; guns kan blive disabled/destroyed, batteriet kan blive Abandoned, fysisk Captured og senere reused efter qualified crew + compatible ammunition + preparation time. Shared tactical supply kan genforsyne artillery inventory. Higher-HQ infantry slot planner ignorerer artillery. QA-OOB har nu ét dansk 6-kanoners testbatteri direkte under Division; seks kanoner er en QA-fixture og ikke en universel historisk batteristørrelse. Artillery er med i battle outcome og shared OOB/hover snapshot. Hele slicen er code-implemented men UE 5.8.3 build/runtime QA pending.

**Unreal combat/fieldcraft/AI 20-block batch (v00.02.58):** Fire discipline er nu adskilt fra fire-range policy (HoldFire/FireAtWill/Volley/Independent), ammunition conservation er gameplay, Standing/Prone påvirker movement/reload/target profile, directional cover virker kun fra dækkets faktiske retning, og infantry kan bygge hasty fieldworks. Sortkrudtsrøg er simulation med density/lifetime og påvirker både accuracy og LOS. Skirmishers kan detach'es i fem roller og senere recall/reforme med tid/cohesion-omkostning. Ammunition har supply source/request/transfer. Doctrine + OrderAgg, Strict/Normal/Independent autonomy, no-cheat Easy/Normal/Hard AI, AI-DIAG reason codes, udvidet officerprofil og mission constraints er koblet ind i decision core. I toggler Officer AI på valgte units, F6/F7/F8 ændrer kun enemy reaction/noise. Grafik/animation/Niagara/UMG-polish er stadig bevidst senere arbejde. Hele batchen afventer UE 5.8.3 build/runtime QA.

**Næste prioritet — artilleribatteri:** Artilleri skal implementeres som en rigtig taktisk enhed før grafikpasset: batteri med antal kanoner, besætning, ammunition, gun type/calibre, deployed/limbered state, facing/traverse arc, range/fire mission, movement kun når limbered, supply/resupply samt samme OOB/command authority som øvrige enheder. Senere kobles heste/limber, kanonmodeller, crew-animation og firing VFX på uden at ændre simulation authority.

**Unreal functionality-first 23-block batch (v00.02.57):** Grafik, skeletal assets, animation, Niagara og endelig UMG er bevidst skubbet til senere. Gameplay-kernen er udvidet med finite Attack-anchor og persistent leaf Defend-reassert; building/fence/fieldwork obstacle detours; slope rejection samt waypoint-Z/uphill/downhill movement; officerprofil og reel command-range order delay; fatigue/experience-effekter; færdigere cavalry defaults, reform-gated CHARGE og non-charging screen AI; current/last-known contact memory og contact+LOS+range+cone fire authority; rigtig SPEJD HER state machine (Seek→Recon→Contact→Screen→Report/LastKnown); OOB tactical attachment API der kun ændrer CurrentCommandParent; bridge occupancy/queue; autonom opposition company AI der respekterer hierarchy/standing intent; routed fallback og safe-distance rally; authoritative ammo exhaustion/resupply; samt udvidet regression fixture. Delayed child-orders tæller som aktiv execution, så higher parent ikke kan gå Completed før ordren faktisk er leveret/udført. Hele batchen afventer lokal UE 5.8.3 build/runtime QA.

**Unreal formation/navigation/combat batch (v00.02.56):** Formation changes har nu en eksplicit Reforming-transition, der pauser men ikke erstatter aktiv mission/route; efter reform fortsætter samme ordre. Final-slot completion venter desuden på fysisk committed-facing rotation inden for 2°. 190-mands Line frontage valideres mod ca. 48 m, og moving companies har lokal sidestep/deconfliction omkring 68 m reserved center spacing. River routes har nu autoritativ valid/invalid-state: target/start i hard water afvises, og movement må ikke fallback'e direkte gennem floden. Nyt mål under aktiv bridge transaction bevarer crossing til exit og retargeter først derefter. Combat sender presentation events kun fra rigtige volleys og positive authoritative casualties. Reforming units kan ikke skyde, Square ejer sin outline-visualisering, og Square volley-event kommer fra den faktiske firing face. 1:1 soldier interpolation og Niagara-binding er stadig eksplicit pending. Hele batchen afventer UE 5.8.3 build/runtime QA.

**Unreal næste 10 blokke (v00.02.55):** Command hierarchy er gjort robust mod cycles og organisk re-parenting respekterer temporary tactical attachment. QA-fixturen bruger nu deterministisk combat seed, validerer StableUnitId/backlinks/cycles og kører automatisk regression checklist. Scenario-state kan afgøre DenmarkVictory/OppositionVictory/Draw, og F5 bygger testen deterministisk igen. RYK FREM, TILBAGETRÆK og SAML er nu recursive coordinated formation missions gennem Division → Brigade → Regiment → Major → Company; command-parent HQ bliver bag formationen og løber ikke selv mod objective. Et fælles OOB/hover snapshot eksponerer identity/NATO, strength/loss, morale/cohesion/fatigue, AI, ordre/execution, formation, OrganicParent/CurrentCommandParent, attachment, ammo og cavalry/Dragon mounted state. Hele batchen er code-implemented og afventer samlet UE 5.8.3 build/runtime QA.

**Unreal 10-block implementation batch (v00.02.54):** Unreal-porten har nu en synlig build/QA HUD, recursive OOB aggregate state, OOB select+camera-focus API, semantic zoom (Close/Medium/Operational/Strategic/VeryFar), NATO echelon labels I/II/III/X/XX, strategic mesh suppression uden simulationstab, selected command-tree/mission/route QA visuals, execution-only command colour state, tactical pause + 1x/2x/3x, finite cavalry CHARGE med swept contact stop samt Dragon 75/25 dismount/horse-holder/horse-park/STIG OP remount core. Kameraets zoomrange er udvidet så operational/strategic semantic states faktisk kan nås. CHARGE bruger mounted speed og stopper ved valid enemy contact i stedet for at passere formationen. Dragon QA-enheden i runtime-scenariet er nu konfigureret som Dragoon. Hele batchen er code-implemented men afventer samlet UE 5.8.3 build/runtime QA.

**Unreal Officer AI + cavalry tasking checkpoint (v00.02.53):** U07 har nu et fælles OfficerAI-lag på alle strategy-units med AI ON/OFF cascade, inherited mission-regler og DirectPlayer authority-beskyttelse. HQ follow er et separat background movement-lag baseret på subordinate centroid og committed facing, så det ikke kan konkurrere med Defend/Attack mission ownership; valgte HQ'er viser command-zone QA-ringe. Rout er nu autoritativ ved lave morale/cohesion thresholds og stopper movement/fire/execution. Infantry anti-cavalry reaction kræver faktisk LOS og går i Square med restore af tidligere formation efter fravær af trussel. U08 har nu temporary cavalry task attachment: højere ANGRIB HER kan lægge cavalry under Major A/B via CurrentCommandParent uden at ændre OrganicParent, to-mod-to pairing vælger laveste samlede travel cost, og cavalry returnerer til tidligere parent/reserve efter missionen. FORSVAR HER placerer cavalry som reserve ca. 150 m bag / 45 m lateralt for parret Major. Runtime QA OOB kan nu spawne to danske cavalry og to ikke-selekterbare preussiske test-companies. Alt afventer UE 5.8.3 build/runtime QA.

**Unreal river/bridge + combat execution checkpoint (v00.02.52):** U05 har nu et data-drevet river/bridge-lag med bank-side klassifikation, true-bank-change, same-bank detour og eksplicit bridge transaction metadata. Cavalry går først i 2-abreast Defile ca. 36 m fra bridge approach eller under selve crossing og gendanner derefter pre-defile formation. U06 har nu rigtig volley/reload/ammunition/casualty core: ammo forbruges, reload times, 0-hit volley er gyldig uden strength loss, positive hits reducerer authoritative strength og kan sætte Destroyed. Morale/Cohesion er separate states, og incoming fire giver shock samt en midlertidig movement-pause uden at afslutte parent missionen. Square bruger fire 90-graders fire sectors. Alt afventer samlet UE 5.8.3 build/runtime QA.

**Unreal broad parity checkpoint (v00.02.51):** Unreal-porten er udvidet på tværs af U03-U06/U08. Click-drag ordreplacering giver committed facing uden at rydde selection. ANGRIB HER/FORSVAR HER kan nu kaskadere Division → Brigade → Regiment → Major/Battalion → Company, mens command-parent HQ ikke selv løber ind på objective. Parent execution afsluttes først, når underordnede executors er færdige. Infantry long-move kan bruge March Column og skifter mod Line ved F30X-reglen `max(own/enemy MaximumFireRange)+35 m`. Movement bruger nu Unreal NavMesh-waypoints med 50 cm final-arrival authority. Et fælles LOS/fire-control lag ejer HOLD/CLOSE/MEDIUM/LONG, ±35° cone og target eligibility, mens QA-cones er rent visuelt lag og kan vises på preussiske test-enheder fra battle-start. Formation core understøtter desuden Square, Cavalry 4-rank Line, 4-abreast Column og 2-abreast Defile med restore af pre-defile formation. Alt i dette checkpoint er kodeimplementeret men afventer samlet UE 5.8.3 build/runtime QA før parity-status kan gives.

**Unreal U04 formation-planning baseline (v00.02.50):** Unreal har nu et selvstændigt formation state/slot-lag. `UStrategyFormationComponent` genererer tre-rank infantry Line og 4-wide March Column geometri. `UStrategyParentFormationPlannerComponent` nedbryder Battalion/Major ANGRIB HER/FORSVAR HER til deterministiske company-slots sorteret efter CompanyNumber, med committed facing og F30X-spacing på 72 m nominal / 68 m reserved minimum. Parent authority bevares på child missions. Early deploy mod enemy range, fysisk 1:1 soldier reform og højere Regiment/Brigade/Division slot planning er næste U04-del.

**Unreal U03 physical executors (v00.02.49):** Alle `AStrategyUnit`-enheder har nu en fælles `UStrategyMovementExecutorComponent`. MOVE udfører fysisk bevægelse mod target; HOLD stopper straks bevægelse og afslutter fysisk execution uden at slette standing intent. ANGRIB HER, FORSVAR HER, RYK FREM, TILBAGETRÆK, SAML og SPEJD HER bruger samme finite fysiske movement-kerne, mens deres endelige subordinate-/formationsgeometri fortsat hører til U04/U03-planner. PlayerController kan afgive DirectPlayer-ordrer til den aktuelle selection og pending target placement committer uden at rydde selection. Arrival tolerance er 50 cm som Unreal-modstykke til F30V-reglen.

**Unreal U03 order-state foundation (v00.02.48):** `UStrategyOrderComponent` adskiller nu standing intent fra fysisk execution. Ordrer har eksplicit authority (`InheritedAI`, `OfficerAI`, `DirectPlayer`) og execution-state (`Idle`, `PendingTarget`, `Pending`, `Executing`, `Completed`, `Failed`, `Superseded`). Lavere authority kan ikke overskrive en højere aktiv ordre, og re-issue kan supersede en igangværende executor uden at slette selve command-state modellen. MOVE/ANGRIB/FORSVAR/HOLD executors bygges oven på denne kerne.

**Unreal U02 runtime OOB test (v00.02.47):** `AStrategyOOBTestScenario` kan nu automatisk bygge en dansk test-kommandokæde i runtime med Division → Brigade → Regiment → Major A/B → otte kompagnier. GameMode kan auto-spawne scenariet, placeholder-enheder har selection-collider/debug-label, og parent/subordinate relationer logges til QA. Dette kobler U01 selection sammen med U02 hierarchy-testen uden manuel level-wiring.

**Unreal U02 hierarchy foundation (v00.02.46):** Unreal-porten har nu et autoritativt unit/command-datalag: stable unit ID, display name, side, echelon, initial/current strength, unit state og AI-state på `AStrategyUnit`; fysisk company- og HQ-actor-baseline; samt `UStrategyCommandComponent` med separate `OrganicParent` og `CurrentCommandParent`, subordinate-lister og restore-to-organic-funktion til senere midlertidig cavalry tasking. Dette er implementeret kode, men endnu ikke build-/runtime-verificeret som U02-paritet.\n\n**Unreal parity backlog (v00.02.45):** En dedikeret feature-for-feature migrationsbacklog er nu etableret i [UNREAL-PARITY-BACKLOG.md](UNREAL-PARITY-BACKLOG.md). Den sporer Unity → Unreal fra BACKLOG/SCAFFOLD/IMPLEMENTED til BUILD VERIFIED og PARITY VERIFIED, med konkrete QA-exit-kriterier for camera/selection, command hierarchy, orders/authority, infantry formations, river/bridge navigation, LOS/fire, Square, Officer AI, cavalry/dragons, OOB/HUD/semantic zoom, assets/animation og den samlede U10 replacement gate.

**Unreal U01 — Camera & Selection (v00.02.44):** Den første interaktive Unreal-kontrolkerne er nu implementeret på `unreal-port` og afventer lokal UE 5.8.3 build/QA. `AStrategyCameraPawn` leverer WASD-pan, Q/E rotation og mouse-wheel zoom. `AStrategyPlayerController` leverer plain click, box selection, Shift-add og Ctrl-remove. Box selection følger den fastlagte RTS-regel om unit-centret inde i markeringen. `AStrategyUnit` bærer persistent selection state og udsender `OnSelectionChanged` til Blueprint-visuals. Selection må ikke nulstilles af efterfølgende order placement; order-input implementeres separat i U03.

Horse/rider-arkitekturen fastlåses som to separate rigs: hesten er et quadruped Skeletal Mesh med eget Skeleton/Animation Blueprint; rytteren er et separat human Skeletal Mesh, der attach'es til et `RiderSocket`/saddle-point på hesten og synkroniseres gennem cavalry animation state. Dette gør hest, rytter, dismount, death/fall og unit-varianter genbrugelige uafhængigt.

**Dynamiske faner og bannere (v00.02.51):** Regimentsfaner, kavalerivimpler og kommandostandarter bygges som en genbrugelig Unreal-arkitektur i stedet for én unik 3D-model pr. enhed. Grundaktøren `BP_UnitStandard` består af separat `PoleMesh`, valgfrit `FinialMesh`, `FlagMesh` og dynamisk flagmateriale. Enhedsdata vælger flagtexture/front-back, flagform, skala, pole type og finial type. Baseline-flagformer er rektangulær standard, lang pennant/vimpel, svalehalefane, ceremoniel/command-standard og ørnestandard. 1–3 standardstænger og udskiftelige finials genbruges på tværs af Danmark/Preussen, mens regimentsidentitet primært ligger i texture/data. Flagbevægelse bruger billigt World Position Offset/wind-materiale på normale LODs; Chaos Cloth kan reserveres til nær-Lod/særlige standarder. Standardbæreren bærer aktøren via hånd-socket; banneret kan senere overdrages, tabes eller erobres uden at ændre enhedens kerne-OOB. De første 3D-referenceark modellerer front/side/back som separate billeder, så hvert objekt kan rekonstrueres uafhængigt i 3D.

Den aktuelle **gameplay-/buildbaseline** ligger på **v00.00.09f30x**. Cavalry visual-fidelity bygger fortsat på **v00.00.09f30l**. F30H fastlåser cavalry som 1:1 med **4 geledder i normal Line og ved Charge**, 4-abreast normal Column og 2-abreast bridge/defile column; formation changes er fysisk synlige, bridge routing er en vedvarende transaction, cavalry HUD følger company-HUD designet, og den aktive semantic-zoom-owner viser cavalry samt Brigade/Division. Det centrale design for regimentschef, cavalry, charge shock og infantry square er samlet i [Designsupplement v00.02.09 — Regimentskommando, cavalry og square](design-supplements/v00.02.09-regimental-command-cavalry-square.md). Map/attack-AI-hardening er dokumenteret i [v00.00.09f29b — Battlefield + Attack AI Hardening](design-supplements/v00.00.09f29b-battlefield-attack-ai-hardening.md), den fælles Major/Company command UI i [v00.00.09f29c — Unified Command HUD](design-supplements/v00.00.09f29c-unified-command-hud.md), semantic zoom/HQ visibility/NATO-symboler i [v00.00.09f29d — Semantic Zoom + NATO Symbols](design-supplements/v00.00.09f29d-semantic-zoom-nato-symbols.md), navigation/sidestep/visual polish i [v00.00.09f29p — Navigation, Side Step & Visual Polish](design-supplements/v00.00.09f29p-navigation-sidestep-visual-polish.md), OOB/HQ-discoverability i [v00.00.09f29q — OOB, HQ Discoverability & Semantic Zoom](design-supplements/v00.00.09f29q-oob-hq-semantic-zoom.md), crop-field concealment/map polish i [v00.00.09f29r — Crop Field Concealment & Map Polish](design-supplements/v00.00.09f29r-crop-field-concealment.md), defensive/UI/terrain-hardening i [v00.00.09f29y — Defensive Stability, TEST AI, HUD Status & Terrain Polish](design-supplements/v00.00.09f29y-defensive-stability-ui-terrain-polish.md), Square face-fire i [v00.00.09f29z — Square Face Fire & Directional Smoke](design-supplements/v00.00.09f29z-square-face-fire-smoke.md), første mounted cavalry implementation i [v00.00.09f30 — First Cavalry Core](design-supplements/v00.00.09f30-cavalry-core.md), command/officer-hardening i [v00.00.09f30a — Command, Square and Officer AI Hardening](design-supplements/v00.00.09f30a-command-officer-hardening.md), og den højere kommandokæde i [v00.00.09f30b — Higher Command HQ](design-supplements/v00.00.09f30b-higher-command-hq.md). Cavalry Officer AI/dynamic attachment ligger i [F30C](design-supplements/v00.00.09f30c-cavalry-ai-dynamic-attachment.md), OOB scroll/drag-drop i [F30D](design-supplements/v00.00.09f30d-oob-scroll-dnd-cavalry-visuals.md), 1:1 cavalry/visual fidelity i [F30E](design-supplements/v00.00.09f30e-1to1-cavalry-visuals.md), og den samlede historik i [Implementation & Fix History](IMPLEMENTATION-AND-FIX-HISTORY.md) samt [F30F-supplementet](design-supplements/v00.00.09f30f-history-consolidation.md). F30G OOB/input/HQ-cavalry visibility hotfix er dokumenteret i [F30G-supplementet](design-supplements/v00.00.09f30g-oob-input-hq-cavalry-visibility-hotfix.md). F30H 4-rank/bridge/HUD/NATO/selection-baseline er dokumenteret i [F30H-supplementet](design-supplements/v00.00.09f30h-cavalry-4rank-bridge-hud-nato-selection.md). F30I authority/single-HUD/order-visual/animation-hardening ligger i [F30I-supplementet](design-supplements/v00.00.09f30i-cavalry-authority-hud-order-visual-animation.md). F30J dismounted Dragon fire/horse-holder/anchor-fix ligger i [F30J-supplementet](design-supplements/v00.00.09f30j-dismounted-dragon-fire-anchor.md). F30K auto-march/RMB-facing/split-selection/higher-HQ-AI ligger i [F30K-supplementet](design-supplements/v00.00.09f30k-auto-march-rmb-facing-higher-hq-ai.md). F30L cavalry visual/gait-pass ligger i [F30L-supplementet](design-supplements/v00.00.09f30l-cavalry-visual-fidelity-gait.md). F30M higher-command delegation, mission-visual parity og midlertidig cavalry task-attachment ligger i [F30M-supplementet](design-supplements/v00.00.09f30m-higher-command-delegation-cavalry-tasking.md). F30N higher-HQ HUD-parity, cavalry screen/opportunity AI og anti-cavalry infantry reaction ligger i [F30N-supplementet](design-supplements/v00.00.09f30n-hud-parity-cavalry-screen-anti-cav.md). F30O higher-AI arming, shared target-circle og true F29G HUD parity ligger i [F30O-supplementet](design-supplements/v00.00.09f30o-command-authority-target-preview.md). F30P committed facing, higher-HQ follow og command zones ligger i [F30P-supplementet](design-supplements/v00.00.09f30p-facing-hq-follow-command-zones.md). F30Q active-order HUD, balanced attack-front og CAV scout-design ligger i [F30Q-supplementet](design-supplements/v00.00.09f30q-active-orders-balanced-attack-scout.md). F30R Dragon fire-control ligger i [F30R-supplementet](design-supplements/v00.00.09f30r-dragon-fire-control.md). F30S execution-state, defensive CAV reserve, cone-QA og selection persistence ligger i [F30S-supplementet](design-supplements/v00.00.09f30s-execution-cav-cone-selection.md). F30T crop-tufts og infantry active-range authority ligger i [F30T-supplementet](design-supplements/v00.00.09f30t-crop-tufts-infantry-cones.md). F30U defend-command authority og HQ-goal conflict fix ligger i [F30U-supplementet](design-supplements/v00.00.09f30u-defend-command-authority.md). F30V single final-slot arrival authority ligger i [F30V-supplementet](design-supplements/v00.00.09f30v-final-slot-arrival-authority.md). F30W enemy-cone QA, same-bank river routing og OOB AI consistency ligger i [F30W-supplementet](design-supplements/v00.00.09f30w-enemy-cone-river-oob.md). F30X early infantry deploy, company spacing, CAV bridge approach og startup enemy cones ligger i [F30X-supplementet](design-supplements/v00.00.09f30x-deploy-spacing-cav-bridge.md).

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
- [Designsupplement — Lande og uniformer 1851–1866 (14 fraktioner, 107 uniformsmodeller)](design-supplements/lande-og-uniformer-1851-1866.md)
- [Implementation & Fix History — samlet historik over implementeringer og fejlrettelser](IMPLEMENTATION-AND-FIX-HISTORY.md)
- [Designsupplement v00.00.09f30f — Implementation & Fix History Consolidation](design-supplements/v00.00.09f30f-history-consolidation.md)
- [Designsupplement v00.00.09f30h — Cavalry 4-rank, bridge, HUD, NATO & selection](design-supplements/v00.00.09f30h-cavalry-4rank-bridge-hud-nato-selection.md)
- [Designsupplement v00.00.09f30m — Higher Command Delegation + Temporary Cavalry Tasking](design-supplements/v00.00.09f30m-higher-command-delegation-cavalry-tasking.md)
- [Designsupplement v00.00.09f30s — Execution State, Defend CAV, Cone QA & Selection](design-supplements/v00.00.09f30s-execution-cav-cone-selection.md)
- [Designsupplement v00.00.09f30t — Crop Tufts + Infantry Active Range Authority](design-supplements/v00.00.09f30t-crop-tufts-infantry-cones.md)
- [Designsupplement v00.00.09f30u — Defend Command Authority](design-supplements/v00.00.09f30u-defend-command-authority.md)
- [Designsupplement v00.00.09f30v — Single Final-Slot Arrival Authority](design-supplements/v00.00.09f30v-final-slot-arrival-authority.md)
- [Designsupplement v00.00.09f30w — Enemy Cone QA, Same-Bank River Routing & OOB AI](design-supplements/v00.00.09f30w-enemy-cone-river-oob.md)
- [Designsupplement v00.00.09f30x — Early Deploy, Company Spacing, CAV Bridge Approach & Startup Enemy Cones](design-supplements/v00.00.09f30x-deploy-spacing-cav-bridge.md)
- [Designsupplement v00.00.09f30g — OOB input / higher HQ & cavalry visibility hotfix](design-supplements/v00.00.09f30g-oob-input-hq-cavalry-visibility-hotfix.md)
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

## Aktuel taktisk baseline — konsolideret efter F30X

Denne sektion er den korte operative reference for den aktuelle P0A battle-prototype. Den ændrer ikke gameplay-buildet; **v00.00.09f30x** er fortsat runtime-baseline.

### Command hierarchy og authority

Den aktive danske command chain er:

`Division → Brigade → Regiment → Major A / Major B → Companies`

- En højere ordre delegeres ned gennem command chain i stedet for at flytte alle underenheder direkte fra samme controller.
- Direkte player order har højere authority end inherited Officer AI mission.
- Selection af Division/Brigade/Regiment/Major bevares gennem objective/facing placement og efter order commit.
- `OrganicParent` ændres ikke ved midlertidige attachments. `CurrentCommandParent` kan midlertidigt ændres ved tactical tasking.
- OOB AI-kolonnen bruger konsekvent `ON / OFF` på alle niveauer.

### Ordrestatus og completion

- **Blå ordreknap = fysisk execution i gang.**
- **Rød ordreknap = ingen relevant movement/charge executor er længere aktiv.**
- Standing intent kan fortsat være `FORSVAR HER` eller `HOLD`, selv om knappen igen er rød.
- Company parent-mission completion kræver fysisk final-slot arrival på **≤0,50 m** fra `mission.Goal`.
- Parent HUD må ikke stole på `mission.Arrived` alene; aktiv destination og fysisk afstand indgår.
- Brigade/Division background follow og ren CAV visual reform må ikke alene holde ordreknappen blå.

### FORSVAR HER

- Den committed objective/facing er autoritativ for defensive disposition.
- DefensiveStability ejer Major-HQ rear placement under `DefendHere`.
- HqDepthGuard må ikke konkurrere om Major-/Regiment-HQ goals under samme committed defend-mission.
- Når et HQ er fysisk settled, skal competing/stale HQ-goals cleares.
- Defensive CAV placeres som reserve/flankesikring **bag** den bataljon, den støtter.

### Infantry movement og formation

- Line er kampformation; Column er march-/passageformation.
- Normal marchformation ejes af `PrototypeMarchColumn09F6`; parent mission ejer destinationen.
- Infantry skal ikke vente til det allerede er inde i fjendens ildområde med at deployere.
- Aktuel TEST-regel: **deploy til Line ved nærmeste fjendes MaximumRange + 35 m buffer**.
- Ved den nuværende 100 m TEST maximum range starter fysisk Line-reform derfor omkring **135 m**.
- Formation change skal være fysisk synlig; enheden må ikke skyde, før Line-reform er klar.
- En aktiv bridge route kan fortsat kræve Column gennem selve passagen.

### Company spacing og deconfliction

- Et full-strength 190-man company i tre geledder har ca. **48 m frontage**.
- Nominal company spacing er **72 m center-center**.
- Minimum reserved centre spacing er **68 m**.
- Hvis terrain/slot-safety kræver korrektion, foretrækkes lateral forskydning før større depth-stagger.
- Companies må ikke ende fysisk gennem/oven i hinanden i den færdige formation.

### Fire cones, TEST visibility og LOS

Aktuelle TEST-ranges er:

- Close: **35 m**
- Medium/Effective: **70 m**
- Long/Maximum: **100 m**
- Fire arc: **±35°**

Visual language:

- aktiv `CLOSE / MED / LONG` er stærk/tydelig;
- øvrige ranges er svage reference-cones;
- `HOLD` viser kun reference-ranges.

I TEST/QA:

- alle levende preussiske infantry cones skal være synlige fra battle-start uden selection eller distance-gate;
- manglende enemy fan-renderers oprettes af QA-overlayet, når enemy Regiment findes;
- synlig enemy cone er **kun QA** og giver ikke LOS, target knowledge eller firing authority.

Reel enemy fire/reaction mod CAV kræver fortsat **LOS + valgt fire-policy range + fire-cone**.

### Cavalry

Normal mounted cavalry baseline:

- Line/Charge: **4 geledder**
- normal march Column: **4 abreast**
- bridge/defile: **2 abreast**

Bridge rule:

- opposite-bank destination kan sætte `bridgeRoutePlanned` uden at ændre formation;
- CAV rider mod broen i normal formation;
- 2-abreast aktiveres først ved actual bridge approach, aktuelt ca. **36 m** fra near-bank approach;
- efter far-bank exit-clearance gendannes pre-bridge formation.

Tactical cavalry behaviour:

- CAV må ikke bare ride direkte gennem fjendtlig infantry;
- Officer AI skal screene/holde stand-off og vente på en taktisk mulighed;
- infantry kan reagere på synlig CAV med ild og/eller Square, når de relevante LOS/range/cone-betingelser er opfyldt;
- ved højere `ANGRIB HER` kan cavalry midlertidigt task-attaches til Major A/B uden at ændre OrganicParent;
- efter afsluttet attack-task returneres det til tidligere command parent som reserve;
- fremtidig FOG/LOS-baseline inkluderer `SPEJD HER` som cavalry reconnaissance-order.

### Dragon

- Mounted Dragon følger samme cavalry movement/bridge-baseline.
- Afsiddet Dragon deles i ca. **75 % mobile combat dragons** og **25 % horse holders**.
- Horse holders og horse park forbliver ved dismount-positionen under normal afsiddet movement.
- `STIG OP` væk fra hestene betyder return-to-horses og derefter remount.
- Dismounted Dragon har `HOLD / CLOSE / MED / LONG` fire-control med samme active-range visual language som infantry.

### HQ og command zones

- Major, Regiment, Brigade og Division er fysiske HQ'er.
- HQ'er følger fremad i taktisk relevante bounds i stedet for at stå permanent ved spawn.
- Division skal ikke blive unødigt langt bagud, men højere HQ skal samtidig bevare større rear-depth end Regiment/Major.
- HQ command circles/zones er en del af command-visualiseringen og skal være læsbare, når det relevante HQ er selected.
- Higher-HQ movement er command housekeeping; det må ikke forveksles med company mission completion.

### River routing

- Åbent vand er hard blocker; bridge er den lovlige crossing i den aktuelle test.
- Bridge route kræver et **reelt bankskifte**.
- Hvis start og mål er på samme bred, men den geometriske straight line skærer en bugtet del af åen, bruges bank-follow i stedet for et unødvendigt bridge round-trip.
- Slutmålet bevares gennem routing; bridge steering er midlertidig fysisk execution, ikke en ny strategisk mission.

### Aktuel QA-prioritet

Ved regressionstest efter F30X kontrolleres især:

1. enemy cones synlige straks ved battle-start;
2. infantry deployer til Line før enemy fire envelope;
3. companies ender uden overlap;
4. CAV går først i 2-abreast tæt ved broen;
5. same-bank routes giver ikke bridge round-trip;
6. `FORSVAR HER` og `ANGRIB HER` forbliver blå kun under reel execution;
7. parent-order går først rød, når final-slot/HQ/CAV execution reelt er afsluttet;
8. OOB viser konsekvent AI `ON/OFF`;
9. selection bevares efter højere ordre;
10. fjendens CAV-/Square-/fire reaction respekterer LOS, range og cone.

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

## v00.00.09f30x Early infantry deploy, company spacing, CAV bridge approach og startup enemy cones — aktuel gameplay baseline

F30X ændrer infantry approach doctrine, så marchkolonne ikke først brydes, når company allerede er inde i fjendens Long/Maximum fire range. Tactical deployment starter nu ved **nærmeste fjendes MaximumRange + 35 m buffer**. Ved en 100 m fjendtlig maksimal ildafstand betyder det ca. 135 m. Formålet er, at den fysiske tre-geleds reformering når at ske, før formationen træder ind i fjendens ildområde.

`PrototypeMarchColumn09F6` er den autoritative movement-formation owner under normal march. F27 må fortsat eje destinationen, men må ikke periodisk tvinge Line ved hver destination-reassert. Det fjerner en skjult Line/Column authority-konflikt.

Company-slots får mere lateral plads. Standard `CompanySpacing` hæves fra 60 m til **72 m**, og reserved minimum centre spacing fra 55 m til **68 m**. Da et fuldt 190-mands company i tre geledder er ca. 48 m bredt, giver dette mere sikker afstand ved skrå/roterede formationer og reducerer visuel/fysisk overlap.

CAV bridge formation deles i to states:

- **bridge route planned:** CAV bevæger sig mod broen i normal Line (4 geledder) eller normal Column (4 abreast);
- **actual bridge approach/crossing:** først inden for ca. **36 m** af near-bank approach går CAV til den specielle **2-abreast** bridge/defile geometry;
- efter far-bank exit-clearance gendannes formationen fra før bridge narrow mode.

TEST enemy-cones skal eksistere fra første relevante battle-frame. F30X enemy QA overlay opretter selv manglende Close/Medium/Long `LineRenderer` children, så visibility ikke afhænger af at et andet visual-system eller selection-flow først har oprettet fan-objekterne.

## v00.00.09f30w Enemy cone QA, same-bank river routing og OOB AI consistency — aktuel gameplay baseline

I TEST/QA er alle levende preussiske infantry fire-cones nu tvunget synlige uden krav om dansk selection eller afstand. Det gamle F29O overlay måtte tidligere skjule dem igen efter den nyere FireVisuals-pass; F30W gør F29O til seneste QA-authority og fjerner selection-/180 m-gaten. Aktiv fire-policy range er tydelig, øvrige ranges svage. Dette er stadig kun visualisering og ændrer ikke LOS, target knowledge eller firing authority.

River-navigation skelner nu mellem **ægte bankskifte** og en straight-line chord, der kortvarigt skærer den bugtede å men ender på samme bred. Kun ægte bankskifte må starte bridge route. Same-bank path følger i stedet en tør bank-waypoint-serie og må ikke ride/marchere over broen for derefter at vende tilbage.

OOB AI-kolonnen er standardiseret til `ON / OFF` på alle niveauer. Division, Brigade, Regiment, Battalion/Major, Company og CAV viser dermed samme statusnotation.

Higher-order execution-state tæller ikke længere Brigade/Division background follow eller ren CAV reform som taktisk ordreexecution. Blå ordrestate følger de faktisk tildelte company/Major/Regiment/CAV move/charge executors.

## v00.00.09f30v Single final-slot arrival authority — aktuel gameplay baseline

F30V fastlåser én fælles regel for, hvornår et company faktisk er fremme ved sit parent-assigned destination footprint. Tidligere lå tre tolerancer oven på hinanden: F27 kunne afslutte ved 4,5 m, F29E forsøgte efterfølgende at korrigere til 0,85 m, og F29L kunne latch DefendHere ved 1,35 m. Det gjorde completion-state vanskelig at læse og kunne i praksis gøre parent-ordren rød, mens en synlig formation stadig stod kort af sin destination.

Den nye autoritet er **fysisk final-slot arrival ved 0,50 m**:

- F27 mission completion: ≤0,50 m fra `mission.Goal`;
- F29E precise-arrival compatibility: samme 0,50 m;
- F29L defensive latch: samme 0,50 m, med 1,25 m release hysteresis;
- parent HUD execution-state må ikke stole på `mission.Arrived` alene;
- hvis company stadig er >0,50 m fra goal, stadig har en movement destination eller `Arrived=false`, er parent-ordren fortsat aktiv/blå.

Destination-footprint og movement bruger samme `mission.Goal`. En blå rectangle er derfor ikke kun et preview; det er det samme fysiske mål, som completion-state valideres imod.

## v00.00.09f30u Defend-command authority og HQ-goal conflict fix — aktuel gameplay baseline

F30U fastlåser command authority for `FORSVAR HER`: den committed defensive facing/formation er autoritativ for Major- og Regiment-HQ placering. Generiske housekeeping-systemer må ikke bagefter flytte HQ efter en anden referenceakse.

QA-videoerne fra F30S viste en direkte konflikt mellem `PrototypeDefensiveStability09F29Y` og `PrototypeHqDepthGuard09F29W`: DefensiveStability satte/stabiliserede HQ ud fra mission-facing, mens HqDepthGuard efterfølgende gav nye rear goals ud fra nærmeste fjende. Resultatet kunne være gentagne `HqGoalStabilized` / `HQ-DEPTH Corrected` loops, unødvendig HQ-bevægelse og en ordre, der så ud til aldrig at blive færdig.

F30U-regler:

- under aktiv `DefendHere` må HqDepthGuard ikke skrive Major-HQ goals;
- under aktiv regimental `DefendHere` må HqDepthGuard ikke skrive Regiment-HQ goal;
- DefensiveStability ejer Major defensive rear-position og bank/facing;
- når Major allerede står på den committed defensive position, cleares enhver resterende/konkurrerende `HqGoal` uanset hvor den peger;
- `HasHqGoal` må derfor ikke holdes kunstigt true af et andet helper-system;
- blå ordrestatus skal kunne afslutte normalt, når faktisk movement/follow er færdigt.

## v00.00.09f30t Crop-tufts og infantry active-range authority — aktuel gameplay baseline

F30T retter to visuelle QA-fejl. Crop-field polish brugte tidligere lange solide box-segmenter på ca. 7–8 m, som ved tæt kamera lignede store gule bjælker. De erstattes af mange korte opretstående krydsede crop-tuftenheder/stængelklynger, der sampler terrænhøjden individuelt. Gameplay-ejeren for concealment/movement forbliver F29R; F30T ændrer kun præsentationen.

Crop-rooten er versionsløftet til `CropFields09F30T`, og den tidligere `CropFields09F29Y` root disables eksplicit. Det sikrer, at hot reload/QA ikke genbruger det gamle beam-mesh.

Infantry fire-cones har nu én endelig visual authority i `PrototypeFireVisuals09F8`. Dette script kører sent i LateUpdate og var årsagen til, at tidligere alpha/width-ændringer i `Regiment.cs` blev overskrevet. F30T gør derfor active/inactive state direkte i FireVisuals:

- aktiv `CLOSE / MED / LONG`: tydelig, næsten opaque og tykkere;
- ikke-aktive fysiske ranges: svage reference-cones;
- `HOLD`: alle ranges svage/reference-only;
- preussiske infantry-cones følger samme visual language i TEST/QA, også uden selection.

TEST-visible enemy cones er fortsat kun QA og ændrer ikke LOS, target knowledge eller firing authority.

## v00.00.09f30s Execution-state, defensive CAV reserve, cone-QA og selection persistence — aktuel gameplay baseline

F30S ændrer betydningen af den blå officerordre-status: **blå betyder kun, at ordren stadig udføres fysisk**. Den blå state er ikke længere et synonym for "seneste standing intent". En ordre går derfor tilbage til rød, når underlagte companies er ankommet, Major-HQ har nået sit mål, Regiment-HQ er faldet på plads, higher-HQ follow er inden for settle-tolerance, og attached CAV ikke længere flytter/reformer/charger.

`FORSVAR HER` og `STOP/HOLD` kan fortsat være den gældende mission/intent i simulationen, men knappen er rød, når ingen længere fysisk udfører en repositionering. Dette adskiller **mission intent** fra **execution state**.

Defensiv CAV er samtidig flyttet til en mere konservativ reservegeometri. Ved `FORSVAR HER` forankres hver CAV-enhed bag det bataljonspunkt, den støtter, i stedet for omkring Divisionens objective. Aktuel QA er ca. **150 m bag battalion anchor + 45 m udad lateralt**. Formålet er, at CAV ligger som reserve/flankesikring bag infantry-linjen og ikke ender foran et kompagni.

Range-cone visual language er nu fælles for Dragon og infantry: den valgte `CLOSE / MED / LONG` range fremhæves tydeligt, mens de øvrige ranges er svagere reference-cones. `HOLD` betyder, at alle ranges kun vises som reference. Dragonens cone-geometri bruger eksplicit spejlede side-rays for at undgå den tidligere asymmetriske/kinkede side.

**TEST/QA-regel:** preussiske infantry range-cones er synlige i test-buildet, også uden normal player selection, så facing, range bands og aktiv fire-policy kan verificeres. Dette er ikke et gameplay-design for fog-of-war: når LOS/FOG bliver autoritativt, skal fjendens cones skjules eller visibility-gates.

Selection persistence er nu en hård command-UX-regel: når spilleren giver en ordre til Division, Brigade, Regiment eller Major, **forbliver det samme HQ selected efter objective/facing commit**. Ordreplacering må ikke fortolkes som et tomt world-click, der rydder selection.

Anti-CAV reaktion er nu bundet til tre samtidige krav: **reel LOS + mål inden for valgt fire-policy range + mål inde i fire-cone**. TEST-synlige fjende-cones er kun QA-visualisering og giver ikke i sig selv target knowledge eller firing authority. Samme LOS-krav bruges til mounted-threat/Square-reaktion.

Cavalry mouse-over bruger nu samme BattleManager-hover pipeline som infantry. Gardehusar/Dragon viser styrke/tab, morale/cohesion, mounted/afsiddet tilstand, formation, AI/order-phase, command parent og — for afsiddet Dragon — fire-policy/range/ammo.

Afsiddet Dragon er i F30S eksplicit delt i **ca. 75 % mobile combat dragons** og **ca. 25 % horse holders**, hvor horse holders og horse park bliver ved dismount-positionen. En normal move-order flytter kun combat group. `STIG OP` væk fra hestene bliver en automatisk **return-to-horses** task; HUD viser `TIL HESTE...`, og remount starter automatisk når combat group reelt er samlet ved horse park. Gather-distance beregnes fra combat-group center, ikke unit-root.

`ANGRIB HER` er fastlåst som en **finite positioning mission** på company-niveau. Når company når sit tildelte attack-slot, skal det holde position/facing og bekæmpe fjender gennem normal fire-policy/cone/range; det må ikke straks få en ny autonomous nearest-enemy chase mission. Parent-facing må heller ikke omskrives hver frame efter arrival; formation-motion ejer den afsluttende visuelle drejning.

Defensive stability må ikke holde execution-state kunstigt aktiv. Når Major-HQ allerede står ved sit stabiliserede defensive mål, skal `HasHqGoal` cleares i stedet for at blive genaktiveret hver frame. Det stopper både falsk blå ordrestatus og gentaget `HqGoalStabilized` logspam.
## v00.00.09f30r Dragon fire-control — aktuel gameplay baseline

F30R lukker den manglende fire-control parity for **afsiddet Dragon**. Når Dragon er dismounted, viser cavalry-HUD nu fire direkte ildpolitikker: **HOLD / CLOSE / MED / LONG**.

- **HOLD:** ingen automatisk måludvælgelse eller karabinild.
- **CLOSE:** engager kun mål inden for 35 m.
- **MED:** engager mål inden for 70 m; dette er default.
- **LONG:** engager mål inden for 100 m.

Fire policy ændrer kun engagement-threshold; den eksisterende ±35° carbine arc, ca. 7 sek. TEST reload, ammunition og combat resolution fortsætter uændret. Alle tre range-cones kan fortsat ses som reference på valgt/afsiddet Dragon, mens den aktive engagement-range fremhæves tydeligere.

Mounted Dragon viser ikke disse fire fire-control knapper. HUD angiver i stedet, at karabin fire-control bliver tilgængelig efter **SID AF**. Mounted fire er fortsat ikke implementeret.

F30R viderefører F30Q som command baseline: pending/aktive officerordrer er blå, BAL `ANGRIB HER` bruger begge bataljoner fremme, committed facing er autoritativ, higher-HQ follow er relativt til formationen, og Major/Regiment/Brigade/Division command-zoner bevares.
## v00.00.09f30q Active-order HUD, balanced attack-front og cavalry scout-design — aktuel gameplay baseline

Officer-ordreknapper bruger nu en særskilt **blå aktiv/pending state**. Rød betyder ingen aktiv mission. Når spilleren vælger en point-order, bliver knappen blå allerede mens objective/facing placeres; efter commit forbliver den blå så længe missionen faktisk udføres. Når en tidsbegrænset mission ikke længere har movement/combat executors, går knappen tilbage til rød. `FORSVAR HER` og `STOP/HOLD` er standing orders og forbliver derfor blå, indtil de erstattes.

`ANGRIB HER` er samtidig justeret for BAL doctrine. Den tidligere BAL-regel kunne holde en hel bataljon ca. 285 m bag angrebsfronten samtidig med, at Major-niveauet også kunne holde et company i reserve. Det gav for meget reserve og kunne se ud som om drag-facing/attack objective blev ignoreret. I F30Q sender **BAL begge bataljoner frem side om side**, mens Majorerne fortsat kan disponere lokal company-reserve. DEF kan fortsat holde én bataljon som regimentsreserve; OFF kan fortsat bruge flankedisposition.

`SPEJD HER` fastlåses som en fremtidig CAV-ordre til den rigtige LOS/fog-of-war-model. Den eksponeres ikke som aktiv runtime-knap endnu, fordi den nuværende prototype stadig har for meget global battlefield knowledge. Når FOG/LOS bliver authority for enemy visibility, skal `SPEJD HER` give en mounted cavalry-enhed et område-/punktmål og drive `SEEK / RECON → CONTACT → SCREEN` uden automatisk charge.
## v00.00.09f30p Committed facing, higher-HQ follow og command zones — aktuel gameplay baseline

F30P gør den retning spilleren trækker ved en point-order til **en del af selve missionen før formation planning**. Det gælder Major-, Regiment-, Brigade- og Division-orders. `FORSVAR HER` skal derfor ikke længere kunne vise en pil i én retning, mens bataljoner/companies beregnes mod en anden automatisk threat/fallback-retning. Samme committed facing bruges til bataljonernes lateral axis, company-slots, company destination-facing, Regiments-HQ rear-position og attached CAV support/reserve-positioner.

Higher-HQ movement følger nu formationsretningen i stedet for world-space hacks. De tidligere faste `+Vector3.forward * 75` / `-Vector3.forward * 95` offsets er fjernet. Brigade ligger i QA ca. **120 m bag Regiment + 65 m relativ lateral separation**, mens Division ligger ca. **145 m bag Brigade - 75 m relativ lateral separation**. Divisionens move speed er løftet til 5,9 m/s for at mindske kunstigt efterslæb under længere repositioneringer. Tallene er QA-værdier og kan tunes efter runtime-video.

Command-zone designet er nu udvidet til alle aktuelle HQ-levels. Når det relevante HQ vælges, vises inner/outer command reach som terrænfølgende cirkler:

- **Major/Battalion:** 320 / 450 m
- **Regiment:** 800 / 1100 m
- **Brigade:** 1350 / 1850 m
- **Division:** 2100 / 2850 m

Inner/outer-cirklerne er i F30P primært visual/QA af command reach. Den langsigtede regel er fortsat, at afstand til HQ påvirker **order/reaction delay, coordination, reserve/flank reassignment, recovery og senere information quality/reporting**, ikke direkte våbenskade eller musket accuracy. Det samme distancegrundlag skal senere bruges af courier/order lifecycle og fog-of-war reporting.
## v00.00.09f30o Higher AI arming, shared target circle og true F29G HUD parity — aktuel gameplay baseline

F30O skelner hårdt mellem **AI enabled** og **mission committed**. Når Division/Brigade AI sættes ON, bliver den underlagte kæde kun armed/ready. Regiment, Major/Battalion, companies og cavalry må ikke generere en implicit mission eller begynde fysisk bevægelse, før en eksplicit higher-HQ ordre faktisk er committed.

Regiment og Battalion har derfor en `AwaitHigherMission` authority-state ved higher cascade. Company Officer AI sættes i HOLD under denne ventetilstand. Cavalry får tilsvarende `VENTER PÅ HQ-ORDRE`; higher AI ON må ikke udløse SEEK eller manøvre mod fjenden. En frisk higher mission frigiver ventetilstanden.

Division/Brigade position-orders bruger nu samme `PrototypeOfficerFacingOrder09F29G` som Regiment/Major. Det betyder samme live objective-circle, click-to-place og drag-for-facing. QA-radius er 64 m for Brigade og 84 m for Division.

Higher-HQ HUD tegnes nu gennem den autoritative `PrototypeUnifiedCommandHud09F29G` renderer. Regiment, Brigade og Division deler derfor samme faktiske runtime-kode for panelgeometri, sektioner, to-rækkers ordregrid, farver og knapdimensioner. Den tidligere separate higher-HQ OnGUI er kun fallback. En eksplicit sort top-edge erstatter den grønne linje over HUD'en.

## Fremtidig LOS/Fog-of-war — cavalry SEEK, RECON og SCREEN

Cavalry skal **beholde en selvstændig SEEK-funktion** som del af den senere LOS/fog-of-war-model. SEEK må dog ikke betyde "rid direkte hen til nærmeste fjende". Rollen er rekognoscering og kontaktbevarelse, ikke automatisk kampkontakt.

Den ønskede taktiske state-sekvens er:

`SEEK / RECON → CONTACT → SCREEN → OPPORTUNITY → CHARGE`

**SEEK / RECON** bruges til at opdage fjendtlige enheder, etablere LOS, identificere type/styrke så langt observationen tillader det og sende observationen op gennem command chain. Når kontakt er etableret, skal CAV normalt bremse, flytte lateralt/flankere og holde observation i stedet for at fortsætte lige imod infantry.

**SCREEN** betyder, at CAV forsøger at bevare kontakt og LOS på sikker afstand. Hvis fjendtligt infantry bevæger sig imod CAV, skal CAV give terræn, repositionere og forsøge at bevare observationsafstand frem for at acceptere unødvendig musketkontakt. CAV bør som udgangspunkt søge flank/rear observationsvinkler frem for at stå direkte foran en infantry-linje.

Stand-off-afstanden skal på sigt være **dynamisk og våbenafhængig**, ikke en fast universel værdi. F30N's nuværende ca. 115/145 m er kun QA-tal. Fremtidig tuning bør tage udgangspunkt i den observerede fjendes aktuelle våbenrange, eksempelvis med designregler i retning af:

- `DesiredReconDistance = max(minimum recon distance, Enemy.EffectiveRange × safety factor)`
- `NeverApproachCloserThan = max(minimum hard safety distance, Enemy.CloseRange × safety factor)`

De præcise faktorer er tuning-data og fastlåses ikke endnu.

**OPPORTUNITY** åbnes først, når den taktiske situation giver mening: målet kan være bundet i infantry-firefight, svækket i morale/cohesion, uorganiseret, flankeret eller på anden måde udsat. Først derefter må autonomous cavalry overveje **CHARGE**.

Når LOS/fog-of-war implementeres, skal observationer kunne bevæge sig op gennem command chain med delay og begrænset informationskvalitet:

`CAV observerer fjende → observation rapporteres til parent HQ → Brigade/Division modtager kontakt efter command/information delay → higher HQ får kendt eller last-known enemy position`

Det betyder, at Division/Brigade senere kan bruge cavalry som rekognosceringsressource uden selv at have direkte battlefield-omniscience.

Vigtig authority-regel: **Division/Brigade AI ON må fortsat ikke automatisk starte SEEK.** SEEK/RECON skal aktiveres af en reel standing task, en eksplicit rekognosceringsordre eller som en defineret del af en allerede committed higher-HQ mission.

### Fremtidig CAV-ordre: `SPEJD HER`

Når den rigtige FOG/LOS-model er aktiv, skal mounted Gardehusar/Dragon kunne få en eksplicit `SPEJD HER`-ordre.

Designregler:

- Ordren vælges som et punkt/område på kortet; objective-circle repræsenterer et rekognosceringsområde, ikke et angrebsmål.
- CAV rider mod området via normal terrain/enemy-avoidance, men må ikke automatisk lukke helt ind på observeret infantry.
- Ved første kontakt skifter adfærden fra `RECON` til `CONTACT/SCREEN`: hold LOS, bevæg lateralt, brug terræn og behold sikker stand-off-afstand.
- Fjendtligt infantry, der aktivt presser mod CAV, skal få CAV til at give terræn og bevare observation frem for at acceptere unødig ildkamp.
- Observationer sendes til `CurrentCommandParent` og derfra op gennem command chain med senere courier/information delay.
- Rapporten skal mindst kunne indeholde observeret enhedstype, omtrentligt antal/styrke, position, tidspunkt og confidence/quality.
- Higher HQ må vise `last-known contact`, når direkte observation mistes; kontakt må ikke blive ved med at være perfekt opdateret uden ny LOS/rapport.
- `SPEJD HER` giver **ikke** automatisk tilladelse til CHARGE. En efterfølgende opportunity/attack authority kræver separat missionregel eller eksplicit ordre.
- Ordren bør kun være synlig/aktiv i CAV HUD, når FOG/LOS-systemet er slået til; uden FOG er den skjult eller disabled for at undgå falsk rekognosceringssimulation.
- Ved afslutning/replacement kan CAV enten blive i SCREEN ved området eller returnere til parent-HQ reserve afhængigt af den senere task-policy.


## v00.00.09f30n Regiment HUD parity, cavalry screen/opportunity AI og anti-cavalry infantry reaction — aktuel gameplay baseline

F30N bruger **Regimental HQ HUD som direkte visuel facit** for Brigade/Division. Paneltema, header, typografi, AI/doctrine-knapper, infofelt, spacing og seks mission-knapper følger samme geometri og palette 1:1. Higher HQ viser fortsat sine egne data, men må ikke have et særskilt visuelt HUD-sprog.

Cavalry attack AI følger nu sekvensen **SCREEN → OPPORTUNITY → CHARGE**. CAV manøvrerer til flank/rear screen-positioner uden for den fjendtlige infantry-krop og venter på et taktisk vindue. Geometri alene er ikke længere nok til charge. Et charge-vindue kan åbnes, når mindst ét dansk company har målet i lokal fire-contact, eller når målets morale/cohesion er tydeligt reduceret.

Hostile infantry er samtidig navigation-obstacles for autonomous cavalry. En direkte rute gennem et preussisk company erstattes af et detour-waypoint; detour er kun et mellempunkt, hvorefter CAV fortsætter mod sit ønskede screen/flank point. Samme safety-regel gælder higher-HQ attack staging.

Preussisk infantry kan nu engagere dansk CAV med normal musketild inden for enhedens aktive fire-policy range. Samme reload, range, accuracy, smoke og cohesion pipeline anvendes som ved infantry-vs-infantry. Treffer reducerer CAV strength/morale/cohesion og kan få en ikke-committed cavalry approach til at faltere.

Mounted-threat/Square assessment er team-neutral. Screening cavalry giver lavere threat score end en committed charge; ved troværdig charge kan enemy Officer AI danne Square. En eksisterende auto-Square får sin threat-timer refresh'et, mens mounted threat fortsat er i nærheden.

## v00.00.09f30m Higher command delegation, mission-visual parity og midlertidig cavalry task-attachment — aktuel gameplay baseline

F30M gør Brigade/Division til reelle delegation-lag i den aktive prototype. Division AI ON cascader gennem Brigade, Regiment, begge Majorer/Battalions, company Officer AI og cavalry Officer AI. Brigade AI cascader tilsvarende fra Brigade og ned. Direkte player-order har fortsat højere authority. RMB på en valgt cavalry-enhed skal derfor altid bryde en inherited higher mission, også når cavalry Officer AI allerede står OFF.

Higher-HQ selection viser nu den eksisterende subordinate plan: company routes, destination footprints, regimental objective circle/cross samt cavalry route/destination ghost. Det er visualisering af samme authoritative mission-state, ikke et nyt movement-system.

Ved **ANGRIB HER** kan cavalry, som organisatorisk ligger ved higher HQ, midlertidigt task-attaches til de angribende bataljoner. OrganicParent ændres aldrig; CurrentCommandParent kan midlertidigt være Major A eller Major B. Hvis både Gardehusar og Dragon er til rådighed, vælges den A/B-fordeling som giver lavest samlet travel cost og dermed mindst unødigt kryds.

Når infantry-angrebet ikke længere har company-missioner under aktiv udførelse, frigives cavalry igen. En allerede committed cavalry charge får lov at afslutte først. Derefter returnerer cavalry til sin tidligere higher parent som **RESERVE** og får et reserve/assemble-goal tæt bag det HQ. En ny ikke-angrebsordre frigiver også den midlertidige attack-attachment.

Ved **FORSVAR HER** får attached cavalry adskilte reserve/support-positioner bag objective. De konkrete offsets er prototype-/QA-værdier og skal senere være doctrine-/scenario-data.

Higher mission-knapper er blå så længe missionen er pending eller faktisk udføres af underlagte enheder. Ankomne company-missioner tæller ikke længere som aktiv udførelse.

Brigade/Division HUD følger samme tre-zone command-sprog som Regiment: **ENHEDSINFO/AI | ORDRER/MISSION | UNDERLAGTE/STATUS/ATTACHMENT**.

## v00.00.09f30l Cavalry visual fidelity og gait polish — aktuel visual baseline

F30L ændrer ikke cavalry gameplay-authority. F30K-reglerne for 4-rank Line/Charge, auto-march Column, 2-abreast bridge, RMB-facing, AI og Dragon fire er fortsat gældende.

F30L genbruger de eksisterende 1:1 figures og forbedrer hestens silhouette/proportioner, rider seating/identitet, tack/equipment og coat-variation. Gardehusar og Dragon skal fortsat kunne genkendes på afstand som to forskellige cavalry-typer.

Mounted animation går fra primært whole-model rocking til artikuleret procedural gait: de fire hesteben/hove bevæges i diagonal par, hoved og hale reagerer på movement, og rider lean/bounce skalerer med MOVE kontra CHARGE.

Ekstra close-detail er LOD-styret og skjules ved høj kamera-altitude. **1:1-count må aldrig reduceres af dette LOD**; kun små detail-renderers må slås fra.

## v00.00.09f30k Auto march, RMB-facing, split Dragon selection og higher-HQ AI — aktuel baseline

F30K flytter cavalry **long-march formation policy ind i cavalry-core**, så den virker både med Officer AI ON og OFF. Mounted MOVE går automatisk til 4-abreast Column ved mindst ca. 140m resterende afstand, hvis ingen gyldig fjende er inden for Long. Ved ca. 90m eller mindre deployerer den tilbage til 4-rank Line. 140/90 hysterese reducerer oscillation. Enemy inside Long tvinger Line; bridge-route har stadig højere prioritet og bruger 2-abreast.

Cavalry følger nu samme RMB-control som infantry: **hold højre mus på destinationen og træk i ønsket facing-retning**. Drag på mindst ca. 4m gemmer en explicit final facing. Destination ghost og den færdige formation bruger samme facing.

Dismounted Dragon selection er opdelt i en box omkring combat-line og en mindre box omkring horse-holder/hestegruppen med en tynd forbindelseslinje. Den tidligere store tomme firkant mellem grupperne er superseded.

SID AF beholder ca. 2,25s transition. STIG OP/remount er sat til ca. **4,5s**, så tilbagebevægelse og remount er tydeligere.

Brigade og Division får samme AI/doctrine command-sprog som Regiment/Battalion: **AI ON/OFF + DEF/BAL/OFF**. Begge higher-HQ AI states starter OFF. Tilknyttet cavalry Officer AI må kun arbejde autonomt hvis det aktuelle parent-HQ har AI ON. Higher-HQ doctrine føres ind i den eksisterende Regimental mission-decomposition.

## v00.00.09f30j Dismounted Dragon fire, horse holders og formation-anchor — aktuel baseline

F30J retter først cavalry footprint-anchor: **4-rank Line er center-anchored**, mens **4-abreast Column og 2-abreast bridge column er front-anchored og strækker sig bag unit root**. Selection rectangle, destination ghost og BoxCollider bruger nu samme anchor-regel som rider slots.

Dragon får derefter reel dismounted fire-adfærd. Som prototypeværdi bliver ca. **25 % horse holders** ved hestene, mens ca. **75 % combat group** går ca. **18 m frem** og danner en to-geleddet skydelinje. Forholdet er QA/designværdi og skal senere kunne være doctrine-/scenario-data.

Når Dragon er fuldt afsiddet og fysisk reformeret, får den valgte enhed Close/Medium/Long cones på **35/70/100 m** med **70° total fire sector**. Mens den står HOLD, vælger den nærmeste gyldige preussiske Regiment-target i cone og skyder carbine-volley. TEST reload er 7 sekunder og TEST ammo 20 rounds/man. Fire producerer directional black-powder smoke.

STIG OP flytter foot figures tilbage mod horse-side remount slots som del af transitionen, før de skjules. Mounted fire er fortsat ikke implementeret.

## v00.00.09f30i Cavalry authority, single HUD, order visuals og animation — aktuel baseline

F30I gør cavalry **MANUEL ved spawn**. Gardehusar/Dragon må ikke selv ride væk fra deres fire-geleds startopstilling; Officer AI aktiveres kun eksplicit. AI ON/OFF-knappen er autoritativ og kan toggles begge veje uden F30H-race condition.

Cavalry har nu én bottom-HUD-ejer. Det gamle F30C cavalry control strip og F2 bottom bar tegner ikke længere parallelle lag.

Selected cavalry markeres med en **gul formation-sized rectangle**, der følger og glider mellem 4-rank Line, 4-abreast Column og 2-abreast bridge footprint. Ved aktiv bevægelsesordre vises en vedvarende order-line og en gul destination ghost-box i den planlagte slutformation.

Dragon **SID AF / STIG OP** har en procedural overgang på ca. 2,25 sekunder i stedet for instant visibility toggle. Mounted MOVE/CHARGE har en enkel procedural riding gait; dette er midlertidig animation indtil riggede horse/rider assets.

F30E 1:1 expansion afsluttes nu med ét initialization-snap efter alle 120/140 figures er skabt, så startformationen faktisk står i fire geledder fra første QA-billede. Senere formation changes er fortsat fysisk animerede.

OOB-panelet er udvidet fra 452 til **535 px** for at give enhedsnavne mere plads uden at ændre de faste MÆND/STATUS/AI/TILK-kolonner.

## v00.00.09f30h Cavalry 4-rank, bridge, HUD, NATO og selection — aktuel baseline

F30H fastlåser en ny PROJECT 1864 cavalry-standard for den nuværende 1:1 tactical scale:

- **Normal mounted Line = 4 geledder.**
- **Charge Line = 4 geledder.**
- **Normal mounted Column = 4 abreast.**
- **Bridge/defile column = 2 abreast.**
- Gardehusar = 120/120 synlige mounted riders.
- Dragon = 140/140 synlige mounted riders.

Den tidligere 2-rank cavalry-line er dermed **superseded** i runtime. Beslutningen er gameplay-/projektkanon for denne prototype og er ikke en påstand om, at alle historiske kavalerienheder altid stod sådan i alle situationer.

Formation-change er fysisk: rytterne bevæger sig til deres nye slots med synlig reform-progress. Charge må ikke opnå fuld charge-hastighed mens formationen stadig er under reformering. Bridge crossing er en vedvarende transaction: **NearBank → FarBank → ExitBank → Direct**. Nye AI-/spiller-mål må opdatere den endelige destination, men må ikke nulstille en aktiv crossing. Hele 1:1 to-abreast-kolonnen skal fri af broen før normal formation gendannes.

Cavalry HUD bruger samme visuelle 90px-baseline som company HUD: header, ENHEDSINFO, AI/DOKTRIN, VÅBEN/TILSTAND, ORDRER/BEVÆGELSE og FORMATION med samme green/red/neutral state language. HUD viser reel F30C Officer AI phase, target og CurrentCommandParent samt bridge/reform-status.

Den autoritative semantic-zoom-owner er **PrototypeSemanticZoomUnified09F29V**. F30H tilføjer Gardehusar/Dragon som **I/CAV** med navne og Brigade/Division som **X/XX HQ** med navne. Strategic mesh suppression omfatter disse entities. Close-view labels viser tilsvarende enheds-/HQ-navne.

RTS box-selection kan vælge cavalry eller higher HQ når boksen ikke indeholder infantry. Den nuværende command model er fortsat single-select for cavalry/higher HQ; nærmeste eligible entity til marquee-center vælges.

F30H retter desuden OOB-blank-row regressionen: row-button-eventet behandles før tekst/visuals, så Division/Brigade/Regiment/Battalion/Company rows igen er synlige.

## v00.00.09f30g OOB input og HQ/cavalry visibility — aktuel baseline

F30G retter fire konkrete regressioner observeret i F30F QA. Det gamle `PrototypeOobStatus09F29V` tegnede fortsat sin sorte `190 KLAR`-statuspatch oven på det nye fixed-column OOB; F30G gør F30D+ OOB til eneste visuelle OOB-ejer og deaktiverer legacy infantry/cavalry/status-renderere.

Cavalry-rækkerne bruger nu ikke længere `GUI.Button` oven i drag-state-maskinen. Interaktionen er eksplicit: **single-click = vælg**, **double-click = fokus bag enheden**, **hold + flyt = drag**, og **slip over Division/Brigade/Regiment/Major A/Major B = ændr CurrentCommandParent**. Drag/drop ændrer fortsat kun tactical attachment; OrganicParent bevares.

Semantic zoom er udvidet med **I/CAV** counters for Gardehusar og Dragon samt **X/XX HQ** counters for Brigade og Division. Strategic mesh suppression/restoration omfatter nu også cavalry og higher HQ, så 3D meshes og counters ikke konkurrerer. Ved close zoom er enheder/HQ fysiske 3D-entities; ved højere zoom bliver de læselige som counters.

For QA-læsbarhed er Gardehusar/Dragon startposition flyttet tættere på den danske formation. Brigade-HQ følger ca. 125 m bag Regimental HQ og Division-HQ ca. 190 m yderligere bag Brigade i stedet for de tidligere 265/355 m. Higher HQ placering clamped til battlefield bounds. Dette er QA-placement/hotfix, ikke en historisk doktrinregel for faste HQ-afstande.

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
- **v00.02.78 / LIVGARDEN 3D INFANTRY IMPORT + 1:1 VISUAL CORE** — brugerleveret Livgarden skeletal mesh, 65 shared-skeleton animationer og to riflemeshes har fået source-staging, automatiseret Unreal-import, company-level 1:1 visual renderer, formation-slot sampling, weapon attachment og første stance/combat-animation mapping. **Source/runtime code implemented; UE 5.8.3 asset import + build/runtime QA pending.**

- **v00.02.79 / UE-P522–531 FIRE-DRILL RESEARCH PROGRESSION** — FrontRank-start, research-gated TwoRankFire/FireByRank/Volley/Independent, Advanced automatic-selection capability, rank-aware participation/cadence, legacy-mode migration og specialist/QA persistence. **Code implemented; UE 5.8.3 build/runtime QA + rank-specifik soldier animation pending.**
- **v00.02.78 / UE-P512–521 LIVGARDEN 3D INFANTRY IMPORT + 1:1 VISUAL CORE** — shared skeleton import, 65 animationer, rifle/rifle+bajonet og company-level 1:1 skeletal renderer. **Source/runtime implemented; UE import/build/runtime QA pending.**
- **v00.02.77 / UE-P502–511 SPECIALIST STATE + QA** — specialist snapshot/restore, deterministic digest, invariant validation, reset/leak checks og QA-fixture integration. **Code implemented; UE 5.8.3 build/runtime QA pending.**
- **v00.02.76 / UE-P492–501 WORKING PARTIES** — worker allocation, NCO-weighted work rate, ammunition party, stretcher/wounded collection, dig, breach og defensive-position repair hooks. **Code implemented; QA pending.**
- **v00.02.75 / UE-P482–491 DEFENSIVE OCCUPANCY** — occupy/leave/search/auto-occupy, infantry/artillery protection, gun-emplacement capability, threat-facing og fieldworks auto-occupancy. **Code implemented; QA pending.**
- **v00.02.74 / UE-P472–481 MORTAR FIRE** — bomb inventory, unit/area/fortification mission state, min/max range, high arc, time-of-flight, reload, casualty/structural resolution, Hold Fire og resupply. **Code implemented; generic area-effect expansion/tuning remains later.**
- **v00.02.73 / UE-P462–471 MORTAR DEPLOYMENT** — LightHand/HeavySiege, mortar battery actor, Transport/Emplacing/Deployed/Packing, work-rate, horses/wagons, piece weight/manhandling og QA spawn. **Code implemented; QA pending.**
- **v00.02.72 / UE-P452–461 FORTIFICATION ASSAULT** — preparation/breach state machine, ladders, planks, axes, crowbars, explosive charges, working-party strength, exposure/crossing modifiers og breach damage. **Code implemented; QA pending.**
- **v00.02.71 / UE-P442–451 DEFENSIVE POSITIONS** — typed physical RiflePit/Breastwork/Trench/GunEmplacement/Barricade/Abatis/Redoubt actors med orientation, condition, capacity, ownership, directional cover, repair, capture og navigation lifecycle. **Code implemented; QA pending.**
- **v00.02.70 / UE-P432–441 FIRE DRILL + KNEELING** — explicit Kneeling stance, loading method, FrontRank/Volley/Independent/Alternating/KneelingFrontRank drill, firing fraction, reload og volley coordination. **Code implemented; QA pending.**
- **v00.02.69 / UE-P422–431 NCO COMMAND CONTINUITY** — `NCOStrength`, `NCOQuality`, officer-loss fallback, command continuity og integration i reform, reload, rally, response delay og detachment control. **Code implemented; QA pending.**
- **v00.02.68 / UE-P412–421 SPECIALIST DETACHMENTS** — temporary detachments med OrganicParent, strength/ammo reservation, casualties, task anchor, recall/reform og parent fire-strength reconciliation. **Code implemented; QA pending.**

- **v00.02.42 / DOC CONSOLIDATION — P0A v00.00.09f30x** — samlet aktuel taktisk baseline efter F30X direkte i hovedmanualen: command authority, execution-state, DefendHere authority, 0,50 m final-slot arrival, early Line deployment, 72/68 m company spacing, TEST enemy cones, CAV 4-rank/4-abreast/2-abreast bridge-regel, Dragon split/remount, HQ/command zones og river routing. Ingen gameplayændring; runtime-baseline forbliver F30X.


- **v00.02.41 / P0A v00.00.09f30x EARLY INFANTRY DEPLOY + COMPANY SPACING + CAV BRIDGE APPROACH + STARTUP ENEMY CONES TEST** — infantry deployer ved enemy MaximumRange +35 m; company spacing 72 m/minimum 68 m; CAV 2-abreast først ca. 36 m før broen; enemy TEST-cones oprettes straks når enemy Regiment findes.


- **v00.02.40 / P0A v00.00.09f30w ENEMY CONE QA + SAME-BANK RIVER ROUTING + OOB AI CONSISTENCY TEST** — enemy cones altid synlige i TEST; same-bank river chords bruger bank-follow i stedet for bridge round-trip; OOB AI er konsekvent ON/OFF; background HQ follow/CAV reform holder ikke ordreknap blå.


- **v00.02.39 / P0A v00.00.09f30v SINGLE FINAL-SLOT ARRIVAL AUTHORITY TEST** — F27/F29E/F29L bruger samme 0,50 m final-slot tolerance; parent HUD validerer fysisk afstand + aktiv destination og kan ikke blive rød før companies faktisk er fremme.


- **v00.02.38 / P0A v00.00.09f30u DEFEND COMMAND AUTHORITY + HQ GOAL CONFLICT FIX TEST** — committed FORSVAR HER ejer defensive HQ-goals; HqDepthGuard yields for Major/Regiment under DefendHere; competing settled HqGoal cleares; stopper DEF-STABILITY/HQ-DEPTH tug-of-war og falsk aktiv ordrestate.


- **v00.02.37 / P0A v00.00.09f30t CROP TUFT VISUALS + INFANTRY ACTIVE RANGE AUTHORITY TEST** — lange crop-beams erstattet af korte terrænfølgende crop-tuftenheder; FireVisuals er autoritativ for infantry cone-emphasis; aktiv fire-range stærk, øvrige svage; enemy QA-cones følger samme regel.


- **v00.02.36 / P0A v00.00.09f30s EXECUTION-STATE ORDERS + DEFEND CAV RESERVE + CONE QA + SELECTION PERSISTENCE TEST** — blå ordrestate følger fysisk execution og går rød ved settle; defensiv CAV bag battalion anchors; Dragon cone symmetri; aktiv range fremhævet på Dragon/infantry; fjendens cones synlige som TEST-QA; HQ-selection bevares efter ordrecommit.


- **v00.02.35 / P0A v00.00.09f30r DRAGON FIRE CONTROL + F30Q COMMAND STATE BASELINE TEST** — afsiddet Dragon får HOLD/CLOSE/MED/LONG 0/35/70/100 m fire-control; MED default; range-cones fremhæver aktiv band; mounted fire-control skjult indtil SID AF; F30Q command-state baseline videreføres.


- **v00.02.34 / P0A v00.00.09f30q ACTIVE ORDER BLUE + BALANCED ATTACK FRONT + CAV SCOUT DESIGN TEST** — pending/aktive officerordrer vises blå; mission lifecycle styrer retur til rød; BAL ANGRIB HER bruger begge bataljoner fremme med lokal Major-reserve; SPEJD HER fastlagt som fremtidig FOG/LOS-baseret CAV rekognosceringsordre.


- **v00.02.30 / P0A v00.00.09f30n REGIMENT HUD PARITY + CAV SCREEN/OPPORTUNITY AI + ANTI-CAV INFANTRY REACTION TEST** — higher-HQ HUD bruger Regimentets layout/palette 1:1; cavalry screen/stand-off og engagement-gated charge; enemy infantry avoidance/detour; preussisk musketild mod CAV; team-neutral mounted-threat/Square reaktion.


- **v00.02.29 / P0A v00.00.09f30m HIGHER COMMAND DELEGATION + TEMP CAV ATTACHMENT + MISSION VISUAL PARITY TEST** — Division/Brigade AI cascade gennem command tree; higher selection viser subordinate routes/destinationer/objective; persistent blå active-order state baseret på reelle executors; FORSVAR HER giver cavalry reserve/support slots; ANGRIB HER task-attacher cavalry midlertidigt til Major A/B uden at ændre OrganicParent, vælger anti-crossing A/B pairing, og returnerer cavalry til tidligere parent som RESERVE efter attack completion/committed charge; higher HUD aligned med Regiment layout.


- **v00.02.28 / P0A v00.00.09f30l CAVALRY VISUAL FIDELITY + GAIT POLISH TEST** — visual-only pass oven på F30K: horse body/chest/neck/head/leg/hoof/tail proportions opgraderet; seks deterministic coat-toner; Gardehusar sabretache og Dragon cartridge-box detail; tack/stirrups; sparse horse markings; close-detail LOD over ca. 210m; articulated Leg0-3/Hoof0-3 gait, head nod, tail swing og speed-dependent rider lean; 1:1 120/140-count bevaret.

- **v00.02.27 / P0A v00.00.09f30k AUTO MARCH COLUMN + RMB FACING + SPLIT DRAGON SELECTION + HIGHER HQ AI HUD TEST** — cavalry long-march policy flyttet til shared core med 140/90m hysterese og enemy-inside-Long deploy; RMB hold+drag final facing som infantry; destination ghost følger explicit facing; dismounted Dragon selection opdelt i combat/horse boxes; STIG OP ~4,5s; Brigade/Division HUD får AI ON/OFF + DEF/BAL/OFF; parent-AI gating for attached cavalry og OOB AI-status.

- **v00.02.26 / P0A v00.00.09f30j DISMOUNTED DRAGON FIRE + HORSE HOLDERS + FORMATION ANCHOR FIX TEST** — Line/Column/bridge selection- og ghost-anchor samlet med rider-slot anchor; collider center følger footprint; Dragon SID AF deler prototypevisuelt i ca. 25% horse holders + 75% combat group 18m frem i 2-rank firing line; 35/70/100m +/-35deg carbine cones; auto fire under HOLD efter transition/reform; 7s TEST reload, 20 rounds/man TEST ammo og black-powder smoke; STIG OP kalder foot figures tilbage mod hestene.

- **v00.02.25 / P0A v00.00.09f30i CAVALRY AUTHORITY + SINGLE HUD + ORDER VISUALS + ANIMATION TEST** — cavalry AI default OFF/MANUEL; AI toggle race fix; sole cavalry HUD owner; legacy bottom layers retired visuelt; 535 px OOB; 1:1 initial four-rank snap; yellow formation-sized selection rectangle; persistent order path + destination ghost; animated Dragon SID AF/STIG OP; procedural mounted gait.

- **v00.02.24 / P0A v00.00.09f30h CAVALRY 4-RANK + BRIDGE + HUD + NATO + SELECTION HARDENING TEST** — cavalry normal Line og Charge fastlåst til 4 geledder; normal Column 4-abreast; bridge/defile 2-abreast; fysisk reformering med progress og reduceret charge-speed under reform; persistent bridge transaction med ExitBank-clearance for fuld 1:1 kolonne; company-style cavalry HUD; authoritative F29V semantic counters/navne for cavalry + Brigade/Division; box-selection udvidet til cavalry/higher HQ; OOB blank-row draw-order fix; let cavalry visual-detail pass.

- **v00.02.23 / P0A v00.00.09f30g OOB INPUT + HQ/CAVALRY VISIBILITY HOTFIX TEST** — legacy F29V `190 KLAR` overlay fjernet; F30D+ gjort eneste OOB-renderer; cavalry single/double-click og drag/drop event conflict rettet; Gardehusar/Dragon får I/CAV semantic counters; Brigade/Division får X/XX HQ counters; strategic mesh suppression udvidet til cavalry/higher HQ; QA start/follow-afstande strammet og higher HQ clamped til battlefield bounds.

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