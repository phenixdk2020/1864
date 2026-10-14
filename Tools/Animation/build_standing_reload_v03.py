"""Standing percussion-musket reload, 331 frames at 30 fps, built in Blender with arm IK around the rifle (v03).

    blender -b -P build_standing_reload_v03.py -- <Kildeassets dir> <Rifle Idle.fbx> <out dir> [preview]

Everything is keyed in the soldier's local frame (x right, y forward, z up, origin between the feet, units as the FBX: soldier 1.99 high).
The rifle, ramrod, cartridge and cap are written to props_v03.json (a transform per frame, soldier frame) next to the animation FBX.
"""
import bpy, sys, json, math
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

args = sys.argv[sys.argv.index('--') + 1:]
KILDE, IDLE, OUT = Path(args[0]), args[1], Path(args[2])
OUT.mkdir(parents=True, exist_ok=True)
FIRST, LAST = 1, 331
RIFLE_SCALE, ROD_SCALE, CART_SCALE, CAP_SCALE = 0.8, 0.9, 0.40, 0.26
RIFLE_HALF = 1.0 * RIFLE_SCALE                      # butt at -1, muzzle at +1 in the FBX
ROD_LEN = 1.196 * ROD_SCALE
AXIS_Z, AXIS_Y = 0.08 * RIFLE_SCALE, 0.02 * RIFLE_SCALE   # the barrel's centre line in the FBX

# ---------------------------------------------------------------------------------------------------------
# key poses (frame, dict). Rifle: butt B and direction A in the soldier frame, roll in degrees about the axis.
# Hands: ('r', s, lat, up) = on the rifle (s metres from the butt, lat to the lock side, up toward the sights),
#        ('w', x, y, z) = in the soldier frame, ('rod', t) = on the ramrod (t = 0 head ... 1 tip).
# Ramrod: (head s, direction d = +1 tip toward the muzzle / -1 toward the butt, lat, up) parallel to the rifle.
# ---------------------------------------------------------------------------------------------------------
READY = dict(B=(0.20, 0.25, 0.95), A=(-0.45, 0.30, 0.60), roll=0)
LOAD = dict(B=(0.22, 0.20, 0.05), A=(-0.20, 0.14, 0.97), roll=0)
HIP = dict(B=(0.24, 0.05, 1.00), A=(0.0, 0.80, 0.55), roll=0)
STORED = (1.55, -1, 0.0, -0.09)


def key(frame, rifle, lh, rh, rod=STORED, cart=None, cap=None, **extra):
    d = dict(rifle)
    d.update(LH=lh, RH=rh, rod=rod, cart=cart, cap=cap)
    d.update(extra)
    return frame, d


GRIP_L = ('r', 1.15, -0.06, 0.0)
GRIP_R = ('r', 0.60, 0.07, 0.0)
POUCH = ('w', 0.30, 0.10, 1.00)
CAPPOUCH = ('w', 0.14, 0.18, 1.08)
MOUTH = ('w', 0.02, 0.22, 1.68)
CHEST = ('w', 0.12, 0.26, 1.36)

KEYS = [
    key(0, READY, GRIP_L, GRIP_R),
    key(24, READY, GRIP_L, GRIP_R),
    key(40, LOAD, ('r', 1.30, -0.06, 0.0), ('w', 0.36, 0.14, 1.18)),
    key(54, LOAD, ('r', 1.30, -0.06, 0.0), POUCH),
    key(70, LOAD, ('r', 1.30, -0.06, 0.0), CHEST, cart='hand'),
    key(84, LOAD, ('r', 1.30, -0.06, 0.0), ('w', 0.10, 0.24, 1.52), cart='hand'),
    key(100, LOAD, ('r', 1.30, -0.06, 0.0), MOUTH, cart='hand'),
    key(114, LOAD, ('r', 1.30, -0.06, 0.0), MOUTH, cart='hand'),
    key(130, LOAD, ('r', 1.30, -0.06, 0.0), ('r', 1.78, 0.0, 0.05), cart='hand'),
    key(144, LOAD, ('r', 1.30, -0.06, 0.0), ('r', 1.78, 0.0, 0.05), cart=None),
    key(168, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.10)),
    key(180, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.10), rod=(1.95, -1, 0.0, -0.09)),
    key(192, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.5), rod=('w', 0.20, 0.34, 1.45, -0.9, 0.2, 0.1)),
    key(200, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.5), rod=('w', 0.12, 0.30, 2.05, 0.0, 0.0, -1.0)),
    key(205, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.88), rod=(1.60 - 0.35, 1, 0.0, 0.0)),
    key(212, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.88), rod=(1.60 - 0.10, 1, 0.0, 0.0)),
    key(220, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.88), rod=(1.60 - 0.60, 1, 0.0, 0.0)),
    key(228, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.88), rod=(1.60 - 0.30, 1, 0.0, 0.0)),
    key(235, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.88), rod=(1.60 - 0.95, 1, 0.0, 0.0)),
    key(250, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.88), rod=(1.65, 1, 0.0, 0.0)),
    key(256, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.5), rod=('w', 0.12, 0.30, 2.05, 0.0, 0.0, -1.0)),
    key(262, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.5), rod=('w', 0.20, 0.34, 1.45, -0.9, 0.2, 0.1)),
    key(266, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.15), rod=(2.60, -1, 0.0, -0.09)),
    key(274, LOAD, ('r', 1.30, -0.06, 0.0), ('rod', 0.15), rod=(STORED[0], -1, 0.0, -0.09)),
    key(280, LOAD, ('r', 1.30, -0.06, 0.0), CAPPOUCH),
    key(294, HIP, ('r', 1.10, -0.06, 0.0), CAPPOUCH, cap='hand'),
    key(305, HIP, ('r', 1.10, -0.06, 0.0), ('r', 0.50, 0.12, 0.04), cap='hand'),
    key(314, HIP, ('r', 1.10, -0.06, 0.0), ('r', 0.50, 0.12, 0.04), cap='nipple'),
    key(330, READY, GRIP_L, GRIP_R, cap='nipple'),
]


def smooth(t):
    return t * t * (3 - 2 * t)


def lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


LEAN = {0: 0, 24: 0, 40: 6, 100: 10, 130: 12, 168: 8, 200: 12, 235: 14, 262: 8, 280: 6, 300: 8, 330: 0}


def lean_at(frame):
    ks = sorted(LEAN)
    for a, b in zip(ks, ks[1:]):
        if a <= frame <= b:
            t = (frame - a) / (b - a)
            return LEAN[a] + (LEAN[b] - LEAN[a]) * smooth(t)
    return 0.0


def key_pair(frame):
    for (f0, k0), (f1, k1) in zip(KEYS, KEYS[1:]):
        if f0 <= frame <= f1:
            t = 0.0 if f1 == f0 else (frame - f0) / (f1 - f0)
            return k0, k1, smooth(t)
    return KEYS[-1][1], KEYS[-1][1], 0.0


# ---------------------------------------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(KILDE / 'DK_Infanteri_1864_textured_skeleton.fbx'))
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
for o in list(bpy.data.objects):
    if o.type == 'MESH':
        bpy.data.objects.remove(o)      # the mesh is not needed to solve the pose
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=IDLE)
src = next(o for o in bpy.data.objects if o not in before and o.type == 'ARMATURE')
act = src.animation_data.action
arm.animation_data_create()
arm.animation_data.action = act
try:
    arm.animation_data.action_slot = act.slots[0]
except Exception:
    pass
for o in list(bpy.data.objects):
    if o not in (arm,):
        bpy.data.objects.remove(o)
sc = bpy.context.scene
sc.frame_set(1)
bpy.context.view_layer.update()

# freeze the idle pose (frame 1) as the base for every frame
base = {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}
arm.animation_data_clear()
for pb in arm.pose.bones:
    pb.rotation_mode = 'QUATERNION'
    pb.matrix_basis = base[pb.name]
bpy.context.view_layer.update()


def head(name):
    return arm.matrix_world @ arm.pose.bones['mixamorig:' + name].head


right_v = head('RightShoulder') - head('LeftShoulder')
right_v.z = 0
right_v.normalize()
up_v = Vector((0, 0, 1))
fwd_v = up_v.cross(right_v).normalized()
origin = (head('LeftFoot') + head('RightFoot')) / 2
origin.z = 0.0
print('FRAME right', tuple(right_v), 'fwd', tuple(fwd_v), 'origin', tuple(origin))


def W(p):
    """soldier-frame point -> Blender world"""
    return origin + right_v * p[0] + fwd_v * p[1] + up_v * p[2]


def Wd(d):
    return right_v * d[0] + fwd_v * d[1] + up_v * d[2]


def rifle_frame(B, A, roll):
    """returns (X axis a, lateral lock-side axis S, top axis T, origin of the barrel centre line at the butt) in the soldier frame (Vectors)"""
    a = Vector(A).normalized()
    s0 = Vector((1, 0, 0))
    s0 = (s0 - a * s0.dot(a)).normalized()
    # roll about a
    s0 = Quaternion(a, math.radians(roll)) @ s0
    t0 = s0.cross(a)                      # top = lock side x axis (horizontal rifle, lock on the right: top is up)
    return a, s0, t0


def rifle_point(k, s, lat=0.0, up=0.0):
    B = Vector(k['B'])
    a, S, T = rifle_frame(B, k['A'], k['roll'])
    return B + a * s + S * lat + T * up


# --- helper objects: IK targets and poles ---
tg = {}
for n in ('LH', 'RH', 'LP', 'RP'):
    e = bpy.data.objects.new(n, None)
    sc.collection.objects.link(e)
    tg[n] = e
for side, p in (('Left', 'LH'), ('Right', 'RH')):
    pb = arm.pose.bones['mixamorig:%sForeArm' % side]
    c = pb.constraints.new('IK')
    c.target = tg[p]
    c.pole_target = tg['LP' if side == 'Left' else 'RP']
    c.pole_angle = math.radians(float(__import__('os').environ.get('POLE_ANGLE', '-90')))
    c.chain_count = 2
    c.use_tail = True
    c.use_stretch = False
    c.iterations = 80
tg['LP'].location = W((-0.65, -0.15, 1.20))      # left elbow out to the soldier's left (= -x of the soldier frame)
tg['RP'].location = W((0.65, -0.15, 1.20))


def hand_pos(spec, k, frame_info):
    kind = spec[0]
    if kind == 'r':
        return rifle_point(k, spec[1] * RIFLE_SCALE / 1.0 * 1.0, spec[2], spec[3])
    if kind == 'w':
        return Vector(spec[1:4])
    raise ValueError(kind)


def rod_pose(k):
    if isinstance(k['rod'][0], str):
        r = k['rod']
        return Vector(r[1:4]), Vector(r[4:7]).normalized()
    hs, d, lat, up = k['rod']
    B = Vector(k['B'])
    a, S, T = rifle_frame(B, k['A'], k['roll'])
    head_p = B + a * hs + S * lat + T * up
    axis = a * d
    return head_p, axis


def solve(frame):
    k0, k1, t = key_pair(frame)
    out = {}
    # rifle
    B = Vector(lerp(k0['B'], k1['B'], t))
    A = Vector(lerp(tuple(Vector(k0['A']).normalized()), tuple(Vector(k1['A']).normalized()), t)).normalized()
    roll = k0['roll'] + (k1['roll'] - k0['roll']) * t
    kk = dict(B=B, A=A, roll=roll)
    # rod (centre + direction, slerped)
    h0, ax0 = rod_pose(k0)
    h1, ax1 = rod_pose(k1)
    ax = (ax0 * (1 - t) + ax1 * t).normalized()
    hp = h0 * (1 - t) + h1 * t
    kk['rod_head'], kk['rod_axis'] = hp, ax
    hands = {}
    for hname in ('LH', 'RH'):
        pts = []
        for kx in (k0, k1):
            spec = kx[hname]
            if spec[0] == 'rod':
                hx, axx = rod_pose(kx)
                pts.append(hx + axx * (ROD_LEN * spec[1]))
            else:
                # evaluate rifle-relative specs against that key's own rifle
                if spec[0] == 'r':
                    pts.append(rifle_point(kx, spec[1] * 0 + spec[1], spec[2], spec[3]))
                else:
                    pts.append(Vector(spec[1:4]))
        hands[hname] = pts[0] * (1 - t) + pts[1] * t
    kk['hands'] = hands
    kk['k0'], kk['k1'], kk['t'] = k0, k1, t
    return kk


frames_data = {}
finals = {}
for f in range(FIRST, LAST + 1):
    kk = solve(f - 1)
    for pb in arm.pose.bones:
        pb.matrix_basis = base[pb.name]
    bpy.context.view_layer.update()
    ang = math.radians(lean_at(f - 1))
    for bn, share in (('mixamorig:Spine', 0.5), ('mixamorig:Spine1', 0.5)):
        pb = arm.pose.bones[bn]
        pivot = arm.matrix_world @ pb.head
        R = Matrix.Translation(pivot) @ Quaternion(right_v, -ang * share).to_matrix().to_4x4() @ Matrix.Translation(-pivot)
        pb.matrix = arm.matrix_world.inverted() @ (R @ (arm.matrix_world @ pb.matrix))
        bpy.context.view_layer.update()
    tg['LH'].location = W(kk['hands']['LH'])
    tg['RH'].location = W(kk['hands']['RH'])
    bpy.context.view_layer.update()
    finals[f] = {pb.name: pb.matrix.copy() for pb in arm.pose.bones}
    frames_data[f] = kk
    if f in (1, 41, 85, 131, 169, 206, 236, 281, 306, 331):
        for hn, bn in (('LH', 'LeftHand'), ('RH', 'RightHand')):
            hp = arm.matrix_world @ arm.pose.bones['mixamorig:' + bn].head
            tgt = tg[hn].location
            print('REACH f%d %s err=%.3f' % (f, hn, (hp - tgt).length))

(OUT / 'pose_finals_v03.json').write_text(json.dumps({str(f): {n: [list(r) for r in m] for n, m in fm.items()} for f, fm in finals.items()}))

# ---- bake the solved pose into keyframes (constraints removed) ----
for pb in arm.pose.bones:
    for c in list(pb.constraints):
        pb.constraints.remove(c)
act = bpy.data.actions.new('A_Combat_ReloadStandingV03')
arm.animation_data_create()
arm.animation_data.action = act
try:
    arm.animation_data.action_slot = act.slots[0] if act.slots else act.slots.new('OBJECT', arm.name)
except Exception as e:
    print('SLOT', e)
order = [pb for pb in arm.pose.bones]
order.sort(key=lambda b: len(b.parent_recursive))
for f in range(FIRST, LAST + 1):
    for pb in order:
        pb.matrix = finals[f][pb.name]
        bpy.context.view_layer.update()
        pb.keyframe_insert('location', frame=f)
        pb.keyframe_insert('rotation_quaternion', frame=f)
        pb.keyframe_insert('scale', frame=f)
sc.render.fps = 30
sc.frame_start, sc.frame_end = FIRST, LAST
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
bpy.context.view_layer.objects.active = arm
path = OUT / 'A_Combat_ReloadStandingV03.fbx'
bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE'}, add_leaf_bones=False,
                         bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0,
                         axis_forward='-Y', axis_up='Z')

# ---- prop transforms per frame (Blender world, i.e. the same space as the armature) ----
props = {'fps': 30, 'frames': LAST - FIRST + 1, 'scale': dict(rifle=RIFLE_SCALE, rod=ROD_SCALE, cart=CART_SCALE, cap=CAP_SCALE),
         'frame': [], 'rifle': [], 'rod': [], 'cart': [], 'cap': []}


def mat_from(origin_p, x_axis, z_hint):
    x = x_axis.normalized()
    z = (z_hint - x * z_hint.dot(x)).normalized()
    y = z.cross(x)
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = origin_p
    return m


for f in range(FIRST, LAST + 1):
    kk = frames_data[f]
    k0, k1, t = kk['k0'], kk['k1'], kk['t']
    a, S, T = rifle_frame(Vector(kk['B']), kk['A'], kk['roll'])
    # the FBX rifle: butt at -x, lock on -y, top +z; the barrel line lies (AXIS_Y, AXIS_Z) above the pivot
    Y = -S
    centre = Vector(kk['B']) + a * RIFLE_HALF - Y * AXIS_Y - T * AXIS_Z
    rifle_m = Matrix((a, Y, T)).transposed().to_4x4()
    rifle_m.translation = centre
    # world transforms: soldier frame -> world
    basis = Matrix((right_v, fwd_v, up_v)).transposed().to_4x4()
    basis.translation = origin
    rifle_w = basis @ rifle_m
    props['rifle'].append([list(r) for r in rifle_w])
    # ramrod: FBX rod has its head at -x; the rod's x axis points from head to tip
    hp, ax = kk['rod_head'], kk['rod_axis']
    rod_c = hp + ax * (ROD_LEN / 2)
    rod_m = Matrix((ax, S, ax.cross(S).normalized() if abs(ax.dot(S)) < 0.99 else T)).transposed().to_4x4()
    rod_m.translation = rod_c
    props['rod'].append([list(r) for r in (basis @ rod_m)])
    props['frame'].append(f)
    # cartridge / cap
    def vis(name):
        k = k0 if t < 0.5 else k1
        return k.get(name)
    rh = kk['hands']['RH']
    cart = vis('cart')
    cap = vis('cap')
    cm = Matrix.Translation(rh + Vector((0, 0, -0.03)))
    props['cart'].append([list(r) for r in (basis @ cm)] if cart == 'hand' else None)
    if cap == 'hand':
        props['cap'].append([list(r) for r in (basis @ Matrix.Translation(rh))])
    elif cap == 'nipple':
        nip = rifle_point(dict(B=kk['B'], A=kk['A'], roll=kk['roll']), 0.50, 0.07, 0.10)
        props['cap'].append([list(r) for r in (basis @ Matrix.Translation(nip))])
    else:
        props['cap'].append(None)
(OUT / 'props_v03.json').write_text(json.dumps(props))
print('BUILD_DONE', path)
