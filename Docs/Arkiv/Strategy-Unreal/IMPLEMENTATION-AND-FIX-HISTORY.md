# PROJECT 1864 — Implementation & Fix History

**Konsolideret ved v00.00.09f30x TEST**  
**Gameplay-baseline: v00.00.09f30x**  
**Unity-baseline: 6000.6.0f1**

Dette dokument er den samlede, kronologiske registrering af de funktioner og kendte fejlrettelser/hardening-trin, der er implementeret i den taktiske prototype frem til og med F30X. F30F konsoliderede historikken; F30G tilføjede OOB/input/visibility-hardening; F30H konsoliderede cavalry formation/bridge/HUD/semantic zoom/selection; F30I authority/order-visual/animation; F30J dismounted Dragon fire og formation-anchor.

> Statusregel: "implementeret" betyder at koden er lagt i repository. Seneste builds er fortsat TEST indtil de er runtime-verificeret i Unity uden røde compilerfejl.

## 1. Fundament — før den nuværende F-serie

### v00.00.01–v00.00.08 / designbaseline v00.02.01–v00.02.08
Implementeret:
- Første P0A Unity battle-prototype og repository-layout.
- Unity project root fastlåst til repository-roden.
- Unity editor metadata/version normaliseret og baseline flyttet til Unity 6000.6.0f1.
- Built-in IMGUI, Particle System, Physics og Audio moduler aktiveret.
- Våbenprofil/reload, experience, volley feedback, ammunition/casualty model og første expanded systems baseline.
- Shared OfficerAIController/OfficerProfile og første command/AI-retning.

Fejlrettelser/hardening:
- Compile-fejl CS0136 i Regiment.cs rettet ved at fjerne lokal navnekollision.
- Deprecated FindFirstObjectByType-kald erstattet hvor de gav problemer; unused state fjernet.
- Manglende Unity-moduler i projektmanifestet rettet.
- Statisk QA-hardening før den taktiske F-serie.

## 2. Tidlig tactical-command serie

### v00.00.09f2 — Isolation baseline
- Isolerede movement/command-ejere for at reducere konkurrerende scripts.
- Etablerede TEST-versionering og tydelig buildmarker.

### v00.00.09f4 — 1:1 infantry visuals, orders, flags
- Infantry-retningen skiftede til 1:1 soldier visuals.
- Ordrer, flags/standards og formation presentation blev knyttet tættere til den simulerede styrke.

### v00.00.09f5 — Company footprint + hover
- Company-scale footprint, hover/selection og fysisk formationslæsning.
- Grundlag for senere selection/formation-collider hardening.

### v00.00.09f6 — Selection, range, march column
- Selection-flow, range-visning og march-column.
- Formation skifter mellem taktisk Line og march Column.

### v00.00.09f7 — Volley, movement, range
- Volley-/movement-/range-pipeline samlet.
- F7 blev senere den autoritative visible soldier renderer.

### v00.00.09f8 — Range cone, morale, turning
- Close/Medium/Long cones, morale/cohesion og formation turning.
- Range/facing blev del af combat eligibility.

### v00.00.09f9 — Bridge attack routing + combat QA
- Bridge-only river crossing og attack routing.
- Combat QA-regler og river authority begyndte at blive samlet.

### v00.00.09f11 — Terrain/pathfinding/map
- Terrain, lokale blockers, pathfinding og map-feedback integreret.

### v00.00.09f12 — Route authority fix
Fejlrettelse:
- Konkurrerende route-writers blev adskilt, så én movement owner har fysisk destination ad gangen.
- Reducerede units der blev trukket mellem inkompatible route states.

### v00.00.09f14 — Combat lethality calibration
- Kalibrerede close-range lethality og casualty resolution.
- Senere terrain/concealment multipliers kunne kobles ind uden at blive overskrevet af hit-cap.

## 3. Major / battalion command

### v00.00.09f15 — Major HQ + map UI
- Fysisk selectable Major HQ, seks battalion orders og genbrugelig HQ UI theme.
- Battlefield udvidet og miljø poleret.
- Andet dansk company holdt aktivt og normaliseret til 190 mand.

### v00.00.09f16 — HQ selection/UI/formation
- HQ-selection og formation UI koblet til tactical authority.

### v00.00.09f17 — Four-company battalion
- Fire kompagnier under Major.
- Reserve/flank roller og attack lanes.

### v00.00.09f18 — Unified AI command authority
- Major/Company authority samlet, så parent mission og local Captain AI kan sameksistere.

### v00.00.09f19 — Discretionary reserve
- Reserve blev en reel rolle/state frem for permanent hardcoded bonus.

### v00.00.09f20 — Selection intent/doctrine
- Selection adskilt fra command intent.
- Doctrine/AI-toggle blev tydeligere command-state.

### v00.00.09f21 — Manual attack authority
Fejlrettelse:
- Direkte spillerordre fik autoritet over lokal AI, så AI ikke straks overskrev manuelle attack-orders.

### v00.00.09f22 — Major formation-selection HUD
- Formation/HUD-styring for Major og companies.

### v00.00.09f23 — Authority + formation slot safety
Fejlrettelse/hardening:
- Ugyldige formation slots og konkurrerende authority writers blev filtreret.
- River/terrain safety kunne korrigere en slot uden at overtage hele missionen.

### v00.00.09f24 — Major mission destination authority
Fejlrettelse:
- Major mission destination blev gjort autoritativ, så lokale scripts ikke omskrev målet under bevægelsen.

### v00.00.09f24a — AI re-enable authority hotfix
Hotfix:
- Rettede authority-state efter AI OFF/ON, så re-enabled AI igen kunne overtage uden stale manual ownership.

### v00.00.09f25 — Infantry charge/local authority/command zone
- Infantry CHARGE og melee-baseline.
- Local Captain authority og command-zone handling.

### v00.00.09f26 — Under-fire reaction
- Under-fire reaction uden at ødelægge parent mission.
- Lokalt svar på fjendtlig ild med højere mission bevaret.

## 4. Regiment og højere infantry hierarchy

### v00.00.09f27 — Two battalions/shared Major core
- To bataljoner, to Majorer og otte kompagnier.
- Shared hierarchy/mission core.

### v00.00.09f28 — Regimental HQ
- Fysisk Oberstløjtnant HQ.
- Kommandokæde Oberstløjtnant → Major → Kaptajn.
- Regimental orders og mission allocation.

### v00.00.09f29 — Infantry Square foundation
- SQUARE/KARRÉ, formation time, mounted-threat API og reduceret square fire effectiveness.

### v00.00.09f29b — Battlefield + Attack AI hardening
- Battlefield skaleret til 5760 × 3840 m.
- To Prussian QA companies.
- Sticky explicit AttackTarget og single physical movement owner.

### v00.00.09f29c — Unified command HUD
- Kompakt fælles Major/Company HUD.
- HOLD/CLOSE/MED/LONG, withdrawal, forced march, charge, STOP og LINE/COLUMN/SQUARE.
- Visible company names 1.–8. KOMPAGNI.

### v00.00.09f29d — Semantic zoom + NATO
- CLOSE/MEDIUM/OPERATIONAL/STRATEGIC semantic zoom.
- NATO I/II/III counters og strategic mesh suppression uden at stoppe simulation.

### v00.00.09f29e — Regimental defense/objective/attack coordination
- Persistent regimental objective, full command-chain lines og company mission footprints.
- Defensive frontage og coordinated attack slots.
Kendt regression:
- Ny Oberstløjtnant HUD blev ikke pålideligt synlig og blev derfor erstattet i F29F.

### v00.00.09f29f — Regimental HUD visibility fix
Fejlrettelse:
- Legacy F28 HUD dækkede/overlevede den nye HUD.
- Ny separat PrototypeRegimentalHud09F29F med høj GUI-prioritet, opaque panel og korrekt runtime disable af eksperimentel overlay.

### v00.00.09f29g — Contact combat + facing + unified HUD
- Hele selection command tree.
- Fire-policy som reel contact threshold.
- Click/drag facing orders.
- Ny higher order canceller local fighting withdrawal.
- Continuous river ribbon.
Fejlrettelse:
- F29E coordinated slot writer deaktiveret, så den ikke konkurrerede med F27 movement/local Captain authority.

### v00.00.09f29h — Compile hotfix
Hotfix:
- CS0104 Object ambiguity mellem System.Object og UnityEngine.Object rettet med eksplicit alias.

### v00.00.09f29i — Tactical visibility
- Close-zoom HQ labels, facing arrow, order/objective tags og KAMP/UNDER ILD overlay.

### v00.00.09f29j — Pre-contact Line deployment
- Column tilladt uden for engagement range.
- Line deployment før enemy MaximumRange.
- Hysteresis mod Line/Column oscillation.
Senere F30A-hardening:
- deployment flyttet tidligere, ca. 35 m uden for MaximumRange, så reform er færdig før skudhold.

### v00.00.09f29k — Charge right-click targeting
Fejlrettelse:
- Charge target-pick gjort eksplicit.
- RIGHT CLICK confirmer charge; samme klik må ikke også blive normal move/attack.
- Empty terrain under armed charge må ikke flytte formationen.

### v00.00.09f29l — Battlefield hardening
- Major/Oberstløjtnant mission-follow.
- Continuous river renderer.
- Defensive arrival hysteresis.
- Square visual positioning.
Fejlrettelser:
- HQ laggede ikke længere ekstremt langt bag missionen.
- Defensive companies stoppede med at shuttle/mikrokorrigere rundt om ankomstslot.

### v00.00.09f29m — Square active-renderer hotfix
Kritisk hotfix:
- Square state var aktiv, men visible soldiers blev i Line/Column.
- Root cause: Square skrev til F5-renderer, mens F7 var den aktive renderer.
- Ny final Square writer mod F7 LocalPositions.
- Legacy selection footprint skjules under Square og collider reassertes.

### v00.00.09f29n — Charge melee + HQ spacing
Fejlrettelser/hardening:
- Charge låst til Line og HOLD FIRE.
- Contact distance reduceret til reel deep melee overlap.
- Fire cones skjules under charge/melee.
- Major/Oberstløjtnant HQ follow-distance reduceret.

### v00.00.09f29o — Enemy threat/contact visibility
Fejlrettelser:
- Attack contact fanger enemy inden for fire-policy range selv uden for nuværende facing.
- Sticky contact buffer reducerer combat/march oscillation.
- FencePosts/Trees fjernet fra hard-detour-listen efter log viste gentagen FencePost replanning.

### v00.00.09f29p — Tactical map/camera/Side Step
- Tactical map, camera focus/behind, HQ shortcuts.
- Formation-preserving lateral Side Step.
- Captain/HQ visual polish.

### v00.00.09f29q — OOB + HQ discoverability
- Collapsible OOB, click/double-click selection/focus.
- Strength/state i rows.
- Semantic zoom threshold polish.

### v00.00.09f29r — Crop concealment/map polish
- Crop fields som gameplay terrain med concealment, movement friction og temporary reveal.
- Terrain multiplier ind i lethality pipeline.

## 5. Square/UI hardening efter F29R

### v00.00.09f29s — Square sector fire
- Square opdelt i retningsbestemte fire sectors i stedet for 360° full-company fire.

### v00.00.09f29t — Interactive tactical map
- Tactical map gjort mere interaktiv uden at overtage movement authority.

### v00.00.09f29v — Tactical UI/Square corrections
Fejlrettelser/hardening:
- Square orientation/facing låses mere stabilt.
- Range/sector visual correction og formation transition state forbedret.

### v00.00.09f29w — HQ depth guard
Fejlrettelse:
- HQ-depth/GUI ordering guard mod HUD-elementer der blev skjult bag andre overlays.

### v00.00.09f29x — Square fire + OOB selection hardening
Fejlrettelser:
- OOB-selected HQ/company selection beskyttet under point-order flow.
- Square/fire transition state hardenet mod legacy writers.

### v00.00.09f29y — Defensive stability/UI/terrain
Fejlrettelser:
- DefendHere bank/slot stability.
- Reserve/flank fjernet fra HQ anchorberegning.
- TEST enemy AI reel ON/OFF.
- Major HUD status normaliseret med OOB.
- River bridge rendering og crop terrain visuals poleret.

### v00.00.09f29z — Square face fire + directional smoke
- Fire uafhængige 90° Square faces.
- Ca. 25% firepower pr. side, independent reload og sidekorrekt black-powder smoke.
Fejlrettelser:
- Ingen shot/smoke uden target.
- Fractional ammo gør fire quarter-face volleys ≈ én normal round/man.
- Legacy reflected full-company volley undertrykt.
- Square logical faces låst til Square rotation i stedet for at rotere med target.

## 6. F30 cavalry og higher command

### v00.00.09f30 — First cavalry core
- Gardehusar + Dragon shared mounted core.
- LINE/COLUMN, move/hold/charge, FRONT/FLANK/REAR contact.
- Bridge-only mounted crossing.
- Dragon SID AF/STIG OP.
- Ready Square giver cavalry FALTER.
- Første version havde midlertidigt F10 Cavalry TEST-panel og proxy visual count.

### v00.00.09f30a — Command/Square/Officer hardening
Implementeret:
- OOB/HUD selection persistence.
- Persistent active regimental-order button state.
- Authoritative DefendHere objective/facing.
- Strongest-company attack allocation efter mænd + erfaring.
- Earlier pre-contact Line deployment.
- Friendly fire-lane Side Step.
- Cavalry flyttet fra F10 testpanel til normal battlefield/OOB/bottom-HUD selection.
Fejlrettelser:
- Square footprint skjules fra FORMING.
- Square cone/range terrain clipping hardenet.
- LINE↔SQUARE må ikke skabe falsk smoke.
- DefendHere må ikke få facing fra point-minus-HQ fallback.
- HUD click må ikke deselecte OOB-valgt enhed.

### v00.00.09f30b — Higher Command HQ + Attachment
- Fysisk Division HQ + Brigade HQ.
- XX Division → X Brigade → III Regiment → II Battalion → I Unit.
- OrganicParent/CurrentCommandParent/AttachmentType.
- Higher mission delegation til eksisterende regiment/missionsmotor.
Fejlrettelser:
- Runtime NullReference hardening i charge target/fire target pipeline.
- Command-tree gjort symmetrisk: valgt company/HQ/cavalry viser hele relevante træ både op og ned.
- Particle repair logspam reduceret.

### v00.00.09f30c — Cavalry Officer AI + Dynamic Attachment + Manual Override
- Cavalry Officer AI søger FLANK/REAR før charge og undgår deliberate ready-Square charge.
- Dynamic attachment Division/Brigade/Regiment/Major A/Major B.
- OOB row flytter live med CurrentCommandParent.
- Direct player order sætter cavalry i MANUAL; AI kan genaktiveres.
- WAIT PARENT AI ved attachment til Major med AI OFF.
Fejlrettelse/hardening:
- Player authority beskyttet mod at parent/cavalry AI straks overskriver direkte ordre.

### v00.00.09f30d — OOB Scroll/Drag-Drop/Visual polish
Implementeret:
- Fixed OOB columns: unit / men / status / AI / attachment.
- Rigtig scroll view med fixed header.
- Drag-and-drop cavalry attachment direkte i OOB.
- Officer role fjernet fra structural row label; fx "1. REGIMENT" i stedet for tekst-overlap med "OBERSTLØJTNANT".
- Første horse-leg/detail pass og cavalry visual density 1:5.
Fejlrettelser:
- OOB text overlap rettet.
- Layout gjort fremtidssikkert til større OOB.
- HQ-heste fik fire ben/hove i stedet for benløs block-look.

### v00.00.09f30e — 1:1 Cavalry + historical visual fidelity
Implementeret:
- Gardehusar: 120 mand = 120 visible mounted riders.
- Dragon: 140 mand = 140 visible mounted riders.
- Dragon SID AF: 140 dismounted figures; 140 horses bliver ved horse-holder position.
- Alle extra figures føjes til eksisterende Line/Column/dismount/remount lists.
- Gardehusar/Dragon får forskellig uniform/headgear/equipment silhouette.
- Heste får body/chest/neck/head/muzzle, fire legs/hooves, mane/tail/ears, saddle/cloth, bridle/reins og flere horse tones.
- Brigade/Division mounted staff får arms/legs/boots/headgear/sabre.
- 1:1 collider footprint følger Line/Column og mounted/dismounted state.
Fejlrettelser/hardening:
- Legacy F30/F30D rider geometry registreres som mountedRiders, så SID AF ikke efterlader ghost riders.
- 1:1 collider reassertes efter senere formation/mode change, så F30 core ikke reducerer footprint tilbage til gammel proxy.
- F30D 1:5 visual proxy er superseded af 1:1.
- WEDGE ikke implementeret som standardformation; LINE/COLUMN forbliver aktive. ECHELON LEFT/RIGHT er næste planlagte cavalry formation.

### v00.00.09f30f — Implementation & Fix History Consolidation
- Ingen combat/movement-regler ændret fra F30E.
- VERSION.txt gjort til kort aktiv buildstatus.
- docs/IMPLEMENTATION-AND-FIX-HISTORY.md etableret som samlet lineage og fejlrettelsesregister.

### v00.00.09f30g — OOB Input + Higher HQ/Cavalry Visibility Hotfix
Fejlrettelser/hardening:
- Legacy F29V OOB status-overlay deaktiveret, så sort `190 KLAR` patch ikke længere overlapper unified OOB.
- Old infantry/cavalry/F30B/F30C OOB renderers eksplicit disabled under F30D+.
- Cavalry OOB row bruger ikke længere GUI.Button, som stjal MouseDown/MouseUp fra drag-state-maskinen.
- Single-click selection, double-click camera focus og hold+drag attachment er separeret i samme cavalry row.
- Gardehusar/Dragon får semantic I/CAV counters.
- Brigade/Division får semantic X/XX HQ counters.
- Strategic mesh suppression/restoration omfatter cavalry og higher HQ.
- Gardehusar/Dragon QA startpositioner flyttet tættere på dansk formation.
- Brigade/Division follow-distance reduceret for normal QA-læsbarhed; higher HQ clamped til battlefield bounds.

### v00.00.09f30h — Cavalry 4-Rank + Bridge + HUD + NATO + Selection Hardening
Implementeret:
- Normal cavalry Line og Charge Line = fire geledder.
- Normal mounted Column = fire abreast.
- Bridge/defile = two abreast.
- Physical formation reform med per-rider slot movement og HUD progress.
- Full charge speed først når reform er tilstrækkeligt færdig.
- Persistent bridge phases NearBank -> FarBank -> ExitBank -> Direct.
- Exit clearance skalerer efter længden af den fulde 1:1 to-abreast kolonne.
- Reissued AI/player destination under crossing bevarer bridge transaction.
- Cavalry HUD bruger company-HUD layout/farvesprog og viser faktisk Officer AI phase/target/parent.
- Authoritative F29V semantic zoom viser I/CAV Gardehusar/Dragon og X/XX Brigade/Division HQ med navne.
- Box-selection kan vælge cavalry/higher HQ når ingen infantry-centre er i boksen.
- Close labels og let mounted detail polish.

Fejlrettelser/hardening:
- F30E collider-maintenance kunne ellers overskrive F30H footprint og er opdateret til 4-rank/4-abreast/2-abreast.
- OOB non-cavalry rows var blanke fordi en tom GUI.Button blev tegnet efter labels; draw-order er vendt.
- Aktiv semantic owner var F29V, mens F30G først havde ændret legacy F29D; F30H retter den reelle runtime-owner.
- Cavalry bridge route kunne nulstilles af AI replan/new OrderMove under crossing; aktiv bridge phase bevares.
- 1:1 column reformer ikke længere ved den fjerne brokant mens halen stadig er på broen.

### v00.00.09f30i — Cavalry Authority + Single HUD + Order Visuals + Animation
- Cavalry AI default OFF/MANUEL.
- AI toggle Update/OnGUI race rettet.
- F30C/F2 parallelle bottom HUD-lag visuelt pensioneret; F30H/F30I cavalry HUD er ene-ejer.
- Higher HQ selection og cavalry selection gøres gensidigt eksklusive.
- OOB bredde 535 px.
- 120/140 figures snapper korrekt til fire-rank initial formation efter 1:1 expansion.
- Gul formation-sized selection footprint.
- Persistent order path og destination ghost footprint.
- Dragon SID AF/STIG OP procedural transition.
- Mounted move/charge procedural gait.

### v00.00.09f30j — Dismounted Dragon Fire + Horse Holders + Formation Anchor Fix
Implementeret:
- Dragon horse-holder/combat split (~25/75 prototypeværdi).
- Combat group ca. 18m foran hestene i to geledder.
- Dismounted 35/70/100m, +/-35deg fire cones.
- HOLD-only automatic carbine target/fire.
- TEST reload/ammo og directional smoke.
- STIG OP foot recall mod hestene.

Fejlrettelser/hardening:
- Column/bridge selection box og destination ghost bruger nu front-anchor i stedet for matematisk center.
- BoxCollider og F30E collider maintenance følger samme anchor.
- F30I remount animation fik faktisk positional recall i stedet for kun scale-out.

### v00.00.09f30k — Auto March Column + RMB Facing + Split Dragon Selection + Higher HQ AI HUD
- Manual cavalry MOVE får shared long-march formation policy: >=140m Column, <=90m Line, enemy inside Long -> Line.
- Manual formation override beskytter bevidst spillerformation mod auto-policy.
- RMB hold+drag sætter destination + explicit final facing ligesom infantry.
- Destination ghost og arrival-facing bruger explicit facing.
- Dismounted Dragon selection split i combat box + horse-holder box + link.
- STIG OP/remount forlænget fra 2.25s til 4.5s.
- Brigade/Division HUD får AI ON/OFF og DEF/BAL/OFF.
- Higher AI state bruges til real delegated-authority gating for attached cavalry.
- Higher doctrine føres ind i Regimental execution pipeline.
- OOB viser Brigade/Division AI ON/OFF.

### v00.00.09f30l — Cavalry Visual Fidelity + Gait Polish
- Visual-only pass; F30K gameplay authority retained.
- Existing 120/140 1:1 cavalry reused.
- Horse body/chest/neck/head/muzzle/leg/hoof/mane/tail silhouette reshaped.
- Six deterministic horse coat tones + sparse blaze/sock variation.
- Gardehusar/Dragon close-detail equipment reinforced.
- Close-detail LOD above ~210m camera altitude.
- Mounted gait articulates horse legs/hooves, head, tail and rider instead of only whole-root rocking.
- CHARGE receives faster cadence, larger leg swing and stronger forward rider lean.
- HOLD restores neutral articulated pose.

### v00.00.09f30x — Early Infantry Deploy + Company Spacing + CAV Bridge Approach + Startup Enemy Cones
- Infantry march formation now deploys to Line before entering enemy fire range rather than at the edge of MaximumRange.
- Deployment threshold uses nearest enemy MaximumRange + 35 m safety/reform buffer.
- F27 parent mission reassert no longer forces Line every 1.1 s; MarchColumn owns normal movement formation.
- Company slot spacing increased from 60 m to 72 m.
- Formation-slot minimum centre separation increased from 55 m to 68 m.
- Enemy TEST cone overlay creates missing range-fan renderers immediately when a Prussian Regiment exists, removing startup dependency on later selection/visual initialization.
- Cavalry bridge routing now separates planned route from actual narrow bridge approach.
- Opposite-bank orders keep cavalry in normal Line/4-abreast Column until approximately 36 m from the near bridge approach.
- Two-abreast bridge geometry applies only during near-bank/crossing/exit transaction, then restores the pre-bridge formation.

### v00.00.09f30w — Enemy Cone QA + Same-Bank River Routing + OOB AI Consistency
- Legacy enemy-cone overlay no longer requires selected Danish infantry within 180 m.
- All living non-routed Prussian infantry cones are forced visible in TEST/QA as the final late visual pass.
- Enemy active fire-policy band is emphasized; inactive ranges remain faint references.
- Same-bank river routes no longer trigger bridge crossing merely because the straight chord intersects a curved river segment.
- True bridge routing now requires actual bank change; same-bank water chords use dry bank-follow steering.
- Higher-order active state ignores Brigade/Division background follow and pure cavalry reform after tactical execution is complete.
- OOB AI column standardized to ON/OFF for all command and unit levels.

### v00.00.09f30v — Single Final-Slot Arrival Authority
- Root cause of early-red/early-stop QA: three different company-arrival tolerances were layered across F27/F29E/F29L.
- F27 core arrival reduced from 4.5 m to 0.50 m.
- F29E precise-arrival threshold aligned to 0.50 m; legacy compatibility window reduced to 0.75 m.
- F29L DefendHere latch aligned to 0.50 m with 1.25 m release hysteresis.
- Parent order active-state now verifies physical distance to mission.Goal and Regiment movement destination in addition to mission.Arrived.
- Parent order cannot return red while a company is physically outside the final-slot tolerance or still moving toward its assigned goal.

### v00.00.09f30u — Defend Command Authority + HQ Goal Conflict Fix
- Reviewed two F30S QA recordings showing repeated DefensiveStability goal writes followed by HqDepthGuard corrections.
- Root cause: two helper systems could own Major/Regiment HQ geometry simultaneously during DefendHere.
- HqDepthGuard now yields for battalions whose current order is DefendHere.
- HqDepthGuard also yields for Regiment HQ while the regimental current mission is DefendHere.
- DefensiveStability clears any competing Major HqGoal when the HQ is already settled at its committed defensive position.
- This removes defensive HQ tug-of-war, repeated stabilize/correct log cycles, and false continued execution state after the formation settles.

### v00.00.09f30t — Crop Tuft Visuals + Infantry Active Range Authority
- Replaced F29Y long solid crop box segments with dense upright crossed crop tufts.
- Crop gameplay/concealment remains owned by F29R; only presentation geometry changed.
- F30T creates a new CropFields09F30T root and explicitly disables the old F29Y visual root to avoid stale hot-reload geometry.
- Crop tufts sample terrain individually, use deterministic micro-jitter/height variation and disable the previous oversized beam shadows.
- PrototypeFireVisuals09F8 is now the final LateUpdate authority for infantry fire-cone active/inactive emphasis.
- Active CLOSE/MED/LONG is thick/strong; inactive physical ranges are faint reference geometry; HOLD leaves all ranges faint.
- Prussian TEST/QA cones remain visible without selection and use the same emphasis rules.
- Enemy TEST cone visibility remains visual-only and does not bypass LOS, fire-policy range or fire-cone checks.

### v00.00.09f30s — Execution-State Orders + Defend CAV Reserve + Cone QA + Selection Persistence
- Officer-order blue state now reflects physical execution only, not persistent standing intent.
- Company mission arrival, Major HQ goal, Regiment HQ relocation, higher-HQ follow and CAV move/reform/charge state all contribute to execution status.
- Defend/Hold buttons return red after all movement is settled even if the intent remains current.
- Defensive CAV is anchored behind its supported battalion rather than around the Division objective.
- Current defend CAV QA geometry is roughly 150 m rear + 45 m outward lateral.
- Dragon range-fan construction uses explicit mirrored side rays to fix one-sided cone distortion.
- Dragon and infantry both emphasize the active fire band while keeping non-selected ranges faint.
- Prussian infantry range cones are forced visible in TEST/QA so enemy facing/range/policy can be inspected; this is explicitly temporary before LOS/FOG gating.
- Division/Brigade/Regiment/Major selection is preserved while shared objective/facing input is active and after order commit.
- Anti-cavalry infantry fire now requires LOS + selected fire-policy range + fire-cone alignment.
- Mounted-threat/Square reaction also requires LOS; TEST-visible enemy cones do not grant target knowledge.
- BattleManager hover now resolves cavalry as well as infantry and renders cavalry unit info in the same hover layer.
- F30S compile hotfix: cavalry hover GUI block moved from Update() to OnGUI(); hoveredCavalry/mainCamera now remain inside their declared scope.
- Dismounted Dragon split hardened: mobile combat group moves independently while horse holders and horses remain anchored at the dismount point.
- STIG OP away from the horse park now creates an automatic return-to-horses task and auto-remounts after combat-group reunion.
- AttackHere arrival no longer hands company OfficerAI a fresh autonomous nearest-enemy chase; the parent move is finite and terminates at its assigned slot.
- Post-arrival facing is no longer rewritten each frame, removing FormationMotion small-angle oscillation.
- Precise-arrival hardening clears stale OfficerAI attack intent before controller re-enable.
- DefensiveStability now clears settled Major HqGoal instead of re-arming the same goal every frame, allowing execution-state to finish and removing repeated stabilization log spam.
### v00.00.09f30r — Dragon Fire Control + F30Q Command State Baseline
- Dismounted Dragon receives explicit HOLD/CLOSE/MED/LONG fire policy in cavalry HUD.
- Trigger ranges are 0/35/70/100 m with MED as default.
- HOLD prevents automatic target acquisition/fire.
- Existing ±35° arc, 7s TEST reload, ammunition, smoke and volley resolution remain authoritative.
- Range cones remain visible for reference while selected; active policy band is emphasized.
- Mounted Dragon does not expose active carbine fire controls; these appear only after SID AF.
- F30Q command-state, balanced attack-front, facing, HQ-follow and command-zone behaviour remains the inherited baseline.
### v00.00.09f30q — Active Order Blue + Balanced Attack Front + Cavalry Scout Design
- Shared officer order grid now has a dedicated blue state for pending and actively executing missions.
- Major, Regiment, Brigade and Division query real mission state instead of drawing all six buttons red.
- Attack mission activity includes movement, local combat/under-fire contact and a live enemy still contesting the objective area; Defend/Hold remain standing-active until replaced.
- BAL regimental AttackHere no longer withholds an entire battalion at 285 m reserve depth.
- BAL commits both battalions to the attack front; Major-level company reserve logic remains available.
- DEF retains whole-battalion reserve authority and OFF retains flank-capable disposition.
- Future `SPEJD HER` / RECON cavalry task is specified for true FOG/LOS but intentionally not exposed in runtime before enemy visibility is no longer omniscient.
### v00.00.09f30p — Committed Facing + Higher HQ Follow + Command Zones
- Explicit drag-facing is now passed into formation planning before Regiment/Battalion/company slots are generated.
- Removed post-hoc facing correction that could make order arrow and actual defensive line disagree.
- Regimental mission stores committed facing and exposes it to higher-HQ follow.
- Higher HQ follow uses committed facing and relative lateral offsets instead of fixed world-axis offsets.
- Division follow distance reduced and move speed increased to reduce excessive rear lag.
- Brigade/Division selected HQs now show inner/outer command-reach circles matching the existing Major/Regiment visual language.
- Current QA reach bands: Major 320/450 m, Regiment 800/1100 m, Brigade 1350/1850 m, Division 2100/2850 m.
- Higher command circles remain QA/visual bands until command-delay/report-quality simulation is wired to them.
### v00.00.09f30o — Higher AI Arming + Shared Target Circle + True F29G HUD Parity
- Higher-HQ AI ON changed from implicit autonomous action to ARMED/WAITING authority.
- Regimental AI no longer generates a default Attack/Defend mission when enabled by Division/Brigade cascade.
- Battalion AI no longer creates default DefendHere while waiting for higher intent, and stale company mission records are frozen until a fresh mission arrives.
- Company Officer AI is held while its Battalion awaits a higher mission.
- Cavalry cascade no longer enters SEEK; it waits on HQ order and stale inherited missions are discarded when higher AI is re-armed.
- Division/Brigade order placement now uses PrototypeOfficerFacingOrder09F29G with the same live objective circle and drag-facing flow as Regiment/Major.
- Higher-HQ HUD moved into PrototypeUnifiedCommandHud09F29G, eliminating the separate approximation and giving true runtime layout parity.
- Added black top edge to the authoritative shared HUD and removed inherited header border slicing that produced the green line.

### v00.00.09f30n — Regiment HUD Parity + Cavalry Screen/Opportunity AI + Anti-Cavalry Infantry Reaction
- Brigade/Division bottom HUD now copies Regimental HQ panel/theme/header/AI-doctrine/info/button geometry and palette 1:1.
- Higher active-order state uses caption marking rather than a separate blue/green/red HUD palette.
- Cavalry autonomous attack flow changed from geometry-only flank charge to SCREEN → OPPORTUNITY → CHARGE.
- Charge opportunity requires friendly local fire-contact on target or degraded target morale/cohesion.
- Hostile infantry companies are cavalry route-avoidance bubbles; detour waypoints prevent pathing through enemy formations.
- Detour waypoints are intermediate and resume toward the original screen/flank objective.
- Higher-HQ cavalry attack staging also respects hostile infantry avoidance.
- Prussian infantry can acquire cavalry inside selected fire-policy range and fire using normal reload/range/accuracy/smoke cadence.
- Cavalry receives infantry-volley casualties and morale/cohesion shock; effective fire can trigger FALTER, and 1:1 cavalry figures now reflect CurrentStrength losses.
- Mounted-threat/Square scoring made team-neutral and auto-Square threat timers refresh while cavalry remains nearby.
- Destroyed cavalry is excluded from autonomous cavalry AI execution.

### v00.00.09f30m — Higher Command Delegation + Temporary Cavalry Attachment + Mission Visual Parity
- Division AI cascades to Brigade/Regiment/Majors/company AI/cavalry AI.
- Brigade AI cascades downward without changing Division.
- Higher selection exposes subordinate routes, destination footprints and objective circle/cross.
- Cavalry routes/destination ghosts remain visible under selected higher HQ.
- Higher HUD aligned to Regiment three-zone command layout and subordinate AI status.
- Higher order buttons remain blue only while pending or actually executing.
- Arrived company missions no longer count as active executors.
- FORSVAR HER gives attached cavalry explicit reserve/support goals.
- ANGRIB HER temporarily task-attaches cavalry to Major A/B using CurrentCommandParent while preserving OrganicParent.
- Two-cavalry/two-battalion assignment chooses the lower total travel-cost pairing to reduce crossing.
- Attack-task cavalry returns to its prior parent as RESERVE after infantry execution completes and any committed charge finishes.
- New non-attack higher mission releases temporary attack attachment.
- Direct cavalry RMB clears inherited higher mission and restores manual authority, including when cavalry Officer AI is already OFF.
- Temporary attack release is tracked per cavalry unit, so only actually task-attached cavalry returns to reserve and unrelated cavalry charges cannot block release.

## 7. Samlet fejlrettelsesregister

De vigtigste kendte fejl, som har fået en konkret implementeret rettelse/hardening i lineage frem til F30M:

1. Unity compile CS0136 local-variable collision.
2. Deprecated/obsolete object lookup cleanup.
3. Manglende Unity IMGUI/Particle/Physics/Audio moduler.
4. Konkurrerende route/movement authority writers.
5. Manual player attack orders overskrevet af AI.
6. Ugyldige formation slots/river slots.
7. Major mission destination overskrevet lokalt.
8. AI OFF→ON stale authority.
9. F29E Oberstløjtnant HUD ikke synlig.
10. CS0104 Object ambiguity.
11. Charge target-click lækkede til normal move order.
12. Defensive companies oscillation/micro-correction ved destination.
13. Square state aktiv men visible F7 soldiers stadig Line/Column.
14. Charge skød under approach/melee eller skiftede formation uhensigtsmæssigt.
15. FencePost hard-detour skabte gentagen replanning.
16. Local attack march fortsatte forbi fjende uden facing/contact capture.
17. River presentation med visuelle gaps.
18. HQ follow-distance/lag under mission.
19. Square full-company/360° fire og forkert smoke direction.
20. OOB/HUD click mistede selection.
21. Square formation footprint blev hængende.
22. Square cone/range lines kunne klippe i terræn.
23. LINE↔SQUARE transition kunne udløse falsk smoke.
24. DefendHere objective/facing kunne omskrives forkert.
25. Companies kunne blokere friendly fire lanes uden taktisk side-step.
26. Column→Line begyndte for sent før enemy fire range.
27. Runtime NullReference i charge/fire target path.
28. Command-tree blev kun vist én vej.
29. Cavalry havde separat F10 testkontrol i stedet for normal unit-control.
30. AI cavalry kunne overskrive direkte player order.
31. OOB højre info overlappede officer/strength/status tekst.
32. OOB manglede scrollbar til voksende hierarchy.
33. Cavalry attachment krævede knap i stedet for drag/drop OOB.
34. Cavalry proxy viste kun ca. 12/14 eller 24/28 ryttere i stedet for 1:1.
35. HQ/cavalry horses manglede læselige ben/hove.
36. Dragon dismount kunne efterlade legacy ghost-rider geometry.
37. 1:1 cavalry collider kunne blive overskrevet af gammel proxy ResizeCollider efter formation/mode change.
38. Legacy F29V `190 KLAR` status-overlay blev fortsat tegnet oven på unified OOB.
39. Cavalry OOB `GUI.Button` konsumerede mouse events og blokerede selection/drag-drop state machine.
40. Gardehusar/Dragon manglede semantic-zoom counters og var derfor svære at lokalisere i operational/strategic view.
41. Brigade/Division HQ manglede semantic counters og lå for langt bag regiments-HQ til normal QA-læsbarhed.
42. F30G ændrede legacy F29D semantic layer, mens runtime F29V disabled det; cavalry/higher-HQ counters udeblev.
43. OOB non-cavalry rows blev visuelt dækket af en tom GUI.Button tegnet efter labels.
44. 1:1 cavalry bridge transaction kunne nulstilles af reissued AI/player destination.
45. Bridge crossing reformerede for tidligt ved far bank før den fulde 1:1 column var fri.
46. F30E collider maintenance kunne overskrive F30H formation footprint.
47. Cavalry Line/Column transition var for hurtig og læstes som magisk snap.
48. Cavalry HUD brugte et separat layout og obsolete tekst om at AI ikke var implementeret.
49. Cavalry Officer AI startede ON og flyttede enhederne autonomt direkte efter spawn.
50. AI-toggle blev behandlet som manual HUD command før knappen og togglet tilbage til ON.
51. F30C control strip og F2 legacy bottom bar kunne stadig tegne ekstra HUD-lag.
52. F30E 1:1-added figures startede i origin og skulle reformere visuelt fra gammel proxy.
53. Cavalry selection marker viste en lille cylinder i stedet for formationens footprint.
54. Cavalry manglede persistent order line og destination ghost footprint.
55. Dragon SID AF/STIG OP skiftede visuals instant uden overgang.
56. Mounted cavalry gled uden nogen riding/gait animation.
57. OOB 452 px var for smal til cavalry-navne og voksende support hierarchy.
58. Mounted Column/bridge selection box var center-anchored selv om formation slots var front-anchored.
59. Destination ghost brugte samme forkerte center-anchor og viste kun ca. halvdelen af column footprint korrekt.
60. Cavalry BoxCollider center fulgte ikke det visuelle formation-anchor.
61. Dragon SID AF manglede horse-holder/combat-group separation og reel firing line foran hestene.
62. Dismounted Dragon havde ingen fire cone, ammunition, reload, target fire eller smoke.
63. F30I STIG OP skalerede foot figures ud, men kaldte dem ikke fysisk tilbage mod hestene.
64. Manual cavalry MOVE gik ikke automatisk i Column, fordi formation policy kun fandtes i Officer AI.
65. Cavalry manglede infantry-lignende RMB hold+drag final-facing control.
66. Dismounted Dragon selection brugte én stor box med meget tom plads mellem horses og combat line.
67. Remount-transition var for hurtig til at læse tydeligt.
68. Brigade/Division HUD manglede AI ON/OFF og doctrine controls.
69. Attached cavalry kunne ignorere AI-OFF på Brigade/Division/Regiment parent.
70. Division AI ON ændrede kun Division state; Brigade/Regiment/Majors/CAV kunne forblive OFF.
71. Brigade/Division selection skjulte eksisterende subordinate routes og destination footprints.
72. Higher objective circle/cross var bundet til regimental.Selected.
73. FORSVAR HER fra higher HQ gav ikke cavalry konkrete reserve/support goals.
74. Higher cavalry mission manglede persistent delegated state og player-override release.
75. Brigade/Division HUD brugte et andet layout og viste ikke hele subordinate AI-kæden.
76. Higher order blue state kunne blive stående på en committed order uden aktive executors.
77. ANGRIB HER brugte cavalry som generisk higher-HQ support i stedet for midlertidig Battalion/Major task attachment.
78. Cavalry havde ingen automatisk release/return-to-reserve efter afsluttet higher attack.

## 8. Status ved F30M

F30M er aktiv TEST gameplay/build-baseline. F30L er den underliggende cavalry visual-fidelity baseline. F30M udvider higher-command delegation, subordinate mission visibility og dynamisk cavalry task-attachment/release.

Aktuel implementeret tactical scope:
- 1:1 infantry og cavalry tactical visuals; F30L cavalry close-detail/gait LOD uden ændring af simuleret styrke.
- 8-company Danish regiment med 2 Majors + Oberstløjtnant.
- Brigade- og Division-HQ.
- Dynamic support attachment til Division/Brigade/Regiment/Major.
- OOB scroll + drag/drop.
- Manual override på alle implementerede control-lag.
- Infantry Line/Column/Square, fire policy, charge/melee, under-fire, withdrawal infrastructure.
- Cavalry 4-rank Line/Charge, 4-abreast march Column, 2-abreast bridge/defile, fysisk reform, mounted move/charge, Dragon dismount/remount, horse-holder/combat split, dismounted carbine fire og flank/rear Officer AI.
- Semantic zoom/NATO inklusive cavalry I/CAV og Brigade/Division X/XX HQ counters, tactical map, crop concealment og terrain/navigation hardening.

Ikke færdigt endnu:
- Artillery/kanonbatteri.
- Mounted cavalry firearms combat.
- Defensive volley / charge momentum / cavalry casualties.
- Persistent cavalry melee/horse casualties.
- Echelon Left/Right.
- Full autonomous Brigade/Division Officer AI.
- Full campaign/OOB expansion til flere regimenter/brigader.
