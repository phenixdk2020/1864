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

## Skøn og usikkerheder

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

