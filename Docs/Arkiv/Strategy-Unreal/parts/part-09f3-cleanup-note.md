# P0A v00.00.09f3 — Formation motion, facing motion og river-only navigation

Dette supplement er gældende for 09f3-testgrenen oven på designbaseline v00.02.08 og den accepterede 09f/09f1/09f2 tactical-command retning.

## Designbeslutning: formationer skal bevæge sig fysisk

Et regiment må ikke visuelt springe direkte mellem Line og Column. Når formation state ændres, flytter de repræsentative `Soldier_*`-figurer sig mod deres nye slots med en begrænset hastighed. Den underliggende formation state må fortsat skifte straks for command/combat-logik, men den grafiske præsentation skal vise selve reformeringen.

Det gælder både manuel `F/C`, automatisk marchkolonne på længere bevægelse og deployment tilbage til Line nær destination eller kontakt. Formationsskiftet skal være læsbart på slagmarken som en faktisk samling/udfoldning af regimentet.

## Designbeslutning: facing skal drejes, ikke teleporteres

`Z/X` og tilsvarende UI-knapper ændrer fortsat ønsket facing med 15 grader pr. ordre. Den synlige formation må imidlertid ikke rotere 15 grader på én frame. Stationære regimenter drejer gradvist mod den ønskede facing. 09f3 bruger ca. 24 grader/sekund som første QA-tuning.

Samme princip gælder slut-facing efter en movement route: regimentet ankommer, deployer og drejer synligt til slutretningen i stedet for at få root-transformen sat direkte til den nye rotation som et visuelt hop.

## Designbeslutning: scenery pass-through bevares i denne baseline

09f2-testen viste, at de tidligere stalls var knyttet til obstacle/pathfinding-lagene. Derfor forbliver Trees, FencePost, Farmhouse og Barn pass-through i 09f3. De må ikke skrive steering destination, recovery relocation eller formation state.

Det betyder ikke, at alle objekter permanent skal være uden gameplay-effekt. Senere kan terræn og objekter igen give cohesion-, speed-, cover- eller frontage-effekter, men de må genindføres som kontrollerede systemer og ikke som kæder af hårde cirkulære blockers, der kan låse regimentets centrum.

## Designbeslutning: floden er undtagelsen

Floden er hård taktisk terrænregel selv i pass-through-baselinen. Infanteri må ikke krydse åbent vand ved almindelig movement-order. Den eksisterende faste bro ved cirka `z=22` er den eneste tilladte krydsning i denne test.

09f3 bruger derfor et separat `PrototypeRiverBridgeOnly09F3`-lag, som kun håndterer vand/barriere-reglen. Hvis en ordre går til modsatte bred, bevares spillerens oprindelige slutmål, mens den aktive steering går til egen brotilgang, over broen og derefter tilbage til det oprindelige mål. En safety guard må flytte et regiment tilbage til sidste lovlige position, hvis et andet system forsøger at placere regimentets centrum i åbent vand.

Pontoner og engineer-crossings er fortsat fremtidige capabilities og må ikke opstå automatisk.

## Navigation authority i 09f3

Følgende gamle obstacle/pathfinding-lag er runtime-disabled: `PrototypeBattlefieldNavigationManager`, `PrototypeNavigationRecoveryManager`, `PrototypeBattlefieldNavigationV3`, `PrototypeBattlefieldNavigationV4`, `PrototypeNavigation09AHotfix`, `PrototypeNavigation09BTreePassThrough` og `PrototypeManualRouteRecovery09H4`.

`PrototypeRiverBridgeOnly09F3` er den eneste aktive route constraint og må kun håndtere river/bridge-reglen. Movement, Officer AI, combat og formation policy forbliver ejet af deres eksisterende systemer.

## QA/teknisk oprydning

Unity 6.6 giver CS0618 på gamle `FindObjectsByType<T>(FindObjectsSortMode)`-kald i de nu runtime-disabled navigation compatibility files. I 09f3 holdes QA Console ren med `Assets/csc.rsp` for CS0618/CS0219, mens legacy-navigationen er frakoblet. Når river-only-baselinen er accepteret, skal næste egentlige navigation-refactor fjerne eller modernisere den døde kode i stedet for permanent at leve med suppression.

Input Manager-deprecation behandles separat. Den nuværende tactical UI bruger legacy `Input` og IMGUI; derfor må projektet ikke skiftes blindt til Input System-only som en warning-fix, fordi control surface først skal migreres samlet.

## 09f3 acceptance

- 1 vs 1-baseline bevares.
- Fjenden står stille indtil F5 / START FJENDE.
- Lang march viser fysisk Line -> Column overgang.
- Deployment viser fysisk Column -> Line overgang.
- Z/X viser gradvis fysisk drejning.
- Trees/fences/buildings stopper ikke regimentet.
- Floden kan kun krydses via den faste bro.
- Range cone, dual flags, bottom command bar og box selection fungerer fortsat.
