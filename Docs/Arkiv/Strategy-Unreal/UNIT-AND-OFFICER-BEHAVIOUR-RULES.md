# PROJECT 1864 — Enheds- og AI-officerregler

**Status:** Kanonisk adfærdsreference  
**Prototypebaseline:** v00.00.09f30x TEST  
**Formål:** Dette dokument samler de regler, der bestemmer hvordan enheder, formationer, kampordrer og AI-officerer skal opføre sig. Når runtime-kode og dette dokument er uenige, skal afvigelsen behandles som en bug eller som en eksplicit ny designændring.

> Bemærk: Den nuværende prototype bruger fortsat klassenavnet `Regiment`, men den aktive test-enhed repræsenterer i praksis et **kompagni på ca. 190 mand**.

---

## 1. Prioritet mellem regler

Når flere systemer vil styre samme enhed, gælder denne prioritet fra høj til lav:

1. **Rout/panik og overlevelse** — en routed enhed følger flugtlogik.
2. **Terrænsikkerhed** — åbent vand må ikke krydses uden lovlig bro/overgang.
3. **Eksplicit spillerordre** — fx ANGREB, FORSVAR HER, EROBR HER eller TRÆK BAGLÆNS.
4. **Aktiv missionslogik / højere command authority** — fx Major-owned mission placement, bro-routing eller fighting withdrawal.
5. **Midlertidig lokal kampreaktion** — fx under-fire reaction, når den eksplicit har overtaget den fysiske udførelse.
6. **AI-officerens mission/doctrine/autonomi.**
7. **Automatisk formationspolitik** — Line/Column under march og kontakt.
8. **Fire policy og kampadfærd.**
9. **Idle/hold-adfærd.**

Et lavere system må ikke overskrive et højere systems aktive hensigt.

### Hård f29b authority-regel — én fysisk movement owner

En formation må kun have **én fysisk movement owner ad gangen**. Hjælpesystemer må ikke samtidig skrive `OrderMove`, `OrderHold`, destination eller formation, hvis et højere system aktivt ejer udførelsen.

I den aktuelle prototype bruges `OfficerAIController.enabled = false` bl.a. som authority-lock, mens en Major fysisk placerer et company. Et lavere AI-/formation-system skal derfor respektere både `AIEnabled` og componentens `enabled` state.

---

# DEL A — FÆLLES ENHEDSREGLER

## 2. Enhedsskala og styrke

### Implementeret nu

- Aktiv dansk og preussisk test-enhed normaliseres til **190 mand**.
- Ved 1:1-visualisering repræsenterer **én synlig soldat én mand**.
- Ved 1:1 casualty-visualisering repræsenterer **én faldet model ét tab**.
- F9 kan fortsat reducere levende/faldne visualisering til `1:2`, `1:5` eller `1:10` af performancehensyn uden at ændre den simulerede styrke.

### Regel

Grafisk LOD eller display-ratio må aldrig ændre den underliggende styrke, casualties eller combat resolution.

---

## 3. Dansk 1851 Line-formation

### Implementeret nu

- Dansk 1851-standard for linjeinfanteri er **3 geledder**.
- Et fuldt 190-mands kompagni fordeles omtrent **64 + 63 + 63**.
- Horisontal spacing er ca. **0,75 m pr. mand** i prototypen.
- Full-strength frontage er ca. **48 m**.

### Godkendt designregel

- **2 geledder** skal være en senere doktrin-/reglements-researchmulighed.
- Formationer skal på sigt være datadrevne pr. nation, våbenart og årstal.

---

## 4. Line vs. Column

### Line

Line er den primære kampformation.

En enhed skal være i Line når:

- en fjendtlig enhed nærmer sig enhedens **Long/MaximumRange + deployment-buffer**;
- enheden skal skyde;
- enheden netop har afsluttet march og skal deployere til kamp;
- enheden udfører kontrolleret fighting withdrawal;
- en ordre kræver forsvar af en position.

### Column

Column er primært en march-/passageformation.

En enhed må automatisk gå i Column når:

- den har en længere bevægelse foran sig;
- **ingen fjendtlig enhed er inden for Long/MaximumRange + deployment-buffer**;
- eller en højere-prioritets terrænregel kræver Column, fx brokrydsning.

### Hård v00.00.09f10+-regel

> **Hvis nærmeste gyldige fjende er inden for Long/MaximumRange + deployment-buffer, må enheden ikke skifte til Column alene fordi den får en ANGREB-ordre eller fordi fire policy ændres fra LONG til MEDIUM/CLOSE.**

Den skal i stedet blive i/deployere til Line og manøvrere i kampformation.

### Eneste nuværende undtagelse

En aktiv **bridge route** må midlertidigt tvinge enheden i Column, selv hvis fjenden er inden for Long, fordi selve passagen kræver en smal formation.

---

## 5. Formation-change skal være fysisk synlig

- Line → Column og Column → Line må ikke teleportere soldaterne.
- Soldater skal fysisk flytte til deres nye slots.
- Range cone skal være skjult mens enheden er i Column.
- Range cone skal forblive skjult mens enheden fysisk reformerer til Line.
- Enheden må ikke skyde før Line-formationen er klar.

---

## 6. Facing og drejning

### Implementeret nu

- Stationære facing-ordrer drejer formationen gradvist.
- Aktuel QA-værdi er **16,8°/sek**.
- Det er 30 % langsommere end den tidligere 24°/sek test.

### Regel

Facing skal være fysisk læsbar; formationen må ikke teleportere 15° ved Z/X-ordre.

March-steering og stationær kampdrejning er separate systemer.

---

## 7. Selection, destination og footprint

- Selection-rammen skal følge enhedens **faktiske formations-footprint**.
- Destination-box skal have samme størrelse og center-offset som den ønskede slutformation.
- Line- og Column-footprint skal beregnes fra formationsslots, ikke gamle hardcodede rektangler.
- Den gule selection-orb følger formationens faktiske center.
- Column-markering må derfor være lang og smal og ligge omkring selve kolonnen, også når formationen ligger asymmetrisk bag pivoten.
- Et formation endpoint ved floden er kun lovligt, hvis **hele company-footprintet** ligger på lovligt terræn; et centerpunkt på bredden må ikke gøre det lovligt at placere dele af linjen i åbent vand.

---

# DEL B — BEVÆGELSE

## 8. Normal march

### Implementeret QA-værdier

Danmark:

- normal march: ca. **3,20 m/s**

Preussen:

- normal march: ca. **3,35 m/s**

### Regel

- Normal bevægelse bruger gå-animation.
- Enheden må ikke glide hen over terrænet uden synlig gait.
- Lange marcher uden fjende inden for Long kan udløse Column.

---

## 9. Forced March

### Implementeret nu

- Forced March kan toggles med **G / TVANG G**.
- Danmark: ca. **4,25 m/s**.
- Preussen: ca. **4,45 m/s**.
- Forced March giver ekstra cohesion/fatigue-belastning.

### Regel

Forced March er hurtigere end normal march men langsommere og mere kontrolleret end panik/rout.

Det må aldrig være en gratis speed boost uden taktisk omkostning.

---

## 10. Rout/panik

### Implementeret QA-værdier

- Danmark: ca. **5,20 m/s**.
- Preussen: ca. **5,40 m/s**.
- Routed enhed bruger løbe-animation.
- Forced March annulleres ved rout.

### Regel

Rout har højere prioritet end normale spiller- og officerordrer.

---

# DEL C — TERRÆN OG BROER

## 11. Aktuel obstacle-baseline

### Implementeret nu

- Træer: pass-through.
- Hegn: pass-through.
- Bygninger: pass-through i den ældre isolation-baseline; aktuelle formation-slot guards kan dog behandle Farmhouse/Barn som hard endpoint blockers i HQ-testen.
- Åbent vand/flod: **hard blocker**.
- Fast bro ved den nuværende testflod: eneste lovlige crossing.

Dette er en bevidst isolation-baseline for at undgå tidligere fastkørselsproblemer.

---

## 12. Bridge routing

### Implementeret v00.00.09f9+

Hvis en Move/Attack-rute kræver flodkrydsning:

1. den strategiske slutordre bevares;
2. bro-routeren overtager fysisk steering;
3. enheden tvinges i Column;
4. enheden går til **near staging** på tør grund;
5. derefter **near entry**;
6. derefter over broen mod **far entry**;
7. derefter videre til **far staging**;
8. først efter far staging frigives den normale slutmission igen.

Aktuel prototype bruger omtrent:

- staging: ca. **32 m** fra broens center;
- entry: ca. **13 m** fra broens center.

### Regel

En attack-order må ikke overskrive bro-ruten med en direkte linje mod fjenden.

Afstand til fjenden i fugleflugtslinje må ikke få AI-officeren til at tro, at den kan engagere gennem en flod.

---

# DEL D — KAMP OG ILD

## 13. Aktuel QA-rækkevidde

Prototype-skala: **1 Unity unit = 1 meter**.

Aktuelle testgrænser:

- **Close:** 35 m
- **Medium / Effective:** 70 m
- **Long / Maximum:** 100 m

Disse er prototype/gameplay-værdier og ikke endelige historiske våbendata.

---

## 14. Fire cone

### Implementeret nu

- Total fire arc: **70°**.
- Ca. **±35°** fra formationens facing.
- Close/Medium/Long deler samme venstre/højre sidegrænser.
- Fanen starter over hele formationens frontage.

### Regel

- Cone vises kun når valgt enhed står i klar Line.
- Cone skjules i Column.
- Cone skjules under Line-reformering.
- Firing eligibility skal følge samme sektor som den synlige cone.

---

## 15. Fire policy

Fire policy er en **maksimal tilladelse til at åbne ild**, ikke en ordre om at holde præcis den afstand.

- HOLD = ingen skydning.
- CLOSE = må åbne ild inden for Close.
- MEDIUM = må åbne ild inden for Medium.
- LONG = må åbne ild inden for Long.

### Vigtig AI-regel

En AI-officer med MEDIUM valgt må gerne vælge at gå tættere på end Medium-grænsen, hvis doctrine, aggressiveness, tactical skill og situation gør det rimeligt.

Men:

> At ændre policy fra LONG til MEDIUM må **ikke i sig selv** få enheden til at gå i Column, hvis fjenden allerede er inden for Long.

---

## 16. Distance påvirker stadig hit chance

Selv når morale midlertidigt er slået fra i QA:

- Close skal i gennemsnit ramme bedre end Medium.
- Medium skal i gennemsnit ramme bedre end Long.
- Accuracy skal ændre sig kontinuerligt med faktisk afstand, ikke kun springe ved band-grænserne.

### Midlertidig QA-regel

- Morale er aktuelt låst til **100** for casualty/hit-test.
- Cohesion er stadig aktiv.
- Denne morale-lock er **ikke** en permanent spildesignregel.

---

## 17. Volley-animation og sortkrudtsrøg

En synlig salvecyklus skal læses som:

`AFVENT -> SIGT -> SKYD -> RELOAD -> AFVENT`

### Visuel regel

- Hele den relevante skydelinje skal bidrage til salven.
- Røg skal læses langs hele frontage, ikke som få tilfældige puff.
- Soldater skal have synlig recoil/fire-pose.
- Reload skal have separat visuel state.
- Gammel røg skal blive i world space og ikke følge regimentet som en fast sky.

---

# DEL E — CASUALTIES

## 18. Faldne

- Ved 1:1 skal ét casualty kunne give én faldet soldat.
- Faldne placeres ved en faktisk formationsslot ved casualty-tidspunktet.
- Tilladt lokal jitter er ca. **±0,30 m**.
- Lig må ikke spawn'e mange meter fra formationen.
- Faldne bliver liggende i world space, når enheden marcherer videre.

---

# DEL F — SPILLERORDRER

## 19. FORSVAR HER

Godkendt/implementeret grundregel:

- gå til valgt punkt;
- deployer i Line;
- hold positionen;
- officer går i defensiv lokal adfærd efter ankomst.

Senere defensive construction/cover-regler kan bygge ovenpå denne ordre.

---

## 20. ANGREB

### Regel

ANGREB betyder:

- mål-enheden er strategisk hensigt;
- lovlig rute og terrænregler har højere prioritet end direkte linje;
- hvis fjenden er på samme side af terrænbarrierer, lukkes der til officerens preferred engagement range;
- hvis fjenden er inden for Long, forbliver/deployer enheden i Line;
- hvis bro skal krydses, bruges midlertidig Column uanset Long-reglen;
- efter broen genoptages den oprindelige attack mission;
- et eksplicit `AttackTarget` forbliver target, indtil det bliver invalidt eller en ny højere ordre ændrer missionen;
- et company må ikke skifte mellem to levende fjender alene fordi "nearest enemy" ændrer sig marginalt fra én think-cycle til den næste.

---

## 21. EROBR HER

- gå til valgt terrænpunkt;
- brug marchformation efter de normale regler;
- deployer til Line nær slutpunkt;
- hold det valgte område efter ankomst.

---

## 22. HOLD

- stop aktiv bevægelse;
- behold/returner kampformation efter gældende formationsregler;
- firing følger stadig valgt fire policy, hvis målet er lovligt.

---

# DEL G — FIGHTING WITHDRAWAL / TRÆK BAGLÆNS

## 23. Formål

Fighting withdrawal er **ikke rout**.

Det er en kontrolleret ordre, hvor enheden:

- bliver i Line;
- holder front mod nærmeste relevante fjende;
- skyder fra stationær position;
- trækker sig et kort stykke baglæns;
- stopper;
- skyder igen;
- gentager indtil valgt afstand er nået.

---

## 24. Tre withdrawal-knapper

### TRÆK → MED

Mål: ca. **Medium/EffectiveRange**.

Med nuværende QA-range: ca. **70 m**.

### TRÆK → LONG

Mål: omtrent to tredjedele inde i intervallet Medium → Maximum.

Med 70/100 QA-range: ca. **90 m**.

### TRÆK → UD

Mål: uden for fjendens nuværende Long/MaximumRange med sikkerhedsmargin.

Med 100 m Long og nuværende margin: ca. **115 m**.

---

## 25. Fighting-withdrawal cyklus

Aktuel prototypecyklus:

1. enheden går/forbliver i Line;
2. AI UNIT slås midlertidigt fra, så en offensiv officer ikke ophæver spillerordren;
3. enheden får midlertidigt LONG fire permission til covering fire;
4. den står stille gennem en reload/fire-pause;
5. den trækker ca. **10 m baglæns**;
6. under selve backstep er fire policy midlertidigt HOLD;
7. backstep-hastighed er ca. **1,70 m/s**;
8. den stopper, vender fortsat fronten mod fjenden og må skyde igen;
9. cyklussen gentages til mål-afstanden er nået;
10. tidligere fire policy gendannes ved afslutning;
11. Officer AI forbliver OFF efter afslutning, indtil spilleren bevidst aktiverer den igen.

### Terrænregel

Fighting withdrawal må ikke automatisk bakke regimentet ud i åbent vand.

Hvis næste backstep krydser den nuværende flod, stoppes ordren. En senere avanceret regel kan give reverse bridge withdrawal som særskilt officer-/pathfinding-adfærd.

---

# DEL H — AI-OFFICERREGLER

## 26. AI UNIT ON/OFF

- AI OFF = direkte spillerkontrol.
- AI ON = OfficerAIController må fortolke missionen efter doctrine/stats.
- En eksplicit spillerordre har højere prioritet end normal officer-autonomi.
- `AIEnabled=true` betyder ikke nødvendigvis, at controlleren har fysisk movement authority i dette frame; `controller.enabled=false` kan være et bevidst higher-command authority-lock.

---

## 27. Doctrine

Aktuelle doctrines:

- **DEFENSIVE**
- **BALANCED**
- **OFFENSIVE**

Doctrine ændrer officerens villighed til at avancere, holde område og vælge engagement range.

---

## 28. Officerstats der påvirker adfærd

Aktuel prototype bruger bl.a.:

- Leadership
- Inspiration
- Tactical Skill
- Initiative
- Staff/Command Skill
- Discipline/Obedience
- Aggressiveness/Caution
- Composure/Nerve
- Officer Experience

### Regel

Officerens stats må påvirke **beslutning og timing**, ikke give skjulte direkte våbenbuffs, medmindre en separat regel udtrykkeligt definerer dette.

---

## 29. Commander Aggressiveness

- Spillerens 0–100 aggression-intent er et commander-bias.
- Officerens egen aggressiveness skal stadig have betydelig vægt.
- OFFENSIVE doctrine kan skubbe effektiv aggressiveness op.
- DEFENSIVE doctrine kan skubbe den ned.

---

## 30. Preferred engagement range

AI-officeren vælger preferred engagement range ud fra bl.a.:

- doctrine;
- effective aggressiveness;
- Tactical Skill;
- decision noise/stress;
- valgt fire policy som maksimal firing permission.

### Regel

Fire policy er ikke identisk med preferred engagement range.

En OFFENSIVE officer med MEDIUM må fx forsøge at lukke tættere end 70 m.

---

## 31. Formation under officer-AI

AI-officeren skal følge de samme globale formationsregler som spilleren:

- fjende inden for Long => Line;
- længere march uden fjende i Long => Column tilladt;
- bro-route => Column påkrævet;
- firing => Line påkrævet;
- fighting withdrawal => Line påkrævet.

Officer-AI må ikke have en skjult undtagelse, som får den til at gå i Column i kampzonen.

Automatisk approach-formation må kun skrive formation, når den lokale Officer AI faktisk ejer formationen; den må ikke ændre et Major-owned eller under-fire-owned company.

---

## 32. Explicit AttackTarget

Hvis spilleren eller en højere officer udpeger et specifikt fjendtligt mål:

- missionen skal bevares som hensigt;
- officer-doctrine må ikke erstatte målet uden gyldig årsag;
- terræn/pathfinding må indsætte midlertidige steering-punkter;
- bridge routing har prioritet over fugleflugtsafstand;
- efter crossing fortsætter den oprindelige attack mission;
- target commitment ophører først ved rout/destruction/invalid target eller en ny autoritativ ordre.

### AttackNearest target commitment

`AttackNearest` må vurdere kandidater under en lang approach. Når et target er valgt ved reel contact/engagement envelope, skal valget blive sticky nok til at formationen kan gennemføre attack/firing cycle. Et marginalt nærmere andet target er ikke i sig selv gyldig årsag til at pivotere hele formationen.

---

## 33. DefendArea

En defensiv officer:

- holder sig omkring sin defensive anchor;
- må reagere på fjender i lokal decision space;
- skal undgå unødigt langt offensivt pursuit;
- kan bruge Line og lokal ild frem for at marchere i Column, når fjenden er i Long-zonen.

---

## 34. MoveToPoint

En MoveToPoint-mission:

- prioriterer det tildelte punkt;
- kan bruge Column på længere afstand;
- men skal stadig deployere til Line hvis en fjende kommer inden for Long;
- undtagelse: en aktiv bridge-route kan fastholde Column gennem crossing.

---

## 35. AI difficulty

Godkendt regel:

Easy/Normal/Hard må påvirke:

- reaction timing;
- decision noise;
- hvor hurtigt computer-AI reassesserer.

Difficulty må **ikke** skjult ændre:

- weapon accuracy;
- reload;
- range;
- movement speed;
- morale;
- cohesion;
- casualties;
- officer stats;
- fog-of-war information.

---

# DEL H2 — CAVALRY F30H FORMATION OG BRIDGE

## 35A. Mounted cavalry formation

F30H fastlåser følgende PROJECT 1864 runtime-regler for Gardehusar og Dragon i den aktuelle 1:1 prototype:

- normal **Line = 4 geledder**;
- **Charge = 4 geledder**;
- normal **Column = 4 abreast**;
- bridge/defile crossing = **2 abreast**.

Den tidligere 2-geleds cavalry Line er superseded i runtime. Dette er en projekt-/gameplayregel for den aktuelle prototype og skal behandles som datadrevet doctrine på sigt.

## 35B. Cavalry formation-change

Line ↔ Column må ikke teleportere rytterne.

- Hver mounted figure flytter fysisk til sin nye formations-slot.
- HUD skal kunne vise reform-progress.
- En cavalry charge må ikke få fuld charge-speed mens formationen stadig er under væsentlig reformering.
- AI og spiller bruger samme formation-change-regel.

## 35C. Cavalry bridge transaction

En aktiv cavalry bridge crossing har højere movement-prioritet end normal Officer AI replanning.

Crossing-faser:

1. NearBank
2. FarBank
3. ExitBank
4. Direct

Regler:

- crossing tvinger 2-abreast bridge column;
- en ny destination under crossing må opdatere final goal men må ikke nulstille bridge phase;
- efter far bank skal hovedet bevæge sig langt nok frem til, at den fulde 1:1 column er fri af broen;
- først derefter gendannes den ønskede normalformation;
- bridge-only terrain authority gælder både player og Officer AI.

## 35D. Cavalry selection/HUD/semantic presentation

- Cavalry skal kunne vælges i world, OOB og via RTS marquee når ingen infantry-enhed ligger i boksen.
- Brigade/Division HQ skal tilsvarende kunne vælges i world/OOB og via marquee.
- Cavalry HUD skal bruge samme visuelle baseline som company HUD.
- Semantic zoom skal vise Gardehusar/Dragon som I/CAV med navn og Brigade/Division som X/XX HQ med navn.
- Strategic mesh suppression må ikke fjerne simulation/collider/selection authority.

---

## 35E. Cavalry manual authority og ordrevisualisering

- Cavalry Officer AI starter OFF/MANUEL.
- AI må kun aktiveres eksplicit af spilleren eller en senere autoritativ higher-command delegation.
- Direkte world-order samt manuelle cavalry HUD-kommandoer tager manual authority.
- AI ON/OFF-knappen skal være stabil og må ikke selv blive fortolket som en separat manual command.
- Kun ét cavalry bottom-HUD må tegnes.
- Selected cavalry skal vise formation-sized selection footprint.
- Aktiv destination skal vise order path og destination ghost footprint.
- Dragon SID AF/STIG OP og mounted movement skal være visuelt animeret; instant model-pop/ren glidning er ikke godkendt slutadfærd.

---

## 35F. Dragon dismounted combat

Når Dragon får SID AF:

- hestene bliver ved dismount-positionen;
- ca. 25% af styrken fungerer i den aktuelle prototype som horse holders;
- ca. 75% danner combat group;
- combat group går ca. 18m frem foran hestene;
- combat group opstiller i to geledder;
- fire cone må først være combat-ready når dismount-transition og fysisk reform er afsluttet;
- valgt dismounted Dragon viser Close/Medium/Long cone;
- nuværende TEST-ranges er 35/70/100m og +/-35deg;
- nuværende TEST reload/ammo er 7s og 20 rounds/man;
- automatic carbine fire kræver HOLD og gyldigt mål i cone;
- STIG OP skal kalde combat group tilbage mod hestene visuelt før foot group skjules.

25% horse-holder ratio, 18m spacing, ranges, reload og ammo er prototype-/QA-værdier og skal senere kunne drives af doctrine/weapon/scenario data.

## 35G. Cavalry footprint anchor

- Four-rank Line bruger center-anchor.
- Four-abreast Column bruger front-anchor og strækker sig bagud.
- Two-abreast bridge/defile column bruger front-anchor og strækker sig bagud.
- rider slots, selection footprint, destination ghost og physical collider skal bruge samme anchor.
- Et formationsskift må ikke efterlade halv formation uden for selection/destination footprint.

---

## 35H. Cavalry automatic march policy og facing

- Mounted cavalry MOVE må bruge automatic march formation uanset Officer AI ON/OFF.
- Lang march med mindst ca. 140m remaining og ingen fjende inside Long -> 4-abreast Column.
- Ved ca. 90m remaining -> 4-rank Line.
- Enemy inside Long -> Line.
- Bridge route -> 2-abreast og har højere prioritet.
- 140/90 hysterese skal forhindre formation-flip omkring én tærskel.
- En eksplicit manual formation selection må ikke straks overskrives af auto-policy.
- Cavalry RMB hold+drag bruger samme destination/facing-semantik som infantry.
- Explicit destination-facing skal også styre destination ghost og endelig facing.

## 35I. Higher-HQ AI authority

- Brigade og Division har selvstændig AI ON/OFF state og DEF/BAL/OFF doctrine.
- AI starter OFF.
- Higher-HQ AI-toggle må ikke kun være kosmetisk.
- Attached cavalry Officer AI kræver AI ON hos dens aktuelle command parent.
- Regiment-attachment kræver Regimental AI ON.
- Major A/B attachment kræver tilsvarende Battalion AI ON.
- Higher-HQ doctrine føres ind i subordinate mission decomposition, når higher HQ udsteder mission.

## 35J. Dismounted Dragon selection/remount presentation

- Dismounted Dragon må ikke markeres med én stor tom rectangle mellem horse group og firing line.
- Selection vises som separate combat-group og horse-holder footprints med relation/link mellem dem.
- SID AF prototype transition er ca. 2.25s.
- STIG OP/remount prototype transition er ca. 4.5s.

---

## 35K. Cavalry visual/animation LOD

- Cavalry tactical scale forbliver 1:1: én simuleret cavalryman = én synlig mounted figure ved tactical rendering.
- Visual LOD må kun reducere detail-rendering, aldrig CurrentStrength, casualties, collider authority eller tactical formation slots.
- Mounted MOVE skal have synlig horse gait; CHARGE skal være tydeligt hurtigere/mere aggressiv end normal MOVE.
- Horse leg/hoof, head/tail og rider motion må være procedural indtil rigged assets erstatter prototypen.
- HOLD skal returnere til en stabil neutral pose.
- Gardehusar og Dragon skal have tydeligt forskellig uniform/equipment silhouette.

---

## 35W. Early deploy, company separation, CAV bridge approach og startup enemy cones

- Infantry må ikke vente med Column → Line til det allerede er inde i fjendens maksimale ildafstand.
- Aktuel TEST deployment-regel er **nærmeste fjendes MaximumRange + 35 m buffer**.
- Ved 100 m enemy MaximumRange starter Line-reform derfor omkring 135 m.
- `PrototypeMarchColumn09F6` ejer normal marchformation; parent mission ejer destinationen og må ikke periodisk gennemtvinge Line under lang march.
- Full-strength 190-man company har ca. 48 m frontage; standard company slot-spacing er derfor **72 m center-center**.
- Formation-slot minimum reservation spacing er **68 m**.
- Hvis terrain/slot-safety kræver forskydning, bør det primært ske lateralt, så companies ikke står gennem hinanden.
- CAV med destination på modsatte bred må gerne have en bridge-route planlagt uden at gå i 2-abreast.
- Speciel CAV **2-abreast bridge/defile column** aktiveres først ved actual bridge approach, aktuelt ca. **36 m** fra near-bank approach.
- Efter far-bank exit gendannes formationen fra før bridge narrow mode.
- I TEST skal enemy fire-cone renderers oprettes straks, når enemy Regiment findes; cone visibility må ikke vente på selection eller senere visual-state.

---

## 35V. TEST enemy cones, river bank routing og OOB AI state

- I TEST/QA skal alle levende preussiske infantry fire-cones være synlige uden krav om dansk selection eller afstand.
- Enemy cone visibility er kun visuel QA og må ikke give LOS, target knowledge eller firing authority.
- Aktiv enemy fire-policy range er stærk; øvrige ranges er svage references.
- En river route må kun bruge bridge crossing ved et reelt bankskifte.
- Hvis start og slutmål er på samme bred, men den rette linje skærer en bugtet å, skal enheden følge en tør bank-rute og ikke krydse broen frem og tilbage.
- OOB-kolonnen `AI` bruger konsekvent `ON / OFF` på alle niveauer.
- Brigade/Division follow og ren CAV-reform er background housekeeping og holder ikke taktisk ordrestate blå.

---

## 35U. Final-slot arrival authority

- Company parent-mission completion har én fysisk final-slot tolerance: **0,50 m** fra `mission.Goal`.
- F27, F29E og F29L må ikke bruge forskellige completion-tolerancer for samme company goal.
- Destination-footprint og movement completion skal referere til samme `mission.Goal`.
- Parent HUD må ikke markere ordren færdig/rød alene fordi `mission.Arrived=true`.
- Parent order er fortsat aktiv, hvis company er >0,50 m fra goal, stadig har aktiv movement destination eller `Arrived=false`.
- Midlertidig under-fire/tactical pause må ikke afslutte parent movement mission; når reaktionen slipper, fortsætter company mod samme final-slot.
- Defensive arrival-hysteresis må kun forhindre ping-pong efter reel arrival, ikke stoppe company tidligt.

---

## 35T. Defend-command authority og HQ-goal ownership

- Under en committed `FORSVAR HER` er missionens committed facing og defensive disposition autoritativ.
- `PrototypeDefensiveStability09F29Y` ejer Major-HQ defensive rear-position under `DefendHere`.
- `PrototypeHqDepthGuard09F29W` må ikke skrive Major-HQ goals, mens bataljonens current order er `DefendHere`.
- `PrototypeHqDepthGuard09F29W` må heller ikke skrive Regiment-HQ goal, mens Regimentets current mission er `DefendHere`.
- Hvis Major allerede står på sin committed defensive position, skal enhver konkurrerende `HqGoal` cleares, også hvis den er skrevet af et andet helper-system og peger et andet sted.
- To helper-systemer må aldrig samtidig eje samme HQ-destination.
- Defensive housekeeping må ikke holde en afsluttet ordre kunstigt blå.

---

## 35S. Crop visual geometry og infantry fire-cone authority

- Crop-field gameplay (concealment/movement) ændres ikke af den visuelle crop-geometri.
- Crop visuals må ikke bestå af lange massive bjælker/strips ved tactical zoom; de skal læses som korte opretstående planter/tufts og følge terrænhøjden lokalt.
- Infantry fire-cone appearance har én endelig runtime-authority: `PrototypeFireVisuals09F8`.
- Aktiv `CLOSE / MED / LONG` skal være visuelt stærkere/tykkere end de øvrige fysiske range-bands.
- Inaktive ranges forbliver svage reference-cones.
- Ved `HOLD` er alle ranges reference-only.
- I TEST/QA må preussiske infantry-cones være synlige uden selection og skal vise samme active/inactive hierarchy.
- Synlig enemy cone er ikke LOS og giver ikke target knowledge eller firing authority.

---

## 35R. Execution-state, defensiv CAV-reserve, cone-QA og selection persistence

- **Blå ordreknap = fysisk execution i gang.** Seneste mission/intent alene må ikke holde en knap blå.
- Når alle underlagte movement executors, relevante HQ follow-moves og CAV move/reform/charge states er afsluttet, går ordren tilbage til rød.
- `FORSVAR HER` og `STOP/HOLD` kan fortsat være den gældende standing intent efter knappen er blevet rød.
- Defensive CAV skal placeres bag den bataljon, den støtter, og ikke foran infantry-linjen.
- Aktuel QA reservegeometri er ca. 150 m bag battalion anchor + 45 m udad lateralt.
- Dragon og infantry bruger samme range-visual language: aktiv `CLOSE/MED/LONG` stærk; øvrige ranges svage referencer; `HOLD` gør alle ranges reference-only.
- Dragon cone-sider skal være geometrisk spejlede; terræn-sampling må ikke give en tydelig ensidig kink.
- I TEST/QA må preussiske infantry-cones vises uden normal player selection for at kontrollere range/facing/fire-policy.
- Enemy cone visibility er midlertidig QA og skal senere styres af LOS/FOG.
- En ordre til Division, Brigade, Regiment eller Major må ikke rydde det valgte HQ. Selection bevares gennem objective/facing placement og efter commit.
- Enemy infantry må kun reagere med ild mod CAV når **LOS + fire-policy range + fire-cone** alle er opfyldt.
- TEST-synlige enemy cones er QA og må ikke i sig selv skabe target knowledge.
- Square/mounted-threat reaction kræver også LOS til CAV.
- CAV skal have mouse-over unit info på samme informationslag som infantry.
- Afsiddet Dragon består funktionelt af mobile combat dragons og stationære horse holders/horse park; normal afsiddet movement må ikke flytte horse-holder elementet.
- `STIG OP` væk fra hestene betyder return-to-horses og derefter auto-remount, ikke direkte remount på afstand.
- `ANGRIB HER` er finite positioning: ved arrival holdes attack-slot; OfficerAI må ikke straks starte nearest-enemy chase.
- Efter arrival må parent mission ikke genskrive facing hver frame; formation-motion afslutter drejningen.
- Et settled defensive HQ-goal skal cleares, så execution-state kan afslutte.

---
## 35Q. Afsiddet Dragon fire-control

- Dismounted Dragon bruger eksplicit fire policy: `HOLD / CLOSE / MED / LONG`.
- `HOLD` giver 0 m trigger-range og forhindrer automatisk karabinild.
- `CLOSE` = 35 m.
- `MED` = 70 m og er default fire policy.
- `LONG` = 100 m.
- Fire policy ændrer engagement-threshold, men ikke carbinens eksisterende ±35° fire arc.
- Reload, ammunition, smoke og damage-resolution fortsætter gennem den eksisterende Dragon-fire pipeline.
- Valgt/afsiddet Dragon må vise alle range-cones som reference, men aktiv policy skal visuelt fremhæves.
- Mounted Dragon må ikke bruge de afsiddede fire-control knapper; de bliver tilgængelige efter `SID AF`.
- Mounted carbine-fire er fortsat ikke implementeret.

---
## 35P. Active-order state, BAL attack commitment og fremtidig CAV reconnaissance

- Officer-order buttons use **blue = pending/active mission** and **red = no current mission**.
- A point-order becomes blue as soon as target placement starts and stays blue after commit while the mission still has active executors.
- `ANGRIB HER` remains active while subordinate companies are moving or still in local combat/under-fire contact.
- `FORSVAR HER` and `STOP/HOLD` may remain the standing intent, but their **blue HUD execution state** ends when physical execution has settled.
- BAL doctrine must not reserve an entire battalion behind an `ANGRIB HER` front by default.
- BAL `ANGRIB HER` commits both battalions forward side-by-side; Majors may still keep local company reserves.
- DEF doctrine may retain a whole battalion as regimental reserve.
- OFF doctrine may use flank disposition according to existing attack-planning rules.
- Future `SPEJD HER` is a mounted CAV reconnaissance task available when true FOG/LOS owns enemy visibility.
- `SPEJD HER` uses SEEK/RECON -> CONTACT -> SCREEN and does not itself grant charge authority.
- Recon CAV must preserve safe stand-off distance, report sightings through current command parent and create time-stamped/quality-limited last-known contacts.
- `SPEJD HER` must remain hidden/disabled before the real FOG/LOS visibility model is active.

---
## 35O. Committed facing, HQ follow og command reach

- En drag-facing fra en point-order er mission-data og skal bruges **før** formation slots genereres.
- En order-pil må aldrig pege i en retning, som den underliggende bataljon/company-plan ikke bruger.
- `FORSVAR HER` bruger committed facing som formationens frontretning og lateral axis.
- Regiment-HQ skal placeres bag sine Majorer i forhold til samme committed facing.
- Brigade-HQ skal følge Regiment-HQ relativt til formationens facing, ikke via faste world-X/world-Z offsets.
- Division-HQ skal følge Brigade-HQ relativt til formationens facing og må ikke akkumulere kunstigt efterslæb.
- HQ command reach visualiseres med inner/outer cirkler på det valgte HQ.
- Aktuelle QA-bands: Major 320/450 m, Regiment 800/1100 m, Brigade 1350/1850 m, Division 2100/2850 m.
- Command reach skal senere påvirke command delay, coordination og information/report quality; det må ikke give direkte bonus/malus til våbenskade.

---
## 35N. Higher-HQ AI arming og shared objective placement

- **AI ON er ikke en ordre.** Division/Brigade AI ON må kun gøre command chain klar til delegated execution.
- Ingen Regiment-, Major/Battalion-, Company- eller CAV-bevægelse må starte alene som følge af higher AI ON.
- Regiment og Battalion skal kunne stå i `AwaitHigherMission` indtil en eksplicit higher mission commits.
- Company Officer AI skal HOLD under denne ventetilstand.
- CAV skal vise en waiting/armed state og må ikke SEEK fjenden før mission commitment.
- Re-arming higher AI må ikke genstarte en stale inherited CAV mission.
- Division/Brigade position-orders skal bruge samme objective-placement system som Regiment/Major.
- Ved pending position-order skal en synlig circle følge ground cursor.
- Click placerer objective; drag fra objective definerer facing.
- Higher-HQ HUD skal tegnes af samme authoritative runtime renderer som Regiment HUD, ikke af en separat visuel approximation.
- HUD'ens øverste kant skal være sort.

---

## 35M. Cavalry screen, engagement opportunity og infantry anti-cavalry response

- Autonomous cavalry må ikke ride direkte gennem en intakt fjendtlig infantry-formation for at nå et flank/rear point.
- Hostile infantry danner avoidance-bubbles; CAV bruger intermediate detour-waypoints og fortsætter derefter mod sit oprindelige tactical point.
- CAV skal normalt holde screen/stand-off afstand, indtil et reelt charge-vindue opstår.
- Flank/rear geometri alene er ikke tilstrækkelig til charge.
- Primært charge-vindue: mindst ét venligt infantry-company har målet i aktiv local fire-contact.
- Sekundært charge-vindue: målet er tydeligt svækket i morale eller cohesion.
- Ready Square er fortsat en no-charge condition for autonomous cavalry.
- Enemy infantry må vælge CAV som ildmål inden for gældende fire-policy range og fire arc.
- Anti-cavalry musketild bruger samme reload/range/accuracy/smoke authority som normal infantry fire.
- Treffer mod CAV reducerer strength, morale og cohesion; en hård salve kan bryde en ikke-committed approach til FALTER.
- Mounted-threat/Square-vurdering er team-neutral.
- Screening/maneuvering cavalry giver lavere Square-threat end en committed charge.
- Auto-Square må ikke udløbe, mens den relevante mounted threat fortsat er til stede.

---

## 35L. Higher-command delegation og cavalry task attachment

- Division AI ON propagates delegated authority til Brigade, Regiment, Major A/B, company Officer AI og attached cavalry.
- Brigade AI ON propagates fra Brigade og ned uden at ændre Division.
- Direkte spillerordre har højere authority end inherited higher mission; RMB skal bryde inherited cavalry-mission også når cavalry Officer AI allerede står OFF.
- Kun cavalry-enheder der faktisk er midlertidigt task-attached må blive auto-released/returneret af attack-completion-logikken.
- Higher-HQ selection skal vise subordinate routes, destination footprints og mission objective.
- Higher order-knappen er blå mens missionen er pending eller mindst én subordinate stadig udfører den.
- En arrived company mission tæller ikke som aktiv execution.

### Cavalry under ANGREB

Når et higher HQ med cavalry under sig giver ANGRIB HER:

- OrganicParent bevares;
- CurrentCommandParent kan midlertidigt ændres til Major A eller Major B;
- cavalry bliver dermed et taktisk attached asset for den angribende Battalion;
- med to cavalry units og to Battalions skal assignment minimere unødigt crossing/travel;
- cavalry kan derefter bruge sin flank/rear Officer AI under den midlertidige parent authority.

Når attack execution er færdig:

- en allerede committed cavalry charge må afsluttes;
- cavalry frigives fra Major/Battalion;
- CurrentCommandParent returnerer til den tidligere higher parent;
- attachment type bliver RESERVE;
- cavalry samles i en reserveposition tæt ved det parent HQ.

En ny ikke-angrebsordre fra higher HQ frigiver midlertidig attack attachment med det samme.

### Cavalry under FORSVAR

FORSVAR HER skal kunne placere attached cavalry i reserve/support-positioner bag hovedforsvaret. Cavalry må ikke blive stående passivt ved sin gamle position, hvis higher HQ har givet en ny forsvarsmission.

---

# DEL I — TESTUND-TAGELSER OG FREMTIDIGE REGLER

## 36. Midlertidige QA-regler

Følgende er testværdi og skal ikke automatisk betragtes som endelig balance:

- Close/Medium/Long = 35/70/100 m;
- morale låst til 100;
- forhøjet QA base accuracy;
- current multi-company tactical QA scenario;
- scenery pass-through hvor ikke en nyere endpoint/terrain guard udtrykkeligt siger andet.

---

## 37. Godkendte senere designretninger

Følgende er godkendte designretninger, men er ikke nødvendigvis fuldt runtime-implementeret endnu:

- 2-geledders linje som doctrine/regulation research;
- defensive fieldworks/cover via FORSVAR HER;
- bedre terræn-effekter på cohesion, hastighed, cover og frontage frem for gamle hard-blocker chains;
- flere historiske weapon profiles;
- mere avanceret fire eligibility pr. frontage-segment;
- LOS, friendly obstruction, smoke og target exposure;
- reverse bridge withdrawal som senere avanceret withdrawal/pathfinding-regel.

---

# 38. Acceptance-regler for nye ændringer

Når en ny version ændrer enheds- eller AI-adfærd, skal den testes mod følgende minimum:

1. Ændringen må ikke bryde bridge-only crossing; cavalry skal gennemføre NearBank → FarBank → ExitBank og først reformere efter fuld clearance.
2. En enhed inden for Long må ikke uventet gå i Column.
3. Line/Column transition skal være visuelt fysisk; cavalry bruger F30H 4-rank Line / 4-abreast Column / 2-abreast bridge column.
4. Selection/destination footprint skal matche formationen.
5. Fire cone og faktisk firing eligibility skal være enige.
6. Fire policy må ikke forveksles med tvungen standoff-distance.
7. AI-officerer skal følge samme globale formations-/terrænregler som spilleren.
8. Explicit player/higher-command mission må ikke stille og roligt blive overskrevet af lavere-prioritets AI.
9. Der må kun være én fysisk movement owner for et company ad gangen.
10. Explicit AttackTarget må ikke erstattes af nearest-target logik uden gyldig missionændring.
11. AttackNearest må ikke oscillere mellem næsten lige nære mål efter engagement er etableret.
12. Casualties må ikke spawn'e langt fra enheden.
13. Rout må være tydeligt forskellig fra kontrolleret fighting withdrawal.
14. Ved regimental front+reserve må den nærmeste battalion ikke sendes bagud alene pga. lavere samlet travel-cost permutation.

---

## 39. Versionslog for dette regelsæt

### v00.00.09f30n

- Higher-HQ HUD visual parity with Regimental HQ.
- Cavalry screen/stand-off + engagement-gated charge logic.
- Hostile infantry route avoidance/detour.
- Infantry fire against cavalry and cavalry volley reaction.
- Team-neutral persistent mounted-threat/Square response.


### v00.00.09f30m

- Division/Brigade AI cascade gennem subordinate command tree.
- Higher mission routes/destinationer/objective forbliver synlige fra selected Brigade/Division.
- Higher order active state er baseret på reelle executors.
- FORSVAR HER giver cavalry reserve/support mission.
- ANGRIB HER task-attacher cavalry midlertidigt til Major A/B via CurrentCommandParent mens OrganicParent bevares.
- Cavalry returnerer til tidligere parent som RESERVE efter attack completion og eventuel committed charge.
- Direct player cavalry order bryder inherited mission.


### v00.00.09f30l

- Cavalry 1:1 visual-count fastholdt under LOD.
- Mounted gait skal artikulere horse/rider og CHARGE skal visuelt adskilles fra normal MOVE.
- Close-detail LOD må ikke ændre simulation/collider/formation authority.

### v00.00.09f30k

- Shared cavalry auto-march policy: 140m Column / 90m Line hysterese + enemy-inside-Long Line.
- RMB hold+drag final-facing gjort fælles med infantry control language.
- Dismounted Dragon selection split i combat/horse footprints.
- Remount forlænget til ca. 4.5s.
- Brigade/Division AI ON/OFF + doctrine gjort kanonisk og parent-AI gating indført.

### v00.00.09f30j

- Dragon SID AF opdeles i horse holders og fremrykket combat group.
- Dismounted Dragon carbine cone/fire/reload/ammo/smoke tilføjet som TEST-baseline.
- STIG OP skal have positional recall mod hestene.
- Cavalry selection/ghost/collider anchor skal være identisk med formationens rider-slot anchor.

### v00.00.09f30i

- Cavalry Officer AI default OFF/MANUEL.
- Single cavalry HUD owner fastlagt.
- Formation-sized selection, order line og destination ghost gjort kanonisk.
- Dragon mount/dismount og mounted movement skal have visuel transition/animation.

### v00.00.09f30h

- Cavalry normal Line og Charge fastlåst til 4 geledder.
- Normal Column = 4 abreast; bridge/defile = 2 abreast.
- Physical reform og reduced charge-speed under reform gjort kanonisk.
- Persistent cavalry bridge transaction med ExitBank-clearance.
- Company-style cavalry HUD, semantic I/CAV + X/XX HQ og marquee-selection tilføjet som presentation/selection-regel.

### v00.00.09f29b

- Tilføjet hård regel om **én fysisk movement owner**.
- `controller.enabled=false` dokumenteret som higher-command authority-lock i F27-kæden.
- Explicit `AttackTarget` gjort sticky til rout/destruction/new order.
- `AttackNearest` får contact commitment og må ikke oscillere på marginale afstandsforskelle.
- Approach formation skal respektere Major- og under-fire authority.
- Hele company-footprintet skal være lovligt ved river endpoints.
- Regimental front+reserve/front+flank bruger nærmeste battalion som FRONT.

### v00.00.09f10

- Oprettet som samlet source-of-truth for enheds- og AI-officeradfærd.
- Tilføjet hård regel: **enemy inside Long => no automatic Column**.
- Dokumenteret bridge-route som eneste aktuelle undtagelse.
- Tilføjet fighting-withdrawal ordren med `MED / LONG / UD`.
- Samlet eksisterende movement, formation, fire, casualty, terrain og Officer AI-regler fra prototypeforløbet.
