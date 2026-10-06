# PROJECT 1864 — Backlog B-210–B-219: Fire eligibility og højere formationsopstilling

**Status: BESLUTTET / PLANLAGT**  
**Designbaseline: v00.02.08**  
**Implementering: efter første P0A v00.00.09 Officer AI-gate**

Dette supplement fastlægger to nært forbundne systemer: hvor stor en del af en formation der faktisk kan deltage i en salve, og hvordan højere formationer som brigader/divisioner opstiller deres underenheder i linje, reserve og kombination med artilleri.

## B-210 — Hele regimentet må ikke automatisk skyde

**BESLUTTET.** Det er ikke nok, at en fjendtlig formation som helhed ligger inden for `MaximumRange`.

Combat resolution skal først beregne en **eligible firing fraction**: hvor stor en del af den skydende formation der faktisk kan bidrage mod det valgte mål.

Eksempel:

- 600 mand authoritative strength.
- 58 % af styrken ville normalt være aktiv i en prototype-salve.
- Kun 50 % af regimentets relevante frontage har frit LOS og mål inden for fire arc.
- Salven beregnes derfor ud fra ca. halvdelen af den ellers eligible firing strength, ikke hele regimentet.

Den endelige model må ikke antage, at soldater gennem egne naboer, terræn, bygninger, skov, røg eller en skæv formation kan skyde uhindret.

## B-211 — Fire arc / skudkegle

**BESLUTTET.** Formationer har et retningsbestemt fire arc centreret omkring facing.

Fire arc bestemmes af mindst:

- formationstype,
- våbentype,
- facing,
- shooter position i formationen,
- fire discipline,
- target bearing,
- eventuelle doctrine-/training-begrænsninger.

En line formation får en bred, fremadrettet engagement sector, men må ikke uden formationsændring levere samme koncentrerede ild direkte bagud eller gennem egen flanke.

UI kan visualisere fire arc som en sektor/kegle sammen med Close/Medium/Long range bands.

## B-212 — Formation opdeles i fire groups / frontage-segmenter

**BESLUTTET.** Combat kernel skal ikke raycaste én kugle pr. soldat. Formationens frontage opdeles i et begrænset antal logiske segmenter/fire groups.

Første tekniske retning:

- fx 8–16 frontage-segmenter pr. bataljon/regiment afhængigt af formation og størrelse,
- hvert segment har en repræsentativ origin/facing,
- segmentet evalueres mod target frontage,
- resultatet giver `eligibleFiringFraction` mellem 0 og 1.

Det giver en performant mellemvej mellem én global LOS-test og individuel soldier ballistics.

## B-213 — Fire eligibility checks

**BESLUTTET.** Et fire-group tæller kun som eligible, hvis mindst følgende krav er opfyldt:

1. Målet ligger inden for segmentets tilladte fire arc.
2. Målet ligger inden for våbnets `MaximumRange`.
3. Segmentet har relevant line of sight til mindst en del af target frontage.
4. Egen formation/venlige formationer må ikke blokere ildlinjen på en måde der burde forhindre skydning.
5. Terræn/bygninger/fieldworks/røg reducerer eller blokerer LOS efter de senere simulation rules.
6. Enheden er i en state, hvor den faktisk må skyde: ikke routed, ikke midt i inkompatibel movement/reformation, ikke Hold Fire osv.

Resultatet aggregeres til en `eligibleFiringFraction` og eventuelt en `targetExposureFraction`.

## B-214 — Target frontage og delvis eksponering

**BESLUTTET.** Det er ikke kun skytten, der kan være delvist eligible; målet kan også være delvist synligt.

Eksempler:

- kun fjendens højre fløj stikker frem bag en bygning,
- en formation ligger delvist bag en jordfold,
- røg skjuler midten men ikke begge fløje,
- en skrå vinkel gør kun en begrænset del af target frontage synlig.

Combat kernel skal derfor kunne bruge både:

`Shooter eligible fraction × target exposure × accuracy-by-range × øvrige modifiers`

Dette er stadig aggregate combat resolution; vi undgår individuel projektilsimulering som authoritative model.

## B-215 — Close / Medium / Long kobles til fire arc

**BESLUTTET.** Range overlay og fire arc er ét samlet targeting-visual.

Når `Range`/fire-control overlay er aktivt, skal spilleren kunne se:

- formationens fremadrettede fire sector,
- Close-band,
- Medium-band,
- Long-band,
- MaximumRange,
- senere evt. områder der er blokeret af terræn/LOS som dæmpede eller skraverede zoner.

De stiplede range-linjer viser kun afstandsbands. Den faktiske hit chance er fortsat kontinuerlig.

## B-216 — Højere formationsskabeloner

**BESLUTTET.** Højere headquarters skal kunne placere deres direkte underenheder efter formation templates i stedet for kun individuelle world positions.

En template består af relative slots omkring parent HQ/formation anchor. Hvert slot kan have:

- role: frontline / reserve / flank / artillery / support,
- relative position,
- facing,
- frontage/depth expectation,
- allowed unit types,
- priority,
- spacing,
- fallback/reform anchor.

Template beskriver ønsket disposition; terrain fitting og local AI må justere den til faktiske forhold.

## B-217 — Første formation templates

**PLANLAGT.** Første sæt bør mindst omfatte:

- **4 abreast** — fire underformationer side om side; maksimal frontage, lille central reserve.
- **3 + 1 reserve** — tre i første linje, én centreret bagved som reserve.
- **2 + 2** — to i front og to i anden linje/reserve.
- **2 + 1 + 1 deep reserve** — smallere frontage med dybere reserve.
- **Echelon left/right** — forskudte underformationer til flankebeskyttelse/angreb.
- **Column / road march** — underformationer efter hinanden på marchakse.
- **Infantry + artillery centre** — infanteri dækker begge sider, artilleri i et centralt eller terrænmæssigt egnet fire position slot.
- **Infantry + artillery wing** — artilleri på venstre/højre fløj med infantry protection.
- **Infantry front + artillery rear/high ground** — artilleri bag fronten på egnet elevation/LOS.

Det præcise historiske repertoire og nationale doctrine skal research-valideres. Templates er derfor taktiske designrammer, ikke påstand om at alle nationer i 1864 brugte alle mønstre ens.

## B-218 — Reserve er en faktisk rolle

**BESLUTTET.** En reserve er ikke bare en enhed, der står bagved. Reserve-role skal have state/intent.

En reserve kan fx bruges til:

- lukke et hul,
- forstærke truet flanke,
- counterattack,
- erstatte udmattet/shaken frontline,
- beskytte HQ/artilleri,
- dække withdrawal.

Officer-AI må ikke automatisk sende alle reserver ind ved første kontakt. Leadership, Tactical Skill, Initiative, Aggressiveness/Caution, Discipline, Composure, mission og situation skal påvirke hvornår reserven frigives.

## B-219 — Officer AI vælger og tilpasser formation template

**BESLUTTET.** På brigade/divisionsniveau bliver formation selection en del af officerens AI-beslutning.

Input kan mindst være:

- mission/commander intent,
- antal og type underenheder,
- terrain frontage,
- observation/FoW,
- forventet enemy axis,
- artillery support,
- flank threats,
- reserve requirement,
- officer stats/personality,
- doctrine og training.

Eksempler:

- høj Tactical Skill + defensiv mission kan favorisere `3 + 1 reserve` på en bred højderyg,
- aggressiv commander kan vælge bredere frontage og mindre reserve,
- forsigtig commander kan holde en større reserve,
- stærkt attached artillery kan udløse en template med beskyttet gun position og infantry support.

### Terrain fitting

Template-slots er målpositioner, ikke absolutte teleport-punkter. Formation AI skal flytte slots til nærmeste taktisk brugbare terræn og bevare:

- spacing,
- facing,
- LOS,
- vej/obstacle constraints,
- cover,
- artillery arcs,
- command connectivity.

## Acceptance-idéer for senere combat/formation-test

Når systemet implementeres, skal mindst følgende kunne demonstreres:

1. Et regiment med kun ca. halvdelen af sin frontage fri mod målet leverer mærkbart lavere salvevolumen end samme regiment med fuldt frit skudfelt.
2. Et mål uden for fire arc kan ikke engageres, selv hvis center-to-center distance er kort.
3. En friendly formation mellem shooter og target kan reducere/blokere eligible firing fraction.
4. En formation, der drejes korrekt mod målet, får større eligible firing fraction.
5. Close/Medium/Long overlay følger samme facing/fire sector.
6. Et parent HQ kan skifte mellem `4 abreast` og `3 + 1 reserve`, og underenheder får nye slot-mål uden at deres identities ændres.
7. Reserve-enheden forbliver faktisk i reserve indtil commander AI/order frigiver den.
8. Artilleri-slot må kun accepteres, hvis gun positionen har brugbar LOS/fire arc og infantry ikke står direkte i ildlinjen.
9. Officer-AI kan vælge forskellige templates for forskellige profiler/missions og give en reason code for valget.

## Prioritet

Dette system kommer **efter v00.00.09 Officer AI MVP**, men firing eligibility bør prioriteres tidligt derefter, fordi den nuværende prototype ellers overvurderer salveeffekten fra formationer med delvist eller dårligt skudfelt.
