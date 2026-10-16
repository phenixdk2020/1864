# Landeskabelon: alt vi skal vide om et land

Formål: én liste over hvad spillet (kampagne og slag) skal bruge for at et land kan spilles eller bruges som modstander, så vi kan føre Danmark mod Sverige-Norge (og senere Preussen, Østrig m.fl.) uden at opdage huller midt i et slag. Danmark er referencen; kolonnen "Danmark i dag" viser, hvor vi står, og "Sverige-Norge i dag" hvor vi mangler.

Dato for dokumentet: 2026-10-14. Alle tal for Sverige-Norge i dette dokument er afrundede ankre fra min viden eller fra Codex' cirka-design og skal kildetjekkes, før de bruges som fakta.

## 1. Tre niveauer af et land

| Niveau | Hvad landet er i spillet | Hvad der kræves | Eksempler i dag |
|---|---|---|---|
| A. Abstrakt nation | Tal i `Nations<år>.json`: befolkning, hær, jernbane-km, skat, vægte, relation. Spilleren ser landet i diplomatiet og i AI'ens beslutninger. | Kun afsnit 2.1, 2.9 og 2.12 | Rusland, Frankrig, Storbritannien, Østrig |
| B. Nabo på kortet | A plus byer, ejer-grid, veje/færger/jernbaner, garnisoner og felthære. Kan angribes, og slag kan udkæmpes i dens grænseområde. | A plus 2.2, 2.4, 2.5, 2.8 (garnisoner og felthær), 2.11 | Preussen, Mecklenburg, Hannover, Hamborg, Lübeck og et udsnit af Sverige (Skåne) |
| C. Fuldt land | B plus eget amtskort med byer og land adskilt, økonomi, produktion, bygninger, officerer, ministre, forskning, begivenheder og uniformer på figurerne. Landet kan spilles af spilleren. | Hele kapitel 2 | Danmark |

Sverige-Norge er i dag niveau A (abstrakt) med et lille niveau-B-udsnit (Skåne). For at "køre det mod" os skal det op på niveau B for hele den sydlige del af Sverige og Norge langs Oslofjorden, og helst C, hvis det skal kunne spilles.

## 2. Hvad vi skal vide, felt for felt

Statuskoder bruges i alle datafiler: `kilde` (afskrevet fra en kilde), `skøn` (afrundet periodeskøn), `spil` (spilvalg, ikke historie). Kilde skal altid stå ved siden af tallet.

### 2.1 Identitet og styre
| Felt | Beskrivelse | Fil i dag |
|---|---|---|
| `id`, `name`, farve, flag | To-bogstavs id (DK, SE, NO), navn, kortfarve, flagbillede (105x67) | `Nations<år>.json`, `cities_neighbours_<år>.json` (nations) |
| Statsform og regent | Fx Oscar I (konge fra 1844), enevælde eller konstitutionelt | `Ministers_1825.json` (sovereign), `Nations<år>.json` (note) |
| Union og delstater | Sverige-Norge: to stater, én konge. Egen regering, parlament (Riksdag og Storting), hær, finanser og lovgivning | Kun som note; skal modelleres (kapitel 4) |
| Controller | `player` eller `ai`; `onMap`: om landet har fuld økonomi og rekruttering på kortet | `Nations<år>.json` |
| Regeringsvægte | Fire (1851) eller otte (1825) ressortvægte, forsigtighed, reserve | `Nations<år>.json` (weights, caution, reserve) |

### 2.2 Geografi og kort
| Felt | Beskrivelse | Fil |
|---|---|---|
| Landmaske og ejer-grid | Hvem ejer hver celle (uint8, 768 x 787, nord øverst) | `Neighbours_Owners.bin` |
| Grænser | Polygoner pr. periode (grænser flytter sig) | `cities_neighbours_<år>.json` (territories) |
| Højdedata | Terræn til slagmarken og til kampagnekortet | `Denmark1851_Height.png` |
| Natur | Skov, vand, mose (feature-kort), floder og søer | `Denmark1851_Features.png`, `Hydro1851.json` |
| Veje og færger | Forbindelser mellem byer med km, vejtype (chaussé), færgesteder | `links` i nabofilen, `roads` i kortfilen |
| Broer og vadesteder | Hvor kan en flod krydses; broer kan sprænges | `Bridges1851.md` |
| Kortudstrækning | Dækker kortet landet? Norge og det nordlige Sverige ligger uden for dagens udsnit | `Naboer.md` |

### 2.3 Administrative enheder
Danmark har 41 enheder (amter og hertugdømmer) i `Denmark1851_Map.json` (`amter`): id, navn, sæde, areal, byer, bybefolkning, landbefolkning, samlet befolkning, region. Naboer bruger `districts` (fx Mecklenburgs domanialamter) med nation, sæde, befolkning, status og grænsestatus. For Sverige-Norge: län (Malmöhus, Kristianstad, Blekinge, Halland, Göteborg og Bohus, Älvsborg, Skaraborg, Jönköping, Kronoberg, Kalmar, Östergötland, Södermanland, Stockholm m.fl.) og norske amter (Smaalenene, Akershus, Hedemarken m.fl.). Kun de, der ligger på kortet, skal med først.

### 2.4 Befolkning og byer
| Felt | Beskrivelse | Fil |
|---|---|---|
| Befolkning pr. enhed | 1825, 1851 og som reference 1864; land og by hver for sig | `Population_1825.json`, kortfilen |
| By | Navn, position, befolkning pr. år, havn, fæstning, garnison, kanoner, jernbaneår, amt | `cities_neighbours_<år>.json` (cities) |
| Status | Hver by og enhed skal have kilde/skøn og en kildelænke | `populationStatus` |
| Vækst | Årlig vækst pr. nation (bruges i simuleringen) | `growth` |

### 2.5 Økonomi
| Felt | Beskrivelse | Fil |
|---|---|---|
| Skat | Skat pr. indbygger på land og i by, samlet `taxPerHead` | `Economy_1825.json` (ruralTax, urbanTax), `Nations<år>.json` |
| Priser og lønninger | Korn, kvæg, smør, jern, kul, tømmer, krudt, tøj, læder; ufaglært dagløn | `Economy_1825.json` |
| Kredit og gæld | Basisrente, gæld, startkasse (Danmark: 5.000.000 i koden) | `Economy_1825.json` (creditBase), `Campaign1851Map.h` |
| Udgifter | Hær, flåde, administration, rente, jernbaner; budgetlinjerne i kampagnen | Koden i `Campaign1851Map.cpp` |
| Handel | Import og eksport (vægt og værdi), toldregler, handelstraktat (`tradeValue`) | `Trade_1851.json` (ny), `Nations<år>.json` |
| Valuta | Danmark: rigsbankdaler/rigsdaler sølv som sammenlignelig enhed. Sverige: riksdaler riksmynt, Norge: speciedaler. Hver omregnes til spillets rigsdaler sølv med en angivet kurs | skal udfyldes (tom for SE/NO) |
| Jernbaneindtægt | Passagerer og fragt efter byernes størrelse og afstand (gravitationsmodel) | `RailwayIncomePerYear` i `Campaign1851Network.cpp`, `Tools/Economy/rail_model.py` |

### 2.6 Landbrug, industri og råstoffer
Husdyr (heste, kvæg, får, svin), korn- og smørproduktion pr. indbygger, industriindeks, industricentre, mineraler og skov. Bruges til forsyning, rekruttering af heste og eksport. Danmark: `Economy_1825.json` plus bygningslisten. Sverige-Norge: mangler helt. Vigtigt for Sverige: jernmalm og stål (Bergslagen), tømmer og tjære. Norge: tømmer, fisk og skibsfart.

### 2.7 Infrastruktur og bygninger
Bygningskatalog med omkostning, byggetid, mandskab, krav og effekt (`Buildings1851.csv`, `Buildings1851-Costs.md`), jernbanelinjer med åbningsår og stationer, havne, fæstninger og fortifikationer (`Fortifications1851.md`), magasiner (`magazines` i hærfilen). Sverige-Norge: de første jernbaner kommer først i 1850'erne (Norge åbner Christiania til Eidsvoll i 1854 ifølge `Nations1851.json`); for 1851 er der i praksis ingen at modellere.

### 2.8 Hær
| Felt | Beskrivelse | Fil |
|---|---|---|
| Generalkommandoer | Hærens inddeling i områder med hovedkvarter, byer og chef | `Army1851.json` (commands) |
| Regimenter | Id, navn, våben (infanteri, kavaleri, artilleri, garde), hjemby, styrke, heste, erfaring, seks færdigheder | `Army1851.json` (regiments) |
| Garnisoner | By, mandskab, mobiliseret, kanoner | `Army_Neighbours_<år>.json` |
| Felthære | Mobiliserbare mænd, hvor de samles, tid til mobilisering | `Army_Neighbours_<år>.json` (armies) |
| Udrustning | Rifler, kanoner, heste, morterer, vogne, råstoffer, madforråd i dage, ladetid, træfsikkerhed, våbentekst | `Army_1825.json` (equipment) |
| Magasiner | Byer med mad, foder og ammunition | `Army_1825.json` (magazines) |
| Rekruttering | Hvordan mænd kommer ind: udskrivning, værnepligt, indelningsverk | skal modelleres pr. land |
| Fredsstyrke og reserve | Adskilt; unioner tælles én gang | `armyMen`, `reserve` |
| Uniformer | Pr. våben og periode; billeder og figurmesh | `Reference/Units`, `import_units_1864.py` |

### 2.9 Flåde
Skibe efter klasse, havne, mandskab og kanonbestykning (`Navy1851.md`). Sverige-Norge har en unionsflåde med skærgårdsflåde og linjeskibe og et dansk-svensk kapløb om Øresund. Afsnit er endnu ikke udfyldt for noget land ud over Danmark.

### 2.10 Officerer, ministre og ledelse
Generaler med ni egenskaber (ledelse, inspiration, initiativ, taktik, stab, disciplin, aggression, ro, politik) og erfaring, navnelister til tilfældige chefer, løn og rekrutteringspris (`Officers1851.json`, `Officers_1825.json`), ministerkandidater pr. ressort (`Ministers_1825.json`). Historiske generaler får skønnede værdier; ingen opdigtede chefer uden at det står.

### 2.11 Diplomati og begivenheder
Relation til Danmark (-100 til 100), handelsværdi pr. år med en traktat, `canAlly`, `canGuarantee`, garantier (stormagterne), spændingsniveau og begivenheder med vindue, sandsynlighed, forudsætninger og effekter (`Events.json`, `Events_1825.json`, `Diplomacy1851.md`). For Sverige-Norge er skandinavismen den vigtigste: en gensidig forsvarsaftale i 1864 var på tale og kom ikke.

### 2.12 Forskning, uddannelse og politik
Forskningsindeks, institutioner (universiteter, krigsskoler), forskningstræ, politisk system og kernekonflikter (`Research1851.md`, `Research-Tree.md`, `Politics1851.md`).

### 2.13 Grafik og lyd
Flag, våbenskjold, uniformsbilleder (fra mange vinkler), portrætter, figurmeshes med bagte animationer, rapportikoner, bynavne og sprog (dansk, svensk, norsk), lydeffekter og kommandoer. Sverige har en uniformsbeskrivelse m/1860 (fra 1860) og en mesh `SK_SE_Infantry_1864`; jeg har ikke tjekket, om der findes uniformer til 1851 til 1859.

## 3. Datakvalitet: regler for at tilføje et land

1. Hver tal står med `status` (kilde, skøn, spil) og kilde. Ingen opfundne navne, tal eller kildehenvisninger.
2. En dato pr. scenarie (1. juli 1825 og 1. juli 1851; 1864 er reference). Årsstrømme (handel, høst) må ikke forveksles med beholdninger pr. dato.
3. Én pengeenhed i spillet (rigsdaler sølv), med en dokumenteret kurs fra landets egen valuta.
4. Unioner tælles én gang (Sverige-Norge, helstaten). Hver delstat har sin egen hær, kasse og regering, hvis historien kræver det.
5. Befolkning, hær og grænser er afrundede skøn, indtil en kilde er fundet; noter hvad der mangler.
6. Samme indeksrække som eksisterende kort (byer og amter bevarer deres rækkefølge, så gemte spil ikke ødelægges; gemte spil har versionsnummer).
7. Hver ny fil får en kort `note` på dansk om kilder og usikkerhed.

## 4. Sverige-Norge: hvor vi står, og hvad der mangler

### 4.0 Gældende integrationsstatus — 2026-10-16

Runde 7 er delvist importeret; **niveau B er ikke fuldført**. Se [Import-Sverige-Norge.md](Import-Sverige-Norge.md) for alle 30 filers mapping, loaderændringer, proveniens, kortdækning, rapport og risici. Afsnit 4.1 og 4.4 nedenfor beskriver det tidligere udgangspunkt.

- SE/NO har separate aktive nationsrækker i både 1825 og 1851; nations-onMap=false bevares, fordi feltet kræver eget økonomi-/rekrutteringskort.
- 17 svenske bypunkter understøttes af eksisterende extent/ejergrid, heraf seks nye. Garnisoner og SE-south er opdateret fra pakken. 46 by-/transportpunkter pr. scenarie ligger udenfor og bevares som onMap=false-metadata. Ingen norsk by er på det aktive kort.
- 42 distrikter pr. scenarie er bevaret som ikke-aktive data; 8 repræsentative punkter er indenfor extent, men der mangler polygoner og distriktsgrid. Ingen kortbinærer ændres.
- National OOB, Supply, økonomi, officerer, ministre, events, valuta og unionsregler er bevaret med proveniens. De er ikke koblet til danske puljer. National OOB og nabohær spawnes ikke samtidigt.
- Niveau B mangler stadig sammenhængende ruter/kort, svensk-norsk krigs-/unionsadapter, national forsynings-/mandskabskobling og spil-/savevalidering. Niveau C mangler derudover fulde nationale systemer og grafik som beskrevet i importnotatet.
- Begge naboscenarier består statisk validering; 350 beskyttede rækker er byteidentiske. Ingen build, spil/editorstart, spiltest eller commit.

### 4.1 Udgangspunkt før importen
- `Nations1851.json`: `SE` (Sverige): befolkning 3.480.000, hær 35.000, reserve 150.000, skat 0,2 pr. hoved, relation 40, handelsværdi 3.000, alliance mulig (`canAlly`), `onMap: false`, vægte [1.2, 1.0, 0.8, 0.5], forsigtighed 0,7. `NO` (Norge): 1.490.000, hær 12.000, reserve 60.000, relation 45, `onMap: false`. Begge er skøn.
- `cities_neighbours_1851.json`: svenske byer i Skåne (Malmö, Helsingborg, Landskrona, Lund, Ystad m.fl.) og punkter uden for kortet (Kristiania/Oslo, Fredrikshald, Karlsborg) med ejer og krone.
- `Army_Neighbours_1851.json`: garnisoner i Malmö (400 mand, 8 kanoner), Landskrona (400, 12), Helsingborg (150), Lund (100), Ystad (150) m.fl. Sveriges 35.000 til 40.000 mobiliserbare er unionens sydlige krigsteater, ikke garnisonen.
- Uniformer og mesh til 1860'ernes svenske infanteri (`SK_SE_Infantry_1864`) og en test (`Start-Livgarden-Svensk-Test.bat`).
- Ingen svenske eller norske begivenheder: `Events.json` (7) og `Events_1825.json` (6) handler kun om helstaten, Holsten og Slesvig, London-protokollen og Forbundseksekutionen. Alliance og skandinavisme findes kun som felter (`canAlly`, `relation`) i nationsfilen.

### 4.2 Hvad der mangler for at køre det mod Danmark (niveau B)
1. Kort: Skåne, Halland, Blekinge og Småland, samt Norge langs Oslofjorden og Østfold, ud over det nuværende nabokort (Naboer.md). Højde, skov og vand for hele krigsteatret.
2. Län og amter med befolkning (1825, 1851), byer og land adskilt, alle byer over ca. 2.000 indbyggere.
3. Veje, færger og broer i Skåne og over Svinesund/Fredrikshald; jernbaner (få i 1851).
4. Hær: indelningsverket (tildelte regimenter pr. landskab), værvede regimenter, garde, artilleri, kavaleri; hvert regiments hjemby, styrke og erfaring; norsk hær separat (værnepligt, hær ca. 12.000); fæstninger (Karlsborg, Landskrona, Karlsten, Fredriksten, Oslo/Akershus).
5. Udrustning: våbentyper pr. periode (glatløbede geværer, de første kapselgeværer, rifler), artilleri, ladetid og træfsikkerhed pr. våben, magasiner.
6. Flåde: unionens skibe og kanonbåde, havne (Karlskrona, Göteborg, Horten).
7. Økonomi: skat, priser, valuta (riksdaler riksmynt og speciedaler til rigsdaler sølv), handel, gæld, startkasse, eksport (jern, tømmer).
8. Styre: Oscar I, ministre, forsigtighed, krigsvilje, unionsreglen (hvem beslutter krig; hvornår stiller Norge tropper).
9. Diplomati: relation, garantier, forhold til Rusland og Storbritannien, begivenheder (skandinavisme, Krimkrig 1853 til 1856, 1864).
10. Officerer: generaler med skønnede værdier, navnelister på svensk og norsk.
11. Billeder: svenske og norske flag, uniformsbilleder pr. periode, portrætter (hvis nødvendigt).
12. Kamp: AI-doktriner, hvordan den svenske hær adskiller sig (fx kavaleri, kampstil), forsyning ved den danske grænse.

### 4.4 Status for Codex' svensk-norske pakke (2026-10-14, aften)

Codex-agenten har leveret en importpakke i `C:/Users/allan/Documents/Codex/2026-10-12/l/outputs/Sverige-Norge-agent/Spilimport/` (README, `Nations1851.json`, `Nations_1825.json`, `Population_1825/1851/1864.json`, `cities_neighbours_1825/1851.json`, `Events.json`, `Officers*.json`, `Trade_1851.json`, `Valuta.json`, `Union-regler.json` samt `SE/` og `NO/` med `Army*.json`, `Army_Neighbours*.json` og `Economy_1825/1851.json`) plus `Research/`, `Arbejdsstatus.md`, `Daekningsmatrix.md` og `verifikationslog.txt`.

- Nøglerne følger spillets filer; ekstra felter er `fieldStatus`, `sovereign`, `union`, `active`, `note`, `source`, `status` og `commodities`. `Nations1851.json` mangler `canGuarantee`, `Trade_1851.json` mangler `comparison1847`. Det er delvise overlays, ikke færdige kampagnefiler.
- Ukendte tal står som `null` med begrundelse (bl.a. fredsstyrke og dele af økonomien); spillets indlæsning skal håndtere `null`, før noget importeres. Intet er importeret i spillet.
- I pakken: SE befolkning 3.498.336 og NO 1.417.310 i 1851-nationsfilen (skøn fra län/amter), norsk skat og gæld dobbelt belagt, svensk statsfinans 1825/1850/1851 fra Fregert og Gustafsson.
- Preussen, Østrig, Hannover/Sachsen og uniformundersøgelsen er ikke lavet: Codex' kvote var brugt op, og den åbner igen 15. oktober kl. 02:38.

### 4.3 Foreslået rækkefølge
1. Kilder for 1825 og 1851: befolkning pr. län (SCB), hærens opbygning (indelningsverket), Norges folketælling 1801, 1835, 1865 (til interpolering), Norges hær (værnepligt).
2. Abstrakte tal (afsnit 2.1, 2.5, 2.8 fredsstyrke) færdige og kildebelagte, så nationen kan indgå i AI'en.
3. Kortudsnit og byer (niveau B), garnisoner og felthær.
4. Uniformer og figurer til 1851 (kun hvis spillet kan føres 1851 til 1859); tjek først, hvad der findes.
5. Test: en slagtest med svenske kompagnier (findes allerede) og en kampagnetest med en svensk mobilisering ved grænsen.

### 4.5 Preussen: status 2026-10-16

Preussen er **niveau B for kortdata** (ikke niveau C, ikke spilbart). Importeret fra `Preussen-agent/Spilimport` med `Tools/Countries/import_prussia.py`; se [Import-Preussen.md](Import-Preussen.md) for mapping, rapport og huller.

- Aktivt: 25 byer, 13 distrikter, 33 ruter, 25 garnisoner og 9 felthærposter pr. scenarie (1825 og 1851 adskilt), alle inden for kortudsnittet og nåelige fra København (fem via færge). Pakkens tal er de samme som spillets arvede skøn; importen tilføjer status/kilde pr. tal og opdaterer kun den nationale befolkning (HGIS-skøn). Köslin er `onMap=false`.
- Arkiveret, ikke aktivt (kræver preussisk pulje eller loaderudvidelse): hærkatalog, officerer (Müffling, Wrangel, Stockhausen), økonomi (alle satser `null`), provinsbefolkning, handel og 12 events. Events er ikke aktiveret, fordi de ændrer dansk spænding og delvis afvises af `ResetWar`.
- Mangler til en fuld modstander: faktiske ruller og chefer, økonomital, forsyning og mobiliseringskalender, krigsmodel ud over det nuværende PR/AT/DE-flag, kortrastre, flåde. Relation 1851 er spillets −20 (pakken har −35; ikke taget over).

## 5. Kalibreringsdata brugt fra Codex' cirka-design (2026-10-14)

- **Handel 1851:** lagt i `Data/Campaign1851/Trade_1851.json` (kilde: Danmarks Statistik, Tabelværk 21. hæfte, Told og Skibsfart 1851). Kongeriget: import 28,1 mio. og eksport 14,7 mio. rigsbankdaler. Slesvig: import 8,5 mio. og eksport 3,9 mio. Samlet omsætning for Kongeriget og Slesvig: 55,2 mio. (48,7 mio. i 1847). Vægt og værdi er årsstrømme med forskellige prisgrundlag. Næste skridt: sammenlign med kampagnens eksportmodel (`exportDuty` 0,02 i `Economy_1825.json`) og juster priser eller mængder, så modellen giver samme størrelsesorden.
- **Befolkning:** Codex' totale tal for kongeriget (1.440.000 i 1851 og 1.170.000 i 1825), Slesvig (380.000/340.000), Holsten (505.000/430.000) og Lauenborg (48.000/40.000) bruges som sanity check. Vores kort summerer til 2.298.100 (alle enheder, også Slesvig og Holsten), omkring 3 % under Codex' 2.373.000.
- **Ikke brugt:** Codex' amtstal (grovere end vores), økonomi (startkasse 1,5/2 mio., indtægter og gæld er gæt), hær (20.000/30.000 mand er "illustrativ spilbalance") og jernbaneudpluk.
- **Diplomati:** Preussen -35 i 1851 som startværdi er en rimelig spilværdi; London-ordningen af 1852 må ikke antages gældende 1. juli 1851.

## 6. Skabelon til et nyt land (kopiér og udfyld)

```
Land: ____   id: __   niveau (A/B/C): __   scenarier: 1825 / 1851 / 1864-reference
Regent: ____   statsform: ____   union/delstater: ____
Valuta: ____ (kurs til rigsdaler sølv: ____, kilde: ____)
Befolkning: 1825 ____ (status/kilde) | 1851 ____ | 1864 ____
Skat pr. hoved: land ____ by ____    Gæld: ____   Startkasse: ____
Landbrug: heste ____ kvæg ____ får ____ svin ____    Industriindeks: ____ centre: ____
Jernbane-km: 1851 ____ (linjer med åbningsår: ____)
Havne: ____    Fæstninger: ____
Hær: fredsstyrke ____ reserve ____ mobiliserbare ved grænsen ____
     kommandoer ____ regimenter (id, arm, hjemby, styrke, erfaring): ____
     udrustning: våben ____ ladetid ____ træfsikkerhed ____ magasiner ____
Flåde: skibe ____ havne ____
Officerer: generaler (navn, født, 9 egenskaber, erfaring) ____ ministre ____
Diplomati: relation ____ handelsværdi ____ alliance ____ garanti ____ begivenheder ____
Kort: län/amter (navn, sæde, befolkning land/by) ____ byer ____ veje/færger/broer ____
Grafik: flag ____ uniformer pr. periode ____ portrætter ____
Kilder og huller: ____
```
