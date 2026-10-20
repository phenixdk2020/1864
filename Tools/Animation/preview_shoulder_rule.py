"""Mirrors the shoulder bake rule of StrategyCrowdModel.cpp in Blender and renders front / side / 3-4 views with the rifle mesh.
blender -b -P preview_shoulder_rule.py -- <Kildeassets dir> <clip> <png prefix> <label> <up> <side> <fwd> [frames=0,6,12] [rifle_scale=0.8] [grip=0.15]
up/side/fwd are the barrel coefficients of the C++ rule: Barrel = Up*up + Side*side + Forward*fwd (fwd positive = muzzle forward).
Before (old rule): 0.92 0.32 -0.10, grip 0.30.  After (new rule): 0.90 0.20 -0.33, grip 0.15.  The grip is the fraction of the rifle length from the butt; the rifle mesh is centred on its origin with the barrel along +X.
"""
import bpy, sys, json, math, os
from pathlib import Path
from mathutils import Vector, Matrix
a = sys.argv[sys.argv.index('--') + 1:]
KILDE, CLIP, PREFIX, LABEL = Path(a[0]), a[1], a[2], a[3]
CU, CS, CF = float(a[4]), float(a[5]), float(a[6])
FRAMES = [int(x) for x in (a[7] if len(a) > 7 else '0,6,12').split(',')]
SCALE = float(a[8]) if len(a) > 8 else 0.8
GRIP = float(a[9]) if len(a) > 9 else 0.15
PROJECT = Path(__file__).resolve().parents[2]
poses = json.loads(Path(os.environ.get('SHOULDER_POSES_JSON', PROJECT / 'SourceAssets/InfantryCombat/shoulder_poses_preview.json')).read_text())[CLIP]   # SHOULDER_POSES_JSON: e.g. the old poses for a before picture
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(KILDE / 'DK_Infanteri_1864_textured_skeleton.fbx'))
sold = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
# V3 evaluates the exported source rig, then displays its world-space bone
# matrices on the existing skinned mesh. No preview pose JSON drives the rig.
actual_fbx = os.environ.get('SHOULDER_FBX')
if actual_fbx:
    display_rig = sold
    display_rig.animation_data_clear()
    bpy.ops.import_scene.fbx(filepath=actual_fbx, automatic_bone_orientation=False)
    source_rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE' and o != display_rig)
    action_start = round(source_rig.animation_data.action.frame_range[0])
bpy.ops.import_scene.fbx(filepath=str(KILDE / 'Riffel.fbx'))
rifle = [o for o in bpy.data.objects if o.type == 'MESH' and o.parent is None][-1]
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'; sc.render.resolution_x = 420; sc.render.resolution_y = 560
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'TEXTURE'
sc.world = bpy.data.worlds.new('w'); sc.world.color = (0.3, 0.32, 0.36)
l = bpy.data.objects.new('s', bpy.data.lights.new('s', 'SUN')); sc.collection.objects.link(l); l.rotation_euler = (0.9, 0.2, 0.6)
cam = bpy.data.objects.new('c', bpy.data.cameras.new('c')); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.8; cam.data.clip_end = 100
for pb in sold.pose.bones: pb.rotation_mode = 'QUATERNION'
order = sorted(sold.pose.bones, key=lambda b: len(b.parent_recursive))
def pose(i):
    if actual_fbx:
        sc.frame_set(action_start + i)
        bpy.context.view_layer.update()
        to_display = sold.matrix_world.inverted() @ source_rig.matrix_world
        for pb in order:
            pb.matrix = to_display @ source_rig.pose.bones[pb.name].matrix
            bpy.context.view_layer.update()
    else:
        for pb in order: pb.matrix_basis = Matrix(poses[i][pb.name])
    bpy.context.view_layer.update()

def H(n):
    rig = source_rig if actual_fbx else sold
    return rig.matrix_world @ rig.pose.bones['mixamorig:' + n].head
pose(0)
right0 = H('RightShoulder') - H('LeftShoulder'); right0.z = 0; right0.normalize()
up = Vector((0, 0, 1)); fwd0 = up.cross(right0).normalized()
orig = (H('LeftFoot') + H('RightFoot')) / 2; orig.z = 0
views = {'front': (fwd0, 0), 'side': (right0, 0), 'q34': ((fwd0 + right0).normalized(), 0)}
for i in FRAMES:
    pose(i)
    side = (H('RightShoulder') - H('LeftShoulder')).normalized()
    f = up.cross(side).normalized()                       # physical forward (the C++ Cross(Side, Up) in left-handed UE)
    barrel = (up * CU + side * CS + f * CF).normalized()
    z = (side - barrel * side.dot(barrel)).normalized()   # MakeFromXZ(Barrel, Side)
    y = z.cross(barrel)
    m = Matrix((barrel, y, z)).transposed().to_4x4()
    hand = H('RightHand')
    grip = (GRIP - 0.5) * 2 * SCALE                       # the hand at GRIP of the length from the butt; mesh centred, butt at -SCALE
    m.translation = hand - barrel * grip
    rifle.matrix_world = m @ Matrix.Diagonal((SCALE,) * 3).to_4x4()
    for vn, (d, _) in views.items():
        pos = orig + d * 5 + up * 1.2
        cam.location = pos; cam.rotation_euler = ((orig + up * 1.25) - pos).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{PREFIX}_{LABEL}_{vn}_{i:02d}.png'; bpy.ops.render.render(write_still=True)
    print('hand', tuple(hand), 'barrel', tuple(barrel), 'fwd', tuple(f))
print('RULE_PREVIEW_DONE')
