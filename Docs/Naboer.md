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

## Syd for Danmark

Udvidelse 2026-10-10 til scenarierne **1825 og 1851**. Der er tilføjet **58** byer/landsbyer, heraf de danske grænsebyer Lauenburg og Büchen. Nabofilen indeholder nu 82 byer, 113 forbindelser og 34 sydlige distrikter. De gamle 24 byer står først, og eksisterende nationsrækker og svenske ejergridceller er bevaret. Monarkiets grundkort, højde-/Features-filer, svenske bydata og svenske hærstyrker omskrives ikke. De to nye danske byer bruger Ratzeburgs eksisterende amt og det almindelige danske bykort; de ændrer ikke amtets startbefolkning eller reserve.

### Datakilde og regenerering

`Tools/Map1851/south_seed.py` indeholder de eksplicitte startskøn og skriver `south.json`. `export_south.py` fletter den sydlige kilde ind i den eksisterende eksport. Normal eksport læser `south.json`; en ny kørsel af seed-scriptet overskriver manuelle ændringer i denne fil. Seed-scriptet tilføjer kun en manglende `MST`-nationsrække til scenarierne; det ændrer ingen eksisterende nationsrækker.

```powershell
python Tools/Map1851/south_seed.py
python Tools/Map1851/export_neighbours.py
python Tools/Map1851/validate_neighbours.py
```

Eksporten skriver `cities_neighbours_<år>.json`, `Army_Neighbours_<år>.json`, `Economy_South_<år>.json`, ejergrid og `South_Districts.bin`. Økonomifilerne er en læsbar eksport af samme distriktsdata som runtime indlæser fra byfilen, ikke et ekstra dansk budget. Köslin ligger uden for kortudsnittet og findes i `outside`; byen flyttes ikke ind på kortet. Berlin ligger ligeledes udenfor; banerne repræsenterer kun deres faktiske nordlige kortudsnit.

### Distrikter, befolkning og økonomi

- Mecklenburg-Schwerin: 16 **spildistrikter** omkring navngivne Domanialämter og byområder. Mindre amter er grupperet; det er ikke en fuldstændig afskrift af den historiske amtsinddeling. Ritterschaft repræsenteres som en skønnet andel på 40 % af landbefolkningen i hvert distrikt, ikke som ét opdigtet sammenhængende territorium. Domanial- og riddergodsbefolkning er delmængder af landbefolkningen og lægges ikke til igen.
- Mecklenburg-Strelitz (`MST`) er en særskilt stat. Neustrelitz, Neubrandenburg og Friedland hører til Herrschaft Stargard. Den vestlige Ratzeburg-landdel og dens historiske enklaver er endnu ikke optegnet; Ratzeburg by forbliver dansk.
- Preussen: Stadtkreis Stralsund, Franzburg, Greifswald, Rügen; Stettin/Randow, Demmin, Anklam, Usedom-Wollin, Ueckermünde, Saatzig; Prenzlau og Angermünde under Potsdam/Brandenburg; Fürstenthum ved Kolberg under Köslin. Sidstnævnte og Brandenburg er kortudsnit, ikke hele regeringsdistrikter. **Slesvig og Holsten er ikke preussiske i disse to scenarier**.
- Hamborg/Bergedorf, Lübeck og kortudsnit af Landdrosteien Lüneburg og Stade har samme økonomi-/reserveskema.

Distriktsgrid: little-endian uint16, 768 × 787, nordlig første række; numre følger `districtGrid.ids`. Grænser er **nearest-seat-skøn klippet af periodens ejergrid**, med autoritativ distriktstilknytning i navngivne bycentre. Dette er synlig administrativ spilgeometri, **ikke verificerede historiske amts-/kreisgrænser**. Bykortet viser distrikt og Regierungsbezirk; distriktsgrænser tegnes i KONTROL-visningen.

Alle nye befolkninger og økonomital er markeret `estimated`. Distriktsbefolkningen inkluderer byerne; `rural = population - urban`. Årlig byskat er 0,36 rd./indbygger og landskat 0,16 rd./indbygger. Korn 0,38 og kvæg 0,24 pr. landindbygger følger den eksisterende 1825-økonomis sammenlignelige spilregneenheder; det er ikke verificerede tyske skatte- eller produktionsregnskaber. Lokale startmidler svarer til tre års modelskat, og den løbende skat går til lokal rekruttering og magasinindkøb. Ingen udgift belaster den danske statskasse.

### Reserver, mobilisering og forsyning

Samme beregningsmodel som dansk `GetAmtManpower` / `YearlyClass`: en distriktsreserve, tilførsel af en tolvtedel af årgangen pr. måned, loft, og betaling for erstatningsmandskab. De udenlandske distrikter ligger i en særskilt container, så danske amtindeks, danske budgetsummer og gamle `AmtManpower`-saves ikke forskydes. Startreserve er 3 % i Preussen (Landwehr-skøn), 2 % hos de øvrige; årgang 0,9 %, loft 9 %, udrustning 10 rd./mand. Disse er modelantagelser, ikke samtidige udskrivningsprocenter. Preussen har almindelig værnepligt fra 1814 med linje/reserve/Landwehr; det gamle kantonsystem behandles ikke som uændret gældende i 1825. Mecklenburgsk lodtrækning, undtagelser og stedfortrædere er forenklet til samme årgangsmodel.

Mobilisering bruger den hjemlige reserve først og derefter kun samme nations distrikter med en tilladt vejforbindelse til regimentets garnisonsby. Forstærkninger kræver fortsat en tilladt forbindelse til hjemmet og højst regimentets krigsstyrke; nye infanterirekrutter skaber ikke kanoner. Udryddede mobiliserede regimenter genopstår ikke ved load. Mobiliseringsforsøg er begrænset til ét pr. kampagnedøgn.

Regimenter bruger eksisterende `EnemyCorps`, krigs-AI, opklaring, slag, march og gemte tab. Infanteri går efter dansk 20 km/døgn, rytteri 30 og fodartilleri 18; eksisterende chaussé-/terræn-/vejregler anvendes. Fæstningsgarnisoner holder deres by. Nærmeste **tilgængelige** egne magasin leverer rationer/foder gennem det eksisterende vejnet, og ikke over vand eller neutrale grænser som en afstandsgenvej. Magasiner dimensioneres efter garnison og feltregimenter hver for sig. Soldater har fire døgns rationer og heste to døgns foder; mangel standser marchen, og fødemangel koster skønnet 0,1 % mandskab/døgn. Rationer/foder og distriktsmidler er begrænsede og gemmes. Genopfyldning er delvis, hvis den lokale kasse ikke rækker til hele indkøbet.

Dette er en forbindelse-/lagerbaseret udenlandsk forsyningsmodel: udenlandske fysiske trænkolonner, leveringstid, ammunition brugt i slag og detaljeret syge-/moralsimulation er ikke ført op på det danske lag endnu. Allierede magasiner kan bruges efter eksisterende allianceregler, og passage til Danmark er gensidig. Mecklenburg og Strelitz bliver **ikke automatisk fjender**, fordi Preussen mobiliserer. Det eksisterende krigssystem understøtter stadig ikke vilkårlige krige mellem alle nationspar eller selvstændige allierede kampordrer.

### Byer, baner og kamporden

Der er nu bl.a. Güstrow, Parchim, Ludwigslust, Waren, Neustrelitz, Neubrandenburg, Boizenburg, Hagenow, Grabow, Bützow, Schwaan, Kleinen, Sternberg, Crivitz, Gadebusch, Grevesmühlen, Ribnitz, Teterow, Malchin, Anklam, Swinemünde, Stargard, Pasewalk, Demmin, Ueckermünde, Wolgast, Barth, Grimmen, Franzburg, Putbus, Garz, Sassnitz, Gartz, Prenzlau, Schwedt, Angermünde, Passow, Tantow og Kolberg samt Bergedorf/Lüneburg/Bleckede. Befolkninger er scenariespecifikke størrelsesskøn. Stralsund/Stettin bevarer deres fæstninger; Swinemünde, Kolberg og Dömitz får særskilte fæstningsgarnisoner, infanteri og artilleri. Fæstningsgeometri er fortsat en kampagnebonus/markør.

Vejnettet når Ratzeburg/Lauenburg og Altona/Holsten. Berlin–Stettin (1843) går i kortudsnittet via Angermünde–Passow–Tantow; Stettin–Stargard åbner 1846. Berlin–Hamborg (1846) går via Büchen–Boizenburg–Hagenow–Ludwigslust–Grabow. Hagenow–Schwerin åbner 1847, Schwerin–Wismar 1848, Kleinen–Bützow–Schwaan–Rostock og Bützow–Güstrow 1850. Den tidligere fejlagtige Rostock-bane via Wismar er erstattet. Lübeck–Büchen ligger i 1851-data, men åbner først 15. oktober; Büchen–Lauenburg sættes konservativt til 1853 og er ikke en åben 1851-bane. Der er **ingen baner i 1825**. Fremtidige stationsår er metadata og er ikke automatisk et færdigt banenettet. Den skønnede Hamborg–Berlin chaussé åbner 1829.

Kampordenen har pr. scenarie otte preussiske regiment-/bataljons-/artillerigrupper og tre mecklenburgske grupper med navngivne kompagnier, eskadroner eller batterier, fredsstyrke, krigsstyrke, garnisonsby og rekrutteringsdistrikt. Preussisk modelmål er 18.000 mand i 1825 og 24.000 i 1851; Mecklenburg 3.000. Preussisk fredsfremmøde er 7.000/7.800; Mecklenburg 1.620. Fæstningspersonel tælles separat. IR 2/9/14/21, Greifswalds 2. jægere, Pasewalks Königin-kürassierer, 2. artilleribrigades feltdetachement og samlet Landwehr er **en foreløbig pommersk rekonstruktion, ikke en dokumenteret fuldstændig II.-korps-kamporden**. Divisionsplacering, garnisonering af linjeregimenterne og styrker skal stadig kontrolleres mod årgangenes rang-/kvarterlister. Mecklenburgs infanteri/chevaulegers-dragoner/artilleri er tilsvarende grupperede skøn. Ukendte chefer står tomme med `chiefStatus=unknown`.

Bykortene viser befolkning, distrikt/region, skat, reserve/årgang, opklaret garnison/feltstyrke, regimentnavne og underenhedsantal samt vej-/banestatus. Der gives ingen danske byggeordrer i udenlandske byer. Fredstidens regimentlabels forskydes ved fælles garnisonsby, så de ikke tegnes oven i hinanden.

`references1864` i kilden adskiller Gardekorps, III. korps og østrigsk VI. korps fra II. korps' lokale pommerske styrker. Det er **kun reference på kommandoniveau**: en fuld regimentvis 1864-offensiv-kamporden er endnu ikke implementeret. Der tilføjes intet tredje scenarie og ingen obligatorisk historisk krig/event. Eksisterende ekspeditions-events er bevaret; mulig dobbelttælling mellem nationale ekspeditionsstyrker og nye lokale regimenter skal afklares ved næste historiske OOB-pass.

### Gem/indlæs og kontrol

Save-version **34**. Eksisterende `UCampaign1851SaveGame.Neighbours` indeholder stadig `hær-id|korps-id`, plus `district|id|reserve|midler` og `army-supply|id|rationer|foder`. Korpsenes tab, position, rute og opklaring gemmes fortsat i `War`, magasiner i `Supply`. Alle nye vedvarende ressourcefelter har save/restore; distriktsgeometri og kamporden indlæses fra scenariet. Ny spilstart nulstiller distriktsressourcer; almindelig fred nulstiller ikke kasser/reserver.

De gamle `PR-pomerania` og `MEC-field` er tomme migrationsankre i nye spil, så de ikke lægges oven i regimenterne. Et v33-save med et af de gamle mobiliserede korps beholder dets korps-id/tab/rute; de nye underregimenter markeres som undertrykt (`-1`) indtil næste fred/ny spilstart. Markeringen gemmes også, så gentagne loads ikke frembringer ekstra hære. v33 uden mobilisering får den nye fredskamporden og standarddistrikter. Ældre saves initialiseres gennem eksisterende v33-fallback. Dette er statisk gennemgået; en faktisk Unreal-save-rundtur er ikke kørt.

Validatoren kontrollerer unikke/stabile identiteter, 1825/1851, ejere og distrikter i bycentre, hele distriktsgridets ejermaske, økonomisummer, reserver/årgange, underenhedssummer, fravær af dobbelte gamle aggregater, alle nye byers forbindelse til Ratzeburg, baneår og fravær af 1825-baner. Svenske startdata kontrolleres særskilt. Supplerende kontrol mod Git HEAD har bekræftet uændrede svenske ejergridceller og alle tidligere nationsrækker. Python-generatorernes determinisme, AST, C++-diff og UE 5.8-headere kontrolleres uden build/spilstart. Klik-kæden er ikke udvidet; ingen nye klikhandlere er nødvendige.

### Kilder, antagelser og det resterende

- [Metropolregion Hamburg: Schwerins jernbanehistorie](https://metropolregion.hamburg.de/natur-und-kultur-erleben/industriekultur-am-wasser/routen/schwerin/hauptbahnhof-eisenbahndirektion-11958) støtter årstallene 1847/1848/1850 og afgrænser Lübeck-forbindelsen til 1870; ikke befolkninger eller militærstyrker.
- [Museumsstiftung Post und Telekommunikation: første Berlin–Stettin-køreplan](https://onlinesammlung.museumsstiftung.de/detail/collection/0f3442a3-8efa-48eb-be81-352e39fa89ef) angiver åbningen til Stettin 15. august 1843.
- [Boizenburgs lokalhistoriske årstalsoversigt](https://www.ortschroniken-mv.de/index.php/Geschichte_Stadt_und_Amt_Boizenburg/Elbe_in_Daten_bis_1918) støtter chaussé 1826–1829 og banen i 1846. Bycentre bruges som stationsankre; linjeføringen er ikke en præcis stations-/matrikelrekonstruktion.
- [Gesellschaft für Schleswig-Holsteinische Geschichte, Mitteilungen 93](https://www.geschichte-s-h.de/wp-content/uploads/2017/11/Mitteilungen93.pdf) skelner Lübeck–Büchen 1851 fra Büchen–Lauenburg to år senere. Nogle oversigter sammenblander disse; derfor bruges 1853 for sidstnævnte.
- [Digitaliseret pommersk garnisonsoversigt](https://kpbc.umk.pl/Content/236544/PDF/Gromadzenie_POPC_018_01.pdf) understøtter Greifswalds jægergarnison fra 1821, betegnelsen bataljon fra 1848 og Königin-rytteriets tilknytning til Pasewalk. Den bruges ikke som belæg for alle regimenters placering eller mandskab.
- [Biblioteka Śląskas ranglistekatalog](https://sbc.org.pl/en/dlibra/publication/389394/kurzgefa-te-stamm-und-rangliste-aller-regimenter-der-koniglich-preu-ischen-armee-von-deren-stiftung-an-bis-ende) peger på primærmateriale til næste kontrol. Årgangenes fulde lister og Mecklenburgs Staatskalender er **ikke transskriberet** her. Ingen ny befolkning eller chef er præsenteret som et verificeret folketællings-/ranglistetal.

Der mangler således stadig fuld historisk amts-/godsmosaik og folketællingskontrol, komplet II.-korps- og mecklenburgsk OOB med chefer samt regimentvis 1864-reference, autonome allierede kampordrer/generelle nationskrige, kobling til hele nationens økonomi/private investeringer, udenlandske forsyningskolonner og ammunition/tab pr. kompagni, detaljeret fæstningsgeometri og spil-/save-/UI-afprøvning efter særskilt anmodning. Udvidelsen gør sydens lokaldata og militære ressourceforbrug langt mere detaljerede, men er ikke historisk eller systemmæssigt fuld paritet med Danmark endnu. Ingen build, editor/spilstart, skærmbilledtest eller commit er kørt.
