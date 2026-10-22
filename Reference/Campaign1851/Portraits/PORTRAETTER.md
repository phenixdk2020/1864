# Udenlandske portrætter — PROJECT 1864

Dato: 20. oktober 2026. Alle ansigter er generiske, fiktive personer. Ingen navngivne historiske personer eller kunstnere er brugt i prompterne.

## Sæt og filnavne

| Nation | Officer_Ung (25–35) | Officer_Aeldre (40–50) | Officer_Gammel (55–65) | General (55–70) | Minister (40–70) | I alt |
|---|---:|---:|---:|---:|---:|---:|
| SE — Sverige | 10 | 10 | 6 | 6 | 10 | 42 |
| NO — Norge | 10 | 10 | 6 | 6 | 10 | 42 |
| PR — Preussen | 10 | 10 | 6 | 6 | 10 | 42 |
| I alt | 30 | 30 | 18 | 18 | 30 | 126 |

Filer: `<NAT>/T_Portrait_<NAT>_<SÆT>_<NN>.png`. Numre starter ved 00; 10 billeder slutter ved 09, 6 billeder ved 05. Format: 512 × 640, 8-bit RGB PNG. Genererede originaler tilpasses til 4:5 og nedskaleres med Lanczos. Smalle mørke bånd øverst og nederst følger referenceformatet.

## Reference og promptskabelon

Visuelt undersøgte danske referencer: `T_Portrait_Officer_Ung_00.png`, `T_Portrait_General_02.png` og `T_Portrait_Minister_03.png`. Første kandidatserie brugte officers-/ministerportrætterne direkte som stilreference. 18 kandidater blev kasseret, fordi ansigterne lignede hinanden for meget. Kun `SE/Officer_Ung_00` blev beholdt fra den serie.

Den endelige serie bruger nedenstående skabelon uden vedhæftede referencebilleder, så referenceansigtet ikke kopieres. Hver fil genereres separat. Nation, kategori, alder, ansigtsform, næse, øjne, hårgrænse, skæg, kropsbygning, hudtone og vendingsretning varieres. `SE/Officer_Ung_01` var prøvebilledet for den nye metode og bruger en kortere variant med bredt rundt ansigt, fregner og kort vigende hår.

```text
Use case historical-scene. ONE standalone 4:5 oil painting bust portrait
for 19th century strategy game, fictional unnamed {nationality}
{officer / general / civilian minister}, age {category age range}.
Essential unique anatomy: {head shape, jaw, nose, eyes}, {hair}, {beard}.
{Age-appropriate wrinkles, greying or youthful skin.}
{Build and complexion.} {National uniform or civilian clothing.}
Bareheaded, clothing circa1825-1864. Head and shoulders tight bust,
whole head visible, three quarter face turned {left / right}, sober calm
expression. Fine realistic nineteenth century oil paint with softly blended
studio light and delicate brush texture, warmly illuminated face upper left,
deep warm umber almost black background, narrow dark painted band along top
and bottom. Close museum painting game-card framing, warm muted colours,
not a photograph. Invent a distinct new face, no actual or named historical
person, no artist names. No hands, text, captions, watermark, modern objects
or collages.
```

Nationale promptdele:

- SE: `Swedish navy blue military coat with yellow or pale blue collar facings and gilt buttons, Swedish cut; Sword order inspired medals for generals.`
- NO: `Norwegian grey-blue military tunic with narrow red collar facings and small gilt lion button badges, Norwegian cut distinctly different from Danish; St Olav inspired medals for generals.`
- PR: `Prussian dark navy Waffenrock coat with narrow red collar, red piping, gilt buttons, characteristic Prussian cut and shoulder cords; black Prussian eagle order inspired medals for generals.`

Ansigtsvariationer omfatter firkantede, smalle, runde og ovale ansigter, forskellige næser og kindben, lyst, rødligt, brunt og sort hår samt glatbarbering, bakkenbarter, overskæg og skæg. Aldersgrupperne har alderssvarende gråning. Nationerne forskyder rækkefølgen af ansigtstræk. Visuel kontrol er nødvendig; promptvariation alene garanterer ikke forskellige identiteter.

## Historiske usikkerheder

Dette er plausible spilportrætter for perioden, ikke verificerede gengivelser af konkrete regimenters uniformsreglementer. Facings, knapmotiver, skulderdistinktioner og ordener er malede fortolkninger; små løve-, sværd- og ørnemotiver kan ikke garanteres heraldisk korrekte. Sverige og Norge er særskilte billedsæt trods periodens union. Svärdsorden eksisterede fra 1748; se [Kungl. Maj:ts Orden](https://kungligmajestatsorden.se/english/orders/the-order-of-the-sword). Preussisk mørkeblåt/rødt overlapper danske farver; snit, skulderdistinktioner og detaljer skal give forskellen.

Generalsæt med St. Olav-inspirerede ordener repræsenterer den senere del af perioden (ordenen indstiftet 1847; se [Det norske kongehus](https://www.kongehuset.no/monarkiet/ordener-og-medaljer/st-olavs-orden)). Waffenrock-snit repræsenterer også den senere del af perioden. Billederne er ikke opdelt efter scenarieår, så brug i 1825 kan give anakronistiske uniforms-/ordensdetaljer. Ingen Pickelhaube er bestilt, så hjelmens introduktion i 1842 giver ikke en ekstra scenarieafhængighed.

De lokale MultiView-mapper er referenceadgang, ikke dokumentation for alle officersgrader og civil dragtskik. Der hævdes ikke kildeverificeret præcision af disse detaljer eller et garanteret eksakt stilmatch.

## Valg og import

Overlayet indlæser `/Game/Campaign1851/Portraits/<NAT>/T_Portrait_<NAT>_...`. Officersnation sendes fra `FCampaign1851Officer::Nation`. Ministre har i det aktuelle repo ingen `Nation`; kabinettilhørsforhold findes gennem `ForeignMinister(nation, portfolio)`. Hjælpefunktionen sammenligner det konkrete ministerobjekts adresse med kabinetternes objekter og sender nationsnøglen til portrætvalget. Dermed forveksles enslydende svenske og norske navne ikke. Hjemlige ministre beholder DK. Ingen data- eller savegame-strukturer ændres.

Valget er deterministisk med eksisterende `GetTypeHash(Name)` modulo puljens størrelse. Udenlandske generaler bruger generalsættet; øvrige officerer bruger samme aldersgrænser som før (38 og 50). Manglende sæt eller ugyldig valgt brush falder tilbage til uændret dansk valg. Indtil manuel import vises dansk fallback.

`Content/Python/import_foreign_portraits.py` kontrollerer alle 126 kildefilers format før import. Indstillinger følger `Tools/Campaign/import_building_cards.py`: `TC_EDITOR_ICON`, `TEXTUREGROUP_UI`, `TMGS_NO_MIPMAPS`, eksplicit sRGB. Importen erstatter kun de udenlandske destinationsassets. Scriptet er skrevet, men ikke kørt. Ingen build, spilstart eller commit.

## Kvalitetskontrol

Afventer afsluttende billedkontrol og filoptælling.

Kasseret fra første kandidatserie: SE Officer_Ung 01–09 og SE Officer_Aeldre 00–08 (18 billeder), på grund af for ens ansigter. De erstattes af selvstændige nye generationer, ikke ændrede kopier.
