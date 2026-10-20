# Import af Preussen — 2026-10-16

Opdatering 2026-10-17: Den efterfølgende niveau B-integration er beskrevet i [Niveau-B-Sverige-Norge-Preussen-2026-10-17.md](Niveau-B-Sverige-Norge-Preussen-2026-10-17.md). Det nedenstående beskriver den tidligere importstatus og dens oprindelige begrænsninger. Niveau B er implementeret og statisk valideret; runtime er ikke afprøvet.


**Niveau B er opnået for kortdata (byer, garnisoner, felthære, veje, distrikter, relation); Preussen er ikke spilbart (ikke niveau C).** 1825 og 1851 er importeret hver for sig. Ingen build, editor-/spilstart, spiltest eller commit. Events er aktiveret senere samme dag (afsnittet Begivenheder); kun event-loaderen er ændret i C++.

Kilde: `C:/Users/allan/Documents/Codex/2026-10-12/l/outputs/Preussen-agent/Spilimport/` (21 hashede filer, alle kontrolleret mod `Manifest.json`). Pakken melder selv `readyForImport=false` og `readyForBattle=false`: den er et delvist overlay, og alle ukendte tal er `null` med begrundelse. Ingen null er gjort til nul.

## Hvad pakken faktisk indeholder

Pakkens PR-byer, -distrikter, -ruter, -garnisoner og -felthære er **de samme 25 byer, 13 distrikter, 33 ruter og 34 hærposter, som spillet allerede havde** (Preussen var allerede delvist niveau B). Tallene er identiske; pakken tilføjer `fieldStatus` (status, kilde og begrundelse pr. tal), noter og null-poster. De eneste ændrede tal er Preussens nationale befolkning (kilde HGIS). Resten af pakken (hærkatalog, officerer, økonomi, events, provinsbefolkning, handel) har ikke noget kompatibelt, aktivt modtagersted uden at ændre dansk spil og er arkiveret (se nedenfor).

## Fil-for-fil-mapping

Alle stier er relative til `Data/Campaign1851/`; C++ ligger i `Source/Game1864/Private/`.

| Pakkefil | Spillets skema | Eksisterende C++-indlæser | Faktisk destination | Handling |
|---|---|---|---|---|
| `Nations_1825.json`, `Nations1851.json` | samme | Campaign1851Nations.cpp:LoadNations | `Nations_1825.json`, `Nations1851.json` | PR-række flettet |
| `cities_neighbours_1825/1851.json` | samme | Campaign1851Neighbours.cpp:LoadNeighbours | samme | 25 byer, 13 distrikter, 33 ruter, nation, `outside` (Köslin) flettet |
| `Army_Neighbours_1825/1851.json` | samme | Campaign1851Neighbours.cpp:LoadNeighbours (hærdelen) | samme | 34 PR-poster flettet (25 garnisoner, 9 felthærposter) |
| `Army_1825.json`, `Army1851.json` | `Army_1825.json`, `Army1851.json` | Campaign1851Army.cpp:LoadArmy (kun dansk pulje) | `Countries/Prussia/GameReady/` | Arkiveret. Kataloget har `null` for ruller; aktiveres det oven i nabohæren, spawnes mandskab to gange |
| `Officers_1825.json`, `Officers1851.json` | `Officers_1825.json`, `Officers1851.json` | Campaign1851Officers.cpp:LoadOfficers (kun dansk pulje) | `Countries/Prussia/GameReady/` | Arkiveret. Müffling (1825), Wrangel og Stockhausen (1851), ni neutrale spilskøn (5/10). Ville blive danske officerer i den danske pulje |
| `Economy_1825.json`, `Economy_1851.json` | `Economy_1825.json` | Campaign1851Economy.cpp:EconomyValue (dansk 1825; 1851 hardcoded) | `Countries/Prussia/GameReady/` | Arkiveret. Alle satser er `null`; kun sølvparitet 37/28 og cirkabudget 1851 findes. Preussens distriktsøkonomi ligger allerede i distrikterne |
| `Events.json` | `Events.json` / `Events_1825.json` | Campaign1851War.cpp:ResetWar | `Countries/Prussia/GameReady/` + aktive eventfiler | Arkiveret; aktiveret af `activate_prussia_events.py` (afsnittet Begivenheder) |
| `Population_1825/1851/1864.json` | `Population_1825.json` | Campaign1851Map.cpp:LoadData (dansk amtsoverlay) | `Countries/Prussia/GameReady/` | Arkiveret. Provins-/Bezirk-id'er er dokumentations-id'er og må ikke lægges i danske amter |
| `Trade_1851.json` | `Trade_1851.json` | ingen | `Countries/Prussia/GameReady/` | Arkiveret. Alle totaler er `null`; spillets `Trade_1851.json` er uændret |
| `Manifest`, `Data-status` (autoritativ status/kilde pr. tal via JSON-pointer), `Sources`, `Kildegrundlag`, `Skema-kontrol`, `Verifikation`, `README`, `Research/*.json|md`, pakkens fire `.md` | dokumentation | ingen | `Countries/Prussia/` | Arkiveret byteidentisk |

Hele arkivet (`Countries/Prussia/`, 36 filer, ca. 5 MB) er pakkens bytes; kun `Research/*.json|md` er medtaget (ikke htm/pdf/png-kilderne).

## Importscript og regler

```powershell
python Tools/Countries/import_sweden_norway.py   # kun hvis kortet er genskabt
python Tools/Countries/import_prussia.py
python Tools/Map1851/validate_neighbours.py
python Tools/Countries/validate_prussia.py
```

`import_prussia.py` genbruger `patch`, `read` og `project` fra `import_sweden_norway.py`. Kør det **efter** `export_neighbours.py` og SE/NO-importen; generatoren kender ikke PR-metadata. Regler:

- Kun PR-rækker (og de 33 ruter med en PR-by som endepunkt, samt `Economy_South_<år>.json`s spejling af PR-distrikterne) ændres. Alle andre rækker bevarer deres bytes; CRLF er bevaret (talt på bytedata).
- `null` overskriver aldrig et eksisterende tal og skrives ikke. Begrundelsen bevares i `fieldStatus`.
- `status` og `source` følger hvert tal som `fieldStatus` (kilde / skøn / spil + kilde-id + begrundelse). Spillets egne statusstrenge (`estimated`) bevares, hvor de findes (100 poster), fordi `validate_neighbours.py` kræver dem; pakkens klassifikation (`spil`) står i `fieldStatus`.
- Spillerrettet tekst (`note`, `period` i nationsrækken) bevares; pakkens tekst gemmes som `packageNote` / `packagePeriod`.
- Et tal, som pakken selv markerer som arvet spilvalg (`spil`, kilde `GAME-…`), og som afviger fra spillet, overskrives ikke: spillets værdi vinder og afvigelsen rapporteres. Det rammer én post: `Nations1851.json` PR `relation` (spillet −20, pakken −35; pakken kalder værdien arvet, men den er ikke den, spillet bruger).
- Ejer-/nationskonflikt springes over og rapporteres.
- 1825 og 1851 behandles hver for sig. Idempotent: anden kørsel giver 0 ændringer og bevarer første rapport.

## Importrapport (`Docs/Import-Preussen-rapport.json`)

Første import: **0 tilføjede, 216 opdaterede, 196 null sprunget over, 3 konflikter, 5 udskudte, 1 værdikonflikt, 100 bevarede statusstrenge.**

- Opdaterede (rækker, begge scenarier): 50 byer, 26 distrikter, 66 ruter, 68 hærposter, 2 `outside` (Köslin), 2 kortnationer, 2 nationsrækker. Det er metadata (`fieldStatus`, `historicalPopulation`, noter, `onMap`, `status`, `canAlly`/`canGuarantee` = spillets defaults). **De eneste ændrede tal:** PR `population` 12.200.000 → 12.144.210 (1825) og 16.330.000 → 16.718.740 (1851), skøn fra HGIS-nationalserien (halvårsinterpolation).
- Null sprunget over: `armyMen` og `taxPerHead` i begge nationsrækker (spillets 127.000/130.000 og 0,25/0,2 bevares som spilskøn), by-`historicalPopulation` og `population.1864` (96 pr. scenarie). Arkivfilernes nulantal står i rapporten (`archiveNulls`).
- Konflikter: tre events (`london`, `execution`, `ultimatum`) har samme id som danske events; ved første import ikke aktiveret, senere flettet (afsnittet Begivenheder).
- Udskudte: Köslin (uden for kortudsnittet), ownerGrid/districtGrid-metadata (fælles for alle nationer), events.

## Kortdækning

Samme formel og extent som SE-importen (LAEA 52°N, 10°Ø; extent x −160,577..384,332, y 111,195..669,478 km, læst fra `Denmark1851_Map.json`).

- **25 PR-byer pr. scenarie ligger inden for extent, på PR-celler i ejergridet og i deres eget distrikt i `South_Districts.bin`.** Ingen kortbinær er ændret.
- **Köslin** (x 401,0 km) ligger udenfor extent og står som `onMap=false` i `outside`; ingen rute, ingen aktiv by.
- Rutenet: alle 25 byer er forbundet med København via de eksisterende ruter (Ratzeburg–Mecklenburg–Pommern). Over land fra Ratzeburg uden færger kan 20 nås; **fem kræver færge**: Bergen på Rügen, Putbus, Garz på Rügen og Sassnitz (Strelasund-færgen fra Stralsund) samt Swinemünde (Peenestrom-færgen fra Wolgast). Ingen er helt uden rute. De eneste uden rute er off-map: Köslin.

## Begivenheder

Brugerens beslutning 2026-10-16: PR-relationen til Danmark bliver på spillets −20, og de preussiske events **må** aktiveres, men historien er udgangspunkt, ikke manuskript. Hver aktiv hændelse er et muligt eller sandsynligt udfald med vindue og sandsynlighed (`spil`), forudsætninger som spillerens valg kan gøre falske, og forskellige følger, hvis den udebliver. Aktiveringen sker reproducerbart med `python Tools/Countries/activate_prussia_events.py` (efter `import_prussia.py`; idempotent). Arkivet `Countries/Prussia/GameReady/Events.json` er uændret.

**Rodårsager, loaderen skulle håndtere**

1. Ni PR-events havde `udeblivelse` kun med `spaending: 0`, som `ResetWar` afviste. Nu har hver spilbar hændelse en lille, ikke-nul udeblivelse (spil); historik-events har ingen udeblivelse.
2. Seks PR-events ligger før 1851-starten og blev afviklet som forfaldne ved første månedsstatus. Nu er de historik (`historik: "indtraf"`): de afspilles aldrig, men står som indtruffet, så senere events kan henvise til dem. En dansk hændelse før start uden `historik` lukkes tavst som uafgjort.
3. Arkivets `effekter`/`udeblivelse` bærer et indlejret `fieldStatus` (status og kilde pr. tal); loaderen behandlede nøglen som en ukendt effekt. Den springes nu over.
4. Tre id'er var delt med danske events (`london`, `execution`, `ultimatum`).
5. Alt var engangsafgørelser på vinduets første dag, så en forudsætning kun blev set én gang.

**Id-konflikterne: flet, ikke dubler.** De tre PR-varianter har betingelse, vindue, sandsynlighed, effekter og udeblivelse identiske med de danske events (kontrolleres af scriptet; kun `navn`/tekst på `london` adskiller sig, og den danske tekst bevares). At give dem egne id'er ville give to avisopslag og to spændingsændringer for samme hændelse og kræve en gensidig udelukkelse, som ingen af dem har indhold til. Derfor får de danske poster kun `source`, `historyStatus`, `designStatus`, `note` og `fieldStatus` tilføjet. Dansk adfærd er uændret (ingen regel, effekt, tekst eller rækkefølge er rørt).

**Nye valgfrie felter** (se `Docs/Events.md`): `vindue.tilAar`/`tilMaaned` (hændelsen forsøges hver måned i vinduet, sandsynligheden gælder hele vinduet, afgøres ved udløb), `historik` (`indtraf`/`udeblev`, kun for vinduer før scenariets start) og fakta `krig`, `begivenhed.<id>`, `udeblevet.<id>`. Eksisterende felter genbruges: `sandsynlighed` med spillets seedede kast (seed, scenarie, id og dag; ingen ny tilfældighedsstrøm), `forudsaetninger`, `blokeringer`, `effekter`, `udeblivelse`.

**Spændingsfølger er moderate** (højst ±3 point, alle `spil`). PR-relationen ændres ikke af nogen hændelse.

| Id | Scenarie | Udløser (forudsætninger) | Sandsynlighed / vindue | Udebliver hvis | Følger / udeblivelse | Status |
|---|---|---|---|---|---|---|
| `pr_zollverein` | 1825 | `ikke mobiliseret og ikke krig` | 0,80 over jan 1834 – jun 1835 | mobilisering eller krig i hele vinduet, eller dårlig chance | spænding +1 / udeblivelse −1 | emne/dato kilde (ZOLL); resten spil |
| `pr_zollverein` | 1851 | – | historik (jan 1834 er før start) | – | indlæses som indtruffet, ingen følge | kilde (ZOLL) |
| `pr_1848`, `pr_malmo`, `pr_berlinpeace`, `pr_olmuetz`, `pr_1851` | 1851 | – | historik (apr 1848 – jan 1851 er før start) | – | indlæses som indtruffet; arkivets effekter er dokumentation og afspilles ikke | emne/dato kilde (WAR, MANTEUFFEL); resten spil |
| `pr_hannover_zoll` | 1825, 1851 | `ikke mobiliseret og ikke krig` | 0,75 over sep 1851 – dec 1852 | mobilisering eller krig, eller dårlig chance | spænding −1 / udeblivelse +1 | emne/dato kilde (ZOLL); resten spil |
| `pr_roon1859` | 1825, 1851 | `forhold.PR < 50 og ikke krig` | 0,70 over dec 1859 – jun 1860 | god relation til Preussen (≥ 50), krig, eller dårlig chance | spænding +1 / udeblivelse −1 | emne/dato kilde (ROON-BIO); resten spil |
| `pr_roon1860` | 1825, 1851 | `forhold.PR < 50 og begivenhed.pr_roon1859 og ikke krig` | 0,70 over jan 1860 – jun 1862 | `pr_roon1859` udeblev, relation ≥ 50, krig, eller dårlig chance | spænding +3 / udeblivelse −1 | emne/dato kilde (ROON); resten spil |
| `london`, `execution`, `ultimatum` | 1851 | uændret fra de danske events (`Docs/Events.md`) | uændret (0,85, enkeltdato) | uændret | uændret; kun status/kilde tilføjet (LONDON, WAR, ULTIMATUM; `kilde`/`spil`) | som arkivet |

Hændelserne 1848–1851 (`pr_1848` m.fl.) er ikke aktive i 1825: scenariet har sine egne danske hændelser for 1848 (`kiel`, `preussen`), og en dublet ville ændre dem. `london`, `execution` og `ultimatum` findes ikke i 1825-filen og er ikke tilføjet. Udeblivelsernes retning og størrelse er spilvalg, ikke historiske størrelser; en udeblevet hændelse sender ikke historien tilbage på sporet, den giver bare en anden (lille) følge.

**Gem og indlæs.** `UCampaign1851SaveGame::CurrentVersion` er 37. Udløste og afviste id'er ligger som før i `War` (`fired|id`, `blocked|id`); formatlinjen er nu `events-format|37`. En ældre gemning får de uafgjorte events, hvis vindue er forbi, lukket uden følger (nye events efteropkræves ikke); historik genskabes fra data. Kastet er en ren funktion af seed, scenarie, id og dag og skal ikke gemmes; en gemning kaster ikke igen.

## Indlæsere, gem og validering

- **Ingen C++-ændring var nødvendig for kortdata** (senere samme dag er event-loaderen ændret og gemmeversionen hævet til 37, se Begivenheder). SE/NO-arbejdets nul-sikre indlæsere (`Campaign1851Json.h`, LoadNeighbours, LoadNations, LoadArmy, LoadOfficers, LoadScenarioMinisters) dækker alle PR-felter. De nye nøgler (`fieldStatus`, `note`, `historicalPopulation`, `packageNote`, `onMap` på `outside`) læses ikke eller ignoreres. `canAlly`/`canGuarantee` er sat til indlæserens defaults (false). Ingen nye gemte felter; `UCampaign1851SaveGame`-versionen er uændret.
- `validate_neighbours.py`: 1825 og 1851 består (88 byer, 113 ruter, 104 hærposter).
- `validate_prussia.py`: JSON parser, 21 pakkehashes, 681 urørte rækker byteidentiske med en isoleret gengivelse (git HEAD + SE/NO + PR), reproducerbar og idempotent. Danske events er uændrede (kun metadata tilføjet) og eventfilernes CRLF er bevaret.
- `validate_sweden_norway.py` er justeret: PR-rækker er undtaget fra de beskyttede rækker, og den isolerede gengivelse kører PR-importen efter SE/NO. Resultat: 230 beskyttede rækker byteidentiske (dansk, hannoveransk, mecklenburgsk m.fl.), SE/NO reproducerbar.
- Dansk spil: alle danske rækker er byteidentiske. Den eneste datamæssige ændring, der kan mærkes, er PR's abstrakte befolkningstal (påvirker kun Preussens abstrakte statskasse/hærloft, ikke danske tal). Ingen spiltest er kørt.

## Hvad der mangler til en fuld modstander

- **Hær:** faktiske ruller, regimentsstyrker og hjemsteder (Rangliste ikke afskrevet), Landwehr/Kriegsreserve, linjekavaleri, pionerer, forsyningstropper, chefer. Nabohæren er fortsat spilmodel (arvede tal). Hærkatalog og officerer kan først aktiveres, når en preussisk pulje (eller et nationalt OOB-lag) findes.
- **Økonomi:** skatter, priser, gæld, startkasse og handelstal er `null`. Kun sølvparitet og 1851-cirkabudget findes. Preussen har ingen national kasse i kampagnen.
- **Events:** kun de seks hændelser, pakken indeholder, og kun med dansk spænding som følge; ingen preussisk mobiliseringskalender eller egne preussiske tilstande.
- **Kort:** ingen nye højde-, skov-, vand- eller ejerrastre; Köslin og Berlin ligger udenfor kortet; grænser er skøn, ikke historiske polygoner; færger/broer/jernbaner er kun de eksisterende.
- **Krig:** krigsmodellen aktiverer kun PR/AT/DE som fjender; ingen preussisk mobiliseringskalender, doktrin, forsyning eller kamp-AI.
- **Niveau C:** flåde, officerspulje og ministre, forskning, bygninger/produktion, flag og uniformer, spilbart UI.

## Risici

- Pakken er ikke importklar efter eget udsagn; der er ingen spil- eller savetest. Tal, der ser ens ud, er arvede spilvalg, ikke historiske observationer.
- `relation` 1851 (−20) afviger fra pakkens −35; spillets værdi er bevaret og skal afgøres bevidst.
- PR-populationen ændrer Preussens abstrakte økonomi lidt; gamle saves beholder deres gemte nationstal.
- En ny `export_neighbours.py`-kørsel overskriver PR-metadata; kør derefter SE/NO- og PR-importen igen.
