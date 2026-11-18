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

Hestens hvile-baekkenhoejde er 105,8 cm (A_Horse_Stand 105,5-106,0); Idle2 ligger 104,3-104,9 cm.

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
Et ark pr. klip: `Preview/<klip>_sheet.png` (menneske) og `Preview/<klip>_side_sheet.png` (hest; turn-klip ogsaa `_top_sheet.png`). 6 frames, loops jaevnt fordelt over cyklussen. Mesh uden hesteens/soldatens endelige materiale i UE.

## Kendte begraensninger
- **A_Horse_Run:** brugbar som en hurtig kantergang/let galop (ca. 487 cm/s stance-hov-hastighed), men IKKE en erstatning for A_Horse_Gallop (757 cm/s); sammenlign i spillet foer brug.
- **Mesh-artefakter ved bagbenet:** i klip hvor venstre bagben staar anderledes end i hvile ses moerke planker bag hasen og en graa spids ved hoven (Eat, Turn, Run, Death). Det er skinning/hale-mesh i Horse_1_rigged_v3 og ses ogsaa i de eksisterende klip (A_Horse_Gallop, kontrolleret i Blender); retargetingen er ikke aarsagen.
- Hestens hale er stille (foelger kun baekkenet): enhver ekstra hale-drejning (selv 25 %) aabner halemeshets skjulte flader til sorte plader/en graa stang ved bagbenet. Kilden svinger halen; det er udeladt.
- Turn-klippene drejer ikke hesten (kilden er ogsaa paa stedet).
- Haender/fingre: kun tommel og pegefinger; haandpositioner kan afvige nogle cm fra kilden (kortere arme). Ingen rekvisitter (rat, stol, skubbet genstand).
- Death_B: toeknoglen ligger kortvarigt ca. 3 cm under nulniveau; Death_C har en luftfase.