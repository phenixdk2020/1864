# Spilimport – Preussen

Dette er **delvise overlays**, ikke komplette erstatninger for kampagnefiler. Ingen import eller motor-/slagtest er udført. Ukendte tal er null og skal håndteres eksplicit af en integrationsproces. Konverter aldrig null til nul automatisk.

Originalformater er læst fra C:/Projekter/1864-Campaign/Data/Campaign1851. Der er ingen formel JSON Schema-fil i dette grundlag; Skema-kontrol.json dokumenterer de faktiske JSON-eksempler og inputhash. Egne metadatafelter omfatter fieldStatus, source, level, parent og note. Et historisk navnekatalog er ikke en udfyldt hær.

| Fil | Skemakilde | Indhold og datakvalitet |
|---|---|---|
| Nations_1825.json | Nations_1825.json | PR-række; national befolkning halvårsskøn, fredsstyrke/skat ukendt; AI og reserve spil. |
| Nations1851.json | Nations1851.json | PR-række; national befolkning halvårsskøn, fredsstyrke/skat ukendt; AI og reserve spil. |
| Population_1825.json | Population_1825.json | Provinser og Bezirke; by/land null. Ingen geometrisk tildeling til danske amter. |
| Population_1851.json | Population_1825.json | Provinser og Bezirke; by/land null. Ingen geometrisk tildeling til danske amter. |
| Population_1864.json | Population_1825.json | Provinser og Bezirke; by/land null. Ingen geometrisk tildeling til danske amter. |
| cities_neighbours_1825.json | cities_neighbours_1851.json | Stabile PR-by/kreds-id’er og rækkefølge; tværstatslige ruter bevares som referencer. |
| cities_neighbours_1851.json | cities_neighbours_1851.json | Stabile PR-by/kreds-id’er og rækkefølge; tværstatslige ruter bevares som referencer. |
| Army_Neighbours_1825.json | Army_Neighbours_1851.json | Garnisoner/feltenheder i kortudsnittet, alle styrker eksplicit spil. |
| Army_1825.json | Army_1825.json | Garde/korps og sikkert identificeret enhedsudsnit; materiel, ruller og depoter null. |
| Army_Neighbours_1851.json | Army_Neighbours_1851.json | Garnisoner/feltenheder i kortudsnittet, alle styrker eksplicit spil. |
| Army1851.json | Army1851.json | Garde/korps og sikkert identificeret enhedsudsnit; materiel, ruller og depoter null. |
| Economy_1825.json | Economy_1825.json | Originale vare-/skatte-/kreditnøgler. Ukendt null; særskilt cirkabudget, dokumenteret sølvparitet. |
| Economy_1851.json | Economy_1825.json | Originale vare-/skatte-/kreditnøgler. Ukendt null; særskilt cirkabudget, dokumenteret sølvparitet. |
| Trade_1851.json | Trade_1851.json | Preussen-område med originale vægt/værdinøgler; totaler null. |
| Officers_1825.json | Officers1851.json | Historiske personer, ni stats/erfaring skøn; ingen opdigtet tilfældig navnepulje. |
| Officers1851.json | Officers1851.json | Historiske personer, ni stats/erfaring skøn; ingen opdigtet tilfældig navnepulje. |
| Events.json | Events.json | Historiske emner i originale danske eventnøgler; dato/status særskilt fra spilbetingelser/effekter. |
| Sources.json | Egen dokumentation, ikke motorinput | Kilderegister med læsestatus og lokatorer |
| Data-status.json | Egen dokumentation, ikke motorinput | Numerisk/null-proveniens via JSON-pointer for samtlige importdatafiler |
| Kildegrundlag.json | Egen dokumentation, ikke motorinput | Observationer, scenarieskøn og administrative afstemninger |
| Skema-kontrol.json | Egen dokumentation, ikke motorinput | Originale nøglelister, filstier og inputhash |
| Manifest.json | Egen dokumentation, ikke motorinput | Importfiler og SHA-256; ikke et selvhash |
| Verifikation.json | Egen dokumentation, ikke motorinput | Maskinlæselig rapport fra kontrollen |
| README.md | Egen dokumentation | Denne vejledning |

## Integration og status

Nations: upsert på PR; alle øvrige nationer bevares. Population: de nye administrative id'er er dokumentations-id'er og må ikke erstatte indeksene i det danske kort. `cities`, `foreignCities` og `amter` følger originale topnøgler; provins-/districtniveauer overlapper. Vælg ét niveau ved summering. Berlin er inkluderet i Brandenburg.

Byer: PR-filter fra basisscenariet bevarer rækkefølge, id'er og koordinater. Ruter til referencedExternalCities forudsætter de øvrige nationers byer i basisfilen. Ejer-/district-grid er referencer til eksisterende spilfiler, ikke filer leveret her. Havn, fæstning, garnison, kanoner og bytallene er arvede spilvalg, ikke historiske observationer.

Army_Neighbours indeholder kortets garnisoner og feltenheder som spilvalg. Army1851/Army_1825 er et dokumentationskatalog med ukendte ruller. De må ikke skabe dobbelte styrker. legacyAggregate er informationspost, ikke en ekstra hær. Ni generalstats er egne neutrale skøn; ingen opdigtede personer eller navnepuljer. Events london/execution/ultimatum opdaterer eksisterende id'er; øvrige PR-events er nye. Begivenheder før startdatoen skal initialisere historik. Motorens betingelsesparser er ikke testet.

Economy: alle oprindelige skattenøgler bevares; ukendte felter er null. Penge normaliseres til rigsdaler sølv med 37/28 pr. preussisk taler, en beregnet sølvparitet fra de to kildeangivne møntfødder. Ingen markeds- eller seddelkurs er dokumenteret. Cirka-budget er særskilt fra realiseret indtægt, direkte skat, kasse og gæld. Trade er årsstrømme; tradeValue i Nations er kun spilbonus. comparison1847 er ukendt for Preussen, ikke kopierede danske beløb.

`kilde` betyder afskrevet fra den angivne læste kilde/databaserække, `skøn` beregnet/afrundet eller neutral spilkarakter, `spil` eksplicit design/arvet spilmodel. Et null er altid ukendt med begrundelse. Observationer og cirkatal findes i adskilte felter/lister. Data-status.json giver hver numerisk/null-værdis kilde-id, status og årsag. Kilde-id'er opløses i Sources.json og ../Kilder.md. År, id-numre, filstørrelser og kontroltal er tekniske metadata.

## Alle resterende huller

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
