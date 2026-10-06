# PROJECT 1864 — parallel development tracks

## Purpose

PROJECT 1864 udvikles nu i to uafhængige spor, så tactical stabilization ikke blokerer campaign/strategic development.

## Track A — Tactical

- Branch: `test` / aktiv leveringskanal `channel-test`
- Lokal standardmappe: `%USERPROFILE%\OneDrive\Strategy-Test`
- Aktuel revisionsserie: `v00.00.09f`.
- Fokus: battlefield controls, formations, Officer AI, combat, melee, navigation, obstacles, river/bridge, UI og tactical QA.
- `v00.00.09f` beholder **Navigation V3** som eneste autoritative movement/steering-lag. V4 og 09A er runtime-deaktiverede.
- `09B` er igen aktiv, men kun som en ikke-steerende kompatibilitetsregel der fjerner individuelle decorative trees fra navigation og nulstiller stale V3 tree-detour state.
- Tactical obstacle-baseline: decorative trees er pass-through; bygninger og fence posts er diskrete forhindringer; river/water er fortsat bridge-only.
- `v00.00.09f` korrigerer 09e-UI: den permanente `VALGT xN` selected-summary-boks skjules helt. Hover-info forbliver kort/transient og bottom command bar bevares.
- Range fans vises fortsat kun på valgte enheder, men 09f undgår dobbelt alpha-multiplikation, så ghost-conen igen er tydeligt synlig uden at blive opak.
- Primær QA: alle fire bevægende/angribende regimenter skal lave kontinuerlig fremdrift; Console må ikke længere spamme `NAV-V3|...|Obstacle=Tree`; V4/09A må ikke styre; range-conen skal være synlig; `VALGT xN`-panelet skal være væk.

## Track B — Campaign

- Branch: `campaign`
- Lokal standardmappe: `%USERPROFILE%\OneDrive\Strategy-Campaign`
- Aktuel revisionsserie starter ved `v00.00.10a`.
- Fokus: campaign scene/map, strategic formations, movement/time, battle trigger, tactical handoff og state return.
- Campaign-kode/scener skal så vidt muligt ligge separat fra tactical navigation/combat-filer for at minimere merge-konflikter.

## Track C — Unreal Port

- Branch: `unreal-port`
- Unreal project root inside repository: `/Unreal`
- Reference baseline: current `channel-test` Unity tactical implementation.
- Fokus: engine-port med feature parity før ny feature-udvikling.
- Unity-koden beholdes som reference under porten; eksisterende designregler, OOB, AI authority, formationsdoktrin, cavalry, river/bridge rules og QA-kriterier må ikke forsimples lydløst.
- Port-status og rækkefølge dokumenteres i [UNREAL-MIGRATION-PLAN.md](UNREAL-MIGRATION-PLAN.md) og [UNREAL-PORTING-INVENTORY.md](UNREAL-PORTING-INVENTORY.md).
- Første Unreal-scaffold indeholder projekt/module, grund-unit, order component og separat horse/rider cavalry-actor.

## Stable

- Branch: `main`
- Lokal standardmappe: `%USERPROFILE%\OneDrive\Strategy`
- Kun accepterede/promoverede gates.

## Integration rule

Campaign må ikke vente på mindre tactical polish. Kun tactical fixes der er nødvendige for campaign-state continuity eller som er accepteret gennem QA, integreres senere i campaign-sporet. Ingen destructive Git-operationer, force-push, reset --hard eller clean anvendes.
