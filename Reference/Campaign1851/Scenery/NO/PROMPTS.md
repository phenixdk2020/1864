# Norske scenery-bygninger – billedprompts

Dato: 2026-10-22; udført 2026-10-23. Kildegrundlag: Docs/Bygninger-NO-Research-2026-10-22.md. Generiske periodetyper ca. 1825–1864; ingen navngivne bygninger som billedforlæg. Modelleringsreferencer, ikke opmålinger.

## Fælles prompt

```text
Use case: historical-scene. Asset: isolated architectural modelling reference for PROJECT 1864 campaign scenery, ordinary Norway c.1825–1864. Generate ONE generic period-typical building only (farm is one connected four-wing complex), never reproduce any real named building. 1024x1024 PNG square. Clean readable architectural forms, restrained realistic materials suitable for later low-poly interpretation. Plain uniform light grey background and ground, soft even studio light, faint contact shadow, whole silhouette with generous margins. No people, animals, labels, text, dimensions, border, collage, landscape, lawns, neighbouring buildings, modern shop glazing, cast iron balconies, corrugated cement, modern machinery or steel mill sails. Palette: dusty cream limewhite, muted ochre, red-orange clay tile, grey-brown thatch, tar black, subdued dark green/oxblood doors. 
```

Fællesprompten er genbrugt ordret fra DK bortset fra land/periode. Den danske palette og gårdparentes overstyres eksplicit af følgende landespecifikke tillæg; kamerateksterne er ordrette.

```text
Country-specific type instructions override the Danish four-wing parenthesis and Danish palette in the unchanged common template: Norwegian painted timber in muted red ochre/Falu-red, ochre and white; dark tarred or weathered logs; subdued sod green over birch bark; clay tile on town houses and dark timber shingles on church. Farm is one detached-building tun compound. No Danish half-timber infill. 
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

Den fulde prompt er fælles prompt + kameraprompt + typeprompt + landespecifikt tillæg. Side bruger sit tilsvarende godkendte Iso og dette ekstra tillæg:

```text
Input image is a visual reference for the same generic building. Change only camera to the required straight orthographic elevation; preserve materials, main proportions, storeys, door/window rhythm, roof and distinctive features. No perspective at all. For farm show the exterior long dwelling facade, other tun buildings may be hidden behind it; never connect the detached buildings. For watermill show the long door facade with launder visible in profile, no windmill sails.
```

## Typer og filnavne

### TownHouse_White_NO

Filer: `TownHouse_White_NO_Iso.png`, `TownHouse_White_NO_Side.png`.

```text
Ordinary compact single-storey Norwegian timber town house, horizontal boarded cladding painted broken warm white, red-orange clay pantile gabled roof, modest small-pane white windows, dark red plank door, one chimney, simple flat early nineteenth-century trim, low stone foundation. Timber wall beneath boarding is log-built, NOT half timber with plaster infill.
```

### TownHouse_Ochre_NO

Filer: `TownHouse_Ochre_NO_Iso.png`, `TownHouse_Ochre_NO_Side.png`.

```text
Ordinary compact TWO FULL STOREY Norwegian timber town house, horizontal boarded cladding painted muted yellow ochre, white small-pane windows in two rows, dark tarred door, red-orange clay pantile gabled roof, one chimney, simple flat early nineteenth-century trim. NO exposed half timber frame or plaster infill.
```

### TownHouse_Timber_NO

Filer: `TownHouse_Timber_NO_Iso.png`, `TownHouse_Timber_NO_Side.png`.

```text
Ordinary compact TWO FULL STOREY Norwegian timber town house, horizontal boarded cladding painted muted iron-oxide Falu red, white small-pane windows and simple corner boards, dark door, red-orange clay pantile gabled roof, modest chimney, simple flat early nineteenth-century trim. NO exposed half timber frame or plaster infill.
```

### MerchantHouse_NO

Filer: `MerchantHouse_NO_Iso.png`, `MerchantHouse_NO_Side.png`.

```text
Generic Norwegian merchant street house c.1825–1864, exactly two storeys plus attic, pale warm grey stucco facade, red clay pantile gabled roof, broad central rectangular carriage gateway through ground floor with dark timber doors, regular small-pane windows, restrained classical flat trim, one small roof loading hatch with plain timber hoist. Single street building only, no warehouse wings, no elaborate later tenement ornament.
```

### Cottage_Laft_NO

Filer: `Cottage_Laft_NO_Iso.png`, `Cottage_Laft_NO_Side.png`.

```text
Modest single-storey Norwegian inland cottage, horizontal hewn grey-brown interlocking log walls with clearly visible projecting notched log corners, low pitched sod gable roof, thin pale birch bark edges beneath green-brown turf retained by timber eave logs, low rough stone footings, small divided windows, one plain plank door and one modest masonry chimney. One dwelling only, NO separate stabbur or outbuilding, no decorative dragon carving.
```

### Farm_Tun_NO

Filer: `Farm_Tun_NO_Iso.png`, `Farm_Tun_NO_Side.png`.

```text
Norwegian inland farm TUN: exactly FOUR small DETACHED log buildings around one open compact earth yard: one modest single-storey dwelling with chimney and small windows, one low barn with broad plank door, one low byre, and one small STABBUR food store on four short timber posts with broad caps and a visible air gap beneath its log floor, NO chimney on stabbur. Dark weathered horizontal interlocking logs, projecting notched corners, rough stone footings and green-brown sod gable roofs over pale birch bark edges, timber turf-retaining eaves. FOUR detached buildings form ONE isolated farm compound, not a connected Danish four-wing farm. No extra buildings or scenery.
```

### Church_Wooden_NO

Filer: `Church_Wooden_NO_Iso.png`, `Church_Wooden_NO_Side.png`.

```text
Generic modest Norwegian white-painted boarded wooden long church c.1825–1864, rectangular nave, slightly lower narrower chancel, attached square west bell tower with restrained short dark timber spire and tiny plain cross, dark weathered wood shingle gable roofs, simple small-pane tall rectangular windows with flat classical trim. One connected church only, no detached belfry, no cemetery, no masonry stepped gables, no Gothic cathedral, no stave-church dragon ornaments.
```

### Windmill_Watermill_NO

Filer: `Windmill_Watermill_NO_Iso.png`, `Windmill_Watermill_NO_Side.png`.

```text
Norwegian small water-powered KVERNHUS grain mill, ONE compact dark weathered horizontal log building with projecting notched corners, green-brown sod gable roof over birch-bark edges, plain plank door and one tiny window, raised on rough stone supports above a small open underfloor water-wheel space. Short narrow timber water launder attached at one side leading underneath the building to a concealed horizontal wooden wheel. NO windmill body, NO sails, NO large exterior vertical wheel, NO chimney, NO river or landscape, no auxiliary buildings. Entire mill and short launder isolated on light grey studio ground.
```

## Skøn og usikkerheder

- S: Alle mål, farver, fagdelinger og vinduesantal er kunstneriske skøn. Regional fordeling er ikke verificeret.
- S: Hvide huse anvendes selektivt i 1825; hvid maling blev mere almindelig omkring 1850, se research.
- S: Farm er ét tun med fire adskilte bygninger inklusive stabbur. Cottage viser kun boligen.
- S: Windmill_Watermill_NO erstatter vindmøllen med et vanddrevet kvernhus. Placering kræver vand; ellers udelades slotten. Kameraets ord »mill sail-facing elevation« overstyres af typeprompten.
- S: Kirken er en trælangkirke med tilsluttet tårn. Stavkirkehistorien indebærer ikke drageornamentik.
- S: Iso/Side er ikke et målfast tegningssæt. Skjulte gavldøre flyttes ikke til langsiden.

## Kontrol og leveringsstatus



Færdigt: 16 PNG-filer, otte Iso og otte Side, visuelt gennemgået for hovedform, tekst, mennesker, fremmede bygninger og kamera. Alle er 1024 × 1024. Genereret med det indbyggede image_gen-værktøj, ét billede pr. kald. Alle Iso blev færdige før Side; hver Side brugte sin tilsvarende Iso som reference.

Kasseret/regenereret: første Windmill_Watermill_NO_Iso (beskåret rende og lodret hjul); første Church_Wooden_NO_Side (gavlportal flyttet til langsiden); første Windmill_Watermill_NO_Side (gavldør flyttet til langsiden). Kun de rettede billeder leveres.

Accepterede begrænsninger: Farm_Side viser boligens langfacade; de øvrige tunbygninger og stabburet er ikke synlige i denne visning. Brug Iso til hele anlæggets plan. Rødt TownHouse har en ændret dørdetalje i Side. Mindre proportioner, vinduesrytmer og slid kan variere mellem visningerne. Kirkens faktiske spir blev slankere end promptens ord »short«; det er accepteret som en generisk variant. Birch-bark/never-kanterne er billedfortolkning, ikke teknisk snit. Baggrunden er lys grå med blød lysvariation og kontaktskygge, ikke en ensfarvet pixelmaske. Kameraer er visuelt vurderet, ikke målte. Generatorens 1254 × 1254-originaler er skaleret bikubisk til 1024 × 1024, som i DK-præcedensen.

Rettelsesprompter (engelsk):

### Kvernhus Iso

```text
CRITICAL CORRECTION to reference: REMOVE the large visible VERTICAL wheel entirely, replace its opening with a plain dark underfloor void; horizontal kvernkall lies concealed below the building and must NOT be drawn as a visible vertical wheel. Make timber launder SHORT, its complete outer end and supports entirely inside image with generous grey margin, nothing cropped. Camera framing 30 percent wider. Keep log house, sod roof, stone supports, one door, tiny window. No text or people.
```

Denne tekst blev tilføjet den fulde Iso-prompt, med første kvernhus-Iso som reference.

### Kirke Side

```text
Use case: historical-scene. Precise architectural correction. Image 1 is edit target, straight orthographic LONG SIDE elevation of a generic Norwegian white wooden church. Image 2 is corresponding approved Iso for geometry. Change ONLY the incorrect red portal and its small porch gable at bottom left of the side elevation: remove this entire door, trim, porch gable and projecting steps, replace with uninterrupted plain white vertical boarding matching the tower side wall and a continuous low stone foundation. West entrance exists on perpendicular HIDDEN west end, so it must not appear on this elevation. Keep tower window above, tower spire, crosses, nave windows, chancel, shingles, proportions, grey background, lighting and camera unchanged. No people, text or perspective. 1024x1024 PNG.
```

### Kvernhus Side

```text
Use case: historical-scene. Architectural correction. Image 1 is edit target, straight orthographic LONG SIDE of Norwegian log watermill. Image 2 is approved Iso geometry. Remove ONLY the incorrectly relocated door, doorstep and staircase on the right of image 1, replace with continuous horizontal dark logs and plain stone support matching the left support, keeping the open underfloor space. In the Iso the door is on the perpendicular gable end, so it is hidden in this LONG SIDE elevation. Keep the one tiny window, short complete timber launder at left, turf roof, foundations, grey background, lighting and strict elevation camera unchanged. No vertical wheel, people or text. 1024x1024 PNG.
```

Side-grundpromptens »long door facade« var misvisende, fordi Iso-døren sidder i gavlen. Den leverede rettelse viser langsiden med vinduet; gavldøren forbliver skjult.
