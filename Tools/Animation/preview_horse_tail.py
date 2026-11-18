"""Kontrolark af hestens hale: bagfra / fra siden / skraat bagfra, i hvile eller 3 frames af et klip (til foer/efter af fix_horse_tail.py).
blender -b -P Tools/Animation/preview_horse_tail.py -- <mesh.fbx> <klip.fbx|rest> <ud.png> [frames, komma-adskilt, 1-baseret; default 3 jaevnt fordelt]
Rendering med Workbench (CPU-venligt). Posering som i preview_horse_clip.py."""
import bpy, sys, math, os
from pathlib import Path
from mathutils import Vector as V
args = sys.argv[sys.argv.index('--') + 1:]
meshfbx, clip, out = args[0], args[1], args[2]
PROJECT = Path(__file__).resolve().parents[2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=meshfbx)
T = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
M = next(o for o in bpy.data.objects if o.type == 'MESH')
REST = clip == 'rest'
if not REST:
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
WA = {} if REST else {b.name: A.matrix_world @ b.matrix_local for b in A.data.bones}
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

if REST:
    fr = [1]
else:
    n = int(act.frame_range[1] - act.frame_range[0]) + 1
    s0 = int(act.frame_range[0])
    fr = [int(x) for x in args[3].split(',')] if len(args) > 3 else [s0 + int(round(k * n / 3.0)) % n for k in range(3)]
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
sc.render.resolution_x, sc.render.resolution_y = 360, 420
sh = sc.display.shading
sh.light, sh.color_type, sh.show_shadows = 'STUDIO', 'TEXTURE', True
sc.world = bpy.data.worlds.new('w'); sc.world.color = (0.82, 0.86, 0.9)
sc.view_settings.view_transform = 'Standard'
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
cam.data.ortho_scale = float(os.environ.get('CAM_SCALE', '1.35'))
CEN = V([float(x) for x in os.environ.get('CAM_CENTER', '0.80,0,0.62').split(',')])
views = {'bagfra': V((-1, 0, -0.05)), 'siden': V((0, 1, -0.05)), 'skraat': V((-1, 0.9, -0.25))}
tmp = Path(out).with_suffix('')
grid = []
for i, f in enumerate(fr):
    if not REST: pose_frame(f)
    row = []
    for vn, d in views.items():
        d = d.normalized()
        cam.location = CEN - d * 20
        cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
        fn = f'{tmp}_{i}_{vn}.png'
        sc.render.filepath = fn
        bpy.ops.render.render(write_still=True)
        row.append(fn + '::%s f=%d' % (vn, f))
    grid.append(row)
code = ("import sys\nfrom PIL import Image, ImageDraw\nout=sys.argv[1];fs=sys.argv[2:]\nncol=3\nims=[]\nfor a in fs:\n    f,l=a.split('::')\n    im=Image.open(f).convert('RGB')\n    ImageDraw.Draw(im).text((8,6),l,fill=(0,0,0))\n    ims.append(im)\n"
        "w,h=ims[0].size\nrows=(len(ims)+ncol-1)//ncol\ns=Image.new('RGB',(w*ncol,h*rows),(255,255,255))\n[s.paste(im,((i%ncol)*w,(i//ncol)*h)) for i,im in enumerate(ims)]\ns.save(out)\n")
tp = Path(out).with_name('_sheet_tmp.py'); tp.write_text(code)
import subprocess
flat = [x for row in grid for x in row]
subprocess.run(['python', '-I', str(tp), out] + flat, check=True)
for f in flat:
    os.remove(f.split('::')[0])
tp.unlink()
