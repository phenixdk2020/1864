"""Death animations for a soldier hit WHILE MARCHING IN COLUMN with the rifle on the right shoulder (2026-10-22).

    blender -b -P build_march_death_set.py -- [<out dir>] [<reference dir with 'Walking To Dying.fbx'>]            (MODE=build, default)
    MODE=verify blender -b -P build_march_death_set.py -- [<out dir>]       (re-imports the written FBX, measures, renders PNG/MP4/sheet)
    MODE=explore blender -b -P build_march_death_set.py -- [<out dir>]      (renders the retargeted SOURCE motions, used to pick keypoints)

Writes A_Combat_DeathMarchFront / DeathMarchBack / DeathMarchCrumple .fbx and march_death_report.json to <out dir>
(default Saved/Codex/march_death, NOT SourceAssets; copy to SourceAssets/InfantryCombat after checking).

Method
  * March pose: SourceAssets/InfantryCombat/A_Combat_WalkShoulder.fbx (30 frames, 30 fps, built by build_shoulder_arms_set_v9.py: rifle butt
    in the right hand in front of the hip, rifle on the right shoulder, Hips X/Z held). The same file (mesh + bind pose) is re-keyed and
    exported with the SAME exporter settings as v9 (axis -Z/Y, WITH skinned mesh, no leaf bones, keys 1..N, 30 fps), so the Unreal import
    is identical (import_rotation roll 90, see Content/Python/import_march_deaths.py).
  * Fall motion: borrowed from the existing Mixamo deaths (A_Combat_DeathFront / DeathBack / DeathHeadshot.fbx) and from the raw Mixamo
    'Walking To Dying.fbx' (same skeleton and bind pose as the walk). The armature-only death FBXs carry another rest pose, so they are
    retargeted by their FULL local transforms (rest_local @ basis) onto the walk rig; hips displacement is scaled by the leg-length ratio.
    Source frames are mapped piecewise-linearly to output frames (SPECS below), which compresses the fall to 1.6-2.6 s.
  * pose(i) = slerp(march cycle(i), death(i), smoothstep(i / BLEND)): the march keeps stepping during the blend, so frame 0 is EXACTLY
    WalkShoulder frame 0. The right arm (Arm/ForeArm/Hand) keeps the shoulder-arms carry for ARM_HOLD frames and then blends to the death
    motion (the rifle is a socket prop in game: the game must detach it when the arm lets go). The rig has no finger bones.
  * Hips: horizontal travel is limited by a soft scale of the displacement (FWD_LIM / BACK_LIM); then a per-frame hips lift keeps every
    skinned vertex above the ground level of the march (lowest vertex of WalkShoulder) and the final pose is settled onto the ground (+5 mm).
  * The last HOLD frames repeat the final pose exactly (lying still).
"""
import bpy, json, math, os, sys
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

a = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
SRC = PROJECT / 'SourceAssets/InfantryCombat'
OUT = Path(a[0]) if a else PROJECT / 'Saved/Codex/march_death'
REF = Path(a[1]) if len(a) > 1 else Path(r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer')
MODE = os.environ.get('MODE', 'build')
OUT.mkdir(parents=True, exist_ok=True)
FPS = 30
BLEND = 9          # frames (0.30 s) from the march pose into the death motion
ARM_HOLD = 7       # frames (~0.23 s) the right arm keeps the shoulder-arms carry
ARM_REL = 9        # frames the right arm then takes to blend to the death motion
HOLD = 13          # frames (~0.43 s) lying still at the end
FWD_LIM, BACK_LIM = 0.70, 0.45   # hips travel limits (m); spec: < 0.8 forward, <= 0.5 back
RIGHT_ARM = ['mixamorig:RightArm', 'mixamorig:RightForeArm', 'mixamorig:RightHand']
HIPS = 'mixamorig:Hips'
SOURCES = {'Front': SRC / 'A_Combat_DeathFront.fbx', 'Back': SRC / 'A_Combat_DeathBack.fbx', 'Headshot': SRC / 'A_Combat_DeathHeadshot.fbx',
           'BackHeadshot': SRC / 'A_Combat_DeathBackHeadshot.fbx', 'WalkDying': REF / 'Walking To Dying.fbx'}
# name -> (source key, [(out frame, source frame index 0-based), ...]); the last key is where the body has come to rest, HOLD frames follow.
# Source frame indices are 0-based positions in the source clip (see MODE=explore sheets).
SPECS = {
    'DeathMarchFront': ('Front', [(0, 4), (14, 30), (30, 52), (44, 68), (52, 80)]),
    'DeathMarchBack': ('Back', [(0, 4), (46, 52), (52, 66)]),
    'DeathMarchCrumple': ('BackHeadshot', [(0, 6), (20, 36), (28, 46), (36, 58), (50, 76), (62, 90)]),
}


def smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


def import_rig(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path))
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = arm.animation_data.action
    try:
        arm.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    for pb in arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
    return arm, act


def sample(arm, act):
    f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
    out = []
    for f in range(f0, f1 + 1):
        bpy.context.scene.frame_set(f)
        bpy.context.view_layer.update()
        out.append({pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones})
    return out


def rest_local(arm):
    r = {}
    for b in arm.data.bones:
        r[b.name] = (b.parent.matrix_local.inverted() @ b.matrix_local) if b.parent else b.matrix_local.copy()
    return r


def leg_len(rest):
    """Thigh + shin length from the chain of local rest offsets (works for any rest pose)."""
    return rest['mixamorig:LeftLeg'].translation.length + rest['mixamorig:LeftFoot'].translation.length


def read_source(path):
    arm, act = import_rig(path)
    return {'rest': rest_local(arm), 'frames': sample(arm, act), 'file': Path(path).name}


def retarget(src, rt, hips_ref, k_hips):
    """Copy FULL local rotations (rest_local @ basis) of the source onto the target rest pose; hips translation = ref + k * displacement."""
    rs, frames = src['rest'], src['frames']
    p0 = (rs[HIPS] @ frames[0][HIPS]).translation.copy()
    one = Vector((1, 1, 1))
    out = []
    for fr in frames:
        d = {}
        for name in rt:
            if name not in fr:
                d[name] = Matrix.Identity(4)
                continue
            full = rs[name] @ fr[name]
            rot = full.to_quaternion()
            if name == HIPS:
                loc = hips_ref + k_hips * (full.translation - p0)
                loc.y = k_hips * full.translation.y      # height: absolute (scaled by the leg-length ratio); X/Z: displacement from the first frame
                d[name] = rt[name].inverted() @ Matrix.LocRotScale(loc, rot, one)
            else:
                d[name] = Matrix.LocRotScale(Vector((0, 0, 0)), rt[name].to_quaternion().inverted() @ rot, one)
        out.append(d)
    return out


def blend2(pa, pb, w, only=None):
    out = {}
    for n, ma in pa.items():
        if w <= 0 or (only is not None and n not in only):
            out[n] = ma.copy()
            continue
        l0, r0, s0 = ma.decompose()
        l1, r1, s1 = pb[n].decompose()
        out[n] = Matrix.LocRotScale(l0.lerp(l1, w), r0.slerp(r1, w), s0.lerp(s1, w))
    return out


def at(frames, x):
    x = max(0.0, min(len(frames) - 1.0, x))
    i = int(math.floor(x))
    return blend2(frames[i], frames[min(i + 1, len(frames) - 1)], x - i)


def keymap(keys, i):
    if i <= keys[0][0]:
        return keys[0][1]
    for (o0, s0), (o1, s1) in zip(keys, keys[1:]):
        if i <= o1:
            return s0 + (s1 - s0) * (i - o0) / max(1e-9, (o1 - o0))
    return keys[-1][1]


class Ctx:
    def __init__(self, arm):
        self.arm = arm
        self.mesh = next(o for o in bpy.data.objects if o.type == 'MESH')

    def setpose(self, poses):
        for pb in self.arm.pose.bones:
            pb.matrix_basis = poses[pb.name]
        bpy.context.view_layer.update()

    def minz(self):
        """Lowest skinned vertex (world z, metres) of the current pose."""
        ev = self.mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
        me = ev.to_mesh()
        co = np.empty(len(me.vertices) * 3, dtype=np.float32)
        me.vertices.foreach_get('co', co)
        M = np.array(ev.matrix_world)
        z = co.reshape(-1, 3) @ M[2, :3] + M[2, 3]
        ev.to_mesh_clear()
        return float(z.min())

    def bone_minz(self):
        w = self.arm.matrix_world
        return min(min((w @ pb.head).z, (w @ pb.tail).z) for pb in self.arm.pose.bones)

    def hips_world(self):
        return self.arm.matrix_world @ self.arm.pose.bones[HIPS].head


def export_fbx(arm, path):
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


def bake_export(arm, name, frames):
    """Same as build_shoulder_arms_set_v9.bake(): keys 1..N on every bone, 30 fps, then the v9 exporter."""
    sc = bpy.context.scene
    arm.animation_data_clear()
    arm.animation_data_create()
    action = bpy.data.actions.new('A_Combat_' + name)
    arm.animation_data.action = action
    try:
        arm.animation_data.action_slot = action.slots[0] if action.slots else action.slots.new('OBJECT', arm.name)
    except Exception:
        pass
    sc.render.fps = FPS
    for i, poses in enumerate(frames, 1):
        for pb in arm.pose.bones:
            pb.matrix_basis = poses[pb.name]
            pb.keyframe_insert('location', frame=i)
            pb.keyframe_insert('rotation_quaternion', frame=i)
            pb.keyframe_insert('scale', frame=i)
    sc.frame_start, sc.frame_end = 1, len(frames)
    path = OUT / ('A_Combat_' + name + '.fbx')
    export_fbx(arm, path)
    return path


# ---------------------------------------------------------------- preview scene (Workbench, side view, facing right)
def setup_render(res=(512, 352)):
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.image_settings.file_format = 'PNG'
    sh = sc.display.shading
    sh.light, sh.color_type, sh.single_color = 'STUDIO', 'SINGLE', (0.55, 0.62, 0.45)
    sc.world = bpy.data.worlds.new('w')
    sc.world.color = (0.80, 0.85, 0.92)
    sc.view_settings.view_transform = 'Standard'

    def box(name, loc, size, col):
        bpy.ops.mesh.primitive_cube_add(location=loc)
        o = bpy.context.object
        o.name = name
        o.scale = size
        m = bpy.data.materials.new(name)
        m.diffuse_color = col
        o.data.materials.append(m)
    box('ground', (0, -0.5, -0.05), (2, 3.5, 0.05), (0.35, 0.3, 0.22, 1))
    box('mark0', (-0.12, 0, 0.2), (0.01, 0.015, 0.2), (0.9, 0.1, 0.1, 1))       # hips start (red)
    for yy in (-1.5, -1.0, -0.5, 0.5):
        box('mk%s' % yy, (-0.12, yy, 0.1), (0.01, 0.012, 0.1), (0.1, 0.1, 0.1, 1))   # every 0.5 m
    box('behind', (-0.12, 0.75, 0.35), (0.01, 0.012, 0.35), (0.95, 0.5, 0.0, 1))     # the man behind (75 cm, orange)
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
    sc.collection.objects.link(cam)
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 3.4
    cam.location = (-6, -0.3, 0.98)
    cam.rotation_euler = (math.radians(90), 0, math.radians(-90))
    sc.camera = cam


def render_png(path):
    bpy.context.scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


# ---------------------------------------------------------------- main
def load_all():
    srcs = {k: read_source(p) for k, p in SOURCES.items() if p.exists()}
    arm, act = import_rig(SRC / 'A_Combat_WalkShoulder.fbx')    # target LAST: the scene keeps this rig
    W = sample(arm, act)
    arm.animation_data_clear()    # poses are set by hand from here on (an action would override them on every frame change)
    ctx = Ctx(arm)
    rt = rest_local(arm)
    hips_ref = (rt[HIPS] @ W[0][HIPS]).translation.copy()
    mot = {}
    for k, s in srcs.items():
        kk = leg_len(rt) / leg_len(s['rest'])
        mot[k] = retarget(s, rt, hips_ref, kk)
        print('SOURCE', k, s['file'], len(mot[k]), 'frames, leg-length scale', round(kk, 3))
    return ctx, rt, W, mot


def build_clip(ctx, rt, W, motion, keys, ground):
    n_fall = keys[-1][0]
    N = n_fall + 1 + HOLD
    P = []
    for i in range(N):
        wk = W[i % len(W)]
        d = at(motion, keymap(keys, min(i, n_fall)))
        body = blend2(wk, d, smooth(i / BLEND))
        arm_w = smooth((i - ARM_HOLD) / ARM_REL)
        # right arm: carry (march frame, torso-relative) until ARM_HOLD, then the death motion
        arm_pose = blend2(wk, d, arm_w, only=set(RIGHT_ARM))
        for n in RIGHT_ARM:
            body[n] = arm_pose[n]
        P.append(body)
    # hips travel limit, in armature space (Z = forward, Y = up)
    full = [rt[HIPS] @ p[HIPS] for p in P]
    z0 = full[0].translation.z
    dz = [f.translation.z - z0 for f in full]
    sf = min(1.0, FWD_LIM / max(1e-6, max(dz) * 0.01))
    sb = min(1.0, BACK_LIM / max(1e-6, -min(dz) * 0.01))
    for i, f in enumerate(full):
        t = f.translation.copy()
        t.z = z0 + dz[i] * (sf if dz[i] > 0 else sb)
        full[i] = Matrix.LocRotScale(t, f.to_quaternion(), Vector((1, 1, 1)))
        P[i][HIPS] = rt[HIPS].inverted() @ full[i]
    # ground: lift (and settle) the hips so no skinned vertex is below the march ground
    ws = [smooth((i - (n_fall - 8)) / 8.0) for i in range(N)]
    for it in range(2):
        mz = []
        for i in range(N):
            ctx.setpose(P[i])
            mz.append(ctx.minz())
        off = [(1 - ws[i]) * max(0.0, ground - mz[i]) + ws[i] * (ground + 0.005 - mz[i]) for i in range(N)]
        r = 4
        mx = [max(off[max(0, i - r):i + r + 1]) if ws[i] < 1 else off[i] for i in range(N)]
        sm = [sum(mx[max(0, i - r):i + r + 1]) / len(mx[max(0, i - r):i + r + 1]) if ws[i] < 1 else mx[i] for i in range(N)]
        sm[0] = 0.0
        for i in range(N):
            f = rt[HIPS] @ P[i][HIPS]
            t = f.translation.copy()
            t.y += sm[i] * 100.0 / max(1e-9, ctx.arm.matrix_world.to_scale().y * 100.0)
            P[i][HIPS] = rt[HIPS].inverted() @ Matrix.LocRotScale(t, f.to_quaternion(), Vector((1, 1, 1)))
    for i in range(n_fall + 1, N):
        P[i] = {k: v.copy() for k, v in P[n_fall].items()}   # exactly still
    return P, {'fwd_scale': sf, 'back_scale': sb}


if MODE == 'explore':
    ctx, rt, W, mot = load_all()
    setup_render()
    d0 = OUT / 'explore'
    d0.mkdir(exist_ok=True)
    for k, m in mot.items():
        (d0 / k).mkdir(exist_ok=True)
        for i in range(0, len(m), 4):
            ctx.setpose(m[i])
            render_png(d0 / k / ('%03d.png' % i))
        ctx.setpose(m[-1])
        render_png(d0 / k / ('%03d.png' % (len(m) - 1)))
elif MODE == 'build':
    ctx, rt, W, mot = load_all()
    ground = 1e9
    for p in W:
        ctx.setpose(p)
        ground = min(ground, ctx.minz())
    print('GROUND (march lowest vertex z)', ground)
    rep = {'ground': ground, 'clips': {}}
    for name, (srck, keys) in SPECS.items():
        P, info = build_clip(ctx, rt, W, mot[srck], keys, ground)
        path = bake_export(ctx.arm, name, P)
        info.update({'file': path.name, 'frames': len(P), 'seconds': len(P) / FPS, 'source': srck, 'keys': keys})
        rep['clips'][name] = info
        print('BUILT', name, len(P), 'frames')
    (OUT / 'march_death_build.json').write_text(json.dumps(rep, indent=2))
elif MODE == 'verify':
    rep = {}
    names = ['WalkShoulder'] + list(SPECS)
    walkF0 = None
    for name in names:
        f = SRC / 'A_Combat_WalkShoulder.fbx' if name == 'WalkShoulder' else OUT / ('A_Combat_' + name + '.fbx')
        arm, act = import_rig(f)
        ctx = Ctx(arm)
        fr = sample(arm, act)
        setup_render()
        d = OUT / 'frames' / name
        d.mkdir(parents=True, exist_ok=True)
        f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
        mz, bz, hy, hd = [], [], [], []
        for i, f_ in enumerate(range(f0, f1 + 1)):
            bpy.context.scene.frame_set(f_)
            bpy.context.view_layer.update()
            mz.append(ctx.minz())
            bz.append(ctx.bone_minz())
            hy.append(ctx.hips_world())
            hd.append(arm.matrix_world @ arm.pose.bones['mixamorig:Head'].head)
            render_png(d / ('%03d.png' % i))
        # forward = -world Y (character faces -Y)
        fwd = [-(h.y - hy[0].y) for h in hy]
        r = {'frames': len(fr), 'seconds': len(fr) / FPS, 'max_forward_m': max(fwd), 'max_back_m': -min(fwd), 'max_lateral_m': max(abs(h.x - hy[0].x) for h in hy),
             'min_vertex_z_m': min(mz), 'final_min_vertex_z_m': mz[-1], 'final_min_bone_z_m': bz[-1], 'min_bone_z_m': min(bz),
             'final_hips_z_m': hy[-1].z, 'last_frames_still': max((fr[-1][k].to_euler().x - fr[-HOLD][k].to_euler().x) ** 2 for k in fr[-1]) if len(fr) > HOLD else None,
             'max_head_back_m': max(h.y - hd[0].y for h in hd), 'max_head_forward_m': max(-(h.y - hd[0].y) for h in hd),
             'final_joint_z_m': {n: round((arm.matrix_world @ arm.pose.bones['mixamorig:' + n].head).z, 3) for n in
                                 ('Hips', 'Head', 'LeftFoot', 'RightFoot', 'LeftToeBase', 'RightToeBase', 'LeftHand', 'RightHand', 'Spine2')},
             'final_lowest_bone': min(((arm.matrix_world @ pb.head).z, pb.name) for pb in arm.pose.bones)[1],
             'min_vertex_z_per_frame_cm': [round(v * 100, 1) for v in mz], 'min_bone_z_per_frame_cm': [round(v * 100, 1) for v in bz]}
        if name == 'WalkShoulder':
            walkF0 = fr[0]
            r['ground'] = min(mz)
        else:
            diff = 0.0
            for k in walkF0:
                diff = max(diff, max(abs(x - y) for rw1, rw2 in zip(walkF0[k], fr[0][k]) for x, y in zip(rw1, rw2)))
            r['frame0_vs_walkshoulder_max_basis_diff'] = diff
        rep[name] = r
        print('VERIFY', name, {k: v for k, v in r.items() if not k.endswith('per_frame_cm')})
    (OUT / 'march_death_report.json').write_text(json.dumps(rep, indent=2))
    import shutil, subprocess
    py = shutil.which('python') or shutil.which('py')
    if py and os.environ.get('NOPREVIEW') != '1':
        subprocess.run([py, '-I', str(Path(__file__).with_name('march_death_assemble.py')), str(OUT), str(SRC / 'Preview'), str(BLEND)], check=False)
