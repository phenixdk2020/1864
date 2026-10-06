# PROJECT 1864 — Unreal porting inventory

This inventory tracks what exists in the Unity tactical prototype and the intended Unreal destination.

| Unity/reference area | Unreal destination | Initial status |
|---|---|---|
| Unreal C++ project/bootstrap | UE 5.8.3 editor target/module | **Build verified** |
| Unity tactical selection/input | `AStrategyPlayerController` + `AStrategyHUD` | **Implemented; build/QA pending** |
| Unity RTS camera | `AStrategyCameraPawn` | **Implemented; build/QA pending** |
| `Regiment.cs` | `AStrategyUnit` + infantry-specific actor/components | **Unit state implemented; infantry behaviour pending** |
| Regiment hierarchy F27 | OOB/command hierarchy data + components | **Command relationship scaffold implemented** |
| Major HQ | Physical HQ Pawn/Actor | **HQ actor scaffold implemented** |
| Regimental HQ | Physical HQ Pawn/Actor | **HQ actor scaffold implemented** |
| Brigade/Division HQ F30B | Higher HQ actors + command components | **HQ actor scaffold implemented** |
| Manual order authority F18 | `UOrderComponent` authority model | **DirectPlayer/OfficerAI/InheritedAI core implemented** |
| Attack/Forsvar/Hold | `EStrategyOrderType` + order executors | **Physical executor core implemented; formation planning pending** |
| March Column F6 | `UFormationComponent` / formation executor | **Formation geometry core implemented** |
| Line/Square | Formation component + animation/slot generation | **Line slot core implemented; Square pending** |
| Company slot deconfliction | Formation planner | **72m/68m Major company-slot baseline implemented** |
| Final-slot arrival F30V | Shared movement completion authority | **50 cm final-arrival core implemented** |
| Navigation V3 | Navigation/path execution service | **NavMesh waypoint route core implemented** |
| River/bridge routing | Route planner + bridge transaction | Planned |
| Fire ranges/cones | Combat/visibility components + debug draw | **Range/cone + QA drawing core implemented** |
| LOS/contact | Visibility/contact subsystem | **LOS eligibility core implemented; contact/FOW pending** |
| Volley/reload | Combat state machine | Planned |
| Casualties/morale | Unit combat state/data | Planned |
| Under-fire reaction F26 | AI/reaction component | Planned |
| Infantry Square F29 | Formation + anti-cavalry reaction | Planned |
| Cavalry core F30 | `ACavalryUnit` | Scaffold builds |
| Cavalry AI F30C+ | Cavalry AIController/behaviour logic | Planned |
| Dynamic task attachment | Command-parent state/data | **Temporary cavalry attack attachment core implemented** |
| Dragon mounted/dismounted | Cavalry specialization | Planned |
| OOB F29Q/F30D | UMG OOB widget | Planned |
| Semantic zoom/NATO | Tactical presentation subsystem | Planned |
| HQ command zones | Debug/gameplay visualization component | Planned |
| Unity infantry FBX | Unreal Skeletal Mesh import | Source asset present |
| Horse + rider | Separate horse/rider Skeletal Meshes | Planned |
| Unity design manual | Engine-neutral source of truth | Preserved |

## Rule

A system is not marked **Ported** merely because an Unreal class exists. It becomes **Parity verified** only when its documented Unity behaviour has passed the corresponding Unreal QA scenario.


## v00.02.57 gameplay inventory additions

- Mission anchors: finite Attack + persistent leaf Defend reassert.
- Navigation obstacles: Building/Fence/Fieldworks/Generic detour actor.
- Terrain: slope validation, uphill/downhill speed effect and waypoint-Z following.
- Officer profile + command-delay delivery.
- Fatigue/experience condition model.
- Contact memory + current-contact fire authority.
- Recon: SPEJD HER Seek/Recon/Contact/Screen/Report/LastKnown.
- Cavalry: finalized base defaults, reform charge gate, non-charging screen AI.
- OOB attachment backend: CurrentCommandParent tasking with OrganicParent preservation.
- Bridge occupancy/queue authority.
- Autonomous opposition company AI.
- Routed fallback + rally recovery.
- Ammunition exhaustion/resupply.
- Expanded gameplay regression fixture.


## v00.02.58 combat and AI additions

- Fire discipline: HoldFire / FireAtWill / Volley / Independent.
- Ammunition conservation.
- Standing/Prone stance effects.
- Directional building/fence/fieldwork cover.
- Hasty fieldworks construction.
- Black-powder smoke LOS/accuracy simulation.
- Skirmisher detachment, five roles, combat density and recall/reform.
- Tactical ammunition supply sources and resupply requests.
- Doctrine + commander aggression.
- Strict/Normal/Independent autonomy.
- No-cheat Easy/Normal/Hard AI decision layer.
- AI-DIAG task/reason telemetry.
- Expanded officer stats feeding command/stress/rally.
- Mission constraints.

### Next inventory target: artillery battery

Planned functional artillery core:
- battery tactical unit;
- gun count and crew state;
- gun calibre/type data;
- deployed / limbered state;
- facing and traverse arc;
- artillery range/fire mission;
- ammunition state and resupply;
- movement only when limbered;
- same command/OOB authority model;
- later limber horses, gun meshes, crews and firing VFX.


## v00.02.59 artillery functional inventory

- Artillery echelon + battery actor.
- Configurable gun profile/count, crew, drivers, horses.
- Timed Limbered/Deploying/Deployed/Limbering/Manhandling state.
- Horse/driver-limited towing and short crew manhandling.
- Round shot, shell, shrapnel and canister stores.
- Manual target default, optional auto target and Hold Fire override.
- Traverse, min/max range, current-contact and LOS eligibility.
- Operational-gun fire resolution and reload state.
- Ammo-specific prototype range/effect curves.
- Artillery black-powder smoke feeds existing LOS simulation.
- Tactical artillery resupply from shared supply source.
- Crew/horse/gun-specific incoming damage.
- Disabled vs abandoned materiel state.
- Physical capture and controlled captured-gun reuse.
- OOB/snapshot/outcome/regression integration.
- QA fixture: one Danish six-gun battery under Division.


## v00.02.60 artillery/logistics additions

- Visible ground/area artillery fire missions.
- Manual mission salvo/time limits.
- Artillery target-priority doctrine and automatic ammo selection.
- Ammunition reserve/conservation policy.
- Terrain-slope deployment legality.
- Firing/manhandling fatigue.
- Disabled-gun field repair.
- Emergency battery abandonment.
- Supply tactical echelon and horse-drawn supply wagon.
- Split small-arms/artillery cargo inventory.
- Driver/horse/wagon-condition mobility.
- Proximity and state-gated resupply.
- Supply wagon damage, abandonment and capture.
- Artillery ammunition-family compatibility.
- Shared OOB snapshot fields for supply cargo/mobility.
- QA chain: low-ammo Danish battery + physical Danish ammo wagon.


## v00.02.61 tactical terrain additions

- Analytic tactical Hill/Ridge/Depression feature actors.
- Combined physical + tactical effective ground elevation.
- Shared local slope query.
- Sampled terrain-profile LOS occlusion.
- Crest and dead-ground detection.
- Unit high-ground/reverse-slope/crest awareness.
- Terrain-aware unit and ground-location LOS.
- Elevation-sensitive contact awareness.
- Crest/reverse-slope fire masking.
- Route terrain projection and no-NavMesh terrain sampling.
- Shared artillery deployment slope.
- Artillery position evaluation, candidate generation and best-position search.
- QA battery hill, central ridge and depression.
- QA units projected to tactical terrain.
- Shared terrain diagnostics in presentation snapshots.


## v00.02.62 projectile presentation additions

- Per-gun resolved artillery impact records.
- Ammo-specific projectile trajectory solver.
- Tactical-terrain-aware paths.
- Round Shot presentation ricochet.
- In-flight projectile presentation actors.
- Optional full trajectory debug lines.
- Canister cone, Shell impact and Shrapnel burst placeholders.
- Per-battery salvo manager, virtual muzzle origins and bounded history.
- Follow Shot camera + impact hold + view restore.
- P follow-shot and F9 trajectory QA inputs.
- Projectile activity/last impact in shared snapshot.
- Campaign→generated-battlefield data contract (future generator input only).


## v00.02.63 shared visual architecture additions

- Shared human skeleton/animation-set profile IDs.
- Reusable human animation action/locomotion state.
- Canonical animation manifests for human, mounted, horse and artillery crew.
- Uniform colour zones, presets and runtime overrides.
- Dynamic material parameter application.
- Equipment socket mapping.
- Shared rig/animation-set compatibility validation.
- Horse gait state and rider/horse animation synchronization.
- Dragoon Mount/Dismount animation intents.
- Artillery crew roles/stations.
- Artillery reload drill phase mapping.
- Push/traverse/limber/unlimber/repair crew work intents.
- Visual state exposed through OOB/presentation snapshot.
- QA validation for rig, manifests, colour overrides, cavalry sync and artillery crew.
