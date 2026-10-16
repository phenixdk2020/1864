# Arbejdsstatus – Preussen

Dato: 15. oktober 2026. Forsknings- og importgrundlag leveret i denne mappe. Ikke importeret i spilprojektet; readyForImport=false og readyForBattle=false. De ønskede filtyper er leveret, men den historiske dækning er delvis.

Eksisterende udkast er færdiggjort med dansk dokumentation, kildeindeks, tydelige resthuller og reproducerbar validering. Landeskabelonen, Sverige-Norges metode og de angivne spilskemaer er læst. Kildearkivets læsestatus er bevaret; ikke alle tidligere webafskrifter er genverificeret som sidebilleder.

Næste trin er at læse den faktiske Rangliste og regimentsruller, afskrive byfolketællinger og finans-/handelstabeller, afstemme geografier og derefter kontrollere motorens håndtering af null, id'er og metadata. Ingen softwaretest kan gøre ukendte historiske felter kendte.

Blokerede/utilstrækkelige kilder: Rangliste kun katalog, Hallische Zeitung botkontrol, Soldatenfreund modstridende OCR, budgetartikel med usikker talafstemning. Detaljer står i Kilder.md og Research/Resthuller.md.

Kun filer i denne mappe er skrevet. Spilprojekt og Sverige-Norge er kun læst; ingen git-kommandoer kørt der. Hashkontrol af læste input dokumenteres i verifikationsloggen.

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
