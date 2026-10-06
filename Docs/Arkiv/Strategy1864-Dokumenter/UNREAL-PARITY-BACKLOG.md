# PROJECT 1864 — Unreal Engine parity backlog

**Backlog baseline:** UE-P v00.01  
**Unreal branch:** `unreal-port`  
**Unity reference branch:** `channel-test`  
**Unity tactical baseline:** `v00.00.09f30x` plus later compile fixes/source assets  
**Unreal baseline:** UE 5.8.3  
**Purpose:** track every required step until the Unreal tactical prototype reaches or exceeds the documented Unity tactical prototype.

> **Hard rule:** Unreal is not considered the replacement tactical implementation until the relevant item is **PARITY VERIFIED**. A class existing or compiling is not parity.

## Status model

| Status | Meaning |
|---|---|
| BACKLOG | Required parity work has not started. |
| SCAFFOLD | Architecture/class exists, but behaviour is incomplete. |
| IMPLEMENTED | Intended behaviour is coded, but build/runtime QA is incomplete. |
| BUILD VERIFIED | C++/UHT/editor build has passed for the item. |
| QA PENDING | Build works; gameplay behaviour still requires scenario QA. |
| PARITY VERIFIED | Unreal reproduces the documented Unity behaviour and passes the parity test. |
| DEFERRED | Approved feature intentionally waits on a named dependency. |

## Priority model

- **P0** — required for the first playable Unreal battle and/or blocks several later systems.
- **P1** — required before Unreal can replace the Unity tactical prototype.
- **P2** — required parity/presentation work that can follow the first playable battle.
- **P3** — future expansion already approved in design, but not needed for the current Unity-parity gate.

---

# 1. Project, build and test framework

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P001 | P0 | UE project/bootstrap | Engine-port baseline | Module/targets/project | PARITY VERIFIED | UE 5.8.3 editor target builds successfully. |
| UE-P002 | P0 | Unreal source layout | Migration plan | Source modules/directories | BUILD VERIFIED | Stable domain folders/classes compile without Unity revision suffixes. |
| UE-P003 | P0 | Tactical QA level | Unity battle test | Unreal test level | RUNTIME FIXTURE IMPLEMENTED / LEVEL ASSET PENDING | Reproducible Danish/Prussian battle test can be launched directly. |
| UE-P004 | P0 | Visible build marker | Unity TEST overlay | Unreal HUD/debug overlay | IMPLEMENTED QA CORE | Running build clearly displays Unreal prototype/version marker. |
| UE-P005 | P1 | Regression checklist | F29/F30 QA | Automated/manual QA suite | IMPLEMENTED CORE | U10 checks can be executed repeatably and results recorded. |
| UE-P006 | P2 | Debug telemetry/logging | Unity prototype logs | UE_LOG + debug subsystem | IMPLEMENTED QA CORE | Orders, movement, AI, contact and completion expose useful reason/state logs. |

# 2. Camera, mouse input and selection — U01

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P010 | P0 | RTS camera pan | Tactical camera | `AStrategyCameraPawn` | IMPLEMENTED | WASD pans reliably over tactical map. |
| UE-P011 | P0 | Camera rotation | Tactical camera | `AStrategyCameraPawn` | IMPLEMENTED | Q/E rotates camera without changing simulation state. |
| UE-P012 | P0 | Camera zoom | Tactical camera | `AStrategyCameraPawn` | IMPLEMENTED | Mouse wheel zooms within configured limits. |
| UE-P013 | P0 | Single-click selection | Selection baseline | `AStrategyPlayerController` | IMPLEMENTED | Plain click selects one player-controllable unit. |
| UE-P014 | P0 | Box selection | RTS box-select rule | PlayerController + HUD | IMPLEMENTED | Units whose actor/formation centre lies inside marquee are selected. |
| UE-P015 | P0 | Shift additive selection | Selection baseline | PlayerController | IMPLEMENTED | Shift adds without clearing existing selection. |
| UE-P016 | P0 | Ctrl removal selection | Selection baseline | PlayerController | IMPLEMENTED | Ctrl removes targeted units only. |
| UE-P017 | P0 | Selection visual hook | Unity selection marker | Unit BP event | IMPLEMENTED | Selected state can drive ring/highlight without owning gameplay state. |
| UE-P018 | P0 | Selection persistence through order | F29X/F30S | Controller/order input | IMPLEMENTED | Issuing point/facing orders never clears selected HQ/formation. |
| UE-P019 | P1 | OOB click selection | F29X | UMG OOB | IMPLEMENTED API CORE | Single-click selects; double-click selects + camera navigation. |
| UE-P020 | P1 | Mixed entity box selection | F30H | Selection subsystem | IMPLEMENTED CORE | Infantry, cavalry and higher HQs are selectable by marquee according to ownership rules. |

# 3. Unit identity, hierarchy and OOB data — U02

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P030 | P0 | Base tactical unit state | `Regiment.cs` family | `AStrategyUnit` | IMPLEMENTED | Stable ID, side, echelon, name, strength, state and selection are authoritative. |
| UE-P031 | P0 | Company entity | Company baseline | Infantry company actor/data | IMPLEMENTED | Company exists independently with current/initial strength and command parent. |
| UE-P032 | P0 | Battalion/Major entity | F27 | HQ actor + hierarchy component | IMPLEMENTED | Major owns correct company set and can receive player/AI mission. |
| UE-P033 | P0 | Regiment entity/HQ | F27/F28 | HQ actor + hierarchy component | IMPLEMENTED | Regiment owns Major A/B and physical regimental HQ. |
| UE-P034 | P0 | Brigade entity/HQ | F30B | Higher HQ actor | IMPLEMENTED | Brigade exists physically and in hierarchy. |
| UE-P035 | P0 | Division entity/HQ | F30B | Higher HQ actor | IMPLEMENTED | Division exists physically and in hierarchy. |
| UE-P036 | P0 | OrganicParent | F30M | Command relationship state | IMPLEMENTED | Permanent OOB ownership survives temporary tasking. |
| UE-P037 | P0 | CurrentCommandParent | F30M | Command relationship state | IMPLEMENTED | Tactical parent can differ from OrganicParent and be restored. |
| UE-P038 | P1 | Command relationship lines | F27/F29D | Presentation component | IMPLEMENTED QA CORE | Selected formations show correct parent/subordinate connections. |
| UE-P039 | P1 | Stable entity IDs | Design B-131 | Data model | IMPLEMENTED VALIDATION CORE | OOB entities persist through save/transfer without duplicate identity. |
| UE-P040 | P1 | OOB aggregate status | F29Q/F30D | UMG OOB model | IMPLEMENTED DATA CORE | Parent rows calculate correct subordinate strength/status. |

# 4. Orders, authority and execution state — U03

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P050 | P0 | Order component/core | F18+ | `UStrategyOrderComponent` | IMPLEMENTED | One authoritative current order + execution state per command entity. |
| UE-P051 | P0 | MOVE | Unity movement baseline | Order executor | IMPLEMENTED | Selected command entity moves to assigned point. |
| UE-P052 | P0 | ANGRIB HER | F18/F29E/F30S | Attack executor | IMPLEMENTED CORE | Finite attack mission reaches assigned attack slots and does not chase forever. |
| UE-P053 | P0 | FORSVAR HER | F29Y/F30U | Defend executor | IMPLEMENTED CORE | Defensive mission owns formation/HQ geometry until replaced. |
| UE-P054 | P0 | HOLD/STOP | Command HUD baseline | Hold executor | IMPLEMENTED | Movement cancels/stops without corrupting standing state. |
| UE-P055 | P0 | Click-drag facing | F30P | Order target/facing data | IMPLEMENTED CORE | Player-committed facing is mission data before slot generation. |
| UE-P056 | P0 | Direct player authority | F18 | Authority model | IMPLEMENTED | Direct order overrides inherited officer AI for its scope. |
| UE-P057 | P0 | Standing intent vs execution | F30S/W | Order state model | IMPLEMENTED CORE + PARENT COMPLETION | Persistent intent and physical execution are independent states. |
| UE-P058 | P0 | Active order colour lifecycle | F30Q/S/W | UMG command state | IMPLEMENTED CORE | Red=no active execution, blue=pending/executing, green=toggle/state. |
| UE-P059 | P0 | Higher order propagation | F27/F30M | Command planner | IMPLEMENTED CORE | Division/Brigade/Regiment/Major correctly generate subordinate missions. |
| UE-P060 | P0 | Re-issue/supersede | F30V/U | Order lifecycle | IMPLEMENTED CORE | New mission cleanly replaces previous destinations and stale executors. |
| UE-P061 | P1 | RYK FREM | F30Q | Order executor | IMPLEMENTED CORE | Movement active only while executors remain. |
| UE-P062 | P1 | TILBAGETRÆK | F30Q | Order executor | IMPLEMENTED CORE | Coordinated withdrawal maintains command ownership. |
| UE-P063 | P1 | SAML | F30Q | Order executor | IMPLEMENTED CORE | Formation assembles at valid slots and completion is physical. |
| UE-P064 | P1 | Mission target visuals | F30M/O | Presentation subsystem | IMPLEMENTED QA CORE | Objective circle/label/ghosts show authoritative mission state only. |

# 5. Infantry formations and movement — U04

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P070 | P0 | Formation component | F6+ | `UFormationComponent` | IMPLEMENTED | Formation owns slot geometry separate from command/order state. |
| UE-P071 | P0 | Three-rank Line | Infantry baseline | Formation generator | IMPLEMENTED CORE | Full company forms documented three-rank line geometry. |
| UE-P072 | P0 | March Column | F6/F30K | Formation generator | IMPLEMENTED CORE | Long movement selects march column without parent tug-of-war. |
| UE-P073 | P0 | Physical reform | Formation baseline | Formation movement | TRANSITION CORE IMPLEMENTED / 1:1 SOLDIER MOTION PENDING | Soldiers/slots move into new geometry rather than teleporting. |
| UE-P074 | P0 | Auto Line before combat | F30X | Formation policy | IMPLEMENTED CORE | Approaching enemy triggers physical Line reform before fire envelope. |
| UE-P075 | P0 | Deploy threshold | F30X | Formation policy | IMPLEMENTED | `max(own range, enemy range)+35m` policy is reproduced/tunable. |
| UE-P076 | P0 | Company nominal spacing | F30X | Parent formation planner | IMPLEMENTED | Full-company centres use ~72 m nominal spacing in current QA baseline. |
| UE-P077 | P0 | Reserved minimum spacing | F30X | Slot deconfliction | IMPLEMENTED BASELINE | ~68 m minimum reservation prevents company overlap at oblique facings. |
| UE-P078 | P0 | Final-slot authority | F30V | Movement completion service | IMPLEMENTED CORE | One 0.50 m company final-slot completion rule owns mission arrival. |
| UE-P079 | P0 | No premature stop | F30V | Movement completion | IMPLEMENTED CORE | Company cannot be marked arrived while still visibly short of slot. |
| UE-P080 | P1 | Formation facing completion | F30S/P | FormationMotion equivalent | IMPLEMENTED CORE | Final visual turn completes without per-frame facing tug-of-war. |
| UE-P081 | P1 | Parent mission reassert | F27/F30X | Mission executor | IMPLEMENTED CORE | Temporary interruption resumes same mission without forcing wrong formation. |
| UE-P082 | P1 | Full 190-man footprint | Infantry baseline | Formation data | IMPLEMENTED QA BASELINE | Formation width/spacing is compatible with ~48 m current full-strength frontage. |
| UE-P083 | P1 | Local sidestep/deconfliction | F29P | Formation movement | IMPLEMENTED CORE | Adjacent formations avoid standing through each other. |

# 6. Terrain, pathfinding, river and bridge routing — U05

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P090 | P0 | Formation-level navigation | Navigation V3 | Navigation service | IMPLEMENTED CORE | Formation path is authoritative; individual soldiers do not independently choose strategic paths. |
| UE-P091 | P0 | Hard water blocking | River baseline | Nav/terrain tags | IMPLEMENTED CORE | Units cannot walk through river except legal crossings. |
| UE-P092 | P0 | True bank-change detection | F30W | River route planner | IMPLEMENTED CORE | Bridge route is required only when destination changes river bank. |
| UE-P093 | P0 | Same-bank bank-follow | F30W | River route planner | IMPLEMENTED CORE | Straight chord crossing a river bend does not cause cross-and-return bridge trip. |
| UE-P094 | P0 | Bridge transaction | F9/F30H | Crossing state machine | IMPLEMENTED CORE | NearBank→FarBank→ExitBank→Direct persists until crossing completes. |
| UE-P095 | P0 | Goal changes during bridge | F30H | Crossing state machine | IMPLEMENTED CORE | New goal updates final destination but does not cancel active bridge transaction. |
| UE-P096 | P1 | Building avoidance | Navigation baseline | Nav obstacles | IMPLEMENTED CORE | Formations route around blocking buildings. |
| UE-P097 | P1 | Fence/obstacle handling | F29P/design | Nav/formation avoidance | IMPLEMENTED CORE | Obstacles affect movement without destroying formation authority. |
| UE-P098 | P1 | Bridge congestion/defile | F30H/X | Formation path policy | IMPLEMENTED CAVALRY CORE | Narrow crossing uses controlled narrow geometry and restores normal formation after exit. |
| UE-P099 | P2 | Expanded battlefield/hills | B-290 | Unreal Landscape | TERRAIN MOVEMENT CORE / LEVEL ASSET PENDING | Tactical QA map includes usable elevation/LOS test terrain. |

# 7. Fire control, LOS, combat and casualties — U06

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P110 | P0 | LOS query | F30S + combat baseline | Visibility subsystem | IMPLEMENTED CORE | Terrain/objects gate target awareness and firing. |
| UE-P111 | P0 | Fire cone | F8/F30S | Combat geometry | IMPLEMENTED CORE | Infantry firing eligibility respects ±35° forward cone where applicable. |
| UE-P112 | P0 | Close range policy | Fire baseline | Fire-control state | IMPLEMENTED CORE | Close policy selects/uses correct range. |
| UE-P113 | P0 | Medium range policy | Fire baseline | Fire-control state | IMPLEMENTED CORE | Medium policy selects/uses correct range. |
| UE-P114 | P0 | Long range policy | Fire baseline | Fire-control state | IMPLEMENTED CORE | Long policy selects/uses correct range. |
| UE-P115 | P0 | HOLD FIRE | Fire baseline/F29X | Fire-control state | IMPLEMENTED CORE | No volley is produced while hold fire owns firing authority. |
| UE-P116 | P0 | Active-range visual language | F30S/T/W | Debug/presentation | IMPLEMENTED QA CORE | Active band strong; other physical bands faint references. |
| UE-P117 | P0 | Enemy TEST cones from startup | F30W/X | QA overlay | IMPLEMENTED QA CORE | Living Prussian infantry cones visible immediately in TEST. |
| UE-P118 | P0 | QA cone != knowledge | F30W/S | Visibility/fire authority | IMPLEMENTED CORE | Debug cone never grants LOS, contact or firing authority. |
| UE-P119 | P0 | Volley/reload cycle | pre-F29 combat | Combat component | IMPLEMENTED CORE | Valid target triggers fire/reload cadence without phantom volleys. |
| UE-P120 | P0 | Hit/casualty resolution | battle baseline | Combat state | IMPLEMENTED CORE | Strength changes only from valid resolved combat. |
| UE-P121 | P1 | 0-hit volley | B-002/current design | Combat state | IMPLEMENTED CORE | Valid volley may resolve zero hits without false strength loss. |
| UE-P122 | P1 | Smoke feedback | F29X/Z | Niagara/combat events | REAL-VOLLEY EVENT CORE / NIAGARA PENDING | Smoke corresponds to real firing event; no fake smoke on formation changes. |
| UE-P123 | P1 | Morale/cohesion | battle baseline | Unit combat state | IMPLEMENTED CORE | Morale/cohesion are separate authoritative values and affect behaviour. |
| UE-P124 | P1 | Routed state | battle baseline | Unit combat state | IMPLEMENTED CORE | Routed unit leaves normal command/fire behaviour and presentation updates. |
| UE-P125 | P1 | Under-fire reaction | F26/F30V | Reaction component | IMPLEMENTED CORE | Temporary reaction may pause movement but cannot complete parent mission. |
| UE-P126 | P2 | Representative casualties | v00.00.08 | Visual subsystem | AUTHORITATIVE EVENT CORE IMPLEMENTED | Visual casualty feedback does not become authoritative casualty state. |

# 8. Infantry Square and anti-cavalry — U06

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P130 | P1 | Square formation geometry | F29 | Formation component | IMPLEMENTED CORE | Infantry physically reforms to square. |
| UE-P131 | P1 | Four square fire sectors | F29V/X/Z | Combat geometry | IMPLEMENTED CORE | Square fires only from eligible face/sector. |
| UE-P132 | P1 | Square visual ownership | F29X | Presentation | IMPLEMENTED QA CORE | Square outline replaces incompatible Line/Column footprint/fans. |
| UE-P133 | P1 | No formation phantom volley | F29X | Fire state machine | IMPLEMENTED CORE | Entering/readying Square does not produce smoke/0-hit volley. |
| UE-P134 | P1 | LOS-gated cavalry threat | F30S | Reaction AI | IMPLEMENTED CORE | Square/anti-CAV reaction requires actual LOS. |
| UE-P135 | P1 | Range/cone-gated CAV fire | F30S | Combat eligibility | IMPLEMENTED CORE | Cavalry must be inside LOS + selected range + valid sector/cone. |
| UE-P136 | P1 | Directional square smoke | F29Z | Niagara | FIRING-FACE EVENT CORE / NIAGARA PENDING | Smoke originates from the firing square face only. |

# 9. Officer AI and command hierarchy — U07

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P150 | P0 | Company/Captain AI | Officer AI baseline | AI controller/component | IMPLEMENTED CORE | Company executes delegated mission locally without overriding direct player authority. |
| UE-P151 | P0 | Major/Battalion AI | F27 | AI controller/component | IMPLEMENTED CORE | Major coordinates subordinate company slots/mission. |
| UE-P152 | P0 | Regiment AI | F28+ | AI controller/component | IMPLEMENTED CORE | Regiment delegates coherently to both battalions. |
| UE-P153 | P0 | Brigade AI | F30B/M | Higher AI | IMPLEMENTED CORE | Brigade delegates through Regiment. |
| UE-P154 | P0 | Division AI | F30B/M | Higher AI | IMPLEMENTED CORE | Division delegates through Brigade/Regiment/Majors. |
| UE-P155 | P0 | AI cascade ON | F30M | Command AI state | IMPLEMENTED CORE | Higher AI ON cascades to intended subordinate command chain. |
| UE-P156 | P0 | Direct order precedence | F18/F30M | Authority model | IMPLEMENTED CORE | Direct player mission beats inherited AI task. |
| UE-P157 | P1 | AI ON/OFF uniform state | F30W | OOB/AI state | IMPLEMENTED DATA/QA CORE | Division→Company+CAV all display only ON/OFF consistently. |
| UE-P158 | P1 | Mission completion | F30S/V/W | AI/order integration | IMPLEMENTED CORE | AI no longer chases/rearms after finite mission physically completes. |
| UE-P159 | P1 | HQ follow | F30P | HQ movement | IMPLEMENTED CORE | Major/Regiment/Brigade/Division HQs follow documented rear geometry. |
| UE-P160 | P1 | HQ depth conflict prevention | F30U | HQ goal authority | IMPLEMENTED ARCHITECTURE CORE | Defend mission owner prevents competing HQ-depth goal writers. |
| UE-P161 | P1 | Command zones | F30P | Command visualization/component | IMPLEMENTED QA CORE | Selected HQ shows correct inner/outer command bands. |
| UE-P162 | P2 | Command-zone gameplay effects | design B-061/F30P | Command simulation | IMPLEMENTED CORE | Order delay/coordination effects implemented after parity presentation. |
| UE-P163 | P2 | Officer stats/personality | B-170/B-190 | AI decision data | IMPLEMENTED DATA CORE | Shared player-delegated/enemy AI decision core uses officer data. |

# 10. Cavalry and dragons — U08

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P180 | P0 | Cavalry base actor | F30 | `ACavalryUnit` | IMPLEMENTED CORE | Mounted tactical entity has strength/order/command state. |
| UE-P181 | P1 | Mounted cavalry movement | F30 | Cavalry movement | IMPLEMENTED CORE | Mounted unit moves as formation with appropriate speed/state. |
| UE-P182 | P1 | 4-rank Line | F30H | Formation generator | IMPLEMENTED CORE | Normal mounted Line physically uses four ranks. |
| UE-P183 | P1 | 4-rank Charge | F30H | Formation generator | GEOMETRY CORE VIA CAVALRY LINE | Charge line uses four ranks. |
| UE-P184 | P1 | 4-abreast Column | F30H | Formation generator | IMPLEMENTED CORE | Normal mounted march column is four abreast. |
| UE-P185 | P1 | 2-abreast bridge/defile | F30H/X | Formation policy | GEOMETRY CORE IMPLEMENTED | Narrow two-abreast geometry exists only near/on crossing. |
| UE-P186 | P1 | 36 m narrow-mode approach | F30X | Bridge formation policy | IMPLEMENTED CORE | Opposite-bank mission does not trigger two-abreast hundreds of metres early. |
| UE-P187 | P1 | Restore pre-bridge formation | F30H/X | Crossing state | DEFILE RESTORE CORE IMPLEMENTED | Formation used before narrow mode is restored after exit clearance. |
| UE-P188 | P1 | Physical cavalry reform | F30H | Formation movement | REFORM/CHARGE-GATE CORE / 1:1 RIDER MOTION PENDING | Riders physically reform; charge speed respects incomplete reform. |
| UE-P189 | P1 | Cavalry charge/contact | F30/F30N | Cavalry combat | IMPLEMENTED CORE | Charge executes to valid contact without passing through enemy formation. |
| UE-P190 | P1 | Defensive cavalry reserve | F30S | Higher mission planner | IMPLEMENTED CORE | Attached CAV stays behind/outside supported battalion (~150m rear/~45m lateral QA baseline). |
| UE-P191 | P1 | Temporary attack attachment | F30M | Command-parent state | IMPLEMENTED CORE | Available CAV task-attaches to attacking Major A/B without changing OrganicParent. |
| UE-P192 | P1 | Travel-cost pairing | F30M | Task allocator | IMPLEMENTED CORE | Two-CAV/two-battalion allocation minimises crossing/travel cost. |
| UE-P193 | P1 | Return to prior command parent | F30M | Command-parent state | IMPLEMENTED CORE | CAV returns to previous parent/reserve after attack mission ends. |
| UE-P194 | P2 | Cavalry screen/opportunity AI | F30N | Cavalry AI | IMPLEMENTED CORE | CAV screens/repositions without unjustified autonomous charge. |
| UE-P195 | P2 | SPEJD HER | F30Q | Recon order | IMPLEMENTED CORE | Enabled only after true FOG/contact subsystem exists. |
| UE-P196 | P2 | Dragon dismount | F30J/S | Cavalry specialization | IMPLEMENTED CORE | Mounted Dragon splits into combat group + horse park/holders. |
| UE-P197 | P2 | Horse-holder anchor | F30J/S | Dismounted state | IMPLEMENTED DATA CORE | Horse holders/horses stay at dismount anchor while combat group moves. |
| UE-P198 | P2 | STIG OP return/remount | F30S | Remount task | IMPLEMENTED CORE | Away-from-horses remount becomes return task then auto-remount. |
| UE-P199 | P2 | Dismounted Dragon fire control | F30R | Fire component | IMPLEMENTED CORE | Dragon firing respects policy, range, LOS and anchor state. |
| UE-P200 | P1 | Horse + rider separate rigs | Engine-port decision | Skeletal meshes + AnimBP | SCAFFOLD IMPLEMENTED | Horse/rider are independent rigs synchronized at RiderSocket/saddle. |

# 11. OOB, HUD, semantic zoom and tactical presentation — U09

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P220 | P1 | Unified command HUD | F29C/G | UMG | QA HUD SCAFFOLD | Relevant selected echelon receives one authoritative HUD. |
| UE-P221 | P1 | Company HUD | HUD baseline | UMG | QA HUD SCAFFOLD | Unit info, state, orders and fire controls are readable. |
| UE-P222 | P1 | Major HUD | F29C | UMG | QA HUD SCAFFOLD | Major orders/status/AI/subordinates shown. |
| UE-P223 | P1 | Regiment HUD | F29C/G | UMG | QA HUD SCAFFOLD | Regiment mission/status/subordinates shown. |
| UE-P224 | P1 | Brigade HUD | F30M/N/O | UMG | QA HUD SCAFFOLD | Brigade HUD has ENHEDSINFO/AI, ORDRER/MISSION, UNDERLAGTE/STATUS/ATTACHMENT. |
| UE-P225 | P1 | Division HUD | F30M/N/O | UMG | QA HUD SCAFFOLD | Division HUD reaches same higher-command parity. |
| UE-P226 | P1 | Cavalry HUD | F30H/I/N | UMG | QA HUD SCAFFOLD | CAV uses shared visual language with cavalry-specific controls/state. |
| UE-P227 | P1 | OOB panel | F29Q/F30D | UMG tree/list | DATA/API CORE / UMG PENDING | Full Division→CAV hierarchy is scrollable/selectable. |
| UE-P228 | P1 | OOB AI status | F30W | UMG | DATA/QA HUD CORE | All echelons show consistent ON/OFF. |
| UE-P229 | P1 | OOB attachment state | F30D/M | UMG | IMPLEMENTED DATA CORE | Organic/current/task attachment is readable. |
| UE-P230 | P1 | OOB cavalry drag/drop | F30D | UMG drag/drop | ATTACHMENT API CORE / UMG PENDING | Valid tactical attachment can be changed via OOB without corrupting OrganicParent. |
| UE-P231 | P1 | Semantic zoom states | F29D | Presentation subsystem | IMPLEMENTED CORE | Close/Medium/Operational/Strategic presentation changes without simulation changes. |
| UE-P232 | P1 | HQ semantic counters | F29D/Q | UMG/world overlay | IMPLEMENTED CORE | Major/Regiment/Brigade/Division counters appear at correct zoom importance. |
| UE-P233 | P1 | Company semantic counters | F29D | UMG/world overlay | IMPLEMENTED CORE | Operational/strategic view presents company state. |
| UE-P234 | P1 | Cavalry semantic counters | F30H | UMG/world overlay | IMPLEMENTED CORE | Cavalry gets appropriate counter/name/echelon. |
| UE-P235 | P1 | NATO echelon symbols | F29D/F30H | Presentation data | IMPLEMENTED CORE | Company I, Battalion II, Regiment III, Brigade X, Division XX. |
| UE-P236 | P1 | Strategic mesh suppression | F29D | Presentation LOD | IMPLEMENTED CORE | Far zoom hides tactical meshes while simulation/colliders/command state continue. |
| UE-P237 | P1 | Selected HQ command circles | F30P | World visualization | IMPLEMENTED QA CORE | Only selected HQ level exposes detailed command range rings. |
| UE-P238 | P1 | Route/destination visuals | F30M/O | World visualization | IMPLEMENTED QA CORE | Higher selection exposes subordinate authoritative routes/slots without duplicate writers. |
| UE-P239 | P1 | Hover info infantry | battle UI baseline | Hover subsystem | IMPLEMENTED DATA CORE | Unit type/strength/loss/morale/order data appears. |
| UE-P240 | P1 | Hover info cavalry | F30S | Hover subsystem | IMPLEMENTED DATA CORE | CAV hover includes mounted state, formation, AI/order, parent and Dragon fire data. |

# 12. Visual assets and animation parity

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P250 | P0 | Infantry source FBX | imported Unity asset | UE Skeletal Mesh | SOURCE PRESENT | Source asset remains preserved for Unreal import/retarget. |
| UE-P251 | P1 | Infantry skeletal import | current infantry rig | UE Skeleton/SkeletalMesh | BACKLOG | Soldier imports with valid materials/skeleton/scale. |
| UE-P252 | P1 | Infantry locomotion | Unity walk/march | AnimBP | BACKLOG | Idle/march/reform movement visually tracks formation movement. |
| UE-P253 | P1 | Infantry firing | combat visuals | AnimBP/Montage | BACKLOG | Fire animation aligns with authoritative volley event. |
| UE-P254 | P2 | Infantry visual fidelity | B-300 | Mesh/material pass | BACKLOG | Soldier silhouette is recognisably armed infantry at tactical zoom. |
| UE-P255 | P1 | Horse skeletal import | horse project asset | Horse Skeleton | BACKLOG | Quadruped skeleton and mesh function independently of rider. |
| UE-P256 | P1 | Horse gait states | F30L | Horse AnimBP | BACKLOG | Walk/trot/charge states correspond to cavalry movement state. |
| UE-P257 | P1 | Rider skeletal import | cavalry asset plan | Rider Skeleton | BACKLOG | Rider remains separate human rig. |
| UE-P258 | P1 | Rider/horse synchronization | Engine-port decision | RiderSocket + state sync | BACKLOG | Rider stays aligned with saddle and gait. |
| UE-P259 | P2 | Dismount/remount animation | F30J/S | Montage/state machine | BACKLOG | Visual transition matches authoritative mounted state. |
| UE-P260 | P2 | Regimental standards/flag system | B-104/B-310 | Flag pole + dynamic cloth/material | BACKLOG | Standard pole is reusable and flag identity/data can vary per unit. |
| UE-P261 | P2 | 1864 cavalry equipment | approved asset direction | Skeletal/static attachments | BACKLOG | Saddle/tack/rifle+bayo/sabre/standard can be equipped independently. |

# 13. Tactical time, scenario flow and baseline utilities

| ID | Pri | Feature | Unity/reference | Unreal target | Status | Exit / parity criterion |
|---|---:|---|---|---|---|---|
| UE-P270 | P1 | Pause | Unity tactical baseline | Game speed subsystem | IMPLEMENTED CORE | Player can pause tactical singleplayer without corrupting AI/orders. |
| UE-P271 | P1 | 1x/2x/3x | Unity tactical baseline | Game speed subsystem | IMPLEMENTED CORE | Time controls behave consistently for movement, combat and AI timers. |
| UE-P272 | P1 | Victory/defeat state | Unity tactical baseline | Scenario subsystem | IMPLEMENTED CORE | Battle can resolve and expose outcome state. |
| UE-P273 | P1 | Restart/test reset | Unity tactical baseline | Scenario subsystem | IMPLEMENTED CORE | QA battle can restart to deterministic baseline. |
| UE-P274 | P2 | Deterministic QA seed | B-133 | Scenario/debug subsystem | IMPLEMENTED CORE | Repeat tests can reproduce critical movement/combat situations. |

# 14. U10 — tactical replacement parity gate

These are **release gates**, not separate optional polish items.

| ID | Pri | Gate | Required result |
|---|---:|---|---|
| UE-G001 | P0 | U01 controls | Camera + click + marquee + modifiers build and pass runtime QA. |
| UE-G002 | P0 | Command hierarchy | Division→Brigade→Regiment→Major→Company + CAV parentage works. |
| UE-G003 | P0 | Order authority | Player/higher-AI authority and pending/executing/standing states do not conflict. |
| UE-G004 | P0 | Infantry movement | March Column, Line, early deploy, spacing and final-slot completion pass. |
| UE-G005 | P0 | River/bridge | Same-bank routes avoid bridge round-trip; true bank changes use bridge. |
| UE-G006 | P0 | Enemy cones | QA cones visible at battle start but provide no knowledge/fire authority. |
| UE-G007 | P0 | Fire eligibility | LOS + range + cone determine valid fire. |
| UE-G008 | P0 | Square/CAV | Square reaction and fire do not bypass LOS/range/sector rules. |
| UE-G009 | P0 | Cavalry crossing | CAV stays normal formation far away and switches to 2-abreast only near bridge. |
| UE-G010 | P0 | Order completion | Parent button stays active until actual final-slot/HQ/CAV execution ends. |
| UE-G011 | P0 | Selection persistence | Higher HQ remains selected through target/facing order commit. |
| UE-G012 | P1 | OOB/HUD | AI status, hierarchy, current command parent and active order state are consistent. |
| UE-G013 | P1 | Higher HQ geometry | Brigade/Division follow, facing and command zones reproduce documented behaviour. |
| UE-G014 | P1 | Semantic zoom | Presentation changes do not alter simulation state. |
| UE-G015 | P1 | Full battle regression | Combined battle scenario runs without critical regression relative to Unity baseline. |

## Current migration order

1. **Finish U01 build/runtime verification** — UE-P010 through UE-P018.
2. **U02 hierarchy/data** — UE-P030 through UE-P040.
3. **U03 order authority** — UE-P050 through UE-P064.
4. **U04 infantry formations/movement** — UE-P070 through UE-P083.
5. **U05 navigation/river/bridge** — UE-P090 through UE-P099.
6. **U06 combat/LOS/Square** — UE-P110 through UE-P136.
7. **U07 officer AI/HQ** — UE-P150 through UE-P163.
8. **U08 cavalry/dragons** — UE-P180 through UE-P200.
9. **U09 OOB/HUD/semantic zoom** — UE-P220 through UE-P240.
10. **Assets/animation parity** in parallel where it does not block simulation.
11. **U10 parity gate** — UE-G001 through UE-G015.

## Definition of Unreal tactical parity

The Unreal port reaches tactical parity only when:

- all **P0** items required by UE-G001..UE-G011 are **PARITY VERIFIED**;
- all **P1** items required by UE-G012..UE-G015 are either **PARITY VERIFIED** or explicitly documented as non-blocking presentation differences;
- the full U10 battle regression has been recorded against the current Unity reference;
- no Unreal system silently simplifies established command authority, formations, navigation, LOS/fire eligibility, cavalry behaviour, OOB semantics or execution completion.

After that gate, Unreal may become the active tactical implementation and Unity can move from behavioural reference to archived/reference status.


## U02 runtime assembly QA

The current Unreal test GameMode can auto-spawn a Danish command hierarchy for U02 validation:

`Division -> Brigade -> Regiment -> Major A/B -> 4 companies each`.

Expected runtime checks:

1. Core OOB spawns 13 Danish command/infantry entities: 1 Division HQ, 1 Brigade HQ, 1 Regiment HQ, 2 Major HQs and 8 Companies. With current default QA toggles, 2 Danish cavalry and 2 Prussian companies are added for 17 strategy entities total.
2. Every entity has a non-empty stable ID and readable debug label.
3. Major A/B report Regiment as both OrganicParent and CurrentCommandParent.
4. Companies 1-4 report Major A; Companies 5-8 report Major B.
5. Parent `OrganicSubordinates` and `CurrentSubordinates` counts match the hierarchy.
6. `RestoreOrganicCommandParent()` can restore a temporarily retasked unit without changing OrganicParent.
7. All placeholder actors can be selected through the Visibility trace collider.
8. No C++/UHT/editor build errors.


## U03 direct-order executor QA

Current U03 executor baseline:

1. Select one or more strategy units.
2. Issue MOVE through `IssueOrderToSelection` or pending-order placement.
3. Selection remains unchanged after the order is committed.
4. Unit enters `Moving`, order enters `Executing`, and actor moves toward the target.
5. Arrival uses a 50 cm tolerance, matching the current Unity final-slot authority scale.
6. On arrival, actor reaches target XY, unit returns to `Ready`, and execution becomes `Completed`.
7. HOLD immediately stops physical movement and completes execution while retaining HOLD as standing intent.
8. A new DirectPlayer order supersedes the previous executing DirectPlayer order cleanly.
9. Lower OfficerAI/InheritedAI authority cannot replace an active DirectPlayer order.
10. ANGRIB HER and FORSVAR HER currently share the finite physical movement executor; their formation/subordinate geometry remains U04/U03-planner work and must not yet be marked parity verified.


## U04 formation-planning baseline QA

Current U04 geometry baseline:

1. Every strategy unit owns a `UStrategyFormationComponent`.
2. Company Line uses three ranks by default.
3. 190-man Line geometry uses 3 ranks and 0.75 m lateral slot spacing, producing roughly the documented Unity frontage scale.
4. March Column uses a 4-wide baseline and longitudinal row spacing.
5. Major/Battalion ANGRIB HER and FORSVAR HER decompose into company missions.
6. Company slot order is deterministic by `CompanyNumber`.
7. Company centres use 72 m nominal spacing and never less than the 68 m reserved baseline.
8. The committed parent facing is copied into company slot missions before movement starts.
9. DirectPlayer authority on the parent mission is preserved on generated company missions.
10. This block does not yet implement enemy-range-driven early deployment, physical individual-soldier reform, or Regiment/Brigade/Division multi-level slot planning.


## Combined U04-U06 implementation checkpoint

Additional implementation in this checkpoint:

- Click-drag order placement now derives committed facing from the target-to-drag vector while preserving selection.
- Attack/Defend orders now cascade recursively through Division -> Brigade -> Regiment -> Battalion/Major -> Company.
- Command-parent HQ actors no longer move directly onto Attack/Defend objectives; they retain mission authority while subordinates execute.
- Parent execution completion polls immediate subordinates, allowing completion to propagate upward only after child execution ends.
- Long company moves can enter March Column automatically.
- F30X early-deploy policy uses `max(own MaximumFireRange, nearest enemy MaximumFireRange) + 35m` and switches March Column -> Line when the threshold is reached.
- Movement now follows formation-level Unreal NavMesh waypoints when a valid nav path exists, with direct fallback only when no path is available.
- Final target completion remains 50 cm; intermediate waypoint tolerance is separate and does not complete the mission.
- A shared LOS component performs Visibility-channel traces.
- A shared fire-control component owns Hold/Close/Medium/Long policy, active range, ±35 degree cone and LOS/range/cone target eligibility.
- QA range cones are presentation only. Target eligibility still calls the LOS/fire-control authority independently.
- Prussian QA companies can spawn in the runtime test scenario, and their Close/Medium/Long range cones are drawn from startup for test visibility.

Still pending before parity:
- runtime UE 5.8.3 build/QA for this checkpoint;
- physical 1:1 soldier reform into generated slots;
- river-bank classification and bridge transaction;
- actual volley/reload/ammunition/casualty execution;
- final command-HQ rear/follow geometry;
- UMG active-order colour presentation.


## Formation extension checkpoint

- Square now has a four-sided slot geometry core with outward-facing side orientations.
- Cavalry Line uses four ranks.
- Cavalry Column uses four abreast.
- Defile Column uses two abreast.
- `ACavalryUnit::SetDefileMode()` stores the pre-defile formation and restores it when narrow mode ends.
- These are geometry/state cores only. Bridge-distance activation, cavalry physical rider reform, Square fire-sector authority and charge-contact logic still require their later systems.


## U05-U06 river/combat execution checkpoint

- `AStrategyRiverBarrier` defines a river axis, half-width, bridge approaches and exit clearance.
- Route planning distinguishes opposite-bank missions from same-bank chords that intersect the river.
- Opposite-bank routes receive an explicit bridge transaction with enter/exit indices.
- Same-bank missions detour along the same bank instead of crossing and returning.
- Cavalry switches to two-abreast Defile only when within ~36 m of the bridge approach or inside the active bridge transaction, then restores its stored pre-defile formation after exit.
- Runtime QA scenario can spawn a configurable river/bridge barrier.
- `UStrategyCombatComponent` now executes volleys with ammunition consumption, reload time, deterministic per-unit random stream and range-adjusted hit chance.
- Zero-hit volleys are valid firing events and do not reduce target strength.
- Positive hits apply authoritative strength loss; strength zero sets Destroyed.
- Incoming volleys apply shock to separate Morale and Cohesion values.
- Under-fire reaction can temporarily pause a moving unit without completing or replacing its parent mission; movement resumes from the same route.
- Square target eligibility now uses four 90-degree faces instead of the normal forward ±35-degree cone.

All items remain build/runtime QA pending until UE 5.8.3 validates this checkpoint.


## U07-U08 officer AI / cavalry tasking checkpoint

- Every strategy unit now owns one shared `UStrategyOfficerAIComponent`; player-delegated and enemy-side units use the same authority machinery.
- AI ON/OFF can cascade recursively through `CurrentSubordinates`.
- Non-Attack/Defend parent missions can be inherited by subordinate AI only when no protected DirectPlayer authority is active.
- Finite DirectPlayer authority is released after physical completion; standing DirectPlayer HOLD/FORSVAR remains protected until replaced.
- HQ follow is a background movement layer, not an order writer. It derives position from subordinate centroid plus mission-facing rear/lateral offsets.
- Current QA follow defaults: Battalion 65 m rear; Regiment 90 m rear; Brigade 120 m rear +65 m lateral; Division 145 m rear -75 m lateral.
- A directly executing player MOVE temporarily owns HQ position and suppresses background follow.
- Selected HQs draw their existing inner/outer command-zone radii; only selection controls the QA visualization.
- Rout thresholds are now authoritative: morale <=20 or cohesion <=10 sets Routed, stops movement, sets HOLD FIRE and fails current physical execution.
- Infantry company anti-cavalry reaction requires actual LOS and a configurable mounted-threat distance; it enters Square, preserves the previous formation and can restore it after the threat has been absent for a delay.
- Higher-HQ infantry slot planning explicitly excludes cavalry.
- On higher ANGRIB HER, available directly attached cavalry can be temporarily task-attached to commanded Major/Battalion HQs using `CurrentCommandParent` only; `OrganicParent` is unchanged.
- With two cavalry and two Majors, the direct vs crossed pairings are compared and the lower total squared travel cost is chosen.
- Cavalry with a currently executing DirectPlayer order is protected from automatic task attachment.
- When the higher attack execution completes/fails/is superseded, temporary cavalry attachments restore the previous command parent and receive a reserve move that does not re-open the higher tactical execution state.
- FORSVAR HER keeps cavalry under its higher parent but gives separated reserve moves behind paired Majors using the ~150 m rear / ~45 m lateral QA baseline.
- Runtime test OOB can now spawn two Danish cavalry directly under Division plus two non-player-controllable Prussian QA companies.


## v00.02.54 — ten-block implementation batch

1. **QA build marker / selected-state HUD:** the tactical HUD now shows the Unreal-port build marker plus selected unit identity, strength, morale, cohesion, AI, current order and execution state.
2. **OOB aggregate data:** every strategy unit owns a recursive aggregate-status component reporting strength, unit count, routed/destroyed count and physically executing descendants.
3. **OOB select/focus API:** one call selects an OOB entity; optional focus recentres the RTS camera, providing the single-click/double-click backend for the future UMG tree.
4. **Semantic zoom:** Close/Medium/Operational/Strategic/VeryFar states are driven by camera zoom. Camera range now reaches the documented operational/strategic scale.
5. **NATO/counter core:** unit labels expose Company I, Battalion II, Regiment III, Brigade X, Division XX and CAV identity. Strategic/VeryFar hides tactical mesh components without disabling simulation/collision.
6. **Command tree / mission / route QA visuals:** selected command entities recursively expose subordinate links, authoritative mission targets/facing and current movement routes.
7. **Order colour lifecycle:** command visual state is BLUE only while PendingTarget/Pending/Executing; completed standing intent is RED. GREEN remains reserved for toggle/state controls.
8. **Time controls:** Space toggles tactical pause; 1/2/3 select 1x/2x/3x and automatically leave pause.
9. **Cavalry charge/contact:** CHARGE is now a finite movement mission with mounted charge speed, 4-rank CavalryLine, swept enemy contact detection and stop-on-contact so cavalry cannot pass through a valid enemy formation.
10. **Dragoon state core:** Dragoon role supports 75/25 combat-group/horse-holder split, horse-park anchor, dismounted movement/fire limit, STIG OP return-to-anchor and automatic remount. The second Danish QA cavalry is configured as Dragoon.

These ten blocks are **code implemented / UE 5.8.3 build+runtime QA pending**. None are PARITY VERIFIED yet.


## v00.02.55 — next ten implementation blocks

1. **Command hierarchy robustness:** organic re-parenting now updates CurrentCommandParent only when the unit was actually following its old organic parent; temporary tactical attachment is preserved. Parent assignments reject self/cyclic command chains.
2. **Deterministic QA combat seed:** combat RNG can be explicitly seeded; the runtime fixture derives a per-unit deterministic seed from `QARandomSeed ^ StableUnitId hash`.
3. **Stable-ID/hierarchy validation:** every QA build checks missing/duplicate IDs, organic/current backlink integrity and command-parent cycles.
4. **Victory/defeat state:** scenario state tracks combat-effective Company/Cavalry forces and exposes InProgress / DenmarkVictory / OppositionVictory / Draw.
5. **Deterministic reset:** F5 clears selection, resets outcome and rebuilds the same QA fixture from the same seed.
6. **Regression checklist:** each QA spawn runs repeatable checks for expected entity count, hierarchy/ID validity, enemy ownership, cavalry/Dragoon configuration and river fixture.
7. **RYK FREM:** higher-command Advance now cascades through Division/Brigade/Regiment/Major and generates physical subordinate slots.
8. **TILBAGETRÆK:** coordinated Withdraw uses the same authority-preserving hierarchy/slot pipeline and parent completion rules.
9. **SAML:** Assemble now cascades to subordinate formation slots and completes from physical subordinate execution rather than HQ bookkeeping.
10. **Shared OOB/hover snapshot:** one presentation data source exposes identity, NATO echelon, strength/losses, morale/cohesion/fatigue, state, AI ON/OFF, order/execution, formation, organic/current parent, temporary attachment, ammunition, cavalry role and mounted state.

Coordinated formation missions (Attack/Defend/Advance/Withdraw/Assemble) are owned by the command parent while subordinate formations execute; the parent HQ does not run directly onto the objective. All items remain UE 5.8.3 build/runtime QA pending.


## v00.02.56 — formation/navigation/combat parity batch

1. **Formation reform transition:** formation changes now create an explicit Reforming interval derived from base time + current strength. Active movement is paused without clearing route/order authority and resumes the same mission afterwards.
2. **Final facing completion:** reaching the final XY slot no longer completes the order immediately when facing is committed; the unit rotates physically at the configured turn rate until within a 2° tolerance, then completes.
3. **Mission reassert after interruption:** reform and under-fire pauses reuse the same movement route/order serial rather than issuing a replacement mission.
4. **190-man footprint QA:** Line frontage is validated against the current ~48 m full-company baseline; the regression fixture checks every Danish infantry company.
5. **Local company deconfliction:** moving friendly companies apply bounded local sidestep separation when centres fall below the 68 m reserved baseline.
6. **Hard-water authority:** route plans now carry validity/failure data. Start/destination positions inside a river barrier are rejected, and the movement executor fails the physical order instead of silently falling back to direct movement through water.
7. **Bridge-goal persistence:** a new movement goal issued during an active bridge transaction preserves the current crossing through its exit, then appends the new post-crossing route.
8. **Real-volley visual event:** smoke/Niagara-ready origin/direction/shots/hits data is broadcast only from a successfully resolved real volley.
9. **Representative casualty event:** presentation receives casualty events only after positive authoritative strength loss; visual casualty representation cannot become the source of truth.
10. **Square visual/fire ownership:** Square owns its outline presentation; reforming units are ineligible to fire, preventing formation phantom volleys. Square volley visual origin is projected to the actual firing face.

All ten blocks remain UE 5.8.3 build/runtime QA pending. Physical 1:1 soldier movement and Niagara asset binding are intentionally not claimed as complete.


## v00.02.57 — 23-block functionality-first batch

Graphics/assets are deliberately deferred. This batch focuses on gameplay authority and simulation:

1. **Finite Attack anchor:** ANGRIB HER stores a fixed mission anchor/facing and remains finite; no target-actor chasing is introduced.
2. **Persistent Defend anchor:** leaf executors reassert the same FORSVAR HER anchor/facing after temporary displacement without creating a replacement mission. Higher HQ geometry remains owned by HQ-follow.
3. **Generic navigation obstacle actor:** buildings, fences, fieldworks and generic hard obstacles have explicit bounds/clearance data.
4. **Building/fence detour:** route post-processing inserts bounded detour points for explicit static obstacles while keeping formation-level route authority.
5. **Slope validation:** routes above the configured traversable slope are rejected rather than silently accepted.
6. **Officer profile data:** Leadership, Initiative, StaffQuality, Aggression and Caution are now shared officer data.
7. **Command-zone order delay:** subordinate orders can be queued with delay based on inner/outer/outside command range plus parent command efficiency. Delayed child orders count as active execution, so parents cannot complete early.
8. **Fatigue accumulation/recovery:** movement, reform and combat build fatigue; rest recovers it.
9. **Experience/fatigue combat effects:** experience/fatigue modify accuracy and morale-shock susceptibility.
10. **Cavalry base gameplay defaults:** mounted speed, cavalry formation defaults, strength fallback and mounted fire-range baseline are self-contained on ACavalryUnit.
11. **Cavalry reform charge gate:** CHARGE cannot begin while cavalry formation transition is incomplete.
12. **Cavalry screen AI:** AI cavalry can screen/reposition relative to parent and visible enemy but never auto-charge and never replaces an active/standing mission.
13. **Contact memory:** units maintain current-visible and last-known enemy contacts with a configurable forgetting horizon.
14. **FOG/contact fire gate:** fire eligibility now requires current contact in addition to LOS, range and cone/sector.
15. **SPEJD HER recon state machine:** ScoutHere now runs Seek -> Recon -> Contact -> Screen -> Report / LastKnown and completes after recon work, not simply on arrival.
16. **OOB tactical attachment API:** cavalry can be task-attached to a valid same-side HQ through CurrentCommandParent and restored to OrganicParent; active DirectPlayer movement is protected.
17. **Bridge congestion/queue:** river barriers own crossing occupancy; units wait at the approach when the crossing is occupied and release the slot after exit/abort.
18. **Autonomous opposition AI:** rootless Prussian/Austrian/enemy companies with AI enabled advance to a finite engagement position based on current contact and selected fire range. It yields to hierarchy and standing intent.
19. **Routed fallback:** routed units clear normal mission authority and withdraw away from the nearest enemy while remaining Routed.
20. **Rally recovery:** routed units recover morale/cohesion only when safely separated; Rally thresholds return them to Ready.
21. **Ammo exhaustion/resupply:** combat exposes authoritative out-of-ammo state and bounded ammunition resupply.
22. **Terrain movement cost/elevation:** fatigue and uphill/downhill modifiers affect movement speed, and movement now follows waypoint Z instead of remaining flat in XY.
23. **Expanded regression gate:** QA validates new gameplay-core components, command-delay sanity, ammunition sanity, cavalry screen/Dragoon setup, river fixture and explicit navigation-obstacle fixture.

Additional fixes in the same pass:
- routed fallback state is preserved through ClearOrder/Withdraw/arrival until rally;
- bridge ownership is released before route-plan reset;
- static obstacle post-processing cannot shift explicit bridge transaction indexes;
- screen AI cannot overwrite active missions;
- autonomous opposition AI cannot override a command parent or standing intent.

All items remain **UE 5.8.3 build/runtime QA pending** until compiled and exercised in editor. 1:1 soldier/rider animation, skeletal assets, Niagara, UMG polish and other graphics remain intentionally deferred.


## v00.02.58 — combat/fieldcraft/AI functionality batch

Functionality-first work added fire discipline/ammunition conservation, Standing/Prone stance, directional cover, hasty fieldworks, black-powder smoke LOS simulation, skirmisher detach/roles/reform, tactical ammunition supply, doctrine/OrderAgg, autonomy, no-cheat AI difficulty, AI-DIAG telemetry, expanded officer dimensions and mission constraints. Graphics remain intentionally deferred. All items remain UE 5.8.3 build/runtime QA pending.


# 15. Artillery battery functionality — v00.02.59

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P300 | P0 | Artillery echelon/types | Artillery unit identity + mobility/fire/ammo/ownership types | IMPLEMENTED CORE |
| UE-P301 | P0 | Battery actor | `AStrategyArtilleryBatteryUnit` | IMPLEMENTED CORE |
| UE-P302 | P0 | Guns/crew/drivers/horses | Battery authoritative personnel/materiel state | IMPLEMENTED CORE |
| UE-P303 | P0 | Limber/unlimber | Timed deployment state machine | IMPLEMENTED CORE |
| UE-P304 | P0 | Towed movement gate | Limbered + horse/driver mobility requirement | IMPLEMENTED CORE |
| UE-P305 | P1 | Manhandling | Short slow movement while unlimbered | IMPLEMENTED CORE |
| UE-P306 | P0 | Artillery ammunition inventory | Round/Shell/Shrapnel/Canister stores | IMPLEMENTED CORE |
| UE-P307 | P0 | Manual target fire mission | Manual target is player-controlled default | IMPLEMENTED CORE |
| UE-P308 | P0 | Hold Fire | Explicit fire-mode override preserving target | IMPLEMENTED CORE |
| UE-P309 | P0 | Battery traverse | Timed deployed facing/traverse authority | IMPLEMENTED CORE |
| UE-P310 | P0 | LOS/contact/range gate | Current contact + LOS + ammo-specific min/max range | IMPLEMENTED CORE |
| UE-P311 | P0 | Reload/fire resolution | Operational-gun volley + crew/fatigue/experience reload | IMPLEMENTED CORE |
| UE-P312 | P1 | Ammunition effects | Ammo-specific range/hit/casualty prototype effects | IMPLEMENTED CORE / HISTORICAL TUNING PENDING |
| UE-P313 | P1 | Auto Target | Explicit opt-in visible-contact target scoring | IMPLEMENTED CORE |
| UE-P314 | P0 | Artillery resupply | Existing tactical supply transfers mixed compatible rounds | IMPLEMENTED CORE |
| UE-P315 | P0 | Artillery damage | Incoming fire allocates crew/horse/disabled/destroyed gun damage | IMPLEMENTED CORE |
| UE-P316 | P0 | Disabled/abandoned state | Disabled materiel separated from abandoned battery | IMPLEMENTED CORE |
| UE-P317 | P1 | Gun capture | Physical nearby enemy control after abandonment | IMPLEMENTED CORE |
| UE-P318 | P1 | Captured-gun reuse | Qualified crew + compatible ammunition + preparation time | IMPLEMENTED CORE |
| UE-P319 | P0 | OOB/QA integration | Danish 6-gun QA battery + snapshot/regression/outcome integration | IMPLEMENTED QA CORE |

### Artillery authority rules

- Manual target is the default for player artillery. AUTO TARGET is opt-in.
- HOLD FIRE overrides both manual and automatic targeting without deleting the stored manual target.
- Batteries fire only when deployed, operational, crewed, supplied, within ammo-specific range, inside traverse, and with current contact + LOS.
- Normal movement requires Limbered state plus drivers/horses; deployed batteries can only use bounded slow manhandling.
- Gun count, crew, drivers, horses, calibre/profile and ammunition stores are data/configurable. The QA six-gun battery is a test fixture, not a historical universal battery size.
- Infantry fire against artillery is routed into artillery-specific personnel/horse/gun damage instead of treating the battery as an infantry company.
- Artillery is excluded from higher-HQ infantry slot planning.
- Abandoned guns remain physical state and can be captured. Reuse requires suitable crew, compatible ammunition and preparation time.
- Division tactical supply can replenish artillery's separate ammunition inventory.
- Artillery participates in battle-outcome evaluation and OOB/hover data.
- Final historical gun profiles, ammunition ballistics, limber drill timings and national battery organisations remain research/data tuning.
- All UE-P300..319 are **code implemented / UE 5.8.3 build+runtime QA pending**, not parity verified.


# 16. Artillery refinement + physical logistics — v00.02.60

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P320 | P0 | Ground/area fire mission | Manual visible ground-point mission with dispersion radius | IMPLEMENTED CORE |
| UE-P321 | P1 | Salvo-limited fire mission | Configurable maximum salvos per manual mission | IMPLEMENTED CORE |
| UE-P322 | P1 | Duration-limited fire mission | Configurable mission-time ceiling | IMPLEMENTED CORE |
| UE-P323 | P1 | Artillery target priority | Balanced / CounterBattery / Infantry / ClosestThreat / ConserveAmmo | IMPLEMENTED CORE |
| UE-P324 | P1 | Auto ammunition selection | Target/range-aware Canister/Shrapnel/Shell/RoundShot choice | IMPLEMENTED CORE |
| UE-P325 | P1 | Ammunition reserve doctrine | Auto fire respects configurable reserve floor; ConserveAmmo enforces reserve | IMPLEMENTED CORE |
| UE-P326 | P0 | Artillery deployability slope | Four-point terrain sample rejects over-steep deploy position | IMPLEMENTED CORE |
| UE-P327 | P1 | Artillery work fatigue | Firing and manhandling create additional fatigue | IMPLEMENTED CORE |
| UE-P328 | P1 | Disabled-gun field repair | Timed crew repair restores disabled but never destroyed guns | IMPLEMENTED CORE |
| UE-P329 | P0 | Emergency abandon battery | Crew/drivers evacuate while physical guns remain capturable | IMPLEMENTED CORE |
| UE-P330 | P0 | Supply echelon + wagon actor | Horse-drawn physical logistics tactical unit | IMPLEMENTED CORE |
| UE-P331 | P0 | Split supply cargo | Separate small-arms and artillery ammunition cargo | IMPLEMENTED CORE |
| UE-P332 | P0 | Supply wagon mobility | Drivers, horses and wagon condition gate/scalar movement | IMPLEMENTED CORE |
| UE-P333 | P0 | Physical proximity resupply | Tactical transfer requires receiver inside source radius | IMPLEMENTED CORE |
| UE-P334 | P1 | Resupply interruption | Transfer pauses while source/receiver moves or is under fire | IMPLEMENTED CORE |
| UE-P335 | P0 | Supply wagon damage/destruction | Drivers, horses, wagon condition and cargo loss are authoritative | IMPLEMENTED CORE |
| UE-P336 | P0 | Supply wagon abandonment | Driver loss/explicit abandonment leaves physical cargo behind | IMPLEMENTED CORE |
| UE-P337 | P1 | Supply wagon capture | Nearby enemy control captures abandoned cargo source | IMPLEMENTED CORE |
| UE-P338 | P0 | Artillery ammunition compatibility | Ammunition-family tag gates captured/foreign artillery supply | IMPLEMENTED CORE |
| UE-P339 | P0 | Logistics QA/OOB integration | Low-ammo artillery + nearby Danish ammo wagon + regression/snapshot | IMPLEMENTED QA CORE |

### v00.02.60 authority notes

- Player artillery can target a visible ground point/area without inventing a hidden enemy actor.
- Manual fire missions can be bounded by salvos, elapsed mission time, or both.
- AUTO TARGET is still opt-in; its target priority and ammunition selection use only current observable targets.
- `ConserveAmmo` preserves a configurable reserve instead of simply picking a different target.
- Deployment legality now samples local terrain and can reject a slope even when navigation itself can traverse it.
- Firing, manhandling and repairs affect fatigue; repairs recover disabled guns only.
- Emergency abandonment leaves guns as physical battlefield state for capture.
- Supply wagons are independent units with cargo, drivers, horses and wagon condition.
- QA no longer depends on Division as a large magic ammunition source when the wagon fixture is enabled.
- Supply transfer is proximity-based, interrupted by movement/fire, and separates small-arms from artillery rounds.
- Artillery resupply from a wagon requires matching ammunition-family tags.
- Captured wagons may provide compatible cargo in place but remain immobile until a future re-crewing/re-horsing mechanic.
- Battlefield salvage/aftermath recovery and final historical ammunition compatibility tables remain later work.
- All UE-P320..339 remain **UE 5.8.3 build/runtime QA pending**.


# 17. Tactical terrain, crest/dead-ground and artillery positioning — v00.02.61

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P340 | P0 | Tactical terrain feature model | Hill / Ridge / Depression gameplay actors | IMPLEMENTED CORE |
| UE-P341 | P0 | Effective terrain elevation | Physical ground + analytic tactical elevation query | IMPLEMENTED CORE |
| UE-P342 | P0 | Local terrain slope | Shared slope sampling for movement/deployment | IMPLEMENTED CORE |
| UE-P343 | P0 | Terrain profile occlusion | Sampled sight-line terrain blocking | IMPLEMENTED CORE |
| UE-P344 | P0 | Crest / dead-ground query | Crest point + dead-ground classification | IMPLEMENTED CORE |
| UE-P345 | P0 | Unit terrain awareness | High-ground, reverse-slope and crest awareness component | IMPLEMENTED CORE |
| UE-P346 | P0 | Terrain-aware unit LOS | Direct LOS uses physical collision + tactical terrain profile | IMPLEMENTED CORE |
| UE-P347 | P0 | Terrain-aware location LOS | Visible ground/area points use same terrain truth | IMPLEMENTED CORE |
| UE-P348 | P1 | Elevation observation advantage | Contact awareness range scales with relative height | IMPLEMENTED CORE |
| UE-P349 | P1 | Crest/reverse-slope fire masking | Terrain exposure modifies infantry and artillery hit resolution | IMPLEMENTED CORE |
| UE-P350 | P0 | Route elevation projection | Waypoints are projected onto tactical terrain height | IMPLEMENTED CORE |
| UE-P351 | P0 | Terrain route sampling | Long routes are subdivided even without NavMesh before slope validation | IMPLEMENTED CORE |
| UE-P352 | P0 | Shared artillery deploy slope | Battery deployability uses tactical terrain slope | IMPLEMENTED CORE |
| UE-P353 | P1 | Artillery position assessment | LOS/slope/elevation/crest/dead-ground/move-cost score | IMPLEMENTED CORE |
| UE-P354 | P1 | Artillery candidate generation | Ring-based projected candidate positions | IMPLEMENTED CORE |
| UE-P355 | P1 | Best direct-fire position search | Filters dead-ground/LOS/slope and returns highest score | IMPLEMENTED CORE |
| UE-P356 | P0 | Terrain QA fixtures | Battery hill + central ridge + depression actors | IMPLEMENTED QA CORE |
| UE-P357 | P0 | Spawn projection | QA units spawn on effective tactical terrain Z | IMPLEMENTED QA CORE |
| UE-P358 | P0 | Artillery area-fire terrain authority | Area fire consumes shared terrain-aware location LOS | IMPLEMENTED CORE |
| UE-P359 | P0 | Terrain diagnostics/regression | Snapshot ground/slope/feature offset + dead-ground/crest/position QA | IMPLEMENTED QA CORE |

### v00.02.61 authority notes

- The gameplay terrain layer may combine real WorldStatic ground with analytic tactical features. Analytic features exist so LOS, slopes and artillery positioning can be tested before final landscape art.
- Terrain occlusion is authoritative for direct fire. Dead ground is not a visual effect.
- A reverse-slope target with terrain between shooter and target cannot be directly engaged merely because its actor exists.
- Partial crest masking can reduce target exposure before complete occlusion.
- Relative elevation modifies observation range, but it never bypasses LOS.
- Skirmisher observation and high-ground observation multiply through the same contact authority rather than separate omniscient logic.
- Route segments are sampled through terrain even if no NavMesh path exists, preventing long straight routes from skipping an intermediate hill.
- Artillery deployability, artillery area fire and artillery candidate scoring all consume the same terrain query layer.
- QA intentionally creates one blocked artillery lane and one clear lane across the ridge.
- Physical battlefield expansion/finished hills art is still separate from this gameplay terrain authority.
- All UE-P340..359 remain **UE 5.8.3 build/runtime QA pending**.


# 18. Artillery projectile presentation + follow camera — v00.02.62

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P360 | P0 | Resolved projectile data | Per-gun shot/impact/hit/casualty presentation records | IMPLEMENTED CORE |
| UE-P361 | P0 | Ammo-specific ballistic path | Round Shot / Shell / Shrapnel / Canister trajectory styles | IMPLEMENTED CORE |
| UE-P362 | P0 | Terrain-aware projectile path | Trajectory samples consume shared tactical ground elevation | IMPLEMENTED CORE |
| UE-P363 | P1 | Round Shot ricochet | Two diminishing post-impact bounces in presentation path | IMPLEMENTED CORE |
| UE-P364 | P0 | Projectile presentation actor | In-flight world actor follows precomputed trajectory | IMPLEMENTED CORE |
| UE-P365 | P1 | Trajectory debug line | Optional full ballistic QA path | IMPLEMENTED CORE |
| UE-P366 | P1 | Canister presentation | Short cone/pellet-ray presentation; not follow-camera eligible | IMPLEMENTED CORE |
| UE-P367 | P1 | Shell impact presentation | Impact sphere placeholder for later explosion VFX | IMPLEMENTED CORE |
| UE-P368 | P1 | Shrapnel burst presentation | Forward fragment-ray burst placeholder | IMPLEMENTED CORE |
| UE-P369 | P0 | Salvo presentation manager | Caps visible projectiles and spawns per resolved gun | IMPLEMENTED CORE |
| UE-P370 | P1 | Virtual per-gun muzzle origins | Battery-facing lateral gun positions + muzzle height | IMPLEMENTED CORE |
| UE-P371 | P1 | Projectile history/telemetry | Bounded shot serial/aim/impact/hit/casualty history | IMPLEMENTED CORE |
| UE-P372 | P0 | Battery projectile integration | Projectile presentation component on artillery battery | IMPLEMENTED CORE |
| UE-P373 | P0 | Direct-fire resolved impacts | Existing authoritative hit rolls now also emit per-gun impact points | IMPLEMENTED CORE |
| UE-P374 | P0 | Area-fire resolved impacts | Existing area salvo impact distribution feeds presentation exactly | IMPLEMENTED CORE |
| UE-P375 | P0 | Follow Shot camera | Camera follows latest live projectile in world space | IMPLEMENTED CORE |
| UE-P376 | P0 | Impact hold + camera restore | Camera holds at last impact then returns; manual camera input cancels cleanly | IMPLEMENTED CORE |
| UE-P377 | P1 | Follow Shot input | P toggles latest followable projectile | IMPLEMENTED QA CORE |
| UE-P378 | P1 | Projectile QA/debug integration | F9 trajectory toggle + snapshot/history + trajectory regression | IMPLEMENTED QA CORE |
| UE-P379 | P1 | Campaign→battlefield contract | Deterministic data contract for campaign terrain/features into future tactical generation | FOUNDATION IMPLEMENTED |

### v00.02.62 authority rules

- Projectile actors are **presentation only**. They never apply collision damage and never become a second combat authority.
- Direct and area fire first resolve their normal deterministic combat outcome; the same resolved per-gun impact points are then supplied to the presentation layer.
- A visible miss is therefore a resolved miss; the visual projectile does not independently decide whether it hit.
- Round Shot may continue visually through ricochets after its primary resolved impact. Those bounces are currently presentation only.
- Shell/Shrapnel/Canister use temporary debug-style presentation until final meshes/Niagara/audio are added.
- P follows the newest live non-canister projectile. WASD, camera rotation or zoom cancel Follow Shot and restore the pre-follow view.
- F9 shows/hides the full ballistic trajectory for QA.
- Projectile history is bounded and available through the shared presentation snapshot.
- The campaign→battlefield header is a **contract only**, not a generator. It already reserves campaign coordinates, deterministic seed, attacker/defender approaches and source features for height, hills, ridges, depressions, rivers, roads, bridges, settlements, forest, fields, marsh and water.
- All UE-P360..379 remain **UE 5.8.3 build/runtime QA pending**.


# 19. Shared human animation + uniform customization core — v00.02.63

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P380 | P0 | Shared human visual profile | Common skeleton/animation-set/profile IDs | IMPLEMENTED CORE |
| UE-P381 | P0 | Canonical human animation actions | Idle/Walk/Run/Aim/Fire/Reload/Die/Mount/Dismount + stance/mounted states | IMPLEMENTED CORE |
| UE-P382 | P0 | Reusable locomotion intent | Unit movement derives shared walk/run/mounted locomotion state | IMPLEMENTED CORE |
| UE-P383 | P1 | Animation manifest | Canonical human/mounted/horse/artillery animation names | IMPLEMENTED CORE |
| UE-P384 | P0 | Uniform colour zones | Coat/Trousers/Facings/Headgear/Leather/Accent/Metal | IMPLEMENTED CORE |
| UE-P385 | P0 | Uniform preset data | Nation/unit/regiment preset identity + base palette | IMPLEMENTED CORE |
| UE-P386 | P0 | Runtime uniform overrides | Independent colour-zone overrides without mesh/animation duplication | IMPLEMENTED CORE |
| UE-P387 | P0 | Dynamic material application | Material parameters applied to selected skeletal mesh components | IMPLEMENTED CORE |
| UE-P388 | P1 | Equipment socket mapping | Rifle/Bayonet/Sabre/Tool/Backpack/CartridgeBox sockets | IMPLEMENTED CORE |
| UE-P389 | P0 | Rig compatibility validation | Enforces shared skeleton + animation-set contract | IMPLEMENTED CORE |
| UE-P390 | P1 | Horse gait animation state | Idle/Walk/Trot/Canter/Gallop from actual movement | IMPLEMENTED CORE |
| UE-P391 | P1 | Rider/horse animation sync | Mounted rider action follows horse gait | IMPLEMENTED CORE |
| UE-P392 | P1 | Dragoon mount/dismount intents | Existing mounted-state transitions emit shared Mount/Dismount actions | IMPLEMENTED CORE |
| UE-P393 | P0 | Artillery crew roles/stations | Gunner/Loader/Rammer/Sponger/Ammo/Wheels/Driver/HorseHandler | IMPLEMENTED CORE |
| UE-P394 | P0 | Artillery reload drill animation state | Recoil→Sponge→Charge→Projectile→Ram→Prime→Clear→Return | IMPLEMENTED CORE |
| UE-P395 | P0 | Artillery work-state animation intents | Push/Traverse/Limber/Unlimber/Repair map from authoritative battery state | IMPLEMENTED CORE |
| UE-P396 | P0 | Battery crew controller integration | Crew animation controller attached to every artillery battery | IMPLEMENTED CORE |
| UE-P397 | P1 | QA uniform presets | Editable neutral/Danish/Prussian/artillery QA palettes | IMPLEMENTED QA CORE |
| UE-P398 | P1 | Visual snapshot diagnostics | Animation/rig/preset/colours/equipment/gait/crew state exposed | IMPLEMENTED QA CORE |
| UE-P399 | P0 | Visual-core regression | Rig compatibility, manifests, runtime colour override, cavalry sync and crew stations | IMPLEMENTED QA CORE |

### v00.02.63 authority notes

- Animation assets belong to the shared human/horse animation systems, not to a particular uniform mesh.
- A new compatible soldier mesh should reuse `SK_Human_1864` + `ABP_Human_1864` rather than duplicating Walk/Run/Fire/Reload/Mount/Dismount.
- Uniform colour variation is material/preset data. It does not create a new skeleton or animation set.
- Historical palette locking is supported, but QA presets remain deliberately editable placeholders until historical visual tuning.
- Cavalry horse and rider remain separate skeletal meshes; rider locomotion is synchronized to horse gait.
- Artillery crew roles are visual stations driven by the authoritative battery state. Animation never decides whether a gun is loaded, traversed or deployed.
- Artillery reload visuals are phase-mapped onto the existing authoritative reload timer.
- Final skeletal meshes, Animation Blueprints, montages and material assets are still pending the later graphics pass.
- All UE-P380..399 remain **UE 5.8.3 build/runtime QA pending**.


# 20. Runtime QA visibility bootstrap — v00.02.64

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P400 | P0 | QA battlefield runtime visibility | OOB scenario draws side-coloured unit boxes + facing arrows independent of final assets | IMPLEMENTED QA CORE |
| UE-P401 | P0 | QA labels | Runtime labels forced visible/readable and coloured by side at operational test zoom | IMPLEMENTED QA CORE |
| UE-P402 | P0 | River/bridge/obstacle visibility | River banks, bridge crossing and navigation obstacle receive runtime debug geometry | IMPLEMENTED QA CORE |
| UE-P403 | P0 | Terrain fixture visibility | Hill/ridge/depression footprints + vertical height markers drawn from authoritative terrain fixtures | IMPLEMENTED QA CORE |
| UE-P404 | P0 | QA camera bootstrap | GameMode focuses strategy camera on battlefield centre and starts at operational overview zoom | IMPLEMENTED QA CORE |

### v00.02.64 notes

- The repository still contains no committed tactical `.umap`/`.uasset` battlefield scene; editor startup may therefore show an `Untitled` world.
- QA actors are created during **Play/PIE** by the existing C++ GameMode/OOB scenario.
- Runtime QA geometry is debug/presentation-only and never becomes movement, combat, LOS, terrain or command authority.
- Final soldier, cavalry, artillery, wagon, terrain and VFX assets remain separate later graphics work.
- UE-P400..404 require UE 5.8.3 local build/runtime verification after sync.


# 21. Physical QA placeholder presentation — v00.02.65

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P407 | P0 | Physical unit placeholder | Every StrategyUnit owns an Engine BasicShapes cube mesh for asset-independent runtime visibility | IMPLEMENTED QA CORE |
| UE-P408 | P0 | Echelon-specific placeholder footprint | Company/HQ/Cavalry/Artillery/Supply placeholders use distinct scales | IMPLEMENTED QA CORE |
| UE-P409 | P1 | Side-tint attempt | Dynamic material tries common Color/BaseColor parameters; coloured debug overlay remains fallback | IMPLEMENTED QA CORE |
| UE-P410 | P1 | Tactical camera readability | QA bootstrap starts closer at approx. 180 m spring-arm | IMPLEMENTED QA CORE |
| UE-P411 | P1 | Compact company labels | Operational QA company labels reduced to side/company identity | IMPLEMENTED QA CORE |

UE-P407..411 are presentation-only and require UE 5.8.3 build/runtime verification.


# 22. Specialist detachment architecture — v00.02.68

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P412 | P0 | Specialist detachment type model | Skirmisher / Marksman / PioneerWorkingParty / AmmunitionParty / StretcherParty / PicketScout | IMPLEMENTED CORE |
| UE-P413 | P0 | Organic-parent identity | Every detachment stores the stable organic parent unit ID | IMPLEMENTED CORE |
| UE-P414 | P0 | Detachment records | Strength, ammo, cohesion, anchor and active state are authoritative records | IMPLEMENTED CORE |
| UE-P415 | P0 | Parent strength reservation | Detached men are excluded from parent firing strength | IMPLEMENTED CORE |
| UE-P416 | P0 | Ammunition reservation | Ammo transfers from parent inventory to detachment and surviving ammo returns on recall | IMPLEMENTED CORE |
| UE-P417 | P1 | Tactical task anchor | Each active detachment owns a task/world anchor | IMPLEMENTED CORE |
| UE-P418 | P0 | Detachment casualties | Losses reduce both detachment current strength and parent authoritative strength | IMPLEMENTED CORE |
| UE-P419 | P0 | Recall/reform reconciliation | Recall closes detachment without duplicating strength/ammo | IMPLEMENTED CORE |
| UE-P420 | P1 | Detachment limits | Concurrent count + maximum detached fraction prevent runaway micro-unit creation | IMPLEMENTED CORE |
| UE-P421 | P0 | Combat integration | Infantry volley budget uses available parent strength after detachments | IMPLEMENTED CORE |

All UE-P412..421 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 23. NCO command continuity — v00.02.69

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P422 | P0 | NCO strength | `NCOStrength` is explicit local cadre state | IMPLEMENTED CORE |
| UE-P423 | P0 | NCO quality | `NCOQuality` is independent from officer stats | IMPLEMENTED CORE |
| UE-P424 | P0 | Officer availability fallback | Local NCO cadre can preserve command when officer becomes unavailable | IMPLEMENTED CORE |
| UE-P425 | P0 | Command continuity score | Strength + quality + officer availability resolve bounded local continuity | IMPLEMENTED CORE |
| UE-P426 | P0 | Formation speed effect | NCO continuity modifies movement/reform execution speed | IMPLEMENTED CORE |
| UE-P427 | P0 | Reload discipline effect | NCO continuity modifies infantry reload discipline | IMPLEMENTED CORE |
| UE-P428 | P1 | Rally effect | Routed recovery consumes NCO rally multiplier | IMPLEMENTED CORE |
| UE-P429 | P1 | Local response delay | Command delay includes receiving-unit NCO response factor | IMPLEMENTED CORE |
| UE-P430 | P1 | Detachment control | Working/specialist-party work rate uses NCO control multiplier | IMPLEMENTED CORE |
| UE-P431 | P0 | Unit/QA integration | Every StrategyUnit owns NCO component; specialist regression validates state | IMPLEMENTED QA CORE |

All UE-P422..431 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 24. Firing drill and explicit kneeling stance — v00.02.70

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P432 | P0 | Kneeling stance | `Standing / Kneeling / Prone` tactical stance enum | IMPLEMENTED CORE |
| UE-P433 | P0 | Kneeling movement | Kneeling has explicit movement multiplier | IMPLEMENTED CORE |
| UE-P434 | P0 | Kneeling target profile | Kneeling has intermediate incoming-hit profile | IMPLEMENTED CORE |
| UE-P435 | P0 | Loading-method model | MuzzleLoader vs BreechLoader is explicit fire-drill state | IMPLEMENTED CORE |
| UE-P436 | P0 | Drill modes | FrontRank / Volley / Independent / AlternatingSections / KneelingFrontRank | IMPLEMENTED CORE |
| UE-P437 | P0 | Eligible firing fraction | Drill mode + stance determine participating fraction | IMPLEMENTED CORE |
| UE-P438 | P0 | Muzzle-loader low-stance penalty | Prone muzzle-loading lowers participation and increases reload time | IMPLEMENTED CORE |
| UE-P439 | P0 | Breech-loader low-stance advantage | Breech-loading remains substantially more usable from low stance | IMPLEMENTED CORE |
| UE-P440 | P1 | Volley coordination | Fire discipline/drill quality modifies volley coordination accuracy | IMPLEMENTED CORE |
| UE-P441 | P0 | Combat/reload integration | Combat shot budget and reload consume drill + stance + NCO modifiers | IMPLEMENTED CORE |

All UE-P432..441 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 25. Physical defensive positions — v00.02.71

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P442 | P0 | Typed fieldworks | RiflePit / Breastwork / Trench / GunEmplacement / Barricade / AbatisObstacle / Redoubt | IMPLEMENTED CORE |
| UE-P443 | P0 | Physical defensive actor | `AStrategyDefensivePosition` is persistent battlefield state | IMPLEMENTED CORE |
| UE-P444 | P0 | Orientation + footprint | Length, depth and facing define physical protection geometry | IMPLEMENTED CORE |
| UE-P445 | P0 | Condition | Position condition degrades under structural damage | IMPLEMENTED CORE |
| UE-P446 | P1 | Capacity | Defensive positions limit supported occupying manpower | IMPLEMENTED CORE |
| UE-P447 | P0 | Ownership/occupier | Owning side and current occupying unit are separate states | IMPLEMENTED CORE |
| UE-P448 | P0 | Directional protection | Frontal/rear multipliers depend on shooter direction and position facing | IMPLEMENTED CORE |
| UE-P449 | P0 | Structural damage/repair | Positions can be damaged, breached and repaired | IMPLEMENTED CORE |
| UE-P450 | P0 | Capture/vacate | Empty usable positions can change side; occupants can leave | IMPLEMENTED CORE |
| UE-P451 | P0 | Navigation obstacle lifecycle | Position owns, refreshes and cleans its navigation obstacle | IMPLEMENTED CORE |

All UE-P442..451 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 26. Fortification assault and breach equipment — v00.02.72

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P452 | P0 | Assault phase machine | Idle → Preparing → Approaching → Breaching → AssaultReady / Failed | IMPLEMENTED CORE |
| UE-P453 | P1 | Ladders | Explicit ladder inventory/capability | IMPLEMENTED CORE |
| UE-P454 | P1 | Planks/storm boards | Gap-crossing equipment state | IMPLEMENTED CORE |
| UE-P455 | P1 | Axes | Breach-tool inventory | IMPLEMENTED CORE |
| UE-P456 | P1 | Crowbars | Breach-tool inventory | IMPLEMENTED CORE |
| UE-P457 | P1 | Explosive charges | Consumable high-effect breach equipment | IMPLEMENTED CORE |
| UE-P458 | P0 | Working-party strength | Assault preparation/breach rate depends on assigned workers | IMPLEMENTED CORE |
| UE-P459 | P0 | Equipment-driven timing | Better equipment reduces preparation and breach time | IMPLEMENTED CORE |
| UE-P460 | P1 | Exposure/crossing modifiers | Equipment quality changes assault exposure/crossing difficulty | IMPLEMENTED CORE |
| UE-P461 | P0 | Breach result | Successful breach applies structural damage and breached state | IMPLEMENTED CORE |

All UE-P452..461 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 27. Mortar deployment and battery core — v00.02.73

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P462 | P0 | Mortar class | `LightHand` and `HeavySiege` classes | IMPLEMENTED CORE |
| UE-P463 | P0 | Mortar battery actor | `AStrategyMortarBatteryUnit` extends artillery tactical unit | IMPLEMENTED CORE |
| UE-P464 | P0 | Transport state | Mortar begins/returns to transport state | IMPLEMENTED CORE |
| UE-P465 | P0 | Emplacing state | Timed emplacing blocks normal movement/fire | IMPLEMENTED CORE |
| UE-P466 | P0 | Deployed state | Deployed state gates mortar fire | IMPLEMENTED CORE |
| UE-P467 | P0 | Packing state | Timed packing returns battery to transport | IMPLEMENTED CORE |
| UE-P468 | P1 | Working-party deployment rate | Assigned work strength modifies emplace/pack speed | IMPLEMENTED CORE |
| UE-P469 | P0 | Horse/wagon transport | Horse + wagon availability scale transport mobility | IMPLEMENTED CORE |
| UE-P470 | P1 | Piece weight/manhandling | Heavy weight strongly limits short-distance manhandling factor | IMPLEMENTED CORE |
| UE-P471 | P0 | Movement + QA fixture integration | Movement executor respects mortar state; heavy Danish QA mortar spawns | IMPLEMENTED QA CORE |

All UE-P462..471 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 28. Mortar fire mission core — v00.02.74

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P472 | P0 | Mortar bomb inventory | Separate bomb ammunition + maximum inventory | IMPLEMENTED CORE |
| UE-P473 | P0 | Unit target mission | Enemy unit can be assigned when inside mortar range | IMPLEMENTED CORE |
| UE-P474 | P0 | Area target mission state | Visible world point can be stored as area fire target | IMPLEMENTED CORE |
| UE-P475 | P0 | Fortification target mission | Defensive position can be assigned directly | IMPLEMENTED CORE |
| UE-P476 | P0 | Mortar min/max range | High-angle weapon uses explicit minimum and maximum range | IMPLEMENTED CORE |
| UE-P477 | P1 | High-arc solution | Deterministic high-arc apex calculation for later presentation | IMPLEMENTED CORE |
| UE-P478 | P1 | Time-of-flight estimate | Distance-based bounded flight-time estimate | IMPLEMENTED CORE |
| UE-P479 | P0 | Reload cadence | Mortar reload timer owns fire cadence | IMPLEMENTED CORE |
| UE-P480 | P0 | Unit/fortification resolution | Bombs can cause infantry casualties or structural fortification damage | IMPLEMENTED CORE |
| UE-P481 | P0 | Hold/resupply/automatic cadence | Hold Fire, bomb resupply and deployed auto-fire loop | IMPLEMENTED CORE |

Area-target **state** is implemented; wider area-effect search/fragment resolution remains a later tuning/expansion item. All UE-P472..481 require UE 5.8.3 build/runtime QA.

# 29. Defensive-position occupancy and combat integration — v00.02.75

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P482 | P0 | Occupy position | Unit explicitly occupies a valid defensive position | IMPLEMENTED CORE |
| UE-P483 | P0 | Leave position | Occupancy is released cleanly before reassignment | IMPLEMENTED CORE |
| UE-P484 | P1 | Nearest usable search | Unit can query nearest compatible free position | IMPLEMENTED CORE |
| UE-P485 | P1 | Auto-occupy | Convenience path occupies nearest usable position inside radius | IMPLEMENTED CORE |
| UE-P486 | P0 | Infantry cover integration | Small-arms hit resolution consumes occupied-position protection | IMPLEMENTED CORE |
| UE-P487 | P0 | Artillery cover integration | Direct artillery casualty resolution consumes occupied-position protection | IMPLEMENTED CORE |
| UE-P488 | P1 | Gun-emplacement capability | GunEmplacement/Redoubt identify artillery-compatible defensive works | IMPLEMENTED CORE |
| UE-P489 | P1 | Threat-facing query | Position can report whether facing is useful against threat direction | IMPLEMENTED CORE |
| UE-P490 | P1 | Condition/status query | Unit/UI/QA can read position condition/breached state | IMPLEMENTED CORE |
| UE-P491 | P0 | Fieldworks auto-occupancy | Newly completed hasty position is physically created and occupied | IMPLEMENTED CORE |

All UE-P482..491 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 30. Working parties, ammunition and casualty support — v00.02.76

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P492 | P0 | Working-party task state | CarryAmmo / Stretcher / Dig / Breach / Repair tasks | IMPLEMENTED CORE |
| UE-P493 | P0 | Worker allocation | Assigned worker count is bounded by available workers | IMPLEMENTED CORE |
| UE-P494 | P0 | Work-rate model | Worker strength + NCO control determine work rate | IMPLEMENTED CORE |
| UE-P495 | P0 | Ammunition load | Party can carry explicit small-arms ammunition quantity | IMPLEMENTED CORE |
| UE-P496 | P0 | Ammunition delivery | Carried rounds transfer into target unit combat inventory | IMPLEMENTED CORE |
| UE-P497 | P1 | Wounded queue | Wounded awaiting collection are persistent party state | IMPLEMENTED CORE |
| UE-P498 | P1 | Stretcher collection | Collection capacity scales with assigned workers | IMPLEMENTED CORE |
| UE-P499 | P1 | Position repair | Working party can repair damaged defensive position | IMPLEMENTED CORE |
| UE-P500 | P1 | Dig fieldworks hook | Completed Dig task starts engineer-rate hasty-fieldworks construction | IMPLEMENTED CORE |
| UE-P501 | P1 | Breach-obstacle hook | Completed Breach task damages target position and marks severe damage breached | IMPLEMENTED CORE |

All UE-P492..501 are **code implemented / UE 5.8.3 build+runtime QA pending**.

# 31. Specialist persistence, validation and QA — v00.02.77

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P502 | P0 | Specialist snapshot struct | Unit/detachment/NCO/stance/drill/working-party state captured together | IMPLEMENTED QA CORE |
| UE-P503 | P0 | Snapshot capture | Current specialist state can be captured deterministically | IMPLEMENTED QA CORE |
| UE-P504 | P0 | Snapshot restore | Specialist state can be restored to same stable unit ID | IMPLEMENTED QA CORE |
| UE-P505 | P1 | Active task restore | Restored working-party task re-enables component tick | IMPLEMENTED QA CORE |
| UE-P506 | P0 | State validation | Strength, NCO and working-party invariants expose failure reason | IMPLEMENTED QA CORE |
| UE-P507 | P1 | Deterministic digest | Stable specialist state produces repeatable QA digest | IMPLEMENTED QA CORE |
| UE-P508 | P1 | Specialist reset | Detachments/NCO/stance/drill/working-party state can reset to baseline | IMPLEMENTED QA CORE |
| UE-P509 | P0 | Detached-strength leak check | Validator detects detached strength exceeding parent strength | IMPLEMENTED QA CORE |
| UE-P510 | P0 | NCO/working-party sanity checks | Negative/out-of-range state is explicitly rejected | IMPLEMENTED QA CORE |
| UE-P511 | P0 | QA scenario integration | First Danish company carries marksman/NCO/drill fixture; all units validate specialist components | IMPLEMENTED QA CORE |

All UE-P502..511 are **code implemented / UE 5.8.3 build+runtime QA pending**. None is parity verified until local UE 5.8.3 compile and scenario QA have passed.

# 32. Livgarden 1864 animated infantry import and runtime visual — v00.02.78

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P512 | P0 | Livgarden source staging | Canonical SourceAssets layout for body mesh, rifle, bayonet rifle and animation FBXs | IMPLEMENTED CORE |
| UE-P513 | P0 | Shared-skeleton preflight | 41 common Mixamo motion-bone names verified across body and all 65 animation FBXs; Unreal remains final authority | IMPLEMENTED QA CORE |
| UE-P514 | P0 | Automated skeletal import | Editor Python imports the Livgarden FBX as canonical Skeletal Mesh + Skeleton | IMPLEMENTED CORE |
| UE-P515 | P0 | Shared animation import | All 65 FBX clips import against the same Livgarden Skeleton with deterministic A_* asset names | IMPLEMENTED CORE |
| UE-P516 | P0 | Rifle asset import | Rifle and rifle+bajonet GLB sources import to canonical weapon Static Mesh paths | IMPLEMENTED CORE |
| UE-P517 | P0 | 1:1 company renderer | Company visual component can render one skeletal soldier per authoritative living man | IMPLEMENTED CORE |
| UE-P518 | P1 | Reduced visual scale sampling | 1:2 / 1:5 / 1:10 render scales sample across the complete authoritative formation footprint | IMPLEMENTED CORE |
| UE-P519 | P0 | Formation + weapon binding | Rendered soldiers consume existing formation slots and attach current rifle variant to the right-hand bone | IMPLEMENTED CORE |
| UE-P520 | P0 | Combat/stance animation mapping | Walk/run/aim/fire/reload, Standing/Kneeling/Prone, bayonet charge and death have first-pass runtime mappings | IMPLEMENTED CORE |
| UE-P521 | P0 | QA company activation + editor verification | First Danish QA company can enable real infantry visuals; UE 5.8.3 import/build/runtime/weapon-offset QA must pass | QA PENDING |

Standing reload has no clearly named dedicated clip in the supplied package, so the first-pass visual falls back to standing aim while gameplay reload timing remains authoritative. UE-P512..521 are **source/runtime implemented; UE 5.8.3 asset import + build/runtime QA pending**.

# 33. Infantry fire-drill research progression — v00.02.79

| ID | Pri | Feature | Unreal implementation | Status |
| --- | --- | --- | --- | --- |
| UE-P522 | P0 | Fire-drill research levels | FrontRankFire / TwoRankFire / FireByRank / ControlledVolley / IndependentFire / AdvancedFireDrill | IMPLEMENTED CORE |
| UE-P523 | P0 | Front-rank campaign baseline | New fire-drill components default to FrontRank rather than Volley | IMPLEMENTED CORE |
| UE-P524 | P0 | Rank-aware participation | Firing fraction derives from formation RankCount instead of fixed 50% assumptions | IMPLEMENTED CORE |
| UE-P525 | P0 | Two-Rank Fire unlock | Up to the first two ranks can contribute once research/adoption reaches tier 1 | IMPLEMENTED CORE |
| UE-P526 | P0 | Fire by Rank unlock | One rank fires per pulse; active firing-rank index advances deterministically after resolved volley | IMPLEMENTED CORE |
| UE-P527 | P0 | Fire-by-rank cadence | Inter-rank pulse interval scales with actual rank count so a rank receives roughly one full reload cycle before firing again | IMPLEMENTED CORE |
| UE-P528 | P0 | Controlled Volley unlock | Existing Volley mode is research-gated and retains full-rank coordinated fire | IMPLEMENTED CORE |
| UE-P529 | P0 | Independent Fire unlock | Existing Independent mode is research-gated and remains available alongside older drills | IMPLEMENTED CORE |
| UE-P530 | P1 | Advanced Fire Drill capability | Final tier exposes automatic drill selection between shock volley, continuous fire and rank fire; campaign research/adoption owner still to bind | IMPLEMENTED CORE / CAMPAIGN BINDING PENDING |
| UE-P531 | P0 | Legacy/persistence/QA integration | Old AlternatingSections/KneelingFrontRank serialize safely as hidden aliases; research level persists in specialist snapshot and Danish QA fixture exercises FireByRank | IMPLEMENTED QA CORE |

Design rule: this progression is an intentional gameplay abstraction. Unlocking a drill does not grant mastery; DrillTraining, FireDiscipline, NCO quality, loading method, stance, fatigue/morale/cohesion and later campaign adoption/training remain separate quality layers. The current generic QA Line remains three ranks, so FrontRank currently represents 1/3 and TwoRankFire 2/3 of that formation. Rank-specific 1:1 soldier fire/reload animation masking is not claimed here and remains pending after the real infantry asset/import pass. All UE-P522..531 are **code implemented / UE 5.8.3 build+runtime QA pending**.

