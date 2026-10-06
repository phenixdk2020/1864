# PROJECT 1864 — Unreal Engine migration plan

**Migration branch:** `unreal-port`  
**Reference branch:** `channel-test`  
**Unity reference baseline:** `v00.00.09f30x` plus subsequent compile fixes and imported infantry rig  
**Unreal Engine baseline:** `5.8.3`  
**Migration principle:** preserve gameplay/design behaviour first; replace engine implementation second.  
**U00 status:** PASS — UE 5.8.3 C++ editor build verified on Windows.

## Goal

Port PROJECT 1864 from the existing Unity tactical prototype to Unreal Engine without discarding the design, historical data, gameplay rules, AI behaviour, UI semantics, formation doctrine, OOB structure, or reusable art assets.

The Unity implementation remains the behavioural reference until the Unreal implementation passes equivalent QA. The Unreal port must not silently simplify or reinterpret established rules.

The detailed feature-for-feature tracker is [UNREAL-PARITY-BACKLOG.md](UNREAL-PARITY-BACKLOG.md). The migration plan defines the phase order; the parity backlog defines the individual required features and QA exits.

## Repository strategy

The existing repository remains authoritative during the port.

- `main`: accepted/stable work.
- `channel-test`: Unity tactical reference.
- `campaign`: Unity campaign work.
- `unreal-port`: Unreal Engine port.

The Unreal project lives under `/Unreal` so the Unity reference remains available in the same branch for direct code/design comparison during migration.

## Migration order

### U00 — Project/bootstrap — PASS
- Create Unreal C++ project scaffold.
- Establish module, source layout and ignore rules.
- Establish source-asset handoff rules.
- Preserve the Unity files as reference.

### QA fixture and deterministic regression — CORE IMPLEMENTED / BUILD+QA PENDING
- Runtime QA fixture creates the Danish hierarchy, optional Danish cavalry/Dragoon, Prussian targets and river/bridge test barrier.
- Stable IDs and organic/current command backlinks are validated on every build.
- Command cycles are rejected at the command-component boundary.
- Combat RNG uses deterministic per-unit seeds derived from a configurable QA seed.
- Each spawn runs a regression checklist and logs PASS/FAIL with individual failures.
- F5 deterministically rebuilds the QA fixture and resets battle outcome.
- Scenario state exposes InProgress, DenmarkVictory, OppositionVictory and Draw.
- A dedicated .umap QA level and recorded UE 5.8.3 runtime results are still pending.

### U01 — Camera and selection — IMPLEMENTED / BUILD+QA PENDING
- RTS camera Pawn with WASD pan, Q/E yaw and mouse-wheel zoom.
- Left-click unit selection through visibility trace.
- Drag box selection uses each unit actor's projected centre, matching the established Unity selection rule.
- Shift adds to the current selection.
- Ctrl removes units from the current selection.
- Empty plain click clears selection; additive/removal modifiers do not clear unrelated units.
- Selection state lives on `AStrategyUnit`; Blueprint receives `OnSelectionChanged` for rings/highlights.
- Order placement is intentionally separate from selection input, preserving selected units through future order placement.
- Default GameMode now wires the RTS camera Pawn, player controller and selection HUD.
- U01 is not marked PASS until the UE 5.8.3 editor target builds and the interaction QA below is completed.

#### U01 QA gate
1. UE 5.8.3 editor target builds without C++/UHT errors.
2. WASD pans over the tactical level; Q/E rotates; mouse wheel zooms within configured limits.
3. Plain click selects one player-controllable `AStrategyUnit`.
4. Drag selection selects every player-controllable unit whose actor centre lies inside the rectangle.
5. Shift-click/drag adds units without dropping the existing selection.
6. Ctrl-click/drag removes only targeted units.
7. Plain click on empty ground clears selection.
8. Future right-click/order placement must not clear selection.

### U02 — Unit/OOB data model — SCAFFOLD IMPLEMENTED / BUILD+QA PENDING
- Base `AStrategyUnit` now owns stable unit ID, display name, side, echelon, initial/current strength, unit state, AI state, order component and command component.
- `AStrategyCompanyUnit` provides the company entity baseline.
- `AStrategyHQUnit` provides physical Battalion/Major, Regiment, Brigade and Division HQ actors.
- HQ defaults preserve the documented command-zone radii for each echelon.
- `UStrategyCommandComponent` separates `OrganicParent` from `CurrentCommandParent`.
- Organic/current subordinate lists update when command parent changes.
- `RestoreOrganicCommandParent()` is available for later temporary cavalry task attachment/release.
- Runtime OOB test assembly is now implemented and auto-spawnable from `AStrategyGameMode`.
- Current U02 test hierarchy: 1 Division → 1 Brigade → 1 Regiment → Major A/B → 4 companies each.
- Placeholder units have visibility-trace selection colliders and debug labels so hierarchy and U01 selection can be tested together.
- U02 remains BUILD+QA PENDING until UE 5.8.3 confirms the spawn counts, parent/subordinate links and selection behaviour.

### U03 — Orders and authority — CORE STATE IMPLEMENTED / EXECUTORS PENDING
- Order type set includes MOVE, ANGRIB HER, FORSVAR HER, HOLD, RYK FREM, TILBAGETRÆK, SAML, SPEJD HER and CHARGE.
- Every order carries explicit authority: inherited AI, Officer AI or Direct Player.
- Lower authority cannot replace a higher-authority current order.
- Standing intent and physical execution are separate states.
- Execution lifecycle supports Idle, PendingTarget, Pending, Executing, Completed, Failed and Superseded.
- New accepted orders receive a serial and can supersede an executing order cleanly.
- Shared physical movement executor is now implemented for MOVE and as the physical core for ANGRIB HER/FORSVAR HER/RYK FREM/TILBAGETRÆK/SAML/SPEJD HER.
- HOLD immediately stops movement and completes physical execution while remaining standing intent.
- PlayerController can issue DirectPlayer orders to the current selection without clearing it.
- Pending-order placement can commit a ground target under the cursor while preserving selection.
- Arrival tolerance is currently 50 cm, aligned with the Unity F30V final-slot authority scale.
- ANGRIB HER/FORSVAR HER formation/subordinate planning and click-drag facing are implemented at core level.
- RYK FREM, TILBAGETRÆK and SAML now use the same recursive hierarchy/slot pipeline through Division → Brigade → Regiment → Major → Company.
- Coordinated parent HQs retain mission ownership while subordinate formations execute; HQ follow owns rear positioning rather than movement to the objective.
- U03 remains build/runtime QA pending rather than parity verified.

### U04 — Infantry movement/formations — GEOMETRY/MAJOR PLANNER IMPLEMENTED, RUNTIME QA PENDING
- `UStrategyFormationComponent` now owns unit formation state and reusable slot generation.
- Three-rank infantry Line geometry is implemented as the default company line model.
- March Column geometry is implemented with a 4-wide baseline.
- `UStrategyParentFormationPlannerComponent` generates deterministic company slots under Battalion/Major command.
- Major ANGRIB HER/FORSVAR HER missions decompose into subordinate company missions.
- Parent committed facing is applied before company slots are generated.
- Current Unity F30X company spacing baseline is preserved: 72 m nominal / 68 m reserved minimum.
- Parent order authority is preserved in generated child missions.
- F30X early deployment is now implemented at formation-state level using max(own/enemy maximum fire range)+35 m.
- Long movement can select March Column automatically and switch back to Line at the deployment threshold.
- Attack/Defend planning now cascades Division → Brigade → Regiment → Battalion/Major → Company with committed facing preserved.
- Parent mission completion waits on immediate subordinate execution, allowing completion to cascade back up the hierarchy.
- Formation changes now own an explicit Reforming transition. Movement pauses and resumes the same route/order rather than replacing the mission.
- Final slot completion waits for physical committed-facing rotation within a 2° tolerance.
- Full-strength 190-man Line frontage is regression-checked against the current ~48 m baseline.
- Moving friendly companies have bounded local sidestep/deconfliction around the 68 m reserved centre spacing.
- Physical 1:1 soldier-slot interpolation remains pending; the current reform implementation is authoritative transition/state core.

### U05 — Terrain/navigation — NAVMESH CORE IMPLEMENTED / RIVER-BRIDGE RULES PENDING
- Formation-level route planner now requests Unreal NavMesh paths and returns ordered waypoints.
- Movement executor follows route waypoints while preserving one authoritative final goal.
- The 50 cm final-arrival rule applies only to the real mission destination; intermediate waypoint tolerance cannot complete the mission.
- NavMesh provides the initial obstacle/building-avoidance foundation.
- River barrier model now classifies bank side and detects true bank changes.
- Opposite-bank routes receive an explicit bridge transaction with approach/exit metadata.
- Same-bank chord-through-water missions create a same-bank detour instead of a bridge round-trip.
- Cavalry defile mode activates only near the bridge (~36 m) or during the bridge transaction and restores its previous formation after exit.
- Hard-water target authority is now implemented through route validity: start/destination inside river water fails the movement order rather than falling back to a direct water crossing.
- Goal changes during an active bridge crossing preserve the current bridge transaction through exit and retarget the post-crossing tail.
- Final map collision/nav-area authoring and multi-unit bridge congestion/queueing still require runtime/map integration.

### U06 — Combat/visibility — ELIGIBILITY CORE IMPLEMENTED / FIRE EXECUTION PENDING
- Shared fire-control state implements HOLD, CLOSE, MEDIUM and LONG policies.
- Shared visibility component performs line-of-sight traces.
- Fire eligibility requires enemy side, combat-effective state, active range, ±35° cone and actual LOS.
- QA Close/Medium/Long cone drawing is separate from fire authority.
- Prussian QA cones can be visible from battle startup without granting target knowledge or firing permission.
- Runtime QA scenario now includes optional Prussian company targets.
- Volley/reload/ammunition and authoritative strength-loss execution are now implemented at core level.
- Zero-hit volleys are valid and do not cause casualties.
- Morale and Cohesion are separate states; incoming volleys apply shock even when hits are zero.
- Under-fire can temporarily pause movement without completing the mission, then movement resumes.
- Square target eligibility uses four 90-degree fire sectors.
- Real volley events now expose presentation-only origin/direction/shots/hits data; no formation transition can generate a volley event.
- Representative casualty events are emitted only from positive authoritative strength loss.
- Reforming units cannot fire, closing the F29X phantom-volley path.
- Square owns its QA outline and Square volley presentation data originates from the selected firing face.
- Niagara asset binding, detailed casualty categories and final visual effects remain pending.

### U07 — Officer AI — DELEGATION/HQ CORE IMPLEMENTED / RUNTIME QA PENDING
- One shared Officer AI component is attached to every strategy unit and uses the same order-authority model for player-delegated and enemy units.
- AI ON/OFF can cascade recursively through the live command hierarchy.
- Inherited non-Attack/Defend missions are accepted only when no protected DirectPlayer authority is active.
- Finite higher-authority orders stop blocking lower delegated AI after physical completion; standing HOLD/FORSVAR remains protected.
- HQ follow is implemented as background movement derived from subordinate centroid and committed mission facing, not as a competing order/HqGoal writer.
- Current follow baselines preserve the F30P direction: Battalion 65 m rear; Regiment 90 m rear; Brigade 120 m rear +65 m lateral; Division 145 m rear -75 m lateral.
- Direct player MOVE owns HQ position while executing and suppresses follow.
- Selected HQs draw the documented inner/outer command zones for QA.
- Parent mission completion already waits on live subordinate execution.
- Tactical decision quality, officer personality/stats and command-delay gameplay effects remain later work.

### U08 — Cavalry — FORMATION/BRIDGE/TASKING CORE IMPLEMENTED / CHARGE+DRAGOON WORK PENDING
- Cavalry base actor, separate horse/rider skeletal components and formation state are present.
- 4-rank Line/Charge geometry, 4-abreast normal Column and 2-abreast Defile geometry are implemented at core level.
- Bridge route metadata activates Defile only near the approach/inside crossing and restores the pre-defile formation afterwards.
- Higher ANGRIB HER can temporarily task-attach available cavalry to Major A/B through CurrentCommandParent without changing OrganicParent.
- Two-cavalry/two-Major allocation compares direct vs crossed total travel cost.
- Executing DirectPlayer cavalry orders are protected from automatic retasking.
- Attack completion restores the previous command parent and issues a background reserve move.
- FORSVAR HER assigns cavalry reserve positions behind/lateral to paired Majors while retaining higher ownership.
- Runtime QA scenario now includes two Danish cavalry under Division.
- Charge is now a finite mounted movement mission with swept enemy contact detection; valid contact stops the cavalry before it can pass through the enemy.
- Dragoon role now has a 75/25 combat-group/horse-holder data split, persistent horse-park anchor, dismounted movement/fire limit and STIG OP return/remount core.
- Physical 1:1 rider reform, detailed melee resolution and SPEJD HER remain pending.

### U09 — OOB/HUD/semantic zoom — QA/PRESENTATION CORE IMPLEMENTED / FINAL UMG PENDING
- HUD now shows a visible Unreal-port build marker and selected-unit QA state.
- Recursive OOB aggregate data exposes strength/status/execution for future UMG rows.
- OOB selection backend supports select and select+camera-focus.
- Command visual state is blue only during physical execution; completed standing intent returns red. Green remains reserved for toggles/state controls.
- Semantic zoom now exposes Close/Medium/Operational/Strategic/VeryFar states.
- Camera zoom range reaches the documented tactical semantic-zoom distances.
- World labels expose NATO echelon markers I/II/III/X/XX and CAV.
- Strategic/VeryFar zoom suppresses tactical meshes without suppressing simulation/colliders.
- Selected command entities expose command-tree lines, mission targets/facing and subordinate routes for QA.
- Existing selected-HQ command circles and range/cone language remain active.
- A shared presentation snapshot now exposes the complete OOB/hover data contract: identity/NATO echelon, strength/losses, morale/cohesion/fatigue, unit state, AI, order/execution, formation, organic/current parent, attachment flag, ammo and cavalry/Dragoon mounted state.
- Final UMG OOB tree, per-echelon production HUD layout, attachment drag/drop and visual hover widgets remain pending.

### Tactical time controls — CORE IMPLEMENTED / QA PENDING
- Space toggles tactical pause.
- 1/2/3 select 1x/2x/3x global time dilation and leave pause.
- Movement/combat/AI timers remain simulation-time driven and therefore scale together.
- Victory/defeat outcome, deterministic QA seed and F5 restart/reset are now implemented at core level.
- A dedicated scenario UI/result screen remains pending.

### U10 — Tactical parity gate
The Unreal battle test is not considered a replacement for the Unity prototype until the current tactical QA baseline is reproduced.

Primary parity checks:
1. Enemy QA cones visible from battle start.
2. Infantry deploys before entering enemy maximum fire range.
3. Company lines do not overlap at final slots.
4. Cavalry uses two-abreast only near bridge/defile.
5. Same-bank river movement avoids unnecessary bridge round-trips.
6. Attack/defend buttons show active execution only while execution remains.
7. Parent order completion waits for actual final-slot arrival.
8. OOB AI status is consistent.
9. Higher-level selection persists through order placement.
10. Cavalry / Square / fire reactions respect LOS, range and cone.

## Asset policy

### Directly reusable
- FBX meshes.
- Rigged Mixamo infantry.
- Textures.
- Audio.
- Historical/OOB data.
- Heightmaps and reference maps where format permits.

### Rebuilt in Unreal
- C# MonoBehaviour systems → Unreal C++ / Blueprint.
- Unity Prefabs → Unreal Blueprints.
- Unity Animator state machines → Animation Blueprints / Montages.
- Unity ParticleSystem → Niagara.
- Unity UI/IMGUI → UMG/Common UI as appropriate.
- Unity scene-specific wiring → Unreal Levels/Actors/Data Assets.

## Animation policy

### Infantry/rider
Use humanoid skeletal meshes and Unreal retargeting/IK tooling. The current Mixamo-rigged infantry FBX is retained as a source asset.

### Horse
Horse is a separate quadruped skeletal mesh and is not treated as Humanoid. Horse animation uses its own Skeleton/Animation Blueprint. Rider and horse remain separate rigs, connected through a saddle/rider socket and synchronized animation state.

## Architecture direction

Initial Unreal module layout:

```text
Unreal/
├── Config/
├── Content/
├── Source/
│   ├── Strategy1864.Target.cs
│   ├── Strategy1864Editor.Target.cs
│   └── Strategy1864/
│       ├── AI/
│       ├── Combat/
│       ├── Command/
│       ├── Formations/
│       ├── Orders/
│       ├── Units/
│       └── UI/
└── Strategy1864.uproject
```

The port should prefer stable domain names rather than carrying Unity prototype revision suffixes such as `09F30...` into permanent Unreal class names.


## v00.02.57 functionality-first checkpoint

The project is intentionally postponing visual/asset work while simulation functionality is completed.

- Attack/Defend now own fixed mission anchors; Defend leaf executors can physically reassert the same standing mission after temporary disruption.
- Navigation has explicit building/fence/fieldwork obstacle data, deterministic detours, slope rejection and elevation-following movement.
- Officer profiles and command range now affect actual order-delivery delay. A delayed child order remains part of parent execution authority.
- Fatigue and experience now influence movement, accuracy and morale shock.
- Cavalry base gameplay is self-contained; charge is blocked during reform and optional screen AI repositions without autonomous charge.
- Contact memory separates current visibility from last-known information. Firing requires current contact + LOS/range/cone.
- SPEJD HER is now an operational recon state machine rather than a MOVE alias.
- OOB tactical attachment backend can change CurrentCommandParent while preserving OrganicParent.
- Bridge barriers serialize crossing ownership; waiting units retain their mission.
- Rootless opposition companies can autonomously close to a finite engagement distance while respecting higher command authority.
- Routed units perform fallback and can rally only after reaching safer separation and recovery thresholds.
- Ammunition exhaustion/resupply is authoritative.
- QA now includes a navigation-obstacle fixture and checks the new gameplay component set.

Remaining emphasis before parity verification is local UE 5.8.3 compilation/runtime QA, followed by defect correction. Visual assets, Niagara/animation and final UMG remain later passes by design.


## v00.02.58 combat, fieldcraft and decision-layer checkpoint

The functionality-first phase now includes:
- separate fire range policy and fire discipline, including ammunition conservation;
- Standing/Prone gameplay with movement/reload/target-profile effects;
- directional cover from buildings, fences and fieldworks;
- hasty fieldworks construction;
- black-powder smoke as LOS/accuracy simulation rather than a visual-only effect;
- abstract skirmisher detachment with five roles, density effects and recall/reform;
- finite tactical ammunition supply and automatic resupply requests;
- Defensive/Balanced/Offensive doctrine plus commander aggression intent;
- Strict/Normal/Independent autonomy;
- Easy/Normal/Hard enemy AI difficulty that changes reaction/noise only and never combat values or knowledge;
- AI-DIAG reason/task telemetry;
- expanded officer profile feeding command, stress and rally behaviour;
- mission constraints for no-pursuit, ammunition conservation and bounded operating area.

Runtime keys now include I for selected-unit Officer AI and F6/F7/F8 for enemy AI difficulty. Graphics remain explicitly outside the current gate.

### Next functional priority — artillery battery

Artillery should enter before the visual pass as a real tactical formation, not a decorative mesh. The next artillery slice should provide an artillery battery actor with individual gun count/crew/ammunition state, deployed vs limbered state, facing/traverse limits, range/fire arc, fire mission execution, movement restrictions while deployed, and later horse-team/limber representation. Artillery must use the same command hierarchy, order authority, LOS/contact and ammunition/supply rules as infantry/cavalry where applicable.


### U11 — Artillery battery — FUNCTIONAL CORE IMPLEMENTED / UE QA PENDING

- New Artillery echelon and `AStrategyArtilleryBatteryUnit`.
- Battery state contains configurable gun profile/count, crew, drivers and horses.
- Limber/unlimber are timed transitions; normal movement requires Limbered state and adequate horse/driver mobility.
- Deployed batteries can manhandle only over a short configured distance at low speed.
- Separate RoundShot/Shell/Shrapnel/Canister ammunition inventory.
- Player artillery defaults to manual target; AUTO TARGET is explicit opt-in; HOLD FIRE overrides either mode.
- Deployed battery traverse is time-based. Fire requires target in traverse, current contact, LOS and ammo-specific range.
- Volley uses operational gun count; reload responds to experience/fatigue/crew availability.
- Prototype ammunition types have distinct range/effect curves pending historical tuning.
- Black-powder artillery fire creates the same simulation smoke layer used by LOS.
- Tactical supply can replenish artillery ammunition.
- Infantry fire and counter-battery hits can damage crew, horses and guns.
- Guns can become disabled, destroyed or abandoned; abandoned batteries can be physically captured.
- In-battle captured-gun reuse requires compatible ammunition, qualified crew input and preparation time.
- Artillery is deliberately excluded from infantry formation slot planning.
- QA OOB now contains one Danish six-gun test battery under Division; six guns are a fixture, not a locked historical battery organisation.
- Final terrain/hill artillery runtime QA remains dependent on the expanded battlefield/hills gate.


## v00.02.60 artillery/logistics refinement checkpoint

Artillery now supports visible ground/area missions, salvo/time mission limits, target-priority doctrine, automatic ammunition selection and reserve conservation. Deployment samples terrain slope; firing/manhandling add workload fatigue; disabled guns may be field-repaired; emergency abandonment leaves guns for capture.

A physical `AStrategySupplyWagonUnit` now provides the first tactical logistics entity. It carries separate small-arms/artillery ammunition, has drivers/horses/wagon condition, obeys mobility damage, may be abandoned/captured and supplies only nearby compatible receivers while both sides are stationary and not under fire. Artillery compatibility uses the gun/cargo ammunition-family tag.

The QA OOB contains a deliberately low-ammo Danish artillery battery and a nearby Danish ammunition wagon. When this fixture is active, Division's former generic 50k ammunition source is disabled, forcing the tactical supply chain to be testable.

Remaining gates: local UE 5.8.3 compile/runtime QA, expanded hills/dead-ground battlefield validation, historical artillery profile tuning, and later battlefield-salvage/aftermath logistics.


## v00.02.61 tactical terrain and artillery positioning checkpoint

The functionality-first Unreal port now has a shared tactical terrain authority. Gameplay may combine physical WorldStatic ground with analytic Hill/Ridge/Depression features, allowing hills and dead ground to be tested before final visual terrain assets.

Implemented:
- effective ground elevation and local slope queries;
- crest and dead-ground detection;
- unit terrain-awareness state for high ground/reverse slope/crest exposure;
- terrain-aware unit LOS and arbitrary ground-location LOS;
- high-ground observation modifiers without LOS bypass;
- crest/reverse-slope masking in infantry and artillery fire resolution;
- route waypoint terrain projection and long-segment sampling without NavMesh;
- artillery deploy slope using the same terrain model;
- artillery candidate generation/evaluation and best direct-fire position search;
- QA terrain with a battery hill, central ridge and depression;
- QA actor spawn projection to tactical terrain Z;
- regression checks for expected dead ground, clear lane, crest and valid artillery position.

This is gameplay terrain authority, not the final landscape-art pass. UE 5.8.3 compile/runtime validation remains required before parity verification.


## v00.02.62 projectile/follow-camera checkpoint

Artillery now has a non-authoritative projectile presentation layer driven by the exact per-gun hit/miss impact points resolved by the fire mission. Round Shot, Shell, Shrapnel and Canister receive different presentation paths; Round Shot can visibly ricochet, Shell/Shrapnel have temporary impact/burst debug feedback, and Canister renders a short cone rather than one followable ball.

Each battery owns a projectile presentation component with virtual per-gun muzzle origins, active-projectile tracking, bounded impact history and trajectory QA. P follows the latest live projectile. The camera holds briefly at impact and restores the previous battle view; manual movement/rotation/zoom cancels follow. F9 toggles full trajectory lines.

A future campaign→battlefield generation contract is also established in `Campaign/StrategyBattlefieldGenerationTypes.h`. It intentionally contains data only: battle id/center/size/seed, attacker and defender approach directions, plus source features for topography, waterways, transport, settlements and land cover. No battlefield generator is implemented yet.

Remaining gate: local UE 5.8.3 compile/runtime QA and later replacement of debug projectile visuals with final cannonball/shell meshes, particles, smoke and audio.


## v00.02.63 shared animation/uniform checkpoint

The visual architecture is now prepared for many soldier types without duplicating animation work. Strategy units carry a shared human visual profile targeting `SK_Human_1864` and `ABP_Human_1864`, reusable animation intent state, equipment socket mapping, uniform appearance data and rig compatibility validation.

Uniform appearance supports independent Coat/Trousers/Facings/Headgear/Leather/Accent/Metal material parameters plus editable presets and runtime overrides. Cavalry owns horse-gait state and rider synchronization; Dragoon mount/dismount state emits the shared human Mount/Dismount intent.

Artillery now has visual crew roles/stations and a drill controller driven by the existing authoritative battery state: manhandling, traverse, deploy/limber, repair and post-shot reload phases. No animation controls gameplay state.

QA includes reusable animation manifests, editable side/artillery palettes, runtime colour override validation and shared snapshot diagnostics. Final SkeletalMesh/AnimationBlueprint/Montage/material assets remain a later graphics task.
