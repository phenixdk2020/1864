# Svenske scenery-bygninger – billedprompts

Dato: 2026-10-23. Grundlag: Docs/Bygninger-SE-Research-2026-10-22.md. Generiske periodetyper 1825–1864, billedfortolkning ca. 1840. S: alle konkrete proportioner og farvefordelinger.

## Fælles prompt

```text
Use case: historical-scene. Asset: isolated architectural modelling reference for PROJECT 1864 campaign scenery, ordinary Sweden c.1825–1864. Generate ONE generic period-typical building only (farm is one connected four-wing complex), never reproduce any real named building. 1024x1024 PNG square. Clean readable architectural forms, restrained realistic materials suitable for later low-poly interpretation. Plain uniform light grey background and ground, soft even studio light, faint contact shadow, whole silhouette with generous margins. No people, animals, labels, text, dimensions, border, collage, landscape, lawns, neighbouring buildings, modern shop glazing, cast iron balconies, corrugated cement, modern machinery or steel mill sails. Palette: dusty cream limewhite, muted ochre, red-orange clay tile, grey-brown thatch, tar black, subdued dark green/oxblood doors. 
```

Fællesprompten er kopieret ordret fra DK med kun land/periode ændret. Dens palet suppleres af de svenske typeprompts; rødt træ og hvide hjørner har forrang på parstugaen.

## Kameraprompts

Iso:
```text
View: orthographic-like oblique strategy map view, camera 38 degrees above horizontal ground and 45 degrees around building; show roof and two sides clearly. 
```

Side:
```text
View: strictly straight-on orthographic SIDE ELEVATION at mid-wall height, horizontal ground line, no perspective, no roof-top view and no visible receding wall. Show long principal facade (farm exterior long dwelling wing; church longitudinal nave and tower profile; mill sail-facing elevation). 
```

Den fulde prompt er fælles prompt + kameraprompt + typeprompt. Side bruger den tilsvarende Iso som reference og følgende tillæg:

```text
Image 1 is the approved Iso architectural reference. Preserve the SAME building, materials, colours, storey count, roof form and openings; change only the camera to the specified side elevation.
```

## Typer og filnavne

### TownHouse_White_SE

Filer: `TownHouse_White_SE_Iso.png`, `TownHouse_White_SE_Side.png`.

```text
Ordinary compact two-storey Swedish town house c.1840, cream-white limewashed plaster over masonry, red-orange clay pantile gabled roof, white small-pane windows, dark green door, simple eaves and modest chimney, restrained early neoclassical proportions. Single building, no wings.
```

### TownHouse_Ochre_SE

Filer: `TownHouse_Ochre_SE_Iso.png`, `TownHouse_Ochre_SE_Side.png`.

```text
Ordinary two-storey Swedish timber market-town house c.1840, muted ochre-yellow painted vertical board cladding, white corner boards and small-pane window trim, red-orange clay pantile gabled roof, modest chimney, dark green plank door, simple eaves, no exposed half-timber grid.
```

### TownHouse_Timber_SE

Filer: `TownHouse_Timber_SE_Iso.png`, `TownHouse_Timber_SE_Side.png`.

```text
Ordinary single-storey Swedish timber town house c.1840, weathered grey-brown vertical board cladding over log structure, cream-white small-pane windows and corner boards, dark wood-shingle gabled roof, one modest masonry chimney, oxblood plank door, no exposed half-timber grid, no veranda.
```

### MerchantHouse_SE

Filer: `MerchantHouse_SE_Iso.png`, `MerchantHouse_SE_Side.png`.

```text
Generic Swedish merchant house c.1840, exactly two storeys plus attic, pale ochre plaster facade over masonry, red clay pantile gabled roof, broad central carriage gateway through ground floor, small-pane white windows, single central roof loading hatch with timber hoist beam, shuttered loading opening. Single street building only, no warehouse wings.
```

### Cottage_Parstuga_SE

Filer: `Cottage_Parstuga_SE_Iso.png`, `Cottage_Parstuga_SE_Side.png`.

```text
Swedish rural parstuga c.1840, ONE low single-storey rectangular timber house, falu-red painted vertical board cladding with white corner boards, central simple plank entrance with one small six-pane window to either side, no porch, modest central masonry chimney, grey-brown wood-shingle gabled roof. Red and white rural palette overrides the common Danish-derived palette. No ornate gables or Norwegian forms.
```

### Farm_Firlaenget_Skane

Filer: `Farm_Firlaenget_Skane_Iso.png`, `Farm_Firlaenget_Skane_Side.png`.

```text
Southern Swedish Skane farm c.1840: exactly four connected low single-storey half-timbered wings enclosing a fully CLOSED rectangular courtyard, dark timber posts and cream limewashed infill, steep grey-brown straw-thatched roofs, small dwelling windows and plain barn doors, integrated carriage passage. One connected compound, no detached buildings, no later industrial brick stable. No falu-red board cladding on this regional half-timber farm.
```

### Church_Village_SE

Filer: `Church_Village_SE_Iso.png`, `Church_Village_SE_Side.png`.

```text
Generic Swedish white-plastered stone village church c.1840: simple rectangular nave, lower narrower chancel, attached square west tower topped by a modest dark timber lantern and SHORT pyramidal spire, grey-brown wood-shingle roofs, small round-arched windows. One connected church only, no detached bell tower, no stepped gables, no Gothic revival ornament, no cemetery or wall. No clock numbers or text.
```

### Windmill_Post_SE

Filer: `Windmill_Post_SE_Iso.png`, `Windmill_Post_SE_Side.png`.

```text
Generic Swedish post mill c.1840, small rectangular weathered grey-brown timber millhouse raised on ONE visible central stout wooden post with timber trestle cross-base, wood-shingle gabled roof, simple external wooden stair and rear tailpole. Exactly FOUR traditional wooden lattice sails on ONE hub. Entire millhouse rotates around post. No gallery, no octagonal tapering smock body, no tail fan, no steel sails, no auxiliary building.
```

## Usikkerheder og kontrol

S: Farver, mål, vinduer, taghældning og regional fordeling er fortolkninger; V-belæg fremgår af researchdokumentet. Farm er en skånsk variant, ikke en svensk standard. Falurød er ikke universel i 1825. Stolpmølle er valgt frem for smock-mølle; kirken har integreret tårn for at bevare ét objekt. Norske former udelades. Ingen billeder er godkendt før individuel visuel kontrol.

## Leverance og visuel kontrol

Alle 16 PNG er genereret med indbygget image_gen og individuelt visuelt gennemgået. Hver Iso er genereret før sin Side; Side brugte kun den tilsvarende Iso som billedreference. Én generisk bygning pr. billede; gården tæller som ét sammenbygget anlæg. Ingen synlig tekst, mennesker eller moderne genstande. Alle er gemt som 1024 × 1024 PNG; eventuel størrelsestilpasning er ren resampling uden ændring af motivet. Lys grå studiobaggrund med blød kontaktskygge; svag lysvariation forekommer.

Godkendt: hvidt stenhus og gult panelhus i to etager; gråt panelhus og rød parstuga i én etage; købmand i to etager med port og hejsekvist; lukket firlænget skånsk gård; hvid kirke med lavere kor og integreret tårn; stolpmølle med synlig stub og fire vinger. S: Kirkespiret er højere end promptens SHORT; det accepteres som generisk historisk fortolkning, og Side bevarer samme hovedform. Der er ikke tilføjet et separat klockstapel-objekt.

S: Vinduesrytmer, små proportioner, tagdetaljer og slid kan variere mellem Iso og Side. Gårdens Side viser den ydre port-/boliglænge; gårdrummets øvrige længer er skjult af elevationen. Møllens Side skjuler naturligt trappe og bagbom. Kameraet er visuelt vurderet, ikke metrisk dokumenteret.

Møllens første Side-forsøg blev kasseret, fordi vingerne var vist fra kanten. Det leverede andet forsøg brugte den sædvanlige fulde Side-prompt samt følgende ordrette tillæg:

```text
CRITICAL correction: show the SAIL-FACING elevation, looking directly along the horizontal windshaft at the hub, with all FOUR lattice sails fully spread visibly in an X in the image plane. The millhouse gabled end is BEHIND the hub. Do not show the long side with the sails seen edge-on. No roof-top or receding wall. The stair and rear tailpole may be hidden naturally behind the mill.
```

Ingen kasserede billeder er kopieret til SE-mappen. Bykompositioner og deres kontrol er dokumenteret i Towns/TOWNS.md.
