# Nationer, byer og økonomi i 1825

Dato: 8. oktober 2026. Afgrænsning: befolkning, nationale fredsstyrker og relationer, handel, skatter, priser og byggeøkonomi. Danske regimenter, politik og militære lagre tilhører andre delopgaver.

## Kilder og sikkerhed

Der findes ingen landsdækkende dansk folketælling fra 1825. Dataene er selvstændige, historisk informerede skøn; de må ikke læses som præcise tællingsresultater. Ingen by eller noget amt beregnes ved at gange 1851-befolkningen med en fælles faktor.

- [Danmarks Statistik: Folketælling 1801 og 1834](https://www.dst.dk/da/Statistik/udgivelser/VisPub?pid=1633): tællingerne før og efter scenarieåret, med købstæder og landdistrikter. Suppleret med [Danske byers folketal 1801–1981, tabel I, s. 13–14](https://www.dst.dk/Site/Dst/Udgivelser/GetPubFile.aspx?id=19910&sid=byersfolk1801): de 40 kortbyer i kongeriget har aflæste 1801-/1834-ankre i kilde-JSON og lineær interpolation, afrundet til 100. PDF-skærmbilledet kunne ikke hentes; værdierne er aflæst fra dokumentets tekstlag.
- [Rigsarkivets vejledning](https://www.rigsarkivet.dk/vejledning/folketaellinger-kom-godt-i-gang/): geografisk inddeling og adgang til de historiske tællinger.
- [Danmarks befolkningsudvikling](https://danmarkshistorien.lex.dk/Danmarks_befolkningsudvikling_1769-2021): kongeriget ca. 929.000 i 1801 og 1.231.000 i 1834. Lineær interpolation giver ca. 1.149.000 i 1825; kortets skønnede kongerigstal ligger nær dette, men dækker kun dets eksisterende grupperinger.
- [Københavns statistiske årbog, historiske folketal](https://www.kk.dk/sites/default/files/2022-02/1989%20%C3%A5rbog.pdf): 100.975 i 1801 og 119.292 i 1834. Interpolation til 1825 giver 114.296, afrundet til 114.300. Dette er et beregnet skøn, ikke en observeret 1825-tælling.
- [Aarhus Universitet: Landbruget 1814–1840](https://cas.au.dk/danmarkshistorien/lektioner/lektion-6-fra-enevaeldig-helstat-til-nationalstat-1814-1914/4-modernisering-internationalisering-og-urbanisering) og [forskningsartikel om landbrugskrisen 1818–1828](https://tidsskrift.dk/labo/article/view/26129): faldende kornpriser og økonomisk krise. Kilderne begrunder retningen, ikke spillets konkrete prisindeks.
- [Helstaten](https://lex.dk/Helstaten) og [Thorvaldsens Museum: bevægelsen 1815–1848](https://arkivet.thorvaldsensmuseum.dk/artikler/den-slesvig-holstenske-bevaegelse-1815-1848): Holsten og Lauenborg i Det Tyske Forbund, mens Slesvig står udenfor. National konflikt i 1848 må ikke overføres direkte til 1825.
- [Cambridge History of Russia, bind 2, afsnittet om kejserhæren](https://portal.tpu.ru/SHARED/d/DVR/eng/teaching/lieven_d_edit_the_cambridge_history_of_russia_volume_2_i.pdf): stående russisk hær på ca. 750.000 i 1825. Opgørelsens afgrænsning er ikke nødvendigvis identisk med andre landes fredsstyrker.
- [Preußische Armee, historisk styrketabel](https://de.wikipedia.org/wiki/Preu%C3%9Fische_Armee): 130.000 i 1825; sekundær kilde, bør efterprøves mod samtidige budgetter.
- [Svenske historiske befolkningstal](https://commons.wikimedia.org/wiki/Data%3ASweden_-_yearly_population_and_population_changes.tab), [Norges befolkningshistorie](https://en.wikipedia.org/wiki/Demographics_of_Norway) og [Frankrigs befolkningstal](https://en.wikipedia.org/wiki/Demographics_of_France): støtte for størrelsesordener. Unionens 3,82 mio. er afrundet; øvrige nationale folketal nedenfor er periode-skøn, ikke verificerede 1825-tællinger.
- [Historic England: Stockton–Darlington](https://historicengland.org.uk/services-skills/education/teaching-activities/stockton-darlington-railway-local-area-and-peoples-lives/): åbning 27. september 1825. Ingen offentlig dampbane ved kampagnens julistart; tidligere britiske industri- og hestebaner udelades af denne netmodel.

## Befolkning og geografiske antagelser

Alle 55 danske/helstatslige kortbyer, 18 udenlandske byer og 41 amtsgrupper har særskilte værdier i `Tools/Map1851/cities_1825.json` og `amter_1825.json`. Tabellen sidst i dette dokument oplister hvert skøn. De 40 kongerigsbyer er interpoleret som København: P1825 = P1801 + (P1834 − P1801) × 24/33, afrundet til 100. Det er en antagelse om jævn vækst mellem to observerede tællinger. De øvrige byer og samtlige amtsfordelinger er plausible, afrundede anslag ud fra førindustriel bystørrelse, havnehandel og samlet befolkning nær periodens niveau. De er ikke afskrifter af enkelttabeller. Tabel I indeholder også nogle nordslesvigske serier, men dens forskudte tællingsår kræver yderligere kontrol; disse byer beholdes som skøn. Især udenlandske byer og hertugdømmernes amter har stor usikkerhed.

Amtsgrænser, id'er, bytilknytninger og de samlede geografiske grupper fra kortet bevares. Det gælder også Skanderborg-grupperingen, som ikke er et selvstændigt historisk amt i 1825. Kortets købstadsudvalg er ufuldstændigt; `urban` betyder de modellerede byers sum, og andre mindre byer ligger i restbefolkningen `rural`. Ærø og Femern får egne ø-skøn frem for de meget lave 1851-modeltal. Dette er fortsat et kampagnekort, ikke et rekonstrueret administrativt atlas.

## Nationer og styrke

| Nation | Befolkning | Hær, mand | Relation til Danmark |
|---|---:|---:|---:|
| Sverige-Norge | 3.820.000 | 42.000 | 5 |
| Preussen | 12.200.000 | 130.000 | 10 |
| Østrig | 33.000.000 | 280.000 | 15 |
| Storbritannien inklusive Irland | 22.800.000 | 110.000 | 10 |
| Frankrig | 31.300.000 | 200.000 | 5 |
| Rusland | 54.000.000 | 750.000 | 30 |
| Hannover | 1.550.000 | 16.000 | 10 |
| Mecklenburg-Schwerin | 450.000 | 3.500 | 10 |
| Oldenburg | 220.000 | 2.000 | 15 |
| Braunschweig | 240.000 | 2.200 | 5 |
| Nederlandene inklusive det senere Belgien | 6.100.000 | 40.000 | 15 |
| Hamborg med opland | 130.000 | 1.300 | 25 |
| Lübeck med opland | 40.000 | 400 | 20 |
| Bremen med opland | 55.000 | 500 | 20 |

Ud over de særskilt kildebelagte russiske og preussiske styrker er hærtallene afrundede fredsstyrke-skøn. De omfatter ingen flåde, kolonial befolkning eller generel mobiliseringsreserve. Sverige-Norge samler svensk indelt/fast styrke og et norsk kontingent; begrebet er derfor ikke strengt sammenligneligt med permanent kaserneret mandskab. Hærstyrken er modellens styrkemål, ikke en påstand om ens kampkraft eller tropper klar til øjeblikkelig udrykning.

Relationer på −100 til 100, handelsindtægter, reservekasser, skatter, vækstrater og AI-prioriteter er spilantagelser. Rusland får et relativt godt forhold gennem konservativ Østersøpolitik; Sverige-Norge er køligere efter 1814. Stormagternes mulighed for en garanti er en hypotetisk spillerhandling, ikke London-protokollen fra 1852. Sverige-Norge kan indgå en ny alliance; den findes ikke automatisk.

Norge og Belgien er inaktive nulrækker for at undgå dobbelttælling. Oprørshæren er ligeledes inaktiv. Det Tyske Forbund er en inaktiv politisk oversigtsrække uden eget folketal, hær eller budget: de allerede modellerede medlemsstater må ikke finansieres eller mobiliseres en gang til. Forbundet er ikke en fuldstændig model af alle dets medlemsstater. Danmarks tal kommer fra kortet og de danske regimenter; nationens nulværdier erstatter ikke disse.

## Økonomi, priser og løn

`Economy_1825.json` bruger rigsdaler sølv som fælles spilregneenhed. Papirpengekurser, rigsbankdaler og lokal valuta rekonstrueres ikke. Tallene er forsigtige spilskøn uden verificerede løn- eller prislister fra 1825.

| Parameter | 1825 | Betydning |
|---|---:|---|
| Land-/byskat pr. hoved og år | 0,16 / 0,36 rd. | Statens udviklingsandel, ikke samlet historisk skattetryk |
| Korn/kvæg/smør pr. landbo og år | 0,38 / 0,24 / 0,07 rd. | Udførselsgrundlag før prisindeks og høst |
| Prisindeks korn/kvæg/smør til og med 1828 | 0,72 / 0,90 / 0,85 | Landbrugskrisens anslåede prisniveau mod 1851-basis |
| Udførselstold | 2 % | Bevaret modelafgift; ingen dokumenteret ensartet historisk sats |
| Grundrente | 5 % | Kredit efter statsbankerotten; øvrige risikotillæg og loft bevares |
| Ufaglært dagløn, reference | 0,25 rd. = 24 skilling | Dokumenteret som antagelse, ikke en selvstændig lønudbetaling |
| Byggekontrakt, pris/løn | 0,90 af eksisterende pris | Antaget 40 % uændrede materialer og 60 % arbejdskraft med 0,25 mod en antaget 1851-løn på 0,30 rd. |
| Jern/kul pr. ton | 45 / 16 rd. | Dyrere import og transport før jernbaner |
| Tømmer pr. læs | 5 rd. | Lokal råvare, skøn |
| Krudt pr. tønde | 28 rd. | Skøn |
| Klæde pr. uniform/læder pr. sæt | 5 / 3,5 rd. | Skøn; modellens enheder, ikke almindelige detailpriser |

Byggekontraktens samlede pris bruges både i kortet og til den eksisterende daglige betaling af restprisen efter udbetaling. Dermed er pris, lønposter og vist restbeløb konsistente; systemet tæller ikke arbejdere eller betaler 0,25 rd. individuelt. Faktoren gælder bygningsmoduler; vej-, skanse- og militærkontrakter er uden for denne delopgave. Civilvirksomheders job, gebyrer og handel har individuelle mindre 1825-tabeller i `Campaign1851Nations.cpp`, ikke længere en fælles befolkningsskalering. Disse virksomhedsvirkninger er balanceanslag.

Kun de tre eksisterende udførselsvarer er aktive: korn, kvæg og smør. Smør er en beskeden førmejeriel udførsel, ikke den senere andelsmejerisektor. Træ, klæde og læder samt importeret jern, kul og krudt bruger de eksisterende råvaretyper. Told, Øresundsindtægt og troppeudgifter uden for ovenstående ændres ikke. Efter 1828 vender prisfaktoren tilbage til 1; udførselsgrundlaget forbliver scenariets. Dette grove konjunkturforløb erstatter ikke en år-for-år prisserie.

## Jernbaner, indlæsning og gemning

1825 bruger fælles terræn/geometri fra `Denmark1851_Map.json` og et komplet `Population_1825.json`-overlay valgt ved `ActiveScenario().Id`. Manglende befolkningsoverlay afviser kortindlæsningen; der faldes ikke tilbage til 80 % af 1851. Nationer kommer fra `Nations_1825.json`, og økonomisatser fra `Economy_1825.json`. Ugyldig/manglende økonomifil giver fejl i loggen og eksisterende standardsatser som teknisk nødværdi.

`python Tools/Map1851/export_1825.py` regenererer runtime-overlayet. `--out=...` kan skrive til en Resources-mappe; den eksisterende `build_map.py` kalder også eksportøren for sit output. Ingen terrænfiler skal duplikeres eller bygges for at opdatere befolkningstal.

1825 indlæser ingen historiske jernbaner, heller ikke planlagte 1851-linjer. Geometriske forslag til spillerbyggede baner bevares og kræver den eksisterende forskning. Abstrakte nationer begynder med 0 km; britisk netvækst tillades fra 1826, øvrige fra 1835. Dette er en grov mulighedsgrænse, ikke faktiske nationale åbningsdatoer. Senere historiske danske baner dukker ikke automatisk op: de må anlægges gennem spillet. Chausséer og færger genbruges fra kortet og er ikke nyundersøgte for 1825.

1851-filer og scenariets satser bevares. PopulationFactor består som legacy-parameter for andre delsystemer, men bruges ikke længere til byer/amter. Hær- og erfaringsskalering ændres ikke i denne delopgave.

Gemmeformatet er uændret: by-/amtsrækkefølge og alle eksisterende nations-id'er og indeks bevares; nye nationer tilføjes sidst. Nationers dynamiske værdier gendannes via id; befolkning og økonomihistorik via de eksisterende felter. Scenarieindekset bliver fortsat gemt og valgt ved kampagnens eksisterende genåbning. Gamle 1825-gemningers faktiske befolkning og hære bevares ved gendannelse; nye 1825-data er grundlag for nye spil, ikke en tvungen migration af gemte værdier.

Kontrol: JSON-dækning, unikke nøgler, positive folketal, `urban + rural = population`, bysum pr. amt, eksportens reproducerbarhed, statisk kontrol af scenariegrene og `git diff --check`. Ingen Unreal-build eller spilkørsel; faktisk runtime-gemning og UI kræver senere afprøvning.

## Alle befolkningsskøn

| By | Befolkning, skøn |
|---|---:|
| København | 114.300 |
| Helsingør | 6.600 |
| Roskilde | 2.500 |
| Køge | 1.800 |
| Holbæk | 1.700 |
| Kalundborg | 1.900 |
| Slagelse | 2.600 |
| Næstved | 2.100 |
| Vordingborg | 1.300 |
| Stege | 1.400 |
| Nykøbing F | 1.500 |
| Nakskov | 2.100 |
| Odense | 7.900 |
| Nyborg | 2.600 |
| Svendborg | 3.000 |
| Faaborg | 1.800 |
| Assens | 2.100 |
| Middelfart | 1.300 |
| Bogense | 1.100 |
| Rudkøbing | 1.500 |
| Aalborg | 6.600 |
| Aarhus | 6.000 |
| Randers | 5.900 |
| Horsens | 4.200 |
| Fredericia | 4.000 |
| Vejle | 2.100 |
| Kolding | 2.200 |
| Viborg | 3.100 |
| Skanderborg | 700 |
| Grenaa | 900 |
| Hjørring | 1.200 |
| Frederikshavn | 1.000 |
| Skagen | 1.000 |
| Thisted | 1.500 |
| Nykøbing M | 1.000 |
| Holstebro | 1.100 |
| Ringkøbing | 1.100 |
| Ribe | 2.300 |
| Varde | 1.300 |
| Rønne | 3.500 |
| Haderslev | 4.200 |
| Aabenraa | 3.500 |
| Sønderborg | 2.300 |
| Tønder | 2.600 |
| Flensborg | 14.500 |
| Slesvig | 9.000 |
| Husum | 3.500 |
| Egernførde | 3.200 |
| Kiel | 8.500 |
| Rendsborg | 7.500 |
| Itzehoe | 4.500 |
| Glückstadt | 5.700 |
| Altona | 25.500 |
| Neumünster | 2.200 |
| Ratzeburg | 2.200 |
| Hamborg | 110.000 |
| Lübeck | 23.500 |
| Malmø | 7.500 |
| Helsingborg | 2.500 |
| Landskrona | 2.800 |
| Lund | 4.300 |
| Ystad | 2.800 |
| Trelleborg | 500 |
| Kristianstad | 4.300 |
| Karlskrona | 10.500 |
| Halmstad | 2.200 |
| Göteborg | 21.000 |
| Rostock | 18.000 |
| Wismar | 8.500 |
| Schwerin | 12.000 |
| Stralsund | 16.000 |
| Stettin | 27.000 |
| Bremen | 40.000 |

| Kortets amtsgruppe | Byer | Restbefolkning | I alt, skøn |
|---|---:|---:|---:|
| Hjørring Amt | 3.200 | 58.000 | 61.200 |
| Thisted Amt | 2.500 | 44.500 | 47.000 |
| Aalborg Amt | 6.600 | 67.000 | 73.600 |
| Viborg Amt | 3.100 | 60.000 | 63.100 |
| Randers Amt | 6.800 | 77.000 | 83.800 |
| Aarhus Amt | 6.000 | 23.000 | 29.000 |
| Skanderborg Amt | 700 | 38.000 | 38.700 |
| Vejle Amt | 12.500 | 58.000 | 70.500 |
| Ringkøbing Amt | 2.200 | 102.000 | 104.200 |
| Ribe Amt | 3.600 | 59.000 | 62.600 |
| Odense Amt | 12.400 | 46.000 | 58.400 |
| Svendborg Amt | 8.900 | 39.000 | 47.900 |
| Københavns Amt | 118.600 | 40.000 | 158.600 |
| Frederiksborg Amt | 6.600 | 32.000 | 38.600 |
| Holbæk Amt | 3.600 | 39.000 | 42.600 |
| Sorø Amt | 2.600 | 39.000 | 41.600 |
| Præstø Amt | 4.800 | 44.000 | 48.800 |
| Maribo Amt | 3.600 | 42.000 | 45.600 |
| Bornholms Amt | 3.500 | 18.000 | 21.500 |
| Haderslev Amt | 4.200 | 45.500 | 49.700 |
| Aabenraa Amt | 3.500 | 19.000 | 22.500 |
| Sønderborg Amt | 2.300 | 11.500 | 13.800 |
| Tønder Amt | 2.600 | 40.000 | 42.600 |
| Flensborg Amt | 14.500 | 31.000 | 45.500 |
| Husum Amt | 3.500 | 36.000 | 39.500 |
| Gottorp Amt | 9.000 | 30.000 | 39.000 |
| Hütten Amt | 3.200 | 26.000 | 29.200 |
| Ærø | 0 | 8.500 | 8.500 |
| Femern | 0 | 7.500 | 7.500 |
| Kiel Amt | 8.500 | 23.500 | 32.000 |
| Rendsborg Amt | 7.500 | 36.500 | 44.000 |
| Nørre Ditmarsken | 0 | 18.000 | 18.000 |
| Søndre Ditmarsken | 0 | 23.500 | 23.500 |
| Steinborg Amt | 10.200 | 28.000 | 38.200 |
| Pinneberg Amt | 25.500 | 23.000 | 48.500 |
| Neumünster Amt | 2.200 | 12.500 | 14.700 |
| Segeberg Amt | 0 | 45.000 | 45.000 |
| Plön Amt | 0 | 36.000 | 36.000 |
| Cismar Amt | 0 | 34.000 | 34.000 |
| Stormarn | 0 | 30.500 | 30.500 |
| Lauenborg | 2.200 | 39.500 | 41.700 |
