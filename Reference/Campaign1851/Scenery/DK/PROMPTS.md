# Danske scenery-bygninger – billedprompts

Dato: 2026-10-21. Kildegrundlag: Docs/Bygninger-DK-Research-2026-10-21.md. Generiske periodetyper ca. 1830–1850; ingen navngivne bygninger er anvendt som reference. Billederne er modelleringsreferencer, ikke opmålinger.

## Fælles prompt

```text
Use case: historical-scene. Asset: isolated architectural modelling reference for PROJECT 1864 campaign scenery, ordinary Denmark c.1830–1850. Generate ONE generic period-typical building only (farm is one connected four-wing complex), never reproduce any real named building. 1024x1024 PNG square. Clean readable architectural forms, restrained realistic materials suitable for later low-poly interpretation. Plain uniform light grey background and ground, soft even studio light, faint contact shadow, whole silhouette with generous margins. No people, animals, labels, text, dimensions, border, collage, landscape, lawns, neighbouring buildings, modern shop glazing, cast iron balconies, corrugated cement, modern machinery or steel mill sails. Palette: dusty cream limewhite, muted ochre, red-orange clay tile, grey-brown thatch, tar black, subdued dark green/oxblood doors. 
```

## Kameraprompts

Iso:
```text
View: orthographic-like oblique strategy map view, camera 38 degrees above horizontal ground and 45 degrees around building; show roof and two sides clearly. 
```

Side:
```text
View: strictly straight-on orthographic SIDE ELEVATION at mid-wall height, horizontal ground line, no perspective, no roof-top view and no visible receding wall. Show long principal facade (farm exterior long dwelling wing; church longitudinal nave and tower profile; mill sail-facing elevation). 
```

Den fulde prompt er fælles prompt + kameraprompt + typeprompt. Sidevisninger skal bruge den tilsvarende Iso som visuel reference, med besked om at bevare bygningens materialer og hovedform.

## Typer og filnavne

### Farm_Firlaenget

Filer: `Farm_Firlaenget_Iso.png`, `Farm_Firlaenget_Side.png`.

```text
Four-wing Danish farm: four connected single-storey half-timbered wings enclosing a fully closed rectangular cobbled courtyard, black tarred posts and cream limewash infill, steep grey-brown thatched roofs, low walls, modest domestic windows in dwelling wing and barn doors in others, integrated carriage passage. Small dunghill and simple well inside yard. No detached buildings.
```

### Cottage_Husmand

Filer: `Cottage_Husmand_Iso.png`, `Cottage_Husmand_Side.png`.

```text
Modest single-storey half-timbered long cottage, whitewashed posts and infill, half-hipped grey-brown thatched roof, small six-pane windows, one central masonry chimney, one dark green plank door. Approximate 10m length.
```

### TownHouse_Ochre

Filer: `TownHouse_Ochre_Iso.png`, `TownHouse_Ochre_Side.png`.

```text
Ordinary compact two-storey Danish market-town house, ochre-yellow limewashed plaster facade, red-orange clay pantile gabled roof, white small-pane windows, dark green door, simple eaves, modest chimney, no ornament.
```

### TownHouse_White

Filer: `TownHouse_White_Iso.png`, `TownHouse_White_Side.png`.

```text
Ordinary single-storey Danish market-town house, dusty cream-white limewashed masonry, red-orange clay pantile gabled roof, small white six-pane windows, muted oxblood plank door, one chimney, simple eaves.
```

### TownHouse_Timber

Filer: `TownHouse_Timber_Iso.png`, `TownHouse_Timber_Side.png`.

```text
Ordinary two-storey East Jutland market-town half-timbered house, tar-black structural posts and beams with muted ochre-yellow limewashed infill, simple regular timber bays without elaborate German ornament, white small-pane windows, oxblood door, red-orange clay pantile gabled roof.
```

### MerchantHouse

Filer: `MerchantHouse_Iso.png`, `MerchantHouse_Side.png`.

```text
Generic Danish merchant house c.1840, exactly two storeys plus attic, pale yellow plaster facade, red clay pantile gabled roof, broad central carriage gateway through ground floor, small-pane white windows, single central roof loading hatch with timber hoist beam, shuttered loading opening. Single street building only, no warehouse wings.
```

### Church_Village

Filer: `Church_Village_Iso.png`, `Church_Village_Side.png`.

```text
Generic Danish village church as it stood c.1840: small Romanesque nave and lower narrower chancel, whitewashed masonry, modest small round-arched nave windows, attached square medieval west tower with stepped gables, simple red clay tile roofs. Clearly medieval massing, no neo-Gothic revival details, no spire, no cemetery or boundary wall.
```

### Windmill_Dutch

Filer: `Windmill_Dutch_Iso.png`, `Windmill_Dutch_Side.png`.

```text
Danish Dutch-type windmill c.1840–1850, tapering tar-black octagonal timber body, continuous timber gallery and wooden railing, small rotating rounded cap, exactly four traditional wooden lattice sail wings on a single hub, sailcloth partly unfurled, low masonry base, no auxiliary buildings, no tail fan, no steel sails.
```

### Farm_Trelaenget

Filer: `Farm_Trelaenget_Iso.png`, `Farm_Trelaenget_Side.png`.

```text
Three-wing Danish farm c.1830–1850: exactly THREE connected single-storey wings forming a U around a rectangular cobbled courtyard, enclosed on three sides and completely OPEN on the fourth side facing camera. Dwelling in rear wing, stable and barn in side wings at right angles; black tarred half-timber posts, cream limewash infill, low walls, grey-brown thatched roofs, small domestic windows and plain barn doors. No fourth wing, no gate spanning opening, no detached buildings.
```

Usikkerhed: S: Øerne/Østjylland er en valgt regional anvendelse, ikke dokumenteret ensartet udbredelse. Den åbne U-form har historiske paralleller; sammenbygning, sort tømmer, kalkfarve, stråtag, mål og brolægning er skøn.

### Farm_Tolaenget

Filer: `Farm_Tolaenget_Iso.png`, `Farm_Tolaenget_Side.png`.

```text
Two-wing Danish angle farm c.1830–1850: exactly TWO connected single-storey wings forming a simple L, one long dwelling wing and ONE shorter stable/barn wing at a right angle. Small open yard in the inside angle; the other two sides are entirely open. Low cream limewashed half-timber walls, tar-black posts, small domestic windows, plain barn door, steep grey-brown thatch. No third or fourth wing, no detached buildings.
```

Usikkerhed: S: L-formen er valgt til mindre gårde; landsdækkende hyppighed i 1825–1851 er ikke verificeret. Lex daterer vinkelgårdens brede indpas blandt småejendomme til omkring 1920. Materialer og tag er en generisk periodetolkning.

### Farm_Parallel

Filer: `Farm_Parallel_Iso.png`, `Farm_Parallel_Side.png`.

```text
North Jutland parallel farm c.1830–1850: exactly TWO separate long single-storey rectangular wings parallel to each other, one dwelling wing with small windows and chimney, one stable/barn wing with plain doors. Open cobbled yard between, BOTH ends entirely open. NO connecting cross-wing, NO U or L shape, no detached third building. Cream limewash walls and modest dark timber framing, grey-brown thatched roofs. Treat the two wings as ONE isolated farm compound.
```

Usikkerhed: S: Bindingsværkets farver, taghældning, længernes relative størrelse og brolægning er skøn. V: To parallelle længer og nordjysk udbredelse støttes af Lex, se researchtillægget.

### Farm_VestjyskLanghus

Filer: `Farm_VestjyskLanghus_Iso.png`, `Farm_VestjyskLanghus_Side.png`.

```text
West/south Jutland longhouse farm c.1830–1850: ONE very long LOW single-storey rectangular building under ONE continuous low grey-brown heather-thatch roof. Dwelling at one end, byre in middle and barn at other end, all under the SAME roof. Low dark-red brick walls with rough fieldstone foundation and a short earth/peat wall section towards barn end, simple tarred timber gable cladding, small dwelling windows and simple stable/barn doors. Modest chimney over dwelling. NO courtyard, NO wings, NO additional buildings.
```

Usikkerhed: S: Sammenstillingen af mørkerøde tegl, tørv/marksten, lyngtækning og tjæret gavl er et modelleringsskøn, ikke en verificeret standardtype. Billedets jord-/stensnit kan ikke identificeres sikkert som tørv.

### Farm_Haubarg

Filer: `Farm_Haubarg_Iso.png`, `Farm_Haubarg_Side.png`.

```text
Generic Eiderstedt Schleswig marsh Haubarg c.1830–1850: ONE broad nearly square farm building, enormous steep pyramid-like hipped grey-brown thatched roof visually about 15–20 metres high dominating very LOW red-brick perimeter walls. Central tall hay/barn hall structurally carried by FOUR internal great timber pillars, unseen beneath roof; dwelling and stable contained under this SAME roof. Tiny modest front gable over entrance subordinate to gigantic main roof, small dwelling windows and barn door. Exterior only, no cutaway, NO open courtyard, NO separate wings or buildings. Roof is a vast four-sided hipped mass with short ridge, not an ordinary longhouse saddle roof.
```

Usikkerhed: S: Den konkrete højde, lave teglmure, lille frontgavl og tagproportioner er skøn. V: Fire indvendige stolper og fælles stort stråtag er kildebelagt; 15–20 m er en generel angivelse, mens Frilandsmuseets eksempel har 12 m tag. Indvendige stolper er skjult og kan ikke billedkontrolleres.

### Farm_Niedersachsen

Filer: `Farm_Niedersachsen_Iso.png`, `Farm_Niedersachsen_Side.png`.

```text
Generic Holstein/Lauenburg geest Niederdeutsches Hallenhaus c.1830–1850: ONE broad rectangular aisled hall farmhouse, dwelling, stables and threshing hall under ONE large steep grey-brown reed-thatched roof. Substantial oak Fachwerk frame with muted red-brick infill, low side eaves and tall timber-framed entrance gable with ONE very large central double plank barn door (Grote Dor), dwarfing small side windows. Simple crossed gable boards (Giebelzier), modest and uncarved. No inscription or lettering. No courtyard, NO side wings or detached buildings. Isometric view must clearly show entrance gable and long side.
```

Usikkerhed: S: Den specifikke gest-/Lauenburg-placering, fagdeling, farver, skorsten og enkle krydsbrædder er skøn. V: Hallenhausets fælles tag, bindingsværk, rørtækning og store gavlport er dokumenteret. Side skjuler naturligt gavlporten; den flyttes ikke til langsiden.

## Skøn og usikkerheder

- Farm_Trelaenget: S: Øerne/Østjylland er en valgt regional anvendelse, ikke dokumenteret ensartet udbredelse. Den åbne U-form har historiske paralleller; sammenbygning, sort tømmer, kalkfarve, stråtag, mål og brolægning er skøn.
- Farm_Tolaenget: S: L-formen er valgt til mindre gårde; landsdækkende hyppighed i 1825–1851 er ikke verificeret. Lex daterer vinkelgårdens brede indpas blandt småejendomme til omkring 1920. Materialer og tag er en generisk periodetolkning.
- Farm_Parallel: S: Bindingsværkets farver, taghældning, længernes relative størrelse og brolægning er skøn. V: To parallelle længer og nordjysk udbredelse støttes af Lex, se researchtillægget.
- Farm_VestjyskLanghus: S: Sammenstillingen af mørkerøde tegl, tørv/marksten, lyngtækning og tjæret gavl er et modelleringsskøn, ikke en verificeret standardtype. Billedets jord-/stensnit kan ikke identificeres sikkert som tørv.
- Farm_Haubarg: S: Den konkrete højde, lave teglmure, lille frontgavl og tagproportioner er skøn. V: Fire indvendige stolper og fælles stort stråtag er kildebelagt; 15–20 m er en generel angivelse, mens Frilandsmuseets eksempel har 12 m tag. Indvendige stolper er skjult og kan ikke billedkontrolleres.
- Farm_Niedersachsen: S: Den specifikke gest-/Lauenburg-placering, fagdeling, farver, skorsten og enkle krydsbrædder er skøn. V: Hallenhausets fælles tag, bindingsværk, rørtækning og store gavlport er dokumenteret. Side skjuler naturligt gavlporten; den flyttes ikke til langsiden.

- Alle konkrete mål, proportioner, vinduesantal, fagdelinger, dørplaceringer, detaljer og slidspor er kunstneriske skøn. Visningerne er ikke et metrisk konsistent tegningssæt.
- Researchens farvepalet er skøn: gråbrunt strå, rødorange tegl, støvet kalkhvid, okker, tjæresort, mørkegrøn og okseblodsrød. Digitale farver er ikke pigmentmålinger.
- Gårdens hældning, sorte bjælker, tavlfarve, mødding og brønd bygger på researchens skøn; firlænget form er valgt som én regional type, ikke som universel dansk gård.
- Husmandshusets cirka 10 m, seksrudede vinduer og centrale skorsten er skøn.
- Købstadshusenes etagevalg, facaderytme, vingetegl og dørfarver er skøn. Bindingsværkshuset bruger den østjyske variant: sort tømmer/gule tavl.
- Købmandshusets to etager plus loft, portplacering, hejselem og pudsfarve er skøn. Kun forbygningen vises for at opfylde ét bygningsobjekt pr. billede.
- Kirken er en generisk kombination af romansk skib/kor og senere middelalderligt tårn med kamtakker; den konkrete komposition er skøn.
- Møllens galleri, hætte, sejlføring og sokkel er generisk fortolkning. Researchens påstande om mølletypers udbredelse er ikke selvstændigt efterprøvet her.
- Gårdens Side viser udvendig facade; gårdspladsens brønd og mødding kan naturligt være skjult. Møllens Side er vingesidens ortografiske elevation.

## Kontrol og leveringsstatus

### Tillæg: seks gårdtyper, 2026-10-23

12 nye PNG-filer: seks Iso og seks Side, 1024 × 1024. Genereret med det indbyggede image_gen-værktøj. Farm_Firlaenget_Iso er stilreference for alle seks nye Iso; hvert Side bruger sit tilsvarende Iso. Fælles- og kameraprompter ovenfor genbruges ordret. Typepromptens plan overstyrer fællespromptens parentes om fire længer. Parallelgårdens to længer er ét gårdanlæg. Ingen bybilleder er lavet; midlertidige referenceark var derfor ikke nødvendige.

Alle 12 er visuelt gennemgået for hovedform, tekst, mennesker og fremmede bygninger. Farm_Trelaenget_Side blev regenereret, fordi første forsøg viste gårdsiden med perspektiv. Den leverede variant viser udvendig langfacade. Vinduer, døre, fagdeling og proportioner kan variere mellem Iso og Side. Baggrunden er lys grå med blød lysvariation og kontaktskygge, ikke en ensfarvet pixelmaske. Enkelte små vækstspor ved sokkel/dør er bevaret som i stilreferencen. Kameravinkler er visuelt vurderet, ikke målt. Større kvadratiske originaler skaleres med bikubisk filtrering til 1024 × 1024.

Kontaktark: `Saved/Codex/scenery_dk_farms_sheet.png`, seks nye Iso samt uændret Farm_Firlaenget_Iso. Mærkater findes kun på kontaktarket; felterne viser former, ikke fælles målestok.

Ekstra instruktion efter typeprompten til Iso (fra Farm_Tolaenget og frem):

```text
The type-specific farm plan overrides the common prompt's four-wing parenthetical. Input reference is for consistent restrained realistic miniature STYLE, thatch texture, materials and lighting only; replace its plan with the requested type. One isolated farm compound, generous margins, no text or people.
```

Første Farm_Trelaenget_Iso brugte samme prioritering med denne formulering:

```text
The type-specific THREE-wing plan overrides the common prompt's four-wing parenthetical. Input image is a STYLE AND MATERIAL reference only; keep its restrained realistic miniature rendering but replace the closed four-wing plan with this open U plan. No well or dunghill needed. One farm compound only.
```

Side bruger den eksisterende ekstra instruktion om at bevare Iso-bygningens materialer og hovedform. Desuden præciseres udvendig langfacade uden vigende vægge: udlænger skjules bag boliglængen; gavlport/frontgavl forbliver på deres oprindelige, skjulte facade. Haubargens store tag og langhusets sammenhængende tag bevares.

Den følgende leveringsstatus gælder de oprindelige otte typer.


Færdigt: 16 PNG-filer, otte Iso og otte Side, alle kontrolleret til 1024 × 1024. Alle visninger er visuelt gennemgået. Iso blev genereret før Side. Ingen navngivne bygninger eller fotografiske bygningsreferencer er brugt.

Kasseret/regenereret: Windmill_Dutch_Iso (for lav kameravinkel) og Church_Village_Side (vestportal fejlagtigt flyttet til langsiden). De oprindelige varianter er ikke leveret i DK-mappen. Ingen øvrige billedvarianter er kasseret.

Accepterede begrænsninger: mindre forskelle i fagdeling og proportioner mellem Iso/Side, tydeligst i TownHouse_White. Gårdens Side viser portlængens ydre med baglængens tag synligt ovenover, ikke en opmålt boligfacade. Gårdens Side har meget snæver vandret margin. Baggrunden har blød lysvariation og kontaktskygge. Overflader er mere detaljerede end de senere low-poly-modeller bør være.

Kirkerettelsens fulde prompt, med første Church_Village_Side som redigeringsmål:

```text
Use case: historical-scene. Precise architectural correction to this generic Danish medieval village church elevation. Keep the complete straight orthographic longitudinal side elevation, tower left, nave middle, lower chancel right, stepped tower gables, all proportions, roof tiles, windows, whitewash, light grey background, even lighting, no text or people. Change ONLY the arched doorway at the bottom of the tower: remove the entire portal, green door and arch mouldings, replace with continuous plain whitewashed masonry matching adjacent tower walls. The west entrance is on the perpendicular hidden end wall, therefore cannot appear on this long-side elevation. Preserve every other feature. 1024x1024 PNG.
```

Møllens første Iso blev kasseret på grund af for lav kameravinkel. Regenereret med samme billede som reference og følgende tilføjelse til den oprindelige prompt:

```text
Crucial correction: camera must be HIGH looking DOWN 40 degrees, strongly see top surface of gallery and cap. NOT near-level frontal view. Keep entire four sails in frame with margins.
```

Side-genereringens ekstra instruktion (efter typeprompten):

```text
Input image is a visual reference for the same generic building. Change only camera to the required straight orthographic elevation; preserve materials, main proportions, storeys, door/window rhythm, roof and distinctive features. No perspective at all. 
```

Husmandshuset blev genereret med mørkt tømmer frem for promptens hvide tømmer. Det er accepteret som en generisk variant; kalkhvide tavl og halvvalmet stråtag er bevaret. Isovinkler er visuelt tilnærmede, ikke målte kameradata. Generatorens større kvadratiske originaler skaleres til 1024 × 1024 med bikubisk filtrering.

