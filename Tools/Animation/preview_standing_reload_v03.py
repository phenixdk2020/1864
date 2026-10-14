"""Contact sheets of the standing reload with the rifle and props.
blender -b -P preview_standing_reload_v03.py -- <Kildeassets dir> <build out dir> <png prefix> [view=front|side|hands] [frames=comma list]
"""
import bpy, sys, json, math
from pathlib import Path
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index('--') + 1:]
KILDE, OUT, PREFIX = Path(a[0]), Path(a[1]), a[2]
VIEW = a[3] if len(a) > 3 else 'front'
FR = [int(x) for x in a[4].split(',')] if len(a) > 4 else [1, 25, 41, 55, 71, 85, 101, 115, 131, 145, 169, 181, 193, 206, 221, 236, 251, 271, 281, 295, 306, 315, 331]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(KILDE / 'DK_Infanteri_1864_textured_skeleton.fbx'))
sold = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
FINALS = json.loads((OUT / 'pose_finals_v03.json').read_text())
sold.animation_data_clear()
for pb in sold.pose.bones: pb.rotation_mode = 'QUATERNION'
order = sorted(sold.pose.bones, key=lambda b: len(b.parent_recursive))
def pose_at(f):
    fm = FINALS[str(f)]
    for pb in order:
        pb.matrix = Matrix(fm[pb.name])
        bpy.context.view_layer.update()
meshes = {}
for name, f in (('rifle', 'Riffel'), ('rod', 'metal-rod'), ('cart', 'wrapped-roll'), ('cap', 'copper-cap')):
    b = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(KILDE / (f + '.fbx')))
    o = next(x for x in bpy.data.objects if x not in b and x.type == 'MESH')
    meshes[name] = o
props = json.loads((OUT / 'props_v03.json').read_text())
sc_ = props['scale']
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
sc.render.resolution_x = 360; sc.render.resolution_y = 560
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'TEXTURE'
sc.world = bpy.data.worlds.new('w'); sc.world.color = (0.3, 0.32, 0.36)
l = bpy.data.objects.new('s', bpy.data.lights.new('s', 'SUN')); sc.collection.objects.link(l); l.rotation_euler = (0.9, 0.2, 0.6)
cam = bpy.data.objects.new('c', bpy.data.cameras.new('c')); sc.collection.objects.link(cam); sc.camera = cam
cam.data.clip_end = 100; cam.data.type = 'ORTHO'
# soldier frame from the pose at frame 1
pose_at(1)
def hd(n): return sold.matrix_world @ sold.pose.bones['mixamorig:' + n].head
right = hd('RightShoulder') - hd('LeftShoulder'); right.z = 0; right.normalize()
fwd = Vector((0, 0, 1)).cross(right).normalized()
orig = (hd('LeftFoot') + hd('RightFoot')) / 2; orig.z = 0
def set_cam(kind):
    if kind == 'front':
        pos = orig + fwd * 5 + Vector((0, 0, 1.2)); look = orig + Vector((0, 0, 1.25)); cam.data.ortho_scale = 2.8
    elif kind == 'side':
        pos = orig + right * 5 + Vector((0, 0, 1.2)); look = orig + Vector((0, 0, 1.25)); cam.data.ortho_scale = 2.8
    else:
        pos = orig + fwd * 3 + right * 0.5 + Vector((0, 0, 1.6)); look = orig + fwd * 0.2 + Vector((0, 0, 1.5)); cam.data.ortho_scale = 1.4
    cam.location = pos
    cam.rotation_euler = (look - pos).to_track_quat('-Z', 'Y').to_euler()
set_cam(VIEW)
scales = {'rifle': 0.8, 'rod': 0.9, 'cart': 0.245, 'cap': 0.19}
for f in FR:
    pose_at(f); i = f - 1
    for n, o in meshes.items():
        m = props[n][i] if n != 'rifle' else props['rifle'][i]
        if m is None:
            o.hide_render = True; continue
        o.hide_render = False
        mat = Matrix(m)
        mat = mat @ Matrix.Diagonal((scales[n],) * 3).to_4x4()
        o.matrix_world = mat
    sc.render.filepath = f'{PREFIX}_{VIEW}_{f:03d}.png'
    bpy.ops.render.render(write_still=True)
print('PREVIEW_DONE')
