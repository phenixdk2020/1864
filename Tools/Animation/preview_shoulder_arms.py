"""Contact sheet of the shoulder-arms march with the rifle placed by the shoulder rule (a preview of the bake rule).
blender -b -P preview_shoulder_arms.py -- <Kildeassets dir> <clip> <png prefix> [view=front|side]
"""
import bpy, sys, json, math
from pathlib import Path
from mathutils import Vector, Matrix
a = sys.argv[sys.argv.index('--') + 1:]
KILDE, CLIP, PREFIX = Path(a[0]), a[1], a[2]
VIEW = a[3] if len(a) > 3 else 'front'
PROJECT = Path(__file__).resolve().parents[2]
poses = json.loads((PROJECT / 'SourceAssets/InfantryCombat/shoulder_poses_preview.json').read_text())[CLIP]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(KILDE / 'DK_Infanteri_1864_textured_skeleton.fbx'))
sold = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
bpy.ops.import_scene.fbx(filepath=str(KILDE / 'Riffel.fbx'))
rifle = [o for o in bpy.data.objects if o.type == 'MESH' and o.parent is None][-1]
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'; sc.render.resolution_x = 360; sc.render.resolution_y = 560
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'TEXTURE'
sc.world = bpy.data.worlds.new('w'); sc.world.color = (0.3, 0.32, 0.36)
l = bpy.data.objects.new('s', bpy.data.lights.new('s', 'SUN')); sc.collection.objects.link(l); l.rotation_euler = (0.9, 0.2, 0.6)
cam = bpy.data.objects.new('c', bpy.data.cameras.new('c')); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.8; cam.data.clip_end = 100
for pb in sold.pose.bones: pb.rotation_mode = 'QUATERNION'
order = sorted(sold.pose.bones, key=lambda b: len(b.parent_recursive))
def pose(i):
    fm = poses[i]
    for pb in order: pb.matrix_basis = Matrix(fm[pb.name])
    bpy.context.view_layer.update()
def H(n): return sold.matrix_world @ sold.pose.bones['mixamorig:' + n].head
pose(0)
right = H('RightShoulder') - H('LeftShoulder'); right.z = 0; right.normalize()
up = Vector((0, 0, 1)); fwd = up.cross(right).normalized()
orig = (H('LeftFoot') + H('RightFoot')) / 2; orig.z = 0
SCALE = 0.8
for i in range(0, len(poses), max(1, len(poses) // 6)):
    pose(i)
    side = (H('RightShoulder') - H('LeftShoulder')).normalized()
    f = side.cross(up).normalized() if False else up.cross(side).normalized()
    hand = H('RightHand')
    axis = (up * 0.90 - f * 0.45).normalized()          # butt in the hand, barrel up and back over the shoulder
    x = axis; z = (-f - x * (-f).dot(x)).normalized()   # the top of the rifle toward the back
    y = z.cross(x)
    m = Matrix((x, y, z)).transposed().to_4x4()
    grip = 0.30 * 2 * SCALE                              # the hand a third of the way up from the butt
    m.translation = hand - x * (grip - SCALE)
    rifle.matrix_world = m @ Matrix.Diagonal((SCALE,) * 3).to_4x4()
    if VIEW == 'front':
        pos = orig + fwd * 5 + up * 1.2
    else:
        pos = orig + right * 5 + up * 1.2
    look = orig + up * 1.25
    cam.location = pos; cam.rotation_euler = (look - pos).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = f'{PREFIX}_{VIEW}_{i:02d}.png'; bpy.ops.render.render(write_still=True)
print('PREVIEW_DONE')
