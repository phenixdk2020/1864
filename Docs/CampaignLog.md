# Kampagnens beslutnings- og tilstandslog

Logningen er altid aktiv under en startet kampagne og har kun analyseformaal. Den aendrer ikke spillets regler, tilfaeldighed eller saveformat (fortsat version 39). Den kraever intet flag. `-CampaignEventLog` styrer fortsat kun den eksisterende verbose Unreal-log.

## Filer og analyse

Loggen ligger i `Saved/CampaignLogs/<UTC-start>_<scenarie>.jsonl`, eksempelvis `20261020T120000_123Z_1825.jsonl`. Ved navnesammenfald indsaettes et loebenummer. UTF-8 uden BOM, en JSON-post pr. linje. Bufferen flushes ved hvert skift af spildato, ved 32 Ki tegn samt ved save/quit. Der skrives ingen logfelter i savegame.

Fra repoets rod:

```powershell
python Tools/Campaign/validate_campaign_log.py Saved/CampaignLogs/DIT_LOGNAVN.jsonl
python Tools/Campaign/analyze_campaign_log.py Saved/CampaignLogs/DIT_LOGNAVN.jsonl --out Saved/CampaignReports
```

Validatoren ligger under Source for at overholde opgavens skrivegraense, som udelukker aendringer i Tools. `--strict-kinds` afviser ukendte beslutningstyper; ellers advares der for at bevare analyserens fremadkompatibilitet. Generatorens bevidste `weird_new_kind` giver derfor en advarsel, men ikke en skemafejl. Analyseren kraever matplotlib for diagrammer.

## Registrering

- `session`: new/load/save/quit med ISO-spildato, scenarieaar, faktisk saveversion og slot. Kun vellykkede saves logges; Autosave hoejst en sessionspost pr. spildato. Hvert save flusher og opdaterer slot-linket, ogsaa naar posten undertrykkes.
- `decision`: alle `AddDecision`-poster, herunder raad (inputs.advice=1) og handlinger (inputs.done=1), nation, portefolje, pris, score, handling og begrundelse. Begivenheder har actor=event. Spillerkald identificeres med en scoped aktorkontekst i controlleren; handlingerne registreres inde i Map-funktionerne. AdvanceTime og ministerudfoerelse saetter deres egen kontekst.
- `war` og `peace`: krigserklaeringer fra PR og AT, fredsslutning, fredskonference og politisk maegling. `war` er forbeholdt erklaeringer, fordi den eksisterende analyser behandler disse poster som krigserklaeringer.
- `relation` og `alliance`: diplomatiske handlinger, maanedlig relations-/spaendingsdrift og allierede brigaders ankomst.
- `mobilise`, `march` og `recruit`: indkaldelse/hjemsendelse, dansk fuld mobilisering, udenlandske mobiliseringstrin, korpsenes rutevalg/afmarch/ankomst, spillerens marchordrer, bataljoner, forsyningskolonner og officerer.
- `budget`, `tax`, `build` og `research`: laan/afdrag, prioritering og ministerbudgetter, udenlandsk skat/eksport/underhold/depotkoeb/rekrutteringsudgifter, abstrakte nationale maanedsbudgetter, skibe/tog/byggeri/infrastruktur og forskning/doctrin inkl. forskningsafslutning.
- `event`: hver uafgjort, forfalden begivenheds faktiske evalueringsforsoeg, ogsaa naar forudsaetninger fejler eller et aabent vindue proever igen senere. possible, effektiv sandsynlighed pr. forsoeg, fired og forklaring med det allerede beregnede kast. Ingen ekstra tilfaeldighedstraek. Tvungne debug-events kan have possible=false og fired=true.
- `other`: ministerudnaevnelse/-afskedigelse, delegation, udfoerelse af raad, blokade, unionskampe med tab for begge nationer, belejringsstart, besaettelse og befrielse. Disse militaere foelger skal ikke fremstaa som nye krigserklaeringer i rapporten.
- `snapshot`: en post pr. nation, ogsaa spillerens og inaktive nationer, hver mandag efter dagens simulering. Faktisk kasse og tilstedevaerende haerstyrke, mobilisering og distrikter hvor de findes. Danske indtaegter/udgifter er det aktuelle maanedsbudget; udenlandske tal er senest observerede maanedsstroemme. Efter load udelades de udenlandske stroemme indtil naeste maanedsafslutning, fordi analysecachen ikke gemmes. Relationsdata omfatter kun modellens eksisterende relationer til Danmark; der opfindes ingen fuld matrix eller stabilitetsvaerdi.

## Genoptagelse og begraensninger

`slot_<slotnavn>.txt` i logmappen forbinder hvert slot med logfilen og savefilens SavedAt-tidsstempel. Load genoptager kun en eksisterende lokal fil med samme tidsstempel og scenarie. Nyt spil faar altid en ny fil; gamle saves uden sidecar starter en ny log med load. Autosave efter et 3D-slag genoptager dermed samme gennemspilnings log. Logmappe og sidecars skal foelge med, hvis saves flyttes. Et tilbagespring til et tidligere save fortsaetter samme log, saa datoer kan gaa baglaens. quit betyder controllerens EndPlay, og kan derfor ogsaa markere niveauovergang til slag.

En skrivefejl stopper kun logningen og giver en Unreal-advarsel. Et crash kan miste den ikke-flushede buffer; ved genoptagelse isoleres en eventuel afkortet slutlinje, som analyseren kan springe over. Sidecar og save er separate filskrivninger: afbrydelse mellem dem kan betyde en ny log ved naeste load. Der er ingen runtime-verifikation endnu, da build, editor og spilstart var udtrykkeligt forbudt.

## Kontrol 2026-10-20

Eksisterende syntetisk generator og analyser blev koert uforandret i en midlertidig mappe: 1.889 poster, 4 nationer, 4 sessioner, 193 beslutninger, 20 eventposter og 1.672 snapshots. Skemavalidering gav 0 fejl; HTML-rapporten blev genereret. Forkerte datoer, ikke-endelige tal, ugyldige aktoerer og ugyldige eventfelter blev afvist. UE 5.8-headere blev kontrolleret statisk; ingen build, spilstart, .bat eller commit.

## Aendrede filer i denne opgave

Nye filer: `Source/Game1864/Private/Campaign1851DecisionLog.h`, `Source/Game1864/Private/Campaign1851DecisionLog.cpp`, `Tools/Campaign/validate_campaign_log.py` og dette dokument.

Instrumenteret: `Source/Game1864/Public/Campaign1851Map.h` samt foelgende filer under `Source/Game1864/Private/` (alle med praefikset `Campaign1851`): `Army.cpp`, `Bridges.cpp`, `Diplomacy.cpp`, `Economy.cpp`, `Endgame.cpp`, `Forts.cpp`, `Manpower.cpp`, `Map.cpp`, `Ministers.cpp`, `Mobilisation.cpp`, `Nations.cpp`, `Navy.cpp`, `Neighbours.cpp`, `Network.cpp`, `Officers.cpp`, `PlayerController.cpp`, `Politics.cpp`, `Research.cpp`, `Resources.cpp`, `Supply.cpp` og `War.cpp`.

Eksisterende aendringer i andre filer, inklusive historikfilen og Strategy1864, er ikke bearbejdet.
