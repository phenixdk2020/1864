# Hestens hale (SK_Horse_1864) - rettelse 2026-11-13

Ikke importeret i Unreal (det goer unreal-integration).

## Hvad var galt
- Halemeshet var ikke en hale men ca. 7.300 flader (4.734 vertices vaegtet paa tail_01-03): en tyk, aaben skal af hvidt/graat haar fra krydset til haserne. UV'erne pegede paa det lyse haar-omraade i `Horse_Texture_0.png` (derfor hvid/graa); ingen vertex-farver, ingen alfa-kort.
- En stor del af haarfladerne var **ikke** vaegtet paa tail_01-03 alene, men delvist paa laar, skinneben og baekken, og store lapper af bagbenenes bagside (haser, mork) var fejlvaegtet paa tail_02/03. Naar bagbenet svingede, blev de dele staaende med halen: de moerke "planker" ved haserne og den graa "stang" (en tynd haarlap midt mellem haserne). Ses i alle klip (A_Horse_Walk/Trot/Gallop og M2M-klippene).
- Mesh2Motion-klippene, skelet og knogler var uden skyld.

## Hvad er aendret
Script: `Tools/Animation/fix_horse_tail.py` (koeres paa Horse_1_rigged_v3.blend, blend-filen gemmes ikke).
1. Det gamle haarmesh fjernet (8.516 flader + ca. 60 smaa loese oeer/splinter midt mellem haserne og moerke splinter).
2. Bagbenenes fejlvaegtede bagside beholdt og vaegtet om fra naermeste ben-/baekken-vertex (ca. 360 vertices).
3. Ny lukket, tilspidset hale (tube, 40 x 56, ca. 2.300 vertices, haarlokke-buler, afrundet spids, lukket top inde i krydset), vaegtet blodt over tail_01-tail_03 (+ baekken foroven).
4. Halens UV'er flyttet til en ensfarvet moerk plet i den eksisterende tekstur (texel ca. 150-154, 870-876, lineaer 0,07/0,05/0,05 = benenes moerke gra-brune farve). **Teksturen er uaendret.**
5. Alt andet er uroert: 39 knogler (identiske navne og positioner), 35 vertex-grupper, skala 1, akser og eksportindstillinger som `export_horse_mesh_b1.py`, materiale `Material_0.001`, ovrig geometri og custom normals (0 aendrede af 73.238 sammenlignede loops). Kun mesh-boundingboxens laengde aendres (1,994 -> 1,963 m, fordi halen er lidt kortere bagud).

## Filer
- `Horse_1_rigged_v3.fbx` - rettet (ny). `Horse_1_rigged_v3.prev.fbx` - den gamle.
- `Preview/tail_{rest,trot,gallop}_foer_efter.png` (oeverst foer, nederst efter; raekker = frames, soejler = bagfra / fra siden / skraat bagfra). Trot = A_Horse_M2M_Trot frame 1/8/15, galop = Reference/Animations/Horse/A_Horse_Gallop. Enkeltark `tail_prev_*.png` / `tail_new_*.png`.
- Kontrolark-script: `Tools/Animation/preview_horse_tail.py`.

## Kendt
- Halen er skinnet rigidt paa tail_01-03; klippene bevaeger kun halen med baekkenet, saa den hanger stift og kan skaere igennem bagbenene i galop (som foer).
- Grundmeshet (genereret) har mange smaa loese flader ved bagbenene; nogle faa lyse smaaflager ses stadig paa haserne i galop.
- Vaegtene paa tail_01-03 er ikke mere end blod interpolation; ingen fysik/sekundaer animation.
