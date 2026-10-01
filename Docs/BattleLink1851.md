# Kampagnen og 3D-slaget

Slag-prototypen (Unity, v00.00.09f30x) og kampagnen (Unreal) skal tale samme sprog. Den samlede historik for slaget og kampagnen står i `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md`.

Dette dokument beskriver tre ting:
- hvad kampagnen sender til slaget;
- hvordan forskning og doktriner rammer slagets egne systemer;
- hvad der mangler på begge sider.

## 1. Enhederne: fra kampagnens bataljon til slagets kompagnier
I `Units.json` har hver enhed nu feltet `battle`:

| Kampagne | Slaget (F30) | `battle.type` | `subunits` |
|---|---|---|---|
| Linjebataillon (760) | Bataljon under major. To bataljoner er et regiment under en oberstløjtnant | `infantry_battalion` | 4 × `company` à ca. 190, hver med sin kaptajn |
| Livgarden, jægerkorps | Som ovenfor | `guard_battalion`, `jager_battalion` | Kompagnier |
| Gardehusarregimentet (480) | Gardehusar, 120 ryttere | `hussar_regiment` | 4 × `squadron` à 120 |
| Dragonregiment (560) | Dragon, 140 ryttere, SID AF / STIG OP | `dragoon_regiment` | 4 × `squadron` à 140 |
| Batteri, ridende batteri | Artilleri (ikke lavet i slaget endnu) | `foot_battery`, `horse_battery` | `battery` med kanoner og morterer |

**Hvert kompagni har:**
- **Mænd:** kun dem, der er ved fanerne (`presentMen`), fordelt på kompagnierne.
- **`establishment`:** kompagniets fulde styrke, 190 mand.
- **`fortId`:** et kompagni, der ligger i en skanse, har 0 mand i feltet og skansens id.
- **Kaptajn:** navn, grad, erfaring og de ti officersegenskaber. Egenskaberne er føring, inspiration, initiativ, taktik, stab, disciplin, aggressivitet, nerve, politisk vægt og forsigtighed. Slagets officer-AI kan bruge dem.

**Hver enhed har også:**
- **`commander`:** chefen, svarende til slagets Major-HQ.
- **`general`:** en general, hvis hans hovedkvarter følger med enheden.
- **`formation`:** hvilken formation enheden hører til.
- **`attachment`:** altid `organic`. Midlertidige tilknytninger er slagets egne.

**`formations`** står øverst i filen. Det er felthærens kommandotræ:
- hver formation har id, navn, `echelon` (`ARMY`, `XX`, `X`, `III` eller `DET`) og `parent`;
- hver har chef, stedfortræder og stabschef.

Det svarer til slagets XX Division → X Brigade → III Regiment → II Bataljon → I Kompagni.

## 2. Forskning, der rammer slaget
Forskningstræet har nu 7 grene. De nye emner er bygget på systemer, der findes i slag-prototypen.

| Gren | I | II | III |
|---|---|---|---|
| Sanitet og forsyning | Sanitetsvæsenet | Konserves | Militærhospitaler |
| Befæstning | Fæstningsbyggeri | Kasematter | |
| Samfærdsel | Felttelegrafen | Jernbanemobilisering | **Pontonnerkorpset** |
| **Infanteriet** | **Karré-eksercitsen** | **Kædelinjer og jægertaktik** | Bagladegeværet |
| Artilleriet | Riflede kanoner | | |
| **Kavaleriet** | **Rytterspejdning** | **Dragonernes ildkamp** | **Rytterchokket** |
| **Kommando** | Stabsskolen | **Generalstaben** | |

Ændringer i rækkefølgen:
- Bagladegeværet kræver nu Kædelinjer og ikke længere Stabsskolen.
- Stabsskolen er flyttet til grenen Kommando.

Virkningerne sendes til slaget i `battleRules`, som står i både `Units.json` og `BattleRequest_N.json`:

| Felt | Uden | Med | Forskning | Slagets system (F-version) |
|---|---|---|---|---|
| `infantry.reloadFactor`, `proneLoading` | 1,0 / nej | 0,35 / ja | Bagladegeværet | Våbenprofil og ladetid |
| `infantry.squareFormTimeFactor` | 1,0 | 0,7 | Karré-eksercitsen | KARRÉ, formationstid (F29) |
| `infantry.squareFaceFireShare` | 0,25 | 0,30 | Karré-eksercitsen | Karréens fire sider skyder hver for sig (F29Z) |
| `infantry.concealmentFactor` | 1,0 | 1,25 | Kædelinjer | Dækning i afgrøder (F29R) |
| `infantry.skirmishFactor` | 1,0 | 1,15 (× 1,1 med Spredt orden) | Kædelinjer | Skyttekamp |
| `infantry.deployBufferM` | 35 | 35 | | Udfolder sig til linje 35 m uden for fjendens rækkevidde (F30X) |
| `cavalry.carbineRangesM` | 35/70/100 | 45/90/130 | Dragonernes ildkamp | Afsiddede dragoners ild, NÆR/MELLEM/LANG (F30J/R) |
| `cavalry.carbineReloadS` | 7 | 5 | Dragonernes ildkamp | Ladetid i test (F30R) |
| `cavalry.reformSpeedFactor` | 1,0 | 1,25 | Rytterchokket | Fysisk omformering (F30H) |
| `cavalry.flankShockFactor` | 1,0 | 1,2 | Rytterchokket | Angreb i FRONT/FLANKE/RYG (F30) |
| `cavalry.reconOrder` | nej | ja | Rytterspejdning | Ordren SPEJD HER (F30Q, venter på FOG/LOS) |
| `artillery.rangeFactor`, `accuracyFactor` | 1,0 | 1,4 / 1,3 | Riflede kanoner | Artilleri (ikke lavet i slaget endnu) |
| `command.reachBandsM` | Major 320/450, Regiment 800/1100, Brigade 1350/1850, Division 2100/2850 | × 1,15 pr. emne | Stabsskolen, Generalstaben | Kommandozonerne (F30P) |
| `command.orderDelayFactor` | 1,0 | 0,75 | Generalstaben | Ordreforsinkelse (når den kobles på zonerne) |
| `command.higherHqAi` | nej | ja | Generalstaben | Brigade- og division-AI (F30M/O) |
| `engineering.pioneerBridge` | nej | ja | Pontonnerkorpset | Floder kan kun krydses ad broer (F9, F30H). Med emnet kan pionererne slå en bro |
| `engineering.fortCoverBonusPercent`, `lossFactor` | | | Befæstning, sanitet, doktrin | Dækning og tab |

**Virkninger i kampagnen:**

| Forskning | Virkning |
|---|---|
| Karré-eksercitsen | Kampværdi +2 % |
| Kædelinjer | Tab −10 % |
| Rytterspejdning | Rytteriet ser 30 km i stedet for 20 km |
| Dragonernes ildkamp | Kampværdi +2 % i slag med rytteri |
| Rytterchokket | Kampværdi +3 % i slag med rytteri |
| Generalstaben | Kampværdi +5 % i slag med 3 eller flere enheder |
| Pontonnerkorpset | Pontonbroer koster 15.000 rd. i stedet for 25.000 og tager 22 dage i stedet for 45 |

## 3. Doktrinen bliver slagets AI-standard
Feltet `aiDefaults` står i både `Units.json` og `BattleRequest_N.json`.

| Kampagnens doktrin | Slaget |
|---|---|
| Operativ: Forsvar i dybden | `higherDoctrine: DEF`: hele bataljoner holdes i reserve (F30Q) |
| Taktisk: Bajonetangreb (uden Forsvar i dybden) | `higherDoctrine: OFF`, `chargeAtWill: true`, `firePolicy: CLOSE` |
| Ellers (Koncentration) | `higherDoctrine: BAL`: begge bataljoner frem (F30Q) |
| Taktisk: Ildkamp | `firePolicy: LONG` |
| Taktisk: Spredt orden | `firePolicy: MED`, `openOrder: true` |
| Strategisk: Fæstningen | `holdForts: true` |

## 4. Slagmarken passer til slagets terræn
Slagets terræn findes i prototypen. Slagmarksfilen giver det tilsvarende:

| Slaget har | Slagmarksfilen giver |
|---|---|
| Afgrøder med dækning (F29R) | Marker i parceller, med hegn langs dem (`hedges`) |
| Floder, der kun kan krydses ad broer (F9, F30H/W/X) | `rivers` og `bridges`, nu også små broer og vadesteder (`crossings`) |
| FencePosts | `hedges` af typerne knick, dige og grøft. Prototypen kender kun hegnspæle endnu |

## 5. Mangler
**I slaget:**
- Artilleri, så batterier, kanoner og morterer i `Units.json` kan bruges.
- Kavaleriets skydevåben til hest, og chok med tab af heste.
- Fjender, der ikke ses overalt (FOG/LOS). Først da kan SPEJD HER bruges.
- Ordreforsinkelse koblet på kommandozonerne.
- Pionerer, der kan slå broer.
- At læse `battleRules`, `aiDefaults`, `subunits` og `formations` fra kampagnens filer.
- `BattleResult_N.json` med tab pr. kompagni og eskadron, og døde eller sårede officerer.

**I kampagnen:**
- At læse tab pr. kompagni og officerernes skæbne fra slagets resultat. I dag læses kun tal for hele enheden.
- At danne regimenter af to bataljoner (`III`) automatisk, når hæren sættes på krigsfod.
