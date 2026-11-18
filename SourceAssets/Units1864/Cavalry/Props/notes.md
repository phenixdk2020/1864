# Kavaleri-rekvisitter 1864 (sabel og sadel)

Bygget af `Tools/Blender/build_cavalry_props_1864.py` (Blender 5.2 headless). Ikke importeret i Unreal (overdrages til unreal-integration).
Genkør: `blender -b --python Tools/Blender/build_cavalry_props_1864.py -- all` (eller `saber` / `saddle`). Eksisterende filer gemmes som `*.prev.*`.
Enhed: 1 Blender-enhed = 1 m; FBX'erne er i cm (apply_unit_scale), som hesteeksporten.

## Kilder (ændres ikke)
`R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\Items\`
- `Saber_1_textured.glb` (samme net som `Saber.fbx`; 40.000 trekanter, tekstur 2048). glb'en bruges, fordi FBX'ens tekstur ikke kan læses af Blender.
- `Saddle_1_textured.glb` (samme net som `Sadel.fbx`; 40.000 trekanter, tekstur 2048). Kilden er et AI-genereret, sammensmeltet net med sadel, skabrak, tasker, oprullet kappe, stigbøjler, bugreb og et helt hovedtøj med tøjler.
- `Horse_1_rigged_v3.blend`: `Horse_1_Armature_v2.001` + `Horse_1_Mesh.001` (samme par som `SourceAssets/Horse/Horse_1_rigged_v3.fbx`, dvs. SK_Horse_1864; blendfilen har et dublet-par uden `.001`).

## 1. SM_Saber_1864.fbx + SM_Saber_1864_Texture_0.png
- Skala fra kilden: 0.47674 (kilden er 1.9927 m lang). Samlet længde **94.98 cm** (mål 95 cm).
- Pivot (origin) = midt i grebet. Pommel 9.04 cm under origo, parerstang ca. 9.06 cm over origo, spids 85.94 cm over origo.
- Akser: **klinge langs lokal +Z (spids op), æggen mod +X**, flad side mod ±Y. Knoglebuen (håndbeskyttelsen) sidder på æggesiden (+X). Klingen er let krum; spidsen ligger 5.32 cm mod ryggen (-X).
- bbox (cm, i FBX): min [-6.25, -2.67, -9.04], max [6.62, 2.6, 85.94].
- **Afvigelse:** klingen fra parerstang til spids er ca. **76.88 cm** (ønsket ca. 82). Kildens proportioner er greb ca. 18.1 cm inkl. pommel og klinge ca. 81 % af længden; ved 95 cm i alt giver det 76.88 cm klinge. Jeg har ikke strakt klingen eller presset grebet sammen. Skal klingen være 82 cm, skal totallængden op på ca. 101 cm (`SABER_TOTAL_CM=101`), eller grebet kortes ned.
- Klingebredde ca. 4.02 cm, tykkelse ca. 1.01 cm (kildenettet er tykt og uregelmæssigt; tykkelsen er større end en rigtig klinges).
- Trekanter: **2900** (kilde 40.000, 38726 efter svejsning); collapse-decimering, UV bevaret. Tekstur 1024 x 1024 (nedskaleret fra 2048). Materiale `M_Saber_1864` (kun BaseColor).
- FBX: forward -Y / up Z, bake_space_transform, apply_unit_scale (samme indstillinger som `SourceAssets/Artillery/Props`). Blender +Z = Unreal +Z.
- **Skøn (ikke verificeret kilde):** længde ca. 95 cm og klinge ca. 82 cm er opgavens skøn for en dansk/svensk let kavalerisabel i 1850'erne. Modellen er en generisk krum sabel med D-formet knoglebue, ikke en bestemt regulativmodel.
- Kontrolark: `Preview/Saber_check_sheet.png`, fra venstre: sabel ved soldaten (set fra siden), set fra +X, sabel alene fra æggesiden, fra fladen, greb i nærbillede. Soldaten er projektets `DK_Infantry_1864` (199.1 cm i spillet, ikke 180 cm); sablen står med grebet i ca. 92 cm højde ved siden af ham.

## 2. SM_Saddle_1864.fbx + SM_Saddle_1864_Texture_0.png
**Placering.** Sadlens vertices er gemt i `saddle_root`-knoglens eget koordinatsystem: origo = knoglens hoved, akserne er knoglens. Hænges sadlen på socket `saddle_root` uden relativ transform, sidder den på hesten i hvilepose.
- Knogle `saddle_root` (forælder spine_02): hoved [0.0598, 0.0, 1.1819] m, hale [0.1994, 0.0, 1.1819] m (Blender-verden; armaturet ligger i origo uden rotation), længde 0.1396 m, **roll 0.0 rad (0.0 grader)**.
- Knoglens akser i Blender-verden: X = [-0.0, -1.0, 0.0] (hestens venstre), Y = [1.0, -0.0, 0.0] (mod halen), Z = [-0.0, 0.0, 1.0] (op). Hesten vender mod -X i Blender, så knoglens +Y peger bagud.
- Hesten i hvilepose (Blender-enheder = m): længde (X) 1.9939, bredde (Y) 0.5578, højde (Z, til ørespidserne) 1.6531; bbox [-0.9969, -0.2789, 0.0] til [0.9969, 0.2789, 1.6531]. Rygfladen bag manken ligger i z ca. 1,19-1,25 m, altså 1-2 cm over knoglens hoved (z 1,182 m).
- **Blender-skala på sadlen i forhold til kilden: 0.5** (uniform). Kildens akser er knoglens akser (+X hestens venstre, -Y fremad, +Z op), så der er kun skala og forskydning, ingen rotation. Kildepunktet (x 0, y 0.36, z -0.05) = sædets laveste punkt/skabrakkens top ligger i knoglerummet i (0, 0.03, 0.012) m.
- **Skala i Unreal.** Sadlen er bygget i hestens egen skala (hesten er 199.39 cm lang i FBX'en). Er hesten i Unreal skaleret til 219 cm, skal sadlen have **samme faktor: 1.0984** (219 / 199.39) på alle tre akser. Hænges den som barn af hestens skeletal mesh component på socket `saddle_root`, arver den skalaen (relativ skala 1,0). Sættes den uafhængigt, bruges 1.0984.
- Sadlens bbox i knoglerummet (cm, i FBX, før Unreal-skalering): min [-42.1, -21.9, -40.1], max [41.6, 34.9, 31.6] (X = hestens venstre, Y = mod halen, Z = op). Skabrak + kappe er ca. 57 cm lang.
- **Sædets højde:** sædets overflade (midterlinjen, ved knoglens y = -10, -5, 0, 5, 10, 15, 20 cm; cm over knoglens hoved, uden Unreal-skala): -10 cm: 23.92, -5 cm: 20.9, +0 cm: 19.45, +5 cm: 19.42, +10 cm: 21.98, +15 cm: 30.64, +20 cm: 24.9. Midt i sædet ca. 19.45 cm over knoglen. Rytteren sidder i koden `SaddleLiftCm` = 6 cm over knoglen (bar hest), så rytterens bækkenknogle skal løftes til ca. sæde + 8-10 cm (ca. 28.45 cm over knoglen), når sadlen sættes på; ikke ændret her. Stigbøjlernes trin ligger ca. 40 cm under knoglen.
- Penetration i hestens net (hvilepose): 324 af 4005 vertices, max 4.64 cm, 95-percentil 1.94 cm.
- Trekanter: **7800** (kilde 40.000; efter udskæring 28797, heraf 2863 fjernet fordi de lå gemt inde i hesten); collapse-decimering, UV bevaret. Tekstur 1024 x 1024 (nedskaleret fra 2048). UV-øerne for de bortskårne dele er ubrugte. Materiale `M_Saddle_1864`.
- FBX: forward -Z / up Y, uden bake_space_transform, apply_unit_scale: **samme indstillinger som hesteeksporten** (`Tools/Animation/export_horse_mesh_b1.py`), så sadlen får samme akseomregning som hestens knogler (sockettransformen = knoglens). Kun kontrolleret i Blender: genimport giver samme punkter i knoglerummet (maks. forskel 0.0 m). **Ikke kontrolleret i Unreal.** Hestens FBX vendes i spillet med HorseYaw; sadlen følger med, når den hænges på socket.

### Hvad er ændret i forhold til kilden
- Bortskåret: hovedtøj med nakkereb og tøjler, forreste stropper/brystreb (kilde-y < -0,12), midterstropper og bugreb under sædet (z < -0,28 ved |x| < 0,2), lavt hængende rester undtagen stigbøjler med læder. Beholdt: sadeltræ (pommel, sæde, cantle), skabrak (blå med rød kant), to sadeltasker, oprullet kappe bag cantle, læderstropper og stigbøjler. Hesten har sit eget hoved; ingen tøjler i dette asset.
- Stigbøjler og stigbøjlerem under skabrakkens kant er spredt 0.2 kildeenheder ud til siderne (kilden hænger dem inde i hestens tønde).
- Faces, der ligger inde i hestens net (sadelunderside), er fjernet.
- Kilden er uregelmæssig: kanter og tasker er grove, nogle strimler er rå. Ved kampafstand uden betydning.

## Skøn / forbehold
- Sabelmålene (95 / 82 cm) og sadlens skala (0.5) er skøn; skalaen er valgt, så skabrak og kappe dækker hestens flade ryg bag manken. Historisk korrekt kavalerisadel fra 1850'erne (model, remtøj) er ikke efterprøvet.
- Sadlen er højere end en rigtig sadel i forhold til hesten (sædet ca. 19.45 cm over knoglen); kildens form er bevaret frem for at flade den ud.
- Kontrolark: `Preview/Saddle_on_horse_sheet.png` (side, forfra med hestens hoved skåret væk, oppefra, 3/4 skråt) og `Preview/Saddle_alone_bone_frame_sheet.png` (sadlen alene i knoglerummet; origo i billedets midte; side/forfra/oppefra).
- Ingen animation og ingen deformation: sadlen følger kun knoglen (stift net).
