# Norske byer på kampagnekortet

Dato: 2026-10-22; udført 2026-10-23. Generiske miniaturebyer ca. 1825–1864, ikke gengivelser af virkelige byer. Alle fire billeder er genereret med det indbyggede image_gen-værktøj i PNG, 1536 × 1024.

Grundlag: ../PROMPTS.md, Docs/Bygninger-NO-Research-2026-10-22.md og de otte godkendte Iso-billeder i overmappen. V-oplysninger og kilder findes i researchdokumentet; byplaner, farvefordeling, terræn, pælekonstruktioner og antal er S: kompositionsskøn. Ingen ændring af spillets scenarier.

## Visuelle referencer

To midlertidige 2048 × 2048-referenceark med uændrede 1024 × 1024-Iso-billeder, samlet 2×2 uden tekst. Boligark: Cottage_Laft_NO / TownHouse_Ochre_NO øverst; TownHouse_White_NO / TownHouse_Timber_NO nederst. Specialark: Farm_Tun_NO / MerchantHouse_NO øverst; Church_Wooden_NO / Windmill_Watermill_NO nederst. Begge blev oprettet midlertidigt i Saved/Codex. Alle kald bruger disse to ark samt købmandshus, kirke og kvernhus individuelt: præcis fem referencefiler. Arkene er slettet efter brug.

## Prompter

Den fulde engelske prompt er fællesprompten nedenfor plus den respektive byprompt, i denne rækkefølge. De tre detailreferencer er tilgængelige, men en byprompt kan udtrykkeligt udelukke disse typer.

### Fælles prompt

```text
Use case: historical-scene. PROJECT 1864 campaign-map town visualisation, generic Norway c.1825–1864, not a copy of any real town. One landscape 1536x1024 PNG. Use supplied approved *_Iso images as strict architectural kit references, preserving materials, colours, proportions, storeys, silhouette and restrained realistically textured miniature rendering style. Repeat these SAME buildable types, do not invent architecture except the explicitly requested port warehouses. Oblique orthographic-like camera 40 degrees above horizontal, see roofs and two sides, no horizon. Soft daylight, gentle contact shadows. Buildings sit directly on continuous painted green-ochre Norwegian countryside with small cultivated field patches, rocks, birch trees and dirt roads extending out of frame, no floating plinth or studio grey background. Red-orange clay tile on town buildings; green-brown sod over birch bark on log cottages, farm and watermill; dark timber shingles on white wooden church. No text, labels, people, animals, modern items, asphalt, electric poles, cars, industrial chimneys, modern glazing, elaborate German half timber, dragon ornament or windmill sails. Reference image 1 is a 2x2 kit sheet: top left log cottage, top right TWO-storey ochre boarded town house, bottom left ONE-storey white boarded town house, bottom right TWO-storey red boarded town house. Image 2: top left detached four-building farm tun with raised stabbur, top right TWO-storey pale stucco merchant house, bottom left white wooden long church with attached tower, bottom right small log watermill. Images 3,4,5 repeat merchant, church, watermill for detail. 
```

### Town_C_Minor

Fil: Town_C_Minor.png

```text
Tier C minor Norwegian settlement: exactly THREE small houses loosely arranged at a dirt crossroads: TWO Cottage_Laft_NO and ONE TownHouse_White_NO. All three countable and fully visible with generous countryside around. No church, merchant, mill, farm compound or additional buildings. Low small rural village.
```

### Town_B_Regional

Fil: Town_B_Regional.png

```text
Tier B regional Norwegian timber town: exactly SIX secular buildings plus ONE church. Two TWO-storey ochre panel houses, two TWO-storey red panel houses, one ONE-storey white panel house, one TWO-storey pale stucco merchant house with broad gateway and loading hatch. Arrange these six along both sides of a short bending dirt main street with a small cobbled market square; the ONE white wooden church stands beside the square, retaining its exact nave, lower chancel and dark slender tower spire. Whole town fully inside frame, country margin. No farms, mills, extra sheds or additional buildings. Seven buildings total.
```

### Town_A_City

Fil: Town_A_City.png

```text
Tier A Norwegian city miniature with Christiania/Bergen-type timber and stucco character, not a named real place: exactly FOURTEEN buildings total, tightly composed but countable. FOUR two-storey ochre panel houses, FOUR two-storey red panel houses, THREE one-storey white panel houses, TWO two-storey pale stucco merchant houses, ONE white wooden long church. Three short street rows and a clear small cobbled market square; urban density distinctly larger than B, all buildings entirely visible. Preserve kit roof silhouettes and floor counts; no houses painted white in two storeys. No additional buildings, farms, mills, harbour or warehouses. No later ornate apartment blocks. Keep streets, green back gardens and surrounding birch countryside visible.
```

### Town_Port

Fil: Town_Port.png

```text
Generic Norwegian wooden harbour town, no named place or exact Bryggen copy: six TWO-storey panel town houses (three ochre, three red), two ONE-storey white panel houses, one TWO-storey pale stucco merchant house, one white wooden long church. Add exactly THREE modest tall gable-front TIMBER WAREHOUSES (two iron-oxide red, one dark tar), narrow rectangular three-level forms with red tile gabled roofs, shuttered loading hatches and simple timber hoist beams, raised on visible timber piles at the water edge, connected by a wooden brygge boardwalk. These warehouses are the sole additional architectural type, period-generic. Quay along a sheltered blue-grey fjord bay, one modest wooden sailing boat and one rowboat, no people. Compact timber street behind warehouses, rising green rocky birch hillside in background, no horizon. All thirteen buildings fully in frame with margins, believable shore meeting piles, no modern harbour infrastructure, no extra buildings or mill. Retain kit proportions and TWO full floors in red/ochre town houses.
```

## Bygninger i de leverede billeder

| Fil | Synligt indhold | Antal |
|---|---|---|
| Town_C_Minor.png | To Cottage_Laft_NO og ét TownHouse_White_NO ved vejkryds. | 3 bygninger |
| Town_B_Regional.png | Ét okker panelhus, to røde panelhuse, ét hvidt panelhus, ét pudset købmandshus og én trækirke. | 5 verdslige bygninger + kirke = 6 |
| Town_A_City.png | Tre okker panelhuse, tre røde panelhuse, tre hvide panelhuse, ét pudset købmandshus og én trækirke ved torvet. | 10 verdslige bygninger + kirke = 11 |
| Town_Port.png | To okker panelhuse, to røde panelhuse, to hvide panelhuse, ét pudset købmandshus, én trækirke, tre havnepakhuse (to røde, ét mørkt) samt ét firebygningstun med stabbur bag byen. | 11 fritstående by-/havnebygninger + 1 tun = 12 scenery-objekter; 15 fysiske bygninger |

Port har desuden én mindre træsejler og én robåd. Tunet tælles som ét samlet scenery-anlæg i objektoptællingen, men som fire huse i den fysiske optælling.

## Kontrol og usikkerheder

S: Billederne er kompositionsreferencer; gentagne huse kan variere i vinduesrytme og mindre proportioner. Hvidt bruges sparsomt i 1825; byernes blanding kan også repræsentere senere år inden for perioden. Havnepakhuse er generiske miljøbygninger, ikke en niende enkeltbygningsmodel. Ingen ekstra bygnings-PNG eller 3D-model leveres til pakhuset.

Alle fire billeder er visuelt gennemgået for kamera, kittyper, etageantal, materialer, tekst, mennesker og moderne genstande. Ingen synlig tekst, mennesker eller moderne genstande er accepteret. Alle fire blev accepteret i første forsøg; ingen byvariant er kasseret.

Afvigelser fra promptens mål: B har ét okkerhus i stedet for to og dermed fem verdslige bygninger; det er inden for briefens 5–6 plus kirke. A har tre okker, tre røde, tre hvide og én købmand i stedet for fire/fire/tre/to; elleve bygninger inklusive kirken ligger inden for briefens 10–15. Port har to okker og to røde huse i stedet for tre af hver samt et ekstra tun fra specialarket i baglandet. Dette er accepteret som en generisk havnekomposition med klart læsbare kitformer. Den faktiske optælling ovenfor gælder frem for promptens måltal.

Accepterede begrænsninger: Kirken gengives med mindre detaljevariation og kan mangle et lille kors fra enkeltreferencen. Havnepakhusenes præcise pæle, hejseværker og etageinddeling er generatorens fortolkning. Små forhaver, brønde, stengærder, kirkegård, hegn og miljødetaljer er tilføjet som periodisk kompositionsskøn (S), ikke som yderligere godkendte bygningsmodeller. Landskabet er mere tekstureret end flad kortmaling. Kameraet er visuelt vurderet, ikke målt. Billederne dokumenterer ikke identisk gentaget 3D-geometri eller en faktisk Unreal-rendering.

Alle leverede bybilleder er kontrolleret til 1536 × 1024 PNG. De to midlertidige referenceark og kontaktarket til studiet af DK-præcedensen er slettet. Ingen referenceark leveres som spilasset.
