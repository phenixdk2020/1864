# Slagydelse – 2026-10-10

Dette afsnit beskriver gpu3-ændringen og erstatter profil-/figurpolitikken i den ældre audit nedenfor. Ingen build, editor, spilstart eller GPU-måling er udført. gpu2 har ingen fungerende Git-reference på den angivne placering; det tidligere forsøg er læst direkte i GameMode, BattleQuality, BattleRenderBudget og dokumentationen. Ingen gammel patch er anvendt.

## Før og efter

| Område | Før i gpu3 | Efter, forventning uden målt resultat |
|---|---|---|
| Måling | Ingen dedikeret GPU-log | `-Strategy1864DebugGpu`: en linje pr. virkelig sekund med frame/game/draw/gpu, figurer, draw calls, primitives og værste frame i intervallet |
| LAV/MIDDEL | UE medium/high-grupper | LAV uden Lumen GI/refleksioner og SSR; MIDDEL med billigere Lumen og skygger |
| HOEJ | Epic, 100 % opløsning | Samme renderer-grupper og lit-røg; figur-/effektbudgetter og culling gælder også her |
| Infanteri | Skeletal til 70 m, retur ved 55 m | VAT ved 40 m, retur ved 32 m; 400 budgetterede skeletal infanterifigurer inkl. faldne, fordelt mellem hele kompagnier |
| Rifler/skygger | Allerede skyggefri rifler/VAT | Bevares. VAT-modellens rifle/bajonet er allerede fusioneret i samme mesh; håndjusterede nær-riflekomponenter bevares inden for budgettet |
| Vegetation | Træer uden cull, buske op til 2.500 m | Træer 800 m, buske 250 m, hegn 300 m; græs fade-start 60 m, slut 120 m; LAV 45 % græstæthed og halvt antal træ-/buskinstanser |
| Røg/blasts | Ubegrænset antal, synkrone spawn-loads | Fælles loft 60 actors, afvisning før normal spawn, unlit-materiale i LAV/MIDDEL og afstandsfade 200–300 m |
| Hak | Animation-load-kald under tick, lazy VAT-bagning, hyppig skylight-capture | Animationer/fademateriale preloads og holdes stærkt; begge rifle-varianter bages ved oprettelse; røg-/ordonnans-assets i CDO; skylight højst hvert 10. virkelige sekund |
| Visuelle scans/buffere | Actor-scans og nye VAT-transform-arrays | Svage enhedsreferencer opdateres hver 0,5 s; aktuelle positioner/status kontrolleres ved brug; VAT-scratch-arrays genbruges |
| HUD | Alle aktive ruter forarbejdes | Ikke-valgte enheders ruter springes over ved off-screen anchor eller >1.500 m; valgte ruter og eksisterende objective-culling bevares |

Kameradistance måles i centimeter; infanteriet bruger som før formationens bounds. Nærbudgettet prioriterer nærmeste kompagnier med en lille bonus til eksisterende skeletal-visning og opdateres hver 0,5 s. Overskydende kompagnier bliver VAT, uden at mænd fjernes. Clips, fase, formationstransforms og pauseposer overføres ved skift, med en frames posehold ved tilbagevenden. Ingen ny materialecrossfade er tilføjet; visuel accept kræver en senere test. Eksisterende specialuniformer/manglende VAT-assets beholder skeletal-fallback og kan overskride budgettet. Kavaleri tælles i målingen, men indgår ikke i infanteriets animationsbudget.

HISM fade-start virker som materialefade, hvis materialet bruger `PerInstanceFadeAmount`; slutafstanden culler stadig. Streamet græs er fortsat ISM og skyggefrit. Vegetationsprofilen gælder ved oprettelse, ikke en regenerering ved profilklik midt i slaget. Gameplayets skov-/terrændata, sigt/røg, skader, ordrer, enhedstal, tidsstyring og kampagnedata er ikke ændret. Ved effektloftet kan visuelle pust/støv/kratermærker udelades; gameplayets SmokeField og ImpactRegistry er separate.

## Omskiftere og logning

Egne CVars registreres i C++ med defaults i `Config/DefaultEngine.ini`. `PROJECT1864-PERF` logger værdier ved start og ændringer (kontrolleret hver 0,5 s), profilens effektive renderer-CVars og HISM-antal/cull. Brug konsollen eller `-ExecCmds="Strategy1864.Perf.Figures 0,..."` til A/B.

| CVar med præfiks `Strategy1864.Perf.` | Default | Virkning |
|---|---|---|
| `Renderer` | 1 | 0 deaktiverer egne renderer-overrides; anvend profil igen |
| `Figures` | 1 | 0 bruger oprindelige property-afstande uden nærbudget |
| `NearCm` / `NearCap` | 4000 / 400 | Afstand/budget; cap 0 er ubegrænset |
| `Vegetation` | 1 | 0 giver oprindelige cull-/tæthedsindstillinger; genskab slagmarken |
| `Effects` / `EffectCap` | 1 / 60 | 0 slår loft/materialevalg/fade fra; cap 0 er ubegrænset; materialevalg ved spawn |
| `Warmup` | 1 | 0 lader VAT bage ved første brug; ændres før figurernes oprettelse |
| `Hitches` | 1 | 0 deaktiverer visuel scan-cache, VAT-buffer-genbrug og skylight-throttle |
| `SkyCaptureSeconds` | 10 | Minimum mellem skylight-captures; første capture sker under Apply |
| `Markers` | 1 | 0 gendanner behandling af alle ikke-valgte ruter |

`-Strategy1864Crowd=0` deaktiverer VAT og omgår nærbudgettet. Eksisterende `-Strategy1864CrowdFar=<cm>` har forrang for afstand. `Figures=0` bruger de eksisterende 7000/5500-property-defaults. Figurdivisoren er fortsat spillerens separate valg. Smoke-asset-preload og animationernes stærke referencer er permanent load-sikkerhed; de visuelle sparepolitikker kan slås fra enkeltvis.

## Kontrollerede UE 5.8-symboler

Alle nedenstående engine-CVars er runtime-variable med `ECVF_Scalability | ECVF_RenderThreadSafe`, uden ReadOnly/Cheat. De sættes med `ECVF_SetByScalability`; højere konsol-/kommandolinje-/ini-prioritet respekteres og effektiv værdi logges. `SetQualityLevels(Quality, true)` er verificeret i `Engine/Public/Scalability.h` og genanvender grupperne ved HOEJ eller deaktivering af overrides.

| CVar | LAV | MIDDEL | HOEJ (UE Epic) | Registrering under Engine/Source/Runtime |
|---|---:|---:|---:|---|
| `r.Lumen.DiffuseIndirect.Allow` | 0 | 1 | 1 | Renderer/Private/Lumen/LumenDiffuseIndirect.cpp |
| `r.Lumen.Reflections.Allow` | 0 | 1 | 1 | Renderer/Private/Lumen/LumenReflections.cpp |
| `r.SSR.Quality` | 0 | 2 | 3 | Renderer/Private/ScreenSpaceRayTracing.cpp |
| `r.Lumen.ScreenProbeGather.DownsampleFactor` | 32 | 32 | 16 | Renderer/Private/Lumen/LumenScreenProbeGather.cpp |
| `r.Lumen.Reflections.DownsampleFactor` | 2 | 2 | 1 | Renderer/Private/Lumen/LumenReflections.cpp |
| `r.Shadow.MaxResolution` | 512 | 1024 | 2048 | Core/Private/HAL/ConsoleManager.cpp |
| `r.Shadow.CSM.MaxCascades` | 1 | 4 | 10 | Renderer/Private/SceneRendering.cpp |
| `r.Shadow.Virtual.ResolutionLodBiasDirectional` | 1 | 0 | -1,5 | Renderer/Private/VirtualShadowMaps/VirtualShadowMapClipmap.cpp |
| `r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving` | 1 | 0 | -1,5 | Samme |

HOEJ-værdier kommer fra `Engine/Config/BaseScalability.ini`; øvrige grupper følger UE medium/high/epic ved 70/85/100 %. CSM/max-resolution gælder traditionelle shadow maps; VSM får egne bias-overrides. Lumen-Allow kan kun aktivere en metode, projektet understøtter. Projektets DynamicGlobalIlluminationMethod, ReflectionMethod, VSM-aktivering, Substrate, distance fields og ray tracing er bevaret; shader-/projektindstillinger ændres ikke ved hvert profilklik. Renderer-CVars er procesglobale og kan fortsætte efter map-travel, som den eksisterende scalability-politik.

Målingen bruger `GGameThreadTime`/`GRenderThreadTime` fra `RenderCore/Public/RenderTimer.h`, `RHIGetGPUFrameCycles(uint32=0)` fra `RHI/Public/DynamicRHI.h` og GPU-indexerede `GNumDrawCallsRHI`/`GNumPrimitivesDrawnRHI` fra `RHI/Public/RHIStats.h`. RHI tilføjes som module dependency. GPU/draw/counters læses på rendertråden og deles atomisk uden flush/wait. De er seneste samples, ikke nødvendigvis samme frame som game/frame. `tris` er RHI's primitiveantal, ikke garanteret kun trekanter. GPU=0 kan være utilgængelig timing. Frame/worst bruger virkelig tid uafhængigt af pause/speed. Figurer inkluderer infanteri/ryttere og faldne, også VAT, ikke kun synlige primitives. Loggen er sekundvis sample plus intervalmaksimum, ikke p95/p99 af alle frames.

Nye komponent-/objektkald er kontrolleret i 5.8-headerne for IConsoleManager, RenderingThread, ConstructorHelpers, UObjectGlobals, WeakObjectPtrTemplates, InstancedStaticMeshComponent, PrimitiveComponent, PlayerController, PlayerCameraManager og SkyLightComponent. Engine-asseten `M_SimpleUnlitTranslucent.uasset` findes med MSM_Unlit, Color og Opacity; HOEJ beholder M_SimpleTranslucent.

## Før/efter-måleplan og usikkerhed

1. Samme build, opløsning, hardware, preset, figurdivisor, enhedstal, kamera og slagtilstand. Baseline: spareomskiftere fra før start/feltgenerering. Optimeret: defaults. Brug DebugGpu i begge runs. Dette dokument giver ikke tilladelse til at starte spillet.
2. Sammenlign første 15 s særskilt fra 60 s opvarmet tomgang, march, salver, artilleri og et længere slag med faldne. Gå begge veje gennem 32–40 m og mellem kompagnier nær budgetgrænsen. Kontroller fase, placering, rifler/bajonetter, specialuniform-fallback og pause/speed. Sammenlign GPU-ms, draw calls, primitives og worst frame, ikke blot GPU-procent/fps.
3. Brug senere stat unit, stat gpu og ProfileGPU til at isolere Lumen, VSM/shadow depths, translucency, vegetation og skylight. Hold fps-cap ens; en separat uncapped sammenligning kan vise skjulte gevinster. Test LAV → MIDDEL → HOEJ → MIDDEL og Renderer=0 efter LAV.
4. Firesekundershakket er ikke reproduceret her. Første-volley-loads, lazy VAT-bagning og hyppig skylight-capture er konkrete fundne kilder; rettelserne beviser ikke, at netop dette hak er væk. VAT-bagning er flyttet til oprettelsen og kan forlænge indlæsning; den er ikke asynkron. Shaders/PSO, terrængenerering og GPU-resource-upload kan stadig give opstartsspids.
5. Artilleri-/kavaleri-/HQ-/flag-loads ligger fortsat i initialization/setup, ikke normal frame-update. Gameplayets AI-/kontakt-/kamp-scans er ikke tidsudtyndet, da det ændrer reaktionstid. Øvrige HUD-scans er bevaret for at holde diffen lille. Skylight-refleksion kan være op til intervallet forsinket ved hurtig spiltid; direkte sol/fog/eksponering opdateres som før.

Statisk validering: ny kode og diff gennemlæst, relevante engine-header-/CVar-definitioner kontrolleret, git diff --check. Ingen kompilering, visuel accepttest, målt forbedring eller commit er påstået.

---

# GPU-ydelse i 3D-slag – 2026-10-08

Audit af `Source/Strategy1864`, konfigurationen og de kodegenererede omgivelser for `Strategy1864_Skirmish` og kampagnens rigtige slag. Spillet er hverken bygget eller startet. Ingen GPU-tider er målt; 90 % belastning ved en 60 fps-grænse identificerer ikke i sig selv flaskehalsen. Budgettet er 16,67 ms pr. frame.

## Fund

- **Infanteri:** nærfigurer er individuelle `USkeletalMeshComponent` med egne animationspositioner; rifler er individuelle statiske komponenter. På afstand bruges allerede to ISM-komponenter (levende/faldne) med VAT, reduceret mesh-LOD og instansdata for klip/tid. Materialer deles via crowd-modellen pr. materialgruppe, ikke én MID pr. fjernfigur. Det er ISM, ikke HISM, og flere materialgrupper betyder stadig flere draw calls. Skift ved 7.000 cm, tilbage ved 5.500 cm, målt fra kameraet til formationens bounds. Manglende crowd-assets/klip kan give fallback til individuelle skeletal meshes: kontroller `PROJECT1864-CROWD` i loggen.
- **Skygger:** nærinfanteriet kastede skygge, også riflerne; VAT-figurerne kastede også skygge med vertexanimation. Projektet aktiverer VSM (`r.Shadow.Virtual.Enable=1`), så bevægelige figurer/VAT og solens bevægelse kan koste shadow-depth arbejde/cache-invalidering. Atmosfærens 60.000 cm/4 cascades er CSM-indstillinger og er ikke en afstandsbegrænsning af VSM.
- **Kavaleri:** hver hest er en statisk mesh-komponent, hver rytter en skeletal komponent med holdt siddepose; sabel er en separat statisk komponent. Ingen ISM/HISM i dette system. `MenPerHorseman=2`; de tre komponenttyper har ikke en eksplicit afstandscull/skyggepolitik. Individuelle heste bevæges hver frame, og faldne bliver liggende. En instanseringsomlægning kræver håndtering af faldne, sadelplacering og sabeltilknytning; ikke ændret i denne sikre optimering.
- **Custom depth/materialer:** de gennemgåede infanteri-/kavaleriskabere aktiverer ikke custom depth og opretter ikke en MID pr. levende soldat. Materialsektioner, geometri-LOD, Nanite-flags og eventuelle asset-overrides skal verificeres i editoren.
- **Lumen/Nanite:** `DefaultEngine.ini` bruger Lumen GI og refleksioner (begge method=1), mesh distance fields og VSM; hardware ray tracing er fra. Nanite er ikke eksplicit konfigureret her; det er ikke bevis for, at alle battle-assets bruger Nanite. Kampkortene får battle game mode via `Strategy1864_`-præfikset. Renderer-defaults er projektfælles; de er bevaret for også at beskytte kampagnens udseende.
- **Post process:** battle-atmosfæren har unbound grading, bloom 0,35, vignette 0,35 og AO-intensitet 0,6, uden film grain/chromatic aberration. SSR/AO-passens faktiske pris afhænger af aktive scalability-grupper, Lumen og map-volumes. Før ændringen var AA/renderopløsning ikke eksplicit angivet i denne konfiguration.
- **Atmosfære:** height fog, sky atmosphere og en movable sol; volumetrisk fog er eksplicit fra. Skylight capture er allerede ikke realtime, men `UpdateForHour` recapturer ved ændringer på mindst 0,02 spiltime. Det kan stadig give spidser med hurtig spiltid. Ingen volumetric-cloud-oprettelse fundet i det gennemgåede battle-atmosfæresystem; map-assets kan indeholde egne actors.
- **Terræn/vegetation:** `StrategyCampaignBattlefield` bruger genereret terræn og HISM for landskabsdele. `AddInstanced` sætter cull-distance kun hvis argumentet er positivt; dele kan derfor være uden afstandscull. Kameraets streamede græs bruger ISM, radius-cull og ingen skygger. Træers alpha cards kan koste overdraw. Standard `sg.FoliageQuality` er ikke garanti for lavere tæthed i denne egen generator.
- **Røg/effekter:** ét mundingspust kan oprettes pr. visuelt skydende soldat; tidligere fire transparente sfærer og fire MID pr. pust. Antallet følger divisor, men der er ingen samlet global grænse. BattleBlast bruger flere mesh-stykker og korte lysglimt uden skygge; støv/røg og langlivede kratermærker kan akkumulere. Gameplayets smoke field er et separat system.
- **HUD:** Canvas-paneler, tekst, knapper, minimap og world-markeringer tegnes hver frame. Transparente paneler og mange overlappende markeringer kan koste overdraw, men om dette dominerer over 3D kræver en GPU-capture. Ingen visuel HUD-forenkling uden måling.

## Ændringer

| HUD-valg | UE-grupper | Renderopløsning | Infanteri: mænd pr. figur | Sfærer pr. mundingspust |
|---|---|---|---|---|
| LAV | Medium (1) | 70 % | 5 | 2 |
| MIDDEL (standard) | High (2) | 85 % | 2 | 3 |
| HØJ | Epic (3) | 100 % | 1 | 4 |

`Scalability::SetQualityLevels` anvender view distance, AA, shadows, GI, reflections, post process, textures, effects, foliage og shading. TSR er eksplicit valgt i renderer-konfigurationen. Opløsningsprocenten gælder 3D-rendering; HUD tegnes ved viewportens opløsning. Engine-/konsol-overrides med højere prioritet kan tilsidesætte værdierne og skal kontrolleres ved måling.

Battle game mode anvender den gemte profil ved start. Kun profilindekset gemmes under `/Script/Strategy1864.Settings`, `BattleQuality` i GameUserSettings; kampagnens game mode anvender ikke denne politik. Kvalitets-CVars er dog procesglobale og kan fortsætte efter map-travel. Profilknapperne ændrer eksisterende infanteri og gemmer profilen. Den eksisterende figur-række kan derefter ændre divisoren for det aktuelle slag (profilmarkeringen slukkes); dette særvalg gemmes ikke. `-Strategy1864FieldLOD=N` har forrang ved start, også efter scenariereset, mens et HUD-profilklik ændrer det aktuelle slag. Skirmish bruger nu samme default/persistens som det store slag. Kavaleri forbliver én figur pr. to mænd.

Fjerne VAT-figurer kaster ikke længere skygge; nærfigurerne gør stadig. Rifler kaster ikke selvstændige skygger. Mundingspust har 30.000 cm render-cull. Enheder fjernes ikke ved stor kameraafstand, så formationer fortsat kan aflæses; der er heller ikke tvunget skeletal LOD uden viden om asset-LOD'erne. Røg-cull sparer rendering, ikke oprettelse/tick. Eksisterende og nye pust kan have forskelligt sfæreantal umiddelbart efter et profilskift.

## Måleplan (manuelt – ikke udført)

1. Brug både `Strategy1864_Skirmish` og det rigtige slag med samme styrker, opløsning, kamera og spiltid. Registrer GPU-model, RHI, viewportstørrelse og aktiv profil. Mål tæt på, ved crowd-overgangen og i fugleperspektiv; både tomgang, march, salve og artilleri samt et længere slag med faldne.
2. `stat unit`: registrer Frame, Game, Draw og GPU i ms. 60 fps-cap kan skjule gevinsten i fps; se GPU-ms og mål eventuelt midlertidigt med `t.MaxFPS 0`, derefter tilbage til 60. Undgå at konkludere fra GPU-procent alene.
3. `stat gpu`: sammenlign Shadow Depths/VSM, BasePass, Lumen GI/reflections, AO, translucency, TSR, fog/atmosphere og post processing for hver profil. Varm shaders/caches op først og sammenlign flere ens captures.
4. `ProfileGPU`: gem captures fra samme kamerastand og salve. Find om VSM-invalidering, materialsektioner, røg-overdraw, vegetation eller TSR dominerer. Kontroller aktiv `r.ScreenPercentage`, `r.AntiAliasingMethod`, `sg.*`, og om crowd-fallback forekommer. Brug GPU-visualisering af overdraw og Nanite/VSM i editoren ved behov.
5. Visuel accept: formationer, uniformer, skygger tæt på, rifle/bajonet, røg, trækroner og HUD-tekst skal være læselige. Test alle tre knapper, særskilt figurvalg, genstart/reset, kommandolinje-override og overgang begge veje mellem skeletal/VAT. Kontroller map-volumes, foliage/landscape-LOD og cloud-actors direkte i de binære maps før yderligere ændringer.

Validering i denne ændring: statisk gennemgang og `git diff --check`; ingen build, spilstart eller påstået målt forbedring.
