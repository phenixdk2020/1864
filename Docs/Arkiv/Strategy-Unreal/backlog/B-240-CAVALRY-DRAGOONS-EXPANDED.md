# PROJECT 1864 — B-240–B-249: Kavaleri, Gardehusarer og dragoner — udvidet model

**Status: BESLUTTET / NÆSTE VÅBENART EFTER STABIL F28 REGIMENT-CONTROL**  
**Designbaseline: v00.02.09**

Dette supplement udvider de eksisterende cavalry-beslutninger. Kavaleri skal ikke reduceres til en charge-knap. Dets værdi ligger i reconnaissance, screening, pursuit, communication, flank security, raids og exploitation, mens dragoner også kan kæmpe afsiddet.

Danske **Gardehusarer og dragoner behandles som separate typer på én fælles cavalry core**. Gardehusarer er let kavaleri; dragoner får desuden mounted/dismounted gameplay. Dansk cavalry modelleres ikke som kun sabel: weapon profiles er data-driven og kan omfatte **sabel + karabin + pistol** afhængigt af konkret regiment/rolle. Endelige 1864-loadouts og stats skal historisk verificeres før locking.

## B-240 — Mounted combat state

**BESLUTTET.** Mounted cavalry/dragoon formationer har egne states for:

- mounted readiness,
- horse fatigue,
- horse casualties,
- formation cohesion,
- terrain suitability,
- charge momentum,
- charge confidence,
- remount availability.

Hestens state er separat fra rytterens personelstate.

Fælles mounted state-chain bør mindst understøtte:

`MOUNTED -> WALK/TROT/GALLOP -> CHARGE -> MELEE`

## B-241 — Dragoner mounted/dismounted

**BESLUTTET.** Dragoner kan:

- marchere og manøvrere mounted,
- dismount for sustained fire/combat,
- efterlade horses ved horse holders,
- remount efter en tidskrævende reorganisering.

State-chain:

`MOUNTED -> DISMOUNTING -> DISMOUNTED -> FIRE/MELEE -> REMOUNTING -> MOUNTED`

Dismounted dragoner behandles kampmæssigt som en let/kompakt infantry-like formation med deres konkrete weapon profile; de må ikke få mounted mobility samtidig.

## B-242 — Horse holders og remount risk

**BESLUTTET.** Når dragoner kæmper afsiddet, eksisterer hestene som en reel tactical asset bag/ved formationen.

- horse holders kan blive ramt,
- heste kan blive dræbt, såret, spredt eller panikke,
- remount tager tid,
- tab af heste reducerer efterfølgende strategic mobility,
- en afsiddet enhed kan i værste fald blive tvunget til at fortsætte som fodtropper.

## B-243 — Reconnaissance og screening

**BESLUTTET.** Cavalry er en vigtig sensor-/counter-sensor-komponent.

Roller omfatter:

- forward reconnaissance,
- flank reconnaissance,
- screening af egne march columns/HQ,
- counter-recon mod fjendens scouts/cavalry,
- observation af roads/bridges/fords,
- maintaining contact med en withdrawing enemy.

Resultatet feeds ind i den eksisterende fog-of-war/knowledge-state model.

## B-244 — Courier support og HQ communication

**BESLUTTET.** Mounted personnel understøtter command networket.

Kavaleri/dragon assets kan senere:

- stille couriers/escorts til rådighed,
- beskytte HQ/order routes,
- jage fjendtlige couriers,
- sikre forbindelse mellem detached formations.

Det giver en direkte kobling mellem cavalry, HQ-systemet og order delay. Cavalry skal genbruge samme authority-, command-zone- og courier-model som infantry; ingen særskilt telepatisk control stack.

## B-245 — Pursuit og exploitation

**BESLUTTET.** Cavalry er særlig effektiv mod:

- routed/disordered infantry,
- retreating artillery/transport,
- exposed supply columns,
- isolated skirmishers,
- broken formations uden støtte.

Pursuit kan øge prisoners, captured guns/wagons og missing casualties, men giver fatigue og risiko for overextension.

## B-246 — Charge model

**BESLUTTET / NÆSTE IMPLEMENTERING EFTER SQUARE-GATE.** Charge-effekt afhænger ikke af en universel cavalry bonus.

Vigtige inputs:

- target formation/state,
- impact angle: FRONT / FLANK / REAR,
- cavalry cohesion/momentum/fatigue,
- charge confidence,
- terrain/obstacles,
- surprise,
- support,
- weapon/doctrine,
- officer stats,
- target fire discipline/morale/cohesion,
- om target allerede er engaged/under fire/moving.

### FRONT

Et frontalt charge mod steady, correctly-facing infantry med god fire discipline er risikabelt. Defensive fire kan reducere momentum og få cavalry til at afbryde angrebet før fysisk kontakt.

### FLANK

Flank charge giver større shock, fordi infantry har dårligere mulighed for at bringe frontage/fire mod angriberen. Det øger casualty-, cohesion- og morale-effekt.

### REAR

Rear charge er en af cavalry-systemets mest destruktive kontakter mod infantry. Især et regiment/kompagni, der allerede kæmper forfra, skal kunne lide:

- høje immediate casualties/stragglers,
- severe cohesion shock,
- severe morale shock,
- formation disorder,
- høj break/rout probability.

Effekten forstærkes mod infantry i Column, under movement, under fire, already engaged, uforberedt eller med lav cohesion/morale.

### Charge Confidence og defensive volley

Mounted charge skal løbende have en `Charge Confidence / Charge Momentum` vurdering. Defensive fire påvirker ikke kun casualties, men også:

- horse hesitation,
- rider/horse losses,
- cavalry cohesion,
- morale,
- speed/momentum,
- willingness to continue.

Prototype outcomes:

`CONTINUE -> FALTER -> ABORT -> ROUT`

En vel-timet **close-range infantry volley** skal have reel mulighed for at stoppe eller bryde et frontalt charge. Long fire giver normalt mindre shock; Medium kan begynde at nedbryde chargen; Close lige før kontakt udløser det stærkeste abort-check.

## Infantry anti-cavalry response — FORM SQUARE

**BESLUTTET.** Infantry skal kunne få ordren **FORM SQUARE / KARRÉ** som specifik anti-cavalry formation.

Infantry-formationer udvides designmæssigt til:

`LINE / COLUMN / SQUARE`

Square er ikke en gratis bonus. Det er en tidskrævende formation change, hvor formationen præsenterer organiserede fronter/bajonetter udad på flere sider.

Regler/egenskaber:

- Square kan gives manuelt af spilleren og vælges automatisk af Officer AI.
- Formation change tager tid; et regiment der overraskes midt i deployment kan blive ramt før square er færdig.
- Høj Cohesion, Discipline og Composure gør formationen hurtigere og mere stabil.
- Lav Morale/Cohesion, flank/rear surprise eller meget kort warning distance øger risikoen for mislykket/ufuldstændig square.
- Et færdigt, steady square skal være meget svært og dyrt at charge med cavalry/dragoner.
- Square reducerer kraftigt betydningen af cavalry flank/rear, fordi formationen har front i flere retninger.
- Square reducerer mobility kraftigt og gør regimentet mindre fleksibelt mod infantry fire/manoeuvre.
- Square præsenterer et tæt mål og er derfor mere sårbart over for artillery og koncentreret infantry fire.
- Square må ikke give 360° full-strength volley. Firing eligibility fordeles mellem relevante square-sider efter targets/frontage.
- Når cavalry-truslen falder, skal officer/player kunne **REFORM LINE**; reform tager også tid.

Officer AI må ikke reagere på ordet "cavalry" alene. Den skal bruge en dynamisk cavalry-threat score baseret på blandt andet:

- om mounted enemy faktisk charger/accelererer mod enheden,
- distance og estimated time-to-contact,
- approach angle FRONT/FLANK/REAR,
- enemy mounted strength/cohesion,
- eget terrain og obstacles,
- støtte fra nabo-infanteri/artilleri,
- eget Morale/Cohesion,
- Officer Tactical Skill, Initiative, Discipline og Composure,
- konkurrerende threats, især nært artillery/infantry fire.

Eksempel: en lille cavalry-enhed 200 m væk skal normalt ikke få et regiment til at forlade en god firing line. Et hurtigt mounted charge på kort afstand mod en eksponeret flank skal derimod kunne udløse FORM SQUARE straks, hvis officerens observation/reaction er god nok.

### Cavalry feint / combined-arms pressure

Cavalry behøver ikke charge for at skabe effekt. En troværdig mounted threat kan presse infantry til Square og dermed:

- reducere mobility,
- reducere effektiv frontal firepower,
- gøre infantry til et bedre artillery target,
- skabe muligheder for coordinated infantry/artillery attack.

Cavalry AI bør normalt ikke suicidal-charge et steady, intakt Square, men officerstats, dårlig information eller ekstrem mission pressure kan senere give fejlbeslutninger.

## B-247 — Terrain restrictions

**BESLUTTET.** Cavalry mobility og charge quality påvirkes kraftigt af:

- woods,
- ditches,
- fences/walls,
- marsh/soft ground,
- steep slopes,
- narrow roads/bridges,
- villages/buildings.

Terrain fitting/pathfinding skal derfor behandle mounted formations anderledes end infantry.

## B-248 — Raids, supply interdiction og rear-area operations

**BESLUTTET.** På operations-/strategikortet kan cavalry bruges til at:

- raid supply routes,
- capture/destroy wagons,
- threaten depots,
- cut couriers/telegraph links,
- force enemy escorts/screens,
- probe crossings and roads.

Det skal ikke kræve, at en hel provins er erobret for at påvirke fjendens logistics.

## B-249 — Cavalry/dragoon acceptance criteria

Næste cavalry-prototype skal mindst verificere:

1. Mounted og dismounted state har reelt forskellig mobility/combat capability.
2. Dismount efterlader horses/holders som separat tactical state.
3. Remount tager tid og fejler/delvist reduceres hvis heste er mistet.
4. Cavalry recon forbedrer egen knowledge state uden omniscience.
5. Screen kan reducere enemy reconnaissance quality.
6. Pursuit mod routed enemy giver væsentligt større capture/straggler-risk end mod steady formations.
7. Charge-resultat varierer kraftigt efter FRONT/FLANK/REAR, target state og terrain.
8. Rear charge mod already-engaged infantry giver stort morale/cohesion shock og høj break/rout risk.
9. En close-range volley fra steady infantry kan få et frontalt cavalry charge til at FALTER/ABORT/ROUT.
10. Woods/fences/ditches reducerer mounted maneuver/charge effectiveness.
11. Cavalry kan true/cut supply/courier routes.
12. Officer AI kan vælge mellem screen/recon/hold/charge/pursuit/dismount efter mission og officer stats.
13. Infantry kan manuelt FORM SQUARE og senere REFORM LINE.
14. Officer AI kan vælge FORM SQUARE ved en konkret høj mounted-charge threat, men undgår unødvendig square mod lav/fjern cavalry threat.
15. Et steady square reducerer cavalry charge success markant, mens et sent/ufuldstændigt square kan brydes.
16. Square har reel trade-off: lav mobility og større vulnerability mod artillery/concentrated fire.
17. Square-fire fordeles på relevante sider og må ikke fungere som 360° full-strength volley.
18. Cavalry AI kan vælge at afbryde et dårligt frontalcharge i stedet for altid at fortsætte til melee.

## Historical research gate

De konkrete 1864-regimentstyper, styrker, weapons, remount practice, doctrine og nation-specifikke forskelle mellem dansk, preussisk og allieret kavaleri/dragoner skal research-verificeres, før endelige stats låses. Dette dokument fastlægger systemarkitekturen, ikke historiske numerical ratings.