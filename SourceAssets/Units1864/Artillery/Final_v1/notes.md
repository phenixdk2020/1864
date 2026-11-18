# Artillerister v1 - bygget af Tools/Animation/build_artillery_rig_1864.py

Blender 5.2 headless (kun CPU-render til kontrolark). Ikke importeret i Unreal (hand over til unreal-integration). Input (uændret): `Artillery/{DK,NO,SE,PR}_Artilleri_1864/*_textured*.glb`.

## Filer
- `SK_Artillery_1864_{DK,NO,SE,PR}.fbx` + `SK_Artillery_1864_{NAT}_Texture_0.png` (2048 x 2048, ved siden af, som officerens `_Texture_0.png`). 40.000 trekanter pr. model, 198,8-199,5 cm høje, sål ved -100 cm (som soldaten).
- `report.json` (mål pr. nation, knoglekontrol, kantstrækning i 7 poser, håndled; `soldier_baseline` = samme kantmål på soldatens eget mesh).
- `Preview/Artillery_{NAT}_sheet.png`: kolonner = Hvile, A_Gun_Sponge fr. 48 og 70, A_Gun_Ram fr. 38, A_Gun_Load fr. 38, A_Combat_WalkShoulder fr. 8 og 22, A_Combat_DeathFront fr. 30; øverste række forfra, nederste fra siden.
- `Preview/Artillery_{NAT}_zoom.png`: overkrop (krave, skuldre, hænder) i Hvile, Sponge 48, Ram 38 (side), Load 38, Walk 8.
- Genkør: `blender -b -P Tools/Animation/build_artillery_rig_1864.py -- all [DK NO SE PR]` (`build` / `preview`). Eksisterende filer gemmes som `*.prev.*`.

## Skelet
Soldatens armatur uændret (`DK_Infantry_1864.fbx`): 41 knogler, navne, forældre og bindepose (største afvigelse 9e-7 mod soldatens FBX, målt efter re-import af den eksporterede FBX), skala 0,01, ingen forskydning. Mesh-noden er barn af armaturet med soldatens transform (skala 100, rot -90), som i soldatens FBX. Eksport: akser -Z/Y, ingen leaf-bones, ingen animation.

## Vægte
Modellerne ligner soldatens proportioner (±3 cm) og flyttes kun translation (≤ 2 mm). Vægte = nearest-surface-transfer fra soldatens mesh (barycentrisk), efter at soldatens arme er posetilpasset modellens (underarm- og håndtyngdepunkt, 2 iterationer; ellers får hænder nær lårene lårvægte - SE). Derefter: udglatning langs kanter (2 gange, sammensvejste UV-split-positioner får ens vægte), håndled efter tekstur (hud = hånd, manchet/ærme = underarm, overgang 5 cm centreret på soldatens håndleds-led), hænder er vanter (kun `Hand`, ingen finger-knogler, 22 vertex-grupper), normaliseret, maks. 4 pr. vertex (kontrolleret efter re-import: 0 vertices med sum != 1).

## Ikke testet / svagheder
Se svarets liste (samme indhold): fingre ikke animerbare, manchetpibninger smøres ved kraftig håndledsbøjning, aksel/armhule strækkes som hos soldaten (NO mere), kun arme er tilpasset (hoved/ben/torso er ren transfer), stropper/pung/fjerbusk følger nærmeste knogle uden sekundær bevægelse, SE-ansigtstekstur er plettet i kilden, kontrolark bruger pose-overførsel af lokale rotationer (ikke Unreals retargeting), ingen Unreal-import.
