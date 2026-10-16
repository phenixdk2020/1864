# Preussen – landepakke 1825 og 1851

Leverancen er et dokumenteret forsknings- og importgrundlag til niveau B. Den er **ikke importklar eller slagklar**. Ukendte felter er null med begrundelse. Scenariestart er 1. juli 1825 og 1. juli 1851; 1864 er reference.

| Provins/rapportgruppe | 1825, skøn | 1851, skøn |
|---|---:|---:|
| Pommern | 839.188 | 1.230.951 |
| Brandenburg | 1.476.142 | 2.171.787 |
| Schlesien | 2.294.309 | 3.122.591 |
| Sachsen | 1.356.446 | 1.804.886 |
| Westfalen | 1.177.842 | 1.490.928 |
| Rheinland | 2.102.342 | 2.862.068 |
| Posen | 1.017.424 | 1.383.284 |
| Preussen | 1.891.613 | 2.554.408 |
| Hohenzollern | ukendt | 61.372 |

Alle tal i tabellen har kilder og metode i JSON under provinces. Nationalt halvårsskøn: 12.144.210 og 16.718.740 personer, fra HGIS-nationalserien. Provincial summer afviger med henholdsvis +11.096 og −36.465 personer (beregnet afstemning i Research/Afstemning.json). Ingen rest fordeles kunstigt. Berlin er allerede inkluderet i Brandenburg. Hohenzollern er uden for Preussen i 1825.

Regenter er Friedrich Wilhelm III i 1825 og Friedrich Wilhelm IV i 1851. Den senere konge Wilhelm I er prins ved 1851-start. Ministerpræsident Otto von Manteuffel og krigsminister August von Stockhausen er identificeret; hele ministerkabinettet mangler.

Hæren har en Garde-/korpsramme og et delvist regimentskatalog. Landwehr og Kriegsreserve beskrives adskilt fra den stående hær. Faktiske styrker er ukendte; de eksisterende kortgarnisoner og Nations-reserven er kun spilmodeller. Dreyse indføres gradvist, og Roons senere reform må ikke være aktiv fra 1851. Historiske generaler får ni neutrale spilskøn.

Økonomien har dokumenteret sølvparitet og særskilt cirkabudget, men ingen sikker scenarieskat, gæld eller startkasse. Kul, jernmalm og råjern har daterede årsserier; landbrug og flere industrier mangler. Banefilen viser kendte kildeankre og null i ukendte år; Nations bevarer det eksisterende modelanker på 2.900 km som spilvalg. Handelsfilens historiske værdier er ukendte; Zollverein behandles som toldområde.

Begivenheder dækker krigen og fredsafviklingen, Olmütz, Zollverein, Londontraktaten, reformen, Forbundseksekutionen og ultimatum. De historiske emner har kildehenvisninger; forudsætninger, sandsynligheder og effekter er spilvalg. Londontraktaten er ikke gældende ved 1851-start.

Se [Kilder.md](Kilder.md), [forskningsmetoden](Research/Metode-og-kildekritik.md), [importvejledningen](Spilimport/README.md) og [dækningsmatrixen](Daekningsmatrix.md). Tal i dette sammendrag opløses i JSON og Data-status.json; tekniske fil- og postoptællinger er ikke historiske data.

## Slutkontrol af Landeskabelon 4.2

| Nr. | Kontrolpunkt tilpasset Preussen | Status | Resultat og resthul |
|---:|---|---|---|
| 1 | Kort og terræn | missing | Ingen nye ejer-/district-rastre, højder, skov, vand eller grænsepolygoner. Eksisterende kortreferencer bevares. |
| 2 | Provinser, Bezirke og bybefolkning | partly | Alle ønskede provinsgrupper og 27 distrikts-/forløberposter samt Berlin. By-/landsplit og kildeverificerede bytal i kortudsnittet mangler; summer afviger. |
| 3 | Veje, færger, broer og baner | partly | Eksisterende kortforbindelser bevaret. Baneregister med kendte ankre og eksplicitte null-år; broer, færger og komplet dateret banenetsgeometri mangler. |
| 4 | Hær og fæstninger | partly | Garde og otte korps, udsnit af historiske regimenter samt arvede kortgarnisoner. Fuld OOB, faktisk fremmøde, chefer og historisk fæstningsbestykning mangler. |
| 5 | Udrustning og magasiner | partly | Dreyse og periodeforskel beskrevet. Udrustningsfordeling, kalibre, lagre, heste, ladetid og træfsikkerhed mangler. |
| 6 | Flåde | missing | Ingen dateret preussisk skibsliste, besætninger, bestykning eller operationsmodel. Reichsflotte må ikke overføres til Preussen. |
| 7 | Økonomi, valuta og handel | partly | Sølvparitet, særskilt cirkabudget og kul-/jernserier. Historiske skatter, gæld, startkasse, priser, løn, kredit og handelstal mangler. |
| 8 | Styre og beslutningsregler | partly | Regenter og ministre identificeret; ingen fuld ministerliste, bevillings- eller krigsmekanisme. |
| 9 | Diplomati og begivenheder | partly | Danmark, Zollverein, Olmütz, London, reform og 1863–64 behandlet. Eventparser, internationale reaktioner og betingelser kræver motortest. |
| 10 | Officerer og navnelister | partly | Historiske personer med ni neutrale spilskøn. Flere chefer, tjenestetilgængelighed, løn og navnepuljer mangler. |
| 11 | Flag, uniformer og portrætter | missing | Ingen billedpakke eller periodekontrollerede modeller leveret. |
| 12 | Kamp, AI og forsyning | missing | Ingen preussisk doktrin, mobiliseringskalender, forsyningsmodel eller kamp-/slagtest. Eksisterende AI-vægte er spilvalg. |
