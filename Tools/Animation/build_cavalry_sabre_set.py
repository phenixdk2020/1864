"""Mounted sabre clips: the rider's seat (pelvis and legs from 'Reload sitting') with the upper body of the Mixamo sabre clips.
blender -b -P build_cavalry_sabre_set.py -- <MultiView dir>
Writes SourceAssets/InfantryCombat/A_Combat_CavSabre{Draw,Slash,Fight}.fbx and manifest_cav.json (imported by import_infantry_combat_set.py).
"""
import bpy, json, sys
from pathlib import Path
from mathutils import Matrix

root = Path(sys.argv[sys.argv.index('--') + 1])
project = Path(__file__).resolve().parents[2]
output = project / 'SourceAssets/InfantryCombat'
INF, CAV = 'Animationer M\u00e6nd', 'Danish Cavalry'
clips = [('CavSabreDraw', 'Draw Sword 1.fbx', 4), ('CavSabreSlash', 'One Hand Sword Combo.fbx', 12), ('CavSabreFight', 'Sword Fight One.fbx', 12)]
UPPER = ('Spine1', 'Spine2', 'Neck', 'Head', 'Shoulder', 'Arm', 'ForeArm', 'Hand')


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path))
    rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = rig.animation_data.action
    try:
        rig.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    return rig


def sample(rig):
    start, end = map(int, rig.animation_data.action.frame_range)
    out = []
    for f in range(start, end + 1):
        bpy.context.scene.frame_set(f)
        out.append({b.name: b.matrix_basis.copy() for b in rig.pose.bones})
    return out


def bake(rig, name, frames):
    rig.animation_data_clear(); rig.animation_data_create()
    act = bpy.data.actions.new('A_Combat_' + name)
    rig.animation_data.action = act
    try:
        rig.animation_data.action_slot = act.slots[0] if act.slots else act.slots.new('OBJECT', rig.name)
    except Exception:
        pass
    for i, poses in enumerate(frames, 1):
        for b in rig.pose.bones:
            b.rotation_mode = 'QUATERNION'
            b.matrix_basis = poses[b.name]
            b.keyframe_insert('location', frame=i); b.keyframe_insert('rotation_quaternion', frame=i); b.keyframe_insert('scale', frame=i)
    sc = bpy.context.scene
    sc.render.fps = 30; sc.frame_start, sc.frame_end = 1, len(frames)
    bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); bpy.context.view_layer.objects.active = rig
    path = output / ('A_Combat_' + name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE'}, add_leaf_bones=False,
                             bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0,
                             axis_forward='-Z', axis_up='Y')
    return {'name': name, 'file': path.name, 'frames': len(frames), 'fps': 30, 'duration': (len(frames) - 1) / 30, 'bones': len(rig.data.bones)}


seat_rig = load(root / INF / 'Reload sitting.fbx')
seat = sample(seat_rig)[3]
report = []
for name, filename, blend in clips:
    rig = load(root / CAV / filename)
    frames = sample(rig)
    n = len(frames)
    out = []
    for i, poses in enumerate(frames):
        w = min(1.0, i / blend, (n - 1 - i) / blend) if blend > 4 else min(1.0, i / blend)
        frame = {}
        for bone, pose in poses.items():
            short = bone.split(':')[-1]
            if any(short.startswith(u) or short.endswith(u) or u in short for u in UPPER) and 'UpLeg' not in short and 'Leg' not in short.replace('LeftHand', '') and short != 'Spine':
                l0, r0, s0 = seat[bone].decompose()
                l1, r1, s1 = pose.decompose()
                frame[bone] = Matrix.LocRotScale(l0.lerp(l1, w), r0.slerp(r1, w), s0.lerp(s1, w))
            elif short == 'Spine':
                l0, r0, s0 = seat[bone].decompose()
                l1, r1, s1 = pose.decompose()
                frame[bone] = Matrix.LocRotScale(l0, r0.slerp(r1, 0.5 * w), s0)
            else:
                frame[bone] = seat[bone].copy()
        out.append(frame)
    report.append(bake(rig, name, out))
(output / 'manifest_cav.json').write_text(json.dumps({'clips': report}, indent=2))
print('CAV_SABRE=' + json.dumps(report))
