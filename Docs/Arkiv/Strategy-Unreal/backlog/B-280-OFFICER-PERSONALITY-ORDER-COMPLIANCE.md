# B-280 — Officerpersonlighed, ordrelydighed og selvstændig fortolkning

**Status:** Senere fase / planlagt  
**Type:** Officer AI / command & control  
**Bygger videre på:** `docs/UNIT-AND-OFFICER-BEHAVIOUR-RULES.md` §§26–31

## Formål

Officerer skal ikke opføre sig som robotter. Spillerens ordre angiver chefens hensigt, men den underordnede officer fortolker ordren gennem egne stats, personlighed, erfaring og den aktuelle situation.

En ordre som **FORSIGTIG**, **BALANCERET** eller **AGGRESSIV** er derfor et commander-bias og ikke en absolut garanti for adfærd.

## Centrale officerstats

- **Leadership** — evne til at holde underordnede enheder samlet og få dem til at fortsætte en plan.
- **Inspiration** — påvirker morale/rally og hvordan underordnede reagerer på pres.
- **Tactical Skill** — kvaliteten af officerens situationsvurdering og valg af position/manøvre.
- **Initiative** — villighed til selv at handle eller fortolke en ordre, når situationen ændrer sig.
- **Staff / Command Skill** — planlægning, koordinering, ordre-delay og styring af flere underordnede enheder.
- **Discipline / Obedience** — hvor tæt officeren følger den givne ordre og doctrine.
- **Aggressiveness / Caution** — naturlig tendens til at presse frem, holde kontakt, lukke afstand eller søge sikkerhed/reserve.
- **Composure / Nerve** — evne til at bevare beslutningskvalitet under ild, tab, flanketrussel og kaos.
- **Officer Experience** — reducerer dårlig decision noise og forbedrer timing og stabilitet.

## Grundregel: ordre = hensigt, ikke robotkommando

Spillerens ordre skal altid definere den overordnede mission, fx:

- **ANGRIB HER** — tag/press området.
- **FORSVAR HER** — hold området.
- **TILBAGETRÆK HERTIL** — gennemfør organiseret retræte til angivet position.
- **RYK FREM HERTIL** — flyt formationen frem uden nødvendigvis at søge nærkamp.

Officerens stats bestemmer derefter, hvordan missionen fortolkes inden for rimelige taktiske grænser.

## Eksempel: spiller vælger FORSIGTIG

En officer med høj Aggressiveness og høj Initiative kan stadig:

- lukke længere frem end spilleren forventer;
- iværksætte et lokalt modangreb;
- bruge mindre reserve;
- fortsætte presset længere;
- søge tættere engagement range;
- vælge melee, hvis officeren vurderer muligheden som fordelagtig.

En officer med høj Discipline/Obedience vil i højere grad holde sig til den forsigtige posture.

## Eksempel: spiller vælger AGGRESSIV

En meget forsigtig officer kan stadig:

- stoppe tidligere;
- holde større reserve;
- vælge ildkamp i stedet for melee;
- undgå udsatte flanker;
- afbryde fremrykning ved store tab;
- fortolke ANGREB som pres frem til en god firing position snarere end blind storm.

## Kombinationer af stats

### Høj Initiative + høj Tactical Skill

Officer må ofte afvige fra detaljer i ordren, men gør det normalt af en taktisk forståelig årsag.

### Høj Initiative + lav Tactical Skill

Officer handler ofte på egen hånd, men beslutningerne kan være dårlige eller risikable.

### Lav Initiative + høj Obedience

Officer følger ordren meget bogstaveligt og reagerer langsommere på nye muligheder eller trusler.

### Høj Aggressiveness + lav Obedience

Stor risiko for at presse længere end commander-intent, overcommitte reserve eller søge melee.

### Lav Nerve / Composure

Under hård ild kan officeren tøve, stoppe, trække sig tidligere eller miste kvalitet i beslutningerne.

## Ikke ren tilfældighed

Afvigelser fra spillerens ordre må ikke opleves som tilfældig sabotage.

En officers beslutning skal kunne forklares af mindst én af disse faktorer:

1. personlighed/stats;
2. doctrine/posture;
3. fjendens afstand og styrke;
4. egne tab/morale/cohesion;
5. flanketrussel eller terræn;
6. reserve-status;
7. HQ-afstand/kommunikation;
8. stress/decision noise.

## Synlighed for spilleren

Spilleren skal kunne forstå, hvorfor en officer afviger.

Senere UI bør vise fx:

- **Order:** FORSVAR HER
- **Posture:** FORSIGTIG
- **Officer tendency:** Aggressiv
- **Obedience:** 42/100
- **Initiative:** 81/100
- **Current interpretation:** Lokalt modangreb
- **Reason:** Enemy flank exposed / tactical opportunity

Det gør systemet læsbart og reducerer frustration.

## Ordrestrenghed — mulig senere mekanik

En senere mulighed kan være at lade spilleren vælge hvor stramt ordren skal følges:

- **STRICT** — lav autonomi; høj prioritet på præcis ordrelydighed.
- **NORMAL** — balanceret fortolkning.
- **DISCRETION** — stor frihed til officerens Initiative/Tactical Skill.

STRICT bør have en pris, fx længere ordre-delay, dårligere lokal reaktion eller større risiko for at ordren bliver forældet, så det ikke altid er den bedste løsning.

## Taktisk mulighed: flankering af en allerede engageret fjende

Når en fjendtlig enhed allerede er bundet i kamp med et andet kompagni, må en kompagni-officer senere kunne vurdere, om han midlertidigt kan fortolke sin overordnede ordre og søge en bedre skudposition mod den optagede fjende.

Det må **ikke** være en automatisk regel for alle kompagnier. Beslutningen skal komme fra kaptajnens egne stats og erfaring.

### Dygtig kaptajn

En officer med høj **Tactical Skill**, **Initiative**, **Experience** og **Composure** bør oftere kunne:

- opdage at en fjende er bundet af en anden venlig enhed;
- se om fjenden vender ryggen eller siden til;
- bruge en kort **Side Step** eller **Oblique** manøvre for at få fri frontage;
- komme ind på valgt CLOSE/MED/LONG skudhold uden først at dreje hele formationen mod bevægelsesretningen;
- undgå positioner hvor andre fjender kan skyde ham i ryg eller side;
- bevare den overordnede mission og vende tilbage til den, når den lokale mulighed er væk.

### Svag eller overaggressiv kaptajn

En officer med lav Tactical Skill/Experience eller lav Composure kan fejlbedømme samme situation. Høj Aggressiveness og høj Initiative kan få ham til at forsøge manøvren alligevel, selv om:

- hans egen flanke bliver eksponeret;
- en anden fjendtlig enhed har fri cone mod den nye position;
- han bevæger sig for langt fra Majorens hensigt;
- den bundne fjende ikke er så optaget, som han tror.

Det skal være en **risikovurderingsfejl**, ikke en fast regel om at dårlige officerer altid gør noget dumt.

### Foreslået beslutningsmodel

En senere `TacticalOpportunityScore` kan blande:

- Tactical Skill: kvalitet af positionsvalg og trusselsvurdering;
- Initiative: sandsynlighed for selv at handle;
- Experience: reducerer decision noise og fejlvurdering;
- Composure: kvalitet under ild/kaos;
- Discipline: vægt mod at blive i den overordnede mission;
- Aggressiveness: øger villighed til at acceptere risiko;
- Morale/Cohesion: hvor realistisk manøvren er lige nu;
- synlige fjendtlige fire cones: risiko mod front/flanke/ryg;
- afstand til egen Major/HQ og parent objective.

En god officer bør derfor have en høj sandsynlighed for at tage **gode** muligheder og en lav sandsynlighed for at tage åbenlyst dårlige muligheder. En dårlig/overaggressiv officer kan både overse gode muligheder og acceptere farlige.

### Authority

Denne opportunistiske manøvre er lokal kaptajnsmyndighed og må ikke afbryde højere prioriterede states:

1. rout / tvungen tilbagetrækning;
2. charge / melee;
3. square / cavalry emergency;
4. bridge-only crossing;
5. direkte under-fire emergency reaction;
6. derefter tactical opportunity;
7. ellers parent Major/regimental mission.

F29P leverer **Side Step-bevægelsen og kort/kamera-synligheden**, men den fulde stat-baserede opportunistiske AI skal aktiveres i en senere revision, så den kan bygges oven på en stabil fysisk manøvre.

## Vigtig designregel

> En dygtig officer skal ikke nødvendigvis være den mest lydige officer. En højtkvalificeret, initiativrig officer kan tilsidesætte detaljer i en ordre og stadig skabe et bedre resultat; en aggressiv eller dårlig officer kan gøre det samme og skabe en katastrofe.

## Implementeringsrækkefølge

Denne feature kommer **senere**. Før den fulde personlighedsmodel aktiveres skal følgende være stabile:

1. pathfinding / destination routing;
2. ranged lethality;
3. melee baseline;
4. fysisk HQ/Major-enhed;
5. Major styrer flere kompagnier stabilt;
6. Side Step / Oblique og lokal contact authority;
7. først derefter fuld personlighed, ordrelydighed og selvstændig opportunistisk fortolkning.
