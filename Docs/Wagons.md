# Vogne og heste i 3D-slaget

`UStrategyWagonTrainComponent` ejer et vogntog; hver vogn er et separat `AStrategyWagonTrain`-mål baseret på det eksisterende forsyningsvognssystem. Vogne bruger de importerede `SM_Wagon_Supply`, `SM_Wagon_Empty` og `SM_Wagon_Mortar` med deres assetmaterialer. En vogn skaleres efter mesh-bounds til cirka 4,2 × 2,0 m. To statiske heste og simple trækstænger vises pr. vogn, så længe enhedens hestetal rækker; heste bevæger kroppen proceduralt ved march og står med en lille tomgangsbevægelse ved stop.

## Antal og afstande

| Enhed i testscenarier | Standard | Afstand til første vogn |
|---|---|---|
| Bataljon / bataljonsstab | 2 vogne, skiftevis ammunition og forsyning | 60 m |
| Regimentstab | 2 vogne | 60 m |
| Feltbatteri | 1 ammunitionsvogn pr. 2 kanoner, rundet op | 25 m |
| Morterbatteri | 1 mortervogn pr. morter | 25 m |
| Brigade- og divisionsstab | 4 forsyningsvogne | 250 m |

Der er 9 m mellem vognene. Teststandarderne aktiveres med `-Strategy1864Train`, også i skirmish. Kompagnier og ryttereskadroner får ingen teststandard. Kampagnens `Saved/Battle/Units.json`-rækker leverer `wagons` og `horses` til bataljonsstabe, rytteri og batterier; eksplicit nul betyder intet vogntog og tilsidesættes ikke af flaget. Den syntetiske felthærsstab får ikke ekstra kampagneudstyr. Antallet begrænses til 128 vogne og 256 heste pr. tog. Rytteriets rideheste reserveres før trækhestene fordeles. Batteriets trækheste flyttes fra batteriets beholdning til vognmålene, så tabsrapporten ikke tæller dem to gange. Overskydende heste bliver i batteriet; hver vogn viser højst to.

Toget optager den tegnede enheds positionsspor (ogs? stabens tegnede centrum) og finder vognens plads efter tilbagelagt afstand langs sporet. Det tager derfor samme hjørner frem for at søge direkte mod enheden. Sporet beskæres bag sidste vogn. Toget stopper, når enhedens tegnede position stopper; en vogn uden mobilitet, en forladt eller erobret vogn bliver stående. Ved initialisering bruges et lige spor bag startfronten, da tidligere marchhistorik ikke findes. Terrænprojektion sker ved hver opdatering. Hjulene er del af de samlede static meshes og roteres derfor ikke separat.

## Kamp, erobring og ammunition

Eksisterende infanteri- og artilleriskade rammer vognmålene og reducerer kuske, heste, vognens stand og last. En ødelagt vogn vises væltet. En fjendtlig kampenhed inden for 3,5 m af vognen forlader den som overløbet. Det eksisterende erobringssystem kræver derefter 6 sekunder inden for 10 m uden effektiv venlig beskyttelse inden for 15 m. Forsyningsenheder og alle stabe kan ikke selv erobre eller beskytte vogne.

`BattleLedger.Captured` / `Lost` føres gennem den eksisterende erobrings- og tabsregistrering. `DrawAfterActionReport` viser vognrækker og totaler med H=heste og V=vogne; JSON-ledgeren forbinder vognen til ejerens campaignId. Vognudstyr indgår også i regimentets kampagneresultat. Transporterede morterer tælles på vognen, og erobring/ødelæggelse fjerner morteren fra batteriets operative antal én gang. Batteritræffere ødelægger ikke samtidig de morterer, som stadig ligger på separate vognmål. Opstillede morterer tilhører batteriet, mens vognen er tom.

En operativ eller erobret venlig ammunitionsvogn inden for 150 m af et effektivt kompagni overfører højst 120 patroner hvert 10. simulationssekund, begrænset af kompagniets manglende `AmmunitionRounds` og vognens resterende last. Standardlasten er 12.000 patroner; batteriets ammunitionsvogne har også 320 artilleriskud som last. Dette tog overfører kun håndvåbenpatroner automatisk. Lasttal, afstande og overførselshastighed er spilestimater. Det normale forsyningskildetick er slået fra for vognmålene, også efter erobring, så samme last ikke uddeles to gange.

Logpræfiks: `PROJECT1864-TRAIN` ved oprettelse, manglende assets, overløb, erobring, ødelæggelse og patronoverførsel.

## Rigget hest til senere brug

`Content/Python/import_horse_rigged.py` er et manuelt editor-importscript, som ikke er kørt i denne ændring. Det opretter `/Game/Units/Horse/SK_Horse_1864` som SkeletalMesh, `SK_Horse_1864_Skeleton` som fælles skelet og de syv `A_Horse_Stand/Walk/Trot/Canter/Gallop/Rear/Fall`-klip fra `Reference/Animations/Horse`. Runtime bruger fortsat `SM_Horse_Static`.

Angiv `PROJECT1864_HORSE_FBX` for en allerede eksporteret mesh-FBX. Ellers eksporteres riggen fra `PROJECT1864_HORSE_BLEND` med `PROJECT1864_BLENDER` eller Blender på PATH. Scriptets eksisterende standard peger på `Horse_1_rigged_v3.blend` i projektets eksterne Reference-mappe; en anden maskine skal angive egen sti. Mesh-FBX uden animation genbruges fra `Saved/HorseImport`. Importen kontrollerer assettyper og fælles skelet og genbruger eksisterende assets. Mesh og klip skal have samme rig; importfejl stopper scriptet frem for at erstatte den statiske runtimehest.

## Kontrol

Statisk C++-gennemgang, kontrol af nye Unreal-kald/includes mod UE 5.8-headerne, Python-syntakskontrol og `git diff --check`. Ingen build, editor-/spilstart, import eller performancecapture er udført. Manuel kontrol i spillet skal omfatte et skarpt hjørne, stop, få/manglende heste, mere end otte vogne, morterens opstilling/pakning, gentagne træffere, beskyttet/ubeskyttet erobring og genforsyning på begge sider af 150 m-grænsen.
