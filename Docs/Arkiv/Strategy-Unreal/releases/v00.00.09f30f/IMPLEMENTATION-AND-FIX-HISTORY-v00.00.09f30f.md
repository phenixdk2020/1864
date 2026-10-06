# PROJECT 1864 — Implementation & Fix History

**Konsolideret ved v00.00.09f30f TEST**  
**Gameplay-baseline: v00.00.09f30e**  
**Unity-baseline: 6000.6.0f1**

Dette dokument er den samlede, kronologiske registrering af de funktioner og kendte fejlrettelser/hardening-trin, der er implementeret i den taktiske prototype frem til og med F30E. F30F ændrer ikke gameplay; den konsoliderer historikken.

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

## 7. Samlet fejlrettelsesregister

De vigtigste kendte fejl, som har fået en konkret implementeret rettelse/hardening i lineage frem til F30E:

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

## 8. Status ved F30F

F30F er en dokumentations-/baselineversion. Runtime gameplay er fortsat F30E, bortset fra den synlige buildmarker der bumpes til F30F.

Aktuel implementeret tactical scope:
- 1:1 infantry og cavalry tactical visuals.
- 8-company Danish regiment med 2 Majors + Oberstløjtnant.
- Brigade- og Division-HQ.
- Dynamic support attachment til Division/Brigade/Regiment/Major.
- OOB scroll + drag/drop.
- Manual override på alle implementerede control-lag.
- Infantry Line/Column/Square, fire policy, charge/melee, under-fire, withdrawal infrastructure.
- Cavalry Line/Column, mounted move/charge, Dragon dismount/remount, flank/rear Officer AI.
- Semantic zoom/NATO, tactical map, crop concealment og terrain/navigation hardening.

Ikke færdigt endnu:
- Artillery/kanonbatteri.
- Mounted/dismounted cavalry firearms combat.
- Defensive volley / charge momentum / cavalry casualties.
- Persistent cavalry melee/horse casualties.
- Echelon Left/Right.
- Full autonomous Brigade/Division Officer AI.
- Full campaign/OOB expansion til flere regimenter/brigader.
