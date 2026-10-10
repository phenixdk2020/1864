# Jævn bevægelse i 3D-slaget

## 2026-10-10: Ordnet omformering og individuel ankomst

Fodfolkets tegnelag fordeler eksisterende mænd til nye pladser én gang ved omformering: linjen forlades fra venstre fløj, fil for fil med forreste mand først; kolonnen deployerer fra hovedet. Sorteringen følges af endpoint-bytninger, der forkorter krydsende ruter. Figuridentitet, VAT-plads og animationsfase bevares; `SoldierSlots` følger den nye plads, så rækkeild fortsat bruger korrekt rang. Slots fra `StrategyFormationComponent` og simulationens formationer er uændrede.

Startbølgen er 0,12 s pr. fil/kolonnerække og 0,04 s mellem mændene i bølgen. Bølgen begrænses til 65 % af den justerbare `FormationChangeDeadlineSeconds` (standard 7 s). Personlig fart ligger i 0,95–1,05. Indhentningen bruger mindst løbefart eller restafstand/resttid; nedbremsning sker kun inden for 30 cm, med mindst 120 cm/s til de sidste 10 cm. Derefter dæmpes farten, mens deadline stadig gælder. Bevægelsestrinnet begrænses til restafstanden, så manden ikke overskyder sin plads. En lokal rumlig hash med fire afstandspassager giver små sidekorrektioner mellem nærliggende mænd under omformering; ventende filer og standsede mænd flyttes ikke. Der er ingen terrænsøgning eller global parvis kollisionskontrol pr. frame. Tætte klynger og faktisk kropsbredde skal stadig kontrolleres visuelt.

På sidste rutestykke inden for 25 m bruger en stående linje individuel ankomst, når destinationsfronten afviger mindst 20°. Hver mand går mod sin faste verdensplads i destinationsboksen og standser og vender sig dér. Marchfarten gemmes ved overgangen, så de bageste ikke sænkes, når actorens march afsluttes. Ankomstbudgettet omfatter den resterende gangtid (længste rute / 160 cm/s) plus omformeringsbudgettet; ankomst over 25 m presses derfor ikke ind i syv sekunder. Små slutvinkler og den øvrige formationsmarch bruger fortsat `FStrategyVisualFormationPath`. Karré, liggende/spredt orden, udskilte skytter og bevægelse med bevaret front beholder deres særlige regler.

`HasVisualArrivalFrontReached()` skelner den første standsede mand ved fronten fra `IsVisualFormationComplete()`, der først bliver sand efter den sidste mands stop og slutdrejning. HUD viser **ANKOMMET / FORMERER** imellem de to og skjuler keglen under den visuelle omformering. Simulationens actor, ordrer, ankomsttolerance og reformeringstimer er uændrede. Flag og HQ læser fortsat de faktisk tegnede mænd; ankomstens tegnede front bruger boksens slutretning frem for actorens igangværende slutdrejning. Ved ny ordre, ændret mål eller bevægelsespause frigives de faste ankomstmål. Tab og ændret figurtæthed omfordeler ikke overlevende.

`-Strategy1864DebugFormation` skriver én afslutningslinje pr. visuel omformering/ankomst:

```text
Strategy1864DebugFormation <enheds-id> complete-seconds=... longest-path-cm=... crossing-pairs-avoided=... arrival=0|1
```

Tiden er simulerede sekunder på tegnelagets animationsklokke. Rutens længde er verdenscentimeter ved planlægningen; tælleren er antal krydsningsfjernende endpoint-bytninger (samme par kan behandles igen). Fortsat march tæller ikke som en uafsluttet omformering, når mændene har indhentet de nye pladser. Afbrudte planer giver ingen afslutningslinje.

Statisk UE 5.8-headerkontrol og `git diff --check` er udført. Numeriske modeller kontrollerer deadline, fartgulv og enkeltvis ankomst; de kompilerer eller afprøver ikke C++/VAT. Afprøv senere linje↔kolonne, 19°/20°/90°/180°-ankomst, pause, nye ordrer, waypoint-ruter, tab, figurtæthed og x10. Ingen build, spil/editorstart eller commit.

## 2026-10-10: VAT på alle afstande

Fodfolk, kavaleriets ryttere, stab og ordonnanser bruger gemte VAT-modeller som normal tegnevej. Det globale nærbudget er 24 levende skeletfigurer, valgt efter individuel kameraafstand (18 m ind, 20 m ud). `-Strategy1864CrowdNearest=N` ændrer budgettet. Transform og `FPlayedClip` bevares ved skift. Nærposen evalueres ud fra samme `VisualAnimationTime`, inden VAT-instansen skjules; skeletfiguren har ingen uafhængig animationstick. Farver, fase, play rate, pause og tidsdilation føres gennem instansdata.

Levende/faldne har hver en tabel fra figuridentitet til stabil instansplads. Tab og ændret figurdivisor nulstiller ikke ISM'en; overlevende beholder deres instans-ID. Ledige pladser genbruges. Bagning sker manuelt med `Content/Python/bake_all_vat.py`, aldrig under tick. Manglende/ufuldstændige assets giver skeletfallback med `PROJECT1864-CROWD`-log.

Stance-overgange bruger importerede rifle-klip, med knælende som mellemtrin mellem stående og liggende. Personlige skud afventer aktive overgange; ready/aim bevarer stance. Mundingsposition samples fra den bagte virtuelle geværbone. Dødsclip holder sin slutpose. Rytterens siddestilling samples ved 0,05 s og følger de eksisterende udjævnede sadeltransforms. Sabelplaceringen bevares, før skeletmesh frigives. Ryttertab skifter til separate faldne instanser uden at flytte overlevendes pladser og deler ligloftet med infanteriet. Heste og flag er fortsat statiske.

Overgangen varer 0,2 s med komplementære pixelmasker for VAT, skeletfigur og gevær. Demoterede figurer frigiver deres skeletplads efter udtoning; nye nærfigurer venter på en ledig plads, så N-loftet bevares. VAT-lig bruger samme dækningsgrad til fade fra 60 til 65 s.

Statisk kontrol er udført; ingen build, editor eller spil er startet. Maskernes visuelle kvalitet skal afprøves: VAT interpolerer 15 samples/s, skeletposen evaluerer klippet direkte, og materialer/skygger kan afvige. LOD0 og bagte normaler reducerer silhuet-/lysafvigelser. Se `Docs/Performance-Battle.md` for asset-kørsel og kontrolpunkter.

Dato: 2026-10-08. Kodegennemgang af `Source/Strategy1864`; ingen Unreal-build eller afprøvning i spillet.

## Simulation til tegning

`StrategyMovementExecutorComponent` flytter allerede actor hver frame med hastighed gange simuleret delta. Terræn, formation, tilstand og vejpunkt kan ændre farten eller retningen brat. Slutmålet sættes præcist inden for ankomsttolerancen, og slutretningen sættes præcist inden for vinkeltolerancen. Overgang til næste ordrevejpunkt kan udføre næste bevægelsestick med samme delta. `RInterpConstantTo` begrænser drejehastigheden, men glatter ikke acceleration eller stop.

De autoritative simulationspositioner, ankomstregler og ordrer er bevaret. Bevægelseskomponenten har eksplicit nul tickinterval. Rettelsen ligger mellem simulation og figurer: mål kan skifte brat, mens tegnelagets position, hastighed og drejehastighed fortsætter fra foregående frame.

## Fodfolk og fælles formationssti

- `VisualPath.Center += Translation` førte actorens spring direkte til tegnelaget. Centrum følger nu positionen med en analytisk, kritisk dæmpet fjeder, inklusive højden. Der er ingen teleportgrænse eller lavt konstant hastighedsloft, der efterlader formationen permanent bagud.
- Den tidligere randpivot flyttede centrum under drejningen og lod det bagefter indhente actor med `VInterpConstantTo`. Det gav forskellige bevægelsesregler ved start og slut af svinget. Rammen drejer nu omkring sit udjævnede centrum; mændene følger buerne til deres pladser. Drejehastigheden begrænses af formationsradius og gangfart.
- Den fælles drejning havde en 1°-port. Den er fjernet. Vinkelhastigheden glider ind og ud, også for mændenes egne drejninger. Den nævnte 30°-port og gren med snap ved stor drejning findes ikke i det gennemgåede udgangspunkt.
- Mændenes tidligere konstante indhentningsfart stoppede øjeblikkeligt ved pladstolerancen. De har nu vedvarende verdenshastighed, acceleration og en bremsezone omkring målet. Arvet actorbevægelse og rotation ophæves før opdateringen.
- Hastigheds- og vinkelintegration bruger interne delstep på højst 1/120 simuleret sekund. Resultatet afleveres hver renderframe. Delsteppene ændrer ingen simulationsposition, ordre, kollisionsberegning eller spilklokke.
- Kolonnens sti blev allerede opdateret hver frame. Ved genindtræden i kolonne nulstilles nu også afstandskoordinaten; tidligere kunne gamle afstande blive kombineret med en ny sti.
- Styrkeændringer og forsinkede tab genberegnede alle pladser. De overlevende bevarer nu plads, animationsfase og personlig tilstand. Huller bevares indtil en rigtig formationsændring. Kolonnens frontreference krymper ikke ved tab. Ændret figurtæthed bevarer tilbageværende figurer; nye figurer får ledige pladser. Ved egentligt formationsskift går figurerne til deres nye målpladser.

`RefreshIntervalSeconds` begrænser ikke denne visuelle tick; den var allerede hver frame. `SoldierSettle` bruger nu den ordnede startbølge og deadline beskrevet ovenfor ved almindelig linje-/kolonneomformering.

## Animation, crowd og LOD

VAT-modellen bager cirka 15 billeder pr. sekund. Materialekilden `Content/Python/make_crowd_material.py` interpolerer allerede mellem to billeder. Bagningen er derfor ikke en 15-Hz-positionstick.

Bagningens sidste billede og shaderens looplængde passede ikke præcist til klippets længde. Der bages nu ensartede tidspunkter fra start til slut, og loopets periode udelader det ekstra slutbillede. Shaderens billedhastighed beregnes fra den faktiske kliplængde. Det fjerner den ekstra pause ved hver omstart og holder VAT og skeletanimation på samme tidsakse.

Samme gangklip genstartes ikke længere, fordi det midlertidigt har nul play rate. Rateændringer bevarer fase og udjævnes. Stop gemmer fasen i `HeldPosition`; tidligere blev den bogførte fase nulstillet, hvilket kunne ses ved VAT/skelet-skift. Ændret antal figurer nulstiller heller ikke hele animationscachen.

En lokal animationsklokke følger komponentens delta og actorens `CustomTimeDilation`. VAT-data oversætter denne fase til shaderens verdensklokke. En frosset actor kan derfor ikke fortsætte med at marchere i VAT, mens skeletversionen står stille. Globale hastigheder x0,5–x10 følger fortsat spillets tid.

En holdt fase mellem to VAT-billeder bevares med en meget langsom, ikke-loopende shaderklokke, som rebases hver opdatering. Det bruger det eksisterende materiales fire custom-data-felter og kræver ingen ny materialebagning eller ændring af `.uasset`. Fasen er numerisk næsten frosset: forskellen mellem CPU-/GPU-tid inden for en frame multipliceres med 0,001.

Crowd-transformer blev allerede opdateret hver frame under bevægelse. Ved uændret antal bruges fortsat batchopdatering, uden at rydde instanserne. `bCrowdDirty` betyder også flyttede figurer, ikke nødvendigvis ny allokering. Hysterese mellem nær- og fjernvisning bevares. Ved tilbagevenden gendannes fase og knogler før visning; selve skifteframen må ikke avancere animationen en ekstra gang. Skeletfigurer bruger hver-frame-poseopdatering uden update-rate-optimering; deres tick følger den visuelle komponent. Døde figurer frigøres fra denne tickafhængighed.

## Rytteri, stab og artilleri

Rytteri bruger den samme udjævnede sti og hastighedsintegration. Det har tickafhængighed af bevægelsen, bevarer overlevendes pladser ved tab og figurtæthedsændring samt udjævner retning og terrænhøjde. Gangartens op/ned-bevægelse har ikke længere et brat amplitudespring ved 20 cm/s.

Stabens visuelle pivot arvede HQ-positionen direkte. Pivoten følger nu en udjævnet verdensposition og drejning. Gangamplituden glider ned ved stop. HQ-simulation og følgerregler er bevaret.

Artilleriets komponent havde et tickinterval på 0,25 sekunder. Marchen blev arvet hver frame fra actor, men formationsskift kom i grove trin med fuld instansgenopbygning. Komponenten følger nu bevægelsen hver frame, har en udjævnet tegningsramme og interpolerer kanonernes position, retning og skala. Ændret antal kanoner kræver stadig en ny instansliste.

## Kamera, framegrænse og beslutningstimer

Kameraets realtidsdelta bevares. Der findes ikke en løbende kamerafølgefunktion for kompagnier i `StrategyCameraPawn`; fokus var en engangsteleport. Fokus er nu en glidende overgang til mændenes tegnede centrum, med actorposition som fallback. Tastatur- eller musepanorering afbryder overgangen. Presets foretager ikke længere ekstra positionsteleporter. Projektilfølge bruger en eksponentiel faktor til positionen.

Testscenariet overskriver ikke længere spillerens framegrænse med `t.MaxFPS 60`. Til sammenlignelige målinger kan man angive `-Strategy1864FpsCap=60`, `=120` eller `=144`; `=0` sætter ubegrænset frame rate. `-Strategy1864NoFpsCap` springer scenariets grænsesætning over, ligesom før. Fjernelsen garanterer ikke højere FPS, hvis CPU/GPU er flaskehalsen.

Officerens `ThinkSeconds` og andre beslutningsintervaller kan give nye mål i grove intervaller, men tegner ikke figurerne. Der er ikke fundet en timer, der driver fodfolkets position eller crowd-transformer med lav frekvens. Ordrernes timing er bevaret, mens tegnelaget udjævner nye mål.

## Måling uden video

Brug `-Strategy1864DebugSmooth` på en normal slagtest. Der logges én linje pr. omtrentligt realtidssekund for det første aktiverede kompagni i verdens actoriteration:

```text
Strategy1864DebugSmooth <enheds-id> max-frame-cm centre=... anchor=... men=[...,...,...,...,...] crowd=... max-frame-ms=... dilation=...
```

`centre` er største afstand mellem to på hinanden følgende frames for gennemsnittet af de levende, tegnede mænd. `anchor` måler det udjævnede formationscentrum. De fem `men` følger konkrete komponentidentiteter, også når arrayindekser ændres. Hvis en prøvefigur dør eller fjernes ved kvalitetsskift, vælges en ny uden at tælle afstanden mellem forskellige mænd. Manglende prøvefigurer giver nul. Uden flaget udføres kun flagkontrollen.

Afstandene er i verdenscentimeter og inkluderer højde. Figurernes rødder måles, ikke en animeret hånd eller et våben. I crowd-tilstand bruges de samme rodtransformer, som afleveres til instanserne. Første placering er målingens udgangspunkt. Tab kan flytte det aritmetiske `centre`, selv om ingen overlevende flytter sig; brug også `anchor` og de fem mænd. `max-frame-ms` og `dilation` skelner et langsomt billede eller x10 fra et ekstra hop.

Kontrollér march, vejpunktshjørner, 90°/180°-sving, stop/genstart, formation, figurtæthed, tab og crowd-grænser ved x0,5/x1/x2/x5/x10. Normal afstand pr. billede vokser med fart × tidsfaktor / FPS. Gentag ved samme framegrænse og med `-Strategy1864Crowd=0` for at isolere VAT.

## Kontrol og begrænsninger

`git diff --check` er gennemført. Nye API-kald er kontrolleret i UE 5.8-headerne på `I:/Spil/Epic Games/UE_5.8/Engine/Source`, herunder tickafhængigheder, skelet-poseindstillinger, instanstransformer og vinkelnormalisering. En separat numerisk kontrol bestod 20 kombinationer af 30/60/120/144 FPS og x0,5/x1/x2/x5/x10: konvergens, hastighedsgrænse, korteste vej over ±180°, nul delta og præcis VAT-loopperiode.

Der er ikke bygget, startet spil/editor eller lavet commit. Det numeriske tjek er ikke C++-kompilering eller en måling i renderer. Lead skal bygge og afprøve tickrækkefølge, nær/fjern-skift, x10 og framebudget med mange figurer. Indhentningen giver en kort visuel forsinkelse; kamplogikken bruger fortsat den autoritative simulation.
