# Kamp-AI Design
## 3D-slag 1825–1864: infanteri, artilleri, morter, kavaleri og AI-officerer

**Version:** 2.0
**Formål:** Designgrundlag til implementering af kamp-AI i Unreal Engine.
**Fokus:** 3D-slag, ikke campaign-laget.
**Bygger på:** `1864_Kamp_AI_Design_v1_1`.

---

# 0. Ændringer fra v1.1

**Ændret efter dine beslutninger**

1. **Samme kampe i alle epoker.** Slagene skal kunne udspille sig i 1825, 1851 og 1864 med samme AI. Epokeforskelle er data (afsnit 2), ikke kodegrene.
2. **Spilleren er øverste led.** Spilleren styrer overordnet, men kan slå AI til på ethvert kommandoniveau, også øverst (afsnit 3).

**Rettet**

3. Alle faste afstande i v1.1 (fx "400 m") er erstattet af afstande relativt til våbnets effektive rækkevidde.
4. Doktrin er nu tosidet: nation og udstyr/epoke (afsnit 2 og 24).

**Tilføjet**

5. Afsnit 13–14: artilleri som trussel og mål, og beskyttelse af eget artilleri.
6. Betingede ordrer, så ordre-latency ikke gør AI'en træg (afsnit 6).
7. Normaliserede trusselsværdier med responskurver (afsnit 10).
8. Sværhedsgrad uden snyd (afsnit 25).
9. Ydelsesbudget (afsnit 26), debug/test (afsnit 27) og revideret implementeringsrækkefølge med vertikal skive (afsnit 29).

---

# 1. Grundprincip

> **Missionen kommer ovenfra. Reaktionen sker lokalt. Information er lokal og ældes.**

En højere officer giver en hensigt og et område. En lavere officer beslutter, hvordan ordren udføres lokalt. Et kompagni må **ikke** selv opfinde en offensiv mission.

Et kompagni må selv:

- Åbne ild mod en kendt fjende inden for relevant afstand.
- Dreje front, søge dækning, fikse bajonetter.
- Foretage en kort lokal countercharge inden for leash.
- Danne carré ved reel kavaleritrussel og opløse den igen.
- Stoppe ild, hvis egne enheder kommer i ildfeltet.
- Beskytte nærliggende eget artilleri inden for leash (afsnit 14).
- Trække sig kortvarigt, reformere og genoptage den suspenderede ordre.

Reaktion må skrive til reaktionstilstand og `SuspendedMission`. Kun bataljon og opefter må skrive en ny mission.

---

# 2. Epoke-profiler (NY)

Samme kamp-AI skal give meningsfulde slag i 1825 (glatløbet flintlås-musket), 1851 (overgang til perkussion og tidlig riffel) og 1864 (riflede forladere, bagladere som tændnålsgevær, riflet artilleri). Det løses ved at flytte forskellene ud i data.

## 2.1 Princip

- **State machines og beslutningslogik er identisk på tværs af epoker.**
- Forskelle ligger i `FEraProfile` og `FEquipmentProfile`.
- Beslutninger bruger relative størrelser: `Distance / EffectiveRange`, ikke meter.
- Nærkampsafstande (bajonet, charge, carré-trussel) er næsten epoke-uafhængige og må godt være absolutte.

## 2.2 Datamodel

```text
FEraProfile
  EraId                      1825 / 1851 / 1864
  DefaultEquipmentByNation   standardudstyr pr. nation i epoken
  FormationDefaults          hvilke formationer er normale (skyttekæde mere udbredt senere)
  CavalryViabilityBias       generel kavaleri-rolle i epoken

FEquipmentProfile            refereres af hver enhed, ikke kun af epoken
  EffectiveRange             infanteri/artilleri
  MaxRange
  ReloadTime
  AccuracyByRange            kurve
  CanFireProne               bool
  AmmoTypes[]                kugle, granat, kartæsk, ...
  MinRange                   morter
  Dispersion
```

Udstyr hænger på enheden, ikke kun på året. To hære i samme slag kan have forskelligt udstyr (fx forladere mod bagladere), og campaign-teknologi kan opgradere udstyr uden at ændre epoke.

## 2.3 Hvad der automatisk ændrer sig

| Adfærd | Hvordan det følger af data |
|---|---|
| Åbningsafstand for ild | `EffectiveRange` fra udstyr |
| Carré-udløsning | Samme tærskel i `CavalryThreat`, men kavaleriets chargescore falder af sig selv mod hurtigere ild (`EnemyFirepower`) |
| Skyttekæde | `SkirmishTendency` i doktrin + epoke |
| Artilleriets kartæsk-afstand | `AmmoTypes` og `EffectiveRange` |
| Morterens rolle | `MinRange` og ammunitionstyper |

## 2.4 Regel

Ingen `if (year == 1864)` i AI-koden. Hvis en epokeforskel ikke kan udtrykkes som data, er det et tegn på, at datamodellen mangler et felt.

---

# 3. Kommandohierarki og spillerens rolle (ÆNDRET)

## 3.1 Hierarki

| Niveau | Ansvar |
|---|---|
| Soldat | Følger formation, sigter, skyder, lader, bevæger sig |
| Kompagni / Kaptajn | Formation, lokal bevægelse, ild, selvforsvar, morale, cohesion, taktiske reaktioner |
| Bataljonschef | Front, slots, reserver, angrebs-/forsvarsroller, genfordeling ved brud |
| Brigadechef | Bataljoner, reserve, hovedangreb, støtte, artillerimålområder |
| Divisionschef | Overordnet kampplan, brigader, kavaleri, counter-battery, morterobservatører |
| Specialenheds-AI | Artilleri, morter, kavaleri med egne state machines og målkontrakter |

Officerens fald ændrer ikke hierarkiet. Næste officer overtager med dårligere reaktionstid og vurdering, indtil enheden er reorganiseret.

## 3.2 Spilleren som øverste led

Spilleren sidder som øverste kommandør (division eller over). Spilleren giver ordrer til brigader og evt. specialenheder. Alt nedenunder kører som AI.

Spilleren styres af **samme regler som AI'en**:

- Ordrer er `FOrder`-objekter med latency (afsnit 6).
- Spilleren ser HQ's billede (kontakter med alder og tillid), ikke simulationens sandhed (afsnit 5).
- Spilleren kan ikke give ordrer direkte til et kompagni uden at tage kommandoen over (3.3).

## 3.3 AI-toggle pr. kommandonode

Hver kommandonode (division, brigade, bataljon) har en `ControlMode`:

```text
EControlMode
  Player     spilleren giver ordrerne for denne node
  Auto       AI-officer planlægger og giver ordrerne
```

Standard: øverste node = `Player`, alle underordnede = `Auto`.

| Handling | Resultat |
|---|---|
| Øverste node sættes til `Auto` | AI spiller hele slaget ud fra missionsmålet ("auto-battle") |
| Spiller tager en brigade `Player` | Brigadens AI-planlægning pauser, spilleren giver bataljonsordrer. Bataljonerne under forbliver `Auto` |
| Spiller sætter en node tilbage til `Auto` | AI overtager nodens planlægning |

## 3.4 Overgangsregler (undgå instant-perfekt overtagelse)

- **Auto → Player:** Nodens nuværende plan vises som "seneste plan". Ordrer under vejs fortsætter. Ingen nulstilling.
- **Player → Auto:** AI overtager de eksisterende ordrer som baseline og annullerer ikke noget. Første genvurdering sker efter officerens reaktionstid (afsnit 23), ikke øjeblikkeligt.
- Begge retninger er synlige for spilleren (tydeligt UI-tegn på hvilke noder der er `Auto`).
- Overgangen må ikke give AI'en adgang til information, spilleren ikke havde (og omvendt).

## 3.5 Spiller-UI-krav (konsekvens af latency og fog)

- Ordre under vejs vises med forventet ankomst og status (`Issued → InTransit → Received → Executing`).
- Kontakter vises med alder og tillid (usikkerhedsområde, ikke punkt).
- Valgfri aktiv pause eller langsom tid til ordregivning (designvalg).
- Valgfrit realismeniveau for egne enheders positioner: perfekt viden om egne, eller kun sidst rapporterede.

---

# 4. Mission og taktisk reaktion er adskilt

Kompagniet har to AI-lag og én blackboard:

```text
CompanyMissionAI
CompanyReactionAI
FCompanyBlackboard
```

**CompanyMissionAI** styres via en ordre, ikke et direkte felt.

```text
MOVE_TO  HOLD_POSITION  DEFEND_AREA  ATTACK_POSITION
SUPPORT  RESERVE  WITHDRAW  GUARD_BATTERY (NY)
```

**CompanyReactionAI** kører hele tiden og vurderer kun det, enheden ved eller med rimelighed kan slutte:

```text
EnemyThreat  CavalryThreat  ArtilleryThreat  FriendlyFireRisk
Morale  Suppression  Cohesion  FlankThreat  Ammunition  Fatigue
OfficerPresent  BatteryUnderThreat (NY)
```

ReactionAI må midlertidigt overstyre udførelsen, men ikke ændre den strategiske hensigt. Den suspenderer missionen og genoptager den.

```text
MISSION: MOVE_TO_POSITION
   ↓
REACTION: CavalryThreat
   ↓
SUSPEND mission → HALT → FORM_CARRE → FIRE
   ↓
CavalryThreat < 30 i 20 sek
   ↓
REFORM → ResumeMission()
```

En ny ordre annullerer ikke en igangværende reaktion. Den køes som `Superseded`, indtil reaktionen slipper, medmindre den nye ordre er `WITHDRAW` under rout eller sammenbrud.

---

# 5. Informationslag

AI'en læser kontakter, ikke simulationens sandhed.

```text
KnownContacts[]
  UnitId
  LastKnownPosition
  LastKnownHeading
  LastSeenTime
  Confidence        0–1
  Source            OwnEyes / Report / HQ
```

- Synsvidde reduceres af røg, skov, mørke, bakkekam og formationstæthed.
- En kontakt ældes. Efter X sekunder bliver positionen et usikkerhedsområde.
- Rapporter går opad med forsinkelse. Divisionen ved ikke automatisk, hvad et kompagni ser.
- Tabt spotter betyder tabt indirekte ild, indtil en ny observatør bekræfter målet.
- Identifikation på lang afstand kan være forkert (infanteri kan ligne kavaleri i støv, et batteri kan ligne en train).
- Artilleri og morter skyder kun på kendt eller observeret position med frisk rapport.
- Røg er både visuel hindring og kampværdi.

Spillerens billede er HQ's aggregerede kontakter, med samme regler.

---

# 6. Ordrer, latency og betingede ordrer

En ordre er et objekt, ikke et instant kald.

```text
FOrder
  Type
  TargetArea / TargetUnit
  IssuedAt / ArriveAt
  Status = Issued → InTransit → Received → Executing → Completed / Failed / Superseded
  IssuerId
  Priority
  Conditions[]      (NY)
  ThenOrder         (NY)
```

Løber eller ordonnans tager tid efter afstand og terræn. Modtagelse forsinkes, hvis kaptajnen er faldet, enheden er i melee eller er suppressed. En ordonnans kan dræbes, og ordren når da aldrig frem.

Officer-kvalitet påvirker, hvor hurtigt ordren forstås, hvor præcist målområdet tolkes, og om en uklar ordre bliver til HOLD frem for et gættet angreb.

## 6.1 Betingede ordrer (NY)

Latency kan gøre AI'en træg. Hvis kavaleriet først forfølger, når et sammenbrud er rapporteret og en ny ordre er nået frem, er vinduet lukket. Historisk løste man det med betingede ordrer.

```text
HOLD_POSITION
  IF EnemyBreaks   THEN PURSUE (inden for pursuit-leash)
  IF AmmoBelow 20% THEN WITHDRAW_TO FallbackPosition
  IF TimeElapsed   THEN ...
```

- Betingelser evalueres lokalt af modtageren ud fra dennes egen viden (kontakter, egen tilstand), aldrig ud fra sandheden.
- Mulige betingelser: `EnemyBreaks`, `EnemyRetreating`, `AmmoBelow`, `FriendlyUnitRouts`, `UnitReachedPosition`, `TimeElapsed`, `CavalryThreatHigh`.
- `ThenOrder` skal tilhøre samme eller lavere kommandoniveau end udstederen og kan aldrig give en enhed en mission, udstederen ikke selv kunne give.
- Spilleren kan bruge betingede ordrer som standard-ordrer ("stå fast, forfølg hvis de viger").

---

# 7. Combat slots og approach lanes

Bataljonschefen ejer en `FrontAllocator`, der fordeler front mellem kompagnier. Hvert kompagni får:

```text
ApproachLane    CombatSlot    FireSector    FallbackPosition    ReservePosition
```

Slots beregnes ved missionsskifte, ny fjendtlig front og brud, ikke hvert tick. Genberegn også ved:

- Kompagni under cohesion- eller mandskabstærskel.
- Hul større end fastsat bredde.
- Reserve indsættes.
- Mission eller fjendefront ændres.

Reserve udfylder huller før de øvrige kompagnier trækkes sidelæns (sidelæns træk under ild koster cohesion).

## 7.1 Fremrykning uden at krydse eget ildfelt

Hvis ét kompagni er i kontakt, må andre ikke gå gennem dets aktive ildfelt:

1. Det forreste kompagni binder fjenden.
2. Sidekompagnier får hver sin `ApproachLane`.
3. De bevæger sig uden om det aktive ildfelt.
4. De ruller ud på linje, får egne ildsektorer og holder sideafstand.

`ApproachLane` er en sti med bredde, ikke en vektor. Lanes gennem hegn, grøft, skov eller bebyggelse tvinger kolonne på strækket.

---

# 8. Ildfelt og friendly fire

Hvert kompagni har et aktivt ildfelt:

```text
FireCone  FriendlyUnitsInsideCone  FireBlocked  DeadGround  SmokeDensity
```

Ildkeglen er 3D: højdeforskel, død grund og røg blokerer eller forringer ild uden at blokere bevægelse.

Hvis en venlig enhed kommer ind foran (`FireBlocked = true`), skal kompagniet stoppe ild, flytte ildpunkt, vente på fri bane eller ændre formation/retning (hvis det ikke bryder en højere reaktion). Friendly-fire opløser ikke en carré og afbryder ikke en igangværende melee.

---

# 9. Operationsområde / leash

```text
AssignedPosition
AllowedRadius   ~100 m
ChargeLeash     100–150 m
PursuitLeash    0 medmindre orden siger andet
```

En lokal countercharge må: `Charge → RepelEnemy → Stop → Reform → Return`. Den må ikke: forfølge 800 m, angribe næste regiment eller forlade bataljonslinjen, medmindre en ny ordre fra bataljon eller højere siger det.

Kavaleriets forfølgelse har egen leash og stopper, når fjenden reformerer, terrænet lukker, hestene er brugt op, eller ordren udløber.

Leash-værdier er startværdier og kan skaleres efter epoke (kortere rækkevidder giver tættere kamp).

---

# 10. Threat-system (RETTET)

Hvert kompagni beregner løbende en lokal `ThreatScore` ud fra kendte kontakter, ikke ud fra alle enheder.

## 10.1 Normalisering

Hvert led normaliseres til 0–1 via en responskurve, og kombineres med vægte. Distance er relativ til fjendens effektive rækkevidde:

```text
RelDistance = Distance / EnemyEffectiveRange     (fra FEquipmentProfile)
DistanceThreat = CurveDistance(RelDistance)      aftagende kurve: tæt = høj
```

Led (alle 0–1): `DistanceThreat`, `EnemyStrength`, `EnemyFacingUs`, `EnemyMovingTowardsUs`, `EnemyFire`, `CavalryThreat`, `FlankThreat`, `ArtilleryThreat`, `ContactConfidence`, `ContactAgeDecay`.

```text
ThreatScore = Σ (Weight_i * Curve_i(Input_i))  skaleret til 0–100
              derefter * ContactConfidence * ContactAgeDecay
```

Kurver og vægte er data (tunes uden kodeændring). Gamle eller usikre kontakter nedskrives og kan kun udløse observation og frontberedskab, ikke carré eller charge.

## 10.2 Eksempler (relative)

```text
Fjende ved 2.5 × effektiv rækkevidde, bevæger sig sidelæns → LOW
Fjende ved 0.7 × effektiv rækkevidde, direkte mod os → HIGH
Sidst set for 90 sek, position usikker → nedskrives
```

---

# 11. Infanteri: lokal reaktion, bajonet og countercharge

| Situation | Automatisk reaktion |
|---|---|
| Fjende langt væk | Følg ordre |
| Fjende nærmer sig | Drej front / gør klar |
| Fjende inden for effektiv rækkevidde | Åbn kontrolleret ild |
| Fjende rykker hurtigt frem | Øg ildintensitet, hvis ammo tillader |
| Fjende kommer tæt på | Fiks bajonetter |
| Fjende tæt + svækket | Lokal countercharge mulig |
| Fjende tæt + stærkere | Hold / withdraw efter moral |
| Lav ammunition | Hold ild til kortere afstand |

At fikse bajonetter betyder ikke automatisk charge.

## 11.1 ASSAULT og LOCAL_COUNTERCHARGE

- **ASSAULT:** kræver overordnet offensiv ordre.
- **LOCAL_COUNTERCHARGE:** selvforsvar eller lokal taktik inden for leash.

```text
ChargeOpportunityScore (0–1-normaliserede led)
  + EnemyLowMorale + EnemyLowCohesion + EnemySuppressed + EnemyRetreating
  + ArtilleryTarget (NY, afsnit 13)
  + OwnHighMorale + OwnHighCohesion + FriendlySupport + OfficerChargeJudgement
  - EnemyStrength - EnemyFirepower - BadTerrain - FlankThreat
  - OwnFatigue - OwnLowAmmo
```

`EnemyFirepower` kommer fra udstyrsprofilen. Mod hurtig ild falder scoren automatisk.

Efter kampen: `STOP → REFORM → RETURN_TO_ASSIGNED_POSITION → RESUME_MISSION`.

Charge gennem grøft, tæt skov eller over bro er afvist (hård afvisning, ikke straf).

---

# 12. Kavaleritrussel og carré

```text
CavalryThreatScore = f(Distance/Speed, HeadingTowardsUs, Formation, OwnFormationWeakness, Terrain, ContactConfidence, ContactAge)
```

Eksempel (relativt til kavaleriets tilbagelagte tid, ikke meter):

```text
CAV går parallelt             → ingen særlig reaktion
CAV drejer mod os             → PREPARE_ANTI_CAV
CAV galopperer direkte mod os → FORM_CARRE
```

Carré må ikke udløses af en gammel rapport eller fordi fjendtligt kavaleri findes på kortet. Carré afvises i tæt skov, på dæmning og på bro (kolonne eller lokal dækning i stedet).

**Carré:** `FORM_CARRE` 15–40 sek (sårbar undervejs), `HOLD_CARRE`, `BREAK_CARRE`, `REFORM_LINE` 10–25 sek. Afbrudt formering = `BROKEN_FORMATION`.

Fordele: meget stærk mod kavaleri, dækker flere retninger. Ulemper: lav mobilitet, kompakt mål for artilleri/morter, dårligere fremadrettet ild, tid at forme og opløse.

**Hysterese:** `CavalryThreat > 70 → FORM_CARRE`, `< 30 i 20 sek → REFORM_LINE`. `FormationChangeCooldown = 10–20 sek`. Hysterese gælder også ildåbning, fallback og genoptagelse af mission.

---

# 13. Artilleri som trussel og som mål (NY / udvidet)

## 13.1 Hvorfor eget afsnit

Artilleri er både en trussel mod enheden og et attraktivt mål. Reglen: **enheden reagerer lokalt på truslen, og angriber kun artilleriet selv, hvis kriterierne nedenfor er opfyldt. Ellers meldes det opad.**

## 13.2 Lokal reaktion når man beskydes

Tilladt uden ordre: `SpreadFormation`, `SeekNearbyCover`, `IncreaseMovementSpeed`, `ChangeFacing`, `ShortDisplacement`. Spread skal valideres mod terræn (kan allerede være opfyldt).

Alle enheder, der observerer fjendtligt artilleri, **skriver kontakten** (`Source = OwnEyes`) og sender den opad. Det er grundlaget for counter-battery.

## 13.3 Hvornår må en enhed selv gå efter et fjendtligt batteri (`ENGAGE_BATTERY`)

`ENGAGE_BATTERY` er en form for `LOCAL_COUNTERCHARGE` (samme leash-regler, samme vægtlag). Kræver **alle**:

1. Batteriet er en kendt kontakt med tilstrækkelig `Confidence` og lav `ContactAge`.
2. Batteriet ligger inden for `ChargeLeash`.
3. Batteriet er **udsat**: ingen væsentlig fjendtlig infanteri- eller kavaleriskærm tæt på, og besætningen er ikke dækket.
4. `ChargeOpportunityScore` (inkl. `ArtilleryTarget`) over tærskel, og ingen hård afvisning (grøft, skov, bro, stærk stilling).
5. Kompagniet er ikke under en højere reaktion (rout, carré, melee, friendly-fire-fare).
6. Handlingen forlader ikke bataljonens front ud over leash.

Opfyldes kriterierne ikke, er svaret: **rapportér og anmod** (`COUNTER_BATTERY_REQUEST`), og lad brigade/division beslutte.

```text
ENGAGE_BATTERY → nå batteriet → neutralisér → STOP → REFORM → RETURN → RESUME_MISSION
```

Erobrede kanoner efterlades som objekter, der kan generobres (se 16.5).

## 13.4 Kavaleri mod artilleri

`ArtilleryAttack` er en kavaleri-rolle og artilleri er et højt prioriteret charge-mål (afsnit 18). Samme krav om udsathed og ingen hård afvisning. Kavaleriet må ikke jagte et batteri uden for sin pursuit-/charge-leash.

## 13.5 Hvem beslutter counter-battery?

Strategisk counter-battery (eget artilleri skal bekæmpe fjendtligt) er en **brigade- eller divisionsbeslutning**. Batterichefen må selv skifte til et nærgående mål, hvis det truer batteriet (afsnit 16.3).

---

# 14. Beskyttelse af eget artilleri (NY)

## 14.1 Problemet

Artilleri er sårbart i nærkamp. Uden beskyttelse bliver batterier enten ødelagt eller tvunget til at flygte. AI'en skal derfor både *planlægge* beskyttelse (top-down) og *reagere* på nær trussel (lokalt).

## 14.2 Beskyttelsestilstand pr. batteri

```text
EBatteryProtection
  Screened      egen skærm tæt på, ingen kendt trussel
  Exposed       ingen skærm, ingen kendt trussel
  Threatened    kendt fjende nærmer sig inden for alarmafstand
  OverrunImminent  fjende tæt på, ingen mulighed for flugt
```

Beregnes af batteriet ud fra kendte kontakter og kendte egne enheder inden for radius (ikke ud fra sandheden).

## 14.3 Planlagt beskyttelse (top-down)

- Ved placering vælger batteriet position med `InfantryProtection` (afsnit 16.2). Brigaden/bataljonen skal derfor planlægge en skærm.
- Ny mission **`GUARD_BATTERY`**: udstedes af bataljon eller højere til et kompagni (eller kavalerienhed). Enheden holder position tæt på batteriet, vender front mod den sandsynlige trussel og følger batteriet ved flytning.
- `FrontAllocator` tager `GUARD_BATTERY` med som en slot-type, så skærmen ikke overlapper andre slots.
- Divisionen kan prioritere batterier med skærm højere i artilleristøtteplanen.

## 14.4 Lokal beskyttelse (reaktion)

Når et batteri i nærheden er `Threatened`, må **ethvert kompagni inden for leash** reagere lokalt:

```text
Reaction: COVER_BATTERY
  Betingelser:
    - Batteriet er inden for AllowedRadius
    - Kompagniet kender truslen (egne øjne eller alarm, se nedenfor)
    - Kompagniet er ikke under en højere reaktion
  Handling:
    - Interposér mellem trussel og batteri
    - Vend front mod truslen, fordel ildsektor
    - Hold inden for leash
  Afslutning:
    - Trussel væk (hysterese) → ResumeMission
```

Det er **ikke** en ny offensiv mission. Det er forsvar af nærliggende eget materiel inden for leash.

**Alarm:** et batteri under trussel sender en `BatteryUnderThreat`-rapport opad (med latency), men enheder inden for en alarmradius ved batteriet får truslen **straks** (de kan se og høre den). Alarmradius er data og skaleres med epoke (lyd, røg, sigtbarhed).

**Kavaleri:** en kavalerienhed i nærheden kan tage `COUNTER_CHARGE_FOR_BATTERY` mod en nærgående fjende, inden for charge-leash og uden hård afvisning.

## 14.5 Batteriets egen beslutning når fjenden er tæt

Eksisterende logik (afsnit 16.4) udvides med skærm-status:

| Situation | Foretrukken reaktion |
|---|---|
| Skærm til stede og truslen er håndterbar | `HOLD_POSITION` + `FIRE_CANISTER` |
| Ingen skærm, tid og heste nok | `LIMBER_AND_RETREAT` |
| Ingen skærm, ingen tid, ingen flugtvej | `FIRE_CANISTER` til sidste øjeblik, derefter `ABANDON_GUNS` |
| Skærm i fare for at bryde | Rapportér + anmod forstærkning |

Tab af kanoner giver morale- og cohesion-straf til nærliggende enheder (egne kanoner var "tabt"), og kanonerne kan generobres.

## 14.6 Vægtlag (indsættes i afsnit 22)

```text
70  LOCAL_COUNTERCHARGE / ENGAGE_BATTERY
65  COVER_BATTERY
```

---

# 15. Ilddisciplin og drill

| Situation | Reaktion |
|---|---|
| Fjende langt væk | Hold ild |
| Fjende nærmer sig | Aim / Prepare |
| Fjende i effektiv range | Kontrolleret ild |
| Fjende meget tæt | Hurtigere ild |
| Fjende angriber | Maksimal ild |
| Lav ammunition | Reduceret ild, kortere åbningsafstand |
| Egne styrker foran | Stop ild |
| Tæt røg | Langsommere, mere usikker ild |

"Effektiv range" kommer fra `FEquipmentProfile`. Ammunition er et førsteklasses input.

**Drill (data, ikke ny AI-gren):** tidlig træning (første række skyder), bedre træning (rangskifte), senere (stående/knælende). Kan udvides med liggende, volley fire, fire by file/rank og skyttekæde styret af doktrin.

---

# 16. Artilleri-AI

## 16.1 State machine

```text
LIMBERED  MOVING  DEPLOYING  UNLIMBERED  AIMING  FIRING
REPOSITION  LIMBER_AND_RETREAT  ABANDON_GUNS
```

Deploy, limber og flytning har varighed. Et batteri under limbering er et dårligt mål at blive stående ved, og batterichefen ved det.

## 16.2 Placering

Batteriet søger: `HighGround`, `ClearLOS`, `ClearFireSector`, `InfantryProtection`, `SafeDistance`, `EscapeRoute`. Placering vælges ud fra kendt fjende og kendt egen front, ikke skjulte enheder. Position uden skærm markeres `Exposed` (afsnit 14).

## 16.3 Målkontrakt

Batteriet modtager et målområde og en prioritet, ikke "skyd på fjenden".

| Mål | Prioritet |
|---|---|
| Fjendtligt artilleri (hvis counter-battery beordret) | Meget høj |
| Tæt infanterimasse i observeret område | Meget høj |
| Infanteri i angreb | Høj |
| Kavaleri i angreb | Høj |
| Fjern spredt infanteri | Lav |
| Enhed bag solid dækning | Lav |

Batterichefen må selv skifte til et nærgående mål, der truer batteriet.

Ammunitionstype vælges efter afstand (relativt til udstyrets rækkevidder), måltype og resterende skud: lang afstand kugle/granat, mellem granat, kort kartæsk. Få skud tilbage: gem kartæsk til nærtrussel.

## 16.4 Under nærtrussel

Batterichefen vurderer uden at spørge opad: `CanisterEffective?`, `FriendlyInfantryNearby?`, `HorseAvailable?`, `TimeToLimber?`, `EscapeRouteAvailable?`, `CrewMorale?`, **`ProtectionState`** (afsnit 14). Mulige reaktioner: `FIRE_CANISTER`, `HOLD_POSITION`, `LIMBER_AND_RETREAT`, `ABANDON_GUNS`. Limbering, der ikke kan nås, vælges ikke, bare fordi retreat står i planen.

## 16.5 Efterladte kanoner

`ABANDON_GUNS` efterlader kanoner som objekter. De kan generobres af egne styrker og erobres af fjenden. Besiddelse af kanoner påvirker nærliggende enheders morale.

---

# 17. Morter

Morteren giver indirekte ild og kræver ikke direkte LOS, men kræver en observeret målposition med frisk rapport.

Observatører: `HQ`, `Officer`, `Infantry`, `Cavalry`, `Scout`. Hvis spotteren dør, mister sigt, eller rapporten bliver for gammel, stopper indirekte ild eller går over til sidst kendte punkt med stærkt nedsat prioritet. Morteren finder ikke selv et nyt strategisk mål.

| Egenskab | Morter |
|---|---|
| Flytning | Meget langsom |
| Opsætning | Langsom |
| Skudhastighed | Lav |
| Præcision | Lav/moderat |
| Effekt mod tæt infanteri | Høj |
| Effekt mod spredt infanteri | Lavere |
| Minimum range | Ja |
| Kræver direkte LOS | Nej |
| Kræver observeret mål og frisk rapport | Ja |

Typiske mål: redoubts, bygninger, infanterimasse, artilleriposition, forsvarsstilling. Målkontrakt: område, prioritet, observatør-id, rapportalder. Morter-opsætning er `Exposed` uden skærm og følger afsnit 14.

---

# 18. Kavaleri-AI

Roller: `Recon`, `Screening`, `Flanking`, `Pursuit`, `ArtilleryAttack`, `RearAttack`, `Messenger`, `HQSupport`.

Kavaleri vurderer: `EnemyFormation`, `EnemyFacing`, `EnemyReadiness`, `Terrain`, `Distance`, `FriendlySupport`, `EscapeRoute`, `HorseFatigue`, **`EnemyFirepower`** (fra udstyr).

Rekognoscering skriver kontakter opad. Den angriber ikke, medmindre chargescoren er høj eller ordren er et angreb.

**Godt mål:** `EnemyFlankExposed`, `EnemyDisorganized`, `EnemyRetreating`, `EnemyArtillery` (hvis udsat, afsnit 13), `EnemyRear`.

**Hård afvisning:** klar infanterifront, carré, tæt skov, grøft, bro, stærk forsvarsstilling.

```text
ChargeScore = EnemyDisorganized + EnemyFlankExposed + EnemyRetreating + ArtilleryTarget + Surprise
            - EnemyFormationStrength - EnemyFirepower - TerrainPenalty - EnemySupport - HorseFatigue
```

Hvis `ChargeScore > Threshold` og ingen hård afvisning: charge. Ellers `SCREEN`, `WAIT`, `REPOSITION`, `HARASS`.

**Mod carré:** `CIRCLE`, `HARASS`, `WITHDRAW`, `CALL_ARTILLERY`, `WAIT`. `CALL_ARTILLERY` er en rapport og en anmodning, ikke en garanti.

Roller som `Messenger` er ordonnans-rollen i afsnit 6.

---

# 19. Bataljon, brigade, division

**Bataljonschef** (den vigtigste lokale taktiske officer) modtager `CaptureArea`, `DefendArea`, `HoldLine`, `AttackEnemy`, `Withdraw` og beregner `BattleFront`, `AttackAxis`, `CompanySlots`, `Reserve`, `SupportDirection`, `FallbackLine`, **`GuardAssignments`** (afsnit 14). Han ejer `FrontAllocator` og micromanager ikke ild hvert tick.

**Reserve:** bruges til `FILL_GAP`, `EXPLOIT` (kræver bataljonsordre), nødstøtte, flanketrussel, gennembrud. Et reservekompagni vælger ikke selv forfølgelse.

**Brigadechef** vælger `MainAttack`, `SupportAttack`, `Reserve`, `DefensiveSector`, `ArtillerySupport`. Artilleristøtte er målområde + prioritet, ikke sigtepunkter.

**Divisionschef** vurderer ud fra rapporter: `Objective`, `KnownEnemyStrength`, `KnownFlanks`, `Terrain`, `BrigadeStrength`, `Artillery`, `Cavalry`, `Morale`, `Reserve`, `ReportAge`. Counter-battery og morterobservatører hører her eller hos brigaden.

**Reevaluering:** hvert 5.–10. sekund for taktiske officerer. Valg: `CONTINUE`, `REINFORCE`, `STOP`, `MOVE_RESERVE`, `CHANGE_TARGET`, `WITHDRAW`, `SHIFT_FRONT`. En ny plan superseder gamle ordrer og sletter ikke en igangværende reaktion.

**Reaktioner pr. niveau:**

| Situation | Bataljon | Brigade | Division |
|---|---|---|---|
| Kompagni mister mange mænd | Send reserve, genberegn slots | Observer | Normalt ingen |
| Fjende på flanken | Drej kompagnier | Flyt bataljon | Evt. flyt brigade |
| Fjendtligt artilleri | Spred lokale enheder | Tildel målområde til eget artilleri | Prioritér counter-battery |
| Eget batteri truet | Omfordel skærm / `GUARD_BATTERY` | Send forstærkning | Prioritér batteri |
| Kavaleritrussel | Anti-CAV | Reserve klar | Eget kavaleri |
| Gennembrud | Udnyt lokalt | Send reserve | Forstærk angreb |
| Egen linje bryder | Stabiliser | Reserve | Tilbagetrækning |
| Fjenden flygter | Fremrykning inden for front | Forfølg, hvis beordret | Kavaleri frem |
| Spotter tabt | Rapport op | Ny observatør eller stop indirekte ild | Omprioritér morter |

Når en node er `Player` (afsnit 3), springes dens AI-planlægning over, men nodens lokale reaktioner og underordnede AI kører uændret.

---

# 20. Moral, cohesion, fatigue, suppression og ammunition

```text
Morale 0–100    Cohesion 0–100    Fatigue 0–100    Suppression 0–100
Ammunition (skud eller relativ)    HorseFatigue 0–100
```

**Moral:** 80–100 normalt, 50–79 reduceret aggressivitet, 30–49 tøver, 15–29 withdrawal sandsynlig, 0–14 rout. Officerer påvirker morale og rally, hvis `OfficerPresent`.

**Moralsmitte:** lokalt (~80 m) fra `Rout`, `OfficerFallen`, `NeighborFleeing`, `StandardLost`, **`GunsLost`** (NY). Smitten er begrænset og har cooldown, så én rout ikke tømmer en brigade på ét tick.

**Cohesion** er noget andet end moral og smitter ikke. Påvirkes af broovergang, skov, charge, retreat, artilleriild, hurtige formationsskift, store tab og afbrudt formation. Under tærskel kan AI vælge `REFORM`.

Disse påvirker: bevægelse, reload, formationstid, charge-villighed, accuracy, retreat-risiko, rally, ordreeksekvering, og om limbering/forfølgelse er mulig. Fatigue er prisen for charge, løb og gentagne formationsskift og falder ved reform og hold.

---

# 21. Terræn, broer og smalle passager

| Formation | Krav |
|---|---|
| Line | Frontage og nogenlunde jævnt terræn |
| Column | Default i skov, på vej og over bro |
| Carré | Afvises i tæt skov, på dæmning og på bro |
| Skirmish | Kræver doktrin eller særlig ordre |
| Spread | Kræver plads; kan allerede være opfyldt af terræn |

Smalle passager bruger kø: første enhed krydser, rydder og reformerer, før næste krydser (broer, vadesteder, landsbygader, dæmninger, skovbryn). `ApproachLane` kender passagebredde og tvinger kolonne på strækket og reform bagefter.

---

# 22. Prioritet og arbitrering

Brug lag, ikke flade tal:

```text
1. Overlevelse der ikke kan udskydes: ROUT, pågående melee, venner i skudlinjen
2. Formationsreaktion med cooldown: carré, spread, cover, fallback
3. Lokal ild, countercharge og beskyttelse inden for leash
4. Mission
```

Vægte inden for samme lag:

```text
100 ROUT
95  IMMEDIATE_CAVALRY_CHARGE
90  MELEE_SELF_DEFENCE
85  FRIENDLY_FIRE_DANGER
80  TAKE_COVER_IMMEDIATE
70  LOCAL_COUNTERCHARGE / ENGAGE_BATTERY
65  COVER_BATTERY                       (NY)
60  ENGAGE_LOCAL_THREAT
50  FORMATION_CORRECTION
40  REFORM
20  CURRENT_MISSION
```

Normalt udfører kompagniet `CURRENT_MISSION`. En højere-lag-trussel overtager midlertidigt. Når truslen er væk og hysterese er opfyldt: `ResumeMission()`. Friendly-fire stopper ild uden at opløse carré. Carré afbryder fremrykning uden at slette missionen (`SuspendedMission` gemmes).

---

# 23. Officerens kvalitet

Officerens kvalitet påvirker beslutninger direkte, ikke kun kampværdier:

```text
Mindre erfaren:  ReactionTime 8 s,  ThreatAccuracy 60%, ChargeJudgement 50%, OrderInterpretError høj
Erfaren:         ReactionTime 2 s,  ThreatAccuracy 90%, ChargeJudgement 85%, OrderInterpretError lav
```

En dårlig officer: reagerer for sent på kavaleri, vælger dårlig formation, charger på dårligt tidspunkt, glemmer at reformere, åbner ild for tidligt/sent, tolker målområde for bredt, **glemmer at beskytte et nærliggende batteri**.

En god officer: opdager trusler tidligere, bruger cover, vurderer charge korrekt, opløser carré til rette tid, genoptager mission hurtigere, **opdager truede batterier tidligt**.

`OfficerPresent` er et særskilt flag. Falder kaptajnen, overtager næste officer med dårligere tal indtil reorganisering.

---

# 24. Doktrin som data (ÆNDRET)

Samme state machine, forskellige tal. Doktrin er nu to uafhængige lag, der kombineres:

```text
FDoctrineProfile      (nation / hær)
  PreferredFormation  OpenFireDistanceRel  FireRate  SkirmishTendency
  ChargeThreshold     AntiCavalryBias      ColumnBiasInCover  AmmoDisciplin
  BatteryProtectionBias (NY)               ArtilleryHuntBias (NY)

FEquipmentProfile     (udstyr/epoke, afsnit 2)
```

`OpenFireDistanceRel` er en andel af `EffectiveRange`, så samme doktrin virker i alle epoker.

Retningsgivende eksempler (startværdier, kalibreres):

- Linjeenheder med forladere og tæt formation: tættere formation længere, lavere skudtakt, mere samlet volley.
- Enheder med hurtigere ild eller bagladere: tidligere ild, højere skudtakt, større tendens til skyttekæde.

Profilen må ikke give kompagniet lov til at opfinde en ny mission. Den ændrer kun, hvordan en ordre udføres.

---

# 25. Sværhedsgrad uden snyd (NY)

AI'en må ikke læse skjulte data på nogen sværhedsgrad. Sværhedsgrad styres via dataparametre:

| Parameter | Lettere AI | Sværere AI |
|---|---|---|
| Officer-kvalitet (reaktionstid, vurdering) | Lav | Høj |
| Rapportforsinkelse (latency) | Længere | Kortere |
| Reevalueringsinterval | Længere | Kortere |
| Brug af betingede ordrer | Sjældent | Ofte |
| Reserve-disciplin og slot-genberegning | Træg | Hurtig |
| Doktrin-tuning (fx koordinering af kavaleri og artilleri) | Enkel | Avanceret |

Valgfri spiller-hjælp ("Generøs fog") kan give spilleren mere viden, men skal være en eksplicit indstilling og gælde spilleren, ikke give AI'en skjult information.

---

# 26. Ydelsesbudget (NY)

Tung AI-logik må ikke køre på alle enheder hvert tick.

| Niveau | Kadence (startforslag) |
|---|---|
| Soldat | Ingen AI (animation, LOD, formation følger kompagniet) |
| Kompagni-reaktion | 2–4 Hz, forskudt (staggered) via scheduler |
| Kompagni-mission | ~1 Hz |
| Bataljon/brigade/division | 5–10 s genvurdering + event-drevne (brud, hul, ny kontakt) |
| Kontakt-ældning | Langsom decay + event-opdatering |
| LOS-/ildkegleforespørgsler | Budgetterede, cachede, asynkrone hvor muligt |
| Slots/lanes | Kun ved event (aldrig hvert tick) |

Faste tick-budgetter pr. frame, og en scheduler der udskyder lavprioritetsarbejde. Overvej Unreals StateTree/Behavior Trees til mission- og reaktionslagene og EQS til slot- og positionsvalg. Vurder i forhold til jeres setup.

---

# 27. Debug, tuning og test (NY)

## 27.1 Debug-overlay (byg tidligt)

Skal kunne vise pr. enhed:

- Aktiv mission, suspenderet mission og reaktion.
- Kontakter med alder og tillid (usikkerhedsområder).
- Slots, lanes, ildkegler, leash-radier.
- Ordrer under vejs med status.
- Artilleriets `ProtectionState` og skærm.
- `ThreatScore`-led med bidrag pr. led.

## 27.2 Beslutningslog

Hver beslutning logges med årsag (inputværdier, vinder, afviste alternativer). Det gør tuning og fejlretning mulig uden at gætte.

## 27.3 Determinisme

Seedet RNG pr. slag og faste AI-tidsskridt, så scenarier kan afspilles og sammenlignes.

## 27.4 Acceptscenarier

| Test | Forventning |
|---|---|
| T1: Kavaleri 200 m væk mod kompagni i linje | Carré inden for X sek, reform efter 20 sek lav trussel, mission genoptages |
| T2: Tre kompagnier på 300 m front | Ingen overlappende slots, ingen går gennem andres ildkegle |
| T3: Ét kompagni router | Naboer inden for ~80 m får morale-hit, cooldown, ingen kædekollaps |
| T4: Ordonnans dræbt / officer faldet | Ordren når aldrig frem / forsinkes |
| T5: Batteri med skærm vs. uden, fjende nærmer sig | Med skærm: `COVER_BATTERY`/hold. Uden: limber eller opgiv |
| T6: Udsat fjendtligt batteri inden for leash vs. skærmet | Udsat: `ENGAGE_BATTERY`. Skærmet: rapportér + anmod counter-battery |
| T7: Bro | Kø fungerer, ingen krydser samtidigt |
| T8: Samme scenarie i 1825 og 1864 | Forskellige engagementafstande og taktik, ingen kodegrene |
| T9: Spiller/AI-overgang midt i slaget | Ingen ordre tabes, ingen øjeblikkelig perfekt overtagelse |
| T10: Betinget ordre ("forfølg hvis fjenden viger") | Forfølgelse starter uden ny ordre, evalueret lokalt |

---

# 28. Eksempel på samlet angreb

Styrke: 3 infanterikompagnier, 1 batteri, 1 kavalerienhed, 1 morterenhed.

1. **Recon:** kavaleri spejder og skriver kontakter opad med forsinkelse.
2. **Fire preparation:** kanoner får målområde. Morter skyder kun mens observatørens rapport er frisk. Batteriet placeres med skærm (`GUARD_BATTERY`).
3. **Infanteri frem:** tre kompagnier ad hver sin lane. Passage tvinger kolonne.
4. **Kontakt:** KMP1 binder, KMP2/KMP3 tager flankerne. Kompagnier reagerer lokalt (kavaleri, flanke, moral, ammo, truet batteri).
5. **Gennembrud:** hvis fjenden bryder og det er rapporteret (eller en betinget ordre udløses), foretager kavaleriet `PURSUIT` inden for leash. Infanteriet følger kun, hvis overordnet ordre tillader det.

Spilleren kan gennem hele forløbet give ordrer som øverste led, eller sætte en eller flere noder på `Auto`.

---

# 29. Implementeringsrækkefølge (REVIDERET)

## Fase 1 – Kontrakter (lås dem først)

```text
FOrder (latency, status, supersede, betingelser)
FContact (alder, tillid, kilde)
FCombatSlot
FCompanyBlackboard
FEraProfile / FEquipmentProfile
```

## Fase 2 – Vertikal skive (spil og test før I går videre)

1. Ét kompagni mod ét kompagni: `MissionAI` + `ReactionAI` + `SuspendedMission` + kontakter + ordre-latency.
2. En bataljon (3–4 kompagnier) med `FrontAllocator`, lanes og ildfelter/friendly-fire.
3. Debug-overlay, beslutningslog og acceptscenarier T1–T3, T7.
4. Spillerens ordre-UI og `Auto`-toggle på ét niveau (T9).

**Stop her og playtest**, før artilleri, morter og kavaleri bygges.

## Fase 3 – Specialenheder

5. Artilleri-AI (inkl. beskyttelse, afsnit 14, T5–T6).
6. Kavaleri-AI (inkl. artilleri-angreb).
7. Morter-AI.

## Fase 4 – Højere niveauer

8. Brigade- og divisions-AI oven på de fungerende lag. De læser rapporter, ikke sandheden.
9. Betingede ordrer og sværhedsparametre (afsnit 25).
10. Epoke-swap-test (T8) og balancering af kurver/vægte.

---

# 30. Foreslåede Unreal-datastrukturer

Enums: `EUnitAIState`, `ECompanyMission`, `ECompanyReaction`, `EFormationType`, `EThreatType`, `EOfficerOrderType`, `EOrderStatus`, `EContactSource`, `EAmmoType`, `EControlMode`, `EBatteryProtection`.

Structs: `FCombatSlot`, `FFireSector`, `FThreatAssessment`, `FCompanyMissionData`, `FCompanyReactionData`, `FCompanyBlackboard`, `FOrder`, `FOrderCondition`, `FContact`, `FBattalionPlan`, `FBrigadePlan`, `FDivisionPlan`, `FDoctrineProfile`, `FEraProfile`, `FEquipmentProfile`.

```cpp
struct FOrderCondition
{
    EOrderConditionType Type;   // EnemyBreaks, AmmoBelow, TimeElapsed, ...
    float Threshold;
};

struct FOrder
{
    EOfficerOrderType Type;
    FVector TargetArea;
    float IssuedAt;
    float ArriveAt;
    EOrderStatus Status;
    int32 IssuerId;
    TArray<FOrderCondition> Conditions;  // NY
    int32 ThenOrderId;                   // NY, indeks til betinget efterfølgende ordre
};

struct FCompanyBlackboard
{
    ECompanyMission Mission;
    ECompanyMission SuspendedMission;
    ECompanyReaction Reaction;
    float Morale;
    float Cohesion;
    float Fatigue;
    float Suppression;
    float Ammunition;
    bool bOfficerPresent;
    float FormationCooldown;
    int32 GuardedBatteryId;              // NY, -1 hvis ingen
    EBatteryProtection NearbyBatteryStatus; // NY
};

struct FEquipmentProfile
{
    float EffectiveRange;
    float MaxRange;
    float ReloadTime;
    float MinRange;
    TArray<EAmmoType> AmmoTypes;
    // AccuracyByRange: kurve (UCurveFloat)
};

struct FCombatSlot
{
    FVector Center;
    FVector Forward;
    float Frontage;
    float Depth;
    int32 CompanyId;
    int32 LaneId;
};
```

---

# 31. Grundregel og opsummering

> **Missionen kommer ovenfra. Reaktionen sker lokalt. Information er lokal og ældes. Epokeforskelle er data.**

Kompagniet må selv: forsvare sig, skyde mod kendte mål, stoppe ild, fikse bajonetter, countercharge lokalt inden for leash, danne carré ved reel kavaleritrussel, søge dækning, reformere, reagere på flankeangreb, **beskytte nærliggende eget artilleri inden for leash**, **angribe udsat fjendtligt artilleri inden for leash når kriterierne er opfyldt**, foretage kort taktisk tilbagetrækning, og suspendere og genoptage missionen.

Kompagniet må ikke selv: vælge et nyt strategisk mål, angribe en fjern enhed af egen drift, forlade bataljonens front, forfølge langt uden ordre, opgive sin mission uden tungtvejende taktisk årsag, læse skjulte fjender eller skrive en ny mission.

Systemet bygger på:

1. Hierarkiske ordrer med latency, og betingede ordrer.
2. Spilleren som øverste led med `Auto`-toggle på ethvert niveau.
3. Lokalt taktisk initiativ, ingen selvstændige offensive missioner på kompagniniveau.
4. Kontakter med alder, tillid og kilde, for AI og spiller.
5. Combat slots og approach lanes, genberegnet ved brud.
6. Fire sectors og friendly-fire-kontrol.
7. Normaliserede, data-drevne trusselsvurderinger relativt til våbenrækkevidde.
8. To tilstandslag: mission og reaktion, med lagdelt arbitrering.
9. Bajonet/countercharge og carré som lokale beslutninger.
10. Aktiv beskyttelse af eget artilleri og kontrolleret jagt på udsat fjendtligt artilleri.
11. Morale, cohesion, fatigue, suppression og ammunition.
12. Officerkvalitet og `OfficerPresent`.
13. Artilleri, morter og kavaleri med egne state machines og målkontrakter.
14. Epoke og doktrin som data på samme maskine.
15. Sværhedsgrad via parametre, ikke snyd.
16. Ydelsesbudget, debug-overlay, beslutningslog og acceptscenarier.
