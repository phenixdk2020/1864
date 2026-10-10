# Danmarks naboer på kampagnekortet

## Fase 0 — inventar (2026-10-10)

Kortets autoritative LAEA-udsnit er x −160,577..384,332 og y 111,195..669,478 km, størrelse 544,91 × 558,28 km; projektionens centrum er 52°N, 10°Ø. Geografisk ramme omtrent 7,6..15,75°Ø og 53..57,9°N. Bornholm er på hovedkortet. Kristiania/Oslo, Fredrikshald/Fredriksten og Karlsborg ligger udenfor og findes kun i metadata; de flyttes ikke ind på kortet. Norge hører til den svenske unions politiske model, uden norske byer i udsnittet.

`build_map.py` bruger Natural Earth 10m og `shp.py`. Moderne landegrænser kan ikke bruges som periodens tyske ejergrænser. Monarkiets eksisterende landmaske, amter, Kongeå/Ejder og 73 monarkibyers indeks bevares. Features-rasterets R angiver monarkiets land; G dets skov. `IsMonarchyLand` bevarer bygge-/ejerskabsbegrænsningen. Udlandets højder og vegetation er kunstneriske skøn; Hydro1851 er ikke fuldstændig hydrografi.

Vejnettet har navngivne endepunkter, færger og jernbaner. 1825-befolkningslaget bruger samme indeks. Nationernes `bOnMap` betyder fuld dansk økonomi/rekruttering og sættes ikke for naboer. Krig bruger fortsat dansk/preussisk/østrigsk krigsflag, korps, bybesættelse og efterretninger; det er ikke et generelt krigssystem mellem alle nationspar.

## Fase 1 — scenariedata

Implementeret i `Tools/Map1851/neighbours.json`, `export_neighbours.py` og `Data/Campaign1851`:

- 24 byer i Sverige, Preussen, Mecklenburg, Hannover, Oldenburg samt Hamborg, Lübeck og Bremen. Eksisterende udenlandske markører opdateres ved navn; nye byer tilføjes efter eksisterende indeks.
- `cities_neighbours_1825.json` og `cities_neighbours_1851.json`: ejer, region, befolkning, havn, fæstningsnavn, garnison, kanoner, 28 vej-/færgeforbindelser, nationale farver og etiketter.
- `Neighbours_Owners.bin`: uint8, 768 × 787, række 0 mod nord. Periodens ejergrænser er eksplicitte skøn; monarkiets maske har forrang. Begge scenarier bruger samme ejergrid.
- `Army_Neighbours_<år>.json`: 24 garnisoner og fem felthære med fredsstyrke og mobiliseret styrke. Sveriges 35.000/40.000 mobiliserbare mand er et skøn for det sydlige krigsteater, ikke en fredsgarnison.
- 1825 har ingen jernbaner. 1851 har to åbne mecklenburgske baner. Øvrige byers jernbaneår er metadata, ikke et fuldt banenettet.
- 1851-nationslisten suppleres med fristæder og en inaktiv Forbundsoversigt. Eksisterende nationer og 1825-listen bevarer deres indeks/data.

Befolkninger, styrker, grænser og linjeføringer er markerede periode-/spilskøn. 1864-befolkninger er referencetal i kilden, ikke et nyt scenarie. Der mangler verificerede folketællingskilder pr. by.

Generatoren kræver Python, NumPy, Pillow og SciPy:

```powershell
python Tools/Map1851/export_neighbours.py
python Tools/Map1851/export_neighbours.py --natural-earth <mappe-med-ne_10m_land>
python Tools/Map1851/validate_neighbours.py
```

Natural Earth-kilder findes ikke på forventet sti i dette checkout. Eksporten bruger derfor højderaster og Features som eksplicit kystfallback. Tærsklen er motorens `0,002 × 65535`, da højder er 16-bit. Landruter bruger A* med en rastercelles tolerance ved kyst/floder; linjeføringen er tilnærmet. Øresund, Elben og Strelasund er eksplicitte færger. Hovedgeneratoren og eksisterende kortdata omskrives ikke.

## Fase 2 — kort og gameplay

`Campaign1851Neighbours.cpp` indlæses før vejnettet. Nationale territoriefarver vises i kontrolvisningen; grænser og nationale etiketter tegnes. Byer kan vælges med eksisterende klikmekanik. Deres skrivebeskyttede kort viser ejer, region, havn, befolkning og fæstning, uden danske bygge-/rekrutteringsknapper. Fæstninger markeres ved bynavnet og giver deres lokale garnison et skønnet forsvarsbonus på 30 % ved automatisk slagberegning.

Garnisons-/feltstyrke er ukendt uden regiment eller bemandet dansk fæstning inden for opklaringsafstand. Fredelige felthære vises kun ved opklaring; mobiliserede korps bruger eksisterende efterretnings-/slagssystem. Prøveflaget afslører ikke styrker.

Passage kontrolleres ved målet, på vejnettets forbindelser og gennem terræn. Danmark kan gå på eget land, hos allierede og på fjendens land under krig; neutrale naboer afviser passage. Preussiske krigskorps har en begrænset nordtysk transitkorridor; Sverige bliver ikke en genvej. Det eksisterende eventudløste Forbundskorps bevarer passage i hertugdømmerne. Vand kræver fysisk rute-/færgehåndtering. Diplomati efter en marchordre omplanlægger endnu ikke alle igangværende marcher.

Mobilisering sker én gang ved fjendtlighed. Garnisoner bliver ved byen og møder dansk kontakt. Den preussiske felthær får første grænsemål ved Ratzeburg og bruger derefter eksisterende korps-AI. Øvrige nabofelthære er neutrale under det nuværende krigsflag. Eksisterende invasionskorps og events bevares som separate ekspeditioner. Felthærens forstærkninger kræver en tilladt forbindelse til hjembyen; garnisoner får ikke automatiske korpsforstærkninger.

Nabobyer har skønnede magasiner i eksisterende forsyningssystem. Dansk adgang kræver alliance; fjendtlige magasiner er ikke gratis danske depoter. Danmark betaler ikke deres månedlige genopfyldning. Udenlandske vejforbindelser afviser danske anlægsordrer.

## Gem/indlæs

Save-version 32: `Neighbours` gemmer stabilt hær-id → korps-id; tab, placering, march og efterretninger gemmes i eksisterende `War`. Udryddede hære genopstår ikke ved genindlæsning. Nabodepoter gemmes i `Supply`. Ældre saves får standardnabohære og manglende magasiner. Eksisterende by-/amt-/nationsindeks bevares. Fred og nyt spil nulstiller nabomobilisering; statiske metadata indlæses fra scenariet.

## Kontrol og prøveflag

Eksempel: `-CampaignScenario=1825 -CampaignNeighbourShot=PR`. Nations-id'er: `SE`, `PR`, `MEC`, `HAN`, `OLD`, `HH`, `LUB`, `BRE`. Flaget starter et nyt pauset scenarie, fokuserer første by, åbner dens informationskort og kontrolvisningen og gemmer `Saved/Screenshots/neighbour_<id>.png` efter renderforsinkelse. Det afslutter ikke spillet. NO har ingen by i udsnittet og giver en logadvarsel. Logpræfiks: `CAMPAIGN-1851|neighbours|`.

Udført: statisk datavalidering af begge scenarier, unikke ids, nationer, hære, vejendepunkter, længder, udsnit, ejergrid og fravær af 1825-jernbaner; C++-gennemgang og kontrol af anvendte UE 5.8-headere. Ingen build, spil/editorstart, skærmbilledtests eller commit. Statisk gennemgang dokumenterer ikke en kompilering eller visuel spiltest.

## Resterende arbejde

- Build og visuel/gameplay-afprøvning efter udtrykkelig anmodning: begge scenarier, prøveflag, neutrale grænser, alliancepassage, krig/fred, mobilisering, slagtab og v31/v32-save-rundtur.
- Verificerede historiske befolkninger, fæstningsstatus, enklaver og vej-/baneforløb; regenerering med Natural Earth-kilder. Udlandets vegetation/hydrografi kræver mere detaljerede kilder.
- Fuld udenlandsk økonomi, krige mellem vilkårlige nationspar, svensk/norsk mobilisering i andre krige, udenlandsk territorial erobring og passageaftaler uden alliance kræver videre udvidelse af kampagnemodellen.
- Taktiske 3D-modeller/belejringsgeometri for nabofæstninger, vedvarende fredstidsrapporter, omplanlægning ved ændret passage og fuld fjendtlig ration-/ammunitionssimulation er ikke implementeret i denne minimale integration.

Geografisk rettelse: navngivne udenlandske bycentre har forrang i én ejercelle ved rasterkyst og ved Lübeck i den moderne Schleswig-Holstein-maske. Bycentrernes ejere valideres. Den danske byggebegrænsning respekterer rettelsen uden at omskrive Features-filen.
