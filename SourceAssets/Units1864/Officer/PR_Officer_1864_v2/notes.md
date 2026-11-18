# PR-officer 1864 — Hunyuan3D-2mv

Genereret 30. oktober 2026. Leverance: `../PR_Officer_1864_v2.glb`, indlejret tekstur samt separat `../PR_Officer_1864_Texture_0_v2.png`.

- Trekanter: **40.000**. Over crowd-målet på ca. 20.000; ingen efterfølgende decimering. Appens standardpipeline reducerer selv geometrien inden teksturering. Den rå geometri havde 127.352 trekanter.
- Tekstur: **2048 × 2048**, PNG/base color. Ikke et komplet PBR-tekstursæt.
- Kilder: `Reference/Units/MultiView/PR Officer/PR_Officer_1864_Front.png`, `PR_Officer_1864_Side.png` (venstre) og `PR_Officer_1864_Back.png` i samme mappe. `Normal.png` blev ikke brugt.
- Indstillinger: multiview, profil 3, 30 trin, guidance 5, seed 1234, octree 256, chunks 8000, baggrundsfjernelse aktiveret.
- Hænder: flade og delvist sammenvoksede fingre, mørke/røde artefakter ved håndled og fingerspidser.
- Ansigt: meget bløde øjne/mund og plettet hud; hjelmens metaltekstur er stærkt marmoreret og udtværet.
- Bælte: spænde og gehæng er uklare og flade; ekstra brunlige rempletter på ryggen.
- Fødder: hele, men lyse/grå teksturartefakter langs støvlesnuder og såler; bukser og støvler flyder visuelt sammen.
- Huller: ingen synlige i fire CPU-renderede visninger. En kontrolkopi med sammenlagte vertices på UV-sømme har 0 åbne kanter og er watertight. Eksporten blev ikke ændret af kontrollen.

Se `preview_v1.png` og `metrics_v1.json`. Modellen er ikke rigget eller valideret i Unreal. Appen tilbyder ikke FBX.
