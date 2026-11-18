# Mesh2Motion-klip paa vores skeletter (retarget i Blender)

Genereret af `Tools/Animation/build_mesh2motion_clips.py` (genkoerbar; tidligere filer gemmes som `.prev`). **Ikke importeret i Unreal** - det goer unreal-integration.

- **Kilde og licens:** Mesh2Motion, CC0 (`D:/pinokio/api/mesh2motion/app/LICENSE-CC0.MD`): `static/animations/horse-animations.glb`, `human-base-animations.glb`, `human-addon-animations.glb`. Kildefilerne er kun laest.
- **Maal:** hest = `SourceAssets/Horse/Horse_1_rigged_v3.fbx` (SK_Horse_1864, 36 deform-knogler som de eksisterende A_Horse_*-klip); menneske = `SourceAssets/Units1864/DK_Infantry_1864.fbx` (SK_Human_1864, 41 Mixamo-knogler, uaendret).
- **30 fps.** Menneskeklippene er resamplet fra 24 fps; hesteklippene var 30 fps. Loops har **ingen dublet-slutframe** (sidste frame foerer direkte over i foerste).
- **Alt paa stedet:** ingen rodbevaegelse (hestens `root` / soldatens `Hips` flytter sig ikke i xy ud over klippets egen kropsbevaegelse).
- **Akser/import:** hest = samme eksport som horse_gaits_blender.py (axis_forward -Z, axis_up Y, kun deform-knogler) -> **roll 0** ved import (HORSE_CLIP_ROLL). Menneske = samme eksport som A_Gun_*/A_Officer_* (armatur + mesh i FBX) -> samme import som de klip (roll 90, ingen ekstra hofte-tillaeg: Hips er loeftet 98,079 cm, saa toeknoglen staar paa 0; mesh-sulen ligger da ca. 1,9 cm under 0).
- **Ikke relevant her:** ingen klip holder en riffel (skulder-gevaer-reglen er ikke i spil); Walk_Carry har tomme baere-haender.

## Metode
- Hest: kroppen (baekken, rygsoejle, hals, hoved, ore, kaebe, skuldre) som verdens-rotationsdelta fra Mesh2Motion-hesten (kaede-broeker for rygsoejle/hale); halen foelger kun baekkenet (stille). Benene: IK (som horse_gaits_blender.py) mod kildens fetlock-forskydning fra hvile x 0,81 (hoejdeforhold), saa hovene staar paa jorden. Baekkenet = hvile + 0,81 x kildens forskydning.
- Menneske: T-pose -> A-pose, hver knogles retning/vridning overfoeres (verdens-orientering + fast hvile-forskel), ben via 2-knogle-IK mod ankel-forskydning fra hvile x 0,937 (benlaengde-forhold), hofte = hvile + 0,937 x kildens forskydning. Kun tommel og pegefinger findes paa soldaten.
- Lodret kontakt-korrektion (kun naar forskellen er over 0,6 cm): mesh-bunden holdes paa samme niveau som kildens mesh-bund (doede/liggende figurer ligger paa jorden). Hest: kun Death.

## Hest (SourceAssets/Mesh2Motion/Horse)
| Klip | Kilde-klip | Frames | Laengde (n-1)/30 | Cyklus n/30 | Loop | Baekkenhoejde cm | Rodbevaegelse cm | Bemaerkning |
|---|---|---|---|---|---|---|---|---|
| A_Horse_Idle2 | Idle | 79 | 2.600 s | 2.633 s | ja | 104.3-104.9 | [0.0, 0.0, 0.0] | staar; hov-glid max 0.21 cm |
| A_Horse_Eat | Eating | 79 | 2.600 s | 2.633 s | ja | 102.6-103.8 | [0.0, 0.0, 0.0] | graesning, hovene stille; mulen gaar ca. 5 cm ned i jorden (graes) |
| A_Horse_TurnLeft | Turn_Left | 139 | 4.600 s | 4.633 s | ja | 98.2-102.1 | [0.0, 0.0, 0.0] | trin paa stedet (hoven drejer rundt i kontakt, glid op til 5.43 cm); selve kursaendringen goer aktoeren |
| A_Horse_TurnRight | Turn_Right | 139 | 4.600 s | 4.633 s | ja | 98.2-102.1 | [0.0, 0.0, 0.0] | som TurnLeft (glid op til 5.81 cm) |
| A_Horse_Death | Death | 42 | 1.367 s | 1.400 s | nej | 50.3-105.1 | [0.0, 0.0, 0.0] | staar, falder om paa siden og ligger (ingen loop); baekkenet ender ca. 50.3 cm over jorden |
| A_Horse_Run | Run | 12 | 0.367 s | 0.400 s | ja | 108.0-114.2 | [0.0, 0.0, 0.0] | kort galop/rask galop-cyklus; stance-hov-hastighed (= rodhastighed hvis aktoeren flyttes) 486.6 cm/s, mellem A_Horse_Canter 357 og A_Horse_Gallop 757; bagbenet naar 99 % af fuld laengde |
| A_Horse_M2M_Walk | Walk | 28 | 0.900 s | 0.933 s | ja | 103.6-106.5 | [0.0, 0.0, 0.0] | GANG, stance-laas (se Gangarter); stance-hov-hastighed ca. 132 cm/s (per hov 120-140); hals/hoved fra kilden |
| A_Horse_M2M_Trot | Trot | 20 | 0.633 s | 0.667 s | ja | 98.7-105.7 | [0.0, 0.0, 0.0] | GANG, stance-laas + loop-lukning (se Gangarter); stance-hov-hastighed ca. 250 cm/s (bagben 242-275); hals/hoved-nik fra kilden |
| A_Horse_M2M_Rear | Rear | 88 | 2.900 s | 2.933 s | nej | 91.0-105.8 | [0.0, 0.0, 0.0] | stejler og ned igen (ingen loop); foerste og sidste frame er staaende hvile; bagbenet glider op til 15.8 cm i de foerste frames (kilden skifter fod) |
| A_Horse_M2M_Kick | Kick | 39 | 1.267 s | 1.300 s | nej | 100.1-125.6 | [0.0, 0.0, 0.0] | baglens spark (ingen loop); baekkenet loefter sig til 125.6 cm; slutter i staaende hvile |

Hestens hvile-baekkenhoejde er 105,8 cm (A_Horse_Stand 105,5-106,0); Idle2 ligger 104,3-104,9 cm.

## Gangarter (Horse/A_Horse_M2M_Walk og A_Horse_M2M_Trot)
Maalt med `Tools/Animation/analyze_horse_gait.py` paa de eksporterede FBX'er (30 fps). Hastighed = stance-hovens hastighed (lineaer tilpasning mens hoven er nede) = den fart aktoeren skal flyttes med ved afspilningsrate 1,0, saa hovene ikke glider. **Afspilningsrate = aktoerens fart / stance-hastighed.**

| | A_Horse_Walk (eget) | **A_Horse_M2M_Walk** | A_Horse_Trot (eget) | **A_Horse_M2M_Trot** |
|---|---|---|---|---|
| Frames / cyklus | 33 / 1.100 s | **28 / 0.933 s** | 21 / 0.700 s | **20 / 0.667 s** |
| Loop | ja, ingen dublet | ja, ingen dublet (sutur 21 mod 18 normalt frame-skridt) | ja, ingen dublet | ja, ingen dublet (sutur 13 mod 24) |
| Rodbevaegelse | 0 (paa stedet) | 0 | 0 | 0 |
| Stance-hov-hastighed cm/s | 60 (rapport; egen maaling 18-62 pr. hov) | **132** (hovene 120-140; kilde-stance-lock 133,7) | 178,6 (176-179) | **ca. 250** (bagben 242-275, laas-maal 241,7, measure_horse 257) |
| Distance pr. cyklus (fart x cyklus) | 66 cm | 123 cm | 125 cm | 167 cm |
| Hovens fremad-udsving (skridtlaengde) | 45-47 cm | 61-77 cm | 60-63 cm | 62-71 cm |
| Bagerste bagfod bag baekkenet | 26 cm | 5-12 cm | 34 cm | 9-22 cm |
| Baekkenhoejde cm (hvile 105,8) | 104,8-106,8 (udsving 2,0) | 103,6-106,5 (3,0); foerste frame 105,8 | 102,5-109,1 (6,6) | 98,7-105,7 (7,0) |
| Hals/hoved (graders udsving) | hals-haeldning 0, hoved-yaw 7,6 | hals-haeldning 0, hoved-haeldning 2,2, hoved-yaw 7,3, rygsoejle-yaw 5,6 | hals-haeldning 0, hoved-yaw 7,4 | **hals-haeldning 8,7, hoved-haeldning 17,4**, hoved-yaw 2,1 |
| Hov-glid i stance (afvigelse fra ret linje) | ikke maalt paalideligt | 0,1-5,4 cm | 0,1-1,9 cm | 0,0-2,8 cm |

- **Stance-laas (Walk, Trot):** kildens hove glider ikke med samme fart (hand-animeret): Walk 92-205 cm/s, Trot front ca. 150-210 og bag 345-373 cm/s (i kildens egen skala x 0,81). Retargetet tvinger derfor hver fetlock, mens den er nede, til en ret linje med faelles jordhastighed (middel af kildens egne, centreret om kildens stance-midte) og laeser z paa stance-medianen; udenfor stance interpoleres rettelsen. Uden laas (`LOCK=0`) er Walk-hovene 116-175 og Trot-hovene 209-392 cm/s, dvs. +-30-50 % glid ved en enkelt afspilningsrate. Kroppen (baekken, hals, hoved, rygsoejle) er uroert af laasen.
- **Loop-lukning (Trot):** kildens sidste tast er ikke en dublet af den foerste (pose-afstand 0,5 mod ca. 0,3 pr. frame-skridt); rettelsen fordeles jaevnt over de 20 frames (`CLOSE=0` slaar fra). Walk bruger kildens 28 foerste taster uden rettelse.
- **Hals/hoved:** retargetet bevarer kildens bevaegelse 1:1 (kilde Trot: hals 9,2 / hoved 17,7 grader; Walk: hoved 2,2). Kildens Walk har selv naesten ingen hals-nik; kun Trot giver tydelig sekundaer bevaegelse.
- **Mesh-fejl (ikke animationen):** de moerke planker og den graa stang ved venstre bagben er haelen/halemeshets egne polygoner (4734 vertices, kun vaegtet paa tail_01-03; forsvinder naar de skjules), som bliver synlige naar bagbenet svinger. Ses ogsaa i de egne A_Horse_Walk/Trot (`Preview/ref_*`) og er mere synlig i M2M-klippene fordi bagbenene svinger mere. Skal rettes i meshet (model3d/rigging), ikke i klippene.
- **Rear:** bagbenet glider op til 15,8 cm i de foerste 14 frames (kilden skifter fod mens hoven er nede); hals/hoved loefter sig med. **Kick:** baekkenet hopper 20 cm, bagbenene sparker op til ca. 90 cm.
- Egne sammenligningsklip ligger i `Reference/Animations/Horse/` (ikke SourceAssets/Horse). Kontrolark af dem: `Preview/ref_A_Horse_Walk_side_sheet.png`, `ref_A_Horse_Trot_side_sheet.png` (frame-valg bestemt af `Tools/Animation/preview_horse_clip.py`).

## Menneske (SourceAssets/Mesh2Motion/Human)
| Klip | Kilde-klip | Frames | Laengde (n-1)/30 | Cyklus n/30 | Loop | Hoftehoejde cm min-max (foerste frame) | Hofte x/y foerste til sidste frame cm | Stance-fod | Bemaerkning |
|---|---|---|---|---|---|---|---|---|---|
| A_M2M_Push | Push | 100 | 3.300 s | 3.333 s | ja | 71.9-76.4 (76.4) | -1.83 / 0.77 | median 16.6 cm/s, glid 1.22 cm | skubber, laenet frem; ingen genstand |
| A_M2M_Walk_Carry | Walk_Carry | 60 | 1.967 s | 2.000 s | ja | 81.5-91.1 (82.7) | -0.85 / 0.0 | median 46.0 cm/s, glid 2.26 cm | GANG: langsom baerende gang, arme foran kroppen (tomme) |
| A_M2M_Driving | Driving | 63 | 2.067 s | 2.100 s | ja | 59.1-59.1 (59.1) | 0.0 / 0.0 | median 0.0 cm/s, glid 0.0 cm | siddende, haender paa et rat (ingen rat/stol i klippet) |
| A_M2M_Sitting_Idle | Sitting_Idle | 63 | 2.067 s | 2.100 s | ja | 59.3-59.3 (59.3) | 0.0 / 0.0 | median 0.0 cm/s, glid 0.0 cm | siddende paa usynlig stol |
| A_M2M_Fixing_Kneeling | Fixing_Kneeling | 195 | 6.467 s | 6.500 s | ja | 45.9-90.7 (90.7) | -0.28 / -5.94 | median 0.0 cm/s, glid 0.31 cm | staar, knaeler og arbejder (156 kilde-frames) |
| A_M2M_Kneeling_Tired | Kneeling Tired | 24 | 0.767 s | 0.800 s | ja | 47.6-47.7 (47.6) | 0.0 / -0.0 | median 0.0 cm/s, glid 0.0 cm | knaelende traet, kort loop |
| A_M2M_Crouch_Idle | Crouch_Idle | 110 | 3.633 s | 3.667 s | ja | 52.5-53.2 (52.5) | 0.2 / 0.06 | median 0.2 cm/s, glid 1.05 cm | hug |
| A_M2M_Death_A | Death_A | 135 | 4.467 s | 4.500 s | nej | 24.6-92.6 (86.8) | 0.49 / -65.25 | median 0.6 cm/s, glid 0.75 cm | staar, falder paa knae, forover paa jorden |
| A_M2M_Death_B | Death_B | 56 | 1.833 s | 1.867 s | nej | 29.6-105.8 (87.4) | -6.99 / 16.06 | median 56.3 cm/s, glid 0.51 cm | hop og fald forover, ligger paa siden |
| A_M2M_Death_C | Death_C | 66 | 2.167 s | 2.200 s | nej | 15.6-98.7 (92.9) | -0.89 / -3.79 | - | baglaens fald med rulle |
| A_M2M_Death_D | Death_D | 66 | 2.167 s | 2.200 s | nej | 12.4-90.7 (90.7) | 0.59 / 43.14 | median 55.8 cm/s, glid 0.62 cm | saenker sig, falder bagover og ligger |
| A_M2M_Idle_Sword | Idle_Sword | 63 | 2.067 s | 2.100 s | ja | 76.8-79.3 (76.8) | -2.09 / -0.05 | median 0.1 cm/s, glid 0.27 cm | staar parat (ingen vaaben i klippet) |

Death_A og Death_D flytter hoften hhv. ca. 65 og 43 cm i xy (selve faldet); alle andre klip bliver paa stedet.

Hoftehoejde = Hips-knoglens hoejde over toeknoglens nulniveau efter eksport (staaende hvile ca. 94 cm som A_Officer_Idle). De lave vaerdier (siddende 59, knaelende 46-48, hug 53) er klippenes egen kropsholdning; Death/Fixing starter staaende (87-93 cm) og ender liggende/knaelende.

**Gangklip (hastighed):** kun `A_M2M_Walk_Carry`. Rodhastighed (= stance-fodens hastighed, saa foedderne ikke glider hvis aktoeren flyttes med denne fart): **46.0 cm/s**, laengde 1.967 s (60 frames, cyklus 2.000 s), rodbevaegelse i klippet 0 (paa stedet), fodglid maks 2.26 cm. Til sammenligning gaar infanteriets march-klip 137,7 cm/s.

## Kontrolark (Preview/)
Et ark pr. klip: `Preview/<klip>_sheet.png` (menneske) og `Preview/<klip>_side_sheet.png` (hest; turn-klip ogsaa `_top_sheet.png`; gang/rear/kick ogsaa `_front_sheet.png` forfra). 6 frames, loops jaevnt fordelt over cyklussen. Mesh uden hesteens/soldatens endelige materiale i UE.

## Kendte begraensninger
- **A_Horse_Run:** brugbar som en hurtig kantergang/let galop (ca. 487 cm/s stance-hov-hastighed), men IKKE en erstatning for A_Horse_Gallop (757 cm/s); sammenlign i spillet foer brug.
- **Mesh-artefakter ved bagbenet:** i klip hvor venstre bagben staar anderledes end i hvile ses moerke planker bag hasen og en graa spids ved hoven (Eat, Turn, Run, Death). Det er skinning/hale-mesh i Horse_1_rigged_v3 og ses ogsaa i de eksisterende klip (A_Horse_Gallop, kontrolleret i Blender); retargetingen er ikke aarsagen.
- Hestens hale er stille (foelger kun baekkenet): enhver ekstra hale-drejning (selv 25 %) aabner halemeshets skjulte flader til sorte plader/en graa stang ved bagbenet. Kilden svinger halen; det er udeladt.
- Turn-klippene drejer ikke hesten (kilden er ogsaa paa stedet).
- Haender/fingre: kun tommel og pegefinger; haandpositioner kan afvige nogle cm fra kilden (kortere arme). Ingen rekvisitter (rat, stol, skubbet genstand).
- Death_B: toeknoglen ligger kortvarigt ca. 3 cm under nulniveau; Death_C har en luftfase.