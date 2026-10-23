# Preussiske byer på kampagnekortet

Dato: 2026-10-22. Generiske kernepreussiske miniaturebyer ca. 1840, ingen navngivne virkelige steder. Genereret med det indbyggede image_gen-værktøj. Fire PNG, 1536 × 1024. S: Byplaner, beplantning, farver, antal og alle konkrete detaljer er kunstneriske skøn; V-kilder står i Docs/Bygninger-PR-Research-2026-10-22.md. Der ændres ingen scenariedata.

## Visuelle referencer

Alle otte godkendte Iso i overmappen er samlet uændret på to midlertidige 2048 × 2048-ark med 2×2 felter. Boligark: Kate/okker øverst, hvid/Fachwerk nederst. Specialark: Hallenhaus/købmand øverst, kirke/stubmølle nederst. C bruger de to ark. B og første A bruger desuden købmand, kirke og mølle individuelt. Andet/tredje A og begge havneforsøg bruger arkene samt okkerhus, købmand og kirke individuelt. Højst fem referencefiler pr. kald. Arkene Saved/Codex/pr_homes_temp.png og pr_special_temp.png er slettet efter brug.

## Prompter

Den faktiske fællesprompt nedenfor efterfølges af typeprompten for C, B, første A og første havneforsøg. Andre forsøg har deres fulde prompt særskilt.

### Fælles prompt

```text
Use case: historical-scene. PROJECT 1864 campaign-map town visualisation, generic Prussian core c.1840, not a copy of any real town. One landscape 1536x1024 PNG. Supplied approved Iso images are strict architectural KIT references: preserve materials, colours, storeys, main silhouette and restrained realistically textured miniature style. Reference 1 is homes sheet: top left Kate, top right TWO-storey ochre Putzbau, bottom left ONE-storey white Putzbau, bottom right TWO-storey black-and-white Fachwerk. Reference 2 is special sheet: top left ONE-roof Hallenhaus, top right red-brick stepped-gable merchant with hoist, bottom left red-brick medieval church with stepped saddle-roof tower and granite plinth, bottom right brown rectangular post mill on exposed trestle with FOUR lattice sails, NO gallery. Repeat these approved models; no invented building types. Oblique orthographic-like campaign-map camera 38 degrees above horizontal, roofs and two sides, no horizon or sky. Soft daylight, gentle shadows. Continuous green-ochre countryside with cultivated fields, gardens, hedges and dirt roads extending to frame; no floating plinth or grey studio background. Whole settlement in frame with margins. No text, labels, people, modern objects, asphalt, electricity, cars, railway, factories, Wilhelmine or Gruenderzeit ornament. Red clay tile on town buildings, grey-brown thatch on Kate and Hallenhaus. 
```

### Town_C_Minor

```text
Tier C minor settlement: EXACTLY THREE buildings, one white one-storey Putzbau, one Kate, one Hallenhaus, arranged around a dirt-road fork with tiny gardens and fields. No church, mill, merchant or extra sheds. Keep three complete distinct silhouettes.
```

### Town_B_Regional

```text
Tier B regional town: exactly SIX town buildings, two TWO-storey ochre houses, two ONE-storey white houses, one TWO-storey black-white timber house, one red-brick stepped-gable merchant, PLUS ONE brick church beside a modest cobbled square. Add ONE post mill in outer fields. Total EIGHT units. Six town buildings face the street and square naturally, all visible and countable. No farms or cottages in this view.
```

### Town_A_City — første forsøg, kasseret

```text
Tier A development city, generic early Berlin/Koenigsberg-type plaster and brick mix: exactly FOURTEEN urban buildings: FIVE TWO-storey ochre houses, FOUR ONE-storey white houses, ONE TWO-storey black-white timber house, THREE red-brick stepped-gable merchants, ONE brick church. Two crossing cobbled streets and a market square with well, compact close-set rows and small back gardens; all fourteen readable. Add ONE post mill and ONE Hallenhaus in outskirts. Total SIXTEEN units. Clearly larger and denser than tier B. No palaces or extra building types.
```

### Town_Port — første forsøg, kasseret

```text
Generic Baltic Hanseatic Prussian port c.1840, red-brick stepped-gable street frontages inspired by the broader Stralsund/Danzig tradition but NEVER copy any real place. Exactly EIGHTEEN urban buildings: EIGHT approved red-brick stepped-gable merchant houses with hoist hatches (some used as waterfront stores), FOUR TWO-storey ochre houses, THREE ONE-storey white houses, TWO TWO-storey black-white timber houses, ONE brick church. Dense short rows along three cobbled lanes and a market square; prominent brick stepped gables face quay. Add ONE approved post mill behind town. Stone/timber quay along one side, mooring posts, TWO modest wooden period sailing ships and one rowing boat, no modern cranes. Water no more than quarter of image. Whole port inside frame. Preserve exact kit silhouettes and full two-storey height of ochre and timber houses.
```

### A — andet forsøg, kasseret

Samme fælles- og første A-typeprompt, efterfulgt af:

```text
CRITICAL CORRECTION: ALL FIVE ochre houses MUST have TWO FULL WALL STOREYS below the roof: a complete upper row of five windows and ground row, exactly as approved ochre reference. Never single-storey ochre. Build compact CLOSE-SET urban rows, houses almost touch along street, not scattered garden villas. At least TEN distinct buildings in urban core. Exactly THREE merchant buildings. Prioritise kit fidelity over decorative gardens. 
```

### A — tredje forsøg, accepteret

```text
Use case: historical-scene. Generate a generic Prussian core provincial city c.1840, PROJECT 1864 campaign-map miniature. Landscape PNG 1536x1024. Supplied images are strict kit references. Image 1 homes sheet: Kate / TWO-STOREY OCHRE above, ONE-STOREY WHITE / TWO-STOREY BLACK-WHITE TIMBER below. Image 2 specials: Hallenhaus / brick merchant above, brick church / post mill below. Image 3 exact TWO-STOREY OCHRE house, image 4 exact red-brick stepped-gable merchant, image 5 exact brick church. SIMPLE INVENTORY: THREE exact copies of image 3 ochre house (each must have TWO complete floors of windows BELOW the roof), THREE white ONE-floor houses, TWO black-white timber TWO-floor houses, THREE brick merchant houses, ONE brick church = TWELVE urban buildings. Group in compact short rows bordering two intersecting cobbled streets and a market square with well. NO yellow one-storey buildings anywhere; yellow colour is permitted ONLY on the exact two-storey model of image 3. Preserve ochre on ALL walls of each ochre building. Do not combine yellow and white buildings. Preserve individual roof silhouettes so 12 buildings are countable. All urban buildings within frame. Add ONE trestle-supported rectangular brown post mill with four wooden sails on outer fields. NO farms or cottages needed. Clearly larger and denser than regional village, modest early plaster-and-brick provincial city, no palace. Camera orthographic-like oblique 38 degrees above horizontal showing roofs and two sides, no sky or horizon. Soft daylight, restrained realistic textures exactly as kit. Continuous green-ochre terrain, fields and small gardens to edges, no plinth or grey backdrop. No text, labels, people, modern objects, electricity, asphalt, railway, Wilhelmine/Gruenderzeit architecture, industrial chimneys. Strict kit fidelity over decoration.
```

### Havn — andet forsøg, accepteret

```text
Use case: historical-scene. PROJECT 1864 generic Prussian Baltic Hanseatic port town c.1840, 1536x1024 PNG landscape. Not any named real place. STRICT architectural kit references: image 1 homes sheet Kate/ochre above, white/timber below; image 2 specials farm/merchant above, church/mill below; image 3 exact TWO-storey ochre house, image 4 exact red-brick stepped-gable merchant, image 5 exact medieval brick church. Make a compact port with EXACTLY SIXTEEN BUILDINGS: EIGHT red-brick stepped-gable merchant houses faithfully repeated from image 4 as dense quay frontage and short side street, THREE exact TWO-STOREY ochre houses from image 3 in one back street row, TWO white ONE-storey plaster houses, TWO black-white TWO-storey timber houses, ONE church as image 5. Add ONE rectangular brown POST MILL on trestle in outskirts, four lattice sails, no gallery. No farm, NO Kate, NO extra sheds, no market stalls. ALL OCHRE BUILDINGS MUST HAVE TWO FULL STOREYS BELOW ROOF: two full rows of windows, never single-storey yellow. Preserve the three exact identical ochre models, no other yellow buildings. Tile roofs throughout town. Brick merchants each two storeys plus attic loading hatch/hoist, stepped gable front, central arched entrance. Dense short building rows separated by cobbled lanes and small central market square, Baltic brick character, no ornate modern buildings. Low stone/timber quay, mooring posts, TWO modest wooden period sailing vessels and ONE rowing boat, water along bottom no more than quarter of image. Whole port within frame with margins, do not crop quay-front houses. Camera oblique orthographic-like 38 degrees above horizontal, roofs and two sides, no horizon or sky. Restrained realistically textured miniature style matching reference. Soft daylight, green-ochre fields and small gardens behind town continuing to edges, no plinth or studio grey background. No people, text, labels, cars, electricity, asphalt, railways, factory chimneys, modern cranes, Wilhelmine/Gruenderzeit architecture. Strict reference building shapes take priority over landscape.
```

## Bygninger i de leverede billeder

| Fil | Synligt indhold |
|---|---|
| Town_C_Minor.png | 1 hvidt Putzbau, 1 Kate, 1 Hallenhaus: tre bygninger ved vejforgrening. |
| Town_B_Regional.png | 2 toetages okkerhuse, 1 hvidt enetageshus, 1 toetages Fachwerk, 1 teglkøbmand, 1 kirke, 1 stubmølle: fem byhuse plus kirke, syv enheder i alt. |
| Town_A_City.png | 3 toetages okkerhuse, 4 lyse enetages pudshuse, 2 toetages Fachwerk, 2 teglkøbmænd, 1 kirke: 12 bygninger i kernen. 1 stubmølle udenfor: 13 i alt. |
| Town_Port.png | 9 teglkøbmands-/lagerhuse med trappegavle, 3 toetages okkerhuse, 1 hvidt enetageshus, 2 toetages Fachwerk, 1 kirke: 16 i kernen. 1 stubmølle: 17 i alt. Desuden 2 træsejlskibe, 1 robåd og sten-/trækaj. |

## Kontrol og usikkerheder

Alle fire billeder er visuelt kontrolleret for genkendelige kitformer, etageantal, tekst, mennesker og moderne genstande. Ingen synlig tekst, mennesker eller moderne genstande er accepteret. Alle hovedbygninger er inden for rammen; yderste havnegavl har snæver margin. Byerne har sammenhængende terræn som DK-præcedensen, ikke enkeltbygningernes grå studiebaggrund. Kamera vurderes visuelt, ikke målt.

C og B blev accepteret i første forsøg. B har fem byhuse mod promptens seks, inden for B-kravet på 5–6 plus kirke. Første A blev kasseret for to enetages okkerhuse og spredt komposition. Andet A havde fortsat enetages okkerhuse og blev kasseret. Tredje A har 12 bygninger inklusive kirken, inden for A-kravet på 10–15. Ét ekstra lyst pudshus står ved torvets venstre side; dets facade er varmere creme end referencehvid. Det er accepteret som lys pudsvariant, ikke som det toetages okkerslot. Der er kun to købmænd mod promptens tre og fire lyse pudshuse mod tre; faktiske antal står ovenfor. Ingen Hallenhaus i leveret A.

Første havneforsøg blev kasseret for enetages okkerhuse og uønskede Kate-/gårdtilføjelser. Andet har korrekt toetages okkerrække og ni købmænd mod promptens otte, men ét hvidt hus mod to; samlet 16 bybygninger. De tætte teglgavle giver den ønskede baltiske havnekarakter. Ingen nye pakhusmodeller er opfundet: købmandstypen fungerer også som havnelager. Kasserede forsøg er ikke kopieret til leveringsmappen.

Referencerne gentages genkendeligt, men ikke metrisk identisk: vinduer, tagsten, fagdeling, murværk og skala varierer. Kirkegård, hegn, brønde/fontæne, trækaj, skibe og haver er S: generiske miljøfortolkninger. Kirkegårdssten har ingen læsbar tekst. Det er kompositionsreferencer, ikke Unreal-renders eller dokumentation for en bestemt bys størrelse. Berlin/Königsberg betyder tidlig puds-/teglblanding, ikke storbyens faktiske monumentalitet. Rhinlandet er ikke afbildet.

