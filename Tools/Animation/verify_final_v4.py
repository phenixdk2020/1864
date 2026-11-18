import bpy,json,hashlib
from pathlib import Path
R=Path('C:/Projekter/1864-Campaign');O=R/'SourceAssets/Units1864/Officer';D=O/'Final_v4'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/Units1864/DK_Infantry_1864.fbx'))
ref=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');bones={b.name:(b.matrix_local.copy(),b.parent.name if b.parent else None) for b in ref.data.bones}
reports=[]
for nat in ['DK','SE','NO','PR']:
 bpy.ops.wm.read_factory_settings(use_empty=True);p=D/f'{nat}_Officer_1864_v4';bpy.ops.import_scene.fbx(filepath=str(p/f'{nat}_Officer_1864_v4.fbx'))
 rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
 err=max(abs(b.matrix_local[i][j]-bones[b.name][0][i][j]) for b in rig.data.bones for i in range(4) for j in range(4))
 assert len(rig.data.bones)==41 and set(bones)==set(b.name for b in rig.data.bones)
 assert all((b.parent.name if b.parent else None)==bones[b.name][1] for b in rig.data.bones)
 assert err<.001
 assert all(0<len(v.groups)<=4 and abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in mesh.data.vertices)
 report=dict(nation=nat,bones=41,rest_matrix_max_error=err,weights_normalised=True,max_influences=4,unweighted=0)
 for label in ['March_Skulder','Ordre','Hvil']:
  bpy.ops.wm.open_mainfile(filepath=str(O/f'Anim_v4/{nat}_{label}_Final_v4.blend'))
  sc=bpy.context.scene;rig=next(o for o in sc.objects if o.type=='ARMATURE');assert sc.render.fps==30
  metrics=json.loads((p/'metrics_v4.json').read_text());from mathutils import Matrix
  for frame in [1,8,16,24,31]:
   sc.frame_set(frame);bpy.context.view_layer.update()
   for name,bone,key in [('Saber_v4','mixamorig:RightHand','saber'),('Scabbard_v4','mixamorig:Hips','scabbard')]:
    expected=rig.matrix_world@rig.pose.bones[bone].matrix@Matrix(metrics[key]['matrix_blender_bone_local'])
    got=sc.objects[name].matrix_world
    assert max(abs(got[i][j]-expected[i][j]) for i in range(4) for j in range(4))<1e-4
 report['props_checked_poses']=3;report['frames_per_pose']=5
 (p/'verification_v4.json').write_text(json.dumps(report,indent=2),encoding='utf-8');reports.append(report)
print('VERIFIED',json.dumps(reports))
