# Import af Sverige-Norge ? 2026-10-16

**Delvis integration; niveau B er ikke fuldf?rt.** 1825 og 1851 har separate imports. Oslofjorden ligger uden for det eksisterende udsnit, og den eksisterende krigsmodel aktiverer kun PR/AT/DE som fjender. Importen udvider svenske kortdata, men opretter ikke en svensk-norsk krigsmekanik. Ingen build, editor-/spilstart, spiltest eller commit.

## Pakke og afgr?nsning

Kilde: `C:/Users/allan/Documents/Codex/2026-10-12/l/outputs/Sverige-Norge-agent/Spilimport/`, g?ldende runde 7. README, manifest, proveniens, arbejdsstatus, d?kningsmatrix og spilklar-tjek er grundlaget. De 30 GameReady-filer er bevaret byteidentisk under `Data/Campaign1851/Countries/SwedenNorway/GameReady/`. Manifest, numerisk proveniens, kildeindeks, valuta, union, events og SE/NO-research er bevaret ved siden af. Research er ikke en underst?ttet runtime-kontrakt.

Status/kilde/metode/intervaller er ikke omskrevet. For et tal findes den autoritative status i `Countries/SwedenNorway/GameReady-proveniens.json`: filnavn og JSON-pointer f?lger den u?ndrede eksport. `Sources.json` opl?ser kilde-id. Ved aktiv import matches byer p? navn, nationer p? id og h?re p? id. `SE-field` matches bevidst til eksisterende `SE-south`; proveniensens feltsti er stadig den oprindelige. Gamle inline-statusser har ikke forrang for runde 7-sidecaren. Der er ingen ny tooltip-visning af sidecaren.

## Fil-for-fil-mapping

Kolonnen skemafil angiver spillets format/oprindelige danske fil. Den faktiske destination kan v?re arkivet: nationale officerer, ?konomi og regimenter kan ikke l?gges i den danske pulje uden at ?ndre dansk spil. Alle stier i destination er relative til `Data/Campaign1851/`; C++ ligger i `Source/Game1864/Private/`.

| Overlay | Spillets skemafil | Eksisterende C++-indl?ser | Faktisk destination |
|---|---|---|---|
| `SE/Army_1825.json` | `Army_1825.json` | Campaign1851Army.cpp:LoadArmy (kun dansk pulje) | `Countries/SwedenNorway/GameReady/SE/Army_1825.json` |
| `SE/Supply_1825.json` | `Army_1825.json` | Campaign1851Army.cpp:LoadArmy / ArmyEquipmentNumber (kun dansk pulje) | `Countries/SwedenNorway/GameReady/SE/Supply_1825.json` |
| `SE/Economy_1825.json` | `Economy_1825.json` | Campaign1851Economy.cpp:EconomyValue (1825 DK; 1851 hardcoded) | `Countries/SwedenNorway/GameReady/SE/Economy_1825.json` |
| `SE/Army_Neighbours_1825.json` | `Army_Neighbours_1825.json` | Campaign1851Neighbours.cpp:LoadNeighbours | `Army_Neighbours_1825.json` |
| `SE/Ministers_1825.json` | `Ministers_1825.json` | Campaign1851Ministers.cpp:LoadScenarioMinisters (kun 1825 DK) | `Countries/SwedenNorway/GameReady/SE/Ministers_1825.json` |
| `NO/Army_1825.json` | `Army_1825.json` | Campaign1851Army.cpp:LoadArmy (kun dansk pulje) | `Countries/SwedenNorway/GameReady/NO/Army_1825.json` |
| `NO/Supply_1825.json` | `Army_1825.json` | Campaign1851Army.cpp:LoadArmy / ArmyEquipmentNumber (kun dansk pulje) | `Countries/SwedenNorway/GameReady/NO/Supply_1825.json` |
| `NO/Economy_1825.json` | `Economy_1825.json` | Campaign1851Economy.cpp:EconomyValue (1825 DK; 1851 hardcoded) | `Countries/SwedenNorway/GameReady/NO/Economy_1825.json` |
| `NO/Army_Neighbours_1825.json` | `Army_Neighbours_1825.json` | Campaign1851Neighbours.cpp:LoadNeighbours | `Army_Neighbours_1825.json` |
| `NO/Ministers_1825.json` | `Ministers_1825.json` | Campaign1851Ministers.cpp:LoadScenarioMinisters (kun 1825 DK) | `Countries/SwedenNorway/GameReady/NO/Ministers_1825.json` |
| `Nations_1825.json` | `Nations_1825.json` | Campaign1851Nations.cpp:LoadNations | `Nations_1825.json` |
| `Population_1825.json` | `Population_1825.json` | Campaign1851Map.cpp:LoadData (1825-overlay; 1851 basekort) | `Countries/SwedenNorway/GameReady/Population_1825.json` |
| `cities_neighbours_1825.json` | `cities_neighbours_1825.json` | Campaign1851Neighbours.cpp:LoadNeighbours | `cities_neighbours_1825.json` |
| `Officers_1825.json` | `Officers_1825.json` | Campaign1851Officers.cpp:LoadOfficers (kun dansk pulje) | `Countries/SwedenNorway/GameReady/Officers_1825.json` |
| `SE/Army1851.json` | `Army1851.json` | Campaign1851Army.cpp:LoadArmy (kun dansk pulje) | `Countries/SwedenNorway/GameReady/SE/Army1851.json` |
| `SE/Supply_1851.json` | `Army_1825.json` | Campaign1851Army.cpp:LoadArmy / ArmyEquipmentNumber (kun dansk pulje) | `Countries/SwedenNorway/GameReady/SE/Supply_1851.json` |
| `SE/Economy_1851.json` | `Economy_1825.json` | Campaign1851Economy.cpp:EconomyValue (1825 DK; 1851 hardcoded) | `Countries/SwedenNorway/GameReady/SE/Economy_1851.json` |
| `SE/Army_Neighbours_1851.json` | `Army_Neighbours_1851.json` | Campaign1851Neighbours.cpp:LoadNeighbours | `Army_Neighbours_1851.json` |
| `SE/Ministers_1851.json` | `Ministers_1825.json` | Campaign1851Ministers.cpp:LoadScenarioMinisters (kun 1825 DK) | `Countries/SwedenNorway/GameReady/SE/Ministers_1851.json` |
| `NO/Army1851.json` | `Army1851.json` | Campaign1851Army.cpp:LoadArmy (kun dansk pulje) | `Countries/SwedenNorway/GameReady/NO/Army1851.json` |
| `NO/Supply_1851.json` | `Army_1825.json` | Campaign1851Army.cpp:LoadArmy / ArmyEquipmentNumber (kun dansk pulje) | `Countries/SwedenNorway/GameReady/NO/Supply_1851.json` |
| `NO/Economy_1851.json` | `Economy_1825.json` | Campaign1851Economy.cpp:EconomyValue (1825 DK; 1851 hardcoded) | `Countries/SwedenNorway/GameReady/NO/Economy_1851.json` |
| `NO/Army_Neighbours_1851.json` | `Army_Neighbours_1851.json` | Campaign1851Neighbours.cpp:LoadNeighbours | `Army_Neighbours_1851.json` |
| `NO/Ministers_1851.json` | `Ministers_1825.json` | Campaign1851Ministers.cpp:LoadScenarioMinisters (kun 1825 DK) | `Countries/SwedenNorway/GameReady/NO/Ministers_1851.json` |
| `Nations1851.json` | `Nations1851.json` | Campaign1851Nations.cpp:LoadNations | `Nations1851.json` |
| `Population_1851.json` | `Population_1825.json` | Campaign1851Map.cpp:LoadData (1825-overlay; 1851 basekort) | `Countries/SwedenNorway/GameReady/Population_1851.json` |
| `cities_neighbours_1851.json` | `cities_neighbours_1851.json` | Campaign1851Neighbours.cpp:LoadNeighbours | `cities_neighbours_1851.json` |
| `Officers1851.json` | `Officers1851.json` | Campaign1851Officers.cpp:LoadOfficers (kun dansk pulje) | `Countries/SwedenNorway/GameReady/Officers1851.json` |
| `Trade_1851.json` | `Trade_1851.json` | Ingen C++-loader; kalibreringsdata | `Trade_1851.json` |
| `Trade_1825.json` | `Trade_1851.json` | Ingen C++-loader; kalibreringsdata | `Trade_1825.json` |

Supply f?lger Army_1825-formatet ogs? i 1851. National Army1851 har ikke selv equipment/magazines. Ingen national OOB aktiveres sammen med naboh?rene, s? samme mandskab ikke spawnes to gange. Population-id-er fra pakken l?gges ikke i danske amter: de er lokale, og ville ellers kollidere. 1851 har ingen generel Population_1851-loader; grundkortets danske befolkning bevares. Economy_1851 i pakken f?lger Economy_1825-skemaet, mens dansk 1851-?konomi anvender hardcodede fallbacktal. Trade-filerne er kalibreringsdata uden C++-loader.

| Research uden GameReady-overlay | Spillets mulige modtager | Resultat |
|---|---|---|
| `Events.json` | `Events_1825.json` / `Events.json`; `Campaign1851War.cpp:ResetWar` | Arkiveret. Parseren kr?ver underst?ttede udtryk/effekter og en ikke-nul udeblivelsesvirkning. Flere pakkekontrakter opfylder ikke dette; de ?ndres ikke vilk?rligt og ?ndrer ikke dansk sp?nding. |
| `Valuta.json` | Ingen valutaloader | Regler og ukendt kurs bevares; ingen ekstra konvertering af allerede normaliserede starttal. |
| `Union-regler.json` | Ingen unions-/bevillingsadapter | Separate nationsr?kker, ingen f?lles kasse eller dobbelt national OOB; ?vrig unionslogik er ikke implementeret. `implementedInGame=false` bevares. |
| `Kortkrav.json` | LAEA i `Campaign1851Map.h`; ejergrid i `LoadNeighbours` | 42 regionspunkter projiceres og rapporteres; ingen polygon/raster opfindes. |
| `Kamp-AI.json`, national fl?deresearch | Ingen passende national runtime-loader | Research bevaret; profiler og fl?de er ikke aktive. |

## Null, manglende n?gler og ekstra felter

F?r ?ndringen: `GetNumberField`/`AsNumber` kan logge fejl og give nul, `GetStringField` kan give tom streng, `GetObjectField`/`AsObject` kan give ugyldig reference som efterf?lgende derefereres, og farve-arrayet blev indekseret uden l?ngdekontrol. `TryGetNumberField`, `TryGetStringField`, `TryGetBoolField`, `TryGetArrayField` og `TryGetObjectField` afviser null/manglende v?rdier; de numeriske output beholdes ved null. JSON-null er en FJsonValueNull, ikke et gyldigt objekt. Ekstra root-/r?kkefelter ignoreres, fordi loaderne kun l?ser kendte n?gler. Event-effektobjekter er en undtagelse: ukendte effekt-n?gler afvises.

Efter ?ndringen:

- `Campaign1851Json.h`: sikker objekt-/r?kke-/arrayadgang; ugyldigt objekt bliver et tomt objekt, manglende array en tom liste. Tekstfallback er tom tekst. Hj?lperne skaber ingen numeriske v?rdier.
- `Campaign1851Neighbours.cpp:LoadNeighbours`: farve kr?ver tre tal. Nye byer kr?ver navn, koordinater og positiv scenariebefolkning; `onMap=false` springes over. Eksisterende byfelter l?ses med TryGet og beholder deres v?rdier ved null. H?re uden men/mobilisedMen springes over. Null underenhedsstyrke vises ikke som nul. Ugyldige ejergrid-dimensioner og distrikter uden id/befolkning afvises; routes uden geometri/endepunkter springes over. Optional guns/garrison bruger de eksisterende struct-defaults 0, ferryKm bruger eksplicit 0 (= ingen f?rge), og manglende distrikt bruger den eksisterende abstrakte nabomodel.
- `Campaign1851Nations.cpp:LoadNations`: sikre r?kker og arrays, id kr?ves, null i v?gte beholder eksisterende struct-default 1. Alle ?vrige TryGet-felter beholder FCampaign1851Nation-defaults, fx caution 0,5 og taxPerHead 0,25. Population/armyMen/railKm har eksisterende default 0; det er loaderens startdefault, ikke et importeret historisk observationstal. Import-null overskriver aldrig eksisterende tal i datafilerne.
- `Campaign1851Army.cpp:LoadArmy`: null-objekter er sikre; regiment uden id/mandskab eller kendt hjemby springes over; magasin kr?ver by og alle tre beholdninger. Skills, equipment og valgfrie felter anvender eksisterende defaults.
- `Campaign1851Officers.cpp:LoadOfficers/MakeOfficer`: null-r?kker og profilobjekter er sikre; id kr?ves. Manglende profilstatistik/alder/erfaring bevarer den allerede genererede v?rdi. Gyldige danske profiler bruger samme r?kkef?lge af tilf?ldighedskald som f?r. L?n/pris beholder allerede eksisterende TryGet-fallback. Den eksisterende statsfallback og 1825-skalering er ikke rebalanceret.
- `Campaign1851Ministers.cpp:LoadScenarioMinisters`: null-r?kker er sikre; kandidat kr?ver navn og alle tre kvaliteter. Ved tom pulje anvendes det eksisterende fallbackforl?b.
- `Campaign1851Map.cpp:LoadData`: Population_1825-match ignorerer null-r?kker/identiteter. Null numerisk populationfelt ?ndrer ikke grundkortets v?rdi; komplet manglende match afvises fortsat som f?r.
- `Campaign1851Economy.cpp:EconomyValue` var allerede null-sikker med TryGet og Value1851 som fallback. Eventloaderen havde allerede type-, udtryks- og effektvalidering. Ingen ?ndringer i disse to filer.

## Fletning og k?rsel

```powershell
python Tools/Countries/import_sweden_norway.py
python Tools/Map1851/validate_neighbours.py
python Tools/Countries/validate_sweden_norway.py
```

Importerens `--package` og `--root` kan angive andre input-/outputmapper. Eksporthashes kontrolleres f?r import. Scriptet parser nuv?rende filer og ?ndrer kun ber?rte JSON-tokenomr?der; ur?rte r?kker bevarer deres bytes. Arrayindeks og gamle identiteter bevares, nye byer/h?re tilf?jes sidst. Null overskriver ikke eksisterende v?rdier. En ejer-/nationskonflikt i et matches?t rapporteres og springes over. Der er ikke foretaget automatisk historisk konfliktl?sning.

Ved genbygning med `export_neighbours.py` skal importscriptet k?res **efter eksporten**; generatoren har stadig den tidligere svenske baseline. Validatoren kontrollerer nu svensk indhold mod den l?ste import, i stedet for at kr?ve de gamle svenske styrker. De ?vrige distrikts-, vej-, ejer-, OOB- og scenariekontroller er bevaret.

F?rste import, genskabt mod Git HEAD med checkout-filtre: **113 tilf?jede, 48 opdaterede, 4 null sprunget over, 0 ejer-/nationskonflikter, 210 udskudte poster/ruter.** Antal er r?kker/omr?der i aktive filer og outside-metadata, ikke antal talfelter; arkivkopier t?lles ikke som runtime-tilf?jelser. Gentagelse giver 0 tilf?jede/opdaterede og byteidentiske datafiler. `validate_sweden_norway.py` bevarer f?rste-import-rapporten efter sin isolerede reproduktion. Se [maskinrapport](Import-Sverige-Norge-rapport.json) for alle navne, spring og filmapping.

De fire null er Trelleborgs 1864-reference i begge byoverlays samt comparison1847 i begge Trade-overlays. Trelleborgs eksisterende 1864-reference bevares i aktive filer; det gamle tals status bliver ikke opgraderet til en ny kilde. Arkivet bevarer original-null med begrundelse.

## Kortd?kning

LAEA: centrum 52?N, 10??, radius 6371,0088 km; x/y-extent l?ses direkte fra Denmark1851_Map.json. Samme formel som Campaign1851Map.h og export_neighbours.py; nordlig rasterr?kke har indeks 0. Der foretages b?de extent- og ejergridkontrol f?r aktivering.

Pr. scenarie indeholder den faktiske eksport 63 unikke by-/transportpunkter, fordelt p? cities og outside (?ldre statusnotaters 39 punkter er ikke den aktuelle eksport). **17 ligger indenfor extent og matcher SE-ejergrid**, **46 ligger udenfor**. Seks svenske byer er nye i aktive data: Karlshamn, J?nk?ping, V?xj?, Bor?s, Marstrand og Eksj?. Ingen norske punkter aktiveres. Af 42 regionspunkter ligger **8 indenfor** og **34 udenfor**. Punkter siger intet om hele regionens udstr?kning. Alle distrikter bevares som onMap=false i `Districts_1825.json`/`Districts_1851.json`, fordi svenske/norske distriktsraster/polygoner mangler.

Alle udenfor-punkter har onMap=false i nabofilernes outside-metadata og indl?ses ikke som Cities. De l?ste r? overlays ?ndres ikke. Nye aktive byer f?r ingen opdigtet distriktsrelation i det sydtyske raster. Ruter uden km-geometri aktiveres ikke; de seks nye byer har derfor ingen ny verificeret vejforbindelse. De kan vises som bypunkter, men er ikke et komplet milit?rt net.

En udvidelse kr?ver ny f?lles extent, h?jde-/Features-/hydro-/landmaske, periode-ejergrid med NO-id, SE/NO-distriktsgrid og polygoner, projicerede og farbare veje/broer/f?rger samt konsistent genprojektion af samtlige gamle kortlag og stabile save-indeks. Intet terr?n-, h?jde-, ejer- eller distriktsraster er ?ndret.

## Validering og gemte spil

- Nabovalidator: 1825 og 1851 best?r; hver har 88 nabobyer, 113 ruter og 104 h?rposter.
- Importvalidator: 350 beskyttede r?kker er byteidentiske mod HEADs checkout-repr?sentation; alle tidligere by-/h?r-/nationsindeks er stabile, 30 eksporthashes matcher, 85 JSON-filer under kampagnedata parser, importen er reproducerbar og idempotent.
- C++-delimiterkontrol best?r for de syv nye/?ndrede kildefiler. Nye JSON-kald/includes er kontrolleret statisk mod UE 5.8 JsonObject.h, JsonValue.h og Core-headere; TryGet-outputtyper er kontrolleret mod kampagnens struct-felter. Dette er ingen kompileringsgaranti.
- CRLF/LF/bare-CR er talt p? bytedata. Eksisterende ?ndrede kilder, v?rkt?j og data beholder CRLF uden lone LF; arkivfiler beholder pakkens bytes. Se [statisk rapport](Import-Sverige-Norge-statisk.json) og maskinrapportens lineEndings.
- `Tools/validate_after_action.py` g?lder slag-/rapportarbejdet og er ikke k?rt; denne import ?ndrer ikke Strategy1864. Generatorer og launchere er ikke k?rt.

Ingen nye vedvarende C++-felter eller saveformat?ndringer er indf?rt; UCampaign1851SaveGame-versionen h?ves derfor ikke. Naboh?r-id SE-south og alle tidligere indeks bevares. Gamle saves beholder eksisterende mobiliserede korps/tab og er ikke automatisk historisk rebalanceret. En reel save-rundtur er ikke udf?rt.

## Det resterende og risici

**Niveau B:** Oslofjord-/sydsvenske kortaktiver, distriktsgeometri, sammenh?ngende veje/f?rger/broer, svensk-norsk fjendtlighed/krigsbeslutning og m?lvalg, begr?nset mandskabs-/forsyningspulje, nationale budgetkoblinger og unionsbevillinger. De nuv?rende svenske naboh?re har fortsat den abstrakte model uden et lokalt distriktsbudget. Der er ikke lavet ny mobiliserings-, forsynings-, kamp- eller saveafpr?vning. Niveau B kan derfor ikke erkl?res opn?et.

**Niveau C:** national ?konomi/startkasse/g?ld, aktiv national OOB med puljeallokering, materiel og magasin-enhedskalibrering, nationale officer-/ministerpuljer, events med underst?ttede foruds?tninger/effekter, fl?de, forskning/politik, bygninger/produktion, AI-adapter og periodekorrekte flag/uniforms-/portr?taktiver samt fuldt spilleligt nationalt UI. Data findes i varierende grad i arkivet, men dette er ikke runtime-integration.

Danske og andre landes datar?kker er bevaret; det er ikke en garanti for u?ndret samlet simulering: de tilsigtede SE/NO-nationstal p?virker abstrakt AI/diplomati. Der er fortsat historiske usikkerheder fra pakken: dagsfremm?de, heste, bestykning, lagre, priser, kursnormalisering og flere produktionstal er sk?n/spil. Isolerede nye bypunkter, gamle saves og fremtidig regenerering er s?rlige risici. Ingen observation eller spiltest h?vdes p? baggrund af JSON-kontrol.
