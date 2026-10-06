# P0A - Technical Notes

**Aktuel prototypebaseline: v00.00.08 TEST**  
**Designmanual: v00.02.08**  
**Unity baseline: 6000.6.0f1 (Unity 6.6), changeset `f7f8ed4d1e24`**  
**Detaljerede release notes:** `docs/releases/P0A-v00.00.08-RELEASE-NOTES.md`

## Formål

P0A beviser det taktiske minimums-loop:

`select -> order -> move -> acquire target -> fire -> casualties/morale -> rout -> battle result`

v00.00.08 udvider dette minimums-loop med mere observerbar combat state, uden endnu at introducere ammunition, casualty categories, LOS, cover, pathfinding eller combined arms.

## Arkitektur

- `PrototypeBootstrap`: bygger prototypeverdenen ved runtime uden eksterne assets.
- `Regiment`: authoritative regimentsimulation, weapon/reload/experience, volley resolution og 1:10 visualisering.
- `BattleManager`: clock, UI, time scale, unit labels, `Ramte N` feedback og victory state.
- `PrototypeCasualtyVisualManager`: observerer strength loss og opretter én ikke-authoritative liggende casualty-figur pr. regiment.
- `PlayerCommander`: selection og tactical orders.
- `RTSCameraController`: kamera med unscaled movement/rotation.
- `PrototypeProjectStartup` (Editor): opretter/åbner prototype-scenen og tilføjer den til Build Settings.

## Authoritative state vs. visual state

`CurrentStrength` er authoritative faktisk mandskab. Det synlige antal aktive soldater er cirka `ceil(CurrentStrength / 10)`.

Vigtige regler:

- Combat beregnes på regimentsniveau.
- En grafisk soldat er ikke en selvstændig combat-agent.
- `Ramte N` er UI-feedback, ikke en separat casualty database.
- Den nye liggende casualty-figur er kun visualisering og skriver ikke tilbage til simulation state.
- Senere killed/wounded/missing/captured state skal være authoritative data og må ikke udledes af antallet af liggende modeller.

## P0A v00.00.07 - valideret observation

- Runtime-bootstrap opretter slagmark og kamera korrekt i Play Mode.
- Battle-resultatet er terminal pause-state; `R` kan stadig restarte.
- RTS-kameraets pan/rotation bruger `Time.unscaledDeltaTime`.
- WASD/piletaster læses direkte.
- Musehjulszoom bruger fast step.
- Defensive guards findes for manglende `MainCamera`, dublet `BattleManager` og dublet-registering.

Den fulde smoke-test for alle controls blev ikke brugt som begrundelse for at markere v00.00.07 som endelig release; observationen dokumenterer specifikt, at bootstrap/rendering virker i den installerede Unity-version.

# P0A v00.00.08 TEST

## Weapon profiles

P0A bruger to midlertidige infantry weapon profiles:

| Weapon profile | Base reload | Effective range | Maximum range | Prototype accuracy |
| --- | ---: | ---: | ---: | ---: |
| Rifled muzzle-loader | 5,0 s | 43 | 55 | 0,014 |
| Dreyse needle rifle | 3,6 s | 37 | 49 | 0,013 |

Tallene bevarer den tidligere P0A-balance og er gameplay-/QA-tuning, ikke endelig historisk weapon-database.

## Experience og reload

`Experience` er en 0-100 regiment-state.

- 50 = neutral.
- 0 = 1,20x reload-tid.
- 100 = 0,80x reload-tid.
- Lineær interpolation mellem yderpunkterne.

Formel:

`CurrentReloadSeconds = BaseReloadSeconds × lerp(1.20, 0.80, Experience / 100)`

### QA-data

| Regiment | Weapon | Experience | Reload ca. |
| --- | --- | ---: | ---: |
| 1. Regiment | Rifled muzzle-loader | 55 | 4,90 s |
| 5. Regiment | Rifled muzzle-loader | 42 | 5,16 s |
| 8th Regiment | Dreyse | 65 | 3,38 s |
| 18th Regiment | Dreyse | 50 | 3,60 s |

Værdierne er bevidst varierede QA-data og skal senere komme fra OOB/unit-data.

## Timing model

Reload deadline sættes med skaleret Unity simulationstid:

`nextFireTime = Time.time + CurrentReloadSeconds × randomCadenceFactor`

Konsekvens:

- Pause stopper reload/combat.
- 2x/3x accelererer combat i realtid.
- Kameraet bruger fortsat unscaled tid og skal derfor ikke accelerere.

## Volley resolution

Den tidligere minimum-clamp på 1 er fjernet.

v00.00.08 kan resolve:

`0..16 hits`

- 0 hits reducerer ikke `CurrentStrength`.
- Shock kan stadig ramme morale/cohesion ved 0 hits.
- Positive hits reducerer foreløbigt `CurrentStrength` direkte.
- Senere casualty pipeline splitter resultatet i killed/wounded/missing/captured m.m.

## `Ramte N` feedback

Ved positive hits:

- `LastVolleyHits` gemmes på målet.
- feedback er aktiv i ca. 1,45 sekunder.
- `Time.unscaledTime` bruges, så tekstens læsetid ikke forkortes af 2x/3x.
- HUD viser `Ramte N` over målregimentet.
- 0-hit salver viser ingen tekst.

## Udvidede unit labels

Labelen viser nu:

- regimentnavn/team,
- CurrentStrength,
- weapon short name,
- Experience,
- beregnet reload,
- morale,
- cohesion,
- routed-state.

Dette er bevidst QA-observability og kan senere erstattes af den kontekstuelle command bar.

## Prototype casualty visual

Ny komponent:

`Assets/Scripts/PrototypeCasualtyVisualManager.cs`

### Lifecycle

1. Komponenten auto-oprettes efter scene-load.
2. Den venter på `BattleManager.Instance`.
3. Den observerer `BattleManager.Regiments`.
4. Den gemmer sidst observerede `CurrentStrength` pr. regiment.
5. Ved første observerede strength loss oprettes én liggende casualty-figur.
6. Regimentet markeres, så der ikke oprettes flere casualty-figurer for samme regiment i denne build.

### Robusthed

Hvis manageren første gang ser et regiment efter det allerede har mistet strength, sammenlignes med `InitialStrength`, så casualty-visual stadig kan oprettes.

### Visual

- primitive capsule/sphere/cube,
- team-farvet uniform,
- separat mørkt gevær,
- tilfældig lille position offset og rotation,
- ground height samples fra `PrototypeBootstrap.SampleGroundHeight`,
- colliders fjernes.

### Ikke implementeret

Casualty-figuren skelner ikke mellem:

- killed,
- wounded,
- unconscious,
- missing,
- POW.

Den har ingen AI, animation, physics interaction eller medical evacuation.

## Repository-metadata

GitHub-baselinen er stadig bevidst minimal. Under `ProjectSettings/` er kun `ProjectVersion.txt` versionsstyret, mens `Assets/Scenes/PrototypeBattle.unity`, `.meta`-filer, `Packages/packages-lock.json` samt øvrige Unity-genererede metadata kan være oprettet lokalt.

Disse filer må ikke slettes eller overskrives blindt. Efter v00.00.08-testen skal lokal `git status` inspiceres, og der træffes derefter særskilt beslutning om den permanente, reproducerbare Unity-baseline.

## Bevidste simplifikationer i v00.00.08

- Direkte bevægelse uden NavMesh/pathfinding.
- Ingen ordre-delay/couriers endnu.
- Ingen bataljonsunderopdeling endnu.
- Ingen ammunition eller supply.
- Resolved hits reducerer stadig strength direkte.
- Ingen killed/wounded/missing-pipeline endnu.
- Kun én repræsentativ casualty-figur pr. regiment.
- Ingen rigtig LOS/cover/smoke-simulation endnu.
- Ingen skirmishers/prone/fieldworks endnu.
- Ingen artilleri-/kavaleri-/supply-enheder endnu.
- Primitive visuals og immediate-mode HUD.
- Weapon/experience-værdier er prototype-tuning/QA-data.

## v00.00.08 release-gate

1. Ingen compiler errors eller blokkerende Console-fejl.
2. Weapon og Experience vises korrekt i unit labels.
3. Reload varierer korrekt med weapon profile og Experience.
4. Positive salver viser `Ramte N`.
5. 0-hit salver kan forekomme uden strength-tab.
6. Første strength loss opretter én liggende casualty-figur pr. regiment.
7. Casualty-figuren bliver ved tabsstedet og blokerer ikke selection/movement.
8. Selection/orders/formation/range virker.
9. Pause samt 1x/2x/3x virker.
10. Kameraet forbliver timeScale-uafhængigt.
11. Rout og battle outcome virker.
12. Victory/defeat kan ikke genoptages med Space/1/2/3.
13. `R` restarter korrekt.
14. Lokal Unity metadata-state inspiceres bagefter.

P0B påbegyndes først efter denne gate og efter repository-metadata er klassificeret.
