import bpy,json
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True)
p=Path('C:/Projekter/1864-Campaign')
bpy.ops.import_scene.fbx(filepath=str(p/'SourceAssets/Units1864/DK_Infantry_1864.fbx'))
for o in bpy.context.scene.objects:
 if o.type=='ARMATURE':
  print('RIG',o.name,tuple(o.scale),len(o.data.bones));print('BONES',[(b.name,tuple(o.matrix_world@b.head_local),tuple(o.matrix_world@b.tail_local)) for b in o.data.bones if any(x in b.name for x in ['Hips','Head','Arm','Leg','Foot','Spine'])])
 if o.type=='MESH': print('SOLDIER',o.name,tuple(o.dimensions),tuple(o.location))
bpy.ops.import_scene.gltf(filepath=str(p/'SourceAssets/Units1864/Officer/DK_Officer_1864_v2.glb'))
for o in bpy.context.selected_objects:
 if o.type=='MESH':print('OFFICER',o.name,tuple(o.dimensions),tuple(o.rotation_euler),[(min((o.matrix_world@v.co)[i] for v in o.data.vertices),max((o.matrix_world@v.co)[i] for v in o.data.vertices)) for i in range(3)])
