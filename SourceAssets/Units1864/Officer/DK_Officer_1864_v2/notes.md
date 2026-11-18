# DK-officer 1864 — Hunyuan3D-2mv

Genereret 30. oktober 2026. Leverance: `../DK_Officer_1864_v2.glb`, indlejret tekstur samt separat `../DK_Officer_1864_Texture_0_v2.png`.

- Trekanter: **40.000**. Over crowd-målet på ca. 20.000; ingen efterfølgende decimering. Appens standardpipeline reducerer selv geometrien inden teksturering. Den rå geometri havde 144.282 trekanter.
- Tekstur: **2048 × 2048**, PNG/base color. Ikke et komplet PBR-tekstursæt.
- Kilder: `Reference/Units/MultiView/DK Officer/DK_Officer_1864_Front.png`, `DK_Officer_1864_Side.png` (venstre) og `DK_Officer_1864_Back.png` i samme mappe. `Normal.png` blev ikke brugt.
- Indstillinger: multiview, profil 3, 30 trin, guidance 5, seed 1234, octree 256, chunks 8000, baggrundsfjernelse aktiveret.
- Hænder: flere fingre er sammenvoksede/flade; tommelfingre og fingerled er svage.
- Ansigt: bløde øjne, mund og øredetaljer; ikke egnet til nærbilleder.
- Bælte: spænde og gehæng er overvejende tekstur/fladt relief; remme flyder sammen med frakken.
- Fødder: hele, men afrundede støvler og udviskede såldetaljer.
- Huller: ingen synlige i fire CPU-renderede visninger. En kontrolkopi med sammenlagte vertices på UV-sømme har 0 åbne kanter og er watertight. Eksporten blev ikke ændret af kontrollen.

Se `preview_v1.png` og `metrics_v1.json`. Modellen er ikke rigget eller valideret i Unreal. Appen tilbyder ikke FBX. Tidligere v1 er en identisk GLB/teksturkopi fra en afbrudt langsom QA-rendering; v2 er den gennemgåede leverance.
