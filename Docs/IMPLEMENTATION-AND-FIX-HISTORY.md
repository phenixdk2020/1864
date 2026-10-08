# PROJECT 1864 — Implementation & Fix History

> **Dokumentet vedligeholdes nu her i Game1864-projektet** (`1864-Campaign/Docs`). Slaget og kampagnen er samlet i ét projekt siden 1. oktober 2026.
> - Afsnit 1–8 er Unity-prototypens historik. Den er uændret fra `Strategy/docs/IMPLEMENTATION-AND-FIX-HISTORY.md`.
> - Afsnit 9 er Unreal-porten af slaget (Strategy1864).
> - Afsnit 10 er samlingen i Game1864 og koblingen til kampagnen.
>
> Nyt arbejde på slaget eller kampagnen noteres i afsnit 10 og frem.

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

## 9. Unreal-porten af slaget (Strategy1864, 27.–30. september 2026)

Slaget blev porteret fra Unity til Unreal Engine 5.8 som modulet `Strategy1864`.
- **Projekt:** `Strategy/Unreal`, grenen `unreal-port`.
- **Omfang:** 634 commits og omkring 28.500 linjer C++.
- **Version:** HUD-markør v00.02.xx-dev, sidst v00.02.79.
- **Kilde:** opsummeringen bygger på commit-beskederne.

### 27. september — fundamentet (196 commits)
- Projektets grundskelet. RTS-kamera og valg af enheder (U01). Kommandohierarki (U02).
- Ordrernes livsforløb og fysisk bevægelse (U03).
- Formationsgeometri og kompagniernes pladser (U04).
- Officers-AI med HQ-følge og kommandozoner (U07).
- Sigtelinje, rækkevidde og ildkegler. Hukommelse om sidst sete fjende. Karré og kavaleriformationer.
- Dragoner sidder af og op igen, med hesteparken.
- Moral, samhold, chok og genopretning under ild. Træthed og erfaring.
- Ilddisciplin, ammunition, opbrugt ammunition og genforsyning.
- Ordrekø med forsinkelse. Officersprofiler og kommandoforsinkelse. Doktrin, aggressivitet og selvstændighed.
- Selvstændig fjende-AI med flugt og samling. AI-sværhedsgrad og telemetri.
- Broer: hvem der står på broen, og bevægelse der tager hensyn til broen. Forhindringer, omveje og hældning.
- Sortkrudtsrøg. Hændelser for salver og tab.
- Kamporden: valg, kamerafokus og tilknytninger. Kontrol af kommandotræet. Faste seeds.
- En test-tjekliste for tidligere fejl.

### 28. september — artilleri, logistik og visuel kerne (346 commits)
**Artilleri:**
- artilleribatteri som taktisk enhed med NATO-symbol;
- ammunition efter type;
- forstille, opstille og flytte med håndkraft; kusk krævet for at køre;
- sideretning; skader på besætning, heste og kanoner;
- erobring og genbrug af kanoner;
- skudopgaver i tilstandene manuel, auto og hold.

**Fælles visuel kerne:**
- skeletter og animationssæt med kontrol af, at de passer sammen;
- fæster til gevær, bajonet, sabel og værktøj;
- hestens gangarter (skridt, trav, galop) ud fra bevægelsen;
- rytter og hest i takt;
- besætningens kanonøvelser som animation;
- uniforms- og udstyrsforvalg samt farvepaletter for Danmark og Preussen.

**Slagets testverden:**
- kavaleri, artilleri og fjender kan slås til og fra hver for sig;
- en fælles kontrakt for testvisningen.

### 29. september — testverden og specialisttropper (68 commits)
**Testverden:**
- et testsystem, der selv starter testscenariet og kameraet;
- synlige pladsholdere for enhederne;
- en flad testslagmark på 600 m (`Strategy1864_QA`);
- etiketter på enhederne.

**Batch 1–10:**
- løsrevne afdelinger og underofficerer (kadre, lokal reaktion, samling og omformering);
- skydeøvelse med knælende stilling;
- befæstninger og markskanser (forsvarsstillinger efter type, besættelse og dækning for artilleri);
- arbejdshold, der graver og bryder igennem;
- storm på befæstninger med sprængt gennembrud;
- morterer og forsvarets logistik;
- gemning af specialistenhedernes tilstand og test af den.

### 30. september — rigtige soldater og skydeøvelse (23 commits)
- Livgarden 1864: rig, automatisk import og det fælles skelet `SK_Human_1864`.
- En komponent, der viser kompagniets soldater i 3D. Livgarden vises på det første danske testkompagni.
- Skydeøvelse som forskning, med ild geled for geled i takt og regler for, hvilke pladser i formationen der må skyde.
- HUD-markør v00.02.79.

## 10. Samlet i Game1864 (1. oktober 2026 og frem)

### Slaget flyttet ind i Game1864
**Ændring i Strategy1864 (commit `66c231c`):**
- `StrategyQARuntimeSubsystem` starter kun på kort, der hedder `Strategy1864…`.
- Ellers ville kampagnekortet få testslaget og slagets kamera.

**Spejling ind i 1864-Campaign** (commits `6e5ffe6` og `1471346`, den sidste med arbejdsversionen oven på `66c231c`):
- modulet ligger i `Source/Strategy1864`;
- indholdet i `Content/Units`;
- slagkortet i `Content/Maps/Strategy1864_QA.umap`;
- tastaturbindingerne i `Config/DefaultInput.ini`.

**Engangsændringer i projektet:**
- Modulet er med i `Game1864.uproject` og begge `Target.cs`-filer.
- Kort med præfikset `Strategy1864_` kører `StrategyGameMode` via `GameModeMapPrefixes`. Kampagnen beholder `Campaign1851GameMode`.

**Beslutning (commit `8e96a8c`):** slaget udvikles fremover kun i Game1864. Strategy1864 bruges ikke længere, og `Tools/Battle/Sync-Battle.ps1` køres ikke mere.

**Verificeret:**
- Hele projektet bygger.
- Kampagnen starter uden testslag.
- `Strategy1864_QA` starter med `StrategyGameMode`, 20 enheder og et kommandotræ, der består kontrollen.

**Kendt fejl i slagets test** (`StrategyOOBTestScenario.cpp`):
- Tjeklisten tæller morterbatteriet (`AStrategyMortarBatteryUnit` arver fra artilleribatteriet) som artilleri og melder derfor "Expected 1 Danish artillery battery, found 2".
- Den melder også "Artillery QA battery did not start deployed", fordi morteren ikke er opstillet fra start.
- Fejlen fandtes allerede før flytningen.

### Kampagnen klar til slaget (kampagne v00.00.53–v00.00.55)
**Til 3D-slaget sendes** `Units.json` og `BattleRequest_N.json` (Docs/BattleLink1851.md):
- `battleRules`: forskning omsat til slagets egne tal, fx ladetid, karré, dækning i afgrøder, karabinens rækkevidde, kommandozoner og pionerbro;
- `aiDefaults`: doktrin omsat til DEF/BAL/OFF og ildpolitik;
- `subunits`: kompagnier à 190 med kaptajner, eskadroner à 120/140 og batterier;
- `formations`: kommandotræet XX/X/III med chef, stedfortræder og stabschef.

**Forskningstræet** har 7 grene, og infanteri-, kavaleri- og kommandoemnerne er bygget på slagets systemer:
- karré;
- kædelinjer;
- dragonernes ildkamp;
- rytterchok;
- rytterspejdning;
- stabsskole og generalstab;
- pontonnerer.

**Slagmarksgeneratoren** (`Battlefield_*.json`) giver:
- højde, vand og floder med våde enge;
- broer og vadesteder;
- markskel (knicks, diger og grøfter);
- veje, jernbaner, byer, gårde og skanser.

**Slagmarken i 3D:** knappen GÅ IND PÅ SLAGMARKEN viser den genererede slagmark som 3D-model ved siden af kampagnekortet, endnu uden enheder.

### Testbanen med duellen Livgarden mod svensk infanteri (1. oktober 2026)
- **Ny testbane:** `Content/Maps/Strategy1864_Duel.umap`, en kopi af QA-kortet.
  - Alle kort med "Duel" i navnet starter duellen. Det gør kommandolinjen `-Strategy1864Duel` også.
  - Duellen er to kompagnier i 3D: Livgarden (`SK_DK_Livgarden_1864`) og svensk infanteri (`SK_SE_Infantry_1864`). Den fulde kamporden er slået fra.
  - Rettelsen ligger i `AStrategyOOBTestScenario::BeginPlay` og bruger `bLivgardenVsSwedishTest`.
  - Grunden til rettelsen: QA-kortets gemte indstilling slog ikke igennem, og den fulde kamporden startede i stedet.
- **Loggen** viser nu kort og valgt test: `PROJECT1864-QA: map …, test …`.
- **Rettet:** spejlingen bevarede kildefilernes gamle tidsstempler. Derfor genbrugte byggeværktøjet gamle objektfiler, og arbejdsversionens byg var delvis den gamle kode (link-fejl på `GetFormationLocalBounds`). Alle filer i `Source/Strategy1864` blev markeret som ændrede og bygget forfra, og `Sync-Battle.ps1` sætter nu tidsstemplet ved kopiering.
- **Verificeret:** `Strategy1864_Duel` starter med "PROJECT1864-DUEL: Livgarden and Swedish infantry, 2 units", og begge kompagnier vises som 3D-soldater.

### Duellen: fremrykning, ild, død og røg (1. oktober 2026)
Ændret i `Tests/StrategyOOBTestScenario` og `Visual/StrategyInfantryVisualComponent`. Ny klasse: `Visual/StrategyMuzzleSmokePuff`.

**Duellen:**
- Livgarden og svensk infanteri starter 300 m fra hinanden.
- Duellen er nu den eneste bevægelsesautoritet (`TickDuel`). Den selvstændige AI er slået fra, fordi den venter på en kontakt, som duellen ikke har.
- Hvert kompagni rykker frem i kolonne, til fjenden er inden for kompagniets egen aktive skudafstand (`GetActiveRangeCm`, MEDIUM 70 m). Så holder det med front mod fjenden og skyder gennem den almindelige `CombatComponent`.

**Ildkegler:**
- Begge kompagnier viser kegle (±35°) med CLOSE, MEDIUM og LONG.
- Den aktive afstand er kraftig, de andre svage.
- Det er kun QA-grafik.

**Den enkelte soldat:**
- Hver salve fordeles på soldaterne med op til 0,6 s forskydning.
- Den, der skyder, hæver geværet (`A_Rifle_Down_To_Aim`), skyder (`A_Firing_Rifle`), får røg fra mundingen og vender tilbage til kompagniets animation.
- Loopende animationer starter forskudt med ±10 % fart, så soldaterne ikke går i takt.

**Tab:**
- Den, der rammes, falder med en af 3 dødsanimationer. Under march bruges også `A_Walking_To_Dying`.
- Han løsnes fra kompagniet og bliver liggende med sit gevær.
- Før forsvandt figurerne bare.

**Røg (`AStrategyMuzzleSmokePuff`):**
- Gennemsigtige kugler med motorens `M_SimpleTranslucent`.
- De vokser fra ca. 0,5 til 3 m, driver frem med skuddet, stiger og tynder ud over 7–11 s.
- Røgfeltets effekt på sigt er fortsat `AStrategySmokeField`.

**Geværer:** de peges hver opdatering fra højre hånd mod venstre hånd (`bAlignRifleBetweenHands`, `RifleGripFraction` 0,22). Før lå de på tværs af brystet, fordi håndknoglens akser er forskellige fra animation til animation.

**Testkamera:**
- `-Strategy1864DuelCamera=<cm>`: afstand.
- `-Strategy1864DuelFocus=0/1`: følg et af kompagnierne.
- `-Strategy1864DuelPitch=<grader>` og `-Strategy1864DuelYaw=<grader>`: vinkel.

**Verificeret visuelt:** kolonnen marcherer frem, de holder på 63 m og danner linje i 3 geledder med geværerne i anslag. Røgen kommer fra rækken, og en falden soldat bliver liggende.

**Ikke verificeret:** om hver af de 3 dødsanimationer faktisk bliver valgt.

**Ildkeglen fra formationens front (brugerens tegning):**
- Keglens sider går fra formationens forreste hjørner (`GetFireFront`) ud i ±35°.
- CLOSE, MEDIUM og LONG følger fronten: lige ud for fronten og runde i siderne.
- Den valgte afstand er tyk og i fuld farve, de andre to tynde og mørke.

**Skytter efter vinkel og afstand (i slagets kampkerne):**
- `UStrategyFireControlComponent::GetBearingFraction` tjekker hver mands plads i formationen mod 22 punkter på tværs af målformationen. Et punkt tæller, hvis det ligger foran ham, inden for den aktive afstand og inden for ±35° fra hans egen plads.
- Salven (`CombatComponent::TryFireAt`) ganges med andelen af mænd, der kan skyde. Det sker efter skydeøvelsens regler for geledder og før skyttekædens tæthed.
- `LastVolleyTarget`, `LastBearingCount` og `LastBearingTotal` gemmes.
- I visningen skyder de mænd, der kan ramme (`CanPointBearOn`), først.
- Karré er undtaget, fordi siderne har deres egen andel.

**Verificeret:**
- Front mod front: "190 of 190 men can bear".
- Med `-Strategy1864DuelOffset=6000` drejer kompagnierne sig stadig mod hinanden, fordi `CombatComponent` vender selvstændige kompagniers front mod nærmeste fjende. Derfor kan alle stadig skyde.
- Et mindre antal skytter er ikke set endnu. Det viser sig først, når fronten er låst, fx ved forsvar af en stilling.

**Senere: FireByRank pr. geled.** Geled 1 skyder og lader, så geled 2 og så geled 3, i stedet for at tilfældige soldater skyder (`FireDrillComponent` kender allerede det geled, der skyder).

### Slagets skærmbillede: kamporden, kommandopanel og minikort (1. oktober 2026)
`Player/StrategyHUD` er skrevet om efter Unity F30X (brugerens video og skærmbillede).

**Kampordenen** (øverst til venstre, [O]):
- Kolonnerne er træet med NATO-mærker (XX/X/III/II/I), MÆND, STATUS, AI og TILK. ATT og ↳ markerer midlertidigt tilknyttede enheder.
- HQ'er viser summen af mænd under sig.
- Grene foldes med −/+, og hele panelet kan foldes.
- Et klik vælger enheden uden at flytte kameraet. Et dobbeltklik stiller kameraet bag enheden, så det ser i enhedens retning.

**Kommandopanelet** (bunden, for den valgte enhed):
- HQ'er: AI ON/OFF, DEF/BAL/OFF og de seks ordrer (ANGRIB HER, FORSVAR HER, RYK FREM, TILBAGETRÆK, SAML, STOP/HOLD). Knappen er blå, mens ordren udføres. De underlagte vises med status, AI og tilknytning.
- Kompagnier: HOLD/CLOSE/MED/LONG, RYK FREM, TILBAGE, CHARGE og STOP samt LINIE/KOLONNE/KARRÉ.

**Minikortet** ("Taktisk kort / kamera", nederst til højre):
- Alle enheder vises: danske i turkis, fjender i rødt, den valgte i guld.
- Kameraets plads er markeret.
- Et klik flytter kameraet dertil.

**Klik på panelerne** bliver i panelerne (`AStrategyPlayerController::SelectionPressed`).

**Ny testbane:** `Strategy1864_OOB` starter hele kampordenen. Kommandolinjen `-Strategy1864FullOOB` gør det samme.

**Verificeret:** kampordenen med 20 enheder, valg af 2. kompagni fra træet og kommandopanelet for et kompagni.

**Ikke prøvet endnu:** ordrer fra panelet med klik og træk for front, dobbeltklik og klik på minikortet.

### Kampagnens slagmark i 3D-slaget, frem og tilbage (1. oktober 2026)
**Kampagnen → slaget:**
- UDKÆMP I 3D skriver `BattleRequest_N.json`, `Units.json` og `Battlefield_Battle_N.json` (med `_ground.png`, dvs. jorden uden tegnede veje og huse).
- Kampagnen gemmes i Autosave.
- Kortet `Strategy1864_Field` åbnes med `?Battle=N`.

**Slaget (`AStrategyOOBTestScenario::BuildCampaignBattle`):**
- `AStrategyCampaignBattlefield` (`Source/Strategy1864/Campaign`) bygger slagmarken i fuld skala (1 enhed = 1 cm):
  - jorden er en `UProceduralMeshComponent` med farver fra billedet og kollision på højdegitteret, så enhedernes terrænspørgsmål står på den;
  - veje, chausséer, markveje, spor, jernbaner og åer er bånd;
  - huse, gårde, kirker, skov og markskel bruger kampagnens scenerimodeller (`Campaign1851Scenery` er nu eksporteret med `GAME1864_API`).
- Kampagnens enheder:
  - bataljoner bliver en major-HQ med kompagnierne fra `battle.subunits`, med deres mænd og 72 m mellem kompagnierne;
  - rytteri står på fløjene og batterier bag midten;
  - alle under HQ'et Felthæren.
- Fjenden: `enemy.men`/190 kompagnier (højst 16) i to linjer, 800 m fra de danske, med AI ON. Det svenske mesh står i for preussisk og østrigsk.
- Figurerne vises 1:5 (`-Strategy1864FieldLOD=`). Simulationen har alle mænd.

**Slaget → kampagnen:**
- AFSLUT SLAGET → KAMPAGNEN skriver `BattleResult_N.json`: tab pr. kampagneenhed, fjendens tab og udfald ud fra den andel, hver side har tilbage.
- Slaget skriver `ReturnToCampaign.flag` og åbner `Campaign1851`.
- Kampagnen indlæser Autosave (også med `-CampaignNew`), springer testflagene over og læser resultatet ind.

**Rettet:** kampagnens slag blev ikke gemt. Et slag, der ventede på 3D, gik tabt ved indlæsning. Nu gemmes de som `battle|…`-linjer i krigens del af gemningen.

**Testflag:**
- `-CampaignFight3D`: første slag sendes til 3D.
- `-Strategy1864AutoFinish=<s>`: slaget afsluttes af sig selv.
- `-Strategy1864Field=<fil>` og `-Strategy1864Battle=N`: start slagkortet direkte.

**Verificeret hele vejen:**
1. Krig, og slaget ved Rendsborg sendes til 3D.
2. Slagmarken bygges med 14. bataljon og 4. batteri mod 16 østrigske kompagnier.
3. Slaget afsluttes, og der skrives et resultat.
4. Kampagnen indlæses, og nyheden lyder "Slaget ved Rendsborg: uafgjort … (fra 3D-slaget)". Resultatfilen er læst (`.read`).

**Mangler:**
- Floderne er kun grafik og spærrer endnu ikke ruterne (`AStrategyRiverBarrier` kan kun lige floder).
- Skanser og broer fra kampagnen sættes ikke ind som stillinger.
- Fjenden har ingen HQ'er.
- `battleRules` og `aiDefaults` læses ikke endnu.
- Opstillingen ignorerer terrænet: de danske kan stå midt i en by.

**Indstillinger:**
- Kameraets fart på tasterne sættes under INDSTILLINGER i slagets skærmbillede, i trin fra x1 til x20.
- Standard er x5 (`AStrategyCameraPawn::GetKeySpeedFactor`, gemt i `GameUserSettings.ini` [PROJECT1864.Settings]).

### Ildkegle, ildcyklus og grafik (1. oktober 2026, aften)
**Ildkeglen** tegnes nu af HUD'en (`AStrategyHUD::DrawFireCone`) for valgte enheder og duellens kompagnier. Udseendet følger brugerens QA-billede:
- keglen går fra formationens forreste hjørner, fra venstre til højre mand;
- siderne er hvide og stiplede, med ±35° ved enderne;
- CLOSE, MEDIUM og LONG er stiplede buer med meter på;
- den valgte afstand er kraftig orange, og båndet op til den er fyldt halvgennemsigtigt;
- der er en forklaring "FIRE POLICY (AKTIV: …)";
- under enheden står et mærke: NATO-symbol, navn og "190 mand | Linie | Ild: MEDIUM".

Duellens gamle debug-kegler er fjernet.

**Rettet:** et nedbrud i Canvas-trekanten (`CanvasItem.cpp:1721`, tekstur mangler). Nu bruges `GWhiteTexture`, og `RenderCore` er tilføjet som afhængighed.

**Ildcyklussen pr. soldat** (`UStrategyInfantryVisualComponent::UpdatePersonalActions`):
1. **March og formation:** duellen marcherer i kolonne, indtil fjendens længste rækkevidde + 35 m (135 m), og danner så linje.
2. **Ladet, klar:** i linje med fjenden inden for lang rækkevidde står soldaterne ladte og klar (`A_Rifle_Idle`).
3. **Sigt:** fjenden inden for den valgte afstand: anslag (`A_Rifle_Down_To_Aim` og derefter `A_Rifle_Aiming_Idle`).
4. **Skyd:** salven giver skud (`A_Firing_Rifle`) og røg.
5. **Lad:** knæ med ladestok (`A_Reload_sitting`, den eneste ladeanimation i sættet), så længe kompagniets ladetid løber.
6. **Rejs, klar:** rejse sig (`A_Rifle_Kneel_To_Stand`) og stå klar.
7. **Forfra.**

Kun ladte soldater kan blive valgt til næste salve.

**Rettet:** soldater i anslag blev aldrig valgt til skud, så cyklussen gik i stå.

**Grafik:**
- **Engen:** testbanerne får en bølget eng (`AStrategyCampaignBattlefield::BuildMeadow`, 1200 m) i stedet for den grå boks. Græs i pletter (frisk, frodigt, tørt, mørkt og blomster), lys fra nordvest, en markvej, skov bag begge sider, krat, enkelte træer og to knick-hegn med åbninger. `-Strategy1864FlatQA` beholder boksen.
- **Faner:** Dannebrog og den svenske korsfane (`AStrategyColourFlag`) følger kompagnierne.
- **Ray tracing:** soldater, geværer, røg og faner er taget ud af ray tracing. Med 380 animerede soldater tæt på løb ray tracing-hukommelsen over (`!Geometry->IsEvicted`), og spillet frøs næsten.

**Verificeret visuelt i `Strategy1864_Duel`:** kolonne til 133 m, linje, holdt på 65 m, salve med røg, knæ og ladning, rejse sig, sigte, faldne der ligger, faner og eng med skov.

**Mangler for at ligne billedet:**
- dansk linjeinfanteri i blåt med oppakning (kræver en model);
- en rigtig græstekstur og belysning med skygger på jorden (materialet er i dag ubelyst med farver i hjørnerne);
- en fanebærer;
- FireByRank pr. geled.

### Danske modeller, faneflag med vilkårligt flag, ray tracing fra (1. oktober 2026, sen aften)
- Nye modeller importeret med `Content/Python/import_units_1864.py` fra `SourceAssets/Units1864` (FBX plus teksturen fra den tilhørende GLB):
  - `SK_DK_Infantry_1864` (linjeinfanteri) og `SK_DK_Jager_1864` (jæger, 6. regiment) på det fælles skelet `SK_Human_1864`, med materialet `M_*`.
  - Statiske genstande i `/Game/Units/Items`: `SM_Flag_Standard`, `SM_Cannon_1864`, `SM_Mortar_1864`, `SM_Saddle`, `SM_Scabbard`, `SM_Saber` og `SM_Horse_Static`. De er normaliseret til ca. 2 m største mål og skal skaleres, når de bruges.
- I kampagneslaget vælges model efter bataljonstypen:
  - `jager_battalion` får jægermodellen;
  - `guard_battalion` får Livgarden;
  - resten får linjeinfanteriet.
- Duellen har fået flaget `-Strategy1864DuelDanish=Infantry|Jager`.
- Faneflaget kan bære et vilkårligt flag:
  - `Flag_Standard_Split.fbx` er delt i Blender i stang (slot `Pole`) og dug (slot `Cloth`, flade UV'er med stangsiden ved u=0 og toppen ved v=0).
  - `Content/Python/import_flag_1864.py` importerer hver PNG i `SourceAssets/Units1864/Flags` som `/Game/Units/Flags/T_<navn>` og laver `M_FlagCloth` (tosidet, med teksturparameteren `Flag`).
  - `AStrategyColourFlag` bruger modellen (2,8 m) med `T_Flag_<nation>` og har fået `SetFlagTexture` til andre flag. Det procedurale flag er kun reserve.
- `r.RayTracing=False`. Puljen gik over budgettet ved tæt kamera, og spillet var næsten frosset.
- Byggefejl C4459: `Gold` skyggede for en global variabel i unity-buildet og er omdøbt.

### Slagmarkens grafik som referencebilledet (1. oktober 2026, nat)
- Årsagen til det flade look: hele slagmarken brugte kampagnekortets *unlit* materiale (`M_Campaign1851Scenery`) med påmalet skygge.
- Nye belyste materialer laves med `Content/Python/import_battle_graphics.py` i `/Game/Battle/Materials`:
  - **`M_BattleGround`:** billedets vertexfarver (sRGB → lineær med pow 2,2) gange en flisebelagt græsdetalje i to skalaer (3 m og 11,3 m). Jordteksturen bruges, hvor farven er brun (R > G). Hertil makrovariation over 400 m (frodige, tørre og mørke pletter) og normal maps.
  - **`M_Foliage`:** maskerede tosidede løvkort med vind (SimpleGrassWind, vægt i vertexfarve R), AO (G), farvevariation pr. instans og udtynding med afstanden til græsset (`FadeStart`/`FadeLength`). Instanserne er `MI_Spruce`, `MI_Pine`, `MI_Leaves` og `MI_Grass`.
  - Desuden `M_Bark`, `M_FenceWood`, `M_BattleScenery` (belyste bygninger og diger) og `M_BattleWater`.
  - Alle har brugsflaget for instansierede meshes. Uden det tegner spillet standardmaterialet (brune firkanter).
- Teksturer laves med `Tools/Battle/make_battle_textures.py` (numpy/PIL), modeller med `Tools/Battle/make_battle_meshes.py` (Blender):
  - Gran, fyr, løvtræ og eg som kort med normaler ud fra kronen, så de virker fyldige.
  - Busk, tre græstotter og stakit (stolper og rafter).
- `AStrategyCampaignBattlefield`:
  - Ny jord og nye træer i skovene.
  - Knikkene er vokset til med buske hver 3,2 m og et træ hver ca. 40 m; buskene tegnes kun inden for 1,8 km, mens volden bliver.
  - Stakit langs veje og spor, med huller.
  - Hver å i sin egen bredde (Ejderen 50 m).
  - Græs i 25 m-fliser inden for 115 m af kameraet, kun når kameraet er under 130 m. Det holdes væk fra veje, vand, huse og knik via en 2 m-maske. Komponenterne genbruges, og der bygges højst 6 fliser pr. tick.
  - Engen har fået en dæmpet sensommerpalet uden påmalet skygge.
  - `-Strategy1864Grass=0` slår græsset fra.
- `AStrategyBattleAtmosphere` (ny) bruges på alle slagkort:
  - Lav sol (24°) fra sydvest, 5400 K, bløde skygger.
  - Himmellyset optager himlen i realtid.
  - Tynd varm dis.
  - Farvekorrektion: mætning 0,92, let varm gain, bloom, vignet og eksponering −0,4.
  - Testflag: `-Strategy1864Sun=`, `-Strategy1864SunYaw=`, `-Strategy1864Haze=`, `-Strategy1864Exposure=`.
- Fjendens ildkegle tegnes kun med streger. Dens fyld lå hen over vores egen linje.

### Blandingen: bagte soldater på afstand, fuld animation tæt på (1. oktober 2026, nat)
- Kompagnier længere end 70 m fra kameraet tegnes "bagt", og de skifter tilbage under 55 m (`CrowdFarCm`/`CrowdNearCm`):
  - Ét instansieret mesh pr. kompagni og et for de faldne, i stedet for 190 skeletmodeller med hvert sit gevær.
  - De skjulte skeletmodeller tikker ikke.
  - Testflag: `-Strategy1864Crowd=0` og `-Strategy1864CrowdFar=<cm>`.
- `UStrategyCrowdModel` (`Visual/StrategyCrowdModel.h/.cpp`) bager første gang en verden bruger en model (26–60 ms pr. model). AnimToTexture-pluginet findes ikke i motoren, så bagningen er vores egen:
  - Soldatens LOD3 (ca. 2.500 vertices; LOD'erne laves af `make_crowd_material.py`) og geværet bliver én statisk model.
  - Knogleindeks ligger i UV1-2 og vægte i UV3-4. Geværet sidder på en virtuel knogle.
  - Alle komponentens klip bages med 15 billeder i sekundet af en skjult skeletmodel til en float-tekstur med skinningsmatricer (tre texels pr. knogle, én række pr. billede).
  - Geværets virtuelle knogle følger samme regel som `AlignWeapons`: fra højre hånd mod venstre.
- `M_CrowdVAT` laver skinningen i vertex-shaderen og blander to billeder. Instansens custom data er første række, antal billeder, starttid og billedrate (negativ betyder: spil én gang og bliv liggende).
- `UStrategyInfantryVisualComponent` husker hver mands klip (`FPlayedClip`: klip, start, rate, loop) i `PlayOnSoldier`, `RefreshAnimation` og `KillSoldiers`:
  - Bagt tilstand fortsætter, hvor den fulde animation var, og omvendt (`RestoreClip`).
  - Ladecyklus, sigte, skud og død kører uændret pr. mand. De faldne falder og bliver liggende også i bagt tilstand.
- Målt på OOB-kortet med to kompagnier i 3D: spil-tid pr. billede 11,3 → 7,8 ms, polygoner 3,4 → 1,3 mio., draw calls 1548 → 958.
- Fejl rettet undervejs: Transform-noden i materialet tager sit input med tomt navn (`''`). Med "Input" fik den intet, og spillet brugte standardmaterialet.

### Kampagneslagene: opstilling på åbent land, fjendens HQ'er, kampagnens regler, ildmetoder med forskning og træning (2. oktober 2026)
- **Opstilling** (`BuildCampaignBattle`):
  - Fjenden kommer fra sit korps' retning (`enemy.bearingDeg` i slagforespørgslen: grader nord for øst, fra korpset til slaget). Står korpset på selve slagets grund, kommer han fra syd.
  - Danskerne forsvarer foran byen: mellem den og fjenden, på første afstand fra midten (mindst 350 m), hvor hele fronten står på mindst 80 % åbent land. Fjenden står 700 m længere ude eller mere, også på åbent land.
  - Alle enheder vender mod hinanden, og kameraet starter bag den danske linje og ser mod fjenden.
  - `AStrategyCampaignBattlefield::IsOpenGround` afviser by, skov, vand, hav, veje, knik og huse. `IsWater` giver også åens bredde.
- **Fjenden** har fået en brigadestab og en bataljonsstab for hver fire kompagnier:
  - Med preussisk tændnålsgevær (Dreyse, eller Preussen uden angivelse) lades der tre gange så hurtigt. Han bruger bagladeren og fri ild; østrigerne bruger salve.
  - Hans træfsandsynlighed ganges med hans kvalitet og de danske tabsfaktorer (`battleRules.lossFactor` × `doctrine.lossFactor`).
- **Danske kompagnier** får regimentets egne kampfaktorer fra Units.json:
  - ladetid × `reloadTime` × `battleRules.infantry.reloadFactor`;
  - træfsandsynlighed × `accuracy` × `doctrine.infantryFactor`;
  - moral og samhørighed;
  - ildpolitik fra `aiDefaults.firePolicy`.
- **Ildmetoderne** følger designet: først forsk, derefter træn, så brug.
  - Fire nye forskningsemner: To-geleds ild (1852), Geledild (1853), Kommanderet salve (1855) og Fri ild (1857).
  - Regimenterne indøver de udforskede metoder i garnison (eksercits, skydeøvelser 0,8, blandet 0,4): ca. 40 dage til 60 under en jævn chef.
  - Værdierne gemmes (`FireDrills`) og vises i regimentsvinduet ("Ildmetoder ... ✓" fra 60). De går ud i Units.json som `fireDrills`.
  - Slaget giver kompagniet den højeste metode, der er indøvet til 60. Geledild er standard, når den er åben.
- **HUD:** ILDMETODE-rækken (1.GLD, 2.GLD, GELED, SALVE, FRI); låste metoder er dæmpede og afvises.
- **Animation:**
  - Kun det geled, hvis tur det er, løfter og skyder. Pladsens geled er slot % geledder, samme regel som simuleringen.
  - Salven går af samlet (0,2 s), geledild som en bølge (0,6 s) og fri ild mand for mand (2,5 s).
  - Hver mand lader sin egen fulde ladetid, så geledderne skifter rytmisk.
- Jordmaterialet bruger kun jordtekstur, hvor farven er grålig-brun (vej, bygrund, tråd). Gule kornmarker beholder græsdetaljen.

### Artilleri, testslag, sejrsregel, tjenestejournal, tooltips og kampagnens brugerflade (5.-6. oktober 2026)
- **Artilleri i 3D** (`UStrategyArtilleryVisualComponent`, oprettes af batteriets konstruktør):
  - kanonmodellen (×2,2) eller morteren for hver kanon, i linje med 15 m mellem kanonerne og i kolonne under march;
  - deaktiverede kanoner står skævt, ødelagte ligger væltet;
  - QA-kassen holdes skjult.
- **Testslaget** (kortet `Strategy1864_Skirmish` eller `-Strategy1864Skirmish=1..4`): en dansk bataljonsstab med 2 kompagnier, som spilleren styrer, mod 1–4 fjendtlige kompagnier med stab og AI, 400 m væk på engen, i fuld figurskala, med faner og kamera bag linjen.
- **Sejrsregel for alle slag** (`GetBattleScore`/`UpdateBattleOutcome`):
  - Kun kæmpende enheder tæller. En side er slået under 35 % af startstyrken, eller når alle dens enheder er brudte.
  - HUD'en viser styrken øverst og banneret SEJR, NEDERLAG eller UAFGJORT.
  - `BattleResult` bruger den afgørelse, når den er faldet.
  - Kompagnierne tæller deres træffere (`TotalHitsInflicted`), og de går ud som `kills` pr. enhed.
- **Tjenestejournal** (`FCampaign1851ServiceEntry`, gemt i `FCampaign1851RegimentSave::Service`):
  - hvert slag med dato, sted, udfald, faldne, sårede, fangne og fjender sat ud af kampen, samt totalerne;
  - 3D-slagenes tal, eller ved automatisk afgørelse fjendens tab fordelt efter styrke.
- **ENHEDSKORT** i enhedspanelet:
  - uniformspladen (fra uniformsreferencerne, `Tools/Campaign/make_uniform_cards.py`; husarerne har deres egen), fanen;
  - mand, erfaring, moral, samhørighed og øvelser;
  - sårede og syge med halveringstid for hjemkomsten (de vender tilbage til egen enhed med erfaringen);
  - chef med vurdering, ildmetoder og journalen.
  - Et fodbatteri kan gøres ridende (12.000 rd., 6 kanoner, 180 mand, 230 heste).
- **Tooltips:**
  - Alle knapper har en forklaring (`ButtonTip`), og labels kan få deres egen (`AddTip`).
  - Tabellernes forkortede overskrifter forklares, og officerens evner har deres betydning.
- **Bekræftelsesdialog** (`AskConfirm`, JA/NEJ): mobilisering (pris, løn, skat, stemning, spænding), ridende batteri og deling af en enhed.
- **Officerer:** samlet vurdering 0–100 (`Campaign1851Army::OfficerRating`) på kortet og som kolonnen "Vurd".
- **Forskning:**
  - Emner på samme niveau i samme gren står side om side (Infanteriet overlappede).
  - Ny gren, Næringsliv: mergling (+8 % landskat), Landbohøjskolen (+7 %), smede (værker +15 %), dampmaskiner (værker +25 %, byskat +5 %), kreditforeninger (byskat +5 %).
- **Byer:**
  - Bygningslisten ruller (hjul, pile, bjælke) og er en række lavere. Klik på billede og navn åbner bygningens kort (giver, kræver, pris, vedligehold, betingelser, status her).
  - 18 nye bygningskort er genereret med ComfyUI/SDXL (`Tools/Campaign/make_building_cards.py`, import med `import_building_cards.py`).
- **Amter:**
  - Navne og værdier holder sig inden for kassen.
  - Kontrol-linjen viser, hvad der skal til. En besat by befries af 300 danske soldater inden for 3 km i 2 døgn uden fjendtlige korps inden for 10 km.
- **Udenrigs:** spændingen med Det tyske forbund vises dér (ikke i Statsrådet) med en kortere tekst, og der er en ALLE LANDE-knap.
- **Budget:** Statskassen har ministeriernes budgetter (−/+), puljerne og den mindste kassebeholdning. Statsrådet har knappen BUDGET OG KASSE.
- **Kamporden:**
  - Fra enhedspanelet vises kun den enhed med dens kompagnier. DEL I TO laver en halvbataljon (`SplitRegiment`, `bDetached`: intet ekstra underhold).
- **Kortvisninger** over Bornholm-boksen: NORMAL, FORSYNING og KONTROL (besatte byer med befrielsesstatus).
- **ANGRIB** i enhedspanelet: et opklaret fjendtligt korps inden for 15 km kan angribes (`EngageCorps`).
- **Portrætter:** en pulje af officerer, generaler og ministre i 1850'er-oliemaleri (`Tools/Campaign/make_portraits.py`). De tildeles fast efter navn og er typer, ikke ligheder.


### 2026-10-06 — Bekræftelse før køb og valg
- Fælles `DescribeAction` i `Campaign1851PlayerController.cpp`: før et skridt der koster penge eller ikke kan gøres om, vises en JA/NEJ-dialog med hvad der sker og hvad det koster (pris, udbetaling, dage, drift eller rente).
- Dækker:
  - byggeri: bygninger, garnison og moduler, chausséer og jernbaner, skanseudbygning (kanoner, forsvar, løbegrave), broer (sprænge, genopbygge, ponton);
  - hæren: bataljon og nye enheder, trænkolonner, mortérer og vogne, store råvarekøb, togsæt og togoverførsel;
  - officerer og ministre: ansættelse, forfremmelse, afsked, udnævnelse;
  - forskning og doktrinskift, lån og afdrag, skibsbestilling, blokade;
  - diplomati (gesandt, traktat, alliance, garanti) og fredstilbud med de byer der afstås;
  - automatisk afgørelse og tilbagetog i slag.
- Små skridt, fx det lille råvarekøb og knapper der kun viser eller vælger, går stadig igennem straks.


### 2026-10-06 — Kamporden del/saml, ridende batteri, hærens status
- **Kamporden (#24):**
  - I enhedens kamporden står begge halvdele, når enheden er delt.
  - Kompagnier kan trækkes fra den ene halvdel til den anden og tilbage (`MoveCompany`). De skal stå samme sted, og en halvdel beholder mindst ét kompagni.
  - SAML IGEN, eller træk den ene halvdel hen på den anden, samler dem efter en JA/NEJ-dialog (`MergeRegiments`). Erfaring og øvelse blandes efter mandtal, og den overtallige chef bliver ledig.
  - Ny `RemoveRegimentAt` retter alle henvisninger til enheder: officerer, skansernes kompagnier, slag, tog, trænkolonner og figurerne på kortet.
- **Ridende batteri (#8):** knappen GØR RIDENDE findes nu også i enhedspanelet og i kampordenen for fodbatterier, ikke kun på enhedskortet.
- **Hærens status (#9):**
  - Nyt vindue fra Krigsministeriet i Statsrådet og fra Hæren-vinduet.
  - Viser hæren pr. våbenart: enheder, i felten, mand, til stede, syge og sårede, heste, kanoner, morterer, erfaring.
  - Viser vores tab (faldne, sårede, fangne, i lazarettet) og fjendens tab (dræbt, såret, fanget, sat ud af kampen).
  - Viser erobret udstyr og de enheder, der har kæmpet mest.
  - Erobret udstyr i `ApplyBattle`: den side, der holder slagmarken, samler geværer, kanoner, heste, vogne og faner. Det går på lager og gemmes (`wartotals`).

### 2026-10-06 — 3D-slaget: kanoner, morterer, rytteri, kolonnemarch
- **`AStrategyBattleBlast`:** effekter af kugler, cylinderskiver og punktlys, uden partikelassets.
  - Mundingsflamme og røgbanke, nedslag med jordfontæne, støv og mærke i græsset.
  - Granat med ild, sort røg og krater, og shrapnel, der springer i luften og sparker støv op.
  - Kardæsk som støvkegle og hovstøv fra rytteriet.
- **Kanonkuglen ses i luften**, og en rundkugle, der hopper hen over marken, kaster jord op ved hvert spring. Fejlsøgningsstreger vises kun på flag.
- **Mortéren** viser nu sine skud: høj bue, egen flyvetid og en stor granateksplosion.
- **Tabene falder, når skuddet lander**, og nærmest nedslaget (`FStrategyImpactRegistry`).
  - Kardæsk dræber inden for keglen fra kanonen, som på brugerens tegning.
  - Kardæsk på kort hold giver op til tre gange så mange tab (12–24 pr. kanon på 100 m).
- **Rytteriet** vises som rækker af heste og ryttere (`UStrategyCavalryVisualComponent`).
  - Kroppen hæver sig og vipper i skridt, trav og galop, og hovene laver støv.
  - Rytteren sidder i sadlen og trækker sablen ved chok. Faldne heste vælter om på siden og bliver liggende.
- **March i kolonne** (`UStrategyFormationPolicyComponent`):
  - Infanteri og rytteri går i kolonne ved en lang marchordre (rytteriet i `CavalryColumn`).
  - De deployerer til deres formation fra før marchen, når fjenden kommer inden for skudvidde plus en margin, 25 m før målet, eller når de standses.
- **Test:**
  - `-Strategy1864SkirmishArms` giver hver side et batteri, en mortér og en eskadron. Husarerne angriber.
  - `-Strategy1864Shots=sek:enhed:afstand:side,...` tager skærmbilleder fra spillet selv (`-Strategy1864ShotsQuit` lukker bagefter).
- **Rettelser:** QA-klodsen i karréerne er skjult, pointtallet tæller højst den oprindelige styrke, og advarslen om RiderSocket fyldte loggen.


### 2026-10-06 — Feltofficerer, fjendens skudvidde, fjenden som angriber/forsvarer
- **`UStrategyFieldOfficerComponent`** på alle enheder fører kampen inden for ordren, når AI er ON. Spillerens direkte ordrer går forud.
  - **Kaptajnen:**
    - rykker frem til sin skudvidde ved ANGRIB/RYK FREM mod fjendens fodfolk eller kanoner, højst 400 m forbi målet;
    - svinger fronten mod en fjende fra siden og holder formationen, når ikke-vaklende rytteri er inden for 150 m;
    - går til bajonetangreb, når fjenden vakler inden for 60–110 m (afhængigt af aggressivitet). Han løber i dobbelt tempo med påsat bajonet, og chokket afgøres af mandtal, moral, samhold og karré;
    - trækker sig tilbage ved under 40 % af mandskabet eller moral under 22.
  - **Rytterofficeren:** chokerer det nærmeste mål, der er åbent for chok (et batteri uden dækning, en kolonne, en vaklende eller flygtende enhed, men aldrig en karré). Han holder afstand til formeret fodfolk og trækker sig tilbage, når eskadronen er slået.
  - **Batterichefen:** vælger selv mål og ammunition.
  - Hans aggressivitet og taktiske evne påvirker, hvor tidligt han angriber, og hvor ofte han tænker.
  - Kommandopanelet viser "OFFICEREN:" med hans beslutning og begrundelse.
- **Fjendens skudvidde** vises ikke længere som standard. INDSTILLINGER har "Fjendens skudvidde (til test)" (gemmes) og flaget `-Strategy1864ShowEnemyRange`.
- **Fjenden ANGRIBER/FORSVARER (test)** under INDSTILLINGER (`SetEnemyAttacking`), og flaget `-Strategy1864EnemyDefends`.
  - Angriber: fjendens stab angriber den danske linje.
  - Forsvarer: FORSVAR HER, hvor han står, med fronten mod danskerne.
- `-Strategy1864SkirmishAttack`: majoren beordrer angreb ved start.
- **Rettelse:** skudkeglens stiplede linjer kunne blive til millioner af streger, når et punkt lå lige foran kameraet. Spillet løb tør for hukommelse. Sådanne linjestykker springes nu over.


### 2026-10-06 — Skærmbilledrettelser i kampagnen, civile bygningers indtægt, billedliste
- **Enhedskortet** er en uigennemsigtig boks i forreste lag, der tager klikkene, og står uden for kampordenen. Chefens portræt er i en ramme.
- **Officerskortet** har et stort portræt i en ramme (`PaintPortraitBox`), samlet vurdering stort og evnerne i fuld bredde.
- **Ministre:** klik på ministerens billede eller navn i Statsrådet. Kortet viser portræt, ressort, tiltrædelse, strømning, evner, arbejdsform, budget og de seneste beslutninger.
- **Hærens status:** knappen står nu øverst i Statsrådets højre kolonne (den lå over Transport-rækken).
- **Statskassen:** ministeriernes budgetter og kassebeholdningen står i højre kolonne over regnskabet, så intet løber ud af kassen.
- **Bygningslisten:** kommer aldrig op under menulinjen. Den står ved siden af kampordenen, hvis den er åben. Bygningens kort er uigennemsigtigt og i forreste lag.
- **Valgpanelet** lægger sig under statskassen og ikke over den.
- **Civile bygninger giver indtægt** (`FCampaign1851CivilEffect`: `Jobs`, `TradeRd`, `IncomeRd`).
  - Arbejdspladser: arbejderne betaler 6 rd. om året pr. plads i skat.
  - Told og eksport: havn, handel og industri.
  - Afgifter og gebyrer: rådhus, post med mere.
  - Budgettet og regnskabet viser tre linjer (told og eksport, skat af arbejdspladser, afgifter). Byggelistens linjer viser jobs og årligt udbytte, og kortet viser hele opgørelsen.
- **Billeder:** `Docs/BILLEDER-TIL-SPILLET.md` har lister med filnavne, størrelser og regler for alle billeder (forfra, ens stil). `Tools/Campaign/Import-Billeder.bat` importerer dem. Portrætterne vælger det næste billede, der findes, hvis puljen er under tolv.
- **Test:** `-CampaignUiShots=sek:cmd;cmd,...` åbner vinduer og kort og gemmer skærmbilleder (`-CampaignUiShotsQuit` lukker bagefter).


### 2026-10-06 — Slagmarken: hvede og marchstøv
- **Moden hvede** på de okkergule marker (`CropAt` i `StrategyCampaignBattlefield`): gule marker (rød en smule over grøn, lidt blå) får stående hvede i stedet for grønne græstotter.
  - Hvedetotterne er krydskort på 0,7–0,8 m, to pr. punkt, og de svajer i vinden.
  - Kortet `T_Card_Wheat` laves af `Tools/Battle/make_wheat_card.py`, meshene (`SM_Wheat_A/B`) af `make_battle_meshes.py`, og `Content/Python/import_wheat.py` importerer kun hvedens egne aktiver.
  - Græstotterne er væk fra markerne (`GrassAt` tæller okker som jord).
- **Marchstøv:** en marcherende kompagnikolonne rejser støv bag sig, kun nær kameraet.
- **Advarsel:** `import_battle_graphics.py` genopbygger hele udseendet og gav en anden jordtekstur end den, der ligger i Content. Kør den ikke igen uden at sammenligne. Hvede-importen er skilt ud for netop det.


### 2026-10-06 — Slagmarkens huse og marker
- **Husene** (`Campaign1851Scenery.cpp`, bruges både i slaget og på kampagnekortet): taget i skifter (to nuancer skiftevis), rygkam, mørk tagfod og mørk sokkel. Vinduerne har lyse karme, en sprosse og tværsprosse og, i bygninger op til to etager, grønne skodder.
- **Markerne** får tone i parceller på omkring 75 m og pletter på 14 m, og en let varm/kold forskydning, så hver mark er lysere eller mørkere end naboen. Hvede og jordklassificering bruger de oprindelige farver.
- **Etiketten** (`[XX] Felthæren`) drejer nu mod kameraet og står ikke længere spejlvendt.


### 2026-10-06 — Kamporden fra en enhed
- KAMPORDEN på de valgte enheder åbner det store kamporden-vindue og viser kun de valgte enheder (`FilterOOB`). HELE HÆREN viser alle igen.
- DEL I TO og SAML IGEN ligger inde i vinduet (ved én valgt enhed). Efter delingen vises begge halvdele som store bokse med deres kompagnier, som trækkes mellem dem. Tilbage fører til listen.
- Vinduet har en uigennemsigtig bund, så teksten ikke ligger oven på kortets tegning.
- Feltgrænserne (hække og parceller) i slagmarksgeneratoren er ændret til en drejet og bøjet inddeling (`FParcelGrid`) med forskellige bredder og vinkler, i stedet for et firkantet net. Kontrolleret på et nyt kort ved Rendsborg: markerne er drejet og uregelmæssige, og hækkene følger kanterne.


### 2026-10-06 — Deling i kamporden: den nye enhed bygges i midten
- KAMPORDEN på valgte enheder viser kun dem. Venstre side hedder I GARNISON, når enhederne står hjemme, ellers DEN VALGTE ENHED (foldet ud med kompagnier, eller eskadroner for rytteriet).
- Midten er tom med en dropzone. Det første kompagni (eskadron), man trækker derover, bliver en ny enhed (`SplitOffCompany`) og bliver stående i midten. Hvert næste kompagni, man trækker over, lægges til den (`MoveCompany`), så enheden bygges.
- Rytteriregimenter deles i eskadroner på 140 mand (`SubUnitCount`, `SubUnitMen`), og eskadroner kan flyttes mellem halvdelene.
- Det sidste kompagni (eskadron) trukket hen på den anden halvdel samler dem (`MergeRegiments`). SAML IGEN gør det samme.
- Uniformkortene bruger nu `pose_Front`-billederne (stående, ikke A-pose) fra `Reference/Units/MultiView` (`make_uniform_cards.py`).
- **Ikke afprøvet:** selve trækningen i spillet (testen kan ikke trække med musen); layoutet og de to varianter af overskriften er kontrolleret på skærmbillede.

### Test af slagmarken
- `-CampaignUiShots=sek:genfield=lat+lon` genererer et slagmarkskort (`Saved/Battle/Battlefield_Test.json`) uden at spille et slag. Det findes i 3D med `-Strategy1864Field=Battlefield_Test.json`. (`-CampaignBattlefield=` virker ikke sammen med `-CampaignNew`.)

### Genveje til 3D-slaget
- `Start-Slagmark-Generer.bat` laver et nyt slagmarkskort ved Rendsborg (`Saved/Battle/Battlefield_Test.json`) og lukker spillet igen.
- `Start-3D-Slag-Test.bat` åbner 3D-slaget på det kort (kamporden med O, taktisk kort med M, INDSTILLINGER øverst).
- `Start-3D-Skirmish-Test.bat` åbner det lille slag: 2 mod 2 kompagnier på en eng med batteri, mørtel og eskadron på hver side. Fjenden angriber, og hans skudvidde vises.
- Himmellyset i slaget er hævet til 2,2 (skyggesiderne var for mørke).

### 2026-10-07 - Codex-review af kamporden: 18 fund rettet

Et uafhængigt Codex-review af del/saml/flyt-koden fandt fejl, rettet her:
- **Gem/indlæs (#4-6):** antal kompagnier, MaxMen, heste, kanoner og nation gemmes nu pr. enhed (`FCampaign1851RegimentSave`), så en delt enhed ikke vender tilbage til fire kompagnier og fuld normering.
- **Forter (#1-3):** fort-poster følger kompagniet ved deling og ved ombytning i `SplitOffCompany`; mandskab fordeles kun over feltkompagnier.
- **Id'er (#7-8):** `SplitBase`/`FreeSplitId`: halve af halve kan samles igen, og ingen dublet-id.
- **Slag (#12):** `IsInBattle` blokerer deling og kavaleriflyt midt i et slag.
- **Flyt/saml (#10-11, 13-15):** tilstedeværende, forsyninger og heste følger kompagniet; foder vægtes efter heste; eskadroner summer til regimentet, max 10, ingen enhed uden mand.
- **Beslutninger (#9):** ventende ministerråd (træning, kaptajn til kompagni) flyttes/droppes når en enhed fjernes.
- **Nation (#16), forhåndstjek (#17), pakning af sammenlægning 1000 -> 10000 (#18).**

### 2026-10-07 - Flyttes sidste kompagni/eskadron, nedlægges enheden

`MoveCompany` afviser ikke længere den sidste; hele enheden går over til modtageren og tomme enhed fjernes (`RemoveRegimentAt`). `OutTo` giver modtagerens nye indeks, så kamporden og toast peger rigtigt.

### 2026-10-07 - Styrke pr. kompagni: fordel mændene mellem kompagnierne

Før delte `CompanyMen` bataljonens mænd ligeligt, så kompagnierne kunne ikke være ulige stærke. Nu har hvert kompagni en vægt (`CompanyWeight`, gemmes): `CompanyMen` fordeler `Men` efter vægtene (summen er præcis `Men`), så tab, sygdom og alt andet, der ændrer `Men`, virker uændret og rammer kompagnierne forholdsmæssigt.
- Kamporden: trækkes et kompagni hen på et andet i samme bataljon, deler de mændene lige (`BalanceCompanies`, max normeringen pr. kompagni). Knappen "Udjævn kompagnierne" (`EqualizeCompanies`) giver alle det samme. Kompagnilisten viser nu mand/normering.
- Deling, flyt og sammenlægning (`SplitRegiment`, `MoveCompany`, `MergeRegiments`) fryser vægtene først (`FreezeCompanyStrength`) og tager kompagniets rigtige mandskab med.
- Ældre gemte spil: ingen vægte = lige fordeling.

### 2026-10-07 - Flyt et valgfrit antal mand mellem kompagnier

Træk et kompagni hen på et andet i samme bataljon: et vindue (`PaintTransfer`) spørger, hvor mange mand der flyttes. Knapper: -10, -1, +1, +10, ALLE (højst hvad giveren har og modtageren har plads til) og LIGE (jævner de to). FLYT udfører (`TransferCompanyMen`), FORTRYD lukker. `CompanyCapacity` er normeringen pr. kompagni.

### 2026-10-07 - Eskadroner og andre enheder: flyt mænd på tværs

- Rytteriets eskadroner har nu egen styrke (samme vægte som kompagnier). `CompanyMen`/`SubUnitMen`/`FreezeCompanyStrength`/`CompanyCapacity` gælder begge dele; "Udjævn" findes også for eskadroner. Deling og flyt af en eskadron tager den valgte eskadrons faktiske mandskab med.
- `TransferCompanyMen` flytter mænd mellem to kompagnier/eskadroner også i to forskellige enheder (samme våbenart, samme sted, ikke på march, ikke i slag). Mændene tager træning, moral og forsyninger med (blandes efter mandskab). Vinduet har en knap "FLYT HELE ENHEDEN" mellem to enheder (flytter hele kompagniet/eskadronen som før).

### 2026-10-07 - Kortet udvidet: Bornholm på kortet og et grovt verdensark under

- Kortgeneratoren ligger nu i projektet: `Tools/Map1851/build_map.py` (Natural Earth 10m i `SourceAssets/NaturalEarth`, ikke i git). Kør `python build_map.py <NaturalEarth> --height=4096 --out=<mappe>` og kopier filerne til `Reference/Campaign1851` og `Data/Campaign1851`.
- Det detaljerede kort er udvidet til 7.6-15.75 °Ø og 53.0-57.9 °N (545 x 558 km): Bornholm, Skåne, Blekinge, Halland, Gøteborg, Mecklenburg, Pommern og Bremen er med. Bornholm-boksen (indsat kort) er fjernet; Rønne ligger på kortet med færgen København-Rønne. Nye færger: Alssund (Sønderborg), Vilsund (Mors) og Langeland-Svendborg, så de byer ikke står uden forbindelse.
- Nye udenlandske byer (Sverige, Mecklenburg, Pommern, Bremen); nye etiketter.
- Nyt lag: et groft verdensark (`World1851_Color`, 3000 x 3000 km, samme projektion) ligger under det detaljerede kort (`WorldSheet` i C++, materialet `M_Campaign1851World`); det detaljerede kort fader ud i det. Norge, Sverige, Storbritannien, Nordtyskland, Polen og Frankrig ses der. Senere kan flere naboegne gives egne detaljerede lag oven på.
- Opsætning i editoren: `Tools/Campaign/setup_world_sheet.py` (importerer teksturer, bygger materialet).

### 2026-10-07 - Sejrspoint og mål i 3D-slaget

- Hvert slag (kampagne-slag og skirmish) får fem mål (`FBattleObjective` i `AStrategyOOBTestScenario`): vor stilling (100), midten (150), venstre og højre fløj (100 hver) og fjendens stilling (100). De to stillinger ejes fra start; resten skal tages.
- Et sted tages ved at have mindst 30 mand i cirklen (ca. 60-120 m), mens modstanderen har under en tredjedel; det tager 90 sekunder, og stedet skifter ejer ved halvvejs (Progress -1..+1). Brudte/udslåede enheder tæller ikke.
- Afgørelse: en side der holder alle mål i et minut vinder; efter 90 minutters kamp vinder den med flest point; ellers gælder de gamle regler (en side under 35 % af sine mænd). Afslutter spilleren slaget før, afgør point (forskel på mindst 100) før mandskabsandelen. Resultatfilen har `danishPoints`/`enemyPoints`.
- Visning: ringe og flagstænger i verden (debug-tegning), flag med navn, værdi, ejer og fremdriftsbjælke i HUD'en, og "MÅL a : b · tid" i panelet øverst.
- Ikke afprøvet i et helt slag endnu (kun opsætning og visning kontrolleret).

### 2026-10-07 - Ordonnanser og officerer, der læser ordrer

- Spillerens ordrer i 3D-slaget (`IssueOrderToSelection`) går nu med en rytter fra hærens stab (den højeste danske stab uden overordnet) til enheden. Inden for stabens egen cirkel (320 m) kaldes ordren ud med det samme; længere væk tager det 2 s + afstand / 12 m/s (ca. 35 s til en enhed 400 m væk). En ny ordre til samme enhed erstatter den, der er på vej.
- Rytteren tegnes som et guldfarvet mærke med spor fra staben; enheden får en tag "Ordre på vej m:ss".
- Når ordren er fremme, læser enhedens officer den (`DeliverOrder`): reaktionstid 2-14 s efter stabsarbejde/ledelse/disciplin (`GetCommandEfficiency`); en usikker og dårlig officer (`GetDecisionStability`, `TacticalSkill`) forstår med op til 35 % sandsynlighed stedet løst og går til et punkt op til ca. 20 % af afstanden ved siden af. Spilleren får besked øverst på skærmen.
- INDSTILLINGER har en ny knap: Ordonnanser TIL/FRA (gemmes; `-Strategy1864NoCouriers` slår dem fra). `-Strategy1864TestCourier` sender en testordre til den fjerneste enhed.
- Ikke lavet endnu: officerer der nægter eller afviger efter forsigtighed/initiativ, ordrer videre gennem bataljon og brigade (kæden), og forstærkninger i slag over flere dage.

### 2026-10-07 - Et slag varer højst tre dage

Tidsgrænsen for pointafgørelsen er sat til tre dage (259.200 s spilletid) i stedet for 90 minutter; panelet viser "Dag d af 3 t:mm".

### 2026-10-07 - Ordonnansen er en rigtig rytter

`AStrategyCourierRider` (Visual): en enkelt rytter på kavaleriets hest- og rytter-modeller galopperer fra staben til enheden (1200 cm/s, følger enheden hvis den flytter sig), støver op og venter, indtil ordren er afleveret; derefter rider han tilbage og forsvinder. Ordren afleveres, når rytteren er fremme (ikke efter et tidsur). Skudkommandoen `-Strategy1864Shots=sek:RIDER:afstand:side` følger rytteren.

### 2026-10-07 - Tid i slaget: ur, sol, nat

- Slaget har et eget ur (`BattleClock`), der løber 6 gange hurtigere end spillet (`ClockRate`; 1 spilsekund = 6 sekunder på uret), så en dag tager ca. 4 timer ved 1x. Starttidspunktet er 07:00 (`-Strategy1864StartHour=`, `-Strategy1864ClockRate=`). Panelet øverst viser "Dag d af 3 · kl. hh:mm". Tidsgrænsen på tre dage regnes på uret.
- Lyset følger uret (`AStrategyBattleAtmosphere::UpdateForHour`): solens højde og retning udregnes af dagen på året (fra slagets dato i BattleRequest) og breddegraden 55,7 °N; en lav sol er rød, skumring, og om natten et blegt blåt måneskin, mørkere himmel og tåge. Kontrolleret ved kl. 05, 22:30 og 01:30.
- Ikke med endnu: at synsvidde og kampkraft følger lyset, at hærene hviler om natten, og forstærkninger ved daggry.

### 2026-10-07 - Alle 100 portrætter og nye bygningskort er i spillet

- Portrætterne fra `Overførsler\Strategy1864_Portrætter` (30 unge, 30 ældre og 15 gamle officerer og 25 ministre, 512 x 640) ligger i `Reference/Campaign1851/Portraits` og er importeret (`Import-Billeder`/`import_building_cards.py`). Officerskortet viser dem efter alder med rangmærket tegnet oven på (kontrolleret på et skærmbillede).
- De 20 bygninger fra `Strategy1864_Buildings_All_20` (768 x 768) ligger uændret i `Reference/Campaign1851/Buildings/Strategy1864`. De der passer til en bygningstype uden billede er lavet til kort (512 x 512): `T_Bld_Barn`, `Blacksmith`, `Stable`, `Railway_Station`, `Church`, `Windmill` og `Watermill`. De øvrige (Kommandobygning, Infanteribygning, Kanonbygning, Officersbolig, Mandskabsbolig, Portnerbolig, Markedshal) venter på en bygningstype.

### 2026-10-07 - Sårede og fangne officerer (aldrig dræbte)

- **3D-slaget:** hver enheds officer har nu navn og rang (de danske fra kampagnen via Units.json: kaptajnerne og bataljonschefen; fjendens er opdigtede). Mister enheden mænd, kan officeren blive såret (0,4 % pr. mand, mindre for stabe); brydes enheden (flugt eller udslettet) med fjenden inden for 40 m, kan han tages til fange (35 %, 55 % hvis allerede såret, mere ved udslettelse; roligere officerer slippes lettere). Såret giver 60 % og fange 40 % af hans evner i resten af slaget (`Impairment` i `UStrategyOfficerProfileComponent`). Ingen dør. Beskeder øverst på skærmen. `-Strategy1864TestOfficers` tvinger et sår og en fange (test).
- **Resultatfilen** har `officers` (vore såret/fanget) og `enemyOfficers` (fjendens, vi har taget).
- **Kampagnen** (`ApplyOfficerCasualties`): en såret officer forlader sin post og er væk 3-12 uger; en fange i op til ti uger (udvekslet tidligere jo flere fjendtlige officerer vi holder). Posten står ledig og kan besættes af råd/spiller som ellers. `DailyOfficers` sender dem hjem igen. Officerskortet viser "Såret (tilbage ca. d.m.)" eller "Krigsfange". Gemmes i savet (`Away`/`AwayUntil`).
- Ikke med: officerer i fjendens kampagnekorps (kun en tæller af fangne fjendtlige officerer), prestige/løsesum for fangne, og at en fanget chef får enheden til at miste moral.

### 2026-10-07 - Knapper: stilling (stå/knæ/lig) og dragoner sidder af

Kommandopanelet har for kompagnier en række STILLING: STÅ, KNÆ, LIG (`UStrategyStanceComponent`: knælende rammes 18 % sjældnere, liggende 35 % sjældnere, men lader langsommere og bevæger sig langsomt; gælder alle valgte enheder). For dragoner (`Role = Dragoon`, sat fra regimentets type) er der SIT AF og STIG PÅ (`DismountAtCurrentPosition` / `RequestRemount`). Før fandtes logikken, men ingen kunne bruge den i spillet.

### 2026-10-07 - Fangne officerer: moral, prestige, løsesum; officerer der tøver

- **Moral i slaget:** tages en chef til fange, mister hans enhed 15 moral og 10 samhørighed; en stabsofficer (bataljonschef osv.) tager 6 moral fra hver enhed inden for 200 m.
- **Prestige:** fanger vi fjendtlige officerer, stiger stemningen hjemme (`PoliticalShock`): 0,3 pr. kaptajn, 0,6 pr. major, 0,9 pr. oberst, højst 4. Mister vi en officer, falder den (0,3 og op efter rang).
- **Løsesum:** et krigsfange-kort har knappen LØSEKØB (`RansomOfficer`): kaptajn 600 rd., major 1.500, oberstløjtnant 2.200, oberst 3.500, generalmajor 8.000, generalløjtnant 12.000, general 20.000; officeren er hjemme med det samme. Ellers udveksles han efter højst ti uger.
- **Officerens egen dømmekraft** (ordonnansens aflevering, `DeliverOrder`): en forsigtig og uaggressiv officer (Forsigtighed over 70, Aggressivitet under 45) angriber ikke en fjende, der er mere end 1,6 gange så stærk som hans egne mænd, men holder sin stilling og siger det. En dårligt disciplineret officer (Disciplin under 35) er i 15 % af tilfældene 30-60 sekunder længere om at udføre en ordre.

### 2026-10-07 - Forstærkninger i slag over flere dage

- Kampagnen sender `reserveUnitIds` i slagbestillingen: de tre nærmeste danske regimenter, der ikke er i et slag, står stille inden for 60 km og har mindst 300 mand i feltet.
- I 3D-slaget bygges de som alle andre enheder, men venter ude af spillet (skjult, tidsdilatation 0, uden kollision: `FreezeReserve`/`DormantUnits`) og tælles hverken i stillingen, målene, officerernes skæbner eller resultatet, før de kommer. Kl. 06 på anden og tredje dag (på slagets ur) rykker de ind bag egen linje (`TickReserves`), med en besked øverst på skærmen.
- Fjenden får 2-8 kompagnier fra sit korps (to hvis det har under 16 kompagnier i forvejen; en i et lille slag), delt i to hold til anden og tredje morgen.
- Kontrolleret med uret sat hurtigt (`-Strategy1864ClockRate=3000`): fjendens første hold kom, og "FJENDEN" gik fra 1520 til 1900 mand. De danske reserver er ikke kørt igennem endnu (testen har ingen slagbestilling).

### 2026-10-07 - Kompagnier rykker ind fra siden i stedet for at stå oven i hinanden

Før gik hvert dansk kompagni lige mod den nærmeste fjende og endte oven i hinanden og i hinandens ildfelt. Nu (`AssignFlanks`/`ApproachGoal` i `UStrategyFieldOfficerComponent`) arbejder kompagnierne under samme stab som ét hold mod samme fjendtlige kompagni:
- Det mellemste kompagni er **ildbasen**: det rykker til sin skudvidde, standser og skyder.
- De andre er **flanker**: de går udenom ildbasens ildkegle (en bue uden om en kegle på ±32° fra basen mod fjenden) og tager plads i en vinkel til fjenden, 45° for det næste ved siden af basen og op til 80° for det næste, på samme skudafstand, så de skyder ind i hans side. Pladserne holder dem fra hinanden, og de rykker frem samtidig.
- Rollerne gælder i 90 sekunder og fordeles forfra, når holdet skifter fjende; de vises i enhedens AI-tekst ("ildbasen: holder og skyder", "flanken: ind fra siden", "går udenom egen ild").
- Test: `-Strategy1864TestFlank` giver de danske kompagnier en offensiv doktrin; loggen viser fordelingen. Prøvet i logfilen (fire kompagnier: en base, tre flanker med hver sit omvejspunkt), ikke set i et helt slag.
- Fjendens kompagnier følger stadig sit eget slagmarks-AI og bruger ikke dette endnu.

### 2026-10-07 - Sidetrin: kompagnierne flytter sig med fronten mod fjenden

Ordrer har et nyt felt `bKeepFacing`: enheden holder fronten mod `FacingYaw`, mens den bevæger sig (`StrategyMovementExecutorComponent`, `bKeepFacingMove`), i stedet for at dreje mod den retning den går; til siden eller baglæns går den med tre femtedele af farten, hvis vejen ligger mere end 50° fra fronten. Kaptajnerne bruger det for korte flytninger (under 90 m) med fjenden tæt foran (`ThinkInfantry`), så kompagnierne kan rykke på plads i flanken uden at vende ryg eller flanke mod ilden. AI-teksten viser "sidetrin". Kun kompileret og logget; ikke set i et slag.

### 2026-10-07 - Codex-review af dagens slagkode: ni fund rettet

Et uafhængigt Codex-review (kørt direkte med `codex.exe`) fandt:
1. Flankens omvejspunkt lå på modsat side (forkert fortegn) og kunne gøre, at kompagniet aldrig nåede sin plads: rettet.
2. Et kompagni, der allerede stod inden for skudvidde, rykkede aldrig til sin flankeplads: flanken flyttes nu også inden for skudvidde, når den er mere end 20 m fra pladsen.
3. Ordren pegede mod en anden fjende end den, kompagniet gik mod: front og mål bruger nu samme fjende.
4. Ventende forstærkninger kunne udpeges og skydes på: `bOutOfPlay` på enheden gør `IsCombatEffective` falsk, til de kommer.
5. Et sted blev taget efter ca. 45 sekunder i stedet for 90: ejerskab skifter nu, når bjælken er fuld (90 s fra neutral); et ejet sted går til neutral, når bjælken passerer midten.
6. En flygtende chef kunne kun tages til fange i første sekund: der slås nu terning hvert andet sekund, så længe enheden er brudt og fjenden er nær.
7. En ældre ordonnansordre kunne overskrive en nyere: en ny ordre til samme enhed annullerer ordren på vej.
8. Manglende hestemodel blokerede leveringen for altid: rytteren er så usynlig, men leverer.
9. Solens dato stod fast efter første dag: dagen på året følger nu slagets dage.
Tillige: flere flankepladser pr. side får hvert sit omvejspunkt. Kontrolleret i logfilen (flankerne går først udenom ildbasen, derefter ind fra siden; forstærkningerne kommer).

### 2026-10-07 - Fjenden flankerer også; lederne bestemmer opførslen

- Fjendens kompagnier følger samme flankeplan (`FlankPlan` kaldes fra `UStrategyAutonomousBattleAIComponent`): ildbase i midten, resten ind fra siden og udenom ildbasens ildfelt. Et kompagni med en plads at gå til går dertil, før det holder, også når det allerede er inden for skudvidde.
- **Bataljonslederens evner** (taktik 50 %, initiativ 25 %, stabsarbejde 15 %, aggressivitet 10 %): under 0,35: ingen plan, alle går lige ind; 0,35-0,55: kun de to ved siden af basen flankerer, ingen omvej uden om ildfeltet; over 0,55: alle flankerer, vinklen bredere jo bedre lederen er. En kaptajn med disciplin under 40 går i 35 % af tilfældene sin egen vej.
- **Kaptajnens opførsel:** skudafstanden følger doktrin og dristighed (`PreferredFraction`, 55-90 % af skudvidden); hvornår kompagniet bryder, følger hans nerve (`GetDecisionStability`): den nervestærke holder til 22 % af styrken, den nervøse bryder ved 52 %.
- **Statistikker:** de danske officerers evner (1-10) fra kampagnen bruges nu i slaget (×10; tidligere stod alle på 50); fjendens officerer får tilfældige evner efter hærens kvalitet (±20, lidt bedre jo højere rang).

### 2026-10-07 - Rettelser fra testen: skydekegle, kavaleri, højre mus, glidende formationer, GPU

1. **Skydekeglen:** vinkeltallene (±35°) og zonenavnene (Kort/Mellem/Lang med [VALGT]) er fjernet; kun afstandsbuerne (40/70/100 m) står tilbage.
2. **GPU:** billedraten holdes på 60 (`t.MaxFPS`, før kørte GPU'en for fuld udblæsning; `-Strategy1864NoFpsCap` løfter den), og himmellyset optages ikke længere hver frame, kun når lyset ændrer sig. INDSTILLINGER har en ny række "Mænd vist": en figur for hver mand, hver anden eller hver femte (standard nu hver anden, før hver femte: der var for få mænd på marken).
3. **Formationsskift og stillinger:** mændene løber nu ud i linje eller ind i kolonne (420 cm/s, drejer efter vejen og vender fronten ind mod pladsen), i stedet for at blive teleporteret; skift mellem stå, knæl og lig sker mand for mand over halvandet sekund (`UpdateSettling`, `ApplyPendingStance`). Kontrolleret på skærmbilleder (kolonne og linje midt i skiftet, knælende og liggende).
4. **Husarer/kavaleri:** skydekegle kun når et dragonregiment er afsiddet; kolonnen er tre og tre; ingen karré for ryttere (KARRÉ-knappen vises ikke); husarer kan ikke sidde af.
5. **Højre mus:** med valgte enheder sender et klik dem derhen; holdes knappen nede og trækkes musen, tegnes en pil, og enhederne ender med fronten i pilens retning (flere enheder side om side; kameraet panorerer ikke, mens knappen giver en ordre; uden valgte enheder panorerer højre mus som før). Ikke afprøvet med rigtig mus.

### 2026-10-07 - Kamporden som organisationsdiagram ved deling, udfoldelige enheder, enhedskort pr. kompagni

- **Deling i kamporden:** når man deler en hær eller enhed, står hele felthærens organisationsdiagram (Felthæren, division, brigader, regimenter, bataljoner og kompagnier) nu i midten, så man kan lægge en brigade eller division på og trække enheder mellem dem; den nye enhed (kassen og "Træk herover") står i en kolonne til højre.
- **Garnisonslisten** (gruppen "I GARNISON" efter generalkommando) har en lille +/- ved hver enhed, der folder kompagnierne (eller eskadronerne) ud med kaptajn og mandskab.
- **Enhedskortet:** fanen er 25 % bredere (105 x 84); for en enhed med flere kompagnier/eskadroner er der knapper ALLE og 1-10: ALLE viser kortet som gennemsnit for enheden, et nummer viser det kompagni: dets mand (x/190) og dets kaptajn med portræt (erfaring, moral, øvelser og tjeneste er enhedens, og det står på kortet).
- Ikke gjort: at fjerne et højere hovedkvarter (division/brigade) direkte i delingsvisningen; kortet kan i dag kun flytte enheder mellem dem.

### 2026-10-07 - Scenarier: 1825 (standard) og 1851; rettelser

- **Scenarier** (`ACampaign1851Map::FScenario`, `Campaign1851Scenarios.cpp`): Danmark 1825 er standard, Danmark 1851 kan vælges i spilmenuen (række "Scenarie" ved NYT SPIL; kortet genindlæses for det valgte scenarie, og hvert gemt spil husker sit: `Scenario` i savet, gamle saves = 1851; `-CampaignScenario=1825|1851` til test). 1825 starter 1. juli 1825, byer og amter har 80 % af folketallet, hæren 55 % af styrken (kanonerne beholdes) og 80 % af erfaringen, spændingen starter på 10, og begivenhederne er vejen til 1848 (Julirevolutionen, stænderforsamlingerne, Christian VIII, Det åbne brev, Kiel, de preussiske tropper). Jernbanerne åbner på deres egne datoer (ingen i 1825). Officerernes alder følger startåret. Titelkortet viser årstallet.
- Første udgave: 1825 er 1851-data skaleret ned; rigtige 1825-tal for hær, officerer, byer og nationer mangler.
- **Målenes ringe** i 3D-slaget følger nu terrænet (de var flade og forsvandt ind i bakkerne).
- **Enhedskortets knapper** (ALLE, kompagni, flyt mænd, udjævn, løsekøb, fold ud): klikket falder ikke længere igennem til kortet, så kortet lukker ikke.

### 2026-10-07 - Reserver og to testslag: dansk kompagni og dansk bataljon mod svenskerne

- **Reserve:** en forsigtig og besindig bataljonsleder (Forsigtighed 40 %, taktik 30 % minus aggressivitet 20 % over 32) med fire kompagnier eller flere holder det bageste tilbage 90 m bag ildbasen (rolle 3 i `AssignFlanks`/`ApproachGoalAt`). Det sættes ind, når ildbasen er nede på 75 % af styrken eller under 55 moral, når basen er væk, eller efter fire minutter. `-Strategy1864HoldReserve` tvinger det (og giver chefen forsigtige evner).
- **Skirmish** kan nu køres mod svenskerne (`-Strategy1864SkirmishSwedes`: siden Enemy, svensk model og flag SE, "Sv. 1. Kp."), med 1-4 danske kompagnier (`-Strategy1864SkirmishDanes=N`).
- **Starter-filer:** `Start-Test-1-Kompagni-mod-Kompagni.bat` (et dansk kompagni går frem og skyder mod et svensk, som holder sin stilling og skyder tilbage, til det ene brydes) og `Start-Test-2-Bataillon-mod-Kompagni.bat` (en bataljon mod et svensk kompagni: et kompagni i reserve, midten holder og skyder, de to andre går udenom ildfeltet og ind fra siden). Kontrolleret i loggen og på et skærmbillede.

### 2026-10-07 - Forskning i 1825: emner åbner i deres egen tid; Jernbaneanlæg

- Forskningsemnerne har fået en åbningstid i 1825-scenariet (`ResearchOpenYear`): de tidlige (kæde- og karré-eksercits, rytterspejdning, smede, to-geleds ild) åbner 1826-28, de fleste 1830-45 (bagladegeværet 1841, felttelegrafen 1844, riflede kanoner først 1855); en lukket står med "åbner 19xx" i træet. I 1851-scenariet er alt åbent som før.
- Nyt emne **Jernbaneanlæg** (åbner 1835, 2.500 rd./md. i 18 måneder): uden det kan der ikke bygges jernbaner på kortet i 1825 (`LinkBlockReason`: "kræver forskning"); i 1851 kendes det på forhånd. Der er altså tog i 1825, men de skal først forskes frem; de eksisterende baner (1844 og frem) åbner på deres egne datoer.

### 2026-10-08 - Militær og civil forskning i to spor

- Forskningen er delt i to spor med hvert sit projekt ad gangen (`Researching`/`ResearchingCivil`, `MonthlyResearch`): det militære betales af Krigsministeriet, det civile af Indenrigsministeriet (hver med sin automatik/rådgivning og sin pengepost). Forskningsvinduet har faner MILITÆR og CIVIL med hver sine kolonner.
- Civile emner (`Campaign1851Research::IsCivil`): hele Næringsliv-kolonnen (mergling, landbohøjskole, smede, dampmaskiner, kreditforeninger, nyt: Landboreformer, skat fra landet +6 %), samt Jernbaneanlæg, Felttelegrafen og nyt: Vej- og kanalbyggeri (chausséer 15 % billigere). Jernbanemobilisering og pontonerne er militære.
- De nye emner åbner i 1825-scenariet 1828 (landboreformer) og 1830 (veje).

### 2026-10-08 – Backlog opdateret
Backlog.md har fået et afsnit med det, der er bygget i oktober (scenarier, delt forskning, kamporden, slagets nye systemer), og en liste over det, der endnu ikke er afprøvet i et rigtigt slag.

### 2026-10-08 - Audit: rekognoscering og tåge

- Læst Intel1851-designet, rekognoscering/AI, War-gemning, scenarier og kortets fjendemærker. Ingen AGENTS.md fundet i worktree.
- Save/restore bevarer nu meldinger undervejs, synlighed, korps-id'er, hvile, næste overvejelse, ventebesked, bådenes klargøring og flådespærrede mål. Sidste krigsdag gemmes for at undgå gentagne forstærkninger ved indlæsning. Ældre saves accepteres stadig.
- Startstyrken gendannes som gemt: forstærkede korps får ikke længere et højere forstærkningsloft efter indlæsning.
- I 1825 afrundes observationer til 10 mand og bymeldinger til 100; 1851 beholder 100/1.000. Synsvidder, usikkerhed, leveringstider og forstærkningstal er uændrede.
- Kortets forskydning af fjendemærker bruger kun kendte positioner. Intel-funktioner og konstanter har filspecifikke navne af hensyn til unity builds.
- Kontrol: statisk gennemgang af save/restore og `git diff --check`; ingen build eller kørsel.
- Uafklaret: historisk 1825-korpskvalitet (nålegevær-bonus), absolutte forstærkningstal og befolkningsvægt kræver balancering. Første by vælges som melder frem for hurtigste; generel War-gemning flytter marcherende korps til vejstrækningens slutby. Begge forhold er bevaret af hensyn til 1851. Nye korps annonceres fortsat med præcis styrke ved krigsudbrud; om denne efterretning skal skjules, kræver afklaring.

### Næste skridt
- Åbne `Strategy1864`-slaget fra kampagnen med terræn fra `Battlefield_N.json` og enheder fra `Units.json`. Typerne til det findes allerede i `StrategyBattlefieldGenerationTypes.h`.
- Skrive `BattleResult_N.json` med tab pr. kompagni og officerernes skæbne og vende tilbage til kampagnen.
- Rette morterfejlen i testens tjekliste.

