# Slagmarksgeneratoren

Knappen **SLAGMARK** (ved siden af AVISEN), eller **SE SLAGMARKEN** i slagpanelet, bygger terrænet til et 3D-slag ud fra stedet på kortet. Enhederne kommer senere.

## Område og opløsning
- Et kvadrat på **4, 8 eller 12 km** omkring midten af kortets udsnit, eller omkring slaget.
- Et gitter på **256 × 256 felter**, dvs. 31 m pr. felt ved 8 km.
- Samme sted og seed giver samme slagmark.

## Hvad der kommer med
| Lag | Kilde |
|---|---|
| Højde | Kortets højdedata (ca. 126 m pr. pixel, fuld skala ca. 172 m, skøn) plus en fin relief af knolde og lavninger fra seed |
| Hav | Kortets kystlinje |
| Eng og strand | Lavt land (under 3 m) inden for ca. 350 m af havet |
| Skov | Kortets skovdata, med takkede kanter og lysninger |
| Byer | Bebygget areal med radius 0,25 × √(indbyggere/1000) km. Huse langs et gadenet, tættere mod midten |
| Landsbyer, gårde og husmandssteder | De samme som på kampagnekortet. En landsby er kirke og en snes huse og gårde |
| Ekstra gårde | Firlængede gårde spredt på markerne |
| Veje | Kortets landeveje og markveje, chausséerne ovenpå og åbne jernbaner. Dertil lokale veje mellem landsbyer og byer og ud til landevejen, og markveje fra hver gård til nærmeste vej |
| Bygninger | Garnisonsbygninger og civile bygninger |
| Skanser | Kortets skanser med kanoner, forsvar og løbegrave |
| Tid og vejr | Dato, årstid, vejr, temperatur og sne |

## Filer i `Saved/Battle/`
### `Battlefield_<navn>.json` (formatet `PROJECT1864-Battlefield-1`)
- **Koordinater:** origo i det sydvestlige hjørne, x mod øst og y mod nord, i meter.
- **Gitterdata:**
  - `heightDm`: 256 × 256 højder i decimeter, række 0 mod syd.
  - `kinds`: ét tegn pr. felt: `.` mark, `~` hav, `m` eng, `w` skov, `t` by, `o` ferskvand (sø eller bred flod).
  - `rivers`, `lakes`, `hedges` (knicks, diger, grøfter) og `crossings` (broer og vadesteder): se Hydro1851.md.
  - `woodDensity`: `0`–`9` pr. felt.
- **Linjer:** `roads`, `lanes`, `tracks`, `chaussees` og `railways` som punktlister `[x, y, x, y, …]`.
- **Objekter:**
  - `buildings`: kind, x, y, w, d og yaw.
  - `towns`: navn, x, y og radius.
  - `forts`: id, navn, x, y, frontYaw, large, guns, defence, built og trenchesTo.

### `Battlefield_<navn>.png`
Billedet på 512 × 512: højdeskygge fra nordvest, højdekurver for hver 5 m, marker i parceller, skov, eng, by, veje, huse og skanser.

## Til 3D-slaget
`BattleRequest_N.json` har feltet `battlefieldFile`. Det er slagmarken på 8 km omkring slaget, og den bygges i samme øjeblik.

Test: `-CampaignBattlefield=lat,lon,km -CampaignOpenWindow=battlefield`.

## Gå ind på slagmarken
Knappen **GÅ IND PÅ SLAGMARKEN** i vinduet SLAGMARK bygger den genererede slagmark i 3D. Modellen står langt ude ved siden af kampagnekortet.

**Modellen:**
- **Skala:** 5 enheder pr. meter. Højden er overdrevet 1,5 gange.
- **Jorden:** billedets farver i 512 × 512 felter, uden veje.
- **Oven på jorden:** veje, chausséer, markveje, jernbaner og åer som bånd; huse, gårde og kirker; skov som træer; markskel som knicks, diger eller grøfter.
- **Rydning:** der står ingen træer på veje og vand eller tæt ved huse.

**Kameraet** styres som på kortet. TILBAGE TIL KORTET fører tilbage til det sted, hvor man var på kortet.

**Imens:**
- Spillet står på pause.
- Kortets skilte er skjult.
- Der er ingen enheder endnu.

**Test:** `-CampaignOpenWindow=battlefield -CampaignBattlefield=lat,lon,km -CampaignBattleView`
