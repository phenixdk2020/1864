"""Horse rig inspection: blender -b <blend> -P inspect_horse_rig_b1.py"""
import bpy
rig=bpy.data.objects['Horse_1_Armature_v2']
for n in ('head','tail_03','pelvis','chest','saddle_root','harness_root','bridle_root','root','front_hoof.L'):
    b=rig.data.bones[n]; print('BONE',n,'deform',b.use_deform,[round(v,3) for v in b.head_local],'parent',b.parent.name if b.parent else None)
m=bpy.data.objects['Horse_1_Mesh']
import mathutils
zs=[(m.matrix_world@v.co).z for v in m.data.vertices]; xs=[(m.matrix_world@v.co).x for v in m.data.vertices]
print('MESHZ',min(zs),max(zs),'X',min(xs),max(xs), 'mats',[s.material.name if s.material else None for s in m.material_slots], 'uv',[u.name for u in m.data.uv_layers])
