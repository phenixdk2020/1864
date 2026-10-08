# Regering, opinion, stormagterne og fredsvilkår (backlog 7, 8, 9 og 11)

Alt vises i **STATSRÅD** (regering og opinion) og i **UDENRIGS** (fred).

## Opinionen
**Tre strømninger** deler 100 %:

| Strømning | Start (± afvigelse) | Hver måned |
|---|---|---|
| **Helstaten** | 45 % | Rest |
| **Ejderpolitikken** | 40 % | Vokser med (spænding − 40) / 40 |
| **Skandinavismen** | Rest | Vokser med (Sveriges forhold − 40) / 60, +1 med alliancen |

**Stemningen i landet** (0–100) ændres sådan:

| Hændelse | Stemning | Ejderpolitikken |
|---|---|---|
| Hver måned | Mod 60 | |
| Mobilisering, pr. måned | −1,5 | |
| Krig, pr. måned | −0,5 | |
| Pr. besat by, pr. måned | −1 | |
| Sejr | +4 | +2 |
| Nederlag | −5 | −3 |
| Fred med afståelser | −1,5 pr. by | −6 |
| Fred på status quo | +10 | +3 |

**Stemningen virker på:**
- **skatterne:** ganges med 0,9–1,1;
- **indkaldelsen:** ganges med 0,8–1,2.

## Regeringen
- **De historiske ministerier** tiltræder på deres datoer, hvis deres strømning har mindst 30 %:
  - Bluhme 1852, Ørsted 1853, Bang 1854, Andræ 1856, Hall 1857, Rotwitt 1859, Hall 1860, Monrad 1863.
  - Strømningerne er forenklet til helstat eller Ejder.
- **Regeringsfald:** en regering falder efter mindst et halvt år, hvis dens strømning er under 28 % eller stemningen under 25. Den førende strømning danner så regering.
- **Regeringens linje:**
  - **Helstaten:** begivenhederne skærper spændingen 15 % mindre. Storbritannien og Rusland +0,5 om måneden. Krigsministeriets vægt × 0,9.
  - **Ejderpolitikken:** begivenhederne skærper spændingen 20 % mere. Sverige-Norge +0,5 om måneden. Krigsministeriets vægt × 1,15.
  - **Skandinavismen:** Sverige-Norge +1 om måneden, og alliancen koster det halve.

## Stormagterne (backlog 8)
- **Ejderpolitik og høj spænding:** med en Ejder-regering og spænding over 60 mister garantimagterne 2 i forhold om måneden (Rusland 3).
- **Tilbagetrukket garanti:** en garanti trækkes tilbage, når forholdet kommer under 20.
- **Mægling:** i krig tilbyder en garantimagt med forhold på mindst 60 mægling efter 30 dage, og en fredskonference samles.

## Fredsvilkår (backlog 9)
**Krigsstillingen** går fra −1 til 1:
- (fjendens tab − danske tab) / max(10.000, summen af tabene);
- − 0,04 pr. besat by;
- + 0,1 ved blokade, når flåden behersker farvandet.

**Tilbuddene:**

| Tilbud | Afstår | Fjenden siger ja, når |
|---|---|---|
| Status quo | Intet | Krigsstillingen er mindst 0,25 |
| Deling efter sprog | Holsten, Lauenborg og Slesvig syd for ca. 54,75° N (Flensborg fjord) | Der er konference, eller krigsstillingen er mindst −0,2 |
| De besatte byer | Det besatte | Krigsstillingen er mindst −0,6 |
| Hertugdømmerne | Slesvig, Holsten og Lauenborg | Altid |

- Afståede byer forlader monarkiet, og deres amter betaler ikke længere.
- Andre besatte byer kommer tilbage.
- Fangerne udveksles.

## Gemning (v25)
`Politics`-linjen: `state|helstat|ejder|skand|stemning|statsminister|linje|siden|næste ministerium|danske tab|fjendens tab`.

## Scenarieaudit 2026-10-08
- 1851 beholder startopinion, ministre, regeringsnavne, budgetter og flådemål.
- 1825 starter med anslået opinion 80 % helstat, 10 % Slesvig/Ejder og resten nordisk orientering (samme seedafvigelse som før). Det kongelige statsråd og fiktive rådgivertitler bruges før 1848; strømningerne er en abstraktion af politiske tendenser under enevælden, forklaret i Statsrådets hjælpetekst. Historiske kandidater bliver tilgængelige fra 1848, men siddende rådgivere beholdes efter de normale regler.
- Startbudgetter/puljer skaleres med scenariets befolkningsfaktor (1825: 0,8). Intendanturens tøj/lædermål skaleres med hærfaktoren (1825: 0,55). Fredelig flådeplanlægning i 1825 regner uden en østrigsk eskadre og uden 26 års vækst allerede i 1851; krigens mål er uændret.
- Ny valgfri Politics-linje: `warweight|vægt`. Den gemmer den aktuelle militære prioritet; ældre gemninger rekonstruerer regeringens faktor efter et regeringsskifte. Manglende ministerposter rekonstrueres fra den indlæste regerings strømning. Ministerevner, budgetter, puljer og næste ministerium begrænses ved indlæsning.
- Råvareforslag bruger nu samme oprundede mængde til pris, budgetkontrol og bestilling.

### Uafklaret efter audit
- Startopinion og rådgivermodellen for 1825 er spillets skøn; historiske rådgivere, overgang til 1848-ministerier og politiske strømningers åbningstid kræver et særskilt design.
- Månedlig opinion normaliserer alle tre strømninger, selv om tabellen ovenfor siger, at helstaten er resten. Besatte byer koster aktuelt 0,5 stemning pr. måned (højst fem), mod dokumentets 1 pr. by. Begge dele beholdes for at bevare 1851-balancen.
- Flådens startliste indeholder skibe bygget efter 1825, og fjendens søstyrke i krig tæller år fra scenariestart. Det kræver en separat flådeaudit; denne ændring retter kun ministerens fredelige mål.
- Finansministerens faste låne-/afdragsbeløb og doktrinrådets generelle omtale af tændnålsgeværet er stadig 1851-prægede. Den langsigtede 1825-balance samt budgetpuljernes evne til at betale store skibe og lån kræver spiltest.
- Nationernes øvrige, manuelt ændrede prioriteter gemmes ikke i SaveWorld. Denne audit gemmer kun den militære vægt, som regeringsskiftet ændrer.
- Gamle 1825-gemninger med historiske minister-/regeringsnavne beholder disse; de erstattes ikke automatisk.
