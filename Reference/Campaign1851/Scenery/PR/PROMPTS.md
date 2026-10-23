# Preussiske scenery-bygninger – billedprompts

Dato: 2026-10-22. Grundlag: Docs/Bygninger-PR-Research-2026-10-22.md. Generiske kernetyper 1825–1864. S: Alle konkrete billeddetaljer er skøn; regionale V-kilder står i researchen.

## Fælles prompt

```text
Use case: historical-scene. Asset: isolated architectural modelling reference for PROJECT 1864 campaign scenery, ordinary Prussia c.1825–1864. Generate ONE generic period-typical building only (farm is one connected four-wing complex), never reproduce any real named building. 1024x1024 PNG square. Clean readable architectural forms, restrained realistic materials suitable for later low-poly interpretation. Plain uniform light grey background and ground, soft even studio light, faint contact shadow, whole silhouette with generous margins. No people, animals, labels, text, dimensions, border, collage, landscape, lawns, neighbouring buildings, modern shop glazing, cast iron balconies, corrugated cement, modern machinery or steel mill sails. Palette: dusty cream limewhite, muted ochre, red-orange clay tile, grey-brown thatch, tar black, subdued dark green/oxblood doors. 
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

Den fulde prompt er fælles prompt + kameraprompt + typeprompt. Hallenhausets typeprompt overstyrer fællespromptens parentes om fire længer.

## Typer og filnavne

### TownHouse_White_PR

Filer: `TownHouse_White_PR_Iso.png`, `TownHouse_White_PR_Side.png`.

```text
Ordinary compact single-storey Prussian core market-town Putzbau, dusty white plaster over masonry, red-orange clay tile gabled roof, small white six-pane windows, dark green plank door, one chimney, restrained plain eaves, no ornament.
```

### TownHouse_Ochre_PR

Filer: `TownHouse_Ochre_PR_Iso.png`, `TownHouse_Ochre_PR_Side.png`.

```text
Ordinary compact two-storey Prussian core market-town Putzbau, muted ochre plaster over masonry, red-orange clay tile gabled roof, two full rows of small white windows, dark green door, modest chimney, simple classical cornice only, no late historicist ornament.
```

### TownHouse_Timber_PR

Filer: `TownHouse_Timber_PR_Iso.png`, `TownHouse_Timber_PR_Side.png`.

```text
Ordinary two-storey Prussian core inland town Fachwerk house, black structural timber posts and beams with dusty white plaster infill, simple bracing without carved ornament, small white windows, dark plank door, red-orange clay tile gabled roof, modest chimney.
```

### MerchantHouse_PR

Filer: `MerchantHouse_PR_Iso.png`, `MerchantHouse_PR_Side.png`.

```text
Generic Baltic Prussian merchant house c.1840, ONE narrow red-brick gable-front street building, simple restrained stepped brick front gable, exactly two full commercial/residential storeys plus attic storage, small windows, broad arched plank entrance in front gable, shuttered attic loading opening with short wooden hoist beam, red clay tile saddle roof behind gable. No side wings, no inscriptions, no ornate pinnacle forest.
```

### Cottage_Kate_PR

Filer: `Cottage_Kate_PR_Iso.png`, `Cottage_Kate_PR_Side.png`.

```text
Modest Prussian core Kate rural labourer's cottage c.1840: ONE low single-storey rectangular long cottage, simple dark oak Fachwerk with dusty white limewashed infill, steep grey-brown thatched half-hipped roof, small windows and plain dark door on long facade, one modest masonry chimney. No wings or barn gateway.
```

### Farm_Hallenhaus_PR

Filer: `Farm_Hallenhaus_PR_Iso.png`, `Farm_Hallenhaus_PR_Side.png`.

```text
Generic western Pomeranian Niederdeutsches Hallenhaus c.1840: ONE broad rectangular aisled farmhouse, dwelling, stable and threshing hall beneath ONE enormous steep grey-brown thatched roof. Dark oak Fachwerk frame with muted red-brick infill, low side eaves, tall entrance gable with ONE very large central double plank barn door dwarfing small side windows, modest chimney at dwelling end. Show entrance gable and long side. Type-specific ONE building plan overrides common farm parenthetical: NO courtyard, NO wings, NO additional buildings.
```

### Church_Backstein_PR

Filer: `Church_Backstein_PR_Iso.png`, `Church_Backstein_PR_Side.png`.

```text
Generic small medieval Prussian core village church preserved c.1840, red-brick longitudinal nave, lower narrower straight-ended chancel, attached square west tower with simple saddle roof and modest brick gables, rough granite fieldstone plinth, restrained pointed-arched nave windows and brick blind arches, red clay tile roofs. No spire, no neo-Gothic revival ornament, no cemetery or walls. One connected church.
```

### Windmill_Bock_PR

Filer: `Windmill_Bock_PR_Iso.png`, `Windmill_Bock_PR_Side.png`.

```text
Generic Prussian Pomeranian Bockwindmuehle post mill c.1840: rectangular two-storey vertical weathered brown timber-clad MILL BOX elevated on a clearly visible heavy timber trestle and central post, low separate stone footings. Simple shingled gabled roof. Exactly FOUR traditional wooden lattice sail wings on ONE hub with partly unfurled sailcloth, rear wooden stairs and long wooden tailpole for turning whole box. NO tapering tower, NO gallery, NO separate rotating cap, NO fantail, NO auxiliary buildings. Entire sails and trestle visible with margins.
```

## Skøn og usikkerheder

S: Farver, proportioner, fagdeling og åbninger er kunstneriske skøn. Hallenhaus er regionalt; Vierseithof beskrives i researchen uden separat billede. Kate som Insthaus er foreløbig erstatning. Ingen opmålinger.

Side bruger tilsvarende Iso som visuel reference og følgende ekstra instruktion:

```text
Input image is a visual reference for the same generic building. Change only camera to the required straight orthographic elevation; preserve materials, main proportions, storeys, door/window rhythm, roof and distinctive features. No perspective at all. 
```

## Kontrol og leveringsstatus

Færdigt: 16 PNG-filer, otte Iso og otte Side, alle 1024 × 1024. Genereret med det indbyggede image_gen-værktøj, Iso før Side. Alle er visuelt gennemgået for enkeltbygning, hovedform, mennesker og tekst. Ingen bygningsbilleder er kasseret. Originaler er bikubisk skaleret til leveringsmål, som i DK-præcedensen.

Alle viser én generisk bygning på lys grå baggrund med kontaktskygge. Baggrunden har blød lysvariation og er ikke en ensfarvet pixelmaske. Kameravinkler er visuelt vurderet, ikke målt. Iso/Side bevarer hovedform og materialer, men smådetaljer, vinduesrytme og proportioner kan variere. Stubmøllens øverste Iso-vinge har snæver, men ubeskåret margin. Hallenhausets Side er meget langstrakt med lav langvæg; gavlporten er skjult. På kirken er vestportalen skjult i Side. Billederne er modelleringsreferencer, ikke metrisk konsistente tegninger.

### Ekstra Side-instruktioner

Efter den generelle Side-instruktion blev følgende tilføjet for de angivne typer:

MerchantHouse_PR:
```text
Show the LONG SIDE wall, not the stepped entrance gable. Entrance gateway and hoist are on the perpendicular hidden gable and MUST NOT be relocated to the side wall. Stepped gable is seen edge-on only. 
```

Farm_Hallenhaus_PR:
```text
Long exterior side elevation only: the huge barn gateway stays on the perpendicular hidden entrance gable. Do NOT move the gateway to the long wall. Gable timber decoration is hidden. Preserve the very low eaves and single continuous enormous thatched roof. 
```

Church_Backstein_PR:
```text
Tower left, nave middle, lower chancel right. West tower entrance stays on the hidden perpendicular west wall; show NO doorway in tower long-side wall. 
```

Windmill_Bock_PR:
```text
Show sail-facing elevation straight on, all four sails fully in frame with generous margins. Rear stairs and tailpole may be naturally occluded; do not relocate them.
```

