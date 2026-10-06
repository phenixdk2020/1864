# PROJECT 1864 — B-270–B-279 Tactical Prototype Next Sequence

**Status:** Aktiv implementeringsrækkefølge / roadmap  
**Branch:** `channel-test`  
**Formål:** Fastlægge den næste konkrete rækkefølge for den taktiske prototype, så fundamentet valideres før nye våbenarter og større AI-kompleksitet tilføjes.

Dette dokument er et **implementerings-roadmap** oven på den eksisterende `PROJECT-BACKLOG.md`. Det erstatter ikke de eksisterende design-backlogs for Officer AI, dragoner eller artilleri; det bestemmer rækkefølgen, hvori vi nu vil gøre dem testbare.

## Prioritetsregel

Vi går som udgangspunkt **ikke videre til næste gate**, før den foregående er stabil nok til at kunne bruges som fundament. Små UI-/compile-fixes må naturligvis laves undervejs.

---

## B-270 — Stabil destination/pathfinding for formationer

**Status:** AKTIV — højeste prioritet

### Mål

En enhed skal kunne få en destination eller et angrebsmål og selv finde en lovlig brugbar rute uden at:

- gå gennem floder uden lovlig overgang;
- gå gennem hårde bygninger/objekter;
- sidde fast ved et obstacle;
- få sin rute overskrevet af `ANGREB`/AI hvert frame;
- oscillere mellem waypoints/phases;
- miste den oprindelige slutdestination efter en omvej.

### Regler

- Route authority skal være entydig: missionen angiver **hvor** enheden skal hen; pathfinding bestemmer **hvordan** den kommer derhen.
- `MOVE`, `ANGREB` og AI-officer-ordrer skal bruge samme grundlæggende route-system.
- Flod/bro er hard constraint.
- Godkendte bygninger er hard blockers og skal omgås automatisk.
- Pathfinding skal være formation-level; vi vil ikke have individuel NavMesh-agent pr. soldat.
- Line/Column skal indgå i clearance og passagebredde.
- Broer/smalle passager må midlertidigt kræve Column.
- Når obstacle er passeret, skal original mission genoptages automatisk.

### Exit-kriterier

1. MOVE til punkt på modsatte side af floden finder broen og krydser stabilt.
2. ANGREB mod enhed på modsatte side finder samme lovlige bro-rute.
3. MOVE gennem Farmhouse/Barn laver stabil omvej og fortsætter til original destination.
4. ANGREB med bygning mellem angriber og mål går udenom og beholder målet.
5. Ingen normal bevægelse er afhængig af "restore last safe position" som primær navigation.
6. Enheden kan passere flere efterfølgende blockers uden loop/oscillation.
7. Pathfinding virker både med AI UNIT OFF og AI UNIT ON.

**Afhængighed:** eksisterende B-018 Formation-level pathfinding.

---

## B-271 — Kalibrér ranged combat: Close / Medium / Long

**Status:** PLANLAGT — næste gate efter B-270

### Mål

Måle og justere hvor mange mænd et ca. 190-mands kompagni typisk rammer pr. salve på:

- Close;
- Medium;
- Long.

### Testprincip

- Morale holdes fortsat ude af testen, så vi måler ren lethality først.
- Cohesion/andre aktive faktorer skal logges, så de kan identificeres som påvirkning.
- Samme formation, samme mål, samme styrke og flere gentagelser pr. afstand.
- Vi skal bruge gennemsnit og spredning, ikke én enkelt salve.

### Data der skal registreres

For hver salve mindst:

- faktisk afstand;
- band: CLOSE / MEDIUM / LONG;
- antal skytter/eligible firing men;
- antal hits;
- target strength før/efter;
- evt. accuracy multiplier;
- reload/cadence.

### Exit-kriterier

1. Vi har et reproducerbart testsæt med flere salver pr. afstand.
2. Close rammer tydeligt mere end Medium; Medium mere end Long.
3. Hit-rate vurderes mod ønsket gameplay/historisk plausibilitet.
4. QA-accuracy justeres eller erstattes med endelige prototypeværdier.
5. Vi beslutter eksplicit om 35/70/100 m fortsat er gode gameplay-bands.

---

## B-272 — Infantry melee baseline

**Status:** PLANLAGT

### Mål

Indføre første kontrollerede nærkamp mellem infanterienheder, før dragoner tilføjes.

### Første scope

- Melee starter kun ved reel fysisk kontakt / meget kort engagement distance.
- Enheden skal kunne få en `MELEE/CHARGE`-tilstand uden at teleporte formationer sammen.
- Resultat påvirkes mindst af:
  - antal mænd der faktisk kan komme i kontakt;
  - cohesion;
  - fatigue senere;
  - experience/training senere;
  - flank/rear-contact senere;
  - officer/morale senere når morale-QA-lock fjernes.
- Casualties skal være separate fra ranged-fire resolution.
- En side skal kunne bryde kontakt, rout'e eller vinde nærkampen.

### Exit-kriterier

1. To infanterikompagnier kan lukke til fysisk kontakt uden overlap/teleport.
2. Melee giver gradvise casualties og en læsbar udfaldsstate.
3. Enheden bliver ikke stående permanent inde i modstanderens formation.
4. Ranged fire og melee-state konkurrerer ikke om samme frame/order authority.

---

## B-273 — Fysisk HQ-enhed / AI-officer styrer ét kompagni

**Status:** PLANLAGT

### Mål

Flytte officer-AI fra primært at være en skjult controller til en **fysisk HQ-/officerenhed**, der har ansvar for én underordnet enhed.

### Første scope

HQ/officeren skal mindst kunne få mission:

- `DEFEND`;
- `ATTACK`;
- evt. `MOVE/HOLD` som støttefunktion.

Officeren skal oversætte missionen til ordre til underenheden og bruge de samme globale regler for:

- pathfinding;
- formation;
- range/fire policy;
- melee;
- terrain legality.

### HQ-state

Første prototype bør mindst have:

- officer identity/profile;
- doctrine;
- current mission;
- current subordinate;
- reaction/decision timer;
- current order/reason telemetry;
- fysisk position på kortet.

### Exit-kriterier

1. Spilleren kan give HQ missionen `Forsvar` eller `Angrib`.
2. HQ giver selv passende ordrer til ét kompagni.
3. Kompagniet kan gennemføre missionen uden direkte spiller-micromanagement.
4. HQ bruger B-270 pathfinding og B-271/B-272 combat-regler frem for særlogik.

**Relateret eksisterende backlog:** B-170–B-179 Officer AI/delegation og B-190–B-199 officerstats.

---

## B-274 — Ét HQ styrer to infanterikompagnier

**Status:** PLANLAGT

### Mål

Tilføje endnu ét kompagni og bevise, at én officer kan koordinere **to underenheder**.

### Officerens nye ansvar

- fordele missioner mellem kompagnier;
- undgå at begge vælger præcis samme plads;
- holde brugbar spacing/frontage;
- kunne vælge fx:
  - begge i line;
  - ét fremme + ét i reserve;
  - venstre/højre sektor;
  - koncentreret angreb;
- reagere hvis ét kompagni bliver slået tilbage/routed.

### Exit-kriterier

1. HQ kan flytte to kompagnier uden formations-overlap.
2. Ved Forsvar fordeles de på to brugbare positioner eller frontslots.
3. Ved Angreb kan de koordinere samme mål uden at stå oven i hinanden.
4. Mindst én simpel reserve-/support-regel virker.
5. Telemetry viser hvorfor officer valgte de to underordnedes roller.

---

## B-275 — Dragoons: mounted / dismounted / fire / remount

**Status:** PLANLAGT

### Mål

Implementere dragoner som første nye våbenart efter infanteri/HQ-baselinen.

### Første states

- `MOUNTED_MOVE`
- `MOUNTED_HOLD`
- `DISMOUNTING`
- `DISMOUNTED_FIRE`
- `REMOUNTING`
- `MOUNTED_MELEE/CHARGE` via B-276

### Regler

- Dragoner kan bevæge sig mounted med højere mobilitet.
- De kan sidde af og føre ildkamp til fods.
- Hestene forsvinder ikke: horse-holder state skal eksistere.
- Dismount/remount tager tid.
- Remount kræver adgang til de efterladte heste.
- Dismounted dragoner bruger ranged-combat-systemet; de skal ikke have en separat magisk combat-model.

### Exit-kriterier

1. En dragoon-enhed kan ride til position.
2. Den kan sidde af med synlig/state-baseret overgang.
3. Dismounted tropper kan skyde.
4. Heste/horse-holder-position bevares.
5. Enheden kan vende tilbage og remounte.

**Relateret eksisterende backlog:** B-040–B-043 og B-240–B-249.

---

## B-276 — Dragoon melee / mounted combat

**Status:** PLANLAGT

### Mål

Udvide B-272 melee, så dragoner kan kæmpe både mounted og dismounted.

### Første scope

- Mounted charge/melee mod passende mål.
- Dismounted melee bruger infanteriets fælles melee-core.
- Mounted charge skal være meget afhængig af:
  - target formation/state;
  - target facing;
  - target cohesion;
  - egen cohesion/fatigue;
  - terrain;
  - momentum/approach distance.
- Frontalt charge mod steady formed infantry skal være risikabelt.
- Routed/disordered infantry skal være langt mere sårbart.

### Exit-kriterier

1. Mounted og dismounted melee bruger samme grundlæggende casualty/state framework.
2. Mounted charge føles tydeligt anderledes end almindelig infantry melee.
3. En charge kan bryde af eller mislykkes; den er ikke automatisk succes ved kontakt.

---

## B-277 — AI-officer lærer at styre dragoner

**Status:** PLANLAGT

### Mål

HQ/officer-systemet fra B-273/B-274 skal kunne bruge dragoner uden særskilt scripted demo-logik.

### Officerens valg

AI skal mindst kunne beslutte:

- hvornår dragoner bør forblive mounted;
- hvornår de skal dismounte og skyde;
- hvornår remount giver mening;
- hvornår melee/charge er egnet;
- hvornår enheden skal undgå et dårligt frontalt charge;
- senere: recon/screen/flank/pursuit.

### Exit-kriterier

1. Officer kan give dragoon-enheden en meningsfuld mission.
2. Mounted/dismounted-valg er situationsbestemt og synligt i telemetry.
3. AI kan gennemføre mindst én offensiv og én defensiv dragoon-test uden spiller-micromanagement.

---

## B-278 — Kanonbatteri: første taktiske prototype

**Status:** PLANLAGT — efter dragoon/HQ-gaten

### Mål

Implementere første rigtige artillery battery-enhed.

### Første scope

- fysisk batteri med kanoner + crew;
- limbered/unlimbered state;
- flytning;
- deploy/unlimber tid;
- manuel måludpegning som standard;
- Hold Fire;
- første ammunitionstype(r);
- range/LOS;
- casualties på crew/battery state senere efter basal ild virker.

### Exit-kriterier

1. Batteriet kan flyttes til en position.
2. Det kan unlimber/deploye.
3. Spilleren kan vælge et mål manuelt.
4. Batteriet skyder kun når state/range/LOS tillader det.
5. Hold Fire virker.
6. Batteriet kan limber og flytte igen.

**Relateret eksisterende backlog:** B-030–B-039.

---

## B-279 — AI/HQ integration af kanonbatteri

**Status:** PLANLAGT — efter B-278

### Mål

Når batteriets manuelle mekanik virker, skal HQ/officer-AI kunne placere og anvende artilleri uden cheats.

### Første AI-scope

- vælge en brugbar firing position;
- holde passende afstand;
- undgå at placere batteriet bag blocker/uden LOS;
- vælge hvornår der skal unlimberes;
- respektere Hold Fire/mission;
- senere vælge mål efter fire-control doctrine og ammunitionstype.

### Exit-kriterier

1. HQ kan flytte batteriet til en fornuftig position.
2. Det deployer og engagerer uden direkte player micro.
3. AI bruger samme range/LOS/pathfinding som spilleren.
4. AI får ingen skjult ekstra accuracy, range eller information.

---

# Samlet rækkefølge

| Gate | Backlog | Emne | Må være stabil før næste? |
| ---: | --- | --- | --- |
| 1 | B-270 | Destination/pathfinding | **Ja** |
| 2 | B-271 | Close/Medium/Long hit-rate | **Ja** |
| 3 | B-272 | Infantry melee | **Ja** |
| 4 | B-273 | HQ/AI officer → 1 kompagni | **Ja** |
| 5 | B-274 | HQ/AI officer → 2 kompagnier | **Ja** |
| 6 | B-275 | Dragoons mounted/dismounted/fire/remount | **Ja** |
| 7 | B-276 | Dragoon melee/charge | **Ja** |
| 8 | B-277 | AI officer styrer dragoner | **Ja** |
| 9 | B-278 | Kanonbatteri baseline | **Ja** |
| 10 | B-279 | AI/HQ styrer kanonbatteri | Senere gate |

## Bevidst udskudt

Følgende må ikke trække fokus fra denne sekvens medmindre de er nødvendige dependencies eller regression-fixes:

- avanceret bykamp/bygningsbesættelse;
- brand/kollaps/ruinsystem;
- strategiske campaign-features;
- fuld fog-of-war;
- avanceret logistics/supply;
- kosmetiske nice-to-have features.

De er fortsat i projektbackloggen, men er ikke del af den aktuelle kritiske sti.
