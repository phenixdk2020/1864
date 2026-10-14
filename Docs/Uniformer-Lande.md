# Uniformer og figurer til landenes militær

Mål: hver hær, vi lader spille eller møde, skal have uniformer og figurer, der passer til landet, våbenarten og tiden. Dette dokument viser, hvor vi står, hvordan figurerne laves, hvordan vi undgår 100 helt separate 3D-modeller, og hvad der skal produceres først.

Dato: 2026-10-14. Kilder: `PROJECT_1864_Lande_og_Uniformer_1851-1866.xlsx` (107 foreslåede uniformmodeller), `3D figur status.xlsx` og mappen `Reference/Units/MultiView` (referencebilleder og 3D-modeller), `StrategyUniformPresetLibrary` i koden.

## 1. Status

| Land / fraktion | Modeller i listen | Lavet eller delvis | Backlog |
|---|---:|---:|---:|
| Danmark | 10 | 5 (3 lavet, 2 delvis) | 5 |
| Sverige | 13 | 5 | 8 |
| Preussen | 10 | 3 (2 lavet, 1 kan genbruges) | 7 |
| Østrig | 9 | 0 | 9 |
| Kongeriget Hannover | 8 | 0 | 8 |
| Mecklenburg-Schwerin | 4 | 0 | 4 |
| Oldenburg | 3 | 0 | 3 |
| Braunschweig | 4 | 0 | 4 |
| Norge | 5 | 0 | 5 |
| Storbritannien | 11 | 0 | 11 |
| Nederlandene | 7 | 0 | 7 |
| Belgien | 8 | 0 | 8 |
| Frankrig | 11 | 0 | 11 |
| Slesvig-Holstens oprørshær | 4 | 0 | 4 |
| **I alt** | **107** | **13** | **94** |

Billeder og 3D i mapperne (efter `3D figur status.xlsx`):
- Dansk: soldat, soldat B, officer, Livgarde-elite, jæger, gardehusar (siddende og stående), dragon (siddende og stående) og kanonmand har front- og A-pose-billeder. 3D (GLB/FBX) og skelet findes for soldat, Livgarde-elite og jæger; husar og dragon har kun GLB.
- Svensk: soldat med 3D, GLB, FBX og skelet. Officer, Livgarde-elite, gardehusar og dragon har kun billeder (delvis).
- Preussisk: intet endnu i status. Mapperne `Prussisk Infantry/Cavalry/Gunbattery` findes.
- Fælles: hest (med gangart), riffel, riffel med bajonet, sabel, skede, sadel, morter, kanon og fem flagtyper har billede og 3D.

Vigtig advarsel: listen starter Sverige i 1860 og Preussen i 1864, men kampagnerne starter 1. juli 1825 og 1. juli 1851 (1864 er reference). Alle lande skal derfor have uniformer for **tre perioder** (omkring 1825, 1851 og 1864), ikke kun én. De danske tropper i 1825 havde røde kjoler og lyseblå bukser (se `Army_1825.json`), 1864 mørkeblå. Listen dækker i dag stort set kun 1864.

## 2. Sådan laves en figur i dag

1. Billede: A-pose forfra, bagfra og fra siden samt en pose forfra (PNG, genereret i ChatGPT billedgenerering ud fra en beskrivelse; fx `DK_Infanteri_1864_Apose_Front.png`).
2. 3D: tekstureret GLB/FBX ud fra billederne (`..._textured.glb`/`.fbx`).
3. Skelet: automatisk rig med 41 Mixamo-knogler (`..._textured_skeleton.fbx`). Alle figurer deler skelettet `SK_Human_1864`, så animationer kan genbruges.
4. Animationer: Mixamo-klip lagt på det delte skelet (se `Content/Units/Human/CombatAnimations`).
5. Import i Unreal (`import_units_1864.py`), farver og detaljer styres af en uniformspreset (`StrategyUniformPresetLibrary`: frakke, bukser, opslag/facings og accent).
6. Bagning af figuren til mængdetegning (VAT) i `bake_all_vat.py`, kun infanteri indtil videre.

Det er omkring ti trin pr. figur (billede, 3D, skelet, import, bagning), så 94 uniformer, hver for tre perioder, er ikke realistiske som separate modeller.

## 3. Strategi: færre modeller, flere varianter

1. **Én krop pr. våbenart og stil, ikke pr. regiment.** Samme grundsnit af infanteriuniform genbruges (modelprincippet i regnearket). Forskellen mellem lande og regimenter lægges oven på:
   - **Farver** via uniformspresets (frakke, bukser, opslag, accent). Koden har i dag presets for neutral, Danmark (rød frakke, mørkeblå bukser), Preussen (mørkeblå frakke, røde opslag) og dansk artilleri. En preset pr. land, våbenart og periode er billig og lader os lave hundredvis af varianter uden nye 3D-modeller.
   - **Hovedbeklædning** som egen mesh pr. type (shako, pickelhaube, kasket, czapka, husarbusby, kask, tschako, lue), da den giver mest visuel forskel.
   - **Udstyr**: tornyster, patrontaske, bajonet og gevær som separate rekvisitter.
2. **Fælles kropstyper:** infanteri (linje), let infanteri/jæger, garde (samme krop som linje, anden hovedbeklædning og snore), kavaleri (siddende rytter plus hest), artilleri (kanonmand), officer og tromme/musik.
3. **Nye 3D-modeller kun, hvor snittet er helt anderledes** (kyras, britiske højlændere, zouaver, østrigske Grenzer-frakker, uhlan-czapka).
4. **Perioder som profiler:** en `EraProfile` pr. land (1825, 1851, 1864) vælger preset, hovedbeklædning og våben. Det er den manglende `FEraProfile` fra acceptancetest T8.

## 4. Prioritet (efter hvad der skal kunne køres)

| Prioritet | Hvad | Hvorfor |
|---|---|---|
| 1 | Danmark 1825, 1851, 1864: linje, garde, jæger, husar, dragon, artilleri, officer | Spillerens hær i alle tre scenarier; i dag kun 1864 |
| 2 | Sverige-Norge 1851 og 1864: linje, garde (Svea og Andra Livgarde), feltjæger, husar, dragon, artilleri; Norge: linje, jæger, dragon, artilleri | Første modstander, vi bygger land til (se `Landeskabelon.md`) |
| 3 | Preussen 1851 og 1864: musketer, fusilier, jæger, garde/grenadier, husar, uhlan, dragon, kyrassér, artilleri | Hovedmodstander i 1864 |
| 4 | Østrig 1864: linje, jæger, Grenzer, husar, uhlan, kyrassér, artilleri | Preussens allierede i 1864 |
| 5 | Hannover og Sachsen 1863 (forbundseksekutionen) | Små, korte kampagner |
| 6 | Mecklenburg, Oldenburg, Braunschweig, Slesvig-Holstens oprørshær (1848-51) | Nabohære i scenarierne |
| 7 | Storbritannien, Frankrig, Nederlandene, Belgien | Diplomatiske spillere, sjældent på slagmarken |

## 5. Hvad der skal bruges for hver uniform (til billedet)

For hver uniform skal vi have kilde og beskrivelse af:

- Hovedbeklædning (type, farve, plume/pompon, messingplade, snore).
- Frakkens snit (kjole, kollet, tunika), farve, knapper og række, opslag/kraver/manchetter (regimentsfarver), epauletter og skuldersnore.
- Bukser (farve, strømper, støvler eller gamacher) og kappe.
- Udstyr: remme (hvide/sorte), patrontaske, tornyster, brødpose, feltflaske, bajonet.
- Våben (musket, kapselgevær, riffel, karabin, sabel, lanse) med år.
- Distinktioner for rang, kompagni og regiment.
- Periode (1825, 1851 eller 1864) og en kilde (museum, regelement eller samtidigt billede).

Skabelon til billedgenerering (ChatGPT): *"Full-body soldier of [land, våbenart, regiment, år], A-pose, arms slightly away from the body, neutral grey background, even soft light, [hovedbeklædning, frakke, bukser, udstyr, våben], period-accurate 19th-century uniform, front view / back view / side view as three separate images, no text."* Samme beskrivelse bruges til alle tre vinkler, så figuren er ens.

## 6. Næste skridt

1. Codex undersøger uniformerne pr. land og periode (se Codex-opgaven i `Uniformer-agent`) og skriver en beskrivelse med kilder og en færdig billedprompt pr. uniform.
2. Du genererer billeder (A-pose forfra, bagfra og fra siden) efter promptene. Billedfiler lægges i `Reference/Units/MultiView/<Land> <våbenart>`.
3. Jeg bygger en uniformspreset pr. land, periode og våbenart (farver og hovedbeklædning), så nye figurer kan genbruge den danske eller svenske krop.
4. Ved nye silhuetter laves en 3D-model; resten løses med presets.
5. Genbagning med de nye figurer (kræver en lukket editor).

Alt i dette dokument er en plan; ingen nye figurer er lavet endnu.
