# PROJECT 1864 — Backlog B-200–B-209: Range bands, HQ, semantic zoom og couriers

**Status: DELVIST IMPLEMENTERET / RESTEN PLANLAGT**  
**Designbaseline: v00.02.08**  
**Aktuel prototype: P0A v00.00.09 TACTICAL COMMAND TEST**

Dette supplement samler tæt forbundne UI-/command-systemer: visuel opdeling af våbenrækkevidde, fysiske HQ-enheder, hierarkiske command-links, semantic zoom og synlige budbringere/ordrer. Range/fire-delen er nu delvist implementeret i v00.00.09; HQ/courier/semantic-zoom-delene følger efter den første runtime-gate.

## B-200 — Close / Medium / Long range bands

**BESLUTTET / IMPLEMENTERET FØRSTE MVP v00.00.09 TEST.** Når spilleren viser en infanterienheds skudfelt, vises tre fremadrettede fan-/sektorgrænser:

- **Close**: 0–50 % af våbnets `EffectiveRange`.
- **Medium**: op til `EffectiveRange`.
- **Long**: op til `MaximumRange`.

Første v00.00.09 implementation bruger en **120° samlet forward fire fan (±60°)**, der følger regimentets facing. Fanen starter ved formationens frontage og åbner udad, så visualet ikke reducerer hele regimentet til ét center-ray, men stadig undgår den tidligere urealistiske 360° range-ring.

UI/runtime i første MVP:

- `T` viser/skjuler range-fan.
- Close/Medium/Long vises som tre nested fan boundaries.
- Long/MaximumRange er den yderste grænse.
- Target skal være inden for den valgte fire policy **og** inde i forward fire arc for at kunne beskydes.
- Senere skal de interne grænser blive mere diskret dashed/stippled og terræn-/LOS-tilpassede.
- Ved multi-select skal detailed range visual senere begrænses til focused unit for clutter control.

### Fire policy

**IMPLEMENTERET v00.00.09 TEST.** Commander kan vælge:

- `HOLD` — ingen automatisk ild.
- `CLOSE` — åbn ild ved Close range.
- `MEDIUM` — åbn ild ved EffectiveRange.
- `LONG` — åbn ild helt ud til MaximumRange.

Fire policy er en ordre/constraint og må ikke automatisk ændre våbnets accuracy. Officer AI må vælge approach og preferred engagement range, men må ikke åbne ild tidligere end den valgte fire policy tillader.

## B-201 — Accuracy er kontinuerlig, bands er UI

**BESLUTTET / IMPLEMENTERET FØRSTE MVP v00.00.09 TEST.** Close/Medium/Long er ikke tre hårde hit-chance-trin. Den underliggende hit probability ændrer sig kontinuerligt med afstand.

Det betyder:

- Close giver typisk den højeste hit probability.
- Medium er våbnets normale effektive kampafstand.
- Long er markant mindre sikker, men stadig mulig op til `MaximumRange`.
- Der opstår ikke et kunstigt accuracy-jump, når et mål krydser en range-band boundary.
- Morale og cohesion påvirker fortsat volley quality.
- Training/experience, stance, target density, cover, smoke, weather og weapon quality kan senere modificere samme continuous curve.

v00.00.09 bruger en første bounded continuous distance-kurve. Senere flyttes den til en data-driven `AccuracyByRange`-curve pr. weapon profile.

## B-202 — HQ som fysisk command-entity

**BESLUTTET / PLANLAGT.** Højere formationsniveauer som brigade, division, korps og army får et egentligt HQ på kortet.

Et HQ repræsenterer mindst:

- commander,
- staff,
- HQ position,
- command state,
- couriers/order capacity,
- communication links,
- eventuel escort,
- senere horses/wagons/telegraph access og HQ fatigue/disruption.

HQ-positionen er simulation. Hvis HQ flyttes langt væk, afskæres, angribes eller mister forbindelser, skal det kunne påvirke order delay, reporting og officer-AI.

## B-203 — Klik på HQ viser kommandokæden

**BESLUTTET / PLANLAGT.** Når et HQ vælges, vises command-links til dets direkte underenheder.

Standardvisning:

- vis kun **immediate children** for at undgå line-spaghetti,
- brigade-HQ → regimenter/bataljoner,
- division-HQ → brigader og direkte attached assets,
- corps-HQ → divisioner og corps assets,
- attached artillery/engineers/supply kan have særskilt attachment-link.

En udvidet command-overlay kan vise hele kæden nedad eller opad efter spillerens valg.

### Visuel regel for relation og aktiv ordre

Command relationship og aktiv ordre deler samme **geometriske link** mellem chef og direkte underenhed, så spilleren ikke skal aflæse to parallelle linjer.

- **Normal command-link** = tynd fast/stabil linje, som viser organisatorisk relation.
- **Aktiv ordre på linket** = samme linje får en tydelig aktiv state, fx stiplet/pulserende/highlightet.
- En **lille rytter-/hestemarkør** bevæger sig langs linket og viser ordre-/rapportprogress.
- Når der ikke transporteres en ordre eller rapport, vises ingen rytter på linket.

Dermed bruges den samme læsbare relation mellem Major og kompagni som visuelt spor for command delay uden at skabe ekstra line-spaghetti.

## B-204 — Semantic zoom og NATO/APP-6-lignende symboler

**BESLUTTET / PLANLAGT.** Enhedsgrafikken skifter med kameraets zoomniveau i stedet for blot at blive mindre og mindre.

Foreslået semantic zoom:

1. **Tæt zoom** — fulde 3D-formationer, soldater, kanoner, heste, HQ-visuals og casualty visuals.
2. **Mellemzoom** — forenklet formationsgrafik/banners og færre individuelle modeller.
3. **Lang zoom** — NATO/APP-6-lignende taktiske symboler med unit type, echelon og retning.
4. **Meget lang strategisk zoom** — formationer kan aggregeres på brigade/division/corps-niveau afhængigt af valgt command layer.

NATO-symbolerne er en moderne **UI-abstraktion**, ikke in-world 1864-grafik.

Teknisk krav:

- overgang skal have hysteresis/fade, så grafik ikke flakker omkring en zoomgrænse,
- simulationen ændres ikke ved LOD/semantic zoom,
- selection og order targets skal bevare samme stable unit ID gennem alle visual modes.

## B-205 — Courier/order progress på eksisterende command-link

**BESLUTTET / PLANLAGT.** En aktiv ordre skal visualiseres direkte på den eksisterende command-link mellem afsender-HQ og modtagende formation/HQ.

På linket vises en bevægelig markør:

- lille courier/horse icon ved tættere zoom,
- simpel kugle/chevron/pil ved større afstand,
- markørens position mellem afsender og modtager svarer til faktisk **order-delivery progress**,
- markørens retning viser om ordren går ud til enheden eller en acknowledgement/report er på vej tilbage.

Eksempel:

`Major  ---- 🐎 ---->  1. Kompagni`

Når spilleren hover/selecter linket eller ordren, vises fx:

`Sent 10:42 | Courier 63% | ETA ~1m40s | En route`

Når rytteren/markøren når modtageren:

- ordren skifter `In transit → Delivered`,
- modtagerens Officer AI må derefter acknowledge/fortolke ordren,
- et eventuelt acknowledgement/report kan vises som en ny rytter, der bevæger sig **modsatte vej** på samme command-link.

### Vigtig teknisk designregel

Rytterikonet er **ikke en fysisk 3D-pathfinding-agent**, der forsøger at ride rundt om træer, huse eller hegn. Det er en command-visualisation, som interpolerer langs command-linket ud fra simulationens beregnede kommunikationsprogress. Derfor kan visualet ikke sidde fast på terræn og kan altid bruges som pålidelig indikator for, hvornår ordren når frem.

Selve command delay beregnes fortsat af simulationen ud fra afstand, kommunikationsform, staff quality og senere terræn/vej/vejr/enemy interference. Rytterens placering er en visualisering af dette resultat, ikke årsagen til delayet.

## B-206 — Courier som letvægts command state, ikke tung soldier-AI

**BESLUTTET / PLANLAGT.** Budbringeren repræsenteres primært som en letvægts command/order state.

Minimum state:

- sender HQ,
- recipient,
- order/report ID,
- transmission start,
- calculated delivery time,
- current progress 0–1,
- status: dispatched / en route / delayed / delivered / lost,
- direction: outbound order eller returning acknowledgement/report.

Første MVP kræver **ingen fysisk terrain pathfinding for courier-modellen**. En lille hest/rytter på command-linket er den visuelle progress-indikator.

Senere kan selve delayberegningen påvirkes af:

- terræn/vej/vejr,
- flyttet HQ,
- afskårne forbindelser,
- cavalry/scouts og enemy interdiction,
- Staff/Command Skill,
- eventuel courier availability og fatigue.

Hvis projektet senere ønsker fysisk courier interception, kan en mere detaljeret world-route-model lægges ovenpå, men den må ikke være nødvendig for den grundlæggende UI-læsbarhed eller ordrelevering.

### Courier interception — område-/risikomodel

**BESLUTTET.** Spilleren skal ikke mikro-jage enkelte budbringere. Interception afgøres primært af formationer, reconnaissance og kontrol af området mellem afsender og modtager.

Hvis en eller flere fjendtlige formationer kommer mellem afsender-HQ og modtageren, kan systemet:

- øge courierens ETA,
- sætte ordren i `Delayed`,
- i sjældnere tilfælde markere courier/order som `Lost` eller `Intercepted`.

Risikoen påvirkes bl.a. af enemy presence/ZOC, cavalry/scouts, own screening/escort, roads, woods/villages, terrain, daylight/night, distance og Staff/Command quality. **Intercepted** skal være mærkbart, men forholdsvis sjældent, så command-friction føles plausibel uden at blive frustrerende.

Fjendens courier-markører er underlagt fog of war. Spilleren ser ikke automatisk alle enemy couriers; de kan først blive synlige gennem scouts/cavalry/local observation.

## B-207 — Command overlay og clutter control

**BESLUTTET / PLANLAGT.** Command-links og courier progress vises ikke permanent for hele hæren.

De vises når mindst én af følgende er sand:

- et HQ er valgt,
- en enhed med aktiv ordre er valgt,
- spilleren aktiverer `Command Overlay`,
- spilleren åbner en specifik ordre i message/order UI.

Standard skal prioritere læsbarhed frem for at vise hele kommunikationsnettet konstant.

Hvis flere beskeder samtidig bevæger sig på samme link, kan markørerne få en lille side-offset eller samles i et kompakt tællerikon, så de ikke ligger præcis oven i hinanden.

## B-208 — Order status på HQ og unit card

**PLANLAGT.** HQ/unit UI viser samme order lifecycle som simulationen:

`Drafted → Sent → In transit → Delivered → Acknowledged → Executing → Superseded/Failed`

Det skal være muligt at skelne mellem:

- ordren er sendt,
- courieren er på vej,
- ordren er leveret,
- officeren har forstået/acknowledged,
- formationen udfører den faktisk.

Ved aktiv ordre kan unit card vise fx:

`Pending order: FORSVAR HER | ETA 00:18 | Courier 63%`

## B-209 — Kobling til Officer AI

**BESLUTTET / DELVIST IMPLEMENTERET.** v00.00.09 har allerede immediate Officer AI missions, doctrine, commander aggression intent og fire policy. Courier/HQ-systemet bliver senere transportlaget under samme controller.

Når `AI UNIT ON` i den senere command model:

- officeren fortsætter sin nuværende mission og lokale autonomi,
- en ny spiller-/overordnet ordre ændrer ikke missionen før ordren faktisk er delivered/understood,
- officerens Staff/Command Skill, Discipline, Initiative og Composure kan påvirke acknowledgement, fortolkning og reaction efter levering.

Når `AI UNIT OFF` i høj realism mode gælder samme transportfriktion: direkte spillerinput repræsenterer en HQ-ordre og må ikke teleporteres.

## Prioritet efter v00.00.09 runtime-gate

Range fan, fire policy og første continuous accuracy-by-range er nu flyttet ind i selve v00.00.09 TEST. Efter runtime-gaten er anbefalet rækkefølge derfor:

1. **Fire eligibility phase 2** — segmenteret frontage/LOS/target exposure oven på den nye forward fire arc.
2. **HQ entity + command relationship overlay**.
3. **Courier/order lifecycle MVP** med ryttermarkør på eksisterende command-links.
4. **Fog-of-war/scouts + command effectiveness** koblet til HQ/courier.
5. **Semantic zoom / NATO-symbol mode**.
6. Derefter dybere courier failure, reports/acknowledgements og højere-level brigade/division AI.