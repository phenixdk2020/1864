# PROJECT 1864 — Infantry Charge + Regiment HQ command hierarchy

**Status:** Godkendt designretning  
**Aktuel prototypebaseline:** v00.00.09f24 TEST  
**Roadmap:** `docs/backlog/B-280-INFANTRY-CHARGE-REGIMENT-HQ-SEQUENCE.md`

## Dansk command hierarchy

Fremover anvendes følgende faste navne i UI, AI og design:

| Niveau | Chef | Direkte underordnede i prototype |
| --- | --- | --- |
| Kompagni | **Kaptajn** | soldater / kompagni |
| Bataljon | **Major** | 4 kompagnier |
| Regiment | **Oberstløjtnant** | 2 bataljoner / 2 Majorer |
| Brigade | **Oberst** | regimenter |
| Division | **Generalmajor** | brigader |

Den næste teststruktur er derfor **ikke** en “bataljonschef over Majorer”. Majoren er selv bataljonschef. Den nye chef over de to Majorer er **regimentschef / Oberstløjtnant**.

## Næste test-OOB

```text
REGIMENT — Oberstløjtnant
├─ 1. Bataljon — Major A
│  ├─ 1. Kompagni — Kaptajn
│  ├─ 2. Kompagni — Kaptajn
│  ├─ 3. Kompagni — Kaptajn
│  └─ 4. Kompagni — Kaptajn
└─ 2. Bataljon — Major B
   ├─ 5. Kompagni — Kaptajn
   ├─ 6. Kompagni — Kaptajn
   ├─ 7. Kompagni — Kaptajn
   └─ 8. Kompagni — Kaptajn
```

Total: **8 kompagnier + 2 Majorer + 1 Oberstløjtnant**.

## Infantry charge før Dragoons

Dragoons må ikke introduceres før infantry charge/melee er stabilt, fordi mounted charge senere skal genbruge samme combat state framework.

Godkendt infantry charge lifecycle:

```text
CHARGE_ORDERED
→ CHARGE_APPROACH
→ CHARGE_COMMIT
→ MELEE_CONTACT
→ MELEE_ENGAGED
→ BREAK / ROUT / VICTORY
```

Charge er en eksplicit target-order og må ikke være det samme som almindelig `ANGRIB`.

### ANGREB

- lukker til passende fire range;
- deployerer og skyder;
- kan senere vælge charge som AI-beslutning.

### CHARGE

- lukker helt til fysisk kontakt;
- preferred ranged distance suspenderes;
- formationen skal vende mod charge-target;
- terræn-/bridge legality gælder stadig;
- ved kontakt overtager melee-core;
- ranged fire må ikke fortsætte normalt gennem melee-state.

## Under fire under højere HQ mission

Når et AI-ON kompagni marcherer på en Major-/Regimentsmission og bliver effektivt beskudt af en kendt fjende inden for egen Long/Maximum range, skal missionen som udgangspunkt **suspenderes**, ikke slettes:

```text
MOVING_TO_MISSION
→ CONTACT_UNDER_FIRE
→ DEPLOYING_TO_CONTACT
→ ENGAGING
→ RESUME_MISSION / HOLD / WITHDRAW
```

Enheden må ikke blindt marchere videre gennem vedvarende effektiv ild.

Bro/smal passage er en undtagelse: enheden må først fri af passagen før normal kampdeployment.

## Hierarkisk AI authority

Normal command flow:

```text
PLAYER
→ Oberstløjtnant / Regiment
→ Major / Bataljon
→ Kaptajn / Kompagni
→ fysisk enhed
```

Hvert HQ-niveau giver mission til **næste direkte niveau**, ikke normalt direkte til alle lavere enheder.

### Oberstløjtnant

Oberstløjtnanten beslutter fx:

- hvilken bataljon der tager venstre/højre sektor;
- hvilken bataljon der udfører hovedangreb;
- om én bataljon holdes i reserve;
- om én bataljon kan forsøge en flanke;
- fallback / støtte / defence in depth.

### Major

Majoren omsætter bataljonsmissionen til konkrete roller og formation slots for sine fire kompagnier.

### Kaptajn / kompagni-AI

Kaptajnen udfører den lokale mission, reagerer på fjendtlig ild, pathfinding, formation, fire policy, charge/melee osv.

## Reserve og flanke

Reserve er aldrig tvungen blot fordi formationen har fire kompagnier eller to bataljoner.

På regimentsniveau kan Oberstløjtnanten vælge:

- 2 bataljoner i front;
- 1 i front + 1 reserve;
- 1 binder + 1 flanker;
- forskudt forsvar / defence in depth.

På bataljonsniveau kan Majoren tilsvarende vælge 0 eller 1 kompagnireserve, afhængigt af situationen.

Beslutninger skal senere baseres på kendt fjende, styrke, cohesion, terræn, officerstats, fog-of-war og ordreintention.

## UI

Kompagni, Major og Oberstløjtnant skal bruge samme grundlæggende bottom-HUD design og samme AI-begreber:

- `AI ON / OFF`
- `DEF / BAL / OFF`

Men ordreknapper varierer efter command level.

Oberstløjtnanten giver regiment-missioner til Majorer; Majoren giver bataljonsmissioner til kompagnier; kompagniet har direkte taktiske ordrer inkl. kommende `CHARGE`.
