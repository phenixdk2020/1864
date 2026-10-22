# Artilleri-props (kanonkugle, ladestok, visker)

Genereret af `Tools/Animation/build_artillery_props.py` (Blender 5.2, headless). Ikke importeret i Unreal.
Eksisterende filer overskrives ved genkoersel; tag backup foer en ny version.

| Prop | Trekanter | Tekstur (BaseColor + Roughness) | Pivot |
|---|---|---|---|
| SM_Cannonball_12pdr | 528 | 1024x1024 | kuglens centrum |
| SM_Rammer | 654 | 256x2048 | graebspunkt 66,7 cm fra den nedre (slidsede) ende |
| SM_Sponge | 1056 | 512x2048 | graebspunkt 66,7 cm fra den nedre (messingkugle-) ende |

## Maal (laest fra den eksporterede FBX, centimeter som Unreal ser dem)
- SM_Cannonball_12pdr: 11.7 x 11.7 x 11.7 cm (X x Y x Z); min (-5.8, -5.8, -5.8), max (5.8, 5.8, 5.8)
- SM_Rammer: 6.0 x 6.0 x 200.0 cm (X x Y x Z); min (-3.0, -3.0, -66.7), max (3.0, 3.0, 133.3)
- SM_Sponge: 14.9 x 14.9 x 200.0 cm (X x Y x Z); min (-7.4, -7.4, -66.7), max (7.4, 7.4, 133.3)
- FBX UnitScaleFactor: 1.0 (1 = cm).

## Enheder, akser, eksport
- I Blender er 1 enhed = 1 m. FBX eksporteres med `apply_unit_scale`, saa vaerdierne i filen er centimeter.
- FBX-akser: forward `-Y`, up `Z` (ingen ekstra rotation paa objektet). Ladestok og visker har laengdeaksen langs +Z i Blender
  og dermed ogsaa +Z i Unreal (staaende lodret). Hoved-enden (ladestokkens kappe / viskerens uldhoved) peger mod +Z.
- Alle transforms er applied, trekantet i mesh'en, kun mesh i FBX (ingen lys/kameraer/armatur).
- Glat shading med haard kant ved vinkler over 40 grader (ferrul-trin, endekapper, slids). FBX-smoothing: EDGE.

## Teksturer
- Kugle: sømløs 1024x1024. Midterste 720x720 px af fotoet (0,71 R, lav forvraengning) goeres tileable (offset-blanding),
  lys-gradient/spejling fjernes (hoejpas), og flisen lægges 4 x 2 gange paa UV-kuglen (u = 36,8 cm, v = 18,4 cm)
  saa kornstoerrelsen er ens i begge retninger. Roughness 0,7-0,95 (lavere paa blanke pletter). Polerne kan vise let
  sammentrækning af kornet. Metallic 0 (ingen glans ud over fotoet).
- Ladestok/visker: cylindrisk projektion af fotoets forside; forsiden spejles om omkredsen (u = 0,5 er fotoets midte),
  saa der er ingen sømme. Fotoets belysning er delvist bagt ind (mild kantkorrektion). Visker: en atlas, et materiale.
  Roughness efter materiale: uld ~0,96, messing ~0,4, trae ~0,72, mere variation fra fotoets lys.

## Estimater / antagelser (ikke verificerede kilder)
- Kugle 11,7 cm diameter: ESTIMAT for dansk 12-pds glatloebet kugle.
- Ladestok 200 cm lang og visker 200 cm lang: ESTIMAT. Fotoet ligner en tynd jernstang; fotoets proportioner er bevaret
  og skaleret til et kanonredskab (skaft ca. 3.4 cm).
- Visker: skaft 4.0 cm og hoved 14.0 cm er sat efter opgaven (12-pds kaliber ca. 12 cm + uld);
  fotoets skaft er forholdsmæssigt tykkere, saa hoved og skaft er skaleret hver for sig. Ulden er en glat cylinder (ikke tuft-geometri).
- Ladestokkens slids (ca. 0,9 cm bred, 8,5 cm dyb) er modelleret som snit gennem den nedre ende.
- Preview: `props_preview.png` (alle tre + 1 m maalestok), `props_detail.png` (kugle, viskerhoved, ladestokshoved).
