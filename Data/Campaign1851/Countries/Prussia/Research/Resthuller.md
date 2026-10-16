# Resthuller

Ukendt betyder `null`, aldrig historisk nul. Klassifikationen `skøn` ved et null er et uafklaret datafelt og indeholder intet numerisk estimat. De konkrete begrundelser står ved hvert felt i JSON og i Data-status.json.

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

Supplerende huller: landbrug/husdyr, skovproduktion, uddannelsestal, forskningsindeks, bygningsomkostninger, fæstningsværker og præcis kortudstrækning. Neuchâtel og Neutral-Moresnet kræver særskilt afgrænsningskontrol; de lægges ikke oven i nationalserien. Hohenzollern-delområderne er ikke to permanente Regierungsbezirke; 1864-reference for deres efterfølger mangler. Provinsernes og Bezirkenes tal er ikke et fælles afstemt befolkningsregnskab.

1825-hæren er et arvet kortudsnit og en dokumentationsramme, ikke en verificeret fuld orden de bataille. 1851 mangler især linjekavaleri, provinsartilleri, pionerer, forsyningstropper, Landwehr-regimenter og Kriegsreserve fordelt på årgange. Ingen præcis 1864-OOB leveres.
