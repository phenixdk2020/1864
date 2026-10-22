# Kortfliser – format, maler v5 og genopbygning

V5 fortsætter genmalingen af originalrektanglets udenlandske land efter brugerens beslutning og retter kantvifter samt den forkerte Elbe-diagonal. Danmark, Slesvig, Holsten og Lauenborg bevares byteidentisk med en eksplicit monarkimaske. Kilde-PNG'er, `Denmark1851_Map.json`, gamle ejer-/distriktsgitre, by-id'er og rutekoordinater ændres ikke. De nuværende game-loadere kan fortsat læse de gamle filer; flisedata ligger ved siden af dem. Ingen Source-, Content-, portræt- eller officersbilledfiler indgår i denne rettelse.

## Genopbygning uden Unreal

Fra repoets rod med den eksisterende Python-installation:

```powershell
# Navnet er bevaret; scriptet producerer nu v5:
python Tools/Map1851/paint_tiles_v2.py --workers 2

# Komplet genopbygning af data og billeder:
python Tools/Map1851/build_tiles.py

# Alle ni kontroller og en frisk samlet rapport:
python Tools/Map1851/validate_tiles_all.py
```

Enkeltkontroller:

```powershell
python Tools/Map1851/validate_tiles.py
python Tools/Map1851/validate_dem_tiles.py
python Tools/Map1851/validate_tile_edges.py
python Tools/Map1851/validate_tile_painter.py
python Tools/Map1851/validate_neighbours.py
python Tools/Countries/validate_level_c.py
python Tools/Countries/validate_officer_careers.py
python Tools/Countries/validate_sweden_norway.py
python Tools/Countries/validate_prussia.py
```

Der bruges numpy, Pillow og scipy. Ingen pakker downloades eller installeres. Ingen Unreal-build, spil-/editorstart eller launcher er nødvendig. Der beregnes højst to fliser samtidig; `--workers 1` reducerer hukommelsesbehovet. Et helt højopløst verdenskort oprettes aldrig i hukommelsen. Originalkilderne, en maske for det gamle sydudsnit og oversigten kan ligge samlet i hukommelsen. Pillow kan dekode én 3600² float32-TIFF (49,4 MiB) for at tage et udsnit; alle seks TIFF'er caches ikke samlet dekodet.

Der gemmes checkpoints efter hver tiende flise. `painterV2InProgress` betyder uafsluttet arbejde og afvises af validatorerne. Filhashes for de fire malede lag og kode-/kildesignaturen kontrolleres før genbrug. En fuldført, uændret maling genbruges uden at gentage farvebalancen. Filer, der efter en afbrydelse ikke matcher checkpointets hashes, genmales.

## Format og koordinater

`Data/Campaign1851/MapTiles.json` er kontrakten til fliselæsningen. Stier er relative til repoets rod med `/`; store PNG'er ligger i `Reference/Campaign1851/MapTiles/`.

Projektionen er uændret sfærisk LAEA, centrum 52°N/10°E, radius 6371,0088 km. Origin er det oprindelige sydkorts nordvestlige hjørne. Flisernes x vokser mod øst, y mod syd; negative indeks er gyldige.

```
xMin = originKm.x + x * 1024 * kmPerPixel.x
xMax = originKm.x + (x+1) * 1024 * kmPerPixel.x
yMax = originKm.y - y * 1024 * kmPerPixel.y
yMin = originKm.y - (y+1) * 1024 * kmPerPixel.y
```

Pixelstørrelsen er x = 0,13636367182696663 km og y = 0,13629962444341387 km. Brug begge værdier: forskellen kommer fra den gamle afrunding af billedbredden. Originalpixels lander dermed præcist 1:1.

Den tæt samplede rand 4,5–31,5°E / 49–71,3°N projiceres, forenes med det gamle `extentKm` og udvides til hele fliser. Det nominelle gitter er 15×20 felter: 251 gemmes, 49 rene havfelter dækkes af oversigten. `gridRange` har eksklusive slutgrænser.

Originalernes størrelser er Color 3996×4096, Height 3996×8192, Features/Amter 1998×4096 og DetailMask 1998×2048. `legacySources[].preserveRect` og `offsetPixels` beskriver kildens sydudsnit og placering, men betyder fra v4 ikke, at hele rektanglet er beskyttet. Beskyttelsen bestemmes af `monarchyProtection`.

- Color: 1024² RGB PNG med sRGB-markering.
- Height: 1024² unsigned 16-bit PNG.
- Features: 512² RGB. Nyt land har R = land, G = skovtæthed, B = sø-/flodmaske. Beskyttede originalpixels beholder deres gamle værdier. Det genererede R-lag må derfor ikke bruges som en ny autoritativ monarkimaske.
- DetailMask: 512² RGBA; mark-/landbrugsmasken ligger i alpha.
- Amter, Amter_1825 og Amter_1851: 512² unsigned 16-bit. Gamle amtsindeks bevares i originaludsnittet; udenfor bruges `256 + districtGridFull.ids[id]` eller nul. Amter er alias for 1851. Nation og gameplaydistrikt afgøres af scenariets ejer-/distriktsgitter.

## Byteidentisk monarki og bevaring af hav

`monarchy_mask.py` læser **originalernes** Features, Amter og eksisterende DK-ejergitter. På halv opløsning bruges unionen af:

1. Features R > 0, inklusive eventuelle antialias-pixels.
2. Oprindeligt amts-id > 0, som tilhører regionerne 1/2/3: Kongeriget, Slesvig, Holsten og Lauenborg, jf. Regions-legenden i Denmark1851_Map.json.
3. DK i det gamle ejergitter, hvor originalens Height/Color viser land. Dette er en konservativ kantbeskyttelse.

Hver beskyttet halvpixel udvides til en 2×2-blok i fuld opløsning. Masken beskytter 783.850 halvpixels / 3.135.400 fulde pixels. Kontrolpunkter ved København, Slesvig, Kiel og Lauenborg ligger i masken. Maskens SHA-256, kildehashes, regioner og antal står i manifestets `monarchyProtection`. Der testes byteidentitet særskilt for Color, Height, Features, Amter og DetailMask; antallet rapporteres pr. lag.

Udenlandsk land uden for masken genmales også inde i det gamle rektangel. Oprindeligt hav identificeres med originalernes nulhøjde/vandfarve og beholdes. Kun en 64-pixelstribe **inden for originalrektanglets yderkant** får en glidende overgang til det nye hav for at fjerne den tidligere rektangelsøm. Monarkiets pixels har altid forrang og ændres heller ikke i denne stribe. Validatoren kontrollerer det øvrige oprindelige hav uafhængigt af monarkiet.

De eksisterende politiske streger inde i beskyttelsen bevares som pixels. V4 fjerner farve-/paletspringet på den nye udenlandske side; den omtegner ikke fx Kongeaaens oprindelige interne linje.

## Samme maler og palet over hele det fremmede land

`paint_tiles_v2.py` kalder `build_map.paint`, som også tegner Danmark og verdensarket. Den bruger den udæmpede danske landpalet, skovblanding, markernes Voronoi-mønstre, hede-/mosefarver, kystskygge, malerisk sløring og korn. Støj og markmønstre følger verdenskoordinater på sydkortets fysiske skala: 558,283 km / 4096 pixels, seed 1864. Hver flise har 128 pixels halo, der beskæres væk. Arkets rektangulære kantudtoning er slået fra.

V4 bruger samme støjskala og ETOPO-hillshade indenfor og udenfor det gamle rektangel. Den tidligere grovere teksturovergang og grå dæmpning omkring rektanglet er fjernet. Der foretages ingen kalibrering tilbage til originalens grå udenlandske farver.

På den udenlandske side af monarkiets kant blandes farven over 64 pixels (cirka 8,7 km) mod filtrerede originale monarkilandfarver. Farvede grænsestreger udelades fra farvereferencen. Kun ubeskyttet land påvirkes; kystgeometri og søer klones ikke fra den gamle geografi. Overgangen fortsætter derefter i den fælles maler og palet. En selvstændig kantkontrol måler farveafvigelsen fra den lokale monarkireference i de første otte udenlandske pixels.

`land_tone.py` summerer PNG-pixels flisevis. Originalt monarkiland er farvereference. Hele det genmalede land tæller med, inklusive oprindeligt udenlandsk land, skove, bjerge og overgangsbånd; monarki og oprindeligt hav udelades fra korrektionen. RGB-middelværdier må højst afvige 3/255 pr. kanal, gennemsnitlig HSV-mætning højst 0,06. Standardafvigelser rapporteres også. Balancen toner ind uden for monarkiets kant og fjerner ingen terræntekstur. Nye sorte kontrastklip afvises.

`validate_tile_painter.py` sammenligner standardmalerens fire outputlag med golden-hashes fra Git HEAD for to seeds og to scener, inklusive blandede regioner og et baggrundsark. De nye keyword-parametre ændrer ikke originalpipelineens standardkald.

## ETOPO-højder

De seks lokale ETOPO 2022 15″ Ice Surface GeoTIFF'er bruges via Pillow mode F. Georeference læses fra ModelTiepoint (33922) og ModelPixelScale (33550). Pixel-is-area håndteres ved sampling i pixelcentre med bilineær interpolation, også over TIFF-kanter. N60E015 dækker eksempelvis 15–30°E / 45–60°N; filnavnets breddegrad er nordkanten.

Kilde: **NOAA NCEI, ETOPO 2022 Ice Surface**, DOI **10.25921/fd45-gt74**. TIFF'erne er lokale, git-ignorerede og indgår ikke i eksporten. Kildestier og SHA-256 findes under `painterV2.etopoSources`.

```
uint16 = round(clamp(meter, 0, 3000) / 3000 * 65535)
meter  = uint16 / 65535 * 3000
```

Dette passer til HeightRangeKm = 3. Vand skrives som nul. Kvantiseringstrinnet er cirka 4,58 cm. Monarkiets oprindelige kunstneriske højder kopieres råt og ændres ikke. Alt andet genmalet land bruger DEM-højder, også inde i originalrektanglet. Overgangen mellem kunstneriske danske højder og fysiske DEM-højder kan kræve særlig behandling af mesh/normals på Unreal-siden; farveovergangen ændrer ikke de beskyttede højder.

DEM dækker 0–45°E / 45–75°N og dermed hele det bestilte geografiske område. LAEA-rektanglets ekstra hjørner går udenfor; der forlænges nærmeste målte DEM-kant. Antallet findes under `heightEncoding.edgeExtendedLandPixels` og pr. flise. Det er en markeret begrænsning. En tydeligt markeret skønnet fallback findes i koden, hvis TIFF'erne fjernes; den bruges ikke ved denne leverance.

## Vandgeometri og skøn

Lokale Natural Earth-søer/floder bruges, hvis de findes. De var ikke til stede ved denne leverance. Søkonturer udledes derfor af ETOPO's flade vandspejl omkring navngivne søgeområder som Vänern, Vättern, Mälaren og Mjøsa. Søgepolygonerne males ikke som søbredder. Metoden kan mangle smalle forbindelser og er ikke verificeret hydrologi; niveauer/resultater står i `painterV2.hydro.lakeExtraction`.

Glomma, Rhinen, Elben, Oder og øvrige referencefloder er fortsat skønnede polylinjer. Skov, marker, hede og moser er visuelle skøn, ikke historiske arealanvendelsesdata. Vandmaskerne ændrer ikke gameplaygitre eller ruter og er ikke et nyt valideret navigationsgrundlag.

## Scenarier og gameplaydata

Begge scenarier har egne `cities_neighbours_full_1825.json` / `_1851.json`, ejer- og distriktsgitre. De oprindelige scenarier har kun de nye topnøgler `mapTiles`, `fullScenario`, `ownerGridFull` og `districtGridFull`; gammel indlæsning fungerer fortsat.

Fulde gitre er 2951×3932 ved præcis 0,71 km/celle. Rækker går fra nord mod syd uden header. Ejergitre er uint8 (11.603.332 bytes pr. scenarie), distrikter little-endian uint16 (23.206.664 bytes). Brug hvert gitters egne dimensioner/extent; randafrunding kan være under én celle. Nul betyder hav/intet udenlandsk distrikt; danske amter ligger fortsat i Amter-laget og gamle data.

Alle nye researchtal er `populationStatus: "skøn"`, jf. Kortudvidelse-Research-2026-10-20.md. Nye tal overskriver ikke gamle rækker. Norske amts-/preussiske distriktsfordelinger er provisoriske; ukendte sædebytal er dokumenterede nulværdier. Ingen navngivne personer oprettes. Ostpreußen og Westpreußen er separate provinser i 1825 og samlet som Preussen i 1851; Hohenzollern er scenarieafhængigt. Grænser, enklaver og nærmeste-sæde-distrikter er omtrentlige. A*-ruter er spilskøn med eksplicit fremmed transit eller bådforbindelse, ikke verificerede historiske postruter.

## Oversigt, previews og validering

Overview_Color.png og Overview_Height.png er 2095×2792, cirka 1 km/pixel, sammensat fra fliserne. Havbaggrunden males også med samme palet og koordinatstøj; udeladte havfelter giver ikke konstante blokke.

V5-previews i Saved/Codex:

- overview_preview_v5.png, højst 900 pixels højt.
- tile_crop_kongeaa_v5.png, Kongeåen (beskyttet indre monarkigrænse), 1024² i oprindelig opløsning.
- tile_crop_northgermany_v5.png, Holsten/Mecklenburg, 1024² i oprindelig opløsning.
- tile_crop_lauenburg_v5.png, Elben/Lauenborg, 1024² i oprindelig opløsning.

Validatorerne kontrollerer kilde-/filhashes, monarkiets pixelidentitet pr. lag, bevaring af oprindeligt hav, faktisk genmaling af fremmed land, regional farvebalance, 4-pixelstriber (cirka 0,55 km) på begge sider af det gamle rektangel, alle naboflisepar, monarkiets farveovergang, DEM-samples, landdækning samt scenariernes gamle id'er/ruter og nye gitre.

MapTiles_Validation.json indeholder dato, manifesthash, faktisk PNG-diskstørrelse, pixelantal, målinger og alle ni exitkoder/stdout. Den samlede kontrol kører højst to uafhængige validatorer samtidig. En ny OK-rapport skrives kun, når alle består, og manifestet er uændret under hele kontrollen. En gammel rapport kan ligge tilbage efter afbrudt genmaling; sammenhold derfor altid dens manifesthash med det aktuelle manifest.

Ændrede værktøjer i v4: monarchy_mask.py (ny), paint_tiles_v2.py, land_tone.py, tile_seams.py, overview_background.py, validate_tiles.py, validate_dem_tiles.py, validate_tile_edges.py og validate_tiles_all.py. Desuden opdateres manifest/rapport, genererede flise-/oversigts-PNG'er, de tre v4-previews og denne dokumentation. `finalize_v4_provenance.py` dokumenterer engangsovergangen mellem render- og kontrolfaser uden at ændre PNG-filer. Gamle inputfiler, byer, ruter, scenarieindhold og gameplaygitre ændres ikke af v4-genmalingen.

Runtime, LOD, højdesømme, import og streaming kræver særskilt Unreal-kontrol. Der er ikke kørt build, spil, editor eller launcher.

Sømtesten beholder tolerancen 3/255 for middelværdi og standardafvigelse ved selve sammenføjningen samt en særskilt direkte pixelkantkontrol. Til sammenligning gemmes også 16-pixelbånd som diagnostik: forskellige naturlige skov-/terrænpletter over 2,2 km kan give større fordelingsforskel uden et brud ved kanten. På vestkanten er den direkte nabostribes standardafvigelsesforskel cirka 0,37/255, mens 16-pixelbåndene er cirka 5,22/255 fra hinanden. De bredere bånd bruges ikke til at udjævne eller ødelægge reelt terræn. Regional farvebalance kontrolleres fortsat over hele det genmalede land.

## Afsluttende v4-kontrol, 21. oktober 2026

Alle ni validatorer er bestået: validate_tiles, validate_dem_tiles, validate_tile_edges, validate_tile_painter, validate_neighbours, validate_level_c, validate_officer_careers, validate_sweden_norway og validate_prussia. Rapporten er MapTiles_Validation.json og er bundet til det aktuelle manifests SHA256.

251 fliser og to oversigter omfatter 1.759 PNG-filer på 509.979.872 bytes (486,35 MiB). Gitrene er fortsat 2951×3932 ved 0,71 km/celle for begge scenarier. Monarkiet er byte-identisk i 3.135.400 Color-pixels, 3.135.400 Height-pixels samt 783.850 pixels i hvert af Features, Amter og DetailMask: i alt 8.622.350 lagvise pixelkontroller. Desuden er 6.509.371 oprindelige indre havpixels bevaret. Alle 6.259.994 oprindelige fremmede landpixels er genmalet.

Den regionale RGB-middelforskel er højst 0,485/255 mod tolerancen 3/255; mætningsforskellen er 0,0061 mod tolerancen 0,06. Ingen nye landpixels er sorte placeholderpixels. Alle 466 naboflisepar består kantkontrollen; monarkiets udvendige 8-pixelbånd har største kanalvise middelforskel 0,438/255 mod den filtrerede oprindelige landfarve. 50.930 DEM-landprøver matcher ETOPO-enkodningen med maksimal uint16-fejl 0; 853 af prøverne ligger i den dokumenterede ydre DEM-kantforlængelse. Golden-kontrollen bekræfter det oprindelige danske painter-standardoutput byte-identisk for 2 seeds × 2 scener × 4 lag.

## V5: fjernelse af kantvifter og forkert Elbe-diagonal

V4-kantblandingen projicerede filtrerede monarkifarver ud fra nærmeste gyldige pixel. Ved konkave hjørner skiftede dette nærmeste punkt brat, hvilket trak brune farvevifter ud på fremmed land. V5 bruger i stedet normaliseret Gauss-konvolution af gyldigt monarkiland (sigma 24 halvpixels = 48 fuldpixels). Det giver et kontinuerligt farvefelt uden Voronoi-/nærmeste-punkt-skillelinjer. Afstandsfeltet glattes med sigma 1,5 halvpixels før den 64-pixel brede smoothstep-feather. 75 % af malerens lokale højfrekvente tekstur (rest efter sigma 6 pixels) bevares i farvereferencen, så overgangen ikke udvisker markernes korn. Der bruges ingen radiale farveopslag. Monarkipixels kopieres stadig til sidst og er uændrede.

Den tynde blå diagonal i v4-croppet kom fra Elbens fallback-polyline: segmentet fra (11,8°E, 53,0°N) til (10,0°E, 53,55°N) skar lige gennem landet. Det var en for grov flodguide, ikke en flisesøm eller en vej. Den er erstattet med flere geografiske støttepunkter via Wittenberge, Dömitz, Lauenborg og Hamburg. Forløbet er fortsat et eksplicit hydrologisk skøn, ikke et verificeret vandløbsdatasæt; lokale Natural Earth-vandløb har fortsat prioritet, hvis de findes. Oprindelige vand-/grænsestreger i den beskyttede monarkimaske ændres ikke.

`border_artifacts.py` indgår i validate_tile_edges og dermed i alle-ni-kørslen. Den gennemgår alle fliser, der møder monarkiets ydre 128-pixelbånd, og søger efter tynde mørke/blå, lange rette komponenter, som ikke forklares af originalt vand, Natural Earths havmaske eller de aktuelle flodguider. Den stoler bevidst ikke på genereret Features.B som fritagelse: en forkert hydrolinje ville ellers godkende sig selv. Den kontrollerer også farvefeltets største naboderivat i featherbåndet (højst 1 farveenhed pr. fuldpixel) for at fange spring og vifteprojektion. Negative kontroller injicerer en blå linje og et trekantet farvespring; linjen accepteres først, når den er autoriseret hydro. Den konkrete v4-diagonal afvises også af detektoren. Kontrollen er en målrettet regression, ikke en garanti mod enhver tænkelig billedartefakt.

Områderne omfatter Elbe/Lauenborg, Trave/Mecklenburg, Holsten mod Hamburg/Lübeck samt Fehmarnsund. Kongeåen ligger mellem to beskyttede monarkiregioner: her sikres originalens pixels frem for en ny feather. V5 ændrer monarchy_mask.py, paint_tiles_v2.py, tile_seams.py, validate_tiles.py og validate_tile_edges.py samt tilføjer border_artifacts.py; manifest, rapport, genererede billeder og denne dokumentation opdateres.

Den faktiske farvefeather kontrolleres også i PNG-billederne: sammenhængende brede, brun/røddominerede pletter (R−G > 8 efter sigma 2-sløring, mindst 64 pixels) i det ydre 8–64-pixelbånd flagges som mulige vifter. Dette fanger den konkrete v4-vifte på 993 pixels ved Holsten/Mecklenburg og accepterer v5-kontrolflisen. Konservative afstande til originale vand-/monarkipixels udelukker uændrede politiske streger. Detektoren er målrettet den aktuelle grøn/gule palet; fremtidige bevidste brune terræntyper tæt på grænsen skal vurderes særskilt.

Da v5 beholder markernes fine tekstur, sammenligner 8-pixel-kantkontrollen middel-RGB med det glatte farvefelt, både samlet og pr. flise med mindst 256 prøver; tolerancen er fortsat 3/255. Absolut fejl pr. pixel rapporteres stadig, men indeholder nu det bevidst bevarede korn. Linje-/viftekontrollerne og pixelidentiteten er separate og kan ikke udlignes af middelværdien.

Alle 12 berørte grænsefliser er desuden samlet og visuelt gennemgået i Saved/Codex/monarchy_borders_v5.png (1024×840). De tre ønskede nærbilleder er 1024×1024 i oprindelig fliseopløsning; overview_preview_v5.png er 675×900. Et midlertidigt renderer-kontrolbillede blev fjernet efter gennemgangen.

## Afsluttende v5-kontrol, 21. oktober 2026

Alle ni ønskede validatorer er grønne på det færdige v5-manifest. Den friske MapTiles_Validation.json indeholder deres exitkoder/stdout og det aktuelle manifests SHA256. De 251 fliser og to oversigter fylder 509.998.373 bytes fordelt på 1.759 PNG'er (486,37 MiB). Scenariegitrene er uændret 2951×3932 ved 0,71 km/celle.

Monarkiets 3.135.400 Color-pixels og samme antal Height-pixels er byte-identiske; Features, Amter og DetailMask har hver 783.850 identiske beskyttede pixels. Det er 8.622.350 lagvise pixelkontroller. Desuden er 6.509.371 oprindelige indre havpixels uændrede. Alle 6.259.994 oprindelige fremmede landpixels er genmalet.

Alle 12 grænsefliser består artefaktkontrollen på 251.139 fremmede grænsepixels: nul uunderstøttede rette linjer og nul brune viftekomponenter. Farvefeltets største naboderivat er 0,6321/255 pr. fuldpixel mod grænsen 1/255. Alle 466 naboflisepar består. Det ydre 8-pixelbånd har samlet middel-RGB-fejl højst 0,235/255; det teksturbevarende absolutte pixelmål rapporteres særskilt. Meget små/coastale delprøver vises diagnostisk og bruges ikke som et selvstændigt farveestimat.

Den regionale RGB-middelforskel over hele det genmalede land er højst 0,483/255 mod tolerancen 3/255; mætningsforskellen er cirka 0,0061 mod tolerancen 0,06. Ingen sorte land-placeholderpixels. 50.930 landhøjder matcher ETOPO-enkodningen med maksimal uint16-fejl 0, og den danske standardpainters golden-output er fortsat byte-identisk for 2 seeds × 2 scener × 4 lag. Byer, ruter, distrikts-id'er og begge scenariers eksisterende data består de fem øvrige validatorer. Ingen Unreal-build, spil-/editorstart, .bat, download eller commit er kørt.

## Codex 21: runtime, LOD og import (21. oktober 2026)

`Campaign1851MapTiles.cpp` l?ser manifestets landfliser og bygger selvst?ndige `UStaticMeshComponent`-meshes med tre LOD'er: 257?257, 129?129 og 65?65 terr?npunkter (cirka fuld, 1/4 og 1/16). UE 5.8's renderer v?lger LOD efter sk?rmst?rrelse og frustum-/occlusion-culler hver komponent. Sk?rter ned til under oversigten lukker mellemrum mellem forskellige LOD'er. Terr?nets h?jdeskala er fortsat 3 km, og Color-kilderne ?ndres ikke.

Oversigten vises f?rst. De 24 fliser n?rmest kameraets m?l holdes som meshes; h?jst ?n flise med tre LOD'er bygges pr. tick. Resten af kortet og udeladte havfliser d?kkes af oversigtsarket. Det er fordeling mellem frames, ikke en garanteret millisekundgr?nse: selve meshbygningen foreg?r stadig p? game thread. Ved panorering frigives fjernere komponenter og deres teksturhandles; igangv?rende requests afbrydes. GPU-/GC-hukommelse kan f?rst blive frigivet senere af engine.

`FStreamableManager` indl?ser Color og DetailMask asynkront fra `/Game/Campaign1851/MapTiles/T_Tile_<x>_<y>_<Layer>`. Hver aktiv flise har en dynamisk materialinstans af `M_Campaign1851Tile`; indtil begge teksturer er klar, samples oversigten med geografiske UV'er. Der benyttes ingen NorthLand/NorthSea-grupper i flisevejen. Den gamle Y-skala bruges kun i den separate UV-kanal til eksisterende markdetaljer/amtsmarkering, s? det danske shaderudtryk bevares; Color/DetailMask bruger almindelige flise-UV'er. Hjemmevisningen er fortsat 55,55 N / 10,45 E ved 960 km. Navigationsgr?nserne kommer fra manifestet, og maks-zoom er 2.500 km.

### Runtimeformat og genskabelse

K?r uden Unreal:

```powershell
python -B Tools/Map1851/build_runtime_tiles.py
python -B Tools/Map1851/validate_runtime_tiles.py
python -B Tools/Campaign/validate_map_tile_api.py
python -B Tools/Map1851/validate_tiles_all.py
```

`MapTileHeights/Tile_<x>_<y>.bin` har ingen header: f?rst 257? little-endian uint16-h?jder, derefter 65? uint8 skovt?thed. Begge felter g?r fra nord mod syd og inkluderer randen. En fil er 136.323 bytes; 251 filer er 34.217.073 bytes (32,63 MiB) plus det lille `Index.json`. Codex 21's nye runtime-data ligger dermed under 80 MB; Codex 20's eksisterende fulde scenariegitre er ikke kopieret. Ingen store PNG'er flyttes til Data eller pakkes som runtime-input.

Der samples biline?rt mellem PNG-pixelcentre ved f?lles globale koordinater, ogs? p? flisekanter. Manglende havnaboer indg?r ikke i v?gtsummen. `Index.json` binder filhashes til manifestets SHA256. Generatoren afviser et manifest under genmaling eller et manifest, der ?ndres under genereringen. Validatoren sammenligner alle 466 nabopars h?jde-/skovrande, f?lles LOD-randpunkter og 574.790 uafh?ngige kildesamples. CPU-h?jdeopslag bruger samme LOD0-trekantdeling uanset mesh-cachens indhold.

### Ejer-/distriktsgitre og danske CPU-data

N?r manifest og gyldige fulde scenariefiler findes, bruges `cities_neighbours_full_<scenario>.json` med `ownerGridFull` og `districtGridFull`, inklusive hvert grids egen fil, dimensioner og extent. Gr?nsen er 4096?4096. Begge aktuelle scenarier bruger 2951?3932. Ellers bruges de gamle scenarier og bin?rfiler. `OwnerUvFromKm` er den f?lles koordinatvej for territorie- og monarkiopslag. Udenlandsk land/hav bestemmes af ejergridet, ikke af danske h?jder uden for deres udsnit.

De eksisterende Features/Amter og koordinater beholdes for monarkiet. Udenlandsk skov kommer fra det kompakte 65?65-felt. `AmtAtWorld` returnerer fortsat kun danske amts-id'er; udenlandske distrikts-id'er blandes ikke ind i det danske id-rum. Farm-, Features- og hydro-l?kker forbliver bundet til det gamle udsnit og vokser derfor ikke med manifestet. Nationale/distriktsgr?nser scannes i f?lles line?re genneml?b med sammenlagte, kolline?re forl?b; der foretages ikke et helt rasterscan pr. nation. Det fulde politiske farvelag samples groft pr. 4?4 celler for at begr?nse meshst?rrelsen. Gameplayopslag og gr?nseudtr?k beholder gridets fulde opl?sning.

### Unreal-import: skrevet, ikke k?rt

`Content/Python/import_map_tiles.py` er den nye importvej. Foruds?tninger: UE 5.8, Python Editor Script Plugin, Editor Scripting Utilities, AssetTools, de eksisterende danske kampagneteksturer og `MPC_Campaign1851Season`. Virtual Texture Support skal v?re aktiveret i projektet, og editoren genstartet efter en ?ndring af den indstilling. Scriptet kontrollerer `r.VirtualTextures` og de gamle assets f?r import.

N?r editoren er lukket, kan projektejeren senere k?re:

```powershell
& 'I:/Spil/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'C:/Projekter/1864-Campaign/Game1864.uproject' -run=pythonscript -script='C:/Projekter/1864-Campaign/Content/Python/import_map_tiles.py' -unattended -nop4
```

Dette er kun en vejledning; kommandoen er ikke k?rt. Importen bruger `TextureFactory`/`AssetImportTask`, sRGB for Color, line?r DetailMask, mips og `virtual_texture_streaming`. Kun de to GPU-lag og Color-oversigten importeres; Height/Features/Amter bruges p? CPU og duplikeres ikke som ubrugte GPU-assets. Oversigten p? 2095?2792 pads til power-of-two af hensyn til VT; shaderens UV-skala kompenserer for padding.

Scriptet opretter flisemaster, oversigtsmateriale og 251 konstante materialinstanser. Det genbruger kun navngivne shaderfunktioner/konstanter fra `setup_campaign1851.py` via AST; det k?rer ikke setup-scriptets import-, level- eller assetoprydning. Eksisterende danske materialer omskrives ikke. En `PAL_MapTiles` med `AlwaysCook` omfatter mappen og begge nye materialer, s? konstruerede soft paths kan komme med ved cook. Kontroll?r, at projektets Asset Manager fortsat scanner `PrimaryAssetLabel` (UE-standard), og at `Data/Campaign1851` stages som l?se Non-UFS-filer ved en senere pakning; C++ l?ser disse via filsystemet. Config er ikke ?ndret i denne opgave.

### Fallback, saves og kontrolstatus

Manglende `MapTiles.json` bevarer det gamle terr?nmesh, nordmaterialerne, verdensarket, koordinaterne og de gamle scenariegitre. Manglende nye materialer eller manglende/fejlst?rrelse runtimeh?jder giver ogs? det gamle terr?n; gyldige fulde gameplaygitre kan stadig benyttes, n?r manifestet findes. En enkelt manglende flisetekstur efter import lader den p?g?ldende flise beholde oversigten. Maks-zoom er 2.500 km ogs? med fallback, men hjemmevisningen er u?ndret.

Ingen nye gemte felter: versionen forbliver 40. Flisernes geometri, cache og streaming er transient pr?sentation; de gemmes ikke. Eksisterende by-/distrikts-id'er og gamle save-migrationer bevares.

Alle ni Codex 20-validatorer og runtime-validatoren er best?et for manifest `d9b533bbe9440135f74924de0c0b94bf548f82b1616d2e596e448fa7eead4e6c`. Rapporter: `MapTiles_Validation.json`, `MapTileRuntime_Validation.json` og `MapTileApi_Validation.json`. API-validatoren kontrollerer nye kald/includes mod lokale UE 5.8-headere samt Python-syntaks/CRLF; den er ikke en C++-kompilering eller en Unreal-Python-test.

Import, shaderkompilering, VT-residency, FPS, framebudget, LOD-overgange, danske n?rbilleder og faktisk save/load skal stadig verificeres i Unreal af projektejeren. Loggen m?ler tidspunktet, hvor f?rste n?rflise indsendes, ikke GPU'ens faktiske presentationstid. Ingen build, spil/editor, .bat, download eller commit er k?rt af Codex 21.
