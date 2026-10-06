# PROJECT 1864 — B-180–B-189: AI difficulty og v00.00.09 prioritering

**Status: BESLUTTET / AKTIV IMPLEMENTERING**  
**Designbaseline: v00.02.08**  
**Target prototype: P0A v00.00.09 TACTICAL COMMAND TEST**

## Prioritetsbeslutning

Officer-AI/delegeret kommando er første gameplay-implementation efter v00.00.08-baselinen. v00.00.09 er udvidet til en sammenhængende tactical-command test, så Officer AI kan afprøves med pause, større manøvrerum, directional fire, fire policy og commander doctrine/intention uden at trække de senere logistik-, FoW- og HQ-systemer ind endnu.

## B-180 — Symmetrisk officer-AI

**BESLUTTET / AKTIV.** Fjenden og spillerens delegerede formationer bruger samme `OfficerAIController` og samme officerprofil-model.

- Samme officer stats/personality inputs.
- Samme mission model.
- Samme morale/cohesion constraints.
- Samme doctrine/order-intent model.
- Samme fire-policy constraints.
- Samme senere knowledge-state/fog-of-war constraints.
- Ingen skjulte enemy-only tactical capabilities.

En dårlig fjendtlig officer forbliver dårlig på høj sværhedsgrad; difficulty omskriver ikke officerprofilen.

## B-181 — Difficulty er separat fra officer quality

**BESLUTTET.** Difficulty ændrer computerens ekstra decision efficiency/noise, ikke officerens faktiske stats og ikke combat-systemets tal.

I v00.00.09 er laget bevidst smalt:

- enemy reaction multiplier,
- enemy bounded decision noise.

Senere kan difficulty også påvirke theatre-level planning/coordination, men stadig uden omniscience eller skjulte combat buffs.

## B-182 — Første difficulty levels

**AKTIV v00.00.09.** Første prototype bruger:

- **Easy** — længere ekstra reaction delay og højere ekstra decision noise.
- **Normal** — referenceadfærd.
- **Hard** — kortere ekstra reaction delay og lavere ekstra decision noise.

Controls:

- `F6` Easy
- `F7` Normal
- `F8` Hard

Difficulty påvirker kun computerens preussiske side i denne Danmark-player P0A. Player-delegerede danske officerer bruger deres faktiske profil på referenceværdier uanset valgt enemy difficulty.

## B-183 — P0A v00.00.09 MVP scope

v00.00.09 implementerer nu en testbar tactical-command slice:

1. `AI UNIT: OFF/ON` på spillerstyrede danske regimenter via **`I`**.
2. Officerprofil pr. regiment med otte kerne-stats: Leadership, Inspiration, Tactical Skill, Initiative, Staff/Command Skill, Discipline/Obedience, Aggressiveness/Caution og Composure/Nerve.
3. Separat Officer Experience som stability/decision-noise input.
4. Missions: DefendArea, Hold, MoveToPoint, AttackTarget og AttackNearest.
5. Officerprofilen påvirker reaction delay, preferred engagement behaviour, mission adherence, stabilisation og decision noise.
6. Fjendens regimenter bruger samme shared controller.
7. Commander doctrine: Defensive / Balanced / Offensive.
8. Commander order aggression intent 0–100, med officerens egen Aggressiveness som dominerende input.
9. Directional 120° fire fan og fire policies HOLD/CLOSE/MEDIUM/LONG.
10. Continuous accuracy-by-range: jo tættere mål, jo større sandsynlighed for hits over mange salver.
11. Battlefield fordoblet til 360x240 med større startafstand og udvidede camera bounds.
12. Danmark er testforsvarere; Preussen testangribere.
13. Enemy AI bruger manoeuvre/preferred range og mindst én flank/approach mission i stedet for blot at løbe lige frem.
14. Easy / Normal / Hard ændrer decision layer, ikke combat stats.
15. HUD/command-menu viser AI state, officer, doctrine, order aggression, fire policy, task og reason.
16. `AI-DIAG` telemetry logger samme beslutningskontekst.
17. Eksisterende v00.00.08 combat/reload/hit-feedback/casualty-visual skal regressionsbevares.

## B-184 — Bevidste begrænsninger

Følgende er stadig ikke del af v00.00.09:

- fuld courier/order-delay simulation,
- brigade/battalion hierarchy,
- autonomy Strict/Normal/Independent selector i runtime,
- segmenteret `eligibleFiringFraction` over formationens frontage,
- fuld terrain/LOS obstruction,
- skirmisher deployment,
- prone/cover/fieldworks,
- ammunition logistics,
- advanced artillery fire-control,
- full fog-of-war sensor network,
- strategic campaign AI,
- historiske officer ratings.

## B-185 — Officer prototype-data

**AKTIV QA-data.** De fire regimenter har bevidst forskellige fiktive QA-profiler. De må ikke fortolkes som historiske vurderinger. Endelige officer stats skal komme fra historisk officer/OOB-data med provenance.

Detaljer: `B-190-OFFICER-STATS-MODEL.md` og v00.00.09 release notes.

## B-186 — Enemy AI validation

Acceptance-test skal verificere, at fjendens tropper ikke bare bruger en global perfekt AI eller en simpel "løb frem"-rutine.

8th Regiment og 18th Regiment har forskellige QA-profiler og skal over flere sammenlignelige beslutninger kunne vise forskellige reaction/task patterns på en forklarlig måde.

I aktuelle QA-scenario:

- begge preussiske regimenter er **angribere**,
- default doctrine = Offensive,
- default commander OrderAgg = 65,
- default fire policy = Medium,
- 18th får et særskilt flank/approach waypoint før reassessment,
- AI lukker til sin beregnede officer/order preferred range og deployer til line ved engagement,
- morale/composure kan få relevante officerer til at stabilisere frem for blindt at fortsætte.

## B-187 — No-cheat difficulty rule

**BESLUTTET.** Easy/Normal/Hard må ikke ændre:

- weapon accuracy,
- reload speed,
- weapon range,
- fire policy,
- morale values,
- cohesion values,
- casualty resistance,
- movement speed,
- ammunition quantity,
- regiment strength,
- officer stats,
- hidden map knowledge.

Hvis der senere tilbydes separate handicap-bonuses, skal de være eksplicit synlige custom settings og ikke blandes sammen med AI difficulty.

## B-188 — AI telemetry

**AKTIV v00.00.09.** AI-statusændringer logges kompakt som `AI-DIAG` med mindst:

- unit,
- team,
- officer,
- AI state,
- difficulty,
- doctrine,
- OrderAgg,
- EffectiveAgg,
- fire policy,
- mission,
- task,
- reason.

Senere udvides dette med perceived threat, score-breakdown og knowledge timestamp/confidence.

## B-189 — Release gate

v00.00.09 må først promoveres efter:

1. Ren Unity 6000.6.0f1 compile.
2. 360x240 battlefield og udvidet kamera virker.
3. Pause/x0.5/x1/x2/x5/x20 virker uden combat-regelændringer.
4. Directional fire fan følger facing og er ikke 360°.
5. HOLD/CLOSE/MEDIUM/LONG fire policy virker.
6. Closer-is-easier continuous accuracy kan observeres statistisk uden hårde range-band jumps.
7. Shared Officer controller er installeret på alle regimenter.
8. AI UNIT ON/OFF via `I` fungerer på danske regimenter uden WASD-konflikt.
9. Doctrine og OrderAgg-menu virker og UI-clicks går ikke igennem til battlefield.
10. Enemy AI bruger shared core, approach/preferred range og ikke legacy/simple straight-line loop.
11. Easy/Normal/Hard fungerer uden combat cheats.
12. Composure påvirker stress reaction/noise.
13. Reason codes/telemetry er brugbare.
14. Fuld v00.00.08 regression er ren.

Den fulde trin-for-trin test ligger i `docs/releases/P0A-v00.00.09-RELEASE-NOTES.md`.
