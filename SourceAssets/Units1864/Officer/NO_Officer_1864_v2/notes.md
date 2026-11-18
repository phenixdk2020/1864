# NO-officer 1864 — Hunyuan3D-2mv

Genereret 30. oktober 2026. Leverance: `../NO_Officer_1864_v2.glb`, indlejret tekstur samt separat `../NO_Officer_1864_Texture_0_v2.png`.

- Trekanter: **40.000**. Over crowd-målet på ca. 20.000; ingen efterfølgende decimering. Appens standardpipeline reducerer selv geometrien inden teksturering. Den rå geometri havde 139.892 trekanter.
- Tekstur: **2048 × 2048**, PNG/base color. Ikke et komplet PBR-tekstursæt.
- Kilder: `Reference/Units/MultiView/NO Officer/NO_Officer_1864_Front.png`, `NO_Officer_1864_Side.png` (venstre) og `NO_Officer_1864_Back.png` i samme mappe. `Normal.png` blev ikke brugt.
- Indstillinger: multiview, profil 3, 30 trin, guidance 5, seed 1234, octree 256, chunks 8000, baggrundsfjernelse aktiveret.
- Hænder: lange, flade og delvist sammenvoksede fingre; pletvis hudtekstur.
- Ansigt: tydeligt plettet/udtværet hud, svage øjne og mund; ikke egnet til nærbilleder.
- Bælte: remme ligger fladt mod frakken; spænde er forenklet. Ærmernes guldrøde pynt er ujævn og udtværet set fra siden.
- Fødder: hele, men grove støvlesnuder og svage sål-/hæledetaljer.
- Huller: ingen synlige i fire CPU-renderede visninger. En kontrolkopi med sammenlagte vertices på UV-sømme har 0 åbne kanter og er watertight. Eksporten blev ikke ændret af kontrollen.

Se `preview_v1.png` og `metrics_v1.json`. Modellen er ikke rigget eller valideret i Unreal. Appen tilbyder ikke FBX.
