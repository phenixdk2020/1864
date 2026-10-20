"""Renders front / side / 3-4 views of an exported shoulder clip (FBX WITH mesh) with the rifle placed by the C++ rule.
blender -b -P preview_shoulder_v9.py -- <clip fbx> <png prefix> <up> <side> <fwd> <grip> <frames e.g. 0,10,24> [rifle glb] [rifle length m]
Rule (v6, contact-point rule): the args <up> <side> <fwd> are read as <a_fwd> <b_up> <c_side> = offsets from the RightShoulder bone head; Barrel = normalize(RightShoulder + Fwd*a + Up*b + Side*c - Hand); the hand holds the rifle <grip> of its length from the butt.
Forward = Cross(Side, Up) in left-handed UE = physical forward here."""
import bpy, sys
from pathlib import Path
from mathutils import Vector, Matrix
a = sys.argv[sys.argv.index('--') + 1:]
FBX, PREFIX = a[0], a[1]
CU, CS, CF, GRIP = float(a[2]), float(a[3]), float(a[4]), float(a[5])
FRAMES = [int(x) for x in a[6].split(',')]
GLB = a[7] if len(a) > 7 else r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer\SM_Musket.glb'
LENGTH = float(a[8]) if len(a) > 8 else 1.40
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
start = round(rig.animation_data.action.frame_range[0])
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=GLB)
new = [o for o in bpy.data.objects if o not in before]
rifle = next(o for o in new if o.type == 'MESH')
for o in new:
    if o is not rifle and o.type == 'EMPTY':
        pass
# rifle mesh: world-space vertices -> long axis (butt = thicker end guess: lower end of the long axis) -> normalise into a parent empty
bpy.context.view_layer.update()
pts = [rifle.matrix_world @ v.co for v in rifle.data.vertices]
lo = Vector((min(p[i] for p in pts) for i in range(3)))
hi = Vector((max(p[i] for p in pts) for i in range(3)))
ext = hi - lo
axis = max(range(3), key=lambda i: ext[i])
ctr = (lo + hi) / 2
print('RIFLE extents', tuple(ext), 'axis', axis)
holder = bpy.data.objects.new('rifle_holder', None)
bpy.context.scene.collection.objects.link(holder)
rifle.parent = holder
rifle.matrix_parent_inverse = Matrix.Identity(4)
# make the mesh's long axis +X, centred, length LENGTH (butt at -X), by building a fixed matrix
perm = {0: Matrix.Identity(4), 1: Matrix(((0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))), 2: Matrix(((0, 0, 1, 0), (0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 0, 1)))}[axis]
rifle.matrix_world = Matrix.Scale(LENGTH / ext[axis], 4) @ perm @ Matrix.Translation(-ctr) @ rifle.matrix_world
if 'FLIP' in __import__('os').environ:
    rifle.matrix_world = Matrix.Rotation(3.14159265, 4, 'Z') @ rifle.matrix_world
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
RS_ = float(__import__('os').environ.get('RES_SCALE', '1')); sc.render.resolution_x, sc.render.resolution_y = int(420 * RS_), int(560 * RS_)
sc.display.shading.light = 'STUDIO'
sc.display.shading.color_type = 'MATERIAL'
sc.world = bpy.data.worlds.new('w')
sc.world.color = (0.3, 0.32, 0.36)
l = bpy.data.objects.new('s', bpy.data.lights.new('s', 'SUN')); sc.collection.objects.link(l); l.rotation_euler = (0.9, 0.2, 0.6)
cam = bpy.data.objects.new('c', bpy.data.cameras.new('c')); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 3.0; cam.data.clip_end = 100


def H(n):
    return rig.matrix_world @ rig.pose.bones['mixamorig:' + n].head


up = Vector((0, 0, 1))
sc.frame_set(start)
bpy.context.view_layer.update()
right0 = H('RightShoulder') - H('LeftShoulder'); right0.z = 0; right0.normalize()
fwd0 = up.cross(right0).normalized()
orig = (H('LeftFoot') + H('RightFoot')) / 2; orig.z = 0
views = {'front': fwd0, 'side': right0, 'q34': (fwd0 + right0).normalized(), 'back': -fwd0}
for i in FRAMES:
    sc.frame_set(start + i)
    bpy.context.view_layer.update()
    side = (H('RightShoulder') - H('LeftShoulder')).normalized()
    f = up.cross(side).normalized()
    barrel = (H('RightShoulder') + f * CU + up * CS + side * CF - (rig.matrix_world @ ((rig.pose.bones['mixamorig:RightHand'].head + rig.pose.bones['mixamorig:RightHand'].tail) / 2))).normalized()
    z = (side - barrel * side.dot(barrel)).normalized()
    y = z.cross(barrel)
    m = Matrix((barrel, y, z)).transposed().to_4x4()
    pbh = rig.pose.bones['mixamorig:RightHand']
    hand = rig.matrix_world @ ((pbh.head + pbh.tail) / 2)
    grip = (GRIP - 0.5) * LENGTH
    m.translation = hand - barrel * grip
    holder.matrix_world = m
    for vn, d in views.items():
        pos = orig + d * 5 + up * 1.2
        cam.location = pos
        cam.rotation_euler = ((orig + up * 1.3) - pos).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{PREFIX}_{vn}_{i:02d}.png'
        bpy.ops.render.render(write_still=True)
    print('hand', tuple(hand), 'barrel', tuple(barrel))
print('V9_PREVIEW_DONE')
