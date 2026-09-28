# Bygninger 1851 — pris og byggetid (forslag)

Genereret af `Tools/Campaign/building_costs.py` — ret konstanterne dér og kør scriptet igen.
Priserne er spilestimater i et 1850'er-prisniveau, ikke historisk efterprøvede tal.

## Model

- **Penge** = løn + materialer + særudstyr. Rigsdaler (rd., 96 skilling).
- **Løn**: 1 rd. pr. mandsdag (jordarbejde 0.8 rd.).
- **Materialer**: tømmer 12 rd./læs, byggematerialer (tegl, sten, kalk) 16 rd./læs, jern 6 rd./centner.
- **Tid** = mandsdage / største sjak + leveringstid for importeret udstyr.
- Hver bygning har en finjustering inden for sin størrelse (FACTORS), fx rådhus ×1,3, skole ×0,7.
- **Drift** pr. år: militært 4 %, statsligt 3 %, by 2 % af byggeprisen; private bygninger drives af ejeren.

| Størrelse | Mandsdage | Største sjak | Grundtid |
| --- | ---: | ---: | ---: |
| XS | 800 | 15 | 53 dage |
| S | 3.000 | 40 | 75 dage |
| M | 10.000 | 80 | 125 dage |
| L | 30.000 | 200 | 150 dage |
| XL | 90.000 | 400 | 225 dage |
| XXL | 250.000 | 800 | 312 dage |

| Byggemåde | Tømmer / 1.000 md | Byggemat. / 1.000 md | Jern / 1.000 md |
| --- | ---: | ---: | ---: |
| bindingsværk | 28 læs | 12 læs | 4 ctr. |
| grundmur | 12 læs | 45 læs | 8 ctr. |
| industri | 12 læs | 40 læs | 30 ctr. |
| jordværk | 6 læs | 6 læs | 2 ctr. |
| stenværk | 10 læs | 60 læs | 10 ctr. |

## Bygninger

| Bygning | Kategori | Bygherre | Str. | Pris | Tid | Tømmer | Byggemat. | Jern | Drift/år | Kræver | Giver |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Infanterikaserne | Garnison | Stat (militær) | L | 57.400 rd. | 150 d | 360 | 1350 | 240 | 2.300 | Købstad over 2.500 indb. med garnisonsgrund | Indkvartering af 1 bataljon (ca. 800 mand) |
| Stalde | Garnison | Stat (militær) | M | 15.300 rd. | 100 d | 96 | 360 | 64 | 610 | Infanterikaserne | 200 heste opstaldet ved garnisonen |
| Depot og magasin | Garnison | Stat (militær) | M | 17.200 rd. | 112 d | 108 | 405 | 72 | 690 | Infanterikaserne | Mobiliseringslager til 1 bataljon (uniformer, våben, feltudstyr) |
| Sygestue | Garnison | Stat (militær) | S | 6.900 rd. | 90 d | 43 | 162 | 29 | 280 | Infanterikaserne | 60 senge til garnisonen |
| Mobiliseringsdepot | Militær | Stat (militær) | L | 45.900 rd. | 120 d | 288 | 1080 | 192 | 1.840 | Infanterikaserne | Samling og udrustning af reserver |
| Remontedepot | Militær | Stat (militær) | L | 41.900 rd. | 135 d | 756 | 324 | 108 | 1.680 | Landområde med græsning | Modtagelse og fordeling af 600 erstatningsheste |
| Officersskole | Militær | Stat (militær) | L | 68.800 rd. | 180 d | 432 | 1620 | 288 | 2.750 | By over 10.000 indb. | Uddannelse af officerer (langsigtet) |
| Arsenal | Militær | Stat (militær) | XL | 176.800 rd. | 225 d | 1080 | 3600 | 2700 | 7.070 | By over 10.000 indb. med havn | Lager, reparation og udlevering af materiel |
| Kanonstøberi | Militær | Stat (militær) | XL | 234.400 rd. (heraf 40.000 rd. boreværk og ovne (import)) | 338 d | 1188 | 3960 | 2970 | 9.380 | Maskinværksted; adgang til jern og kul | Nye kanoner og lavetter |
| Geværværksted | Militær | Stat (militær) | M | 34.600 rd. (heraf 15.000 rd. værktøjsmaskiner (import)) | 185 d | 120 | 400 | 300 | 1.380 | Maskinværksted | Nye og ombyggede geværer |
| Ammunitionsfabrik | Militær | Stat (militær) | M | 25.700 rd. (heraf 8.000 rd. presser og støbeforme) | 142 d | 108 | 360 | 270 | 1.030 | Krudtværk eller krudtmagasin i nærheden | Patroner og granater |
| Krudtværk | Militær | Stat (militær) | L | 57.100 rd. (heraf 10.000 rd. stampe- og kornværk) | 150 d | 288 | 960 | 720 | 2.280 | Vandløb; mindst 2 km fra by | Krudt |
| Krudtmagasin | Militær | Stat (militær) | S | 3.800 rd. | 98 d | 23 | 23 | 8 | 150 | Uden for byen | Sikkert krudtlager |
| Kystbatteri | Militær | Stat (militær) | M | 21.800 rd. (heraf 12.000 rd. 6 kystkanoner fra arsenalet) | 125 d | 60 | 60 | 20 | 870 | Kyst ved sejlløb | Forsvar af havn eller sejlløb |
| Skanse / fæstningsværk | Militær | Stat (militær) | XXL | 275.000 rd. (heraf 30.000 rd. palisader, blokhuse og kanoner) | 312 d | 1500 | 1500 | 500 | 11.000 | Strategisk punkt (fx Dannevirke, Dybbøl, Fredericia) | Permanent forsvarsstilling |
| Lazaret | Sundhed | Stat (militær) | M | 19.100 rd. | 125 d | 120 | 450 | 80 | 760 | By over 2.500 indb. | 200 senge; hurtigere helbredelse af sårede |
| Sygehus | Sundhed | By | L | 63.100 rd. | 165 d | 396 | 1485 | 264 | 1.260 | By over 5.000 indb. | 300 senge; civil og militær behandling |
| Rådhus | Civil | By | M | 24.900 rd. | 162 d | 156 | 585 | 104 | 500 | Købstad | Administration og ro i byen |
| Tinghus / domhus | Civil | Stat | M | 21.000 rd. | 138 d | 132 | 495 | 88 | 630 | Amtsby | Retspleje og orden i amtet |
| Posthus | Civil | Stat | S | 5.700 rd. | 75 d | 36 | 135 | 24 | 170 | Købstad | Post og rapporter hurtigere |
| Skole | Civil | By | S | 4.000 rd. | 52 d | 25 | 94 | 17 | 80 | – | Uddannelse (langsigtet) |
| Telegrafstation | Infrastruktur | Stat | S | 6.900 rd. (heraf 4.000 rd. apparater og batterier (plus ledning pr. km)) | 68 d | 18 | 68 | 12 | 210 | Fra 1854 (den første danske telegraf) | Øjeblikkelige ordrer og rapporter |
| Jernbanestation | Infrastruktur | Stat | M | 19.100 rd. | 125 d | 120 | 450 | 80 | 570 | Jernbanelinje (1851: kun København-Roskilde) | Lastning og losning af tog |
| Godsbanegård | Infrastruktur | Stat | M | 15.700 rd. | 100 d | 96 | 320 | 240 | 470 | Jernbanestation | Større godskapacitet på banen |
| Banevogterhus | Infrastruktur | Stat | XS | 1.500 rd. | 53 d | 10 | 36 | 6 | 40 | Jernbanelinje | Sikker overkørsel og banevogter |
| Stenbro | Infrastruktur | Stat | M | 20.200 rd. | 112 d | 90 | 540 | 90 | 610 | Vej over å | Fast overgang; tåler tung trafik |
| Toldbod / havnekontor | Havn | Stat | S | 6.900 rd. | 90 d | 43 | 162 | 29 | 210 | Havneby | Told og havneadministration |
| Havnepakhus | Havn | By | M | 21.000 rd. | 138 d | 132 | 495 | 88 | 420 | Havneby | Lager ved kajen (import og eksport) |
| Fyrtårn | Havn | Stat | S | 12.400 rd. (heraf 3.000 rd. linseapparat (import)) | 150 d | 42 | 252 | 42 | 370 | Kyst | Sikrere sejlads om natten |
| Skibsværft | Havn | Privat | XL | 131.700 rd. (heraf 20.000 rd. beddinger og kraner) | 180 d | 2016 | 864 | 288 | – | Havneby | Bygning og reparation af skibe |
| Marinestation | Havn | Stat (militær) | M | 22.900 rd. | 150 d | 144 | 540 | 96 | 920 | Orlogshavn | Flådens administration og forsyning |
| Teglværk | Industri | Privat | M | 17.300 rd. (heraf 2.000 rd. ringovn) | 100 d | 96 | 360 | 64 | – | Lerforekomst | Byggematerialer |
| Savværk | Industri | Privat | S | 7.700 rd. (heraf 3.000 rd. vandhjul eller dampmaskine) | 105 d | 84 | 36 | 12 | – | Skov i amtet | Tømmer |
| Maskinværksted | Industri | Privat | M | 31.600 rd. (heraf 12.000 rd. dampmaskine og drejebænke (import)) | 185 d | 120 | 400 | 300 | – | By over 10.000 indb. | Maskinkapacitet og reparationer |
| Vognfabrik | Industri | Privat | M | 10.900 rd. | 88 d | 196 | 84 | 28 | – | Købstad | Vogne til hær og civilsamfund |
| Sadelmageri | Industri | Privat | S | 3.700 rd. | 60 d | 67 | 29 | 10 | – | Købstad | Seletøj og sadler |
| Garveri | Industri | Privat | S | 4.200 rd. | 68 d | 76 | 32 | 11 | – | Købstad ved vand | Læder |
| Klædefabrik | Industri | Privat | L | 78.900 rd. (heraf 20.000 rd. væve og spindemaskiner (import)) | 210 d | 360 | 1200 | 900 | – | By over 5.000 indb. | Klæde til uniformer |
| Bryggeri og brænderi | Industri | Privat | M | 20.200 rd. (heraf 3.000 rd. kobberkedler) | 112 d | 108 | 405 | 72 | – | Købstad | Skatteindtægt og fødevarer |
| Kulmine | Industri | Privat | L | 66.900 rd. (heraf 8.000 rd. pumpe og hejseværk) | 180 d | 360 | 1200 | 900 | – | Kun Bornholm | Kul |
| Købmandsgård | Handel | Privat | M | 15.300 rd. | 100 d | 96 | 360 | 64 | – | Købstad | Handel og lager |
| Kornmagasin | Landbrug | Stat | M | 19.100 rd. | 125 d | 120 | 450 | 80 | 570 | Købstad eller havn | Regional kornreserve |
| Stutteri | Landbrug | Stat | L | 42.200 rd. (heraf 5.000 rd. avlsheste) | 120 d | 672 | 288 | 96 | 1.270 | Landområde med græsning | Avl af heste (virker efter år) |
| Kirke | Kirke | By | L | 40.200 rd. | 105 d | 252 | 945 | 168 | 800 | – | Sognets kirke |
| Herregård | Bolig | Privat | L | 80.300 rd. | 210 d | 504 | 1890 | 336 | – | – | Godsets hovedbygning |
| Proprietærgård | Bolig | Privat | M | 22.900 rd. | 150 d | 144 | 540 | 96 | – | – | Større landbrug |
| Lade | Land | Privat | S | 3.700 rd. | 60 d | 67 | 29 | 10 | – | – | Høst- og foderlager |
| Smedje | Land | Privat | XS | 1.500 rd. | 53 d | 10 | 36 | 6 | – | – | Beslag og reparationer |
| Husmandssted | Land | Privat | XS | 1.000 rd. | 43 d | 18 | 8 | 3 | – | – | Bolig for husmand |
| Bondegård (firlænget) | Land | Privat | S | 6.100 rd. | 98 d | 109 | 47 | 16 | – | – | Landbrug |
| Kro | Land | Privat | S | 5.100 rd. | 82 d | 92 | 40 | 13 | – | Landevej | Rasteplads for rejsende og tropper |
| Stald | Land | Privat | S | 3.300 rd. | 52 d | 59 | 25 | 8 | – | – | Heste og kvæg |
| Vindmølle | Land | Privat | S | 5.200 rd. (heraf 1.500 rd. møllehat og kværne) | 60 d | 67 | 29 | 10 | – | – | Mel til by og hær |
