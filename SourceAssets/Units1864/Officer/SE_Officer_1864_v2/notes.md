# SE-officer 1864 — Hunyuan3D-2mv

Genereret 30. oktober 2026. Leverance: `../SE_Officer_1864_v2.glb`, indlejret tekstur samt separat `../SE_Officer_1864_Texture_0_v2.png`.

- Trekanter: **40.000**. Over crowd-målet på ca. 20.000; ingen efterfølgende decimering. Appens standardpipeline reducerer selv geometrien inden teksturering. Den rå geometri havde 134.816 trekanter.
- Tekstur: **2048 × 2048**, PNG/base color. Ikke et komplet PBR-tekstursæt.
- Kilder: `Reference/Units/MultiView/SE Officer/SE_Officer_1864_Front.png`, `SE_Officer_1864_Side.png` (venstre) og `SE_Officer_1864_Back.png` i samme mappe. `Normal.png` blev ikke brugt.
- Indstillinger: multiview, profil 3, 30 trin, guidance 5, seed 1234, octree 256, chunks 8000, baggrundsfjernelse aktiveret.
- Hænder: sammenvoksede fingre og mørke artefakter ved fingerspidserne; håndformen er forenklet.
- Ansigt: udtværet og let fladt, især øjne/mund; uregelmæssig hudtekstur på siden.
- Bælte: fladt spænde og gehæng, ujævne gule remdetaljer smeltet ind i frakken.
- Fødder: hele, men afrundede og med svage sål-/hæledetaljer.
- Huller: ingen synlige i fire CPU-renderede visninger. En kontrolkopi med sammenlagte vertices på UV-sømme har 0 åbne kanter og er watertight. Eksporten blev ikke ændret af kontrollen.

Se `preview_v1.png` og `metrics_v1.json`. Modellen er ikke rigget eller valideret i Unreal. Appen tilbyder ikke FBX.
