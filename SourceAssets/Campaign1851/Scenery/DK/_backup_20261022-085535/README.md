# Danske bygninger til kampagnekortet (lavpoly)

Genereres af `Tools/Map1851/build_scenery_models.py` (Blender 5.2, headless). Ikke importeret i Unreal.
Eksisterende output kopieres til `_backup_<tid>/` ved genkørsel.

## Filer
- `SM_<Type>.fbx` (8 stk.), `scenery_models.blend` (alle otte side om side), `T_SceneryDK_Atlas.png` (2048x2048),
  `scenery_models_preview.png`, `scenery_models_closeup.png`.
- Enheder: 1 Blender-enhed = 1 m. FBX: vertices x100, UnitScaleFactor 1 (= cm). Pivot ved jordcentrum, +X = facade, Z op.
  Akser eksporteret som i projektets øvrige props (forward -Y, up Z).

## Trekanter og størrelser (bounding box i FBX, cm, X x Y x Z)
| Model | Tris | Budget | Bbox cm |
|---|---|---|---|
| SM_TownHouse_Ochre | 124 | 150-400 | 910 x 1170 x 1114 |
| SM_TownHouse_White | 102 | 150-400 | 860 x 1270 x 844 |
| SM_TownHouse_Timber | 142 | 150-400 | 860 x 1270 x 1054 |
| SM_MerchantHouse | 183 | <= 600 | 950 x 1270 x 1354 |
| SM_Cottage | 106 | 150-400 | 745 x 1090 x 704 |
| SM_Farm | 467 | <= 900 | 3460 x 3620 x 804 |
| SM_Church | 278 | <= 700 | 1060 x 3042 x 2240 |
| SM_Windmill | 358 | <= 600 | 1133 x 1431 x 1896 |

Bbox er ikke symmetrisk om 0 i X, fordi dørtrin og sokkel stikker ud på facaden (+X).

## Atlas (4 x 4 felter à 512 px, række fra toppen)
1. thatch, tile (teglrød), ochre, white (støvet creme, sokkel nederst i alle vægfelter)
2. timber_ochre, timber_white (2 fag bredt, repeteres pr. ca. 3 m), brick, window (venstre halvdel sprosset rude, højre halvdel rundbuet kirkerude)
3. door_green (med overlys), door_ox, gateway (portbue, ochre-hjørner), tar (tjæret bræddeklædning)
4. sail (gitter + sejldug), cobbles, grass (kun brugt i preview-reservefelt), wood_stone (øvre halvdel træ, nedre halvdel markstensmur)

Vinduer og døre er decal-quads 3-9 cm uden for væggen (ingen huller). Tagfladerne beskærer feltet efter længde i stedet for at gentage.

## Skønnet / afvigelser
- Alle mål er skøn efter referencebillederne og researchdokumentet, ikke opmålte bygninger.
- Tykkelse på tage er kun antydet med fascia-strimler; undersider af tagudhæng mangler (ses ikke oppefra).
- Gården: hjørnerne er symmetriske geringer (alle fløje 7,5 m brede); brønd og møddingen er enkle former.
- Møllens vingegitter er malet ugennemsigtigt (ingen alfa) for at undgå masked materiale i Unreal.
- Kirkens kamtakker er 5 trin pr. side som simpel pladeprofil; tårnets faste vinduer sidder på alle sider.
- Skygger i preview er Eevee-lys; materialet i Unreal skal blot bruge atlasset som BaseColor.
- Tråde-/halmstrukturen på stråtag er upræcis (strithed ses som bånd på de glatte flader).
