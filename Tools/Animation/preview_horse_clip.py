"""Kontrolark af et eksisterende hesteklip paa Horse_1_rigged_v3-meshet (til sammenligning med M2M-klippene).
blender -b -P Tools/Animation/preview_horse_clip.py -- <klip.fbx> <ud.png> [frames, komma-adskilt, 1-baseret; default 6 jaevnt fordelt]"""
import bpy, sys, math, os
from pathlib import Path
from mathutils import Vector as V
args = sys.argv[sys.argv.index('--') + 1:]
clip, out = args[0], args[1]
PROJECT = Path(__file__).resolve().parents[2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(PROJECT / 'SourceAssets/Horse/Horse_1_rigged_v3.fbx'))
T = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
M = next(o for o in bpy.data.objects if o.type == 'MESH')
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=clip)
new = [o for o in bpy.data.objects if o not in before]
A = next(o for o in new if o.type == 'ARMATURE')
act = A.animation_data.action
try:
    A.animation_data.action_slot = act.slots[0]
except Exception:
    pass
# Klippets FBX har andre knogle-roll end meshets rig (HORSE_CLIP_ROLL): en direkte action-overfoersel ville forvride legemet.
# Derfor kopieres verdens-rotationsdelta (pose mod hvile) + hovedposition knogle for knogle (foraeldre foerst), ligesom i build_mesh2motion_clips.py.
bpy.context.view_layer.update()
WA = {b.name: A.matrix_world @ b.matrix_local for b in A.data.bones}
WT = {b.name: T.matrix_world @ b.matrix_local for b in T.data.bones}
def _depth(pb):
    n = 0
    while pb.parent:
        pb = pb.parent; n += 1
    return n
order = sorted(T.pose.bones, key=_depth)
for pb in T.pose.bones:
    pb.rotation_mode = 'QUATERNION'
def pose_frame(f):
    """Knoglens retning (hoved->hale) og hovedposition fra klippets armatur, vridning = meshets hvilevridning (roll/akse kan afvige efter FBX-genimport)."""
    from mathutils import Matrix
    bpy.context.scene.frame_set(f)
    cur = {pb.name: (A.matrix_world @ pb.head, A.matrix_world @ pb.tail) for pb in A.pose.bones}
    for pb in order:
        n = pb.name
        if n not in cur:
            continue
        h, t = cur[n]
        Rt = WT[n].to_3x3()
        R = (Rt @ V((0, 1, 0))).rotation_difference((t - h).normalized()).to_matrix() @ Rt
        pb.matrix = T.matrix_world.inverted() @ (Matrix.Translation(h) @ R.to_4x4())
        bpy.context.view_layer.update()
n = int(act.frame_range[1] - act.frame_range[0]) + 1
s0 = int(act.frame_range[0])
fr = [int(x) for x in args[2].split(',')] if len(args) > 2 else [s0 + int(round(k * n / 6.0)) % n for k in range(6)]
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
sc.render.resolution_x, sc.render.resolution_y = 420, 340
sh = sc.display.shading
sh.light, sh.color_type, sh.show_shadows = 'STUDIO', 'TEXTURE', True
sc.world = bpy.data.worlds.new('w'); sc.world.color = (0.82, 0.86, 0.9)
sc.view_settings.view_transform = 'Standard'
HIDE = [x for x in os.environ.get('HIDE_GROUPS', '').split(',') if x]
if HIDE:   # diagnose: skjul vertices hvis stoerste vaegt ligger paa disse knogler (HIDE_GROUPS=tail_01,tail_02,...)
    import bmesh
    names = [g.name for g in M.vertex_groups]
    bm = bmesh.new(); bm.from_mesh(M.data); dl = bm.verts.layers.deform.active
    kill = [v for v in bm.verts if v[dl] and names[max(v[dl].items(), key=lambda kv: kv[1])[0]] in HIDE]
    bmesh.ops.delete(bm, geom=kill, context='VERTS'); bm.to_mesh(M.data); bm.free()
tex = PROJECT / 'SourceAssets/Units1864/Horse_Texture_0.png'
m = bpy.data.materials.new('m'); m.use_nodes = True
tn = m.node_tree.nodes.new('ShaderNodeTexImage'); tn.image = bpy.data.images.load(str(tex))
m.node_tree.links.new(tn.outputs['Color'], m.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
M.data.materials.clear(); M.data.materials.append(m)
bpy.ops.mesh.primitive_cube_add(size=1)
g = bpy.context.object; g.scale = (30, 30, 0.02); g.location = (0, 0, -0.01)
gm = bpy.data.materials.new('g'); gm.diffuse_color = (0.38, 0.46, 0.3, 1); g.data.materials.append(gm)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam')); cam.data.type = 'ORTHO'; cam.data.clip_end = 100
sc.collection.objects.link(cam); sc.camera = cam
d = V((0, 1, -0.05)).normalized()
cam.location = V((-0.1, 0, 0.8)) - d * 20
cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler(); cam.data.ortho_scale = 2.6
tmp = Path(out).with_suffix('')
files = []
for i, f in enumerate(fr):
    pose_frame(f)
    fn = f'{tmp}_{i}.png'
    sc.render.filepath = fn
    bpy.ops.render.render(write_still=True)
    files.append(fn + '::f=%d' % f)
code = ("import sys\nfrom PIL import Image, ImageDraw\nout=sys.argv[1];fs=sys.argv[2:]\nims=[]\nfor a in fs:\n    f,l=a.split('::')\n    im=Image.open(f).convert('RGB')\n    ImageDraw.Draw(im).text((8,6),l,fill=(0,0,0))\n    ims.append(im)\n"
        "w,h=ims[0].size\ns=Image.new('RGB',(w*3,h*2),(255,255,255))\n[s.paste(im,((i%3)*w,(i//3)*h)) for i,im in enumerate(ims)]\ns.save(out)\n")
tp = Path(out).with_name('_sheet_tmp.py'); tp.write_text(code)
import subprocess
subprocess.run(['python', '-I', str(tp), out] + files, check=True)
for f in files:
    os.remove(f.split('::')[0])
tp.unlink()
