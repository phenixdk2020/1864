# PROJECT 1864 — B-250–B-259: Fog of war, scouts og command effectiveness

**Status: BESLUTTET / PLANLAGT EFTER P0A v00.00.09**  
**Designbaseline: v00.02.08**

Dette supplement fastlægger den fælles model for fog of war, reconnaissance/scouts og HQ-command effectiveness. Systemet skal bygge oven på de eksisterende officerstats, HQ-entities, couriers/order lifecycle og knowledge-state. Det må ikke reduceres til en global synsradius eller en hård magisk command-cirkel.

## B-250 — Fog of war er knowledge state, ikke bare skjult grafik

**BESLUTTET.** Hver side og relevante HQ-/formationsniveauer arbejder ud fra observeret information med alder og usikkerhed. Simulationen kender den sande world state, men spillerens HQ og Officer AI må kun bruge den knowledge state, de faktisk har adgang til.

En fjendtlig formation kan mindst være:

- **Unknown** — ingen brugbar observation.
- **Suspected** — mulig aktivitet/rygte/indirekte tegn.
- **Contact** — fjendtlig tilstedeværelse observeret, men type/styrke er usikker.
- **Identified** — type/echelon eller konkret enhed er vurderet med rimelig confidence.
- **Fresh observation** — relativt præcis position og bedre strength/state-estimat.
- **Stale** — informationen er gammel; last known position bevares, men confidence falder.

En fjende forsvinder derfor ikke nødvendigvis øjeblikkeligt, når LOS brydes. UI viser last known position, observation age og confidence i stedet for perfekt live-tracking.

## B-251 — Scouts og reconnaissance sources

**BESLUTTET.** Reconnaissance kommer fra faktiske formationer/assets og forskellige kilder:

- cavalry patrols og mounted scouts,
- dragoner i recon/screen mission,
- infantry skirmishers/scout detachments,
- observation fra line units/HQ,
- højt terræn/observation points,
- civilians/local reports,
- couriers og reports fra underordnede,
- senere telegraph/naval/fortification observation hvor relevant.

Forskellige kilder har forskellig range, speed, stealth, reliability og report delay.

## B-252 — Spotting og identification

**PLANLAGT.** Observation afgøres ikke af én fast cirkel. Vigtige inputs omfatter:

- afstand,
- terrain/vegetation/buildings,
- elevation,
- daylight/dusk/night,
- weather,
- smoke,
- target formation size/density,
- target movement,
- firing/smoke/signature,
- scout quality/training,
- officer/staff interpretation,
- enemy screening/counter-recon.

At se "noget" er ikke det samme som at identificere regiment, styrke og morale præcist.

## B-253 — Reports skal tilbage til HQ

**BESLUTTET.** En scout eller frontenhed kan opdage fjenden før overordnet HQ ved det.

Observationen går gennem command/reporting-nettet:

`Scout/front unit -> local officer/HQ -> courier/report -> superior HQ`

Derfor kan:

- en lokal officer reagere på noget, division HQ endnu ikke kender,
- spilleren i høj realism mode først få information, når rapporten når eget HQ,
- en courier/report blive forsinket, omdirigeret eller gå tabt,
- HQ flytte sig og gøre rapportlevering vanskeligere.

Dette er samme transportprincip som for ordrer, blot i modsat retning.

## B-254 — Command effectiveness field, ikke hard radius

**BESLUTTET.** HQ får et visuelt command envelope, men det repræsenterer **gradvis command effectiveness**, ikke en magisk maksimal afstand.

Første konceptuelle bands:

1. **Command Core** — tæt/velknyttet forbindelse; lav ekstra friction.
2. **Supported** — normal feltkommando gennem couriers/roads/under-HQ'er.
3. **Extended** — længere eller vanskelig forbindelse; mere delay/usikkerhed.
4. **Detached** — formationen opererer med betydelig lokal autonomi og sjældnere kontakt.
5. **Isolated** — ingen brugbar aktuel command link; formationen fortsætter sidste gyldige mission og egen lokale vurdering.

Der skal ikke være et kunstigt spring, hvor en enhed ved én meter uden for en cirkel pludselig mister command bonus.

## B-255 — Command effectiveness påvirker friktion, ikke rå combat cheats

**BESLUTTET.** Dårlig command connectivity skal primært påvirke command/control:

- større order delay,
- større usikkerhed om order status,
- langsommere acknowledgement/reporting,
- mindre koordinering med naboformationer,
- sværere reserve- og supportreaktion,
- langsommere rally/reorganisation hvis relevant commander/staff-support mangler,
- større afhængighed af lokal officer Initiative/Tactical Skill/Composure,
- større risiko for at fortsætte en forældet mission.

Det skal **ikke** direkte give en vilkårlig accuracy- eller damage-penalty alene fordi enheden er langt fra HQ. Combat påvirkes indirekte gennem dårligere information, timing, formation, morale support og coordination.

## B-256 — Hierarkisk command chain

**BESLUTTET.** Division HQ behøver ikke have en enorm direkte cirkel rundt om alle regimenter. Command effectiveness følger kommandokæden.

Eksempel:

`Division HQ -> Brigade HQ -> Regiment/Battalion`

Hvis regimentet er langt fra Division HQ men stadig har god forbindelse til sit Brigade HQ, og Brigade HQ har god forbindelse tilbage til Division HQ, kan command chain stadig være effektiv.

Omvendt kan en brigade tæt på Division HQ være command-isoleret, hvis dens eget HQ er disrupted/captured eller communication routes er skåret over.

## B-257 — UI for command envelope

**PLANLAGT.** Når et HQ vælges eller Command Overlay aktiveres, vises command effectiveness som diskrete contours/envelopes.

Første prototype kan bruge omtrent cirkulære/elliptiske bands for læsbarhed. Senere bør envelope deformeres af faktisk cost-distance:

- roads og gode courier routes udvider effektiv command reach,
- woods, marsh, steep terrain og rivers uden crossings reducerer den,
- telegraph/field communication kan skabe stærke links mellem bestemte nodes,
- enemy interdiction kan skabe "skår" eller svage sektorer.

UI må ikke afhænge af farve alene; contour-style, labels og line-pattern skal også vise Core/Supported/Extended/Detached.

## B-258 — Scouts, screens og counter-recon

**BESLUTTET.** Screening bliver en aktiv mission.

En cavalry/skirmisher screen kan:

- opdage fjendtlige scouts tidligere,
- reducere enemy observation confidence,
- beskytte HQ/courier/supply approaches,
- holde fjenden længere væk fra hovedformationens præcise disposition,
- skabe egne reports/early warning.

Counter-recon betyder ikke automatisk at alle fjendtlige scouts bliver fysisk destrueret. Det kan også blot drive dem væk, tvinge dem til større afstand eller reducere kvaliteten af deres rapporter.

## B-259 — Acceptance criteria

Senere prototype skal mindst verificere:

1. Fjendtlig formation uden observation er ikke tilgængelig som perfekt AI/player target-data.
2. En scout kan opdage en fjende før overordnet HQ modtager rapporten.
3. Report delay følger courier/command-systemet og kan blive forsinket.
4. Last known position persisterer med faldende confidence efter tabt kontakt.
5. Night/woods/smoke reducerer observation/identification på forklarlig måde.
6. Selected HQ viser et command effectiveness envelope med mindst Core/Supported/Extended/Detached.
7. En enhed længere ude får gradvist mere command friction, ikke en binær hard-radius penalty.
8. En intakt Brigade-HQ-link kan holde et regiment effektivt forbundet, selv når Division HQ er længere væk.
9. Afskåret HQ/courier route kan gøre en formation Detached/Isolated uden at ændre dens rå weapon stats.
10. Officer AI reagerer forskelligt på samme situation afhængigt af lokal knowledge state, command connectivity og officerstats.

## Scopeværn

Dette implementeres ikke i den første P0A v00.00.09 Officer AI-gate. v00.00.09 validerer shared officer-AI og time controls først. Fog-of-war/scouts/command effectiveness designes nu, så det efterfølgende kan erstatte prototype-AI'ens perfekte nearest-enemy knowledge uden at omskrive officer-AI-kernen.
