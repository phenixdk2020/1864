"""Read-only saved-file validation. No rendering or UE calls."""
import bpy,json,tempfile
from pathlib import Path
from mathutils import Matrix
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Units1864/Officer';summary={}
for nat in ['DK','SE','NO','PR']:
 report=json.loads((O/'Final_v4'/f'{nat}_Officer_1864_v4/verification_v4.json').read_text(encoding='utf-8'))
 for label in ['March_Skulder','Ordre','Hvil']:
  bpy.ops.wm.open_mainfile(filepath=str(O/'Anim_v4'/f'{nat}_{label}_Final_v4.blend'));s=bpy.context.scene;r=next(o for o in s.objects if o.type=='ARMATURE')
  assert s.render.fps==30 and len(r.data.bones)==41
  assert len(report['night_finish_2026_10_30'][label]['frames'])==31
  maximum=0
  for frame in range(1,32):
   s.frame_set(frame);bpy.context.view_layer.update()
   for prop,bone,key in [('Saber_v4','mixamorig:RightHand','saber_socket'),('Scabbard_v4','mixamorig:Hips','scabbard_socket')]:
    expected=r.matrix_world@r.pose.bones[bone].matrix@Matrix(report[key]['matrix_blender_bone_local']);got=s.objects[prop].matrix_world
    maximum=max(maximum,max(abs(got[i][j]-expected[i][j]) for i in range(4) for j in range(4)))
  assert maximum<1e-4,(nat,label,maximum)
  summary[nat+'_'+label]={'socket_max_matrix_error':maximum,'bones':41,'fps':30,'frames':31}
  print('SAVED_FILE_OK',nat,label,maximum,flush=True)
(Path(tempfile.gettempdir())/'officer_finish_night/saved_validation.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
