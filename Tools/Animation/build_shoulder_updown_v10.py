"""v10 (2026-10-18): ShoulderUp / ShoulderDown as REAL drill transitions, standing on the spot.
    blender -b -P build_shoulder_updown_v10.py -- [<MultiView dir>|-] [<out dir>]
Body (hips, legs, torso, head) = the pose of SourceAssets/InfantryCombat/A_Combat_Idle.fbx (first frame), retargeted by bone directions onto the
Walking.fbx skinned rig and held (feet planted). Right arm: Idle ready pose -> v9 carry (IK in the Spine2 frame, identical offsets to v9: butt in the
hand at belt height, elbow ~107 deg). Left arm: grips the rifle in front of the right hand, follows the blended barrel up to the right shoulder, then
lets go and returns to the walk-frame-0 swing pose. ShoulderDown = ShoulderUp played backwards.
Rifle in the game: d = slerp(A, B, w(u)), grip = lerp(0.22, 0.03, w(u)), w(u) = smoothstep(clamp((u-0.10)/0.65)).
Exports A_Combat_ShoulderUp.fbx / A_Combat_ShoulderDown.fbx (mesh + bind pose, like v9) + shoulder_v10_report.json; WalkShoulder is copied from v9."""
import bpy, json, math, os, sys, shutil
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

a = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
MULTI = Path(a[0]) if a and a[0] != '-' else Path(r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView')
OUT = Path(a[1]) if len(a) > 1 else PROJECT / 'Saved/Codex/shoulder_v10'
OUT.mkdir(parents=True, exist_ok=True)
WALK = MULTI / '3D figurer' / 'Walking.fbx'
IDLE = PROJECT / 'SourceAssets/InfantryCombat/A_Combat_Idle.fbx'
NF = int(os.environ.get('NF', '36'))                        # frames per clip (36 = 1.2 s at 30 fps)
# v9 carry parameters (unchanged)
HAND_DOWN, HAND_OUT, HAND_FWD = 0.55, -0.10, 0.31
POLE = [0.05, -0.25, -0.5]
POLE_ANGLE = 180.0
PIN_BONE = 'Spine2'
# rule B (carry): contact point relative to the RightShoulder bone head, grip 3 %; rule A: right hand -> left hand, grip 22 %
CA, CB, CC = 0.117, 0.05, 0.10
GRIP_A, GRIP_B = 0.22, 0.03
ARM_BONES = ['Arm', 'ForeArm', 'Hand'] + ['Hand%s%d' % (f, i) for f in ('Index', 'Middle', 'Ring', 'Pinky', 'Thumb') for i in (1, 2, 3, 4)]
U_REL0, U_REL1 = float(os.environ.get('U_REL0', '0.70')), float(os.environ.get('U_REL1', '0.97'))     # left-hand release window
LANG = float(os.environ.get('LANG_', '0'))
BUMP = float(os.environ.get('BUMP', '0.05'))          # m, OUTWARD (to the right) push of the right hand at the middle of the motion (small; more makes the left arm overstretch)
U_UP = float(os.environ.get('U_UP', '0.22'))             # phase at which the rifle stands upright in front of the body
LAGW = float(os.environ.get('LAGW', '0.20'))
BUMP_FWD = float(os.environ.get('BUMP_FWD', '0.0'))
BUMP_UP = float(os.environ.get('BUMP_UP', '0.0'))
LPOLE = [float(x) for x in os.environ.get('LPOLE', '-0.15,0.05,-0.5').split(',')]   # left elbow hint (right, fwd, up) from the left shoulder


def S_(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


def wgt(u):     # the game's blend weight between rule A and rule B
    return S_((u - 0.10) / 0.65)


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(WALK))
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
act = arm.animation_data.action
try:
    arm.animation_data.action_slot = act.slots[0]
except Exception:
    pass
sc = bpy.context.scene
sc.render.fps = 30
f0 = int(act.frame_range[0])
for pb in arm.pose.bones:
    pb.rotation_mode = 'QUATERNION'
hips = arm.pose.bones['mixamorig:Hips']
sc.frame_set(f0)
bpy.context.view_layer.update()
A0 = {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}    # walk frame 0 (left-arm swing + relaxed fingers end up in the final pose)


def setpose(poses):
    for pb in arm.pose.bones:
        pb.matrix_basis = poses[pb.name]
    bpy.context.view_layer.update()


def head(n):
    return arm.matrix_world @ arm.pose.bones['mixamorig:' + n].head


def hcentre(side_):
    pb = arm.pose.bones['mixamorig:%sHand' % side_]
    return arm.matrix_world @ ((pb.head + pb.tail) / 2)


# ---- v9 carry offsets, computed on the walk frame 0 exactly as in v9
setpose(A0)
right_v = head('RightShoulder') - head('LeftShoulder')
right_v.z = 0
right_v.normalize()
up_v = Vector((0, 0, 1))
fwd_v = up_v.cross(right_v).normalized()
sh0 = head('RightArm')
arm_len = (head('RightForeArm') - head('RightArm')).length + (head('RightHand') - head('RightForeArm')).length
pin0 = (arm.matrix_world @ arm.pose.bones['mixamorig:' + PIN_BONE].matrix).inverted()
hand_local = pin0 @ (sh0 + right_v * HAND_OUT + fwd_v * HAND_FWD - up_v * (HAND_DOWN * arm_len))
pole_local = pin0 @ (sh0 + right_v * POLE[0] + fwd_v * POLE[1] + up_v * POLE[2])
hips_xy = (arm.matrix_world @ hips.head).copy()

# ---- import the Idle clip as the source of the standing pose
before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(IDLE))
src = next(o for o in bpy.data.objects if o.type == 'ARMATURE' and o not in before)
try:
    src.animation_data.action_slot = src.animation_data.action.slots[0]
except Exception:
    pass
fi = int(src.animation_data.action.frame_range[0])
sc.frame_set(fi)
bpy.context.view_layer.update()


def SH(n):
    return src.matrix_world @ src.pose.bones['mixamorig:' + n].head


slat = SH('RightShoulder') - SH('LeftShoulder')
slat.z = 0
slat.normalize()
Rz = slat.rotation_difference(right_v).to_matrix()   # rotate the Idle man so that he faces like the walking figure
arm.animation_data_clear()
Rw3 = arm.matrix_world.to_3x3().normalized()


def rot_about_head(pb, R4):
    M = pb.matrix.copy()
    t = M.translation.copy()
    M.translation = Vector((0, 0, 0))
    M = R4 @ M
    M.translation = t
    pb.matrix = M
    bpy.context.view_layer.update()


def retarget():
    for pb in arm.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()

    def depth(pb):
        d = 0
        while pb.parent:
            pb = pb.parent
            d += 1
        return d
    for pb in sorted(arm.pose.bones, key=depth):
        if pb.name not in src.pose.bones:
            continue
        sb = src.pose.bones[pb.name]
        dsrc = Rz @ ((src.matrix_world @ sb.tail) - (src.matrix_world @ sb.head))
        dsrc.normalize()
        dcur = (arm.matrix_world @ pb.tail) - (arm.matrix_world @ pb.head)
        dcur.normalize()
        rot_about_head(pb, (Rw3.inverted() @ dcur.rotation_difference(dsrc).to_matrix() @ Rw3).to_4x4())
        if pb.name in ('mixamorig:Hips', 'mixamorig:Spine2'):         # twist about the bone axis from the lateral vector
            la, lb = ('RightUpLeg', 'LeftUpLeg') if pb.name.endswith('Hips') else ('RightShoulder', 'LeftShoulder')
            lt = head(la) - head(lb)
            lsv = Rz @ (SH(la) - SH(lb))
            ax = dsrc
            lt -= ax * lt.dot(ax)
            lsv -= ax * lsv.dot(ax)
            ang = lt.angle(lsv)
            if lt.cross(lsv).dot(ax) < 0:
                ang = -ang
            rot_about_head(pb, (Rw3.inverted() @ Quaternion(ax, ang).to_matrix() @ Rw3).to_4x4())
    # hips position: x/y of the walk, height so the toes stand on the ground (z = 0)
    M = hips.matrix.copy()
    w = arm.matrix_world @ M.translation
    w.x, w.y = hips_xy.x, hips_xy.y
    M.translation = arm.matrix_world.inverted() @ w
    hips.matrix = M
    bpy.context.view_layer.update()
    lowz = min(head('LeftToeBase').z, head('RightToeBase').z)
    M = hips.matrix.copy()
    w = arm.matrix_world @ M.translation
    w.z -= lowz
    M.translation = arm.matrix_world.inverted() @ w
    hips.matrix = M
    bpy.context.view_layer.update()
    return {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}


P0 = retarget()                                    # Idle pose on the walking rig (ready stance)
setpose(P0)
print('RETARGET hips z', round(head('Hips').z, 3), 'toes z', round(head('LeftToeBase').z, 3), round(head('RightToeBase').z, 3))
rv2 = head('RightShoulder') - head('LeftShoulder')
rv2.z = 0
rv2.normalize()
fv2 = up_v.cross(rv2).normalized()
print('FACING', tuple(round(x, 3) for x in fv2), 'walk', tuple(round(x, 3) for x in fwd_v))

tg = bpy.data.objects.new('RH', None)
pl = bpy.data.objects.new('RP', None)
tgL = bpy.data.objects.new('LH', None)
plL = bpy.data.objects.new('LP', None)
for o in (tg, pl, tgL, plL):
    sc.collection.objects.link(o)


def ik_arm(poses, side_, target, pole, angle):
    setpose(poses)
    fa = arm.pose.bones['mixamorig:%sForeArm' % side_]
    t, p = (tg, pl) if side_ == 'Right' else (tgL, plL)
    t.location = target
    p.location = pole
    c = fa.constraints.new('IK')
    c.target, c.pole_target = t, p
    c.pole_angle = math.radians(angle)
    c.chain_count = 2
    c.use_tail = True
    c.use_stretch = False
    c.iterations = 80
    bpy.context.view_layer.update()
    mA = arm.pose.bones['mixamorig:%sArm' % side_].matrix.copy()
    mF = fa.matrix.copy()
    fa.constraints.remove(c)
    setpose(poses)
    arm.pose.bones['mixamorig:%sArm' % side_].matrix = mA
    bpy.context.view_layer.update()
    fa.matrix = mF
    bpy.context.view_layer.update()
    return {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}


# ---- torso: from the Idle stance (u = 0) to the v9 walk-frame-0 torso (u = 1): world bone directions of Spine..Head + clavicles are blended
TORSO = ['mixamorig:' + n for n in ('Spine', 'Spine1', 'Spine2', 'Neck', 'Head', 'LeftShoulder', 'RightShoulder')]


def world_dirs():
    d = {}
    for n in TORSO:
        pb = arm.pose.bones[n]
        v_ = (arm.matrix_world @ pb.tail) - (arm.matrix_world @ pb.head)
        d[n] = v_.normalized()
    lat = head('RightShoulder') - head('LeftShoulder')
    return d, lat


setpose(P0)
idle_dirs, idle_lat = world_dirs()
setpose(A0)
walk_dirs, walk_lat = world_dirs()


def set_torso(tau):
    for n in TORSO:
        arm.pose.bones[n].matrix_basis = P0[n]
    bpy.context.view_layer.update()
    for n in TORSO:                                   # TORSO is ordered parent first
        pb = arm.pose.bones[n]
        dcur = ((arm.matrix_world @ pb.tail) - (arm.matrix_world @ pb.head)).normalized()
        dd = (Quaternion().slerp(idle_dirs[n].rotation_difference(walk_dirs[n]), tau) @ idle_dirs[n]).normalized()
        rot_about_head(pb, (Rw3.inverted() @ dcur.rotation_difference(dd).to_matrix() @ Rw3).to_4x4())
        if n.endswith('Spine2'):
            lt = head('RightShoulder') - head('LeftShoulder')
            lw = (Quaternion().slerp(idle_lat.rotation_difference(walk_lat), tau) @ idle_lat)
            lt -= dd * lt.dot(dd)
            lw -= dd * lw.dot(dd)
            ang = lt.angle(lw)
            if lt.cross(lw).dot(dd) < 0:
                ang = -ang
            rot_about_head(pb, (Rw3.inverted() @ Quaternion(dd, ang).to_matrix() @ Rw3).to_4x4())
    return {pb.name: pb.matrix_basis.copy() for pb in arm.pose.bones}


def carry_pose(pose0):
    """right arm in the v9 carry pose on the given torso (hand + pole pinned in the Spine2 frame, v9 offsets; v9 hand/finger rotations)"""
    setpose(pose0)
    pm = arm.matrix_world @ arm.pose.bones['mixamorig:' + PIN_BONE].matrix
    bp = ik_arm(pose0, 'Right', pm @ hand_local, pm @ pole_local, POLE_ANGLE)
    for n in list(bp):
        if n.split(':')[-1] in ['Right' + b_ for b_ in ARM_BONES if b_ not in ('Arm', 'ForeArm')]:
            bp[n] = A0[n].copy()
    return bp


Bp = carry_pose(set_torso(1.0))
setpose(Bp)
_sh, _el, _wr = head('RightArm'), head('RightForeArm'), head('RightHand')
print('CARRY elbow', round(math.degrees(math.acos(max(-1, min(1, (_sh - _el).normalized().dot((_wr - _el).normalized()))))), 1),
      'hand rel shoulder (fwd,up,right)', tuple(round(v, 3) for v in ((_wr - _sh).dot(fv2), (_wr - _sh).z, (_wr - _sh).dot(rv2))))


def is_side(name, side_):
    return name.split(':')[-1] in [side_ + b for b in ARM_BONES]


def mix(pa, pb_, w, names):
    out = dict(pa)
    for n in names:
        l0, r0, s0 = pa[n].decompose()
        l1, r1, s1 = pb_[n].decompose()
        out[n] = Matrix.LocRotScale(l0.lerp(l1, w), r0.slerp(r1, w), s0.lerp(s1, w))
    return out


R_NAMES = [n for n in P0 if is_side(n, 'Right')]
L_NAMES = [n for n in P0 if is_side(n, 'Left')]

# ---- authoring of the Up motion
setpose(P0)
Rc0, Lc0 = hcentre('Right'), hcentre('Left')
e0 = (Lc0 - Rc0).normalized()
s0 = (Lc0 - Rc0).length
print('READY: R->L dist', round(s0, 3), 'dir (fwd,up,right)', tuple(round(x, 2) for x in (e0.dot(fv2), e0.z, e0.dot(rv2))))
lsh0 = head('LeftArm')
lhp = arm.pose.bones['mixamorig:LeftHand']
HV = ((arm.matrix_world @ lhp.tail) - (arm.matrix_world @ lhp.head)) / 2

frames_up, log = [], []
for i in range(NF):
    u = i / (NF - 1)
    tor = set_torso(S_(u / 0.75))                         # torso straightens from the Idle stance to the walk-frame-0 torso
    Bu = carry_pose(tor)
    base = mix(tor, Bu, S_(u / 0.70), R_NAMES)            # right arm: ready (Idle rotations) -> carry
    setpose(base)
    bump = BUMP * math.sin(math.pi * S_(u / 0.70))       # the right hand is pushed outwards (to the right) in mid-motion so the raised, near-vertical rifle passes beside the face and shako, not in front of them
    if bump > 1e-4:
        pm_ = arm.matrix_world @ arm.pose.bones['mixamorig:' + PIN_BONE].matrix
        rsx, lsx = head('RightShoulder'), head('LeftShoulder')
        sdx = (rsx - lsx).normalized()
        fwx = up_v.cross(sdx).normalized()
        wr_ = head('RightHand')
        ikp = ik_arm(base, 'Right', wr_ + sdx * bump + fwx * (BUMP_FWD * bump / max(BUMP, 1e-6)) + up_v * (BUMP_UP * bump / max(BUMP, 1e-6)), pm_ @ pole_local, POLE_ANGLE)
        kk = S_(u / 0.08)
        for n in ('mixamorig:RightArm', 'mixamorig:RightForeArm'):
            base[n] = mix(base, ikp, kk, [n])[n]
    setpose(base)
    Rc = hcentre('Right')
    rs = head('RightShoulder')
    lsh = head('LeftShoulder')
    side_v = (rs - lsh).normalized()
    fw = up_v.cross(side_v).normalized()
    contact = rs + fw * CA + up_v * CB + side_v * CC
    dirB = (contact - Rc).normalized()
    # left-hand direction e (right hand -> left hand): first lifts the rifle to an upright, slightly forward-leaning stance in front of the body
    # (clear of face and shako), then follows the blend towards rule B.  The 0.25 / w term keeps w * angle(e, B) small, i.e. the left hand on the blended line.
    e_up = (up_v * 0.99 + fw * 0.14).normalized()
    wv = wgt(u)
    if u < U_UP:
        e = Quaternion().slerp(e0.rotation_difference(e_up), S_(u / U_UP)) @ e0
    else:
        v2 = max(0.0, min(1.0, 1.0 - LAGW / max(wv, 1e-6)))
        e = Quaternion().slerp(e_up.rotation_difference(dirB), v2) @ e_up
    s = s0 + (0.24 - s0) * S_(u / U_UP)
    Lt = Rc + e * s
    posed = base
    for it in range(3):
        lpole = lsh0 + side_v * LPOLE[0] + fw * LPOLE[1] + up_v * LPOLE[2]
        posed = ik_arm(base, 'Left', Lt - HV, lpole, LANG)
        HV = ((arm.matrix_world @ lhp.tail) - (arm.matrix_world @ lhp.head)) / 2
    qrel = S_((u - U_REL0) / (U_REL1 - U_REL0))
    ell = S_(u / 0.10)
    pose = dict(base)
    for n in ('mixamorig:LeftArm', 'mixamorig:LeftForeArm'):             # ready -> IK grip -> walk swing
        pose[n] = mix(mix(P0, posed, ell, [n]), A0, qrel, [n])[n]
    for n in L_NAMES:                                                    # hand + fingers: Idle grip -> walk fingers at release
        if n not in ('mixamorig:LeftArm', 'mixamorig:LeftForeArm'):
            pose[n] = mix(P0, A0, qrel, [n])[n]
    frames_up.append(pose)
    setpose(pose)
    Rc2, Lc2 = hcentre('Right'), hcentre('Left')
    dA = (Lc2 - Rc2).normalized()
    rs = head('RightShoulder')
    lsh = head('LeftShoulder')
    side_v = (rs - lsh).normalized()
    fw = up_v.cross(side_v).normalized()
    dB = ((rs + fw * CA + up_v * CB + side_v * CC) - Rc2).normalized()
    w = wgt(u)
    dg = Quaternion().slerp(dA.rotation_difference(dB), w) @ dA
    tline = (Lc2 - Rc2).dot(dg)
    dist = ((Lc2 - Rc2) - dg * tline).length
    log.append(dict(frame=i, u=round(u, 3), w=round(w, 3), left_to_line=round(dist, 3), left_t=round(tline, 3),
                    grip=round(GRIP_A + (GRIP_B - GRIP_A) * w, 3), angAB_deg=round(math.degrees(dA.angle(dB)), 1)))
for r in log:
    print('LOG', json.dumps(r))


def export(path):
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


def bake(name, frames):
    arm.animation_data_clear()
    arm.animation_data_create()
    action = bpy.data.actions.new('A_Combat_' + name)
    arm.animation_data.action = action
    try:
        arm.animation_data.action_slot = action.slots[0] if action.slots else action.slots.new('OBJECT', arm.name)
    except Exception:
        pass
    for i, poses in enumerate(frames, 1):
        for pb in arm.pose.bones:
            pb.matrix_basis = poses[pb.name]
            pb.keyframe_insert('location', frame=i)
            pb.keyframe_insert('rotation_quaternion', frame=i)
            pb.keyframe_insert('scale', frame=i)
    sc.frame_start, sc.frame_end = 1, len(frames)
    path = OUT / ('A_Combat_' + name + '.fbx')
    export(path)
    return {'name': name, 'file': path.name, 'frames': len(frames), 'fps': 30, 'bones': len(arm.data.bones)}


src.hide_set(True)
report = [bake('ShoulderUp', frames_up), bake('ShoulderDown', frames_up[::-1])]
v9 = PROJECT / 'Saved/Codex/shoulder_v9/A_Combat_WalkShoulder.fbx'
if v9.exists():
    shutil.copyfile(v9, OUT / 'A_Combat_WalkShoulder.fbx')
(OUT / 'shoulder_v10_report.json').write_text(json.dumps({'source': str(WALK), 'idle': str(IDLE), 'frames': NF, 'clips': report, 'log_up': log}, indent=2))
print('SHOULDER_V10=' + json.dumps(report))
