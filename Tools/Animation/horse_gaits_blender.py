"""Procedural horse animations on the rigged horse (Horse_1_rigged_v3.blend): stand, walk, trot, canter, gallop, rear, fall, lie.

Run headless:
  blender -b "<...>/Horse_1_rigged_v3.blend" -P Tools/Animation/horse_gaits_blender.py -- <out_dir> [preview_dir]

Every clip is in place (no root motion), 30 fps, loops are exact (frame N+1 == frame 1). Foot IK controllers drive the legs,
the body bones get bob / pitch / swing. Each clip is exported as an animation FBX (deform bones only, IK baked) for Unreal
and optionally a 6-frame contact sheet is rendered to preview_dir. The horse looks along -X, up is +Z, left is -Y.
"""
import math
import os
import sys
import bpy
from mathutils import Vector

FPS = 30
OUT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else "C:/tmp/horse_out"
PREVIEW = sys.argv[sys.argv.index("--") + 2] if "--" in sys.argv and len(sys.argv) > sys.argv.index("--") + 2 else None

# name: cycle seconds, stride (fraction of body height), lift, stance fraction, leg phases, bob amplitude (fraction of height), pitch (rad), bob harmonics
GAITS = {
    "Walk":   dict(T=1.10, stride=0.55, lift=0.05, stance=0.66, phases={"hind.L": 0.0, "front.L": 0.25, "hind.R": 0.5, "front.R": 0.75}, bob=0.006, bobh=2, pitch=0.02),
    "Trot":   dict(T=0.70, stride=0.70, lift=0.08, stance=0.46, phases={"hind.L": 0.0, "front.R": 0.0, "hind.R": 0.5, "front.L": 0.5}, bob=0.020, bobh=2, pitch=0.015),
    "Canter": dict(T=0.62, stride=0.95, lift=0.11, stance=0.34, phases={"hind.R": 0.0, "hind.L": 0.30, "front.R": 0.30, "front.L": 0.62}, bob=0.045, bobh=1, pitch=0.07),
    "Gallop": dict(T=0.48, stride=1.20, lift=0.13, stance=0.28, phases={"hind.R": 0.0, "hind.L": 0.10, "front.R": 0.42, "front.L": 0.55}, bob=0.060, bobh=1, pitch=0.10),
}
LEGS = ("hind.L", "hind.R", "front.L", "front.R")


def fcurves_of(action):
    curves = []
    if hasattr(action, "layers"):
        for layer in action.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    curves.extend(bag.fcurves)
    else:
        curves = list(action.fcurves)
    return curves


class Rig:
    def __init__(self):
        cands = [o for o in bpy.context.view_layer.objects if o.type == 'ARMATURE' and 'front_cannon.L' in o.data.bones and 'hind_cannon.R' in o.data.bones]
        self.arm = cands[0]
        self.meshes = [o for o in bpy.context.view_layer.objects if o.type == 'MESH' and any(m.type == 'ARMATURE' and m.object == self.arm for m in o.modifiers)]
        print('RIG', self.arm.name, [m.name for m in self.meshes], [o.name for o in cands])
        bpy.context.view_layer.objects.active = self.arm
        verts = [self.arm.matrix_world.inverted() @ o.matrix_world @ v.co for o in self.meshes for v in o.data.vertices]
        self.height = max(v.z for v in verts) - min(v.z for v in verts)
        if self.arm.animation_data:
            self.arm.animation_data_clear()
        bpy.ops.object.mode_set(mode='EDIT')
        self.ctrl = {}
        for leg in LEGS:
            kind, side = leg.split('.')
            pastern = self.arm.data.edit_bones[f'{kind}_pastern.{side}']
            b = self.arm.data.edit_bones.new(f'CTRL_{kind}_foot.{side}')
            b.head, b.tail, b.roll = pastern.head.copy(), pastern.tail.copy(), pastern.roll
            b.use_deform = False
            self.ctrl[leg] = b.name
        bpy.ops.object.mode_set(mode='OBJECT')
        for leg, name in self.ctrl.items():
            kind, side = leg.split('.')
            cannon = self.arm.pose.bones[f'{kind}_cannon.{side}']
            ik = cannon.constraints.new('IK')
            ik.target, ik.subtarget = self.arm, name
            ik.chain_count = 3
            ik.use_stretch = False
            ik.iterations = 128
            for pb in (cannon, cannon.parent, cannon.parent.parent):
                pb.ik_stretch = 0
            pastern = self.arm.pose.bones[f'{kind}_pastern.{side}']
            orient = pastern.constraints.new('COPY_ROTATION')
            orient.target, orient.subtarget = self.arm, name
            orient.target_space = 'POSE'
            orient.owner_space = 'POSE'
        for pb in self.arm.pose.bones:
            pb.rotation_mode = 'XYZ'

    def reset(self):
        for pb in self.arm.pose.bones:
            pb.location = (0, 0, 0)
            pb.rotation_euler = (0, 0, 0)
            pb.scale = (1, 1, 1)

    def loc(self, name, v):
        pb = self.arm.pose.bones[name]
        pb.location = pb.bone.matrix_local.to_3x3().inverted() @ Vector(v)

    def key(self, name, frame, loc=True, rot=True):
        pb = self.arm.pose.bones[name]
        if loc:
            pb.keyframe_insert('location', frame=frame, group=name)
        if rot:
            pb.keyframe_insert('rotation_euler', frame=frame, group=name)

    def new_action(self, name):
        self.reset()
        self.arm.animation_data_create()
        action = bpy.data.actions.new(name)
        self.arm.animation_data.action = action
        return action

    def finish(self, action, frames, loop=True):
        for fc in fcurves_of(action):
            for k in fc.keyframe_points:
                k.interpolation = 'LINEAR'
            if loop:
                fc.modifiers.new('CYCLES')
        action.use_fake_user = True
        bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, frames


def foot_path(phase, stance, stride, lift):
    phase %= 1.0
    if phase < stance:
        return stride * (phase / stance - 0.5), 0.0
    t = (phase - stance) / (1 - stance)
    tangent = (1 - stance) / stance
    x = (2 * t ** 3 - 3 * t * t + 1) * 0.5 + (t ** 3 - 2 * t * t + t) * tangent
    x += (-2 * t ** 3 + 3 * t * t) * (-0.5) + (t ** 3 - t * t) * tangent
    return stride * x, lift * math.sin(math.pi * t) ** 2


def make_gait(rig, name, g):
    frames = max(8, int(round(g['T'] * FPS)))
    action = rig.new_action(name)
    stride, lift = rig.height * g['stride'] * 0.5, rig.height * g['lift']
    for frame in range(1, frames + 2):
        t = (frame - 1) / frames
        for leg, ctrl in rig.ctrl.items():
            x, z = foot_path(t - g['phases'][leg], g['stance'], stride, lift)
            rig.loc(ctrl, (x, 0, z))
            rig.key(ctrl, frame, rot=False)
        bob = rig.height * g['bob'] * math.sin(2 * math.pi * g['bobh'] * t + math.pi / 2)
        rig.loc('pelvis', (0, 0, bob))
        rig.key('pelvis', frame, rot=False)
        # spine pitch (nose up/down about the horse's Y axis) follows the bob, neck counter-balances
        pitch = g['pitch'] * math.sin(2 * math.pi * g['bobh'] * t)
        for bone, k in (('spine_01', 0.5), ('spine_02', 0.5), ('chest', 0.5)):
            pb = rig.arm.pose.bones[bone]
            pb.rotation_euler = (0, k * pitch, 0)
            rig.key(bone, frame, loc=False)
        for bone, k, ph in (('neck_01', -0.5, 0.4), ('neck_02', -0.6, 0.6), ('head', -0.3, 0.8)):
            pb = rig.arm.pose.bones[bone]
            pb.rotation_euler = (0, k * pitch * 1.6 + 0.02 * math.sin(2 * math.pi * t + ph), 0)
            rig.key(bone, frame, loc=False)
        for bone, a, ph in (('tail_01', 0.10, 0.0), ('tail_02', 0.14, 0.4), ('tail_03', 0.18, 0.8)):
            pb = rig.arm.pose.bones[bone]
            pb.rotation_euler = (a * math.sin(2 * math.pi * t * max(1, g['bobh'] // 1) + ph), 0, 0)
            rig.key(bone, frame, loc=False)
    rig.finish(action, frames)
    return action, frames


def make_stand(rig):
    frames = 120
    action = rig.new_action("Stand")
    for frame in range(1, frames + 2):
        t = (frame - 1) / frames
        for leg, ctrl in rig.ctrl.items():
            rig.loc(ctrl, (0, 0, 0))
            rig.key(ctrl, frame, rot=False)
        breath = math.sin(2 * math.pi * t * 2)
        rig.loc('pelvis', (0, 0, rig.height * 0.0015 * breath))
        rig.key('pelvis', frame, rot=False)
        for bone, a in (('chest', 0.012), ('spine_02', 0.006)):
            rig.arm.pose.bones[bone].rotation_euler = (0, a * breath, 0)
            rig.key(bone, frame, loc=False)
        rig.arm.pose.bones['neck_02'].rotation_euler = (0, 0.03 * math.sin(2 * math.pi * t), 0.04 * math.sin(2 * math.pi * t * 1))
        rig.key('neck_02', frame, loc=False)
        rig.arm.pose.bones['head'].rotation_euler = (0, 0.04 * math.sin(2 * math.pi * t + 1.0), 0.05 * math.sin(2 * math.pi * t * 2 + 0.5))
        rig.key('head', frame, loc=False)
        for bone, a, ph in (('ear.L', 0.15, 0.0), ('ear.R', 0.15, 2.0)):
            rig.arm.pose.bones[bone].rotation_euler = (a * max(0.0, math.sin(2 * math.pi * t * 3 + ph)) ** 4, 0, 0)
            rig.key(bone, frame, loc=False)
        for bone, a, ph in (('tail_01', 0.08, 0.0), ('tail_02', 0.12, 0.5), ('tail_03', 0.16, 1.0)):
            rig.arm.pose.bones[bone].rotation_euler = (a * math.sin(2 * math.pi * t * 2 + ph), 0, 0)
            rig.key(bone, frame, loc=False)
    rig.finish(action, frames)
    return action, frames


def smooth(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def make_rear(rig):
    frames = int(2.4 * FPS)
    action = rig.new_action("Rear")
    for frame in range(1, frames + 1):
        t = (frame - 1) / (frames - 1)
        up = smooth(t / 0.35) * (1 - smooth((t - 0.6) / 0.4))
        for leg, ctrl in rig.ctrl.items():
            if leg.startswith('front'):
                rig.loc(ctrl, (rig.height * 0.10 * up, 0, rig.height * 0.55 * up))
            else:
                rig.loc(ctrl, (rig.height * 0.20 * up, 0, 0))
            rig.key(ctrl, frame, rot=False)
        rig.loc('pelvis', (rig.height * 0.10 * up, 0, -rig.height * 0.20 * up))
        rig.key('pelvis', frame, rot=False)
        for bone, k in (('spine_01', 0.22), ('spine_02', 0.22), ('chest', 0.18), ('neck_01', -0.20), ('neck_02', -0.25), ('head', -0.15)):
            rig.arm.pose.bones[bone].rotation_euler = (0, k * up * 1.4, 0)
            rig.key(bone, frame, loc=False)
    rig.finish(action, frames, loop=False)
    return action, frames


def make_fall(rig):
    frames = int(2.0 * FPS)
    action = rig.new_action("Fall")
    for frame in range(1, frames + 1):
        t = (frame - 1) / (frames - 1)
        down = smooth(t / 0.6)
        roll = smooth((t - 0.15) / 0.6)
        for leg, ctrl in rig.ctrl.items():
            fold = 0.5 if leg.startswith('front') else 0.3
            rig.loc(ctrl, (0, 0, rig.height * 0.25 * down * fold))
            rig.key(ctrl, frame, rot=False)
        rig.loc('pelvis', (0, 0, -rig.height * 0.34 * down))
        rig.key('pelvis', frame, rot=False)
        rig.arm.pose.bones['root'].rotation_euler = (math.radians(88) * roll, 0, 0)
        rig.loc('root', (0, 0, 0))
        rig.key('root', frame)
        for bone, k in (('neck_01', 0.3), ('neck_02', 0.35), ('head', 0.2)):
            rig.arm.pose.bones[bone].rotation_euler = (0, k * down, 0)
            rig.key(bone, frame, loc=False)
    rig.finish(action, frames, loop=False)
    return action, frames


def export(rig, action, name, frames, loop):
    rig.reset()   # unkeyed channels keep their last pose value: start every clip from the rest pose
    rig.arm.animation_data.action = action
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, frames
    bpy.context.scene.render.fps = FPS
    for o in bpy.data.objects:
        o.select_set(False)
    rig.arm.select_set(True)
    bpy.context.view_layer.objects.active = rig.arm
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f"A_Horse_{name}.fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'ARMATURE'}, use_armature_deform_only=True,
                             add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                             bake_anim_simplify_factor=0.0, bake_anim_step=1.0, bake_anim_force_startend_keying=True,
                             apply_unit_scale=True, axis_forward='-Z', axis_up='Y')
    print("EXPORTED", path, frames)


def preview(rig, action, name, frames):
    if not PREVIEW:
        return
    os.makedirs(PREVIEW, exist_ok=True)
    scene = bpy.context.scene
    rig.reset()
    rig.arm.animation_data.action = action
    cam = bpy.data.objects.get('PREVIEW_CAM')
    if cam is None:
        cam = bpy.data.objects.new('PREVIEW_CAM', bpy.data.cameras.new('PREVIEW_CAM'))
        scene.collection.objects.link(cam)
        sun = bpy.data.objects.new('PREVIEW_SUN', bpy.data.lights.new('PREVIEW_SUN', 'SUN'))
        scene.collection.objects.link(sun)
        sun.rotation_euler = (0.9, 0.2, 0.6)
    scene.camera = cam
    mid = Vector((0.2, 0, rig.height * 0.5))
    cam.location = mid + Vector((0.0, -rig.height * 3.2, rig.height * 0.25))
    cam.rotation_euler = (mid - cam.location).to_track_quat('-Z', 'Y').to_euler()
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.render.resolution_x, scene.render.resolution_y = 480, 360
    scene.display.shading.light = 'STUDIO'
    for i in range(6):
        scene.frame_set(1 + int((frames - 1) * i / 6))
        scene.render.filepath = os.path.join(PREVIEW, f"{name}_{i}.png")
        bpy.ops.render.render(write_still=True)


def main():
    rig = Rig()
    done = []
    for name, g in GAITS.items():
        action, frames = make_gait(rig, f"Horse_{name}", g)
        done.append((name, action, frames, True))
    done.append(("Stand",) + make_stand(rig) + (True,))
    done.append(("Rear",) + make_rear(rig) + (False,))
    done.append(("Fall",) + make_fall(rig) + (False,))
    for name, action, frames, loop in done:
        export(rig, action, name, frames, loop)
        preview(rig, action, name, frames)
    print("HORSE DONE")


if __name__ == "__main__":
    main()
