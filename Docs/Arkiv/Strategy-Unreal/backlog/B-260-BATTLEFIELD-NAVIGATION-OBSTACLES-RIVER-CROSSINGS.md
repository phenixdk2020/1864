# B-260–B-269 — Battlefield navigation, obstacles and river crossings

Status: **ACTIVE TEST / v00.00.09 tactical-command slice**

## B-260 — Physical tactical obstacles

Regiments may not pass through or finish deployed inside buildings, trees or other blocked battlefield objects. The prototype navigation layer treats farmhouse, barn, trees and fence posts as blocked tactical space and applies formation-dependent clearance.

Blocked player route waypoints are moved to the nearest valid deployment point rather than allowing the formation footprint to remain inside an obstacle.

## B-261 — River as non-traversable terrain

The prototype stream is a hard movement barrier for infantry. A unit may not simply walk through the water because its destination lies on the opposite bank.

Cross-bank movement must use a valid crossing:

1. existing fixed bridge;
2. later pontoon bridge built by engineer support;
3. other historically/data-defined ford or bridge if a scenario provides one.

No magical direct water crossing is allowed.

## B-262 — Fixed bridge routing

v00.00.09 TEST adds a fixed bridge at the road/stream crossing. If a movement order would cross the stream outside the bridge, the movement safety layer redirects the regiment through the bridge approaches.

A regiment crossing the bridge temporarily uses COLUMN to fit the narrow crossing and restores its previous formation after reaching the opposite bank.

## B-263 — Pontoon / engineer capability

Pontoon crossing is **not automatically available**. A formation or parent command must have appropriate engineer/pontoon capability, equipment, manpower and time.

Future implementation must model at minimum:

- engineer attachment/capability;
- pontoon/bridge equipment availability;
- selected crossing site;
- construction time;
- terrain/bank suitability;
- enemy fire/interference;
- damage/destruction and repair;
- traffic capacity and congestion.

Until that system exists, open water remains blocked and the fixed bridge is the only prototype crossing.

## B-264 — Destination validation

Movement destination and waypoint positions must be validated against blocked terrain. Invalid positions are rejected or moved to a nearby valid point with sufficient formation clearance.

The final destination footprint must never intentionally deploy a regiment through the geometry of a house, tree or other hard obstacle.

## B-265 — Local obstacle avoidance

A direct movement leg that intersects a static obstacle is rerouted locally around the obstacle instead of allowing the regiment centre to pass through it. Later versions should replace the prototype steering layer with terrain-aware pathfinding/cost fields at larger battlefield scale.

## B-266 — Friendly frontage deconfliction

When multiple AI-controlled regiments attack the same target, later arrivals receive distinct lateral frontage slots beside the first engager. Once a secondary regiment reaches its slot, it holds that local frontage and faces/fires on the target instead of receiving a centre-seeking attack order that would collapse the formations into each other.

## B-267 — Diagnostics

Prototype diagnostics:

- `NAV-DIAG` — blocked destination adjustment, waypoint correction and bridge routing.
- `AI-SPACING` — shared-target frontage slot assignment.

## Acceptance

- Infantry cannot cross the stream outside the bridge.
- Cross-river movement routes through the bridge.
- Bridge crossing uses column and restores previous formation afterward.
- A destination inside farmhouse/barn/tree/fence blocked space is moved to valid terrain.
- A direct route through a hard object detours around it.
- Two or more AI regiments engaging the same target form separate firing frontages rather than stacking on the same point.
- Pontoon crossing remains unavailable until engineer capability is explicitly implemented.
