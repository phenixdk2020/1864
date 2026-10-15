"""Bake the Mixamo clips added 2026-10-13 (reload, melee, signalling, sabre) onto the shared rig.
Run with Blender --background --python this_file -- <MultiView directory>.
"""
import bpy, json, sys
from pathlib import Path

root = Path(sys.argv[sys.argv.index('--') + 1])
project = Path(__file__).resolve().parents[2]
output = project / 'SourceAssets/InfantryCombat'
output.mkdir(parents=True, exist_ok=True)
INF = 'Animationer M\u00e6nd'
CAV = 'Danish Cavalry'
# name, folder, file, lock_root_xz (movement belongs to the strategy unit)
clips = [
    ('ReloadKneelingB', INF, 'Reload.fbx', True),
    ('ReloadKneeling', INF, 'Reload kneeling.fbx', True),
    ('RiflePunch', INF, 'Rifle Punch.fbx', True),
    ('SideStep', INF, 'Rifle Side Step.fbx', True),
    ('Turn', INF, 'Rifle Turn.fbx', True),
    ('Signal', INF, 'Military Signaling.fbx', True),
    ('DeathBackHeadshot', INF, 'Death From Back Headshot.fbx', False),
    ('SabreDraw', CAV, 'Draw Sword 1.fbx', True),
    ('SabreWithdraw', CAV, 'Withdrawing Sword.fbx', True),
    ('SabreSheath', CAV, 'Sheathing Sword.fbx', True),
    ('SabreSheath2', CAV, 'Sheath Sword 2.fbx', True),
    ('SabreCombo', CAV, 'One Hand Sword Combo.fbx', True),
    ('SabreFight', CAV, 'Sword Fight One.fbx', True),
]


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path))
    return next(o for o in bpy.data.objects if o.type == 'ARMATURE')


def sample(rig):
    start, end = map(int, rig.animation_data.action.frame_range)
    frames = []
    for f in range(start, end + 1):
        bpy.context.scene.frame_set(f)
        frames.append({b.name: b.matrix_basis.copy() for b in rig.pose.bones})
    return frames


def bake(rig, name, frames):
    rig.animation_data_clear()
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new('A_Combat_' + name)
    for i, poses in enumerate(frames, 1):
        for bone in rig.pose.bones:
            bone.rotation_mode = 'QUATERNION'
            bone.matrix_basis = poses[bone.name]
            bone.keyframe_insert('location', frame=i)
            bone.keyframe_insert('rotation_quaternion', frame=i)
            bone.keyframe_insert('scale', frame=i)
    sc = bpy.context.scene
    sc.render.fps = 30
    sc.frame_start, sc.frame_end = 1, len(frames)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    path = output / ('A_Combat_' + name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE'},
        add_leaf_bones=False, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0, axis_forward='-Z', axis_up='Y')
    return {'name': name, 'file': path.name, 'frames': len(frames), 'fps': 30,
            'duration': (len(frames) - 1) / 30, 'bones': len(rig.data.bones)}


report = []
for name, folder, filename, lock in clips:
    rig = load(root / folder / filename)
    frames = sample(rig)
    if lock:
        hips = rig.pose.bones[0].name
        o = frames[0][hips].translation.copy()
        for poses in frames:
            t = poses[hips].translation.copy()
            t.x, t.z = o.x, o.z
            poses[hips].translation = t
    report.append(bake(rig, name, frames))
(output / 'manifest_extra.json').write_text(json.dumps({'clips': report}, indent=2))
print('EXTRA_ANIMATIONS=' + json.dumps(report))
