# Officersmodeller — 30. oktober 2026

De gennemgåede leverancer er `DK_Officer_1864_v2.glb`, `SE_Officer_1864_v2.glb`, `NO_Officer_1864_v2.glb` og `PR_Officer_1864_v2.glb`. Hver har indlejret base color-tekstur samt en separat `*_Texture_0_v2.png`, og en tilsvarende `*_v2/`-mappe med `notes.md`, målinger og fire CPU-renderede kontrolvisninger.

Alle fire har 40.000 trekanter og tekstur på 2048 × 2048. Crowd-målet på ca. 20.000 er ikke nået. Ingen efterfølgende decimering, rigging eller Unreal-import er foretaget. Appens standardteksturpipeline reducerer selv rågeometrien til 40.000 trekanter.

Kildebillederne Front, Side, Back og Normal eksisterede allerede for alle fire nationer. Ingen nye referencebilleder blev fremstillet i denne opgave; Front/Side/Back blev brugt, Side som venstre visning. Normal blev ikke brugt.

## Drift og begrænsninger

- Ingen blokerende GPU-apps var i gang før start. Codex/bridge-processer blev ignoreret efter brugerens præcisering.
- Pinokios kontrolserver var utilgængelig. Appen blev startet direkte med det angivne Pinokio-Pythonmiljø, `gradio_app.py --mv --profile 3`, port 42003; derefter blev dens Gradio-API kaldt lokalt.
- Første opstart brugte ikke Pinokios HF-cache og afsluttede med manglende lokale snapshots. Anden opstart pegede på appens eksisterende cache og lykkedes. Offline-indlæsning blev brugt.
- Ingen GPU-OOM opstod; lavere octree/profil var derfor ikke nødvendig.
- FBX tilbydes ikke af appen (GLB/OBJ/PLY/STL).
- Den første DK-kontrolrendering blev afbrudt som for langsom. Dens GLB og tekstur er bevaret som v1; identiske v2-kopier blev kontrolleret med et hurtigere CPU-script.
- Hunyuan-processerne, som blev startet her (anden opstart: 7976 og 3968), er stoppet. Efterkontrol fandt ingen `gradio_app.py`-proces og ingen lytter på port 42003. GPU: 1217 MiB, 0 % aktivitet.
- Ingen build, Unreal-start, repo-batfiler eller commit blev kørt. Ingen projektdokumentation uden for den tilladte outputmappe blev redigeret.

Rå API-resultater, originale app-eksporter og logs er bevaret under `Hunyuan_run_v1/` og `Hunyuan_run_v2/`. API-statistikkens `number_of_faces` gælder rågeometrien; leverancernes faktiske trekantantal står i hver `metrics_v1.json`.
