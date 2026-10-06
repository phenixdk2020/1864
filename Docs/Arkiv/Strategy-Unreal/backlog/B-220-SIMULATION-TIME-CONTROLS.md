# PROJECT 1864 — B-220–B-229: Simulationstid, pause og klokke

**Status: AKTIV / IMPLEMENTERET I P0A v00.00.09 TEST**  
**Designbaseline: v00.02.08**  
**Target prototype: P0A v00.00.09 TEST**

## B-220 — Fælles simulation clock

**BESLUTTET / IMPLEMENTERET.** BattleManager ejer den aktuelle battle time. P0A starter 1. februar 1864 kl. 10:20. Klokken vises permanent i topbaren.

Tiden er simulationstid og skal ikke være bundet til wall-clock. Valgt speed skalerer simulationens forløb.

## B-221 — Time-control bar

**IMPLEMENTERET i v00.00.09 TEST.** Topbaren har klikbare controls:

`PLAY | PAUSE | x0.5 | x2 | x5 | x20 | dato/klokkeslæt`

- `PLAY` = x1.
- `PAUSE` = Time.timeScale 0.
- `x0.5` = Time.timeScale 0.5; slow tactical mode til ordreafgivelse og observation under pres.
- `x2` = Time.timeScale 2.
- `x5` = Time.timeScale 5.
- `x20` = Time.timeScale 20.

Aktiv state markeres i UI. x0.5 er en rigtig simulation speed og ikke en animation-only slow motion; AI, movement, reload, combat timers og battle clock afvikles alle med halv simulationstakt.

## B-222 — Tastaturgenveje

**IMPLEMENTERET.**

- `Space` = pause/resume med senest valgte speed.
- `0` = x0.5.
- `1` = Play/x1.
- `2` = x2.
- `3` = x5.
- `4` = x20.

## B-223 — Pause semantics

**BESLUTTET / IMPLEMENTERET.** Pause stopper den authoritative taktiske simulation samlet:

- Officer AI think/reaction timers.
- Regiment movement.
- Reload og Time.time-baseret fire cadence.
- Combat resolution der kræver simulation progress.
- Battle clock.

Følgende skal fortsat være brugbart under pause:

- kamera pan/rotate/zoom,
- UI og hover/inspection,
- selection,
- analyse af officerstats/reason codes.

Senere command-model kan tillade, at spilleren udarbejder ordrer under pause; hvornår de bliver effective afhænger stadig af valgt order-delay/realism model.

## B-224 — Input isolation

**IMPLEMENTERET.** Klik på time-control-panelet må ikke samtidig ramme slagmarken bag UI. PlayerCommander blokerer derfor selection/order-input, når pointeren ligger over simulation controls.

## B-225 — Speed semantics og high-speed determinism guard

**BESLUTTET / QA.** x0.5/x2/x5/x20 ændrer kun simulationens tidsrate. De må ikke med vilje ændre:

- weapon accuracy,
- reload-sekunder målt i simulationstid,
- movement speed målt i simulationstid,
- morale/cohesion-regler,
- AI officer stats,
- difficulty bonuses,
- casualty resolution.

x0.5 skal derfor give spilleren dobbelt så meget realtid til at reagere på samme mængde simulationstid uden at give enheden en skjult combat-bonus.

Diskrete Update-timere kan være følsomme over for store simulation-deltaer ved x20. Det skal derfor testes specifikt, og senere authoritative systems bør bruge robuste timers/state transitions frem for frame-afhængige antagelser.

## B-226 — UI readability ved speed

Combat-feedback som `Ramte N` må gerne bruge unscaled realtime for at forblive læsbar ved x5/x20. Det ændrer ikke simulationens combat-resultat; kun hvor længe feedbacken er synlig for spilleren.

Ved x0.5 må UI heller ikke utilsigtet fordoble feedback-varigheden i realtid, hvis feedbacken bevidst er defineret som UI/readability og ikke simulation state.

## B-227 — Battle result

Når slaget afsluttes, går simulationen terminalt i pause. Time controls må ikke genstarte et afsluttet slag. Kun `R`/restart må starte en ny battle state.

## B-228 — v00.00.09 acceptance-test

1. Topbaren viser dato og klokkeslæt.
2. PLAY sætter simulation til x1.
3. PAUSE stopper battle clock, AI, movement og reload/fire progression.
4. Space pauser og genoptager den tidligere speed, også hvis den tidligere speed var x0.5.
5. x0.5 afvikler battle clock, AI, movement og reload ved omtrent halv realtidsrate i forhold til x1.
6. x2, x5 og x20 accelererer battle clock og simulation i korrekt relativ retning.
7. Kamera/UI fungerer under pause og x0.5.
8. Klik på time controls ændrer ikke unit selection bag panelet.
9. `Ramte N` forbliver læsbart ved både x0.5 og høj speed.
10. Officer-AI telemetry/reason state kan inspiceres under pause og i slow mode.
11. Terminal battle result kan ikke unpauses via Play/x0.5/x2/x5/x20.
12. Samme testscenario må ikke få en tilsigtet accuracy/reload/morale-bonus alene fordi x0.5 er valgt.

## B-229 — Senere strategisk tid

Når strategilaget kobles på, skal samme princip udvides til kampagneklokken. Strategisk og taktisk tid må ikke være to uafhængige sandheder. Ved manuelt taktisk slag fryses strategisk world simulation efter den allerede besluttede model og fremskrives kontrolleret til slagets sluttid ved return til strategilaget.
