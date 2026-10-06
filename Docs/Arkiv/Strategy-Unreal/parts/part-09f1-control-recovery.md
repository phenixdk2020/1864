# PROJECT 1864 – Tactical control/recovery supplement v00.00.09f1

## Purpose
This supplement keeps the accepted v00.00.09f tactical/combat baseline from the reference video and backports only the control and movement corrections required by the 2026-09-13 QA recording.

## RTS box selection
- LMB drag beyond 9 px creates a marquee selection rectangle.
- Normal drag replaces the current selection.
- Shift+drag adds Danish regiments.
- Ctrl+drag toggles Danish regiments.
- Selection is based on the regiment tactical centre projected into screen space.
- Routed/enemy regiments are excluded.
- Box selection never writes movement or combat state.

## March formation policy
### Manual control / Officer AI OFF
- A PlayerCommander route uses COLUMN while the active waypoint is more than 14 m away and no enemy is inside the deployment zone.
- Deployment begins when either:
  - active waypoint distance <= 14 m, or
  - nearest enemy distance <= EffectiveRange + 12 m.
- The route final formation is restored in the deployment zone; current baseline final formation is LINE.

### Officer AI ON
- Attack approach uses COLUMN while the nearest enemy is outside EffectiveRange + 12 m.
- MoveToPoint uses COLUMN while destination distance > 16 m and the nearest enemy is outside the deployment zone.
- AI deploys to LINE inside the deployment zone.

## Stall recovery
- Manual routes are sampled every 0.25 s.
- Useful progress threshold: 0.45 m.
- Stall threshold: 3.5 s without useful progress while >5 m from current waypoint.
- Recovery inserts a controlled bypass waypoint into the existing PlayerCommander route.
- Candidates are scored by route distance plus Navigation V3 path penalties.
- Unintended river-crossing bypasses are rejected.
- Maximum four inserted recovery waypoints per active route.
- Recovery cooldown: 4 s.
- Navigation V3 remains the local steering authority.

## Obstacle semantics
- Tree: soft/pass-through decorative obstacle.
- FencePost: soft/pass-through decorative obstacle.
- Farmhouse: hard tactical obstacle.
- Barn: hard tactical obstacle.
- River/water: hard terrain, bridge-only crossing.

The design intent is that a visual fence row should not become an invisible chain of overlapping circular hard obstacles. Fence effects can later be represented as a movement/cohesion penalty or segment/area obstacle rather than one hard obstacle per post.

## Retained v00.00.09f baseline
- 120-degree forward Close/Medium/Long range cone remains unchanged.
- Range-cone alpha fix from 09f remains unchanged.
- Movement V3 remains authoritative.
- Navigation V4 and 09A remain disabled at runtime.
- Existing Officer AI, combat, ammunition, melee, morale/cohesion, waypoint, multi-unit and facing systems remain unchanged except for the approach-formation policy above.

## QA telemetry
Expected prefixes:
- `SELECT-09F1`
- `MANUAL-FORMATION-09F1`
- `MANUAL-ROUTE-09F1`
- `FORMATION-09F1-AI`
- `NAV-SOFT-09F1`

## Acceptance gate
1. No red compile errors in Unity 6000.6.0f1.
2. Build marker reads `PROJECT 1864 | v00.00.09f1 TEST`.
3. LMB marquee can select multiple Danish regiments.
4. Long player orders switch regiments to Column.
5. Manual formations deploy by 14 m from destination or EffectiveRange + 12 m from enemy.
6. Long AI approaches use Column and deploy near contact.
7. Trees and FencePost objects do not stop a regiment.
8. Farmhouse/Barn still produce a detour.
9. River remains bridge-only.
10. A genuine manual stall after 3.5 s produces a bounded bypass recovery attempt.
11. The v00.00.09f 120-degree range cone remains visible.

## Non-blocking technical debt
- Legacy Unity 6.6 `CS0618` warnings from `FindObjectsByType<T>(FindObjectsSortMode)` remain separate cleanup work.
- Legacy Input Manager deprecation warning remains separate Input System migration work.
