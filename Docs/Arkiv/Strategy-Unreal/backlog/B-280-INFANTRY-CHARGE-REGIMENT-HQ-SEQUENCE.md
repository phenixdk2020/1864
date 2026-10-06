# PROJECT 1864 — B-280–B-289 Infantry Charge, 2nd Battalion and Regiment HQ

**Status:** AKTIV NÆSTE SEKVENS  
**Branch:** `channel-test`  
**Formål:** Fastlægge den næste taktiske udviklingsrækkefølge før Dragoons implementeres.

## Kommandokæde — dansk baseline

Den danske kommandokæde i denne prototype er fremover:

- **Kompagni:** Kaptajn
- **Bataljon:** Major
- **Regiment:** Oberstløjtnant
- **Brigade:** Oberst
- **Division:** Generalmajor

Den næste OOB-gate bliver derfor:

```text
Oberstløjtnant — Regiments-HQ
├─ Major — 1. Bataljon
│  ├─ 1. Kompagni
│  ├─ 2. Kompagni
│  ├─ 3. Kompagni
│  └─ 4. Kompagni
└─ Major — 2. Bataljon
   ├─ 5. Kompagni
   ├─ 6. Kompagni
   ├─ 7. Kompagni
   └─ 8. Kompagni
```

Det giver **8 infanterikompagnier, 2 Majorer og 1 Oberstløjtnant**.

---

## B-280 — Infantry CHARGE command

**Status:** NÆSTE IMPLEMENTERINGSGATE

### Mål

Infanteri skal kunne få en eksplicit `CHARGE`-ordre mod en fjendtlig enhed.

### Første regler

- CHARGE er en målrettet ordre mod en konkret fjendtlig enhed.
- Charge må ikke teleportere formationer sammen.
- Enheden skal først orientere/deployere mod målet og derefter lukke fysisk afstand.
- Ranged fire og charge må ikke samtidigt kæmpe om movement authority.
- En charge må afbrydes hvis enheden router eller målet bliver ugyldigt.
- Bro, flod og hard blockers har stadig højere fysisk route-authority.
- Charge target beholdes strategisk gennem en nødvendig obstacle-/bridge-route.
- Under sidste charge-fase skal enheden prioritere fysisk kontakt over preferred ranged distance.

### Første charge-states

```text
CHARGE_ORDERED
→ CHARGE_APPROACH
→ CHARGE_COMMIT
→ MELEE_CONTACT
→ MELEE_ENGAGED
→ BREAK / ROUT / VICTORY
```

### UI

- Kompagni-HUD får knappen `CHARGE`.
- Ved CHARGE vælges en fjendtlig enhed som target.
- Aktiv charge skal vises tydeligt i unit status/telemetry.

---

## B-281 — Infantry melee baseline færdiggøres

**Status:** NÆSTE IMPLEMENTERINGSGATE EFTER B-280

### Eksisterende baseline

`PrototypeMeleeCombatManager` har allerede en første fysisk contact-baseline med:

- melee ved fysisk kontakt / meget kort afstand;
- begge formationer stopper ved contact;
- facing mod modstander;
- separate melee casualty-pulser;
- morale/cohesion-tab;
- mulighed for rout.

### Skal tilføjes/forbedres

- kun reelt engagerede/contact mænd skal tælle mod melee-effekt;
- charge momentum skal påvirke første melee-puls;
- frontal/flanke/bagfra kontakt skal senere kunne påvirke udfaldet;
- formation/cohesion ved kontakt skal have tydelig effekt;
- ranged fire skal suspenderes korrekt i melee;
- enheder må ikke stå permanent oven i hinanden;
- vinder skal kunne holde eller fortsætte efter modstanderen bryder;
- tab skal stadig skabe normale casualty visuals.

### Første QA

Test mindst:

1. 190 vs 190 frontal charge;
2. 190 vs svækket kompagni;
3. charge mod enhed med lav cohesion;
4. failed charge / attacker routes;
5. defender routes og attacker bliver på slagmarken uden at følge routed target gennem alt terræn.

---

## B-282 — Under fire response under mission movement

**Status:** BESLUTTET — implementeres sammen med melee/mission authority

En AI-ON enhed, der er på vej mod en Major-/højere HQ-destination, må ikke blindt marchere videre under effektiv fjendtlig beskydning.

Første regel:

```text
MOVING_TO_MISSION
+ confirmed hostile fire
+ attacker within own Long/MaximumRange
→ CONTACT_UNDER_FIRE
→ deploy Line
→ face attacker
→ return fire
```

Den højere mission slettes ikke; den **suspenderes**. Efter lokal kontakt vurderes om enheden skal:

- fortsætte missionen;
- holde og kæmpe;
- trække sig.

Undtagelse: enhed midt på bro/smal kritisk passage må først fri af passagen før normal deploy.

AI-OFF/direct player control skal som udgangspunkt ikke automatisk annullere en eksplicit spillerordre.

---

## B-283 — Anden Major + yderligere 4 kompagnier

**Status:** EFTER CHARGE/MELEE BASELINE

### Mål

Udvide dansk test-OOB fra én bataljon til to:

- **1. Bataljon:** Major A + 4 kompagnier
- **2. Bataljon:** Major B + 4 kompagnier

### Krav

- begge Majorer er fysiske HQ-enheder;
- begge har samme selection/HUD/AI/doctrine-model;
- hver Major ejer kun sine fire kompagnier;
- Major A må ikke skrive missioner til Major B's kompagnier;
- manuel company authority virker identisk i begge bataljoner;
- begge bataljoner bruger samme formation-slot/pathfinding/combat/charge/melee-system.

### Selection

Når en Major vælges:

- dens fire direkte underordnede kompagnier kan vises med command-links;
- den anden Majors kompagnier skal ikke vises som direkte underordnede.

---

## B-284 — Regiments-HQ: Oberstløjtnant

**Status:** EFTER B-283

### Rolle

Oberstløjtnanten er **regimentschef**, ikke bataljonschef.

Han har to direkte underordnede:

- Major A / 1. Bataljon
- Major B / 2. Bataljon

Han kommanderer derfor ikke de otte kompagnier direkte som normalregel.

### Fysisk HQ

Første prototype skal have:

- fysisk Oberstløjtnant-HQ på kortet;
- selectable HQ;
- samme overordnede bottom-HUD stil som Major/kompagni;
- command-links til de to Majorer;
- AI ON/OFF;
- DEF/BAL/OFF doctrine;
- current mission / current subordinate assignments;
- telemetry for beslutninger.

---

## B-285 — Oberstløjtnant giver ordrer til Majorerne

**Status:** EFTER B-284

### Grundprincip

Spilleren giver en **regimentsmission** til Oberstløjtnanten. Oberstløjtnanten fordeler derefter opgaver til de to Majorer, og Majorerne fordeler deres opgaver til egne kompagnier.

```text
PLAYER
  ↓
OBERSTLØJTNANT / REGIMENT
  ↓              ↓
MAJOR A          MAJOR B
  ↓↓↓↓             ↓↓↓↓
4 kompagnier     4 kompagnier
```

### Første regimentsordrer

- `ANGRIB HER`
- `FORSVAR HER`
- `RYK FREM`
- `HOLD`
- `TILBAGETRÆK`
- `SAML`

### Første AI-opgaver

Oberstløjtnanten skal kunne beslutte fx:

- begge bataljoner i front;
- én bataljon hovedangreb + én reserve;
- én bataljon binder fjenden + én flanker;
- én bataljon forsvarer hovedsektor + én støtter/fallback;
- om reserve overhovedet er nødvendig.

Reserve/flanke er **ikke obligatorisk**. Beslutningen skal baseres på kendt fjende, egen styrke, cohesion, terræn og senere officerstats/fog-of-war.

### Authority

- ny Oberstløjtnant-ordre må erstatte tidligere regimentsmission;
- ny regimentsmission skal give de relevante Majorer AI ON og ny mission;
- manuel ordre direkte til en Major kan senere detach'e bataljonen fra regiments-AI efter samme authority-model som kompagni ↔ Major;
- direkte manuel ordre til et kompagni følger den allerede besluttede company authority-model.

---

## B-286 — Regiment formation planning

**Status:** EFTER B-285

Oberstløjtnanten skal placere **bataljoner**, ikke individuelle kompagnier.

Første templates:

- 2 bataljoner side om side;
- 1 fremme + 1 reserve;
- venstre/højre sektor;
- angreb med én bataljon + flankemulighed for den anden;
- defence in depth.

Majoren løser derefter sin lokale 4-kompagni-formation inden for den tildelte bataljonssektor.

Dette er hierarkisk formation planning:

```text
Regiment slot/sector
→ Battalion slot/sector
→ Company slots
```

---

## B-287 — Command delay mellem Oberstløjtnant og Major

**Status:** SENERE I SAMME HQ-FASE

Når hierarchy fungerer uden delay, kobles courier/order lifecycle på:

- Oberstløjtnant sender ordre til Major;
- ordre har fysisk/progress-baseret delay;
- lille courier-rytter kan visualisere ordren;
- Majoren fortsætter tidligere mission indtil ny ordre modtages;
- acknowledgement/status kan returnere til regiments-HQ.

Dette skal senere genbruge samme model højere op i kæden.

---

## B-288 — Dragoons

**Status:** UDSKUDT TIL EFTER B-280–B-287 BASELINE

Dragoons implementeres først når:

1. infantry charge virker;
2. infantry melee virker;
3. to bataljoner / to Majorer virker;
4. Oberstløjtnant kan give missioner til Majorer;
5. authority mellem Regiment → Major → Kompagni er stabil.

Derefter genbruges samme charge/melee-core til mounted/dismounted combat.

---

## B-289 — Artillery gate

Kanonbatteri forbliver efter Dragoons-baselinen, medmindre vi eksplicit ændrer roadmap igen.

---

# Ny aktiv implementeringsrækkefølge

| Gate | Emne | Prioritet |
| ---: | --- | --- |
| 1 | Stabiliser aktuelle f24 mission/destination/pathfinding regressions | AKTIV |
| 2 | Infantry CHARGE command | NÆSTE |
| 3 | Infantry melee baseline færdiggøres | NÆSTE |
| 4 | Under-fire response / suspend-resume mission | HØJ |
| 5 | Anden Major + yderligere 4 kompagnier | HØJ |
| 6 | Oberstløjtnant / fysisk Regiments-HQ | HØJ |
| 7 | Regiments-AI fordeler ordrer til to Majorer | HØJ |
| 8 | Regiment formation planning | HØJ |
| 9 | Courier/order delay Regiment → Major | EFTER BASELINE |
| 10 | Dragoons | DEREFTER |
| 11 | Kanonbatteri | EFTER DRAGOONS |
