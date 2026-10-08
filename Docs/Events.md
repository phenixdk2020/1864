# Kampagnens event-system (MVP E-1–E-6)

1825 læser `Data/Campaign1851/Events_1825.json`; 1851 læser `Events.json`. Der er ingen historiske event-tabeller i C++. De seks og syv oprindelige id'er er bevaret. Historiske måneder er udgangspunkt; sandsynligheder, tærskler og økonomiske følger er spilskøn, ikke verificerede historiske størrelser.

## Data og afvikling

Roden er en JSON-liste. Hvert event har `id`, `navn`, `vindue: {aar, maaned}`, `sandsynlighed` (0–1), `forudsaetninger`, `blokeringer`, `effekter`, `udeblivelse`, `avis` og `log`. De to effektfelter er objekter med numeriske værdier. `udeblivelse` er obligatorisk og skal ændre spænding, stemning eller kasse. Id'er skal være entydige inden for scenariet.

Ved første månedsafvikling fra vinduets første dag afgøres eventet én gang: forudsætninger skal være sande, og en eventuel blokering skal være falsk. Ved afvisning eller tabt sandsynlighedskast anvendes `udeblivelse`. Eventet forsøges aldrig igen. Månedsloopet kører ved månedsskifte; debugtvang kan afvikles dagligt. Begivenheder afhænger af verdens tilstand, aldrig af et tidligere event-id.

Den faktiske sandsynlighed er `clamp(p * (1 + (0.5 - p) * Deviation), 0, 1)`. Et separat `FRandomStream`-kast beregnes af seed, scenarie-id, event-id og heltallig kampagnedag. Evalueringens rækkefølge og debugforklaringer forbruger ikke en fælles tilfældighedsstrøm. Samme data, tilstand, seed, scenarie og dag giver samme udfald; spillerens ændringer af fakta kan give et andet udfald.

Gemmeversion 29 bevarer udløste og afviste id'er i `UCampaign1851SaveGame::War` som `fired|id` og `blocked|id`, samt `events-format|29`. Plan og kast rekonstrueres fra data og seed. Gamle gemninger beholder udløste id'er og får allerede passerede vinduer lukket uden nye effekter, så nye regler ikke efteropkræver historiske konsekvenser. Fremtidige events bruger det nye system. Gemninger, hvor datafiler ændres bagefter, følger de nye data for endnu uafgjorte events.

## Lukket udtrykssprog

Et udtryk er ét eller flere led adskilt af præcis ` og `. Hvert led er `[ikke ]faktum` eller `[ikke ]faktum operator tal`. Operatorerne er `<`, `<=`, `>`, `>=`, `==`, `!=`; skriv mellemrum omkring operatoren. En nøgen faktaværdi er sand, når den ikke er nul. `ikke` negerer hele det efterfølgende led. Tomme blokeringer betyder ingen blokering. Tomme forudsætninger afvises.

Eksempel: `spaending >= 45 og opinion.ejder >= 25 og ikke besat.Kiel`.

Der er ingen parenteser, `eller`, aritmetik, variabeltildeling, eventreferencer eller scripting. Ukendte fakta, operatorer og ikke-numeriske grænser afviser hele eventet ved indlæsning med en logadvarsel.

## Fakta fra eksisterende systemer

Alle fakta læses af den ene funktion `ACampaign1851Map::EventFact`; der findes ingen separat global faktatabel. Nøgler og id'er skelner mellem store og små bogstaver.

| Nøgle | Betydning |
|---|---|
| `spaending` | Spænding 0–100 |
| `stemning` | Folkestemning 0–100 |
| `opinion.helstat`, `opinion.ejder`, `opinion.skandinavisk` | Den eksisterende opinionsstrøms støtte i procent |
| `forhold.<nation>` | Relation −100–100; eksisterende nations-id, fx PR, GB eller RU |
| `forskning.<id>` | 1 når eksisterende forsknings-id er kendt, ellers 0 |
| `besat.<bynavn>` | 1 når byen har en besætter, ellers 0; fx Kiel |
| `mobiliseret` | 1 under mobilisering eller på krigsfod, ellers 0 |
| `garant.<nation>` | 1 når nationens eksisterende garanti er aktiv, ellers 0 |
| `vaerk.dannevirke`, `vaerk.dybbol`, `vaerk.fredericia` | 1 når det tilsvarende eksisterende værksprogram er påbegyndt (ProgrammeState 2), ellers 0; ikke et løfte om færdige skanser |
| `maegling` | 1 når den eksisterende fredskonference er aktiv, ellers 0 |
| `kasse` | Statskassen i rigsdaler |

## Tilladte effekter

| Nøgle | Enhed og grænse pr. event |
|---|---|
| `spaending` | Ændring i point, højst ±25. Positive ændringer bruger garantidæmpning og regeringens faktor; den endelige ændring begrænses også til ±25, tilstand 0–100. |
| `stemning` | Ændring i point, højst ±10; tilstand 0–100 |
| `forhold.<nation>` | Ændring i relationspoint, højst ±15 pr. nation; tilstand −100–100 |
| `kasse` | Ind-/udbetaling i rigsdaler, højst ±50.000; den eksisterende transaktionsfunktion anvendes |
| `gaeldrente` | Ændring i procentpoint, fx +0,5 bliver +0,005 i den interne rate; højst ±100, tilstand 0–1 |
| `forskning.<id>` | Kun værdien 1: tildel eksisterende forskning; opfind ikke nye emner |
| `forbundskorps` | Kun værdien 1: opret det eksisterende forbundskorps ved Altona før krig; bevarer eksekutionens militære følge uden et hardcodet event-id |

Ukendte effektkeys, ugyldige id'er, overskredne grænser og ugyldige tal afviser hele eventet med advarsel; ingen delvise effekter anvendes. Begge udfald valideres. Krigsudbrud ved spænding 80 er fortsat aktivt både ved events og i det eksisterende daglige krigsloop.

## Spillerens mulighed for at ændre historien

1825: `abent` kræver spænding mindst 30 og helstatsstøtte under 75 og blokeres af britisk garanti. `kiel` kræver spænding mindst 45, Ejderstøtte mindst 25 og ubesat Kiel; britisk garanti sammen med et godt preussisk forhold kan blokere. `preussen` kræver spænding mindst 60, preussisk forhold under 30 og ingen mægling; britisk/russisk garanti kan blokere. Ingen af dem kræver et andet event.

De tre simple events modellerer den **danske politiske reaktion** på julirevolutionen, stænderforsamlingerne og Christian VIII. Mobilisering gør deres forudsætning falsk. At den danske eventreaktion udebliver betyder ikke, at spilleren har forhindret kongens biologiske død eller en revolution i Frankrig.

1851: London kræver britisk forhold mindst 45 og fredsfod; november kræver helstatsstøtte under 40 og spænding mindst 45; ultimatum kræver spænding mindst 70 og ingen mægling. Øvrige events afhænger af opinion, relationer, garantier eller spænding. Spillerens diplomati, mobilisering og deres løbende virkning på opinionen kan dermed stoppe optrapningen. Udeblivelse giver konkret spændings- og kasseændring; historien tvinges ikke tilbage på sporet.

## Avis, Statsråd og debug

Hvert udfald giver to avislinjer: overskrift og forklaring med alle led, aktuelle værdier, sand/falsk, årsag og konsekvenser. Statsrådets eksisterende beslutningspost har tilsvarende overskrift (`Action`) og forklaring (`Reasons`). Sandsynlighedsafvisning angiver kastet i stedet for at opfinde en falsk forudsætning.

Diplomatiske ordrer og mobilisering/demobilisering markerer deres relevante fakta i den eksisterende gemte beslutningsnøgle. Forklaringen citerer seneste registrerede egen beslutning, som berører eventets fakta, med handling og dag. Opinionsnøgler ved nordisk diplomati og mobilisering angiver den eksisterende indirekte påvirkning gennem månedsmodellen. Det er en registreret påvirkning, ikke bevis for, at én ordre alene forklarer hele den aktuelle værdi. Hvis ingen relevant handling findes i den bevarede beslutningshistorik, står dette udtrykkeligt; spillet tilskriver ikke spilleren en opdigtet handling. Statsrådets eksisterende historikgrænse på 300 poster gælder fortsat.

- `-CampaignEvents=abent,kiel`: tving de angivne uafgjorte events ved næste daglige evaluering, også før vinduet og trods forudsætninger, blokeringer og chance. Gemte udfald gentages ikke.
- `-CampaignEventLog`: log hver månedsevaluering, inklusive ikke-aktuelle og afgjorte events, med dag, vindue, chance, kast og fakta.
- `-CampaignEventWhy=kiel`: forklar forudsætninger og blokeringer med dagens værdier i loggen dagligt; forklaringen udløser ikke eventet.

Der er udført datavalidering og statisk kildegennemgang. Unreal-build, runtime-evaluering, avisens visuelle layout og faktisk save/load skal kontrolleres senere; spillet er ikke bygget eller startet som del af denne ændring.
