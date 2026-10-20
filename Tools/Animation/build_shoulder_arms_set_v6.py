"""Shoulder-arms march, v6 (2026-10-17): built from the RAW Mixamo walk WITH skinned mesh and bind pose, exported WITH the mesh.
(Armature-only exports import into Unreal lying down; the raw Mixamo files stand.)

    blender -b -P build_shoulder_arms_set.py -- [<MultiView dir>] [<out dir>]

Writes A_Combat_{WalkShoulder,ShoulderUp,ShoulderDown}.fbx and RoundTrip_Walk.fbx (the unmodified walk through the same exporter)
to <out dir> (default Saved/Codex/shoulder_v6, NOT SourceAssets) plus shoulder_v6_report.json.
Method: legs, left arm, spine and head come from the Mixamo walk; Hips X/Z held (walk in place, vertical bob kept); the right arm is
re-posed with two-bone IK so the hand holds the rifle butt low in FRONT of the right hip (arm almost straight, elbow not lifted).
v5: hand target + pole are stored in the Spine bone frame of frame 0 and re-applied with that bone's pose every frame, so the hand
follows the torso (steady in front of the hip) and ignores the walk's arm swing.
ShoulderUp/Down blend the right arm (and fingers) between the walk pose and the carry pose with smoothstep + slerp.
Environment: PIN_BONE (default Spine2 in v6), FWD_MIN/FWD_MAX wrist forward band (m from the right shoulder) in the blends, SPINE_FIX=<deg> extra backwards pitch spread over Spine/Spine1/Spine2 (hunch fix), POLE_ANGLE=<deg> IK pole angle.
"""
import bpy, json, math, os, sys
from pathlib import Path
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
MULTI = Path(a[0]) if a else Path(r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView')
OUT = Path(a[1]) if len(a) > 1 else PROJECT / 'Saved/Codex/shoulder_v6'
OUT.mkdir(parents=True, exist_ok=True)
WALK = MULTI / '3D figurer' / 'Walking.fbx'
ARM_BONES = ['Arm', 'ForeArm', 'Hand'] + ['Hand%s%d' % (f, i) for f in ('Index', 'Middle', 'Ring', 'Pinky', 'Thumb') for i in (1, 2, 3, 4)]
# carry pose relative to the right shoulder: fraction of the arm length straight down (1.0 = hanging straight), lateral/forward offsets in m
HAND_DOWN = float(os.environ.get('HAND_DOWN', '0.86'))
HAND_OUT = float(os.environ.get('HAND_OUT', '-0.03'))
HAND_FWD = float(os.environ.get('HAND_FWD', '0.17'))
PIN_BONE = os.environ.get('PIN_BONE', 'Spine2')   # the carry target and the IK pole are pinned in this bone's frame (pelvis/spine), not in world axes
SPINE_FIX = float(os.environ.get('SPINE_FIX', '0'))
FWD_MIN = float(os.environ.get('FWD_MIN', '0.12'))
LAT_MIN = float(os.environ.get('LAT_MIN', '-0.05'))   # (v6) wrist lateral band (m, + = outwards) from the right shoulder joint: keeps the barrel line off the neck/head in the blends
LAT_MAX = float(os.environ.get('LAT_MAX', '-0.01'))
FWD_MAX = float(os.environ.get('FWD_MAX', '0.17'))   # (v6) and never further forward than this either, so the barrel tilt stays small   # during ShoulderUp/Down the wrist is never allowed behind this forward distance (m) from the right shoulder
POLE_ANGLE = float(os.environ.get('POLE_ANGLE', '-90'))


def load():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(WALK))
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = arm.animation_data.action
    try:
        arm.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    return arm, act


def export(arm, path):
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


arm, act = load()
sc = bpy.context.scene
sc.render.fps = 30
f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
N = f1 - f0 + 1
export(arm, OUT / 'RoundTrip_Walk.fbx')   # the UNMODIFIED walk through the same exporter

for pb in arm.pose.bones:
    pb.rotation_mode = 'QUATERNION'
hips = arm.pose.bones['mixamorig:Hips']


def setpose(poses):
    for pb in arm.pose.bones:
        pb.matrix_basis = poses[pb.name]
    bpy.context.view_layer.update()


# 1) original local poses + root lock (hips X/Z in armature space held, Y = vertical bob kept) + optional spine fix
A = []
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
    if SPINE_FIX:
        for n in ('Spine', 'Spine1', 'Spine2'):
            pb = arm.pose.bones['mixamorig:' + n]
            m = pb.matrix.copy()
            t = m.translation.copy()
            m.translation = Vector((0, 0, 0))
            m = Matrix.Rotation(math.radians(SPINE_FIX / 3), 4, 'X') @ m   # armature X = the shoulder axis (rotated about the bone head)
            m.translation = t
            pb.matrix = m
            bpy.context.view_layer.update()
    A.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})
setpose(A[0])


def head(n):
    return arm.matrix_world @ arm.pose.bones['mixamorig:' + n].head


# 2) body frame and carry target, from the first pose
right_v = head('RightShoulder') - head('LeftShoulder')
right_v.z = 0
right_v.normalize()
up_v = Vector((0, 0, 1))
fwd_v = up_v.cross(right_v).normalized()
sh0 = head('RightArm')
arm_len = (head('RightForeArm') - head('RightArm')).length + (head('RightHand') - head('RightForeArm')).length
hips0 = head('Hips')
hand_target0 = sh0 + right_v * HAND_OUT + fwd_v * HAND_FWD - up_v * (HAND_DOWN * arm_len)
pin0 = (arm.matrix_world @ arm.pose.bones['mixamorig:' + PIN_BONE].matrix).inverted()
hand_local = pin0 @ hand_target0
pole_local = pin0 @ (sh0 + right_v * 0.35 - fwd_v * 0.6)
print('FRAME right', tuple(right_v), 'fwd', tuple(fwd_v), 'armlen', arm_len, 'hand', tuple(hand_target0), 'hips', tuple(hips0))

# 3) IK for the right arm per frame; the target follows the hips (bob)
tg = bpy.data.objects.new('RH', None)
pl = bpy.data.objects.new('RP', None)
sc.collection.objects.link(tg)
sc.collection.objects.link(pl)
fa = arm.pose.bones['mixamorig:RightForeArm']
c = fa.constraints.new('IK')
c.target, c.pole_target = tg, pl
c.pole_angle = math.radians(POLE_ANGLE)
c.chain_count = 2
c.use_tail = True
c.use_stretch = False
c.iterations = 80
finalsA, finalsF = [], []
for i in range(N):
    setpose(A[i])
    pinm = arm.matrix_world @ arm.pose.bones['mixamorig:' + PIN_BONE].matrix
    tg.location = pinm @ hand_local
    pl.location = pinm @ pole_local
    bpy.context.view_layer.update()
    finalsA.append(arm.pose.bones['mixamorig:RightArm'].matrix.copy())
    finalsF.append(fa.matrix.copy())
fa.constraints.remove(c)
B = []
for i in range(N):
    setpose(A[i])
    arm.pose.bones['mixamorig:RightArm'].matrix = finalsA[i]
    bpy.context.view_layer.update()
    fa.matrix = finalsF[i]
    bpy.context.view_layer.update()
    B.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})


def is_arm(name):
    return name.split(':')[-1] in ['Right' + b for b in ARM_BONES]


def blend(a_, b_, w):
    out = {}
    for name, ma in a_.items():
        if not is_arm(name):
            out[name] = ma.copy()
            continue
        l0, r0, s0 = ma.decompose()
        l1, r1, s1 = b_[name].decompose()
        out[name] = Matrix.LocRotScale(l0.lerp(l1, w), r0.slerp(r1, w), s0.lerp(s1, w))
    return out


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


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
    export(arm, path)
    return {'name': name, 'file': path.name, 'frames': len(frames), 'fps': 30, 'bones': len(arm.data.bones)}


def clamp_arm(poses):
    """The blended arm swings back with the walk; push the wrist forward (two-bone IK) so it stays >= FWD_MIN in front of the right shoulder
    along the torso forward axis, i.e. the rifle in the hand can never go behind the shoulder/chest."""
    setpose(poses)
    pinm = arm.matrix_world @ arm.pose.bones['mixamorig:' + PIN_BONE].matrix
    f_w = (pinm.to_3x3() @ (pin0.to_3x3() @ fwd_v)).normalized()   # torso forward axis in this frame (spine-frame rotation, world units)
    wrist = head('RightHand')
    s_w = (pinm.to_3x3() @ (pin0.to_3x3() @ right_v)).normalized()
    cur = (wrist - head('RightArm')).dot(f_w)
    cl = (wrist - head('RightArm')).dot(s_w)
    if FWD_MIN <= cur <= FWD_MAX and LAT_MIN <= cl <= LAT_MAX:
        return poses
    tg.location = wrist + f_w * (min(max(cur, FWD_MIN), FWD_MAX) - cur) + s_w * (min(max(cl, LAT_MIN), LAT_MAX) - cl)
    pl.location = pinm @ pole_local
    c2 = fa.constraints.new('IK')
    c2.target, c2.pole_target = tg, pl
    c2.pole_angle = math.radians(POLE_ANGLE)
    c2.chain_count = 2
    c2.use_tail = True
    c2.use_stretch = False
    c2.iterations = 80
    bpy.context.view_layer.update()
    mA = arm.pose.bones['mixamorig:RightArm'].matrix.copy()
    mF = fa.matrix.copy()
    fa.constraints.remove(c2)
    setpose(poses)
    arm.pose.bones['mixamorig:RightArm'].matrix = mA
    bpy.context.view_layer.update()
    fa.matrix = mF
    bpy.context.view_layer.update()
    out = {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}
    # keep the blended hand and finger rotations
    for n in poses:
        if is_arm(n) and n.split(':')[-1] not in ('RightArm', 'RightForeArm'):
            out[n] = poses[n]
    return out


T = N - 6   # the transition is complete a few frames before the end, like the earlier generator
report = [bake('WalkShoulder', [blend(A[i], B[i], 1.0) for i in range(N)]),
          bake('ShoulderUp', [clamp_arm(blend(A[i], B[i], smooth(i / T))) for i in range(N)]),
          bake('ShoulderDown', [clamp_arm(blend(A[i], B[i], 1.0 - smooth(i / T))) for i in range(N)])]
(OUT / 'shoulder_v6_report.json').write_text(json.dumps({'source': str(WALK), 'clips': report, 'hand_down': HAND_DOWN, 'hand_out': HAND_OUT, 'hand_fwd': HAND_FWD, 'fwd_min': FWD_MIN, 'fwd_max': FWD_MAX, 'lat_min': LAT_MIN, 'lat_max': LAT_MAX, 'pin_bone': PIN_BONE}, indent=2))
print('SHOULDER_V6=' + json.dumps(report))
