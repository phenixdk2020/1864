import bpy,json
from pathlib import Path
R=Path('C:/Projekter/1864-Campaign')
for path in ['Officer/DK_Officer_1864_v3/DK_Officer_1864_v3.fbx','Saber.fbx','Scabbard.fbx']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/Units1864'/path))
 for o in bpy.context.scene.objects:
  if o.type=='ARMATURE':
   print('RIG',o.name,list(map(list,o.matrix_world)))
   for b in o.data.bones:
    if any(s in b.name for s in ['Arm','Hand','Shoulder','Hips','Leg']): print('BONE',b.name,list(o.matrix_world@b.head_local),list(o.matrix_world@b.tail_local),list((o.matrix_world@b.matrix_local).to_quaternion()))
  if o.type=='MESH':
   vs=[o.matrix_world@v.co for v in o.data.vertices]; print('MESH',path,o.name,len(vs),'bounds',[[min(v[i] for v in vs),max(v[i] for v in vs)] for i in range(3)])
   if 'Officer' in path:
    for z in [1.0,1.1,1.2,1.3,1.4]:
     vv=[v for v in vs if abs(v.z-z)<.015 and v.x<-.15]; print('SLICE',z,[[min(v[i] for v in vv),max(v[i] for v in vv)] for i in range(3)] if vv else [])
