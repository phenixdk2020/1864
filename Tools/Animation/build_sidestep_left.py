"""Mirror Mixamo 'Rifle Side Step' (right) -> A_Combat_SideStepLeft.fbx with skinned mesh (Walking.fbx bind pose), v9 exporter settings.
    blender -b -P build_sidestep_left.py -- <out dir>
Mirror = armature-space delta (pose * rest^-1) reflected across the sagittal plane (armature X), applied to the paired Left/Right bone."""
import bpy, glob, json, math, sys
from pathlib import Path
from mathutils import Matrix, Vector
a = sys.argv[sys.argv.index('--') + 1:]
OUT = Path(a[0]); OUT.mkdir(parents=True, exist_ok=True)
MULTI = Path(r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView')
STEP = glob.glob(str(MULTI / 'Animationer M*' / 'Rifle Side Step.fbx'))[0]
WALK = MULTI / '3D figurer' / 'Walking.fbx'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(WALK))
armW = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=STEP)
armS = next(o for o in bpy.data.objects if o.type == 'ARMATURE' and o not in before)
sc = bpy.context.scene; sc.render.fps = 30
act = armS.animation_data.action
try: armS.animation_data.action_slot = act.slots[0]
except Exception: pass
f0, f1 = int(act.frame_range[0]), int(act.frame_range[1]); N = f1 - f0 + 1
print('SS frames', f0, f1, 'W obj scale', tuple(armW.scale), tuple(armW.rotation_euler), 'S', tuple(armS.scale), tuple(armS.rotation_euler))
# rest compatibility
mx = 0
print('MISSING_IN_WALK', [b.name for b in armS.data.bones if b.name not in armW.data.bones])
for b in armS.data.bones:
    if b.name not in armW.data.bones: continue
    mx = max(mx, max(abs(x - y) for r1, r2 in zip(b.matrix_local, armW.data.bones[b.name].matrix_local) for x, y in zip(r1, r2)))
print('REST_DIFF_MAX', mx)
armW.animation_data_clear()   # the Walking action would otherwise re-evaluate over the poses
names = [b.name for b in armW.data.bones]
def pair(n):
    if 'Left' in n: return n.replace('Left', 'Right')
    if 'Right' in n: return n.replace('Right', 'Left')
    return n
Sm = Matrix.Diagonal((-1, 1, 1, 1))
R = {b.name: b.matrix_local.copy() for b in armW.data.bones}
RS = {b.name: b.matrix_local.copy() for b in armS.data.bones}
# Walking.fbx (the skinned bind pose) is arms-down, the Mixamo clip's rest is a T-pose, so local keys do not transfer. Retarget by ABSOLUTE bone
# orientation: the walking bone gets the clip's world bone direction; the per-bone rest roll offset (about the bone axis) is kept.
ROLL = {}
for n in names:
    ys, yw = RS[n].to_3x3().col[1].normalized(), R[n].to_3x3().col[1].normalized()
    fs = ys.rotation_difference(yw).to_matrix() @ RS[n].to_3x3()
    xs, xw = fs.col[0], R[n].to_3x3().col[0]
    ROLL[n] = Matrix.Rotation(math.atan2(yw.dot(xs.cross(xw)), xs.dot(xw)), 3, 'Y')
parent = {b.name: (b.parent.name if b.parent else None) for b in armW.data.bones}
hips_n = 'mixamorig:Hips'
H0 = None
orig, mirr = [], []
order = names
for pb in armW.pose.bones: pb.rotation_mode = 'QUATERNION'
def put(poses_arm):
    """poses_arm: name -> 3x3 armature-space orientation (+ hips location); set bones parent-first."""
    P = {}
    for n in order:
        par = parent[n]
        if par is None: t = poses_arm[n][1]
        else: t = (P[par] @ R[par].inverted() @ R[n]).translation
        M = poses_arm[n][0].to_4x4(); M.translation = t
        armW.pose.bones[n].matrix = M; bpy.context.view_layer.update()
        P[n] = armW.pose.bones[n].matrix.copy()
    return P
for f in range(f0, f1 + 1):
    sc.frame_set(f)
    PS = {pb.name: pb.matrix.copy() for pb in armS.pose.bones}
    hs = armS.pose.bones[hips_n].head.copy()
    if H0 is None: H0 = hs.copy()
    pose = {n: (PS[n].to_3x3() @ ROLL[n], None) for n in names}
    d_h = hs - armS.data.bones[hips_n].head_local
    d_h0 = H0 - armS.data.bones[hips_n].head_local
    d_h.x, d_h.z = d_h0.x, d_h0.z   # root lock: hips X/Z held at frame 0 (clip is in place), vertical bob kept
    pose[hips_n] = (pose[hips_n][0], R[hips_n].translation + d_h)
    PW = put(pose)
    orig.append({n: armW.pose.bones[n].matrix_basis.copy() for n in names})
    delta = {n: PW[n] @ R[n].inverted() for n in names}
    tgt = {n: Sm @ delta[pair(n)] @ Sm @ R[n] for n in names}
    mp = {n: (tgt[n].to_3x3(), tgt[n].translation) for n in names}
    put(mp)
    mirr.append({n: armW.pose.bones[n].matrix_basis.copy() for n in names})
# source clip's own feet/hips (the truth) for the report
src = []
for f in range(f0, f1 + 1):
    sc.frame_set(f)
    g = lambda n: armS.pose.bones['mixamorig:' + n].head * armS.scale[0]
    src.append({'hips': tuple(g('Hips')), 'LF': tuple(g('LeftFoot')), 'RF': tuple(g('RightFoot'))})
json.dump(src, open(OUT / 'metrics_source.json', 'w'))

def setpose(poses):
    for n in names: armW.pose.bones[n].matrix_basis = poses[n]
    bpy.context.view_layer.update()
def bake(poses_list, name):
    armW.animation_data_clear(); armW.animation_data_create()
    action = bpy.data.actions.new(name); armW.animation_data.action = action
    try: armW.animation_data.action_slot = action.slots[0] if action.slots else action.slots.new('OBJECT', armW.name)
    except Exception: pass
    for i, poses in enumerate(poses_list, 1):
        for pb in armW.pose.bones:
            pb.matrix_basis = poses[pb.name]
            pb.keyframe_insert('location', frame=i); pb.keyframe_insert('rotation_quaternion', frame=i); pb.keyframe_insert('scale', frame=i)
    sc.frame_start, sc.frame_end = 1, len(poses_list)
def export(path):
    for o in bpy.data.objects: o.select_set(False)
    armW.select_set(True)
    for o in bpy.data.objects:
        if o.type == 'MESH' and o.parent == armW: o.select_set(True)
    bpy.context.view_layer.objects.active = armW
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
                             use_mesh_modifiers=False, bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                             bake_anim_simplify_factor=0, bake_anim_force_startend_keying=True, axis_forward='-Z', axis_up='Y')
# metrics: armature-space coordinates in metres (object scale .01), lateral = armature X, character's RIGHT = -X
def metrics(plist):
    rows = []
    for poses in plist:
        setpose(poses)
        g = lambda n: armW.pose.bones['mixamorig:' + n].head * armW.scale[0]
        rows.append({'hips': tuple(g('Hips')), 'LF': tuple(g('LeftFoot')), 'RF': tuple(g('RightFoot')),
                     'LH': tuple(g('LeftHand')), 'RH': tuple(g('RightHand')), 'LT': tuple(g('LeftToeBase')), 'RT': tuple(g('RightToeBase'))})
    return rows
mo, mm = metrics(orig), metrics(mirr)
json.dump({'orig': mo, 'mirr': mm}, open(OUT / 'metrics.json', 'w'))
# renders: front view, workbench
sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'SINGLE'; sc.display.shading.single_color = (0.7, 0.75, 0.8)
sc.render.resolution_x, sc.render.resolution_y = 320, 480
sc.render.film_transparent = False
w = bpy.data.worlds.new('w'); w.color = (0.9, 0.9, 0.9); sc.world = w
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam')); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.2
cam.location = (0, -5, 0.9); cam.rotation_euler = (math.radians(90), 0, 0)   # looks along +Y; soldier faces -Y -> sees his front
frames = [0, 7, 14, 21, 28, 35]
for tag, pl in (('orig', orig), ('mirr', mirr)):
    for k in frames:
        setpose(pl[k]); sc.render.filepath = str(OUT / ('r_%s_%02d.png' % (tag, k))); bpy.ops.render.render(write_still=True)
bake(mirr, 'A_Combat_SideStepLeft'); export(OUT / 'A_Combat_SideStepLeft.fbx')
print('DONE')
