# Officer v5 (kaptajn) - bygget af Tools/Animation/build_officer_clips_1864.py

Input fra Final_v4/Anim_v4 er uændret. Skelet: samme 41 Mixamo-knogler (navne, forældre, bindepose og roll) som DK_Infantry_1864 / SK_Human_1864 (maks. afvigelse 0,0001). Armaturet er eksporteret med skala 0,01 og uden forskydning som soldaterne (v4 havde 0,009 og +0,902 m). Officeren er derfor lige så høj som soldaten (ca. 199 cm, sål ved -100 cm i bindeposen).

## Filer
- Mesh: SK_Officer_1864_{DK,SE,PR,NO}.fbx (+ _Texture_0.png). Importér med skeleton = SK_Human_1864 (som import_units_1864.py).
- Klip (30 fps, armatur + mesh som fanebærerens klip, importér animation-only med roll 90): A_Officer_Walk (30 frames), A_Officer_WalkSheathed (30), A_Officer_Idle (90, loop), A_Officer_Order (60, loop), A_Officer_Death (104).
- Rekvisitter: SM_Officer_Saber.fbx, SM_Officer_Scabbard.fbx (uændrede kopier af Props_v4, cm efter import).
- Walk: ben/hofte/ryg/hoved = soldatens A_Combat_WalkShoulder (samme hofte, rod 0 cm, afspilningsrate = hastighed / 137,70097 som i koden). Arme/hænder = March_Skulder-posen (sabel opret ved højre skulder; hånden og grebet er fast i håndknoglen).
- WalkSheathed: samme ben/rod; højre arm svinger frit (fra den rå Mixamo-gang); venstre hånd på hjaltet via to-knogle-IK (hjalt = prop-punkt x=-0,03 m i skedens socket).
- Death: soldatens A_Combat_DeathFront med hofte-position som soldatens; første frame = Hvil-posen, indblandet over 8 frames (krop) og 14 frames (arme).
- Højre hånds mesh har krummede fingre til sabelgreb; i WalkSheathed er højre hånd tom, men bevarer den form.

## Faste Unreal-sockets (uændret fra Final_v4, forudsætter bone-rum i cm som hos soldaten; Scale skal anvendes)
- Sabel (i hånden) - Unreal-knogle `RightHand`: Location (8.859873, -10.981565, 4.255536); Rotation P/Y/R (-38.611339, -44.486661, -148.198385); Scale (1.106196, 1.106196, 1.106196).
- Skede - Unreal-knogle `Hips`: Location (33.243320, -9.561045, -19.432434); Rotation P/Y/R (-20.730171, 84.857239, -90.000003); Scale (1.106196, 1.106196, 1.106196).
- **Sabel i skede (Hips)**: præcis samme transform som skeden (Location (33.243320, -9.561045, -19.432434), Rotation P/Y/R (-20.730171, 84.857239, -90.000003), Scale 1.106196). Sabel- og skede-FBX deler origin og akse (begge x fra -0,085 til 0,856 m; klingeradius ca. 3 cm mod skede 3,4-4,7 cm), så relativ transform sabel->skede er identitet. Ikke målt med mesh-intersektion.

## Sabelklip i spillet (A_Combat_SabreDraw/Sheath/Sheath2/Withdraw)
Samme skelet (41 knogler), men de passer ikke til officerens skede: håndfladen kommer højst til 32-39 cm fra hjaltet (hånden ender ca. 36 cm foran hoften, hjaltet sidder 18 cm bag hoftens midte). Se report.json (sabre_clips).

## Ikke testet
Ingen Unreal-import, ingen afspilning i spillet. Sockets er ikke kontrolleret i Unreal. Mesh-node-skala 110,6 (som i v4) er ikke prøveimporteret.
