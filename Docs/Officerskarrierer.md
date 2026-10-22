# Officerskarrierer — Niveau C

Danmark, Sverige, Norge og Preussen benytter samme karrieremodel. Sverige og Norge har hver sin pulje i den eksisterende unionsmodel. 1825 er standard; 1851 læser fortsat sine egne officerer. Historiske navne, fødselsår og startrang bevares; nye reservepersoner er eksplicit `historical: false`. Ingen historiske døds- eller pensionsdatoer styrer simulationen.

Alle følgende tal, rangkrav og puljestørrelser er **spilskøn**, ikke dokumenterede tjenesteregler.

| Regel | Værdi |
|---|---|
| Erfaring | 0–100; optjent karriereerfaring tælles særskilt fra scenariets starterfaring |
| Kommandotjeneste pr. måned | +0,25; +0,75 hvis tilknyttede tropper marcherer ved månedsskiftet |
| Aktiv reserve pr. måned | +0,10 for reservetjeneste/studier |
| Accepteret slag | +4 og ét registreret slag, højst én gang pr. officer pr. afgørelse |
| Udmærkelse i accepteret 3D-rapport | +2 ekstra |
| Evnebonus | +1 føring og inspiration pr. 25 optjente point; højst +2, samlet egenskab højst 10 |
| Forfremmelse | Ét trin; aktiv tjeneste, ledig post samt 24 måneder i rang eller 12 måneder og mindst tre slag |
| Erfaringskrav | Major 30, oberstløjtnant 40, oberst 50, generalmajor 60, generalløjtnant 70, general 80 |
| Udvælgelse | Erfaring + 0,1 × måneder i rang; danske reserver før nye ansættelser |
| Pension pr. måned | Fra 60 år: min(20 %, (alder−59) × 0,4 %); yderligere 2 procentpoint ved mindst to sår og aktuelt sårfravær |
| Naturlig død pr. måned | Fra 45 år: (0,4 % + (alder−45) × 0,2 %) / 12; ukendt fødselsår giver ingen aldersrisiko |
| Automatisk slag / udenlandsk chef | Ved tab: dødsrisiko 0,2 % + 2 % × tabsandel; samlet risiko for død eller sår 2 % + 10 % × tabsandel |
| Sårfravær | 21–84 dage; 3 % risiko for dødeligt sår ved dansk 3D-sårrapport; eksplicit `dead`/`killed` accepteres også |
| Årlig tilgang | Danmark: eksisterende otte fiktive kaptajner; SE/NO/PR: én fiktiv kaptajn pr. land, alder 26, erfaring 20 |
| Udenlandsk startreserve | Fem tydeligt fiktive officerer pr. land og scenarie; kaptajn til generalmajor, neutrale evner 5 |

Rangstigen bruger danske visningstekster: kaptajn → major → oberstløjtnant → oberst → generalmajor → generalløjtnant → general. Sveriges øverste tekst er »General af kavaleriet«, Preussens »General af kavalleriet« som i landepakkerne; Norge bruger »General«. Arkivets kvalificerede norske generalmajor-titel genkendes uden at overskrive startrangen. Udenlandske feltposter kræver generalmajor eller højere; AI avancement fra juniorpuljen stopper ved denne minimumsrang. En eksisterende aktiv general prioriteres som afløser.

Månedsskiftet opdaterer anciennitet, fravær, erfaring og aldersrisiko. Det danske krigsministerium besætter ledige poster fra puljen og forfremmer kvalificerede reserver til resterende ledige poster. Spilleren kan stadig bruge eksisterende forfremmelse, `AssignOfficer` og `RecruitOfficer`. En forfremmelse frigør den gamle post og besætter en konkret ledig post. En ny ansættelse oprettes kun, når rekrutteringsforløbet ikke finder en egnet reserve. Ingen nye klikhandlere er tilføjet.

Døde og pensionerede bliver i arkivet med deres identitet, journal og status; de kan ikke udnævnes, får ingen erfaring og oppebærer ingen dansk officersløn. Sår og fangenskab frigør posten. Udenlandske afløsere udnævnes ved slagtilbageførsel eller månedsskifte; danske ledige poster håndteres ved næste månedsskifte eller af spilleren. Udtømte puljer kan efterlade ledige poster, indtil en kvalificeret officer er til rådighed. Den eksisterende udenlandske model kan dele en general mellem flere hærposter; erfaring tælles én gang, og modellen opfinder ikke separate historiske chefer.

## Hookpunkter og gemning

- `CloseMonth` → `MonthlyOfficerCareers`: fælles månedstik; januar kalder fortsat `YearlyOfficers` for skolekandidater.
- `ApplyBattle` → `BattleOfficerCareer`: fælles accepteret slagresultat, før tab/tilbagetog ændrer tropperne. Omfatter danske regiments-, kompagni-, kommando- og formationsposter samt modstanderens kendte chef.
- `RecordBattleOfficerCareers`: eksisterende 3D-deltagerjournal og udmærkelser. `ApplyOfficerCasualties`: dansk 3D-fravær/død. Der rulles ikke ekstra automatisk dansk sårrisiko oven i 3D-rapporten.
- `DailyOfficers`: eksisterende dansk helbredelse/udveksling; udlandet vender tilbage ved månedstik.
- `ReplaceForeignOfficers`: opdaterer både officer-id og synlig cheftekst; manglende chef giver neutral marchfaktor.
- `Stat`: lille, afgrænset bonus på eksisterende føring og inspiration. Grundegenskaberne overskrives ikke.
- `LogOfficerCareer` → `LogCampaignDecision`, kind `other`, handling `OfficerCareer`: journal og JSONL med nation, id, navn, rang, proveniens, erfaring, anciennitet og status. Årlig tilgang logges også.

Saveversion **40** gemmer danske og udenlandske officerer i `UCampaign1851SaveGame::Officers`, adskilt med `Nation`. Hver post gemmer `bHistorical`, `CareerStatus` (0 aktiv, 1 pensioneret, 2 død), `RankMonths`, `CommandMonths`, `BattlesServed`, `Wounds` og `CareerExperience` samt de eksisterende felter. `CareerCommanderIds` gemmer nationale standardchefer; `Neighbours` gemmer hver hærs chef-id som `career-officer|hær-id|officer-id`.

Ældre saves: tom `Nation` betyder Danmark; nye tællere starter på nul uden retroaktiv evnebonus. Historisk dansk proveniens genskabes fra eksisterende historiske id-præfikser. Udenlandske puljer genindlæses fra det valgte scenarie, når savefilen ikke indeholder deres karrierer. Eksisterende danske poster gendannes uden forfremmelse. Døde/pensionerede kan ikke genbesætte formationsposter ved indlæsning. O-/R-/K-id-tælleren gendannes også fra inaktive officerer.

## Statisk kontrol og afgrænsning

Kør `python -B Tools/Countries/validate_officer_careers.py` og `python -B Tools/Countries/validate_level_c.py`. De kontrollerer begge scenarier, identiteter, alders-/evneintervaller, eksplicit fiktiv proveniens, uændrede historiske arkivfelter, savefelternes kobling og referencegrænser. Niveau C-validatoren genimporterer også i en midlertidig mappe to gange og sammenligner byteidentisk.

UE 5.8-headerne er kontrolleret statisk for de anvendte container-, streng-, dato-, tilfældigheds-, hash-, matematik- og JSON-kald. Dette erstatter ikke C++-kompilering, UHT eller runtime-tests. Modellen måler kommandotid/felttjeneste ved månedsskiftet og er derfor grov ved skift midt i måneden. Langtidsbalance, hyppige udskiftninger blandt de gamle 1851-chefer, store journaler og rigtige save/load-forløb kræver senere godkendt spiltest. Ingen build, editor, spil eller launcher er kørt.
