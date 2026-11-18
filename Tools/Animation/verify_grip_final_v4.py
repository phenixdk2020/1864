"""Read-only Blender round-trip and rigid socket checks, no Unreal launch."""
import bpy,json,math,tempfile
from pathlib import Path
from mathutils import Matrix
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Units1864/Officer';F=O/'Final_v4'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/Units1864/DK_Infantry_1864.fbx'))
r=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
ref={b.name:(b.matrix_local.copy(),b.parent.name if b.parent else None) for b in r.data.bones}
summary={}
for nat in ['DK','SE','NO','PR']:
 dst=F/f'{nat}_Officer_1864_v4';report=json.loads((dst/'verification_v4.json').read_text(encoding='utf-8'))
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(dst/f'{nat}_Officer_1864_v4.fbx'))
 rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
 assert len(rig.data.bones)==41
 error=max(abs(b.matrix_local[i][j]-ref[b.name][0][i][j]) for b in rig.data.bones for i in range(4) for j in range(4))
 assert error<.001,(nat,error)
 assert all((b.parent.name if b.parent else None)==ref[b.name][1] for b in rig.data.bones)
 assert all(0<len(v.groups)<=4 and abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in mesh.data.vertices)
 mesh.data.calc_loop_triangles()
 summary[nat]=dict(bones=41,triangles=len(mesh.data.loop_triangles),rest_matrix_max_error=error,max_influences=max(len(v.groups) for v in mesh.data.vertices),weights_normalised=True)
 for label in ['March_Skulder','Ordre','Hvil']:
  pose=report['poses'][label];assert pose['grip_to_palm_cm']<1.5
  for measurement in pose['frames']:
   for key in ['blade','whole_saber_including_hilt','scabbard']:
    assert measurement[key]['triangle_intersections']==0,(nat,label,key,measurement)
    assert measurement[key]['max_penetration_cm']==0,(nat,label,key,measurement)
  bpy.ops.wm.open_mainfile(filepath=str(O/f'Anim_v4/{nat}_{label}_Final_v4.blend'))
  scene=bpy.context.scene;rig=next(o for o in scene.objects if o.type=='ARMATURE');assert scene.render.fps==30
  for frame in range(1,32):
   scene.frame_set(frame);bpy.context.view_layer.update()
   for name,bone,key in [('Saber_v4','mixamorig:RightHand','saber_socket'),('Scabbard_v4','mixamorig:Hips','scabbard_socket')]:
    expected=rig.matrix_world@rig.pose.bones[bone].matrix@Matrix(report[key]['matrix_blender_bone_local'])
    got=scene.objects[name].matrix_world
    assert max(abs(got[i][j]-expected[i][j]) for i in range(4) for j in range(4))<1e-4
 print('ROUNDTRIP_OK',nat,'41 bones, unchanged rest/parents, normalised weights <=4, fixed sockets all 93 pose frames; rest error',error,flush=True)
(Path(tempfile.gettempdir())/'officer_grip_v4/roundtrip_summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
