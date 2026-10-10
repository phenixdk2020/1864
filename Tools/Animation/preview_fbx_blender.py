"""Blender (headless) preview of an animation FBX: prints skeleton/frame info and renders a contact sheet.

    blender -b -P preview_fbx_blender.py -- <in.fbx> <out_prefix> [frames=6]
"""
import sys
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
src, out = argv[0], argv[1]
count = int(argv[2]) if len(argv) > 2 else 6

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src)
arm = next((o for o in bpy.data.objects if o.type == 'ARMATURE'), None)
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
print("ARMATURE", arm.name if arm else None, "bones", len(arm.data.bones) if arm else 0, "meshes", len(meshes))
if arm and arm.animation_data and arm.animation_data.action:
    act = arm.animation_data.action
    f0, f1 = act.frame_range
    print("ACTION", act.name, "frames", f0, f1, "fps", bpy.context.scene.render.fps)
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = int(f0), int(f1)
else:
    f0, f1 = 1, 30
hips = None
if arm:
    for name in ("mixamorig:Hips", "Hips", "hips"):
        if name in arm.pose.bones:
            hips = arm.pose.bones[name]
            break
    if hips is None and len(arm.pose.bones):
        hips = arm.pose.bones[0]
if hips:
    for f in (int(f0), int((f0 + f1) / 2), int(f1)):
        bpy.context.scene.frame_set(f)
        print("HIPS", hips.name, "frame", f, "world", tuple(round(v, 2) for v in (arm.matrix_world @ hips.matrix @ Vector((0, 0, 0)))))

cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
bpy.context.scene.collection.objects.link(cam)
bpy.context.scene.camera = cam
light = bpy.data.objects.new("sun", bpy.data.lights.new("sun", 'SUN'))
bpy.context.scene.collection.objects.link(light)
light.rotation_euler = (0.9, 0.2, 0.6)
scene = bpy.context.scene
scene.render.engine = 'BLENDER_WORKBENCH'
scene.render.resolution_x, scene.render.resolution_y = 512, 512
scene.display.shading.light = 'STUDIO'
scene.world = bpy.data.worlds.new("w")
scene.world.color = (0.2, 0.22, 0.25)
# frame the whole motion: bounding box over a few frames
pts = []
for f in range(int(f0), int(f1) + 1, max(1, int((f1 - f0) // 6) or 1)):
    scene.frame_set(f)
    for m in meshes:
        pts += [m.matrix_world @ Vector(c) for c in m.bound_box]
    if not meshes and arm:
        pts += [arm.matrix_world @ b.head for b in arm.pose.bones]
if pts:
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    mid = (lo + hi) / 2
    size = max((hi - lo).length, 1.0)
else:
    mid, size = Vector((0, 0, 1)), 3.0
cam.location = mid + Vector((0, -size * 1.4, size * 0.2))
direction = mid - cam.location
cam.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
for i in range(count):
    f = int(f0 + (f1 - f0) * i / max(1, count - 1))
    scene.frame_set(f)
    scene.render.filepath = f"{out}_{i}.png"
    bpy.ops.render.render(write_still=True)
print("DONE")
