# Floder, søer og markskel

## Hvor dataene kommer fra
- **Ikke fra højdekortet:** højdekortet er for groft til at finde floderne i. Prøver man, bliver det til lige streger og søer, der ikke findes.
- **Fra en datafil:** floder og søer står i `Data/Campaign1851/Hydro1851.json`.
  - Floder og kanaler står som punkter (bredde og længde) fra kilden til mundingen.
  - Søer står som ellipser (centrum, halvakser i km og retning).
  - Alt omregnes med kortets egen projektion.
- **Samme stil på hele kortet:** når kortet udvides med flere lande, lægges deres floder og søer i samme fil. De tegnes så på samme måde.
- **Nøjagtighed:** positionerne passer inden for et par hundrede meter til en km.

| Klasse | Eksempler | Bredde i virkeligheden | Tegnet bredde |
|---|---|---|---|
| 0 bæk/å | Lindenborg Å, Vejle Å, Rheider Å, Sorge | 8 m | 90 m |
| 1 å/flod | Storå, Kongeå, Trenen, Stør, Trave, Ejderkanalen | 20 m | 140 m |
| 2 stor flod | Gudenå, Skjern Å, Ejderen, Lagan | 50 m | 220 m |
| 3 Elben | Elben | 400 m | 600 m |

**Med i alt:** 42 floder og kanaler (Jylland, øerne, Slesvig og Holsten, Mecklenburg, Skåne og Halland) og 22 søer (fra Arresø og Esrum Sø til Plønsø, Schweriner See og Müritz).

## På kortet
- **Slyngninger:** hver flod får bløde slyngninger efter sit navn. De er de samme hver gang. Kanaler løber lige.
- **Mundingen:** floden fortsætter til havet, hvor kortets kyst er. En fjord, som kortet har som hav, afslutter floden der.
- **Vandbånd:** floderne er bånd af vand med mørkere bredder. Bække og åer vises sammen med vejene, store floder og søer altid.
- **Søer:** søerne har en lysere bred om mørkere dybt vand.
- **Navne:** større floder får deres navn med kursiv blå skrift, når man er tæt på.
- **Bygninger og træer** står ikke i vandet.

## Broer over floderne
- **Hvor:** hvor en vej mellem to byer krydser en flod eller kanal, er der en bro, fx "Broen over Gudenå ved Randers".
- **Spræng og genopbyg:** broen kan sprænges og genopbygges som de andre broer.
- **Forsvarslinjer:** Kongeåen, Ejderen, Ejderkanalen og Trenen bliver derved linjer, der kan forsvares.
- **Elben** har ingen bro i 1851. Elbbroerne er fra 1872.
- **Ældre gemte spil:** broerne over floderne nummereres efter de andre broer, så de andre broer beholder deres numre.

## Markskel
Gårde og landsbyer får et net af marker omkring sig. Markernes sider er markskel med åbninger til led.

| Skel | Hvor | På kortet |
|---|---|---|
| Knick (levende hegn på jordvold) | Hertugdømmerne og Østjylland | Brun vold med grønne buske |
| Sten- og jorddige | Øerne, Nordjylland og heden | Lav grå mur |
| Grøft | Vestkystens marsk (Tønder, Ribe, Ejdersted, Ditmarsken) | Vandfyldt grøft mellem græskanter |

## Slagmarken
`Battlefield_*.json` har fået nye felter:

| Felt | Indhold |
|---|---|
| `kinds` | Nyt tegn `o` = ferskvand: søer og de bredeste floder |
| `rivers` | `[{name, widthM, points:[x,y,…]}]`. Forløbet har små ekstra slyngninger hver 40 m |
| `lakes` | Søernes bredder som polygoner |
| `hedges` | `[{kind: knick / dike / ditch, points}]`, langs parcellernes sider, som billedet har dem |
| `crossings` | `[{kind: bridge / ford, river, x, y}]` |

**Ved floderne:**
- Langs floderne ligger våde enge (`m`), lidt lavere end markerne omkring.
- Vej over flod: hvor en vej eller sti krydser en flod, er der en lille bro, hvis kampagnekortet ikke allerede har en bro der.
- Markvej over bæk: hvor en markvej krydser en bæk, er der et vadested. Over en større flod er der intet vadested.

**Andelen af skel:** 70 % af parcelsiderne har skel i knick-land, 60 % i marsken og 45 % ellers.

**Billedet:**

| Ting | Udseende |
|---|---|
| Floder | Mørk bred og vand |
| Knicks | Mørkegrøn |
| Diger | Grå |
| Grøfter | Blågrå |
| Broer | Mørkt dæk |
| Vadesteder | Lys grus |
