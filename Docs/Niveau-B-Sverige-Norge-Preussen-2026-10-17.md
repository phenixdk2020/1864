# Sverige-Norge og Preussen: niveau B — 2026-10-17

Implementeret for **1825 og 1851**, statisk valideret. 1825 er fortsat standard. Ingen build, spil/editorstart eller commit. Dette er status pr. 17. oktober; første Niveau C-del er beskrevet i tillægget af 2026-10-18 nedenfor. De tidligere importnotater og rapporter dokumenterer importen pr. 2026-10-16; denne rapport beskriver den efterfølgende integration.

Kort og data:

- Kortet udvides additivt mod nord med `build_map.py --extend-north`. `extentKm.yMax` bliver 1227,7617423320336 km, mens x-grænserne og yMin bevares. Ejer-/distriktsgrid er 768 × 1574. Oslofjorden og det sydøstlige Norge er indenfor. Bergen og Trondheim er fortsat udenfor; dette er et sydligt krigsteater, ikke hele Norge.
- Det oprindelige danske korts byer, amter, veje og indeks er uændrede. Nederste halvdel af de tre PNG-rastre er pixelidentisk. Det gamle ejergrid er bevaret byte for byte som nederste halvdel. Gamle distriktsceller uden for Sverige er uændrede; svenske celler har nu distriktsnumre. Hele rasterfilernes hashes ændres nødvendigvis, fordi nordlige rækker tilføjes.
- Begge scenarier har 36 svenske og 15 norske aktive punkter, 20 nordiske distrikter og ingen isolerede nordiske punkter. Samlet nabonetværk: 122 byer, 153 ruter og 139 hæroptegnelser, herunder garnisoner og historiske nulstillede aggregater.
- Ruter suppleres med A* på ejerrasteret. Afbrudte kystforbindelser ved Marstrand er eksplicitte bådforbindelser. Ruterne er geometriske spilskøn, ikke verificerede historiske postruter. Distriktsgeometrien bruger samme model som de eksisterende sydlige distrikter: nærmeste sæde, klippet efter ejer, med eksplicit korrektion ved bypunkter.
- Norske bytal, hvor pakken kun har amtets urbane befolkning, er fordelingsskøn med `populationStatus` og forklaring ved rækken. De er ikke nye påståede byfolketællinger. Byernes befolkning indgår allerede i distriktet og lægges ikke oven i det.
- Nordligt relief er et fladt visuelt skøn, kysten er fra den lokale Natural Earth 10m-kilde. Danmark beholder sin eksisterende materialetekstur med korrigerede UV'er; nordlige land-/havflader tegnes med særskilte materialer. Der kræves ingen Content-import. Den hidtidige danske terrænmeshtæthed bevares ved at fordoble antal rækker.

Hære, union og forsyning:

- SE/NO bruger samme udenrigspolitiske sidebeslutning: alliance med én unionsdel giver begge passage og støtte; uden alliance gør svensk relation ≤ −50 dem begge fjendtlige under den eksisterende krigstilstand. Det er en spilregel, ikke en historisk sikker hændelse. Ingen selvstændig bilateral krigs-/fredsmaskine er tilføjet.
- Hærene, distriktsreserverne og distriktskasserne er separate. Svenske rekrutter kan ikke trækkes fra norske distrikter. Magasiner kan benyttes på tværs af unionen, men genopfyldningen betales lokalt. Danmark betaler ikke udenlandske reserver eller magasiner. Eksisterende `Campaign1851Army.cpp`-ruteplanlægning benytter de nye adgangsregler og ruter.
- Allierede kolonner vælger fjendtlige mål og kan kæmpe automatisk mod dem på kampagnekortet; de angriber ikke danske regimenter og kan ikke vælges som danske angrebsmål. Daglig tabsudveksling er et markeret spilskøn. Allierede figurer vises med særskilt farve og korrekt nationsnavn. Fuld koalitionskamp i 3D er ikke en del af dette niveau.
- PR bruger den eksisterende landepakkes scenarieopdelte pommerske regiments-OOB: 18.000 mobiliserede feltmænd i 1825 og 24.000 i 1851. Der er korps-/divisionshenvisninger samt en scenarievalgt historisk officer med markeret skøn for feltkommandoen. Officeren og stabsværdien indlæses i nabohærens marchmodel. Arkivets nationale regimentskatalog spawnes ikke oveni. Det ekstra abstrakte preussiske korps i `DeclareWar()` er fjernet.
- Mobiliseringen har tre trin: fredskadre efter samling, halv reserve og fuld reserve. Samling: PR 7 dage, SE/NO 14 dage; fuld mobilisering: 42 dage i 1825 og 28 dage i 1851. Tiderne er spilskøn, markeret ved dataene. Reserve og betaling trækkes fra tilgængelige nationale distrikter. Månedlige erstatninger omgår ikke det opnåede mobiliseringstrin. Forsyning omfatter mad og foder, også for blandede feltstyrkers kanoner.
- Relation PR–Danmark er −20 i begge scenarier. 1825-importen stod før dette arbejde på +10; det er rettet eksplicit efter opgavens krav. Aktive preussiske events er fortsat probabilistiske med forudsætninger, som kan blive falske. Events og 1851-relationen er ikke omskrevet.

Gemte spil og regenerering:

- Saveversionen er **38**. Mobiliseringens startdag og trin gemmes i `UCampaign1851SaveGame::Neighbours`; korpsenes position, tab og marcher ligger fortsat i `War`, reserver/kasser i distriktsrækker og depoter i det eksisterende lagerformat. Nye statiske kildefelter og UV-skala genindlæses fra scenarie-/kortdata.
- Ældre gemte korps bevares med deres tab. Manglende mobiliseringsrækker behandles som allerede indkaldte korps, så reserver ikke hæves igen. Et gammelt krigssave kan stadig indeholde det tidligere abstrakte preussiske korps; det fjernes ikke uden videre, da det ville kassere gemt kampstatus. Start et nyt scenarie for den nye styrkebalance. Kamera og save/load er ikke afprøvet i runtime.
- `--extend-north` er en engangsudvidelse og er en no-op på et allerede udvidet kort. Gentag efterfølgende landeimporter og `integrate_level_b.py` i den dokumenterede rækkefølge. En almindelig fuld regenerering af det gamle kort kan overskrive udvidelsen og ændre danske rasterhashes; dette kræver særskilt review og må ikke forveksles med den additive kommando.
- Geografisk raster deles mellem scenarierne; befolkning, hære, mobilisering og distriktsøkonomi vælges pr. scenarie. Nationale `onMap=false`-felter bevares, fordi de betegner fuldt nationalt økonomi-/spillerkort, ikke om enkelte nabobyer vises.

Validering:

```text
python Tools/Map1851/validate_neighbours.py
CAMPAIGN-1851|neighbours|valideret|1825|122 byer|153 ruter|139 hære
CAMPAIGN-1851|neighbours|valideret|1851|122 byer|153 ruter|139 hære

python Tools/Countries/validate_sweden_norway.py
OK SE/NO: 114 JSON; danske indeks/geografi/pixels og beskyttede gridceller bevaret;
arkivhashes OK; fuld genimport x2 byte-idempotent.

python Tools/Countries/validate_prussia.py
OK PR: 114 JSON; danske indeks/geografi/pixels og beskyttede gridceller bevaret;
arkivhashes OK; fuld genimport x2 byte-idempotent.

git diff --check
Exit 0, ingen whitespacefejl.
```

Validatorerne bruger den nye fælles kontrol, når `Countries/LevelB.json` findes; den gamle kontrol bevares for det tidligere importstadie. Den tidligere regel om ingen norske byer/SE-distrikter er erstattet af kontroller for aktive byer, distrikter og sammenhæng. Arkivhashes og beskyttelse af danske data er bevaret. Genimporten køres to gange i en midlertidig kopi på disk; validering ændrer ikke arbejdsfilerne. A* er også kontrolleret på en lille landmaske med vandblokering og én landpassage. Ingen kompilering eller runtime-test er udført.

UE 5.8-kontrol mod `I:/Spil/Epic Games/UE_5.8/Engine/Source`:

- `Runtime/Json/Public/Dom/JsonObject.h`: `HasField` (362), `TryGetNumberField` float/double/int32 (414–426) og `TryGetStringField`.
- `Runtime/Core/Public/Containers/Array.h`: `FindByPredicate` (1710/1723), `ContainsByPredicate` (1777), samt de anvendte Add/Reset/RemoveAt-overloads.
- `Runtime/Core/Public/Misc/CString.h`: `FCString::Atoi` (500), `Atod` (515). `Math/UnrealMathUtility.h`/platformmath: Clamp, Min/Max, IsFinite, RoundToInt og FloorToDouble. `Math/Vector.h` og `Vector2D.h`: division, Distance og de anvendte positionsoperatorer.
- `Runtime/Engine/Public/Materials/MaterialInstanceDynamic.h`: `Create` (176), `SetVectorParameterValue` (109); `Engine/Classes/Engine/StaticMesh.h`: `FStaticMaterial` (492). `CoreUObject/Public/UObject/ObjectPtr.h`: typet pointeradgang.
- `Runtime/MeshDescription/Public/MeshDescription.h`: `CreateTriangle` (334), `CreatePolygonGroup` (404). `Runtime/StaticMeshDescription/Public/StaticMeshAttributes.h`: `GetPolygonGroupMaterialSlotNames` (120). Eksisterende includes dækker kald og typer; ingen nye UE-includes er tilføjet.

Niveau C mangler fuld national spillerstyring, produktions-/bygningsøkonomi, komplette verificerede distriktsgrænser og bytal, fulde regimenter/officerskarrierer, komplet preussisk kabinet og ministerudskiftninger, forskning, egen flåde, uniforms-/enhedsgrafik samt fuld koalitions- og bilateral krigsmodel. Der mangler desuden godkendt runtime-/save-/slagafprøvning af niveau B.

Ændrede filer for denne opgave (historikfilen indeholder desuden en anden agents selvstændige afsnit):

- `Data/Campaign1851/Army_Neighbours_1825.json`
- `Data/Campaign1851/Army_Neighbours_1851.json`
- `Data/Campaign1851/Countries/LevelB.json`
- `Data/Campaign1851/Denmark1851_Amter.png`
- `Data/Campaign1851/Denmark1851_Features.png`
- `Data/Campaign1851/Denmark1851_Height.png`
- `Data/Campaign1851/Denmark1851_Map.json`
- `Data/Campaign1851/Economy_South_1825.json`
- `Data/Campaign1851/Economy_South_1851.json`
- `Data/Campaign1851/Nations_1825.json`
- `Data/Campaign1851/Neighbours_Owners.bin`
- `Data/Campaign1851/South_Districts.bin`
- `Data/Campaign1851/cities_neighbours_1825.json`
- `Data/Campaign1851/cities_neighbours_1851.json`
- `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md`
- `Docs/Import-Preussen.md`
- `Docs/Import-Sverige-Norge.md`
- `Docs/Landeskabelon.md`
- `Docs/Niveau-B-Sverige-Norge-Preussen-2026-10-17.md`
- `Source/Game1864/Private/Campaign1851Battles.cpp`
- `Source/Game1864/Private/Campaign1851Intel.cpp`
- `Source/Game1864/Private/Campaign1851Map.cpp`
- `Source/Game1864/Private/Campaign1851Neighbours.cpp`
- `Source/Game1864/Private/Campaign1851War.cpp`
- `Source/Game1864/Private/SCampaign1851Overlay.cpp`
- `Source/Game1864/Public/Campaign1851Map.h`
- `Source/Game1864/Public/Campaign1851SaveGame.h`
- `Tools/Countries/integrate_level_b.py`
- `Tools/Countries/validate_level_b.py`
- `Tools/Countries/validate_prussia.py`
- `Tools/Countries/validate_sweden_norway.py`
- `Tools/Map1851/README.md`
- `Tools/Map1851/build_map.py`
- `Tools/Map1851/extend_north.py`
- `Tools/Map1851/shp.py`
- `Tools/Map1851/validate_neighbours.py`

Supplerende slutkontrol: Den nye streaming-generator er kørt fra HEAD-data i en midlertidig mappe og genskaber alle tre aktive PNG-rastre byteidentisk. Strengt JSON-format (inklusive afvisning af NaN/Infinity), Python AST og streaming-shapefilernes recordrækkefølge er kontrolleret. `--extend-north` er også genkørt mod arbejdsdata som no-op.


## Niveau C, første del — 2026-10-18

Begge bestilte dele er implementeret og statisk valideret. Ingen build, spil/editorstart, .bat-kørsel eller commit. Der er ikke ændret i `Source/Strategy1864`. Niveau B var ifølge projektejeren kompileret uden fejl; dagens Niveau C-ændringer er **ikke** kompileret eller runtime-testet.

Personer og kommando:

- `ForeignPersonnel_1825.json` og `ForeignPersonnel_1851.json` aktiverer landepakkernes officerer (4/6 personer) og alle SE/NO-ressorter. Arkiverne er byteuændrede; `integrate_level_c.py` genskaber de aktive filer.
- `Campaign1851Officers.cpp` indlæser personerne som samme `FCampaign1851Officer` som danskerne, men i nationale registre uden dansk ansættelses-/udnævnelsesadgang. Pakkernes officerstats er 1–10 i begge scenarier; de bliver ikke fejlagtigt divideret med ti som Danmarks 1825-data. Ukendt fødselsår bevares som ukendt.
- Nabohærene henviser til officerens ID i den eksisterende `officer`-række; SE/NO bruger scenariets nationale feltchef. Flere regimenter under samme korps deler chef/stab som et eksplicit spilskøn. `NeighbourMarchPace` bruger nu den levende officers stab gennem samme `StaffPaceFactor` som Danmark. Daglig moralgenopretning bruger samme ledelse/inspiration-formel som danske regimenter; gennemsnitlig uddannelse 50 er et markeret skøn.
- `Campaign1851Ministers.cpp` indlæser samme `FCampaign1851Minister` som Danmark. Opslagene `ForeignCommander(nation, id)` og `ForeignMinister(nation, portfolio)` kan bruges af diplomati/events. Ministerstats 0–100 omsættes til 1–10; samme person med flere ressorter får ikke ekstra løn eller en ekstra rekruttering.
- PR-pakken har ingen ministerliste. Dokumenteret udenrigsminister er derfor tilføjet i den aktive fil: [Christian Günther von Bernstorff i 1825](https://www.deutsche-biographie.de/gnd116147695.html), [Otto von Manteuffel i 1851](https://www.deutsche-biographie.de/sfz57925.html). Stockhausen i 1851 er genbrugt fra pakkens dokumenterede krigsministeroplysning; han bliver ikke feltchef. Øvrige preussiske ressorter er ukendte, ikke opdigtede. De nye ministeregenskaber er neutrale spilskøn.
- Udenrigsvinduet viser udenrigsminister, officerantal og national kasse. Peg på landets navn for den skrivebeskyttede personliste med ressorter, roller og stabs-/ledelsesværdier. Der er ingen nye klik-handlere. Skærmlayout og tooltip er endnu ikke runtime-afprøvet.
- Personerne er statisk scenariegrundlag. Der er endnu ikke tilføjet udenlandske officerskarrierer, løbende udnævnelser, fangenskab eller historisk tvangsskifte af ministre. Nye events kan vælge andre udfald; eksisterende events er urørte.

Økonomi og gemte spil:

- `ForeignEconomy_1825.json` og `ForeignEconomy_1851.json` vælger produktion/eksporttold pr. scenarie. **Alle satser og startkasser er markerede spilskøn**, ikke verificerede nationale budgetter eller historiske valutakurser.
- De aktive distrikter leverer by-/landskat og eksporttold til hver sin nationale SE-, NO- eller PR-kasse. Det er økonomien for kortets dækkede distrikter; der tilføjes ikke skjulte indtægter fra hele Sverige, Norge eller Preussen uden for kortet.
- Produktions-/eksportberegningen er udskilt fra den danske funktion til en fælles funktion: landbefolkning × scenariets produktion pr. indbygger × høst × pris × krigs-/søfaktor. Kun tolden på producenternes omsætning tilfalder staten, præcis som i den danske eksportmodel. Dansk beregning og scenarievalg bevares. Udenlandske distrikter får ikke Danmarks reform-/forsknings-/humørbonusser. Besættelse af distriktets sæde standser dets indtægt, et forenklet skøn.
- Reserver og årgangslofter forbliver lokale; pengene deles kun inden for samme nation. SE og NO har fortsat adskilte konti. Reserveindkaldelse, erstatninger og magasiner debiterer nationalkassen; andre naboer beholder Niveau B's lokale kasser. Ingen dansk `AddTransaction` bruges.
- Månedens indtægter bogføres, derefter underhold og til sidst magasinindkøb. Underhold bruger den danske `RaisedUpkeepPerMonth` pr. tilsvarende standardenhed, valgt pr. scenarie; blandede korps regnes som infanteri (skøn). Udstationerede korps erstatter fredskadren i regnskabet, så den ikke betales dobbelt. Ubetalt underhold tømmer kassen uden gratis kredit og giver et proportionalt moraltab (maks. 0,1 pr. måned, skøn).
- Mobilisering køber kun så mange reservemænd, som reserve og national kasse rækker til. Delvise trin forsøges igen dagligt. `StartMen` holder kampens tab uden for det ekstra mobiliseringsmål, så tab ikke genskabes gratis. Månedlige erstatninger begrænses altid til faktisk indkøbt `StartMen`, også når det tidsmæssige trin er fuldt; de kan ikke købe den samme mobiliseringsmangel en ekstra gang. Den eksisterende fredskadre opkræves ikke som nye reservister.
- Saveversionen er **39**. `UCampaign1851SaveGame::ForeignTreasuries` gemmer de tre kasser. Ved v38 og ældre overføres de indlæste distriktskasser én gang og nulstilles lokalt; korpsenes tab, positioner og forsyning bevares i de eksisterende savefelter. Ved v39 genindlæses de gemte nationale beløb efter distriktsrestore; ugyldige tal/ukendte nationsnøgler afvises. Meget gamle saves uden distriktsrækker får fortsat scenariets eksisterende startkasser.
- Statiske officerer, ministre og økonomisatser genindlæses fra det valgte scenarie og er ikke nye mutable savefelter. Ældre saves opgraderes fremad; v39 bør ikke åbnes og gemmes med ældre kode. Migration og Unreal-serialisering skal stadig runtime-testes. Gamle abstrakte korps betaler også nationalt underhold, men får ingen automatiske erstatninger, når deres distriktsidentitet mangler; deres eksisterende kampstatus bevares.

Validering (alle kommandoer exit 0):

```text
python Tools/Countries/validate_sweden_norway.py
OK SE/NO: 118 JSON; danske indeks/geografi/pixels og beskyttede gridceller bevaret; arkivhashes OK; fuld genimport x2 byte-idempotent.
python Tools/Countries/validate_prussia.py
OK PR: 118 JSON; danske indeks/geografi/pixels og beskyttede gridceller bevaret; arkivhashes OK; fuld genimport x2 byte-idempotent.
python Tools/Map1851/validate_neighbours.py
CAMPAIGN-1851|neighbours|valideret|1825|122 byer|153 ruter|139 hære
CAMPAIGN-1851|neighbours|valideret|1851|122 byer|153 ruter|139 hære
```

Alle tre kalder desuden den nye fælles `validate_level_c.py`:

```text
OK niveau C 1825: 4 officerer; SE/NO ressorter og PR udenrigsminister; 3 separate økonomier; relation -20.
OK niveau C 1851: 6 officerer; SE/NO ressorter og PR udenrigsminister; 3 separate økonomier; relation -20.
OK niveau C: 118 JSON; genimport x2 byteidentisk; statisk kommando/betaling/save v39; budgetreference: nul/delvis/genforsøg og DK-adskillelse.
git diff --check
Exit 0; ingen whitespacefejl.
```

Budgetreferencen er en Python-kontrol af kravene og kontoadskillelsen; den kører ikke C++-økonomien eller Unreal-save/load. JSON læses strengt med afvisning af NaN/Infinity. Niveau B's fulde isolerede genimport indeholder nu også Niveau C-generatoren. Historiske PR–DK-relationer forbliver −20 i begge scenarier, og arkivhashes er bevaret.

UE 5.8-headerkontrol, uden kompilering:

- `Runtime/Json/Public/Dom/JsonObject.h`: `TryGetNumberField` float/double/int32 (414/417/426), `TryGetStringField` (450), samt de eksisterende JSON-hjælperes objekt-/arrayopslag. `Serialization/JsonReader.h` og `JsonSerializer.h`: Create/Deserialize-overloads.
- `Runtime/Core/Public/Containers/Map.h.inl`: Reset (156), Add (323–326), Find const/mutabel (489/498), FindOrAdd (603–604). `Array.h`: FindByPredicate (1710/1723), SetNum (2531), Add (2948/2961), IsValidIndex/Num/IsEmpty.
- `Misc/FileHelper.h` (130) og `Misc/Paths.h` (262): LoadFileToString/ProjectDir. `Containers/UnrealString.h.inl`: IsEmpty, sammensætning, stier og Printf (1376). `GenericPlatform/GenericPlatformMath.h`: FloorToInt, RoundToInt, IsFinite; `Math/UnrealMathUtility.h`: Clamp og de anvendte Min/Max-operationer. Ingen nye UE-includes.

Tilbage for Niveau C: fuld national spillerstyring, bygninger/industri/råvarelagre og fuld national geografisk økonomidækning, verificerede distriktsgrænser/bytal, komplette preussiske kabinetter, officerskarrierer/udnævnelser, ministerudskiftninger, forskning, egen flåde, uniforms-/enhedsgrafik samt fuld koalitions- og bilateral krigsmodel. Der mangler godkendt runtime-/save-/slagafprøvning. Startkasser og underhold er endnu ikke balanceret ved langtidssimulering.

Ændrede filer i denne Niveau C-del (tidligere Niveau B-ændringer og andre agenters filer er ikke denne opgaves ændringer):

- `Data/Campaign1851/ForeignPersonnel_1825.json`
- `Data/Campaign1851/ForeignPersonnel_1851.json`
- `Data/Campaign1851/ForeignEconomy_1825.json`
- `Data/Campaign1851/ForeignEconomy_1851.json`
- `Source/Game1864/Private/Campaign1851Officers.cpp`
- `Source/Game1864/Private/Campaign1851Ministers.cpp`
- `Source/Game1864/Private/Campaign1851Neighbours.cpp`
- `Source/Game1864/Private/Campaign1851Economy.cpp`
- `Source/Game1864/Private/Campaign1851Manpower.cpp`
- `Source/Game1864/Private/Campaign1851Intel.cpp`
- `Source/Game1864/Private/Campaign1851PlayerController.cpp`
- `Source/Game1864/Private/SCampaign1851Overlay.cpp`
- `Source/Game1864/Public/Campaign1851Map.h`
- `Source/Game1864/Public/Campaign1851SaveGame.h`
- `Tools/Countries/integrate_level_c.py`
- `Tools/Countries/validate_level_c.py`
- `Tools/Countries/validate_level_b.py`
- `Tools/Countries/validate_sweden_norway.py`
- `Tools/Countries/validate_prussia.py`
- `Tools/Map1851/validate_neighbours.py`
- `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md`
- `Docs/Niveau-B-Sverige-Norge-Preussen-2026-10-17.md`
