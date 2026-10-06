# B-270–B-279 — Campaign Map / Strategic Layer v00.00.10

## Purpose

Efter P0A v00.00.09 Tactical Command TEST er næste naturlige vertical-slice trin at forbinde de taktiske slag til en enkel campaign/strategisk realtidsmodel. Målet er ikke at bygge hele grand-strategy-spillet på én gang, men at få den første komplette loop:

`Campaign map -> movement/contact -> tactical battle -> battle result -> tilbage til campaign map`

## Gate før implementering

v00.00.09 bør først bestå én sidste tactical stability-test:

1. Unity compiler uden blocking errors.
2. Fire/range/reload/ammunition fungerer.
3. Multi-select + group facing fungerer med AI OFF.
4. Officer AI kan angribe uden at stable regimenter oven i hinanden.
5. Huse/træer/forhindringer kan ikke bruges som destination eller passeres direkte.
6. Floden kan kun krydses via bro; ponton kræver senere engineer capability.
7. Infantry contact udløser melee i stedet for gennemgang.
8. Pause/speed og UI fungerer stabilt.

Når dette er godkendt, kan v00.00.09 fryses som tactical baseline og v00.00.10 begynde.

## v00.00.10 MVP scope

### B-270 Campaign scene

- Ny separat campaign scene.
- 2D/2.5D strategisk kort med terrain/background og pan/zoom.
- Tactical battle scene forbliver separat.
- Ingen Unity-scene skal destruere den anden branches baseline; campaign bygges additivt i TEST.

### B-271 Geografisk datamodel

Første prototype bruger et begrænset område med noder/regioner frem for pixel-perfect fri bevægelse.

En node kan indeholde:

- navn,
- koordinat,
- ejer/kontrol,
- terrain type,
- road links,
- rail link senere,
- river/crossing flags,
- depot/supply capability,
- fortification flag,
- tactical battlefield template reference.

Den historiske node-/regionliste skal senere dokumenteres med kilder; QA-layout må ikke præsenteres som historisk præcis før research er gennemført.

### B-272 Strategic formations

Campaign map viser formationer som stacks/tokens med stabil ID.

Første state:

- nation/team,
- formation name,
- regiments,
- manpower,
- ammunition,
- morale/cohesion summary,
- officer/HQ reference,
- current node,
- destination/route,
- movement state.

Samme regiment-ID skal kunne leve videre fra campaign til tactical og tilbage igen.

### B-273 Strategic movement og tid

- Campaign time kører i realtid med pause og speed controls.
- Formationer flyttes langs road/node links.
- Travel time afhænger først af distance + terrain/road modifier.
- Senere kobles weather, fatigue, supply, bridge damage og staff quality på.
- To fjendtlige formationsgrupper i samme contact area kan udløse battle.

### B-274 Battle trigger

Ved kontakt vises et battle panel med minimum:

- location,
- angriber,
- forsvarer,
- deltagende formationer,
- estimated strength,
- `Fight Tactical Battle`,
- senere `Auto Resolve`.

MVP går direkte fra campaign scene til den eksisterende tactical prototype.

### B-275 Campaign -> Tactical transfer

Følgende state skal overføres:

- participating regiment IDs,
- strength,
- ammunition,
- morale/cohesion,
- officer profiles,
- attacker/defender role,
- battle location/template,
- campaign date/time.

Det taktiske bootstrap-scenarie med hårdkodede QA-regimenter skal gradvist kunne erstattes af en BattleContext payload.

### B-276 Tactical -> Campaign result

Efter slag returneres som minimum:

- surviving strength,
- ammunition,
- morale/cohesion,
- routed/destroyed state,
- winner/control result,
- officer state senere,
- captured equipment/prisoners senere.

Der må ikke ske midnight/full reset mellem lagene.

### B-277 Campaign UI MVP

- Topbar: dato/tid + pause/speed.
- Venstre eller nederste compact selection panel.
- Klik på formation viser stats og route.
- Højreklik node = movement order.
- Ghost route/path mellem noder.
- Ownership/control vises enkelt og tydeligt.

### B-278 Supply hook

MVP kan begynde med simpelt supply-connected boolean/level, men dataarkitekturen skal støtte de senere fysiske supply-systemer:

- depot,
- road/rail route,
- ammunition replacement,
- food/fodder,
- wagons,
- cut-off state.

### B-279 Acceptance for first campaign loop

1. Campaign scene åbner uden errors.
2. Kort kan pan/zoomes.
3. Mindst én dansk og én preussisk formation kan flyttes mellem noder.
4. Campaign clock kan pause/accelereres.
5. Enemy contact udløser battle prompt.
6. Tactical scene åbnes med attacker/defender context.
7. Tactical battle kan afsluttes.
8. Resultat returneres til campaign map.
9. Strength/ammunition efter slag matcher tactical result.
10. Det er muligt at fortsætte campaign efter slaget.

## Ikke i første v00.00.10 slice

Følgende designes til senere og må ikke blokere første loop:

- fuld diplomacy,
- national economy,
- production chains,
- research trees,
- recruitment/training depth,
- full strategic fog of war,
- historical OOB completeness,
- rail timetables,
- naval campaign,
- engineer/pontoon construction UI,
- save-game completeness.

## Design principle

Første campaign prototype skal bevise **state continuity** mellem strategisk og taktisk lag. Når det virker, kan dybere economy, logistics, fog of war, engineering, cavalry reconnaissance, HQ/couriers og historical OOB bygges oven på en allerede fungerende game loop.
