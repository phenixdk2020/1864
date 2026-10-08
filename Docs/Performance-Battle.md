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
