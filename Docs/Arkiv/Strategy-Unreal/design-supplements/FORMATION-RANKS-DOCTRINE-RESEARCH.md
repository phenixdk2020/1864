# PROJECT 1864 — Design-supplement: formationsdybde som doktrin/research

**Status:** Godkendt designidé  
**Gælder:** Taktisk infanteri, militær doktrin, drill og research/adoption  
**Baseline:** 1851-start

## Formål

Antallet af geledder i en infanterilinje skal ikke være en universel hardcoded værdi for hele spillets tidsperiode. Formationsdybde behandles som en del af nationens militære reglement, drill og doktrin og kan derfor ændres gennem historisk udvikling, reformer og research/adoption.

## 1851-baseline

For dansk linjeinfanteri anvendes **3 geledder som standard i 1851**. Et testkompagni på ca. 190 mand vil derfor ved fuld styrke fordele sig omtrent som **64 + 63 + 63 mand** i tre geledder.

To-geledders linje skal ikke være standard for den danske 1851-start. Historisk indføres en dansk to-geledders regulær opstilling med det senere reglement, og dette gør overgangen velegnet som en konkret doktrin-/reformudvikling i spillet.

## Research / doktrin

Eksempel på research-/reformnode:

### To-geledders linjetaktik

**Kategori:** Militær doktrin / Infantry Drill / Regulations  
**Forudsætninger:** passende drill-niveau, officers-/NCO-træning og national adoption  
**Resultat:** giver adgang til 2-geledders linjeformation for relevante infanterienheder.

Research alene bør ikke nødvendigvis ændre hele hæren øjeblikkeligt. Systemet skal skelne mellem:

1. **Viden/research** — doktrinen er kendt.
2. **Reglement/reform** — staten/hæren beslutter at indføre den.
3. **Træning/adoption** — officerer, underofficerer og soldater skal lære den.
4. **Enhedsstatus** — enkelte regimenter/kompagnier kan fortsat bruge ældre drill, indtil de er omskolet.

Dette følger designmanualens generelle princip om, at forskning, beslutning og faktisk adoption ikke er det samme.

## Spilmæssige forskelle

Formationsdybde skal have reelle taktiske konsekvenser og må ikke blot være kosmetik.

| Formation | Fordele | Ulemper |
| --- | --- | --- |
| **3 geledder** | Mere kompakt frontage; lettere command/control; kan være mere robust under bevægelse og dårlig træning | Færre mænd i første ildlinje; større dybde; mindre frontage pr. mand |
| **2 geledder** | Større frontage; flere geværer kan bringes effektivt i ild; bedre udnyttelse af forbedrede håndvåben | Kræver større plads; sværere at holde samlet; mere følsom over for terræn, command delay og dårlig drill |
| **1 geled / skyttekæde** | Maksimal spredning og terrænudnyttelse; mindre tæt mål | Lavere cohesion; vanskeligere kontrol; ikke egnet som normal tæt linjeformation |

De præcise modifiers skal balanceres gennem test og historisk research. Systemet bør primært påvirke:

- frontage og fysisk footprint
- antal mænd med fri skudsektor
- fire eligibility / firing fraction
- cohesion og reformeringstid
- sårbarhed over for artilleri og tæt geværild
- command/control og ordre-delay
- evne til at bevæge sig gennem ujævnt terræn
- tid til at skifte mellem kolonne og linje

## Nationer og tidsperiode

Formationsregler skal være **nation- og tidsafhængige data**, ikke én global regel. Danmark, Preussen, Sverige og andre nationer kan derfor have forskellige startdoktriner, reformdatoer, træningskrav og muligheder.

Historisk scenario-start skal som udgangspunkt bruge den formation, som nationens relevante reglement og enhedstype faktisk anvendte på tidspunktet. Sandbox-/grand-strategy-spilleren skal kunne fremskynde, forsinke eller undlade reformer, hvis de nødvendige institutionelle forudsætninger er til stede.

## Enhedstype

Standard formationsdybde kan også variere efter enhedstype:

- linjeinfanteri
- let infanteri / jægere
- skirmishers
- garde-/eliteenheder
- milits/reserve

Det betyder, at eksempelvis lette tropper kan have adgang til mere spredte eller to-geledders formationer før almindeligt linjeinfanteri.

## UI

Når flere formationsdoktriner er låst op, skal den taktiske formationsmenu vise tilgængelige valg, fx:

- **Linje — 3 geledder**
- **Linje — 2 geledder**
- **Kolonne**
- **Skyttekæde** (hvis enhed/doktrin tillader det)

Hover/tooltips skal vise formationens forventede frontage, dybde, reformeringstid og vigtigste taktiske konsekvenser.

## PROJECT 1864 implementeringsregel

For den aktuelle 1851-prototype anvendes **3 geledder som dansk standardlinje**. Den nuværende v00.00.09f5-test skal derfor bruge tre geledder for det danske 190-mands kompagni.

Senere implementeres formationsdybde som datadrevet egenskab, så `LineRanks` ikke forbliver hardcoded. Research-/adoptionssystemet skal kunne ændre de formationer, en enhed må vælge, uden at ændre dens faktiske mandskabstal.
