# PROJECT 1864 — Backlog B-170–B-179: Officer AI og delegeret kommando

**Status: BESLUTTET / AKTIV IMPLEMENTERING**  
**Designbaseline: v00.02.08**  
**Target prototype: P0A v00.00.09 TACTICAL COMMAND TEST**

Dette supplement fastlægger spillerens mulighed for at delegere styringen af egne formationer til deres officerer. Systemet bygger oven på commander stats, personality, autonomy, order delay, fog of war og hierarchical AI.

Fjendens formationer bruger samme officerbaserede decision core. Difficulty ligger som et separat lag ovenpå og må ikke omskrive officerens stats eller give skjulte combat-bonusser/omniscience. Den detaljerede difficulty-plan ligger i `B-180-AI-DIFFICULTY-AND-V009.md`, mens officerstats ligger i `B-190-OFFICER-STATS-MODEL.md`.

## B-170 — AI Unit ON/OFF

**BESLUTTET / IMPLEMENTERET v00.00.09 TEST.** En valgt formation har en tydelig kommando:

`AI UNIT: OFF / ON`

- **OFF** = spilleren vælger selv formationens taktiske handlinger.
- **ON** = den lokale officer fører formationen inden for mission, commander intent og constraints.
- Toggle-state er synlig i battle UI.
- AI ON er delegation, ikke en gameplay-bonus.

I v00.00.09 toggles valgte danske regimenter med **`I`**. `A` er reserveret til WASD-kamera og bruges ikke som AI-toggle.

## B-171 — Missionen er AI'ens ramme

**BESLUTTET / DELVIST IMPLEMENTERET v00.00.09.** AI-officeren må ikke kende spillerens skjulte intention eller frit opfinde et helt nyt strategisk mål.

Første runtime missions:

- DefendArea.
- Hold.
- MoveToPoint.
- AttackTarget.
- AttackNearest.

Højreklik går i v00.00.09 direkte til Regiment, når AI er OFF, og gennem `OfficerAIController`, når AI er ON.

Senere udvides dette med Defend line, Support formation, Screen, Guard artillery/supply, Reserve, Delay, Withdraw og Pursue samt mission constraints som `do not leave area`, `conserve ammunition` eller `do not pursue`.

## B-172 — Officer stats styrer reaktioner

**BESLUTTET / AKTIV v00.00.09.** Officerens beslutninger afledes af en konkret 0–100 profil.

Første otte kerne-dimensioner:

1. Leadership.
2. Inspiration.
3. Tactical Skill.
4. Initiative.
5. Staff / Command Skill.
6. Discipline / Obedience.
7. Aggressiveness ↔ Caution.
8. Composure / Nerve.

Officer Experience er separat baggrundsværdi.

**Composure/Nerve** beskriver officerens egen evne til at bevare overblik under pres og må ikke blandes sammen med Leadership, Inspiration eller regimentets morale. I v00.00.09 påvirker den stress-relateret reaction delay og decision noise.

To officerer med samme mission kan derfor reagere forskelligt.

## B-173 — AI bruger kun tilgængelig information

**BESLUTTET.** Officer AI må kun reagere på den knowledge state, officeren/formationen realistisk har adgang til.

Senere kilder omfatter:

- synlige fjender,
- rapporter med timestamp/confidence,
- egne scouts/skirmishers/cavalry,
- meldinger fra overordnede/naboer,
- terræn og kendte objectives,
- observeret artilleriild og andre slagmarkssignaler.

v00.00.09 bruger endnu prototype-slagmarkens simple target visibility og er derfor ikke fuld fog-of-war implementation.

## B-174 — AI reaction set

**BESLUTTET / MVP DELVIST IMPLEMENTERET.** Når AI Unit er ON kan officerens controller:

- move/advance,
- hold/stabilise,
- vælge/holde attack target,
- vælge approach/engagement distance,
- skifte line/column i simple situationer,
- overholde eller lokalt afvige fra movement mission ud fra Discipline/effective Aggressiveness,
- tage commander doctrine og aggression intent med som bias,
- respektere commander fire policy som hard constraint.

Senere reaction set omfatter cover, prone, skirmishers, reserve support, rally, withdrawal, artillery fire-control og cavalry/dragoon actions.

## B-175 — Doctrine, ordre-intent og officerens egen personlighed

**BESLUTTET / IMPLEMENTERET v00.00.09 TEST.** Selected-unit command-menuen giver tre doctrine-valg:

- `DEFENSIVE`
- `BALANCED`
- `OFFENSIVE`

Desuden har spilleren en 0–100 ordre-slider:

`forsigtig 0  <---->  100 aggressiv`

Dette er **commander intent**, ikke en direkte omskrivning af officerprofilen.

Før doctrine-bias beregnes første MVP effective aggressiveness som:

`Officer Aggressiveness * 0.70 + Commander OrderAggressiveness * 0.30`

Doctrine giver derefter bounded bias:

- Defensive: `-12`
- Balanced: `0`
- Offensive: `+12`

Officerens egne stats er dermed fortsat den dominerende adfærdskilde. To officerer kan udføre den samme Offensive ordre forskelligt.

Danmarks testregimenter starter som **Defensive / OrderAgg 35**, når AI aktiveres. Preussiske angribere starter **Offensive / OrderAgg 65**.

Autonomy `Strict/Normal/Independent` er fortsat et separat senere system og må ikke blandes sammen med doctrine eller aggression intent.

## B-176 — Player override og order delay

**BESLUTTET.** Spilleren kan slå AI Unit OFF og udstede nye ordrer. Toggle må på sigt ikke teleportere information eller ophæve command friction.

v00.00.09 bruger immediate prototype-control. Senere order-delay/courier-system håndterer realistisk kontrolskift.

## B-177 — Hierarkisk delegation

**PLANLAGT.** Delegation skal senere kunne bruges på regiment/bataljon/brigade/division-niveau. En brigadechef med AI ON skal kunne fordele opgaver til underformationer, mens spilleren stadig kan tage enkelte formationer tilbage under direkte kontrol.

v00.00.09 arbejder kun på regimentsniveau.

## B-178 — AI transparency / reason codes

**BESLUTTET / AKTIV v00.00.09.** AI-beslutninger skal kunne forklares.

Første reason/task eksempler:

- `Moving to assigned mission point`
- `Discipline keeps unit on assigned movement`
- `Attacker assessing nearest enemy`
- `Holding assigned defensive area`
- `Morale pressure - officer pauses advance`
- `closing to officer/order preferred range`
- `preferred engagement range reached`

Console telemetry bruger `AI-DIAG|...` med:

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

## B-179 — Battle UI

**IMPLEMENTERET v00.00.09 TEST.** Battle UI bruger nu en mere læsbar todelt model:

### Contextual regiment-info

Den store regiment-info må ikke ligge permanent oven på formationen.

- Fuld regiment-info vises kun, når enheden er **valgt** eller når musen er **over enheden**.
- Info-panelet placeres med en tydelig offset til siden/over formationen, så soldaterne fortsat er synlige.
- `Ramte N` kan fortsat vises som kort combat-feedback uden at kræve selection/hover.
- Pointer over UI-paneler skal ikke samtidig aktivere battlefield hover/order input.

### Fast command-bar nederst

Ved valg af danske regimenter vises command-menuen som en **fast fuldbredde bundbar** i stedet for et flydende højre-sidepanel.

Bundbaren indeholder:

`AI UNIT | DOCTRINE | ORDER AGGRESSION | FIRE POLICY`

Konkret vises:

- valgt regiment / antal valgte regimenter,
- officer og compact profile,
- current task,
- AI ON/OFF,
- Defensive/Balanced/Offensive,
- 0–100 aggression-intent slider,
- HOLD/CLOSE/MEDIUM/LONG fire policy,
- Close/Medium/Long range-værdier og fire arc.

Klik på bundbaren isoleres fra battlefield input, så UI-click ikke samtidig bliver selection/move/attack. Layoutet er skærmbreddebaseret, så alle fire fire-policy-knapper og doctrine-valg forbliver synlige i normale Game View-opløsninger.

Den fremtidige hierarkiske menu udvides senere med bl.a. `AUTONOMY | MISSION | ORDRESTATUS | COURIER STATUS`.

## Designværn

- AI ON må ikke give information, formationen ikke burde have.
- Officerstats er AI-/command-inputs, ikke skjulte generiske combat buffs.
- Difficulty må ikke omskrive officerens historiske profil.
- Commander doctrine/order må bias adfærd, men ikke erstatte officerens personality/stat-model.
- Dårlige/stressede officerer må kunne reagere suboptimalt på en forklarlig måde.
- Composure beskriver officerens egen stressstabilitet; regimentets morale/cohesion er separate states.
- En spiller skal senere kunne føre alt selv, delegere dele eller fungere primært som øverstkommanderende.

## Implementeringsstatus v00.00.09

P0A v00.00.09 har nu:

- AI UNIT ON/OFF på `I`,
- OfficerProfile til alle fire regimenter,
- shared decision core for Danmark/Preussen,
- otte QA-officerstats + separat Experience,
- Defensive/Balanced/Offensive doctrine,
- commander 0–100 aggression intent,
- fire-policy constraint,
- preferred-range close movement,
- attacker/defender testscenario,
- contextual hover/selection regiment-info,
- fast fuldbredde command-bar nederst,
- UI pointer isolation,
- Easy/Normal/Hard,
- UI-state/reason codes og telemetry.

Fuld brigadehierarki, courier-system, fog-of-war og avancerede combat-actions kommer efter denne gate.
