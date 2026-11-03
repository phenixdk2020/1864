"""Rider clips for the skeletal horse: the 'Reload sitting' seat moved with the horse's own gait clips.
    blender -b -P build_cavalry_rider_set.py -- <MultiView dir>
Writes SourceAssets/InfantryCombat/A_Combat_Cav{Walk,TrotRise,Canter,Gallop,Charge,Fall}.fbx + manifest_cav_rider.json
(import_infantry_combat_set.py reads that manifest too) and preview images in SourceAssets/InfantryCombat/Preview/Cav*.

Method (as build_cavalry_sabre_set.py): pelvis and legs from frame 3 of 'Reload sitting', then per frame
  - the pelvis is moved in armature space (up / forward / pitch), driven by the real pelvis curve of the matching A_Horse_*.fbx
    (two frames of lag) plus the gait's own rise (rising trot, two-point seat at the gallop, standing in the stirrups at the charge);
  - the spine is leant forward, spread over Spine/Spine1/Spine2;
  - both legs are solved with a two-bone IK so the feet stay where the seat has them (the legs hold the rider in the stirrups);
  - CavFall throws the rider clear to his right (the horse falls to its left in A_Horse_Fall) and lays him on his back.
Every clip has exactly as many frames as its horse clip, so one play rate drives horse and rider together (30 fps, in place).
In game the rider sits on the horse's saddle_root bone, so the horse's own bounce is already in his position: the clips hold only
the rider's movement relative to the saddle (follow at the walk, absorb at the canter/gallop/charge). Preview images show the sum.
Armature space (Mixamo): +Y up, +Z forward, +X the rider's left; units cm. Previous versions of existing FBX are kept as .prev.fbx.
"""
import bpy, json, math, shutil, sys
from pathlib import Path
from mathutils import Matrix, Vector

root = Path(sys.argv[sys.argv.index('--') + 1])
project = Path(__file__).resolve().parents[2]
output = project / 'SourceAssets/InfantryCombat'
previews = output / 'Preview'
horse_dir = project / 'Reference/Animations/Horse'
SEAT_CLIP = root / 'Animationer Mænd' / 'Reload sitting.fbx'
HORSE_SCALE = 1.1            # SK_Horse_1864 is scaled to ~220 cm in the battle; the preview and the fall ground assume the same
SADDLE_LOCAL = 1.182         # saddle_root height in the rigged horse (m)
HIPS_Y = 48.7                # the seat pose's pelvis above its root (cm) = RiderSeatCm in the visual component
GROUND_Y = -(SADDLE_LOCAL * HORSE_SCALE * 100.0 - HIPS_Y)   # the ground in the rider's own space when he sits on the saddle (cm)
X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))


# ------------------------------------------------------------------ horse curves (for timing and for the preview)
def load_fbx(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path))
    rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = rig.animation_data.action
    try:
        rig.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    return rig, int(act.frame_range[0]), int(act.frame_range[1])


def horse_clip(name):
    rig, s, e = load_fbx(horse_dir / ('A_Horse_%s.fbx' % name))
    frames = []
    for fr in range(s, e + 1):
        bpy.context.scene.frame_set(fr)
        w = rig.matrix_world
        d = {b.name: (w @ b.head, w @ b.tail) for b in rig.pose.bones}
        p, c = d['pelvis'][0], d['chest'][0]
        direction = c - p
        up = Vector((-direction.z, 0, direction.x)) if direction.x < 0 else Vector((direction.z, 0, -direction.x))
        up = up.normalized()
        if up.z < 0:
            up = -up
        saddle = p.lerp(c, 0.675) + up * 0.113
        pitch = math.atan2(direction.z, -direction.x)       # nose-up positive (the horse looks along -X)
        frames.append({'bones': d, 'pelvis_z': p.z, 'pitch': pitch, 'saddle': saddle})
    return frames


HORSE = {n: horse_clip(n) for n in ('Walk', 'Trot', 'Canter', 'Gallop', 'Fall')}


def horse_drive(name, lag=2):
    """Per frame: the horse pelvis rise (cm, mean removed) and pitch (rad, mean removed), `lag` frames late and looping."""
    fr = HORSE[name]
    n = len(fr)
    mz = sum(f['pelvis_z'] for f in fr) / n
    mp = sum(f['pitch'] for f in fr) / n
    return [((fr[(i - lag) % n]['pelvis_z'] - mz) * 100.0, fr[(i - lag) % n]['pitch'] - mp) for i in range(n)]


# ------------------------------------------------------------------ rider rig and seat
rig, s0, e0 = load_fbx(SEAT_CLIP)
bpy.context.scene.frame_set(s0 + 3)
names = [pb.name for pb in rig.pose.bones]
short = {n.split(':')[-1]: n for n in names}
parent = {pb.name: (pb.parent.name if pb.parent else None) for pb in rig.pose.bones}
rest = {b.name: b.matrix_local.copy() for b in rig.data.bones}
blen = {b.name: b.length for b in rig.data.bones}
seat_basis = {pb.name: pb.matrix_basis.copy() for pb in rig.pose.bones}
children = {n: [] for n in names}
for n, p in parent.items():
    if p:
        children[p].append(n)


def fk(basis):
    M = {}
    for n in names:
        p = parent[n]
        M[n] = (rest[n] @ basis[n]) if not p else (M[p] @ (rest[p].inverted() @ rest[n]) @ basis[n])
    return M


def to_basis(M):
    out = {}
    for n in names:
        p = parent[n]
        out[n] = (rest[n].inverted() @ M[n]) if not p else ((rest[p].inverted() @ rest[n]).inverted() @ M[p].inverted() @ M[n])
    return out


SEAT = fk(seat_basis)
check = max((SEAT[pb.name].translation - Vector(pb.head)).length for pb in rig.pose.bones)
print('SEAT_FK_CHECK_CM', round(check, 4))
assert check < 0.05, 'FK does not reproduce the seat pose'


def subtree(n):
    out, stack = [], [n]
    while stack:
        c = stack.pop()
        out.append(c)
        stack.extend(children[c])
    return out


def transform(M, bone, T):
    for n in subtree(bone):
        M[n] = T @ M[n]


def about(pivot, axis, angle):
    return Matrix.Translation(pivot) @ Matrix.Rotation(angle, 4, axis) @ Matrix.Translation(-pivot)


def rot_between(a, b):
    return a.rotation_difference(b).to_matrix().to_4x4()


def ik_leg(M, side):
    up, leg, foot = short[side + 'UpLeg'], short[side + 'Leg'], short[side + 'Foot']
    hip = M[up].translation.copy()
    knee_seat, foot_target, hip_seat = SEAT[leg].translation, SEAT[foot].translation, SEAT[up].translation
    l1, l2 = (knee_seat - hip_seat).length, (foot_target - knee_seat).length
    v = foot_target - hip
    d = min(v.length, (l1 + l2) * 0.999)
    direction = v.normalized()
    pole = knee_seat - hip_seat
    pole -= direction * pole.dot(direction)
    pole = pole.normalized() if pole.length > 1e-4 else Z.copy()
    a = (l1 * l1 - l2 * l2 + d * d) / (2 * d)
    knee = hip + direction * a + pole * math.sqrt(max(l1 * l1 - a * a, 0.0))
    target = hip + direction * d
    current_knee = M[leg].translation.copy()
    R = rot_between(current_knee - hip, knee - hip)
    transform(M, up, Matrix.Translation(hip) @ R @ Matrix.Translation(-hip))
    k = M[leg].translation.copy()
    R2 = rot_between(M[foot].translation - k, target - k)
    transform(M, leg, Matrix.Translation(k) @ R2 @ Matrix.Translation(-k))
    f = M[foot].translation.copy()
    R3 = (SEAT[foot].to_3x3() @ M[foot].to_3x3().inverted()).to_4x4()
    transform(M, foot, Matrix.Translation(f) @ R3 @ Matrix.Translation(-f))
    M[foot].translation = target


def pose(up=0.0, fwd=0.0, side=0.0, hips_pitch=0.0, hips_roll=0.0, lean=0.0, spine_roll=0.0, legs=True, leg_drop=0.0):
    """Seat pose moved: cm / radians. lean > 0 leans the upper body forward (spread over three spine bones)."""
    M = {n: SEAT[n].copy() for n in names}
    hips = short['Hips']
    p = M[hips].translation.copy()
    T = Matrix.Translation(Vector((side, up, fwd))) @ about(p, X, hips_pitch) @ about(p, Z, hips_roll)
    transform(M, hips, T)
    for bone, frac in (('Spine', 0.30), ('Spine1', 0.35), ('Spine2', 0.35)):
        n = short[bone]
        pv = M[n].translation.copy()
        transform(M, n, about(pv, X, lean * frac) @ about(pv, Z, spine_roll * frac))
    if leg_drop:
        for side_name in ('Left', 'Right'):
            n = short[side_name + 'UpLeg']
            transform(M, n, about(M[n].translation.copy(), X, leg_drop))
    if legs:
        ik_leg(M, 'Left')
        ik_leg(M, 'Right')
    return M


# ------------------------------------------------------------------ the clips
def smoothstep(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


def clip_walk():
    drive = horse_drive('Walk')
    n = len(drive)
    out = []
    for i in range(n):
        t = i / n
        dz, dp = drive[i]
        out.append(pose(up=dz * 0.3, fwd=0.9 * math.sin(2 * math.pi * t), side=0.8 * math.sin(2 * math.pi * t + 0.4),
                        hips_pitch=-0.8 * dp, lean=math.radians(1.5) + 0.6 * dp, spine_roll=math.radians(1.0) * math.sin(2 * math.pi * t)))
    return out


def clip_trot_rise():
    drive = horse_drive('Trot')
    n = len(drive)
    out = []
    for i in range(n):
        t = i / n
        dz, dp = drive[i]
        u = 0.5 - 0.5 * math.cos(2 * math.pi * (t - 0.05))            # one rise per stride (rising trot: up on one diagonal, down on the other)
        out.append(pose(up=9.0 * u ** 1.2 + dz * 0.3 * (1 - u), fwd=3.5 * u, hips_pitch=-0.6 * dp,
                        lean=math.radians(4.0 + 7.0 * u), spine_roll=math.radians(1.0) * math.sin(2 * math.pi * t)))
    return out


def clip_canter():
    drive = horse_drive('Canter')
    n = len(drive)
    out = []
    for i in range(n):
        t = i / n
        dz, dp = drive[i]
        out.append(pose(up=1.5 - dz * 0.4, fwd=3.5 * math.sin(2 * math.pi * t - 0.6), hips_pitch=-1.1 * dp,
                        lean=math.radians(8.0) + 1.2 * dp, spine_roll=math.radians(1.5) * math.sin(2 * math.pi * t)))
    return out


def clip_gallop():
    drive = horse_drive('Gallop')
    n = len(drive)
    out = []
    for i in range(n):
        t = i / n
        dz, dp = drive[i]
        out.append(pose(up=6.0 - dz * 0.6, fwd=-3.0 + 2.5 * math.sin(2 * math.pi * t - 0.3), hips_pitch=-0.9 * dp,
                        lean=math.radians(17.0) + 0.8 * dp, spine_roll=math.radians(1.0) * math.sin(2 * math.pi * t)))
    return out


def clip_charge():
    drive = horse_drive('Gallop')
    n = len(drive)
    out = []
    for i in range(n):
        t = i / n
        dz, dp = drive[i]
        out.append(pose(up=14.0 - dz * 0.8, fwd=-2.0 + 2.0 * math.sin(2 * math.pi * t - 0.3), hips_pitch=-0.6 * dp,
                        lean=math.radians(13.0) + 0.5 * dp, spine_roll=math.radians(0.8) * math.sin(2 * math.pi * t)))
    return out


def clip_fall():
    n = len(HORSE['Fall'])           # 60 frames = 2.0 s, as the horse; the horse is down at about 75 % of it
    out = []
    for i in range(n):
        t = i / (n - 1)
        tip = smoothstep(t / 0.2)                                   # sits on while the horse stumbles (it drops 20 cm in the first quarter)
        s = smoothstep((t - 0.15) / 0.60)                           # thrown clear
        arc = 4.0 * s * (1 - s)
        land = smoothstep((t - 0.65) / 0.25)
        hips_y = (GROUND_Y + 28.0 - HIPS_Y) * s * s + 38.0 * arc - 8.0 * tip * (1 - s)
        fwd = 105.0 * (s ** 1.3)
        side = -75.0 * s                                            # the rider's right, away from the horse's left-hand fall
        pitch = -math.radians(88.0) * smoothstep((t - 0.25) / 0.55) + math.radians(25.0) * math.sin(math.pi * min(1.0, (t - 0.15) / 0.3 if t > 0.15 else 0.0))
        roll = -math.radians(25.0) * s
        bounce = 3.0 * math.sin(math.pi * land) * (1 - land)
        out.append(pose(up=hips_y + bounce, fwd=fwd, side=side, hips_pitch=pitch, hips_roll=roll, lean=-math.radians(8.0) * s, legs=False,
                        leg_drop=math.radians(80.0) * smoothstep((t - 0.55) / 0.30)))
    return out


CLIPS = [('CavWalk', 'Walk', clip_walk), ('CavTrotRise', 'Trot', clip_trot_rise), ('CavCanter', 'Canter', clip_canter),
         ('CavGallop', 'Gallop', clip_gallop), ('CavCharge', 'Gallop', clip_charge), ('CavFall', 'Fall', clip_fall)]


# ------------------------------------------------------------------ bake and export (same settings as build_cavalry_sabre_set.py)
def bake(name, frames_M):
    rig.animation_data_clear()
    rig.animation_data_create()
    act = bpy.data.actions.new('A_Combat_' + name)
    rig.animation_data.action = act
    try:
        rig.animation_data.action_slot = act.slots[0] if act.slots else act.slots.new('OBJECT', rig.name)
    except Exception:
        pass
    for i, M in enumerate(frames_M, 1):
        basis = to_basis(M)
        for b in rig.pose.bones:
            b.rotation_mode = 'QUATERNION'
            b.matrix_basis = basis[b.name]
            b.keyframe_insert('location', frame=i)
            b.keyframe_insert('rotation_quaternion', frame=i)
            b.keyframe_insert('scale', frame=i)
    sc = bpy.context.scene
    sc.render.fps = 30
    sc.frame_start, sc.frame_end = 1, len(frames_M)
    for o in bpy.data.objects:
        o.select_set(False)
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    path = output / ('A_Combat_' + name + '.fbx')
    if path.exists():
        shutil.copy2(path, path.with_suffix('.prev.fbx'))
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE'}, add_leaf_bones=False,
                             bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0,
                             axis_forward='-Z', axis_up='Y')
    return {'name': name, 'file': path.name, 'frames': len(frames_M), 'fps': 30, 'duration': (len(frames_M) - 1) / 30, 'bones': len(rig.data.bones)}


report, all_M = [], {}
for name, horse, builder in CLIPS:
    frames_M = builder()
    assert len(frames_M) == len(HORSE[horse]), name
    all_M[name] = frames_M
    r = bake(name, frames_M)
    r['horse_clip'] = 'A_Horse_' + horse
    r['hips_travel_cm'] = [round(v, 1) for v in (max((frames_M[i][short['Hips']].translation[k] for i in range(len(frames_M)))) -
                                                  min((frames_M[i][short['Hips']].translation[k] for i in range(len(frames_M)))) for k in range(3))]
    report.append(r)
(output / 'manifest_cav_rider.json').write_text(json.dumps({'clips': report}, indent=2))
print('CAV_RIDER=' + json.dumps(report))


# ------------------------------------------------------------------ previews: stick rider on stick horse, side view, 6 frames per clip
def preview():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 560, 420
    sc.render.image_settings.file_format = 'PNG'
    sh = sc.display.shading
    sh.light, sh.color_type = 'FLAT', 'MATERIAL'
    sc.world = bpy.data.worlds.new('w')
    sc.world.color = (0.82, 0.87, 0.93)
    sc.view_settings.view_transform = 'Standard'

    def lines(name, count, colour, radius):
        cu = bpy.data.curves.new(name, 'CURVE')
        cu.dimensions = '3D'
        cu.bevel_depth = radius
        cu.bevel_resolution = 1
        for _ in range(count):
            sp = cu.splines.new('POLY')
            sp.points.add(1)
        ob = bpy.data.objects.new(name, cu)
        sc.collection.objects.link(ob)
        mat = bpy.data.materials.new(name)
        mat.diffuse_color = colour
        ob.data.materials.append(mat)
        return ob

    def set_lines(ob, segs):
        for sp, (a, b) in zip(ob.data.splines, segs):
            sp.points[0].co = (a[0], a[1], a[2], 1.0)
            sp.points[1].co = (b[0], b[1], b[2], 1.0)

    ground = bpy.data.objects.new('ground', bpy.data.meshes.new('ground'))
    gm = ground.data
    gm.from_pydata([(-3, -1.5, 0), (3, -1.5, 0), (3, 1.5, 0), (-3, 1.5, 0)], [], [(0, 1, 2, 3)])
    sc.collection.objects.link(ground)
    mat = bpy.data.materials.new('ground')
    mat.diffuse_color = (0.45, 0.4, 0.3, 1)
    ground.data.materials.append(mat)
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
    sc.collection.objects.link(cam)
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 3.3
    cam.location = (0, 8, 1.15)                                  # looking along -Y: the horse (head at -X) faces right
    cam.rotation_euler = (math.radians(90), 0, math.radians(180))
    sc.camera = cam
    horse_ob = lines('horse', len(next(iter(HORSE['Walk']))['bones']), (0.35, 0.2, 0.1, 1), 0.012)
    rider_names = [n for n in names if not any(k in n for k in ('Thumb', 'Index', 'HeadTop', 'Toe_End'))]
    rider_ob = lines('rider', len(rider_names), (0.1, 0.25, 0.7, 1), 0.016)
    saddle_ob = lines('saddle', 1, (0.9, 0.1, 0.1, 1), 0.02)
    ground_ob = lines('groundline', 1, (0.2, 0.15, 0.1, 1), 0.01)
    set_lines(ground_ob, [((-3, 0, 0), (3, 0, 0))])

    def rider_world(p, origin):
        return Vector((origin.x - p.z * 0.01, origin.y - p.x * 0.01, origin.z + p.y * 0.01))

    for name, horse, _ in CLIPS:
        frames_M, hf = all_M[name], HORSE[horse]
        n = len(frames_M)
        picks = sorted({int(round(k * (n - 1) / 5)) for k in range(6)})
        folder = previews / name
        folder.mkdir(parents=True, exist_ok=True)
        origin0 = hf[0]['saddle'] * HORSE_SCALE - Vector((0, 0, HIPS_Y * 0.01))
        for idx in picks:
            hb = hf[idx]['bones']
            set_lines(horse_ob, [(a * HORSE_SCALE, b * HORSE_SCALE) for a, b in hb.values()])
            origin = origin0 if name == 'CavFall' else hf[idx]['saddle'] * HORSE_SCALE - Vector((0, 0, HIPS_Y * 0.01))
            M = frames_M[idx]
            segs = []
            for nm in rider_names:
                head = M[nm].translation
                tail = head + M[nm].to_3x3() @ Vector((0, blen[nm], 0))
                segs.append((rider_world(head, origin), rider_world(tail, origin)))
            set_lines(rider_ob, segs)
            if name == 'CavFall' and idx == picks[-1]:
                zs = [min(a.z, b.z) for a, b in segs]
                print('FALL_END_RIDER_Z_RANGE_M', round(min(zs), 3), round(max(max(a.z, b.z) for a, b in segs), 3), 'hips z', round(segs[0][0].z, 3), 'x', round(segs[0][0].x, 3))
            sp = hf[idx]['saddle'] * HORSE_SCALE
            set_lines(saddle_ob, [(sp, sp + Vector((0, 0, 0.03)))])
            sc.render.filepath = str(folder / ('%02d.png' % idx))
            bpy.ops.render.render(write_still=True)
    return picks


preview()
print('CAV_RIDER_DONE')
