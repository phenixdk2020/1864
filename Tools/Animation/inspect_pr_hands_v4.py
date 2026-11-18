import bpy
from mathutils import Vector
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath='C:/Projekter/1864-Campaign/SourceAssets/Units1864/Officer/PR_Officer_1864_v2.glb')
o=next(o for o in bpy.context.scene.objects if o.type=='MESH');vv=[o.matrix_world@v.co for v in o.data.vertices];low=min(v.z for v in vv);scale=1.8/(max(v.z for v in vv)-low);cx=(max(v.x for v in vv)+min(v.x for v in vv))/2;vv=[Vector(((v.x-cx)*scale,v.y*scale,(v.z-low)*scale)) for v in vv];w=max(abs(v.x) for v in vv)/.65
print('WIDTH',w)
for z in [.6,.7,.8,.9,1,1.1,1.2]:
 vs=[v for v in vv if abs(v.z-z)<.02 and v.x>.2];print(z,[(min(v[i] for v in vs),max(v[i] for v in vs)) for i in range(3)] if vs else [])
