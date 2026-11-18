# Rytterfigurer v1 - bygget af Tools/Animation/build_cavalry_rig_1864.py

Blender 5.2 headless (CPU-render til kontrolark). Ikke importeret i Unreal (hand over til unreal-integration). Kilder (uændret), `R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\`:
`Swedish Cavalry\SE_Husar_1864_Apose_textured`, `Prussisk Cavalry\PR_Husar_1864_Apose_textured`, `PR_Dragon_1864_Apose_textured`, `Swedish Cavalry\SE_Dragon_1864_Apose_textured`, `Danish Cavalry\DK_Dragon_1864_textured`. Meshet læses fra .glb ved siden af FBX'en (samme 40k-mesh; DK_Dragon-FBX'ens tekstur kan Blender ikke læse - størrelse 0 - og FBX'en indeholder kamera og lys; glb'en har teksturen). Alle fem står i A-pose (DK_Dragon: armene ca. 30 grader ned, ok som de andre).

## Filer
- `SK_Cavalry_1864_{SE_Husar,PR_Husar,PR_Dragon,SE_Dragon,DK_Dragon}.fbx` + `_Texture_0.png` (2048 x 2048) ved siden af, som artilleristerne og officeren.
- `report.json` (mål pr. figur, knoglekontrol, kantstrækning i fire rytterposer, håndmål; `soldier_baseline` = soldatens eget mesh i de samme klip).
- `Preview/Cavalry_{NAVN}_sheet.png`: kolonner = A-pose, A_Combat_CavWalk fr. 14, CavGallop fr. 8, CavCharge fr. 8, CavSabreDraw fr. 10; øverste række forfra, nederste fra siden. `Preview/Cavalry_{NAVN}_zoom.png`: A-pose (hoved/overkrop), Gallop 8 forfra og fra siden (hoved, hænder), Gallop 8 og Charge 8 sædet fra siden, SabreDraw 10 (højre hånd).
- Genkør: `blender -b -P Tools/Animation/build_cavalry_rig_1864.py -- all [SE_Husar PR_Husar PR_Dragon SE_Dragon DK_Dragon]` (`build` / `preview`). Eksisterende filer gemmes som `*.prev.*`. Env: UNPOSE=0 (ingen af-posering), BRIDGE, ISLAND, HAND_MODE, SMOOTH, BASELINE=1, DIAG=n.

## Skala og højde (afvigelse fra opgaveteksten)
Opgaven bad om ca. 180 cm og fødder på 0. Figurerne er i stedet bygget som artilleristerne/officeren: **ca. 199 cm, sål ved -100 cm** i bindeposen (SK_Human-armaturet har origo ved hoften, og knoglerne ligger fast i den højde; et 180 cm mesh ville ikke passe til knoglerne). Skalaen er 1,000 (kun translation, maks. 0,6 cm); højde 199,1-199,4 cm = soldatens 199,1. Rytteren får samme størrelse som fodfolket; det er dér, in-game-skalaen (mesh-node-skala) skal justeres, ikke i filerne.

## Skelet
Soldatens armatur uændret (`DK_Infantry_1864.fbx`): 41 Mixamo-knogler, navne/forældre/bindepose (afvigelse 9e-7 efter re-import), skala 0,01, ingen forskydning; mesh er barn af armaturet. Eksport -Z/Y, ingen leaf-bones, ingen animation. Samme skelet som A_Gun_*, A_Officer_* og A_Combat_Cav*.

## Vægte og rettelser (ud over artillerimetoden)
Som artilleristerne: nearest-surface-transfer fra soldatens mesh efter arm-tilpasning, kantudglatning, håndled efter tekstur, hænder = kun `Hand` (vanter, ingen fingerknogler), maks. 4 pr. vertex (kontrolleret 0 vertices med sum != 1).
Nyt for rytterne:
1. **Bro-flader fjernet.** Figurerne er ét sammensvejset net. Hænder/albuer hviler mod lår, pung og bælte og er svejset fast; i rytterposerne rives de (kanter 25-50 cm for lange, skarpe "pigge"). Før udglatning fjernes flader mellem en underarms-/håndvertex (Hand+ForeArm > 0,5) og en vertex med arm-andel < 0,3; smalle øer under 150 flader droppes. Det giver små mesh-huller ved kontaktfladen: SE_Husar 213 flader, DK_Dragon 67, de øvrige 0 (se trekanter nedenfor).
2. **Af-posering af armene.** Armene i modellernes A-pose lå op til 14 cm ved siden af soldatens Hand-knogle (sablen sættes på den). Modellen flyttes nu (vægtet invers af tilpasningsposen) så arme/hænder ligger på soldatens hvilearme. Største vertex-flytning: SE_Husar 12,0 cm, SE_Dragon 12,6, DK_Dragon 7,7, PR_Dragon 5,2, PR_Husar 2,3. Mesh-armbredde er derefter soldatens (+-0,47 m).
3. Rytterklippenes bindepose er sædet, ikke soldatens. Kontrolarket bruger klippets absolutte lokale transform (som Unreal) udtrykt i modellens bindepose (artilleriscriptet brugte delta mod klippets egen bindepose, som kun er rigtigt for klip med soldatens bindepose). Rod/hofte: klippets absolutte hofteposition.

## Kontrol (tal fra report.json)
- Hat/hoved: vertices over 0,80 m (over hagen): 100 % har Head/HeadTop_End-vægt > 95 % i alle fem; i Gallop 8 afviger de højst 0,1-0,2 cm fra en stiv Head-bevægelse (hue, kolpak, fjerbusk, kartusche, hestehaleplym følger hovedet). Skæg/overskæg er tekstur, ikke geometri.
- Højre hånd (vertex-tyngdepunkt for vægt > 0,5 mod RightHand-knogle, bone-rum, cm; soldatens egen hånd = (0,6 ; 8,2 ; 0,8)): SE_Husar (1,7 ; 10,3 ; 3,7), PR_Husar (2,3 ; 9,2 ; -0,5), PR_Dragon (2,8 ; 8,1 ; 1,4), SE_Dragon (-2,0 ; 8,1 ; 6,3), DK_Dragon (2,4 ; 10,2 ; 5,8). Dvs. 2-6 cm fra soldatens håndposition; størst for SE_Dragon og DK_Dragon (bone-z 6 cm). Sabelsocketten (officerens: Location 8,86 / -10,98 / 4,26 på RightHand) kan derfor sidde op til ca. 5 cm fra knytnæven; skal kontrolleres in-game.
- Deformation i rytterposer (Gallop 8, 99,99 %-percentil af kantstrækning; soldatens eget mesh i samme klip: 12,2): SE_Husar 11,6, PR_Husar 8,7, PR_Dragon 14,4, SE_Dragon 8,7, DK_Dragon 10,8. Største enkelt kantforlængelse 11-19 cm (soldatens 23). Altså på niveau med soldatmeshet i samme klip.

## Svagheder / ikke testet
- Rytterklippene er sæde-klip til hesten: uden hest ser figuren ud som en dyb hugsiddende stilling (hofte 48,7 cm over sædet). Ikke kontrolleret mod hestemeshet/sadlen (ingen hest i arkene); ben/lår kan gennembryde sadel/hest - kontrolleres i spillet.
- SE_Husar: små flige ved armhulerne i hvile efter af-posering (A-pose-arm mod krop), guldsnore/tressen smøres noget i håndled og albue; hængende snor/kvast fra kolbak (SE_Husar) følger Head-knoglen uden sekundær bevægelse.
- Ansigtsteksturen er plettet/rød i kilden (især DK_Dragon, SE_Husar).
- Fingre ikke animerbare (vanter), manchetter smøres ved kraftig håndledsbøjning (SE_Dragon guldmanchetter).
- Mesh-huller: bro-fladerne er fjernet i stedet for at lappe nettet; set udefra ikke synligt i kontrolarkene, men bagsider ikke særskilt undersøgt.
- Trekanter: 39.735-40.000 (ikke præcis 40.000 pga. bro-flader/øer).
- Kontrolarket bruger pose-overførsel af lokale rotationer, ikke Unreals retargeting; ingen Unreal-import eller afspilning.

## Ikke brugt
- **Rider_2_textured_skeleton.fbx** (Danish Cavalry): 33 Mixamo-knogler (`mixamorig:`; kun Index1-4 som fingre, ingen Thumb), ingen animation, mesh 40.000 trekanter, 28 vertex-grupper (teksturen er ikke læsbar i FBX'en; `Rider_2_textured.glb` findes). Figuren er **en færdig siddende rytter** (hofte 4,7 cm, hoved 50 cm over origo, hænder foran; bindeposen er sædet, ben bøjet 90 grader), ikke A-pose. Den kan ikke bindes til SK_Human uden at blive af-posert til soldatens stående bindepose (ellers passer knoglerne ikke), og skelettets knoglepositioner er en anden proportion. Ikke retargetet; bruges ikke i v1. Kan bruges som reference for, hvordan sædet skal se ud.
- DK_Garderhusar og SE_Gardehusar (Ridepose) er ikke del af opgaven.

## Mixamo-klippene i Danish Cavalry (ikke overført nu)
`Draw Sword 1.fbx` (16 frames), `Sheath Sword 2.fbx` (26), `One Hand Sword Combo.fbx` (137), `Withdrawing Sword.fbx` (46), `Sword Fight One.fbx` (246) og `Sheathing Sword.fbx` (51): alle **samme 41-knogles Mixamo-skelet som SK_Human** (`mixamorig:Hips` ... med Thumb1-4 og Index1-4, uden Middle/Ring/Pinky; armatur-skala 0,01, hofte -1,3 cm), kun armatur uden mesh. Frame-tal i Blender (start 1). De har samme knoglenavne som soldaterne og kan derfor bruges uden retargeting (stående sabelklip).
