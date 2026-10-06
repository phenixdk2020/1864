# PROJECT 1864 — Designmanual v00.02.09 — Del 3D

## 19.1 Battle supply og resupply under kamp

Ammunition og andre kritiske forsyninger er konkrete beholdninger. En formation må ikke regenerere ammunition automatisk blot fordi den ikke skyder.

Battle resupply kræver en supply source med kompatibel beholdning, transportkapacitet, en brugbar rute og tid til fysisk distribution. Forward ammunition wagons/caissons kan placeres tættere på fronten for hurtigere resupply, mens større field trains normalt ligger længere tilbage og er mindre udsatte.

Resupply efficiency afhænger af distance, terræn/vej, enemy interdiction, formationens movement/combat pressure, available wagons/horses/drivers og staff/logistics quality. En formation i aktiv ildkamp kan derfor blive delvist genforsynet eller slet ikke modtage forsyninger, selv om en overordnet supply line stadig eksisterer.

## 19.2 Skumring, nat og flerdagsslag

Tactical battle har en lys-/tidsfase:

`DAYLIGHT -> DUSK -> NIGHT -> DAWN -> DAYLIGHT`

Skumring er normalt overgangen fra organiseret dagskamp til en natlig operationsfase, men er ikke et universelt hard stop for selve battle instance.

Under natten kan enheder:

- holde position,
- trække sig,
- gennemføre begrænset repositionering,
- samle/evakuere sårede,
- hvile og reorganisere,
- forbedre hasty fieldworks,
- sende couriers/reports,
- modtage ammunition, food, forage og medical supply,
- udføre patrols/screens,
- gennemføre begrænset night attack/infiltration, hvis order/doctrine/officer tillader det.

Normal formation combat reduceres kraftigt af dårlig LOS, command/control og cohesion. Night attack er derfor et specialvalg med stor friktion, ikke standard-adfærd.

## 19.3 Overnight resupply

Natten giver et naturligt resupply-vindue, men resupply er betinget af fysisk forbindelse til egne linjer.

En formation kan kun få overnight resupply, hvis:

- supply route til eget field train/depot er åben eller kan genetableres,
- source har relevante stocks,
- transportkapacitet eksisterer,
- fjenden ikke kontrollerer/interdict'er ruten effektivt,
- formationen kan modtage/distribuere forsyningen.

En afskåret formation får ingen magisk natlig ammunition. Den må fortsætte på carried stocks, erobret materiel eller genetablere forbindelsen.

Overnight resupply kan være delvis. Spilleren kan senere sætte resupply priority, og infanteri-/artilleriammunition behandles som separate beholdninger.

## 19.4 Daylight, dusk og auto-pause

Sunrise/sunset/dusk skal senere afledes af dato, geografisk position og relevant weather/light state. Et fast universelt “slag slutter kl. 18” er ikke ønsket.

Ved skumring kan UI auto-pause og præsentere eksempelvis:

`Hold positions | Withdraw | Continue night operations | Reorganize/resupply | Prepare dawn attack`

Auto-pause er en hjælp til command-level beslutninger og kan senere være en assistance-setting.

Hvis begge sider stadig er i kontakt efter natten, fortsætter samme battle næste dag med persistent casualties, ammunition, fatigue, morale/cohesion, damaged/captured equipment, fieldworks, officer state, positions og supply connectivity.

## 19.5 Kavaleri, Gardehusarer og dragoner

Kavaleri er en operations- og informationsressource, ikke kun en charge-enhed.

Danske **Gardehusarer og dragoner er separate cavalry-typer**, men skal bygges på en fælles cavalry core. Gardehusarer repræsenterer let kavaleri med stærk rolle i reconnaissance, screening, flankering, pursuit, raids og exploitation. Dragoner skal desuden kunne sidde af og kæmpe med skydevåben.

Dansk cavalry må ikke modelleres som kun sabel. Weapon loadout er data-driven; 1864-profiler kan omfatte **sabel, karabin og pistol** afhængigt af konkret regiment og rolle. Endelige regimentsspecifikke våbenprofiler skal historisk verificeres før combat stats låses.

Centrale roller:

- reconnaissance,
- screening/counter-recon,
- flank security,
- courier support/escort,
- pursuit af routed enemy,
- raids mod supply/courier/telegraph routes,
- exploitation af gaps,
- beskyttelse af HQ, artillery og trains.

Mounted mobility afhænger af horse fatigue, horse casualties, terrain og cohesion. Woods, ditches, fences, villages, steep ground og narrow crossings kan gøre mounted maneuver eller charge ineffektiv.

## 19.6 Dragoner mounted/dismounted

Dragoner kan bevæge sig mounted og sidde af til sustained fire/combat. Ved dismount efterlades horses og horse holders som en reel tactical state. Remount tager tid og afhænger af, om hestene stadig er til rådighed og organiserede.

En dragoon formation må ikke samtidig få fordelene ved mounted mobility og dismounted fire capability. Dismounted combat bruger formationens konkrete weapon profile og relevante infantry-like combat rules.

Fælles cavalry state-model bør mindst understøtte:

`MOUNTED -> WALK/TROT/GALLOP -> CHARGE -> MELEE`

og for dragoner:

`MOUNTED -> DISMOUNTING -> DISMOUNTED -> FIRE/MELEE -> REMOUNTING -> MOUNTED`

Heste og horse holders er tactical assets, ikke ren animation.

## 19.7 Charge, facing og shock

Charge-effekt skal afhænge af target state, facing, terrain, cavalry fatigue/cohesion, momentum, surprise, support og officer quality.

Et frontalt charge mod steady formed infantry med god fire discipline er meget risikabelt. Cavalry er langt farligere mod routed/disordered infantry, exposed skirmishers, retreating artillery/transport og isolerede rear-area targets.

Impact direction er central:

- **FRONT:** laveste cavalry shock-bonus mod infantry, hvis infanteriet står klar og vender korrekt.
- **FLANK:** markant højere casualties, cohesion loss og morale shock.
- **REAR:** meget høj shock-effekt, især hvis infantry allerede er engageret forfra. Rear charge skal kunne give store immediate casualties/stragglers, severe cohesion/morale shock, disorder og høj break/rout probability.

Rear/flank-effekten forstærkes yderligere, hvis infantry er i Column, i bevægelse, under fire, allerede engaged, uforberedt eller har lav cohesion/morale.

## 19.8 Defensive fire mod cavalry charge

Et mounted charge er ikke garanteret at nå fysisk kontakt. Defensive fire påvirker både casualties og selve chargens vilje/sammenhæng.

Cavalry charge skal have en særskilt `Charge Confidence` / `Charge Momentum` vurdering. Defensive fire kan påvirke:

- horse hesitation,
- rider/horse casualties,
- cavalry cohesion,
- morale,
- formation integrity,
- speed/momentum,
- willingness to continue.

Særligt en vel-timet **close-range volley** fra steady infantry skal have reel mulighed for at få et frontalt charge til at:

`CONTINUE -> FALTER -> ABORT -> ROUT`

Long-range fire giver normalt mindre shock; Medium kan begynde at nedbryde chargen; Close lige før kontakt kan udløse et stærkt abort-check. Det giver et vigtigt timing-element: for tidlig volley giver mindre anti-charge shock, mens for sen volley risikerer at komme efter kontakt.

## 19.9 Infantry anti-cavalry formation — SQUARE / KARRÉ

Infanteriets basisformationer er designmæssigt:

`LINE / COLUMN / SQUARE`

Square er en specifik anti-cavalry formation og ikke en universel defensiv bonus.

- Formation change tager tid og skal være fysisk synlig.
- Bajonetter/frontage orienteres ud mod alle sider.
- Square har meget lav mobility og dårligere fleksibilitet end Line.
- Et færdigt, steady square er meget svært og dyrt at charge med cavalry.
- Square reducerer kraftigt betydningen af cavalry flank/rear, fordi formationen har organiserede fronter i flere retninger.
- Square må ikke give én 360° full-strength volley; firing eligibility fordeles mellem de sider, der faktisk har targets.
- Square er et tæt mål og er derfor mere sårbart over for artilleri og koncentreret infantry fire.
- Hvis cavalry rammer mens square stadig dannes, kan infanteriet få kraftig disorder, cohesion shock og høj break/rout risk.
- Når cavalry-truslen er væk, skal `REFORM LINE` eller anden relevant formation tage tid.

Officer AI må ikke danne square bare fordi cavalry findes på kortet. Den skal beregne en cavalry-threat score ud fra distance, speed/time-to-contact, approach angle, mounted strength/cohesion, terrain, egen morale/cohesion, støtte og konkurrerende threats.

## 19.10 Cavalry feint og combined-arms effekt

Cavalry behøver ikke fysisk charge for at skabe taktisk effekt. En troværdig mounted threat kan tvinge infantry til at danne square, hvilket:

- reducerer mobility,
- reducerer effektiv frontal firepower,
- gør formationen til et tættere artillery target,
- kan skabe muligheder for infantry/artillery cooperation.

Cavalry AI bør normalt ikke udføre et selvmorderisk frontalcharge mod et steady, intakt square. Officerstats, dårlig information eller ekstrem mission pressure kan senere skabe undtagelser.

## 19.11 Pursuit

Pursuit kan øge prisoners, captured guns/wagons og missing/straggler outcomes, men koster fatigue og kan føre til overextension.

Pursuit er især effektiv mod routed/disordered infantry, retreating artillery og transport, men langt mindre sikker mod steady formed infantry.

## 19.12 Kobling til Officer AI, Regimental HQ og supply

Officer AI skal kunne vælge mellem screen, recon, hold, charge, pursuit, dismount/remount og raid efter mission, knowledge state, officer stats og unit condition.

Når F28 Regimental HQ-kæden er stabil, skal cavalry genbruge samme command-princip:

`Oberstløjtnant -> Major/Ritmester -> cavalry formation`

Kavaleri må ikke få et separat telepatisk control-system. Det skal bruge samme authority, destination, command-zone og senere courier/order-delay model som infantry.

Cavalry raids og counter-recon kobles direkte til supply- og courier-systemet: en cavalry formation kan true en supply route eller communication link uden at erobre hele regionen. Dette kan igen forhindre overnight resupply og skabe command delay.