"""Read-only FBX roundtrip QA, writes a new verification_v3.json per nation."""
import bpy,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'SourceAssets/Units1864/Officer'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'SourceAssets/Units1864/DK_Infantry_1864.fbx'))
ref=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
signature={b.name:(b.parent.name if b.parent else None,[list(row) for row in b.matrix_local]) for b in ref.data.bones}
for nat in ('DK','SE','NO','PR'):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    folder=BASE/f'{nat}_Officer_1864_v3'
    target=folder/'verification_v3.json'
    assert not target.exists()
    bpy.ops.import_scene.fbx(filepath=str(folder/f'{nat}_Officer_1864_v3.fbx'))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    mesh.data.calc_loop_triangles()
    coords=[mesh.matrix_world@v.co for v in mesh.data.vertices]
    lo=min(v.z for v in coords); hi=max(v.z for v in coords)
    bone_error=max(abs(b.matrix_local[i][j]-signature[b.name][1][i][j]) for b in rig.data.bones for i in range(4) for j in range(4))
    parents=all((b.parent.name if b.parent else None)==signature[b.name][0] for b in rig.data.bones)
    unweighted=sum(not v.groups for v in mesh.data.vertices)
    texture=folder/f'{nat}_Officer_1864_Texture_0_v3.png'
    same_texture=hashlib.sha256(texture.read_bytes()).digest()==hashlib.sha256((BASE/f'{nat}_Officer_1864_Texture_0_v2.png').read_bytes()).digest()
    result=dict(nation=nat,bones=len(rig.data.bones),same_names=set(signature)==set(rig.data.bones.keys()),same_parents=parents,
                rest_matrix_max_error=bone_error,triangles=len(mesh.data.loop_triangles),uv_layers=len(mesh.data.uv_layers),
                z_min=lo,height_m=hi-lo,unweighted_vertices=unweighted,texture_identical=same_texture,
                armature_modifier=any(m.type=='ARMATURE' and m.object==rig for m in mesh.modifiers))
    assert result['same_names'] and parents and len(rig.data.bones)==41 and bone_error<.001
    assert 15000<=result['triangles']<=20000 and unweighted==0 and same_texture
    assert abs(lo)<.001 and abs(hi-lo-1.8)<.001
    target.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('VERIFIED',json.dumps(result),flush=True)
