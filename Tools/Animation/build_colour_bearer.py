"""Fanebaerer (colour bearer), 2026-10: A_Combat_ColourWalk.fbx + A_Combat_ColourIdle.fbx from the Mixamo Walking.fbx (skeleton + mesh + bind pose), same method as build_shoulder_arms_set_v9.py.
    blender -b -P build_colour_bearer.py -- [<out dir>]
The RIGHT hand is pinned (two-bone IK, target stored in the Spine2 frame of frame 0) at chest height: HAND_FWD m in front of / HAND_DOWN m below the right shoulder joint, HAND_OUT m outwards (negative = inwards).
Walk: legs/torso/left arm = the Mixamo walk (hips X/Z held). Idle: legs/hips = bind pose (straight, feet planted), torso/head/left arm = the walk frame with the left arm closest to the body, plus a slow breathing sway; 60 frames = 2 s, loops.
Env: HAND_FWD(0.22) HAND_DOWN(0.26) HAND_OUT(-0.03) POLE=right,fwd,up elbow hint from the shoulder, POLE_ANGLE.
"""
import bpy, json, math, os, sys
from pathlib import Path
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
WALK = Path(r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer\Walking.fbx')
OUT = Path(a[0]) if a else PROJECT / 'Saved/Codex/colour_bearer'
OUT.mkdir(parents=True, exist_ok=True)
HAND_FWD = float(os.environ.get('HAND_FWD', '0.22'))
HAND_DOWN = float(os.environ.get('HAND_DOWN', '0.26'))
HAND_OUT = float(os.environ.get('HAND_OUT', '-0.03'))
POLE = [float(x) for x in os.environ.get('POLE', '0.12,-0.25,-0.45').split(',')]
POLE_ANGLE = float(os.environ.get('POLE_ANGLE', '180'))
PIN_BONE = 'Spine2'
ARM_BONES = ['Arm', 'ForeArm', 'Hand'] + ['Hand%s%d' % (f, i) for f in ('Index', 'Middle', 'Ring', 'Pinky', 'Thumb') for i in (1, 2, 3, 4)]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(WALK))
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
act = arm.animation_data.action
try:
    arm.animation_data.action_slot = act.slots[0]
except Exception:
    pass
sc = bpy.context.scene
sc.render.fps = 30
f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
N = f1 - f0 + 1
for pb in arm.pose.bones:
    pb.rotation_mode = 'QUATERNION'
hips = arm.pose.bones['mixamorig:Hips']
PB = lambda n: arm.pose.bones['mixamorig:' + n]


def export(path):
    for o in bpy.data.objects:
        o.select_set(False)
    arm.select_set(True)
    for o in bpy.data.objects:
        if o.type == 'MESH' and o.parent == arm:
            o.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
                             use_mesh_modifiers=False, bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                             bake_anim_simplify_factor=0, bake_anim_force_startend_keying=True, axis_forward='-Z', axis_up='Y')


def setpose(poses):
    for pb in arm.pose.bones:
        pb.matrix_basis = poses[pb.name]
    bpy.context.view_layer.update()


def head(n):
    return arm.matrix_world @ PB(n).head


# 1) walk poses with root lock
W = []
T0 = None
for f in range(f0, f1 + 1):
    sc.frame_set(f)
    bpy.context.view_layer.update()
    M = hips.matrix.copy()
    if T0 is None:
        T0 = M.translation.copy()
    M.translation.x, M.translation.z = T0.x, T0.z
    hips.matrix = M
    bpy.context.view_layer.update()
    W.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})
setpose(W[0])
right_v = head('RightShoulder') - head('LeftShoulder')
right_v.z = 0
right_v.normalize()
up_v = Vector((0, 0, 1))
fwd_v = up_v.cross(right_v).normalized()

# 2) idle base: bind pose legs/hips, torso+left arm from the walk frame where the left wrist is nearest the body line
best, bk = 9, 0
for k in range(N):
    setpose(W[k])
    d = abs((head('LeftHand') - head('LeftArm')).dot(fwd_v))
    if d < best:
        best, bk = d, k
print('IDLE source walk frame', bk, 'left wrist fwd offset', best)
keepW = ('Spine', 'Spine1', 'Spine2', 'Neck', 'Head', 'LeftShoulder', 'RightShoulder') + tuple('Left' + b for b in ARM_BONES)
IDLE_BASE = {}
for pb in arm.pose.bones:
    n = pb.name.split(':')[-1]
    IDLE_BASE[pb.name] = W[bk][pb.name].copy() if n in keepW else Matrix.Identity(4)
# hips: bind rotation, translation from the walk frame, lowered/raised so the soles touch the same ground level as in the walk
mesh_ob = next(o for o in bpy.data.objects if o.type == 'MESH')


def mesh_minz():
    ev = mesh_ob.evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = ev.to_mesh()
    z = min((ev.matrix_world @ v.co).z for v in me.vertices)
    ev.to_mesh_clear()
    return z


walk_ground = 9
for k in range(N):
    setpose(W[k])
    walk_ground = min(walk_ground, mesh_minz())
hn = 'mixamorig:Hips'
IDLE_BASE[hn] = Matrix.Translation(W[bk][hn].to_translation())
setpose(IDLE_BASE)
dz = walk_ground - mesh_minz()
dv = arm.matrix_world.inverted().to_3x3() @ Vector((0, 0, dz))
IDLE_BASE[hn] = Matrix.Translation(W[bk][hn].to_translation() + dv)
setpose(IDLE_BASE)
print('IDLE ground fix', dz, 'walk ground', walk_ground, 'now', mesh_minz())
NI = 60
I = []
for i in range(NI):
    setpose(IDLE_BASE)
    ph = 2 * math.pi * i / NI
    for n, amp in (('Spine1', 0.7), ('Spine2', 0.9), ('Head', -0.6)):
        pb = PB(n)
        m = pb.matrix.copy()
        t = m.translation.copy()
        m.translation = Vector((0, 0, 0))
        m = Matrix.Rotation(math.radians(amp * math.sin(ph)), 4, 'X') @ m
        m.translation = t
        pb.matrix = m
        bpy.context.view_layer.update()
    I.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})

# 3) right-arm IK
tg = bpy.data.objects.new('RH', None)
pl = bpy.data.objects.new('RP', None)
sc.collection.objects.link(tg)
sc.collection.objects.link(pl)
fa = PB('RightForeArm')


def ik_arm(P):
    setpose(P[0])
    sh0 = head('RightArm')
    target0 = sh0 + right_v * HAND_OUT + fwd_v * HAND_FWD - up_v * HAND_DOWN
    pin0 = (arm.matrix_world @ PB(PIN_BONE).matrix).inverted()
    hand_local = pin0 @ target0
    pole_local = pin0 @ (sh0 + right_v * POLE[0] + fwd_v * POLE[1] + up_v * POLE[2])
    c = fa.constraints.new('IK')
    c.target, c.pole_target = tg, pl
    c.pole_angle = math.radians(POLE_ANGLE)
    c.chain_count, c.use_tail, c.use_stretch, c.iterations = 2, True, False, 80
    fA, fF = [], []
    for p in P:
        setpose(p)
        pinm = arm.matrix_world @ PB(PIN_BONE).matrix
        tg.location = pinm @ hand_local
        pl.location = pinm @ pole_local
        bpy.context.view_layer.update()
        fA.append(PB('RightArm').matrix.copy())
        fF.append(fa.matrix.copy())
    fa.constraints.remove(c)
    out = []
    for i, p in enumerate(P):
        setpose(p)
        PB('RightArm').matrix = fA[i]
        bpy.context.view_layer.update()
        fa.matrix = fF[i]
        bpy.context.view_layer.update()
        out.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})
    return out


def bake(name, frames):
    arm.animation_data_clear()
    arm.animation_data_create()
    action = bpy.data.actions.new('A_Combat_' + name)
    arm.animation_data.action = action
    try:
        arm.animation_data.action_slot = action.slots[0] if action.slots else action.slots.new('OBJECT', arm.name)
    except Exception:
        pass
    for i, poses in enumerate(frames, 1):
        for pb in arm.pose.bones:
            pb.matrix_basis = poses[pb.name]
            pb.keyframe_insert('location', frame=i)
            pb.keyframe_insert('rotation_quaternion', frame=i)
            pb.keyframe_insert('scale', frame=i)
    sc.frame_start, sc.frame_end = 1, len(frames)
    path = OUT / ('A_Combat_' + name + '.fbx')
    export(path)
    return {'name': name, 'file': path.name, 'frames': len(frames), 'fps': 30}


report = [bake('ColourWalk', ik_arm(W)), bake('ColourIdle', ik_arm(I))]
(OUT / 'build_report.json').write_text(json.dumps({'clips': report, 'hand_fwd': HAND_FWD, 'hand_down': HAND_DOWN, 'hand_out': HAND_OUT, 'pole': POLE}, indent=2))
print('COLOUR_BUILD_DONE', json.dumps(report))
