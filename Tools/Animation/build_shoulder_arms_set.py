"""Shoulder-arms march: the column carries the rifle on the shoulder (right hand at the butt on the hip, left arm swinging).
Legs and torso stay the project's own march ('Walking with rifle', so the walking speed still fits); only the arms change.

    blender -b -P build_shoulder_arms_set.py -- <Kildeassets dir> <MultiView dir>

Writes SourceAssets/InfantryCombat/A_Combat_{WalkShoulder,ShoulderUp,ShoulderDown}.fbx and manifest_shoulder.json
(imported by import_infantry_combat_set.py). ShoulderUp / ShoulderDown are one walking cycle in which the arms move between the
rifle-in-hands carry and the shoulder carry.
"""
import bpy, json, math, sys
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

args = sys.argv[sys.argv.index('--') + 1:]
KILDE, MULTI = Path(args[0]), Path(args[1])
PROJECT = Path(__file__).resolve().parents[2]
OUT = PROJECT / 'SourceAssets/InfantryCombat'
OUT.mkdir(parents=True, exist_ok=True)
WALK = MULTI / 'Animationer Mænd' / 'Walking with rifle.fbx'
ARM_BONES = ['Arm', 'ForeArm', 'Hand'] + ['Hand%s%d' % (f, i) for f in ('Thumb', 'Index', 'Middle', 'Ring', 'Pinky') for i in (1, 2, 3, 4)]
RIGHT_HAND = (0.26, 0.12, 1.00)    # soldier frame (x right, y forward, z up): the butt held at the hip
LEFT_HAND = (-0.30, 0.00, 0.93)    # the left arm hangs and swings


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


arm, act = load()
f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
N = f1 - f0 + 1
sc = bpy.context.scene
# original pose per frame (A)
A = []
HIPZ = []
for f in range(f0, f1 + 1):
    sc.frame_set(f)
    bpy.context.view_layer.update()
    A.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})
    HIPZ.append((arm.matrix_world @ arm.pose.bones['mixamorig:Hips'].head).z)
sc.frame_set(f0)
bpy.context.view_layer.update()


def head(n):
    return arm.matrix_world @ arm.pose.bones['mixamorig:' + n].head


right_v = head('RightShoulder') - head('LeftShoulder')
right_v.z = 0
right_v.normalize()
up_v = Vector((0, 0, 1))
fwd_v = up_v.cross(right_v).normalized()
origin = (head('LeftFoot') + head('RightFoot')) / 2
origin.z = 0.0
print('FRAME right', tuple(right_v), 'fwd', tuple(fwd_v))


def W(p):
    return origin + right_v * p[0] + fwd_v * p[1] + up_v * p[2]


tg = {}
for n in ('LH', 'RH', 'LP', 'RP'):
    e = bpy.data.objects.new(n, None)
    sc.collection.objects.link(e)
    tg[n] = e
for side, p in (('Left', 'LH'), ('Right', 'RH')):
    pb = arm.pose.bones['mixamorig:%sForeArm' % side]
    c = pb.constraints.new('IK')
    c.target = tg[p]
    c.pole_target = tg['LP' if side == 'Left' else 'RP']
    c.pole_angle = math.radians(-90.0)
    c.chain_count = 2
    c.use_tail = True
    c.use_stretch = False
    c.iterations = 80
tg['LP'].location = W((-0.65, -0.20, 1.15))
tg['RP'].location = W((0.65, -0.20, 1.15))

finals = []
for i in range(N):
    sc.frame_set(f0 + i)
    bpy.context.view_layer.update()
    bob = HIPZ[i] - HIPZ[0]
    swing = math.sin(2 * math.pi * i / N)
    tg['RH'].location = W((RIGHT_HAND[0], RIGHT_HAND[1], RIGHT_HAND[2] + bob))
    tg['LH'].location = W((LEFT_HAND[0], LEFT_HAND[1] + 0.12 * swing, LEFT_HAND[2] + bob))
    bpy.context.view_layer.update()
    finals.append({pb.name: pb.matrix.copy() for pb in arm.pose.bones})
# turn the solved poses into local (basis) matrices: remove the constraints, assign parent first
for pb in arm.pose.bones:
    for c in list(pb.constraints):
        pb.constraints.remove(c)
arm.animation_data_clear()
for pb in arm.pose.bones:
    pb.rotation_mode = 'QUATERNION'
order = sorted(arm.pose.bones, key=lambda b: len(b.parent_recursive))
B = []
for i in range(N):
    for pb in order:
        pb.matrix = finals[i][pb.name]
        bpy.context.view_layer.update()
    B.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})


def arm_bone(name):
    short = name.split(':')[-1]
    return any(short == side + b for side in ('Left', 'Right') for b in ARM_BONES)


def blend(a, b, w):
    out = {}
    for name, ma in a.items():
        if not arm_bone(name):
            out[name] = ma.copy()
            continue
        l0, r0, s0 = ma.decompose()
        l1, r1, s1 = b[name].decompose()
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
            pb.rotation_mode = 'QUATERNION'
            pb.matrix_basis = poses[pb.name]
            pb.keyframe_insert('location', frame=i)
            pb.keyframe_insert('rotation_quaternion', frame=i)
            pb.keyframe_insert('scale', frame=i)
    sc.render.fps = 30
    sc.frame_start, sc.frame_end = 1, len(frames)
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = OUT / ('A_Combat_' + name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE'}, add_leaf_bones=False,
                             bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0,
                             axis_forward='-Z', axis_up='Y')
    return {'name': name, 'file': path.name, 'frames': len(frames), 'fps': 30, 'duration': (len(frames) - 1) / 30, 'bones': len(arm.data.bones)}


# the local poses of the three clips, for the preview script (Tools/Animation/preview_shoulder_arms.py)
POSES = {'WalkShoulder': [blend(A[i], B[i], 1.0) for i in range(N)],
         'ShoulderUp': [blend(A[i], B[i], smooth(i / (N - 6))) for i in range(N)],
         'ShoulderDown': [blend(A[i], B[i], 1.0 - smooth(i / (N - 6))) for i in range(N)]}
(OUT / 'shoulder_poses_preview.json').write_text(json.dumps({k: [{n: [list(r) for r in m] for n, m in fr.items()} for fr in v] for k, v in POSES.items()}))
report = []
report.append(bake('WalkShoulder', [blend(A[i], B[i], 1.0) for i in range(N)]))
report.append(bake('ShoulderUp', [blend(A[i], B[i], smooth(i / (N - 6))) for i in range(N)]))
report.append(bake('ShoulderDown', [blend(A[i], B[i], 1.0 - smooth(i / (N - 6))) for i in range(N)]))
(OUT / 'manifest_shoulder.json').write_text(json.dumps({'clips': report}, indent=2))
print('SHOULDER=' + json.dumps(report))
