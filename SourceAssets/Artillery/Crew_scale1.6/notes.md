# Kanonbesætning v1 (6-pund feltkanon) - bygget af Tools/Animation/build_gun_crew_clips_1864.py

Blender 5.2 headless. Ikke importeret i Unreal (hand over til unreal-integration). 30 fps, på stedet, soldaternes skelet (41 Mixamo-knogler som DK_Infantry_1864 / SK_Human_1864, 199 cm hoej). Eksport som fanebaererens/officerens klip: armatur skala 0,01, akser -Z/Y, armatur + mesh i hver FBX (importer animation-only med roll 90). Klippene er ren procedural pose (IK) uden kilde-klip; historisk korrekthed (hvem står hvor, håndgreb, rytme) er et SKØN.

**Hofte-hvilehøjde (rettet):** Hips i klippene er løftet 98.079 cm i rigrummets op-akse, så Hips Lcl Translation svarer til A_Officer_Idle / A_Combat_*-klippene (ca. 93,5 cm). Uden dette sank figurerne ca. 98 cm. **Importen skal derfor køres med GUN_CREW_FIX_HIPS=0** (ingen ekstra tillæg). De gamle 2,2-klip i Crew/ har IKKE rettelsen.

## Kanonens mål i spillets skala (Cannon.fbx x 1.6; højder over jorden = hjulenes underkant)
- Kanon-ramme F: f = mod mundingen (Unreal-meshens +Y), l = kanonens venstre set bagfra (meshens +X), u = højde over jorden. Meshens origo ligger 77.6 cm over jorden (hjulunderkant), så meshens Z = u - 77.6.
- **Mundingshøjde: 125.2 cm** (boringens centrum, mundingsfladen ved f = 159.1 cm fra meshens origo; mundingens ydre radius ca. 17.8 cm).
- **Lavettens ende**: f = -159.7 cm, overkant (håndtag) u = 42.2 cm, halv bredde 15.3 cm; sporet slutter på jorden.
- Hjulaksel: f = 60.9 cm, u = 77.0 cm, hjulradius 73.5 cm, hjulets yderside l = +-104.0 cm. Kanonen er i alt 319 x 209 x 154 cm (uskaleret mesh [130.7, 199.3, 96.2]).
- Fænghul/sigteknop (højeste punkt på øverste løb): f = 1.6 cm, u = 151.3 cm. Bagende af løbet (knappen) ca. f = -72.0 cm.
- Skala 1.6 (projektlederens valg): soldaten er 199 cm; mundingen sidder 125 cm oppe (skulderhøjde 144 cm), fænghullet 151 cm, hjuldiameter 147 cm. Hænderne er ved sponge/ram ca. 128 cm over jorden (brysthøjde), og fænghullet kan nås med strakt arm. Stationer og baner er genfittet til denne skala.

## Klip, stationer og frames (frames er 1-baserede som i FBX)
Station = hvor figurens jordpunkt (under hofterne) står i F (f, l i cm fra meshens origo) og hvilken vej den ser (grader fra mundingsretningen mod venstre). Unreal-værdi: i kanonmeshens lokale ramme (X = l, Y = f, Z = højde over meshens origo; blikretning som (x, y)). Stationerne er forslag, valgt så IK rammer; de er ikke koordineret mod andre figurer eller hjul-kollision i spillet.

| Klip | Frames | Sek | Loop | Station F (f, l) / retning | Unreal (X, Y, Z) / blik (x, y) | Maks. IK-miss (cm) |
|---|---|---|---|---|---|---|
| A_Gun_StandBy | 90 | 3.00 | ja | (-73, -54) / 90 grader | (-54, -73, -78) / (1.000, 0.000) | 0.0 |
| A_Gun_Sponge | 91 | 3.00 | nej (første = sidste frame) | (211, -25) / 75 grader | (-25, 211, -78) / (0.966, 0.259) | 0.0 |
| A_Gun_Load | 76 | 2.50 | nej (første = sidste frame) | (130, 38) / 0 grader | (38, 130, -78) / (0.000, 1.000) | 2.7 |
| A_Gun_Ram | 76 | 2.50 | nej (første = sidste frame) | (211, -25) / 75 grader | (-25, 211, -78) / (0.966, 0.259) | 0.0 |
| A_Gun_Prime | 61 | 2.00 | nej (første = sidste frame) | (-6, -42) / 100 grader | (-42, -6, -78) / (0.985, -0.174) | 0.1 |
| A_Gun_Aim | 90 | 3.00 | ja | (-169, 0) / 0 grader | (0, -169, -78) / (0.000, 1.000) | 1.1 |
| A_Gun_Push_Loop | 36 | 1.20 | ja | (-27, -121) / 60 grader | (-121, -27, -78) / (0.866, 0.500) | 0.0 |

Roller: Sponge, Ram = nr. 1 til **højre** for mundingen (står foran mundingen, l < 0); Load = nr. 2 til **venstre** (l > 0); Prime = ved fænghullet på højre side; Aim = bag lavettens ende; Push_Loop = ved højre hjul; StandBy = valgfrit sted.

## Hastighed, længde og rodbevægelse
Ingen klip bevæger sig frem (alle på stedet). Hofteknoglen (Hips) er roden; netto rodforskydning første til sidste frame er 0 for alle klip. Udsving (hofteknoglens max-min x / z / y, cm):
- A_Gun_StandBy: 90 frames = 3.00 s; udsving x/z/y = [2.0, 0.0, 0.7] cm; netto = [-0.07, 0.0, -0.05] cm.
- A_Gun_Sponge: 91 frames = 3.00 s; udsving x/z/y = [22.3, 6.0, 6.5] cm; netto = [0.0, 0.0, 0.0] cm.
- A_Gun_Load: 76 frames = 2.50 s; udsving x/z/y = [0.0, 9.0, 2.7] cm; netto = [0.0, 0.0, 0.0] cm.
- A_Gun_Ram: 76 frames = 2.50 s; udsving x/z/y = [25.5, 6.8, 7.4] cm; netto = [0.0, 0.0, 0.0] cm.
- A_Gun_Prime: 61 frames = 2.00 s; udsving x/z/y = [22.0, 16.4, 7.1] cm; netto = [0.0, 0.0, 0.0] cm.
- A_Gun_Aim: 90 frames = 3.00 s; udsving x/z/y = [0.0, 0.0, 2.0] cm; netto = [0.0, 0.0, -0.14] cm.
- A_Gun_Push_Loop: 36 frames = 1.20 s; udsving x/z/y = [2.4, 0.0, 1.0] cm; netto = [-0.21, 0.0, -0.17] cm.
- **A_Gun_Push_Loop gangfart**: standfodens hastighed relativt til kroppen (median over frames med foden på jorden): venstre 54.5 cm/s, højre 54.5 cm/s (skridtlængde 40 cm, 2 skridt pr. 1,2 s). Afspil med kanonens/mandens faktiske hastighed / dette tal for at undgå fødder der glider. Fødderne er flade (ingen hælløft), swing-højde 7 cm.

## Rekvisit-sockets (Unreal-konvention som Officer/Final_v5/notes.md: knogle RightHand, Location cm, Rotation P/Y/R grader, Scale 1)
Skala er 1,0 (rekvisitterne er i cm). Rekvisittens relative transform under socket = identitet. Blender-matrixen er i report.json (socket_blender_bone_local). Rekvisitternes origo: SM_Sponge / SM_Rammer ved græbspunktet 66,7 cm fra butten (hovedets spids z = +133,3 cm); kuglen ved sit centrum. Venstre hånd er IK på skaftet (ingen socket; den er aldrig fastgjort).
- **A_Gun_Sponge - SM_Sponge**: Location (3.20, -6.00, 45.00); Rotation P/Y/R (0.00, 90.00, 180.00); Scale (1, 1, 1).
  - Højre hånd griber skaftet ved prop-z = 45 cm (overhåndsgreb, tommel mod hovedet), venstre hånd glider på skaftet ved z ca. 80 cm (IK, ca. 35 cm foran højre).
  - Kontrol (målt på den eksporterede pose): skaftets spids er dybest ved frame 31: F = [130.3, -0.0, 125.2] cm, dvs. 29 cm inde i mundingen (mundingsflade f = 159), afstand fra boringens akse (l, u) = [-0.0, -0.0] cm. Start/slut: spids ved F = [172.3, 0.0, 125.2] (uden for mundingen).
- **A_Gun_Load - SM_Cannonball_12pdr**: Location (4.20, -6.00, 0.00); Rotation P/Y/R (0.00, 0.00, 0.00); Scale (1, 1, 1).
  - Kuglen er synlig i hånden frame 15-45. **Frame hvor den slippes/forsvinder ved mundingen: frame 45** (kuglens centrum ved slip: F = [153.1, 0.0, 124.2] cm; mundingsflade f = 159, akse u = 125; dvs. ca. 6 cm inde i mundingen).
- **A_Gun_Ram - SM_Rammer**: Location (3.20, -6.00, 45.00); Rotation P/Y/R (0.00, 90.00, 180.00); Scale (1, 1, 1).
  - Højre hånd griber skaftet ved prop-z = 45 cm (overhåndsgreb, tommel mod hovedet), venstre hånd glider på skaftet ved z ca. 80 cm (IK, ca. 35 cm foran højre).
  - Kontrol (målt på den eksporterede pose): skaftets spids er dybest ved frame 41: F = [124.3, -0.0, 125.2] cm, dvs. 35 cm inde i mundingen (mundingsflade f = 159), afstand fra boringens akse (l, u) = [-0.0, -0.0] cm. Start/slut: spids ved F = [172.3, 0.0, 125.2] (uden for mundingen).

## Hændelser (frames, 1-baserede)
- A_Gun_Sponge: {"deepest_insertion_frames": [31, 55], "twist_frames": [31, 55]}
- A_Gun_Load: {"ball_taken_frame": 15, "ball_released_frame": 45}
- A_Gun_Ram: {"thrust_1_frame": 23, "thrust_2_frame": 41}
- A_Gun_Prime: {"friction_tube_in_vent_frame": 23, "lanyard_pull_frame_FIRE": 45, "recoil_flinch_frames": [45, 55]}
- A_Gun_Aim: {"signal_frames": [49, 73]}
- A_Gun_Push_Loop: {"step_length_cm": 40.0, "swing_frames": 14.0}

## Kontroltal (målt på de eksporterede poser)
- A_Gun_StandBy: laveste mesh-punkt ift. jord (cm) min/max = [0.04, 0.06]; fødder på jorden: venstre 90/90 frames, højre 90/90 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 0.0 cm, højre 0.0 cm; maks. IK-miss {'LeftFoot': 0.0, 'RightFoot': 0.0, 'LeftHand': 0.0, 'RightHand': 0.0} cm.
- A_Gun_Sponge: laveste mesh-punkt ift. jord (cm) min/max = [0.06, 0.08]; fødder på jorden: venstre 69/91 frames, højre 91/91 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 0.35 cm, højre 0.0 cm; maks. IK-miss {'LeftFoot': 0.0, 'RightFoot': 0.0, 'LeftHand': 0.0, 'RightHand': 0.0} cm.
- A_Gun_Load: laveste mesh-punkt ift. jord (cm) min/max = [0.06, 0.08]; fødder på jorden: venstre 76/76 frames, højre 76/76 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 0.0 cm, højre 0.0 cm; maks. IK-miss {'LeftFoot': 0.0, 'RightFoot': 0.0, 'LeftHand': 0.0, 'RightHand': 2.74} cm.
- A_Gun_Ram: laveste mesh-punkt ift. jord (cm) min/max = [0.06, 0.08]; fødder på jorden: venstre 56/76 frames, højre 76/76 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 0.0 cm, højre 0.0 cm; maks. IK-miss {'LeftFoot': 0.0, 'RightFoot': 0.0, 'LeftHand': 0.0, 'RightHand': 0.0} cm.
- A_Gun_Prime: laveste mesh-punkt ift. jord (cm) min/max = [0.04, 9.08]; fødder på jorden: venstre 20/61 frames, højre 25/61 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 0.01 cm, højre 0.0 cm; maks. IK-miss {'LeftFoot': 0.06, 'RightFoot': 0.0, 'LeftHand': 0.0, 'RightHand': 0.0} cm.
- A_Gun_Aim: laveste mesh-punkt ift. jord (cm) min/max = [0.08, 0.08]; fødder på jorden: venstre 90/90 frames, højre 90/90 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 0.0 cm, højre 0.0 cm; maks. IK-miss {'LeftFoot': 0.0, 'RightFoot': 0.0, 'LeftHand': 1.05, 'RightHand': 0.14} cm.
- A_Gun_Push_Loop: laveste mesh-punkt ift. jord (cm) min/max = [0.04, 0.1]; fødder på jorden: venstre 23/36 frames, højre 23/36 frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre 1.82 cm, højre 1.82 cm; maks. IK-miss {'LeftFoot': 0.0, 'RightFoot': 0.0, 'LeftHand': 0.0, 'RightHand': 0.0} cm.

## Preview
Preview/A_Gun_<klip>_side_sheet.png = 6 frames set fra siden (kamera på mandens side, orthografisk, kanon + jordlinje; mundingen mod højre når kameraet er på højre side). Preview/A_Gun_<klip>_top_sheet.png = samme 6 frames set oppefra (mundingen mod højre). Preview/A_Gun_grips_closeup.png = nærbilleder af håndgreb (Sponge f=31, Ram f=23, Load f=35). Billederne er Workbench-renders; rekvisitterne er placeret som i spillet (RightHand + socket).

## Svagheder (ærlig liste)
- A_Gun_Prime når fænghullet (IK-miss 0.0 cm) men kun med strakt arm, tåhævning og fremadlæning; hånden er højt over skulderen. Sponge/Ram/Aim/Push er fittet til skala 1.6 og kan ikke genbruges ved en anden skala uden genkørsel.
- Sponge/Ram: skaftet føres svagt skråt (stigning 0 grader før, ca. 60 % heraf ved indføring). Hovedet er kun 25-35 cm inde i mundingen (200 cm lang stok mod 231 cm løb i spillets skala); svabring til bunden er ikke vist.
- Hænderne er votte (kun Index1-3 og Thumb1-3 har vægte): fingre kan kun krummes som en blok, tommelfingeren er ikke bevaeget. Greb er derfor omtrentlige; skaftet går gennem håndfladen men kan skaere gennem fingerblokken. Se A_Gun_grips_closeup.png.
- Håndleds-vrid (sponge drejer ca. 110 grader) er delt 50/50 mellem underarm og hånd uden twist-knogler: mulig "slikpapir"-deformation i underarmen.
- Ladning/kugle gives ikke videre fra en anden figur: A_Gun_Load viser en modtagegestus i luften (frame 12-14) uden en partner. Tændrør og aftrækkersnor er ingen rekvisitter; Prime viser dem kun som håndbevægelse. Rekyl-reaktionen (frame 44-54) er en lille læning/hovedvending, ikke et trin tilbage.
- Aim: hovedet kigger op langs løbet, men øjenhøjden (ca. 150 cm i bøjning) er under løbets overkant (151 cm); kommandoeren ser altså i retning af løbet, ikke over det. Tegnet med hånden er en simpel haeven arm.
- Push_Loop: fødder flade, ingen hælløft; hænder fast på hjulets bagkvadrant (hjulet drejer ikke i klippet - kanonen/hjulet skal føres af spillet; hænderne vil ikke følge eger). Hastigheden er fast (se ovenfor), ikke aflæst fra kanonen.
- Stationer, kollision med kanonens hjul/lavet og med andre mand er ikke testet i spillet. Alle sockets og stationer er ikke kontrolleret i Unreal (ingen import foretaget). Unreal-konverteringen af socket er den samme formel som officerens, ikke prøvet på disse rekvisitter.
- Referencen Kanon_Animationer_Begge_Rettet_Samlet.zip (R:\Onedrive\AI) er kun brugt som skitse (pakket ud i Saved/Codex/gun_crew/ref/zip_20261105: kun beskrivelse, kontrol og preview-billedet, intet kørt): sekvens fremrulning - svabring - ladning/indstoedning - retning - skud, fem mand på en række. Videoerne og kildekoden er ikke åbnet.
- Første preview-kørsel blev ved en fejl startet mens GPU'en var optaget (94 %); resten blev kørt efter at GPU'en var under 30 %.
