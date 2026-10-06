# PROJECT 1864 — Unreal porting inventory

This inventory tracks what exists in the Unity tactical prototype and the intended Unreal destination.

| Unity/reference area | Unreal destination | Initial status |
|---|---|---|
| `Regiment.cs` | `AStrategyUnit` + infantry-specific actor/components | Planned |
| Regiment hierarchy F27 | OOB/command hierarchy data + components | Planned |
| Major HQ | Physical HQ Pawn/Actor | Planned |
| Regimental HQ | Physical HQ Pawn/Actor | Planned |
| Brigade/Division HQ F30B | Higher HQ actors + command components | Planned |
| Manual order authority F18 | `UOrderComponent` authority model | Scaffold started |
| Attack/Forsvar/Hold | `EStrategyOrderType` + order executors | Scaffold started |
| March Column F6 | `UFormationComponent` / formation executor | Planned |
| Line/Square | Formation component + animation/slot generation | Planned |
| Company slot deconfliction | Formation planner | Planned |
| Final-slot arrival F30V | Shared movement completion authority | Planned |
| Navigation V3 | Navigation/path execution service | Planned |
| River/bridge routing | Route planner + bridge transaction | Planned |
| Fire ranges/cones | Combat/visibility components + debug draw | Planned |
| LOS/contact | Visibility/contact subsystem | Planned |
| Volley/reload | Combat state machine | Planned |
| Casualties/morale | Unit combat state/data | Planned |
| Under-fire reaction F26 | AI/reaction component | Planned |
| Infantry Square F29 | Formation + anti-cavalry reaction | Planned |
| Cavalry core F30 | `ACavalryUnit` | Scaffold started |
| Cavalry AI F30C+ | Cavalry AIController/behaviour logic | Planned |
| Dynamic task attachment | Command-parent state/data | Planned |
| Dragon mounted/dismounted | Cavalry specialization | Planned |
| OOB F29Q/F30D | UMG OOB widget | Planned |
| Semantic zoom/NATO | Tactical presentation subsystem | Planned |
| HQ command zones | Debug/gameplay visualization component | Planned |
| Unity infantry FBX | Unreal Skeletal Mesh import | Source asset present |
| Horse + rider | Separate horse/rider Skeletal Meshes | Planned |
| Unity design manual | Engine-neutral source of truth | Preserved |

## Rule

A system is not marked **Ported** merely because an Unreal class exists. It becomes **Parity verified** only when its documented Unity behaviour has passed the corresponding Unreal QA scenario.
