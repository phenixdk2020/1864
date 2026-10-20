# FBX-undersøgelse, 2026-10-16 - korrigeret efter spiltests

Brugerens spiltests fastslår: med den aktuelle default FbxImportUI ligger Walk-konventionen (-Y/Z, Z-op, Armature ca. 90/0/180 grader) ned. Den gamle skulderkonvention (-Z/Y, Y-op, Armature ca. 0/0/0) står op. Min tidligere anbefaling om Walk-konventionen var forkert og er trukket tilbage. Memory-reglen `blender-fbx-export-axis` gælder.

Det første fejlslagne skulderforsøg havde allerede Y-op. Akser alene forklarer derfor ikke den regression. Restpose/knogleorientering/kildeskelet er stadig ikke isoleret som enkelt rodårsag. V3 undgår den ændring ved at bruge hver af de tre kendt oprejste filer i `Saved/Codex/shoulder_arms/old_clips/`, ikke DK-skelettet eller `walk_rifle_basis.json`.

Ingen Unreal-import, VAT-bagning, build, spil/editor-start eller commit er udført. Otte originale FBX og otte originale uassets er SHA-256-kontrolleret uændrede.

## Eksport og backup

Alle 20 nedenstående FBX er regenereret. Før betyder filen ved starten af denne opgave (forsøg 2 for skulderklippene).

- Z: axis_forward='-Y', axis_up='Z'; UpAxis/sign=2/+1, FrontAxis/sign=1/+1, CoordAxis/sign=0/-1; Armature rotation=(90.0000093,0,180.0000050).
- Y: axis_forward='-Z', axis_up='Y'; UpAxis/sign=1/+1, FrontAxis/sign=2/+1, CoordAxis/sign=0/+1; Armature rotation=(-0.000004325710697,0,0), dvs. numerisk nul.
- Begge: UnitScaleFactor=OriginalUnitScaleFactor=1; Armature translation=(0,0,0), scale=(1,1,1).

| FBX | Før -> efter |
|---|---|
| A_Combat_ReloadKneelingB.fbx | Z -> Y |
| A_Combat_ReloadKneeling.fbx | Z -> Y |
| A_Combat_RiflePunch.fbx | Z -> Y |
| A_Combat_SideStep.fbx | Z -> Y |
| A_Combat_Turn.fbx | Z -> Y |
| A_Combat_Signal.fbx | Z -> Y |
| A_Combat_DeathBackHeadshot.fbx | Z -> Y |
| A_Combat_SabreDraw.fbx | Z -> Y |
| A_Combat_SabreWithdraw.fbx | Z -> Y |
| A_Combat_SabreSheath.fbx | Z -> Y |
| A_Combat_SabreSheath2.fbx | Z -> Y |
| A_Combat_SabreCombo.fbx | Z -> Y |
| A_Combat_SabreFight.fbx | Z -> Y |
| A_Combat_ReloadStandingV03.fbx | Z -> Y |
| A_Combat_CavSabreDraw.fbx | Z -> Y |
| A_Combat_CavSabreSlash.fbx | Z -> Y |
| A_Combat_CavSabreFight.fbx | Z -> Y |
| A_Combat_WalkShoulder.fbx | Z -> Y |
| A_Combat_ShoulderUp.fbx | Z -> Y |
| A_Combat_ShoulderDown.fbx | Z -> Y |

De 17 ekstra animationer er importeret fra de eksisterende FBX og geneksporteret i Blender 5.2 med alle frames ved 30 fps; ingen ny bevægelse er genereret. Importens frame-offset (start ved Blender-frame 2) normaliseres til 1 uden at miste første/sidste sample. Kun armature eksporteres, ingen leaf bones, ingen NLA/all-actions, simplify=0.

Backup af de 17: `Saved/Codex/axes_fix/before/`. Backup af opgavens tre skulderfiler: `Saved/Codex/shoulder_arms/v3/before/`. De oprejste `old_clips` er kun læst. Script: `Tools/Animation/fix_extra_fbx_axes.py`. De tre eksisterende extra-buildscripts bruger allerede -Z/Y og er ikke ændret. Originalgeneratoren `build_infantry_combat_set.py` er ikke kørt.

## Skulderpositur og ShoulderUp

`build_shoulder_arms_set.py` bruger nu de tre gamle FBX direkte. Hips X/Z holdes ved animationens første sample i armaturets koordinater; Y-bob bevares. Bemærk at den gamle statiske Model-translation Z=121.002 cm var forskellig fra animationens første Z=-0.3696 cm; det er animationssamplet, der skal fastholdes. Ingen kunstig højdejustering anvendes.

Kun RightArm og RightForeArm rotationsspor korrigeres med to-knogle-IK. Mål i soldatens koordinater: (0.21,0.15,1.06) m, fulgt af hofternes vertikale bevægelse. Hånd/fingre, venstre arm, ben og overkrop bevarer den respektive gamle fils lokale spor bortset fra FBX-flydende afrunding. Ingen ombygning fra DK-skelettet.

Før viste ShoulderUp højre hånd ved ansigtet fra starten, med kolben oppe ved bryst/hals; den forrige generator anvendte DK-restposen og genposerede også venstre arm. Nu starter højre arm i Walk-posituren og blandes med smoothstep og quaternion-slerp til IK-posituren gennem hele klippet. ShoulderDown gør det omvendte. De oprindelige venstrearmsspor i hver fil bevares som krævet.

Rå endpoint-kontrol: højre overarm, underarm og hånd matcher Walk/skulder ved begge ender (0 grader inden for målingens float-præcision). Største rotation mellem naboframes: Up 10.020 grader, Down 8.305 grader ved 30 fps. Overgangen starter/slutter uden et nyt spring i højre arms endepose.

Den eksisterende C++ Shoulder-gren bruger samme skulderhældning på geværet under HELE Up/Down, også når hænderne endnu er i Walk-positur. Derfor er geværet allerede opret i Up-frame 0, og venstre hånd griber ikke geværet der. Det er synligt i renders; fuld våbenrotation fra tohåndsbæring kræver en senere ændring af prop-reglen. C++ er ikke ændret i denne opgave.

## Rå kontrol

`Tools/Animation/verify_fbx_axes_fix.py` læser FBX direkte med io_scene_fbx.parse_fbx. Alle 20 matcher referencefilens HELE GlobalSettings, HELE Armature-Properties70 og navngivne hierarki eksakt: scene 0 -> Armature -> mixamorig:Hips, 41 knogler, ingen BindPose. Armaturrotationens lille ikke-nulværdi er præcis samme float som referencen.

Skuldernes første Hips-rotation: ca. (-12.35685,-32.37911,4.96935) grader, bevaret fra referencebevægelsen; ingen ekstra 90-graders rotation. Hips Y=84.20558..87.12918 cm. X/Z-spænd er under 0.0001 cm (eksportafrunding). Bevarede skulderspor afviger højst 0.000710 i kurvernes enheder. De 17 extras bevarer sampleantal og bevægelse: største translation/scale-afvigelse 0.000121; rotationsafvigelse under 0.056 grader (quaternion-kontrol). Euler-triples sammenlignes som rotationer, da fx dødsklippet kan have ækvivalente triples med 180-graders komponentforskelle. Knælende/monterede positurer må naturligt have bøjet hofte.

- Komplette rå træer og summaries: `Saved/Codex/shoulder_arms/raw_audit/{before_axes_*,after_axes_*,v3_*}.{tree,summary}.json`.
- Alle extra-checks og før/efter-hashes: `Saved/Codex/axes_fix/verification.json`.
- Original-hashes: `Saved/Codex/axes_fix/original_hashes_{before,after}.json`.
- Skulder-, bevarelses- og endpoint-kontrol: `Saved/Codex/shoulder_arms/v3/verification.json`.

## Renders, set visuelt

`preview_shoulder_rule.py` kan nu evaluere den faktiske FBX via miljøvariablen SHOULDER_FBX. DK-meshen er kun visningsmesh: dens knoglematricer følger kilderiggens verdensmatricer, mens geværpositionen beregnes fra kilderiggen. JSON-preview driver ikke denne kontrol.

Front/side/3-4, frames 0,10,24 for alle tre; desuden frame 41 for Up/Down. Filer: `Saved/Codex/shoulder_arms/v3/{WalkShoulder,ShoulderUp,ShoulderDown}_v3_{front,side,q34}_{00,10,24}.png`. Samleark: `WalkShoulder_v3_sheet.png`, `ShoulderUp_v3_sheet.png`, `ShoulderDown_v3_sheet.png`. Før-sammenligning: `ShoulderUp_before_sheet.png`.

Alle fire samleark er åbnet og inspiceret. Marchen viser hånd ved bæltet, bøjet højre albue og gevær tilbage over skulderen, som AFTER_final-referencen. Up/Down viser glidende armovergang; venstre arm følger de gamle spor. Materialerne vises magenta pga. manglende preview-teksturer, hvilket ikke påvirker silhuetkontrollen.

Geværreglen er læst i StrategyCrowdModel.cpp: Up*0.90 + Side*0.20 - Forward*0.33, greb 15 procent fra kolben. Preview bruger samme koefficienter og skalering 0.8 som det aftalte render. Blender-preview er ikke bevis for Unreal-importen; endelig spilkontrol af de nye filer mangler.

## Præcis genimportliste (kun navne)

```text
A_Combat_ReloadKneelingB
A_Combat_ReloadKneeling
A_Combat_RiflePunch
A_Combat_SideStep
A_Combat_Turn
A_Combat_Signal
A_Combat_DeathBackHeadshot
A_Combat_SabreDraw
A_Combat_SabreWithdraw
A_Combat_SabreSheath
A_Combat_SabreSheath2
A_Combat_SabreCombo
A_Combat_SabreFight
A_Combat_ReloadStandingV03
A_Combat_CavSabreDraw
A_Combat_CavSabreSlash
A_Combat_CavSabreFight
A_Combat_WalkShoulder
A_Combat_ShoulderUp
A_Combat_ShoulderDown
```

Samme liste findes i `Saved/Codex/axes_fix/reimport_names.txt`. Det nuværende importscript omfatter netop disse 20 extras; originalerne skal fortsat udelades. Ingen genimport er kørt.
