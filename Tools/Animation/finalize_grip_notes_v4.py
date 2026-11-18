"""Write final Danish notes from the measured result and read-only FBX checks."""
import json,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[2];F=R/'SourceAssets/Units1864/Officer/Final_v4'
summary=json.loads((Path(tempfile.gettempdir())/'officer_grip_v4/roundtrip_summary.json').read_text(encoding='utf-8'))
for nat in ['DK','SE','NO','PR']:
 dst=F/f'{nat}_Officer_1864_v4';p=dst/'verification_v4.json';d=json.loads(p.read_text(encoding='utf-8'))
 for key in ['saber_socket','scabbard_socket']:
  socket=d[key];socket['source_bone']=socket.get('source_bone',socket['bone']);socket['bone']=socket['source_bone'].rsplit(':',1)[-1]
 d['roundtrip_static']=summary[nat]
 d['palm_landmark']='Uafhængige ellipsefit til fæstets vertices og den evaluerede inderside af fingrene. Socket-anchor-fejl er en separat kontrol.'
 d['remaining_problems']=['Håndens modellering og hudmateriale er stiliseret; ikke anatomisk færdig til nærfilm.', 'Unreal-import og runtime er ikke testet.', 'Nationalt eksercits er ikke historisk verificeret.']
 d['unreal_conversion_source']='UE 5.8 FbxUtilsImport.cpp ConvertPos/ConvertRotToQuat; FbxMainImport.cpp MakeName fjerner namespace.'
 p.write_text(json.dumps(d,indent=2,ensure_ascii=False),encoding='utf-8')
 metrics=json.loads((dst/'metrics_v4.json').read_text(encoding='utf-8'));metrics.update(summary[nat]);metrics['saber']=d['saber_socket'];metrics['scabbard']=d['scabbard_socket'];metrics['grasp_geometry_replaced']=True
 (dst/'metrics_v4.json').write_text(json.dumps(metrics,indent=2,ensure_ascii=False),encoding='utf-8')
 text=f'''# {nat} officer v4 — sabelgreb, 2026-10-30

Fire krummede fingre og tommelfinger erstatter den sammenvoksede højre vante. Manchetten er tilpasset håndleddet i bindeposen. Håndgeometrien følger `mixamorig:RightHand`. To læderremme forbinder venstre gehæng med skeden og følger `mixamorig:Hips`. Samme faste socket anvendes i alle tre poser; ingen offsets pr. pose. Rekvisittens geometri og origin er uændret.

Poserne er rettet med toleddet IK og faste knoglelængder. 30 fps. March er en 1-sekunds in-place testcyklus, rodbevægelse 0 cm og hastighed 0 cm/s. Ikke en produktionsgang med sikret fodkontakt. Ordre og Hvil er holdte poser. Der er {summary[nat]['triangles']} trekanter. FBX-genindlæsning bekræfter 41 knogler med uændrede navne, forældre og bindematricer; normaliserede vægte, højst fire pr. vertex.

## Faste Unreal-sockets

Location er X/Y/Z i cm; rotation er Pitch/Yaw/Roll i grader. Værdierne gælder de eksisterende separate Props_v4-FBX'er, som bruger cm efter import. Socket Scale skal også anvendes; den kompenserer riggens eksisterende skala. Rekvisittens relative transform under socket sættes til identitet.

'''
 for key,title in [('saber_socket','Sabel'),('scabbard_socket','Skede')]:
  s=d[key];loc=', '.join(f'{v:.6f}' for v in s['location_cm']);rot=', '.join(f'{s["rotation_degrees"][axis]:.6f}' for axis in ['pitch','yaw','roll']);scale=', '.join(f'{v:.6f}' for v in s['scale'])
  text+=f'- **{title}** — Unreal-knogle `{s["bone"]}` (Blender: `{s["source_bone"]}`). Location: `({loc})`. Rotation P/Y/R: `({rot})`. Scale: `({scale})`.\n'
 text+='''
FBX eksporteres med -Z frem/Y op og bevarede knogleakser. Konverteringen er afledt af UE 5.8 `FFbxDataConverter::ConvertPos`/`ConvertRotToQuat` og `FFbxImporter::MakeName`, som fjerner namespace. Forudsætter standard Convert Scene/Convert Scene Unit uden ekstra importrotation eller ændret rodskala. Ingen Unreal-import er udført; værdierne er matematisk konverteret og kræver kontrol ved den senere import. Blender-matricer og quaternioner findes i verification_v4.json.

## Geometrisk kontrol

Grebets og håndfladens centre er målt med separate ellipsefit til rekvisittens vertices og den evaluerede inderside af fingrene i gribezonen. Metode, sampleantal og målte centre findes pr. pose i verification_v4.json. Den næsten nul socket-anchor-fejl er en separat fastgørelseskontrol og erstatter ikke håndmålingen.

Lineær skinning er evalueret på alle 31 marchframes og på frame 1 og 31 i de holdte poser. Hele sablen inklusive fæste, bøjle og knop er kontrolleret mod hånd, håndled og krop. Bladet rapporteres også separat. Kontrollen bruger BVH-trekantintersektion og flervejs ray-paritet på rekvisitvertices. Kildemesh indeholder åbne områder, så overfladeintersektioner rapporteres særskilt. Tallene nedenfor er maksimum over de målte frames, i cm.

| Pose | Greb/håndflade | Blad ind i krop | Hele sabel ind i krop/hånd | Skede ind i krop |
|---|---:|---:|---:|---:|
'''
 for label,v in d['poses'].items():
  whole=max(f['whole_saber_including_hilt']['max_penetration_cm'] for f in v['frames'])
  text+=f'| {label} | {v["grip_to_palm_cm"]:.5f} | {v["blade_penetration_cm"]:.5f} | {whole:.5f} | {v["scabbard_penetration_cm"]:.5f} |\n'
 march=d['poses']['March_Skulder']['frames'][0]['blade']['minimum_surface_clearance_cm'];rest=d['poses']['Hvil']['frames'][0]['tip_ground_clearance_cm']
 text+=f'\nMarch: bladets ryg ligger ved højre skulder med målt mindste overfladeafstand {march:.5f} cm. Ordre: fremad/opad. Hvil: spidsens nederste geometri ligger {rest:.5f} cm over jordplanet ved højre fod. Skeden hænger med mundingen frem/op og spidsen bagud/ned.\n\n'
 text+='Preview: `preview_v4.png` viser tre poser. `sabre_closeup_v4.png` viser hver højrehånd med modsat side i et indsat billede samt skeden ved venstre hofte.\n\nResterende begrænsninger: '+' '.join(d['remaining_problems'])+' Kildens uniforms-/ansigtstekstur er bevaret. Ingen build, spilstart, .bat eller commit.\n'
 (dst/'notes.md').write_text(text,encoding='utf-8')
 print('FINAL_NOTES',nat,flush=True)
