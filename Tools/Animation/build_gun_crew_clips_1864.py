"""Kanonbesaetning (6-pund feltkanon) 2026-11: 7 klip paa soldaternes skelet SK_Human_1864 (Mixamo-knogler, DK_Infantry_1864.fbx).
    blender -b -P build_gun_crew_clips_1864.py -- [<out dir>]        (default SourceAssets/Artillery/Crew)
Klip (30 fps, paa stedet): A_Gun_Sponge, A_Gun_Load, A_Gun_Ram, A_Gun_Prime, A_Gun_Aim (loop), A_Gun_Push_Loop (loop), A_Gun_StandBy (loop).
Input (aendres ikke): SourceAssets/Units1864/{DK_Infantry_1864.fbx, Cannon.fbx}, SourceAssets/Artillery/Props/{SM_Sponge,SM_Rammer,SM_Cannonball_12pdr}.fbx.
Metode: ren procedural pose (ingen kilde-klip): hofte/ryg/hoved + ben-IK (fodder paa jorden) + arm-IK (to-knogle) mod haandmaal, som build_officer_clips_1864.py.
  Maal paa kanonen laeses fra Cannon.fbx i spillets skala (GUN_SCALE, default 2.2). Rekvisitter holdes af RightHand med fast socket (venstre haand
  er kun IK paa skaftet). Positioner er i kanonens ramme F = (f fremad mod mundingen, l venstre set bag kanonen, u hoejde over jorden), cm.
Env: ONLY=Sponge,Load,... (kun disse klip), SKIP_RENDER=1, GUN_SCALE=2.2, DEBUG=1.
Eksisterende outputfiler flyttes til <navn>.prev.<ext> foer de overskrives.
"""
import bpy, json, math, os, sys, shutil, subprocess
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
SOLDIER = PROJECT / 'SourceAssets/Units1864/DK_Infantry_1864.fbx'
SOLDIER_TEX = PROJECT / 'SourceAssets/Units1864/DK_Infantry_1864_Texture_0.png'
CANNON = PROJECT / 'SourceAssets/Units1864/Cannon.fbx'
CANNON_TEX = PROJECT / 'SourceAssets/Units1864/Cannon_Texture_0.png'
PROPS = PROJECT / 'SourceAssets/Artillery/Props'
OUT = Path(args[0]) if args else PROJECT / 'SourceAssets/Artillery/Crew'
CACHE = PROJECT / 'Saved/Codex/gun_crew'
GUN_SCALE = float(os.environ.get('GUN_SCALE', '2.2'))
ONLY = [x for x in os.environ.get('ONLY', '').split(',') if x]
DEBUG = bool(os.environ.get('DEBUG'))
M_ = 'mixamorig:'
V = Vector
for d in (OUT, OUT / 'Preview', CACHE):
    d.mkdir(parents=True, exist_ok=True)


def backup(path):
    path = Path(path)
    if path.exists():
        shutil.copyfile(path, path.with_name(path.stem + '.prev' + path.suffix))


def rad(a):
    return math.radians(a)


def Ry(a):
    return Matrix.Rotation(rad(a), 3, 'Y')


def Rx(a):
    return Matrix.Rotation(rad(a), 3, 'X')


def Rz(a):
    return Matrix.Rotation(rad(a), 3, 'Z')


def T4(v):
    return Matrix.Translation(v)


def R4(R3):
    return R3.to_4x4()


def cols_to_mat(cols):
    return Matrix([[cols[j][i] for j in range(3)] for i in range(3)])


# ---------------------------------------------------------------- tidskurver (monoton kubisk Hermite, valgfrit periodisk)
class Tr:
    def __init__(self, keys, loop=None):
        self.vec = not isinstance(keys[0][1], (int, float))
        k = sorted(keys, key=lambda x: x[0])
        if loop:
            body = k[:-1] if abs(k[-1][0] - k[0][0] - loop) < 1e-6 else k
            k = [(t - loop, v) for t, v in body] + body + [(t + loop, v) for t, v in body] + [(body[0][0] + 2 * loop, body[0][1])]
        self.k = k

    @staticmethod
    def _h(ts, vs, x):
        n = len(ts)
        if x <= ts[0]:
            return vs[0]
        if x >= ts[-1]:
            return vs[-1]
        i = max(j for j in range(n - 1) if ts[j] <= x)
        h = [ts[j + 1] - ts[j] for j in range(n - 1)]
        dl = [(vs[j + 1] - vs[j]) / h[j] for j in range(n - 1)]
        m = [0.0] * n
        for j in range(1, n - 1):
            if dl[j - 1] * dl[j] > 0:
                w1, w2 = 2 * h[j] + h[j - 1], h[j] + 2 * h[j - 1]
                m[j] = (w1 + w2) / (w1 / dl[j - 1] + w2 / dl[j])
        t = (x - ts[i]) / h[i]
        return ((2 * t ** 3 - 3 * t ** 2 + 1) * vs[i] + (t ** 3 - 2 * t ** 2 + t) * h[i] * m[i]
                + (-2 * t ** 3 + 3 * t ** 2) * vs[i + 1] + (t ** 3 - t ** 2) * h[i] * m[i + 1])

    def __call__(self, x):
        ts = [kk[0] for kk in self.k]
        if not self.vec:
            return self._h(ts, [float(kk[1]) for kk in self.k], x)
        n = len(self.k[0][1])
        return V([self._h(ts, [kk[1][c] for kk in self.k], x) for c in range(n)])


# ---------------------------------------------------------------- kanonens maal (i spillets skala)
def measure_gun(S):
    import numpy as np
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(CANNON))
    ob = next(o for o in bpy.data.objects if o.type == 'MESH')
    co = np.array([(ob.matrix_world @ v.co)[:] for v in ob.data.vertices])
    zmin = float(co[:, 2].min())
    k = 100.0 * S
    m = co[:, 1] < co[:, 1].min() + 0.035
    mc = co[m].mean(0)
    wheel = co[np.abs(co[:, 0]) > 0.52]
    cen = co[np.abs(co[:, 0]) < 0.12]
    vz = cen[(cen[:, 1] > 0.15) & (cen[:, 1] < 0.25)][:, 2].max()
    br = cen[(cen[:, 1] > -0.5) & (cen[:, 1] < 0.0)][:, 2].max()
    wc = np.array([-(wheel[:, 1].min() + wheel[:, 1].max()) / 2 * -1, 0.0])
    yc, zc = (wheel[:, 1].min() + wheel[:, 1].max()) / 2, (wheel[:, 2].min() + wheel[:, 2].max()) / 2
    rr = np.hypot(co[:, 1] - yc, co[:, 2] - zc)
    rim = co[(rr > 0.9 * (wheel[:, 2].max() - wheel[:, 2].min()) / 2) & (np.abs(co[:, 0]) > 0.3)]
    kn = cen[(cen[:, 1] > -0.12) & (cen[:, 1] < 0.12)]
    kv = kn[kn[:, 2].argmax()]
    return {
        'scale': S, 'mesh_size_cm_unscaled': [round(float(x) * 100, 1) for x in (co.max(0) - co.min(0))],
        'muzzle_f': float(-co[:, 1].min()) * k, 'muzzle_u': float(mc[2] - zmin) * k,
        'muzzle_outer_radius': float((co[m][:, 2].max() - co[m][:, 2].min()) / 2) * k,
        'trail_end_f': float(-co[:, 1].max()) * k,
        'axle_f': float(-(wheel[:, 1].min() + wheel[:, 1].max()) / 2) * k, 'axle_u': float((wheel[:, 2].min() + wheel[:, 2].max()) / 2 - zmin) * k,
        'wheel_radius': float((wheel[:, 2].max() - wheel[:, 2].min()) / 2) * k,
        'wheel_outer_l': float(co[:, 0].max()) * k, 'wheel_rim_outer_l': float(np.abs(rim[:, 0]).max()) * k, 'wheel_rim_inner_l': float(np.abs(rim[:, 0]).min()) * k, 'wheel_inner_l': 0.53 * k,
        'vent_f': float(-kv[1]) * k, 'vent_u': float(kv[2] - zmin) * k, 'barrel_top_at_0.2_u': float(vz - zmin) * k, 'breech_top_u': float(br - zmin) * k,
        'cascabel_f': -0.45 * k, 'mesh_origin_height_above_ground': -zmin * k,
        'trail_end_top_u': float(co[co[:, 1] > 0.9][:, 2].max() - zmin) * k,
        'trail_half_width': float(np.abs(co[co[:, 1] > 0.8][:, 0]).max()) * k,
        'zmin_blender': zmin,
    }


GUN = measure_gun(GUN_SCALE)
MF, MU = GUN['muzzle_f'], GUN['muzzle_u']
GROUND_RIG = -100.0


# ---------------------------------------------------------------- station: kanon-ramme F -> rigrum
class Station:
    """Figuren staar paa jorden i F-punktet (cf, cl) og ser i retning phi (grader fra +f mod +l). Rigrum: x venstre, y op, z fremad (cm), jord y=-100."""

    def __init__(self, cf, cl, phi):
        self.cf, self.cl, self.phi = cf, cl, phi
        c, s = math.cos(rad(phi)), math.sin(rad(phi))
        self.R = Matrix(((-s, c, 0), (0, 0, 1), (c, s, 0)))

    def pt(self, p):
        return self.R @ V((p[0] - self.cf, p[1] - self.cl, p[2])) + V((0, GROUND_RIG, 0))

    def vec(self, v):
        return self.R @ V(v)

    def frame(self, xF, yF, zF, pF):
        M = cols_to_mat([self.vec(xF), self.vec(yF), self.vec(zF)]).to_4x4()
        M.translation = self.pt(pF)
        return M

    def info(self):
        c, s = math.cos(rad(self.phi)), math.sin(rad(self.phi))
        return {'F_ground_point_cm': [round(self.cf, 1), round(self.cl, 1)],
                'heading_deg_from_muzzle_dir_toward_left': self.phi,
                'unreal_gun_mesh_local_cm': [round(self.cl, 1), round(self.cf, 1), round(-GUN['mesh_origin_height_above_ground'], 1)],
                'unreal_gun_mesh_forward_xy': [round(s, 3), round(c, 3)]}


# ---------------------------------------------------------------- rig
def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def depth(pb):
    n = 0
    while pb.parent:
        pb = pb.parent
        n += 1
    return n


SX = Matrix.Diagonal((-1, 1, 1, 1))


def mirror(M):
    return SX @ M @ SX


class Rig:
    def __init__(self):
        clear()
        bpy.ops.import_scene.fbx(filepath=str(SOLDIER))
        self.arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
        self.mesh = next(o for o in bpy.data.objects if o.type == 'MESH')
        for o in list(bpy.data.objects):
            if o.type in ('CAMERA', 'LIGHT'):
                bpy.data.objects.remove(o)
        self.sc = bpy.context.scene
        self.sc.render.fps = 30
        for pb in self.arm.pose.bones:
            pb.rotation_mode = 'QUATERNION'
        self.order = sorted(self.arm.pose.bones, key=depth)
        self.names = [pb.name for pb in self.order]
        self.par = {pb.name: (pb.parent.name if pb.parent else None) for pb in self.order}
        self.restL = {b.name: b.matrix_local.copy() for b in self.arm.data.bones}
        self.rrel = {b.name: ((b.parent.matrix_local.inverted() @ b.matrix_local) if b.parent else b.matrix_local.copy()) for b in self.arm.data.bones}
        self.blen = {b.name: b.length for b in self.arm.data.bones}
        self.kids = {n: [] for n in self.names}
        for n, p in self.par.items():
            if p:
                self.kids[p].append(n)
        self.Qf = mirror(self.restL[M_ + 'RightHand']).inverted() @ self.restL[M_ + 'LeftHand']

    def basis_from_P(self, P):
        out = {}
        for n in self.names:
            p = self.par[n]
            loc = (P[p].inverted() @ P[n]) if p else P[n]
            out[n] = self.rrel[n].inverted() @ loc
        return out

    def apply(self, basis):
        for pb in self.order:
            pb.matrix_basis = basis[pb.name]
        bpy.context.view_layer.update()

    def mesh_minz(self):
        ev = self.mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
        me = ev.to_mesh()
        z = min((ev.matrix_world @ v.co).z for v in me.vertices)
        ev.to_mesh_clear()
        return z


def rebuild_subtree(rig, P, root, newP):
    old = {n: P[n].copy() for n in P}
    P[root] = newP
    stack = list(rig.kids[root])
    while stack:
        n = stack.pop()
        p = rig.par[n]
        P[n] = P[p] @ (old[p].inverted() @ old[n])
        stack.extend(rig.kids[n])


def rot_about(rig, P, bone, R3, pivot=None):
    M = P[bone]
    piv = pivot if pivot is not None else M.translation.copy()
    t = piv + R3 @ (M.translation - piv)
    rebuild_subtree(rig, P, bone, T4(t) @ R4(R3 @ M.to_3x3()))


def aim(rig, P, bone, new_dir):
    M = P[bone]
    cur = (M.to_3x3() @ V((0, 1, 0))).normalized()
    rot = cur.rotation_difference(new_dir.normalized()).to_matrix()
    rebuild_subtree(rig, P, bone, T4(M.translation) @ R4(rot @ M.to_3x3()))


def two_bone_ik(rig, P, upper, lower, target, pole):
    S = P[upper].translation.copy()
    l1, l2 = rig.blen[upper], rig.blen[lower]
    dv = target - S
    d = min(dv.length, (l1 + l2) * 0.9995)
    d = max(d, abs(l1 - l2) + 0.5)
    u = dv.normalized()
    p = pole - pole.dot(u) * u
    p = p.normalized() if p.length > 1e-6 else V((0, 0, 1))
    a = (l1 * l1 - l2 * l2 + d * d) / (2 * d)
    h = math.sqrt(max(l1 * l1 - a * a, 0.0))
    E = S + u * a + p * h
    aim(rig, P, upper, E - S)
    aim(rig, P, lower, target - P[lower].translation)


def rhand_frame(palm, finger):
    """Hoejre haandknogles akser: X = haandfladens normal, Y = fingre, Z = X x Y (tommel = -Z). Rigrum-4x4 (uden position)."""
    X = V(palm).normalized()
    Y = V(finger)
    Y = (Y - Y.dot(X) * X).normalized()
    Z = X.cross(Y)
    return cols_to_mat([X, Y, Z]).to_4x4()


def with_pos(M, p):
    M = M.copy()
    M.translation = V(p)
    return M


def base_spec():
    return dict(hips=V((0, -1.0, 0)), hy=0.0, hp=0.0, hr=0.0, lean=0.0, twist=0.0, side=0.0, hdy=0.0, hdp=0.0,
                footL=dict(pos=V((11.5, -87, -4)), yaw=0.0, heel=0.0), footR=dict(pos=V((-11.5, -87, -4)), yaw=0.0, heel=0.0),
                knee_out=0.15, handL=None, handR=None)


def hand_spec(M, pole, curl=0.0, torso=False):
    return dict(M=M, pole=V(pole), curl=curl, torso=torso)


FINGER_CURL = (1.0, 1.25, 0.8)


def solve(rig, sp):
    """spec -> ({knogle: armaturrums-matrix cm}, fejl)."""
    P = {n: rig.restL[n].copy() for n in rig.names}
    hips = M_ + 'Hips'
    h0 = P[hips].translation.copy()
    Rh = Ry(sp['hy']) @ Rx(sp['hp']) @ Rz(sp['hr'])
    T = T4(h0 + sp['hips']) @ R4(Rh) @ T4(-h0)
    for n in rig.names:
        P[n] = T @ P[n]
    for bone, w in (('Spine', 0.25), ('Spine1', 0.35), ('Spine2', 0.4)):
        ld = sp.get('lean_dir', V((0, 0, 1)))
        Rl = Matrix.Rotation(rad(sp['lean'] * w), 3, V((0, 1, 0)).cross(ld).normalized())
        rot_about(rig, P, M_ + bone, Ry(sp['twist'] * w) @ Rl @ Rz(sp['side'] * w))
    for bone, w in (('Neck', 0.4), ('Head', 0.6)):
        rot_about(rig, P, M_ + bone, Ry(sp['hdy'] * w) @ Rx(sp['hdp'] * w))
    err = {}
    for side, key, sgn in (('Left', 'footL', 1), ('Right', 'footR', -1)):
        ft = sp[key]
        up, lo, fo, to = M_ + side + 'UpLeg', M_ + side + 'Leg', M_ + side + 'Foot', M_ + side + 'ToeBase'
        tgt = ft['pos'].copy()
        pole = Ry(sp['hy'] + ft['yaw']) @ V((sgn * sp['knee_out'], 0, 1))
        two_bone_ik(rig, P, up, lo, tgt, pole)
        ax = Ry(ft['yaw']) @ V((1, 0, 0))
        Rf = Matrix.Rotation(rad(ft['heel']), 3, ax) @ Ry(ft['yaw']) @ rig.restL[fo].to_3x3()
        rebuild_subtree(rig, P, fo, T4(P[fo].translation) @ R4(Rf))
        if ft['heel']:
            rot_about(rig, P, to, Matrix.Rotation(rad(-ft['heel']), 3, ax))
        err[side + 'Foot'] = round((P[fo].translation - tgt).length, 2)
    for side, key, sgn in (('Left', 'handL', 1), ('Right', 'handR', -1)):
        hs = sp[key]
        if hs is None:
            continue
        up, lo, ha, sh = M_ + side + 'Arm', M_ + side + 'ForeArm', M_ + side + 'Hand', M_ + side + 'Shoulder'
        Mt = hs['M']
        if hs['torso']:
            Mt = (P[M_ + 'Spine2'] @ rig.restL[M_ + 'Spine2'].inverted()) @ Mt
        tgt = Mt.translation.copy()
        sj = P[sh].translation.copy()
        cur = (P[up].translation - sj).normalized()
        want = (tgt - sj).normalized()
        ang = cur.angle(want, 0.0)
        if ang > 1e-4:
            kk = min(0.3 * ang, rad(22)) / ang
            rot_about(rig, P, sh, cur.rotation_difference(cur.slerp(want, kk)).to_matrix())
        two_bone_ik(rig, P, up, lo, tgt, hs['pole'])
        Rt = Mt.to_3x3()
        Fo = P[lo].to_3x3()
        rel = Fo.inverted() @ Rt
        rel0 = rig.restL[lo].to_3x3().inverted() @ rig.restL[ha].to_3x3()
        q = (rel @ rel0.inverted()).to_quaternion()
        tw = 2 * math.atan2(q.y, q.w)
        tw = (tw + math.pi) % (2 * math.pi) - math.pi
        rot_about(rig, P, lo, Matrix.Rotation(0.5 * tw, 3, (Fo @ V((0, 1, 0))).normalized()))
        rebuild_subtree(rig, P, ha, T4(P[ha].translation) @ R4(Rt))
        err[side + 'Hand'] = round((P[ha].translation - tgt).length, 2)
        if hs['curl']:
            Zl = (P[ha].to_3x3() @ V((0, 0, 1))).normalized()
            for i, w in enumerate(FINGER_CURL, 1):
                rot_about(rig, P, M_ + side + 'HandIndex%d' % i, Matrix.Rotation(rad(sgn * hs['curl'] * w), 3, Zl))
    return P, err


# ---------------------------------------------------------------- rekvisit-greb
GRIP_O = V((3.2, 6.0, 0.0))     # skaftets midte i haandknoglens lokale rum (haandfladesiden = +X, 6 cm fra haandleddet)
BALL_O = V((4.2, 6.0, 0.0))
STAFF_TIP_Z = 133.3             # hovedets spids paa SM_Sponge / SM_Rammer (cm fra origo = graebspunkt)


def staff_socket(zR):
    """Prop-rum -> hoejre haandknogle-lokal (4x4): overhaandsgreb, tommel mod hovedet (+Z), haanden paa skaftpunkt z=zR (cm fra rekvisittens origo)."""
    Rp = cols_to_mat([V((0, -1, 0)), V((-1, 0, 0)), V((0, 0, -1))])
    M = Rp.to_4x4()
    M.translation = GRIP_O - Rp @ V((0, 0, zR))
    return M


def staff_hands(rig, Pp, zR, zL):
    HR = Pp @ staff_socket(zR).inverted()
    HL = Pp @ (SX @ staff_socket(zL).inverted() @ SX) @ rig.Qf
    return HR, HL


def ball_socket():
    M = Matrix.Identity(4)
    M.translation = BALL_O
    return M


def staff_frame(st, tip_f, delta, twist, lateral=0.0):
    """Skaft i F: linje gennem mundingens midte med stigning delta (spidsen hoejere end haanden); spidsen ved f=tip_f. -> (prop-ramme i rigrum, spids i F)."""
    cd, sd = math.cos(rad(delta)), math.sin(rad(delta))
    a = V((-cd, 0, sd))
    s = (MF - tip_f) / cd
    tipF = V((tip_f, lateral, MU + s * sd))
    piv = tipF - a * STAFF_TIP_Z
    xp = V((0, -1, 0))
    yp = a.cross(xp)
    c, sn = math.cos(rad(twist)), math.sin(rad(twist))
    return st.frame(xp * c + yp * sn, -xp * sn + yp * c, a, piv), tipF


# ---------------------------------------------------------------- hjaelpere til klip
POLE_R = V((-0.5, -0.9, -0.3))
POLE_L = V((0.5, -0.9, -0.3))


def lhand_frame(rig, palm, finger):
    """Venstre haandknogle-orientering der er spejlbilledet af en hoejre haand med spejlede vektorer (palm/finger angives i rigrum)."""
    mv = lambda v: V((-v[0], v[1], v[2]))
    return lhand_from_right(rig, rhand_frame(mv(palm), mv(finger)))


def lhand_from_right(rig, Mr):
    return mirror(Mr) @ rig.Qf


def relaxed(rig, side, curl=18.0, dz=1.0):
    Mr = with_pos(rhand_frame((1, 0, 0), (0, -1, 0)), (-23.5, -6, dz))
    if side == 'R':
        return hand_spec(Mr, POLE_R, curl, True)
    return hand_spec(lhand_from_right(rig, Mr), POLE_L, curl, True)


def quat_slerp_mat(A, B, w):
    qa, qb = A.to_3x3().to_quaternion(), B.to_3x3().to_quaternion()
    return qa.slerp(qb, w).to_matrix().to_4x4()


def feet_default(sp, dz=0.0):
    sp['footL']['pos'] = V((11.5, -87, -4 + dz))
    sp['footR']['pos'] = V((-11.5, -87, -4 + dz))


def smooth(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


# ---------------------------------------------------------------- KLIP
TIPOFF = MF - 218.8        # skaftspidsens baner er givet relativt til mundingsfladen (designet ved skala 2,2)
D0 = max(0.0, min(14.0, math.degrees(math.atan((MU - 128.0) / 88.0))))   # skaftets stigning (grader): haenderne ca. 128 cm over jorden
def clip_standby(rig, st):
    N = 90

    def spec(f):
        ph = 2 * math.pi * f / N
        sp = base_spec()
        sp['hips'] = V((1.0 * math.sin(ph), -1.0 + 0.35 * math.sin(2 * ph), 0))
        sp['side'] = 0.8 * math.sin(ph)
        sp['lean'] = 1.0 + 0.7 * math.sin(2 * ph)
        sp['hdy'] = 5 * math.sin(ph + 0.6)
        sp['hdp'] = -1.0 + 1.0 * math.sin(2 * ph + 1.0)
        sp['handR'] = relaxed(rig, 'R', 18)
        sp['handL'] = relaxed(rig, 'L', 18)
        return sp
    return dict(N=N, loop=True, spec=spec, prop=None, events={}, cam=(-60.0, 3.2, -1))


def staff_clip(rig, st, name, tipkeys, twistkeys, deltakeys, N, step_in, step_out, zR=45.0, zL=80.0, ev=None):
    tip = Tr(tipkeys)
    twist = Tr(twistkeys)
    delta = Tr(deltakeys)
    tip0 = tipkeys[0][1]
    adv = st.vec((-1, 0, 0))
    adv.y = 0.0
    adv.normalize()
    fz = Tr([(0, 0.0), (step_in[0], 0.0), (step_in[1], 24.0), (step_out[0], 24.0), (step_out[1], 0.0), (N, 0.0)])
    lift = Tr([(0, 0.0), (step_in[0], 0.0), ((step_in[0] + step_in[1]) / 2, 7.0), (step_in[1], 0.0),
               (step_out[0], 0.0), ((step_out[0] + step_out[1]) / 2, 7.0), (step_out[1], 0.0), (N, 0.0)])

    def spec(f):
        sp = base_spec()
        trav = tip0 - tip(f)
        sh = 0.55 * trav
        sp['hips'] = adv * sh + V((0, -1.0 - 0.28 * sh, 0))
        sp['lean_dir'] = adv
        sp['lean'] = 6.0 + 0.35 * trav
        sp['hdp'] = -4.0
        sp['footR']['pos'] = V((-11.5, -87, -8))
        sp['footL']['pos'] = V((11.5, -87, -4)) + adv * fz(f) + V((0, lift(f), 0))
        Pp, tipF = staff_frame(st, tip(f), delta(f), twist(f))
        HR, HL = staff_hands(rig, Pp, zR, zL - 0.0 * trav)
        sp['handR'] = hand_spec(HR, V((-0.4, -1.0, -0.1)), 35)
        sp['handL'] = hand_spec(HL, V((0.4, -1.0, -0.1)), 35)
        sp['_tipF'] = tipF
        return sp
    return dict(N=N, loop=False, spec=spec, prop=dict(name=name, socket=staff_socket(zR), zR=zR, zL=zL), events=ev or {}, cam=(260.0, 3.6, -1))


def clip_sponge(rig, st):
    N = 90
    tipk = [(t, v + TIPOFF) for t, v in [(0, 232), (6, 232), (30, 190), (54, 190), (80, 232), (90, 232)]]
    twk = [(0, 0), (30, 0), (40, 110), (46, 110), (54, 0), (90, 0)]
    dk = [(0, D0), (30, D0 * 0.57), (54, D0 * 0.57), (80, D0), (90, D0)]
    return staff_clip(rig, st, 'SM_Sponge', tipk, twk, dk, N, (4, 16), (66, 80), ev={'deepest_insertion_frames': [30, 54], 'twist_frames': [30, 54]})


def clip_ram(rig, st):
    N = 75
    tipk = [(t, v + TIPOFF) for t, v in [(0, 232), (8, 232), (22, 196), (28, 206), (40, 184), (46, 186), (66, 232), (75, 232)]]
    twk = [(0, 0), (75, 0)]
    dk = [(0, D0), (22, D0 * 0.57), (40, D0 * 0.5), (66, D0), (75, D0)]
    return staff_clip(rig, st, 'SM_Rammer', tipk, twk, dk, N, (4, 14), (58, 70), ev={'thrust_1_frame': 22, 'thrust_2_frame': 40})


def clip_load(rig, st):
    N = 75
    ballp = Tr([(0, V((-18, -4, 34))), (12, V((-31, 8, 46))), (14, V((-31, 8, 46))), (34, st.pt((MF + 16, 0, MU - 1))),
                (44, st.pt((MF - 6, 0, MU - 1))), (46, st.pt((MF - 6, 0, MU - 1))), (60, V((-18, -4, 34))), (75, V((-18, -4, 34)))])
    wk = Tr([(0, 0.0), (14, 0.0), (34, 1.0), (46, 1.0), (60, 0.0), (75, 0.0)])
    pa = rhand_frame((0, 1, 0), (0, 0, 1))
    pb = rhand_frame((0, 1, 0), st.vec((-1, 0, 0)))
    lean = Tr([(0, 6.0), (14, 10.0), (34, 24.0), (46, 24.0), (60, 6.0), (75, 6.0)])
    hz = Tr([(0, 0.0), (14, 2.0), (34, 9.0), (46, 9.0), (60, 0.0), (75, 0.0)])
    twist = Tr([(0, 0.0), (14, 8.0), (34, -14.0), (46, -14.0), (60, 0.0), (75, 0.0)])

    def spec(f):
        sp = base_spec()
        sp['lean'] = lean(f)
        sp['twist'] = twist(f)
        sp['hips'] = V((0, -1.5 - 0.3 * hz(f), hz(f)))
        sp['hdp'] = 3.0
        R = quat_slerp_mat(pa, pb, wk(f))
        Hb = with_pos(R, ballp(f) - R.to_3x3() @ BALL_O)
        sp['handR'] = hand_spec(Hb, V((-0.5, -1.0, -0.2)), 22)
        sp['handL'] = relaxed(rig, 'L', 18)
        sp['handL']['M'] = with_pos(sp['handL']['M'], V((23.5, -3, 4)))
        sp['_ballF'] = ballp(f)
        return sp
    return dict(N=N, loop=False, spec=spec, prop=dict(name='SM_Cannonball_12pdr', socket=ball_socket(), visible=(14, 44)),
                events={'ball_taken_frame': 14, 'ball_released_frame': 44}, cam=(215.0, 3.0, 1))


def clip_prime(rig, st):
    N = 60
    vent = st.pt((GUN['vent_f'], -3, GUN['vent_u'] - 13))
    pv = rhand_frame(st.vec((GUN['vent_f'] - st.cf, -st.cl, 0)), (0, 1, 0))
    ready = V((-16, 18, 26))
    r_fire0, r_fire1 = V((-29, 6, 34)), V((-27, 0, -4))
    pr = rhand_frame((1, 0, 0), (0, 1, 0))
    pl = rhand_frame((0, -1, 0), (0, 0, 1))
    hp = Tr([(0, ready), (8, ready + V((0, 6, 6))), (20, vent), (26, vent), (32, ready), (38, r_fire0), (42, r_fire0), (44, r_fire1),
             (50, V((-24, -8, -6))), (60, ready)])
    w_vent = Tr([(0, 0.0), (8, 0.2), (20, 1.0), (26, 1.0), (32, 0.0), (60, 0.0)])
    w_pull = Tr([(0, 0.0), (32, 0.0), (38, 1.0), (50, 1.0), (60, 0.0)])
    reach = Tr([(0, 0.0), (8, 0.3), (20, 1.0), (26, 1.0), (32, 0.1), (60, 0.0)])
    sidex = Tr([(0, 0.0), (28, 0.0), (40, -22.0), (48, -22.0), (58, 0.0), (60, 0.0)])
    fr_x = Tr([(0, 0.0), (26, 0.0), (34, -22.0), (50, -22.0), (56, 0.0), (60, 0.0)])
    fl_x = Tr([(0, 0.0), (32, 0.0), (40, -22.0), (46, -22.0), (52, 0.0), (60, 0.0)])
    liftR = Tr([(0, 0.0), (26, 0.0), (30, 6.0), (34, 0.0), (50, 0.0), (53, 6.0), (56, 0.0), (60, 0.0)])
    liftL = Tr([(0, 0.0), (32, 0.0), (36, 6.0), (40, 0.0), (46, 0.0), (49, 6.0), (52, 0.0), (60, 0.0)])
    lean = Tr([(0, 5.0), (20, 40.0), (26, 40.0), (32, 8.0), (44, 6.0), (47, -7.0), (52, 0.0), (60, 5.0)])
    hdy = Tr([(0, 0.0), (20, 0.0), (32, 0.0), (44, 0.0), (48, -30.0), (54, -20.0), (60, 0.0)])
    toe = Tr([(0, 0.0), (8, 4.0), (20, 9.0), (26, 9.0), (32, 0.0), (60, 0.0)])
    lh_up = Tr([(0, 0.0), (44, 0.0), (48, 1.0), (54, 0.6), (60, 0.0)])

    def spec(f):
        sp = base_spec()
        sp['lean'] = lean(f)
        sp['hdy'] = hdy(f)
        sp['hips'] = V((sidex(f), -1.0 - 0.15 * lean(f), 0.35 * lean(f)))
        sp['footR']['pos'] = V((-11.5 + fr_x(f), -87 + liftR(f), -4))
        sp['footL']['pos'] = V((11.5 + fl_x(f), -87 + liftL(f), -4))
        t = toe(f)
        for k in ('footL', 'footR'):
            sp[k]['pos'] = sp[k]['pos'] + V((0, t, 0))
        R = quat_slerp_mat(pr, pv, max(w_vent(f), 0.0))
        R = quat_slerp_mat(R, pr, min(1.0, w_pull(f)))
        sp['handR'] = hand_spec(with_pos(R, hp(f)), V((-0.6, -0.7, -0.3)), 20)
        up = lh_up(f)
        Ml = relaxed(rig, 'L', 18)['M']
        Mup = with_pos(lhand_frame(rig, (1, 0, 0), (0, 1, 0)), V((20, 56, 10)))
        sp['handL'] = hand_spec(Ml if up < 0.01 else with_pos(quat_slerp_mat(Ml, Mup, up), Ml.translation.lerp(Mup.translation, up)), POLE_L, 18, up < 0.01)
        return sp
    return dict(N=N, loop=False, spec=spec, prop=None, events={'friction_tube_in_vent_frame': 22, 'lanyard_pull_frame_FIRE': 44, 'recoil_flinch_frames': [44, 54]},
                cam=(-30.0, 3.0, -1))


def clip_aim(rig, st):
    CROUCH = 1.1 * max(0.0, 58.0 - GUN['trail_end_top_u'])
    N = 90
    trail_f = GUN['trail_end_f'] + 4
    hR = with_pos(rhand_frame((0, -1, 0), st.vec((1, 0, 0)) if False else st.vec((1, 0, 0))), st.pt((trail_f, -12, GUN['trail_end_top_u'] - 2)))
    hLf = lhand_frame(rig, (0, -1, 0), st.vec((1, 0, 0)))
    hLm = with_pos(hLf, st.pt((trail_f, 12, GUN['trail_end_top_u'] - 2)))
    sig = with_pos(rhand_frame(st.vec((1, 0, 0)), (0, 1, 0)), V((-24, 52, 18)))
    wsig = Tr([(0, 0.0), (44, 0.0), (54, 1.0), (66, 1.0), (76, 0.0), (90, 0.0)], loop=90)
    lean = Tr([(0, 52.0), (44, 52.0), (54, 40.0), (66, 40.0), (76, 52.0), (90, 52.0)], loop=90)

    def spec(f):
        ph = 2 * math.pi * f / N
        w = wsig(f)
        sp = base_spec()
        sp['lean'] = lean(f) + 1.2 * math.sin(ph * 2) + 1.0 * CROUCH
        sp['hips'] = V((0, -24.0 - CROUCH + 1.0 * math.sin(ph * 2), -2.0))
        sp['hdp'] = -44.0 + 2.0 * math.sin(ph)
        sp['hdy'] = 4.0 * math.sin(ph) + 6.0 * w
        sp['twist'] = -4.0 * w
        sp['footR']['pos'] = V((-13.5, -87, -4))
        sp['footL']['pos'] = V((13.5, -87, -4))
        Mr = hR
        Mr2 = with_pos(quat_slerp_mat(hR, sig, w), hR.translation.lerp(sig.translation, w))
        sp['handR'] = hand_spec(Mr2, V((-0.8, -0.8, -0.3)), 30 * (1 - w) + 5 * w)
        sp['handL'] = hand_spec(hLm, V((0.8, -0.8, -0.3)), 30)
        return sp
    return dict(N=N, loop=True, spec=spec, prop=None, events={'signal_frames': [48, 72]}, cam=(-120.0, 3.6, -1))


def clip_push(rig, st):
    PC = 0.5 * max(0.0, 101.1 - GUN['wheel_radius'])
    N = 36
    S = 40.0
    swing = 14.0
    d = st.vec((1, 0, 0))
    d.y = 0.0
    d.normalize()
    yawf = math.degrees(math.atan2(d.x, d.z))

    def foot_s(f):
        f = f % N
        if f < swing:
            return -S / 2 + S * smooth(f / swing)
        return S / 2 - S * (f - swing) / (N - swing)

    def foot_lift(f):
        f = f % N
        return 7.0 * math.sin(math.pi * f / swing) if f < swing else 0.0
    cx = GUN['axle_f']
    cu = GUN['axle_u']
    R = GUN['wheel_radius'] * 0.9
    lw = -(GUN['wheel_rim_outer_l'] + 1.0)
    pa, pb = math.radians(142), math.radians(176 if GUN["wheel_radius"] < 90 else 196)
    pU = st.pt((cx + R * math.cos(pa), lw, cu + R * math.sin(pa)))
    pD = st.pt((cx + R * math.cos(pb), lw, cu + R * math.sin(pb)))
    palm = st.vec((1, 0, 0.0))
    fing = st.vec((0, 1, 0))
    MrU = with_pos(rhand_frame(palm, fing), pD)
    MlU = with_pos(lhand_frame(rig, palm, fing), pU)

    def spec(f):
        ph = 2 * math.pi * f / N
        sp = base_spec()
        sp['lean'] = 15.0 + 1.0 * PC + 1.0 * math.sin(2 * ph)
        sp['hips'] = V((1.2 * math.sin(ph), -4.0 - PC - 1.0 * abs(math.sin(ph)), 3.0))
        sp['hy'] = yawf * 0.0
        for key, sgn, off in (('footL', 1, 0), ('footR', -1, N / 2)):
            s = foot_s(f + off)
            sp[key]['pos'] = V((sgn * 11.5, -87 + foot_lift(f + off), -4)) + d * s
            sp[key]['yaw'] = yawf
        sp['handR'] = hand_spec(MrU, V((-0.7, -0.8, -0.3)), 28)
        sp['handL'] = hand_spec(MlU, V((0.7, -0.8, -0.3)), 28)
        sp['hdp'] = -6.0
        return sp
    return dict(N=N, loop=True, spec=spec, prop=None, events={'step_length_cm': S, 'swing_frames': swing, 'foot_swing_forward_direction_rig': [round(x, 3) for x in d]},
                cam=(40.0, 3.4, -1))


STATIONS = {
    2.2: {'StandBy': (-100.0, -75.0, 90.0), 'Sponge': (290.0, -34.0, 75.0), 'Load': (200.0, 46.0, 0.0), 'Ram': (290.0, -34.0, 75.0),
          'Prime': (-8.0, -52.0, 100.0), 'Aim': (-232.0, 0.0, 0.0), 'Push_Loop': (-10.0, -146.0, 120.0)},
}
STATIONS[1.6] = {'StandBy': (-72.7, -54.5, 90.0), 'Sponge': (210.9, -24.7, 75.0), 'Load': (130.0, 38.0, 0.0), 'Ram': (210.9, -24.7, 75.0),
                 'Prime': (-5.8, -42.0, 100.0), 'Aim': (-168.7, 0.0, 0.0), 'Push_Loop': (-27.3, -121.2, 60.0)}
_BUILD = {'StandBy': clip_standby, 'Sponge': clip_sponge, 'Load': clip_load, 'Ram': clip_ram, 'Prime': clip_prime, 'Aim': clip_aim, 'Push_Loop': clip_push}
_st = STATIONS.get(GUN_SCALE)
if _st is None:   # ukendt skala: start fra 2,2-stationerne skaleret (kør FIT=Navn og indsaet i STATIONS)
    _k = GUN_SCALE / 2.2
    _st = {n: (v[0] * _k, v[1] * _k, v[2]) for n, v in STATIONS[2.2].items()}
CLIPS = {n: (_BUILD[n], _st[n]) for n in _BUILD}
if os.environ.get('ST'):
    # ST=Navn:cf,cl,phi overstyrer station (til afproevning)
    for item in os.environ['ST'].split(';'):
        nm, v = item.split(':')
        CLIPS[nm] = (CLIPS[nm][0], tuple(float(x) for x in v.split(',')))


# ---------------------------------------------------------------- evaluering, maal, eksport
def evaluate(rig, name):
    builder, stv = CLIPS[name]
    st = Station(*stv)
    C = builder(rig, st)
    C['name'], C['st'] = name, st
    frames = range(C['N']) if C['loop'] else range(C['N'] + 1)
    C['Ps'], C['bases'], C['errs'], C['specs'] = [], [], [], []
    for f in frames:
        sp = C['spec'](f)
        P, err = solve(rig, sp)
        C['Ps'].append(P)
        C['bases'].append(rig.basis_from_P(P))
        C['errs'].append(err)
        C['specs'].append(sp)
    return C


def max_err(C):
    out = {}
    for e in C['errs']:
        for k, v in e.items():
            out[k] = max(out.get(k, 0.0), v)
    return out


def root_motion(C):
    h = [(P[M_ + 'Hips'].translation.x, P[M_ + 'Hips'].translation.z, P[M_ + 'Hips'].translation.y) for P in C['Ps']]
    rng = [round(max(x[i] for x in h) - min(x[i] for x in h), 1) for i in range(3)]
    net = [round(h[-1][i] - h[0][i], 2) for i in range(3)]
    return {'excursion_x_z_y_cm': rng, 'net_first_to_last_x_z_y_cm': net}


def foot_slide(C):
    """Ankelens maks. vandrette flytning pr. frame (cm) mellem to paa hinanden foelgende frames hvor foden er paa jorden (y < laveste + 0.5 cm): 0 = foden staar fast;
    en tilsigtet skridt giver kun loeft (foden er i luften) og ingen glid."""
    res = {}
    for side in ('Left', 'Right'):
        pts = [P[M_ + side + 'Foot'].translation.copy() for P in C['Ps']]
        ymin = min(p.y for p in pts)
        n = len(pts)
        mv = 0.0
        g = 0
        for i in range(n):
            if pts[i].y < ymin + 0.5:
                g += 1
            j = i + 1
            if j >= n:
                if not C['loop']:
                    continue
                j = 0
            if pts[i].y < ymin + 0.5 and pts[j].y < ymin + 0.5:
                mv = max(mv, math.hypot(pts[j].x - pts[i].x, pts[j].z - pts[i].z))
        res[side] = {'on_ground_frames': g, 'of': n, 'max_horizontal_move_per_frame_cm': round(mv, 2)}
    return res


def stance_speed(C):
    """Standfodens hastighed relativt til kroppen (cm/s) = klippets implicitte gangfart: median af -dz/dt langs gangretningen for frames med foden paa jorden."""
    d = C['spec'] and None
    res = {}
    dirv = V((C['events'].get('foot_swing_forward_direction_rig') or [0, 0, 1]))
    for side in ('Left', 'Right'):
        pts = [P[M_ + side + 'Foot'].translation.copy() for P in C['Ps']]
        n = len(pts)
        ymin = min(p.y for p in pts)
        v = []
        for i in range(n):
            j = (i + 1) % n
            if pts[i].y < ymin + 0.5 and pts[j].y < ymin + 0.5:
                v.append(-(pts[j] - pts[i]).dot(dirv) * 30.0)
        v.sort()
        res[side] = {'stance_frames': len(v), 'speed_cm_s_median': round(v[len(v) // 2], 1) if v else None}
    return res


HIPS_REST_FIX_CM = float(os.environ.get('HIPS_FIX', '98.079'))   # soldaternes/officerens klip har hofteknoglen ~93,5 cm oppe; DK_Infantry_1864.fbx staar med hoften ved ca. -4,6


def with_hips_fix(rig, bases):
    """Loefter hofteknoglen HIPS_REST_FIX_CM (rigrum, op) saa Lcl Translation paa Hips svarer til A_Officer_Idle / A_Combat_*-klippene. Kun eksporten bruges; preview/maal bruger de ufiksede poser."""
    h = M_ + 'Hips'
    out = []
    for b in bases:
        nb = dict(b)
        P = rig.rrel[h] @ b[h]
        nb[h] = rig.rrel[h].inverted() @ (T4(V((0, HIPS_REST_FIX_CM, 0))) @ P)
        out.append(nb)
    return out


def bake(rig, name, bases, path):
    bases = with_hips_fix(rig, bases)
    backup(path)
    arm = rig.arm
    arm.animation_data_clear()
    arm.animation_data_create()
    act = bpy.data.actions.new(name)
    arm.animation_data.action = act
    try:
        arm.animation_data.action_slot = act.slots[0] if act.slots else act.slots.new('OBJECT', arm.name)
    except Exception:
        pass
    for i, basis in enumerate(bases, 1):
        for pb in arm.pose.bones:
            pb.matrix_basis = basis[pb.name]
            pb.keyframe_insert('location', frame=i)
            pb.keyframe_insert('rotation_quaternion', frame=i)
            pb.keyframe_insert('scale', frame=i)
    rig.sc.frame_start, rig.sc.frame_end = 1, len(bases)
    for o in bpy.data.objects:
        o.select_set(False)
    arm.select_set(True)
    rig.mesh.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, use_mesh_modifiers=False,
                             bake_anim=True, axis_forward='-Z', axis_up='Y', bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                             bake_anim_simplify_factor=0, bake_anim_force_startend_keying=True)
    arm.animation_data_clear()
    return {'name': name, 'file': Path(path).name, 'frames': len(bases), 'fps': 30}


def ue_socket(M):
    """Blender-knogle-lokal 4x4 (prop -> knogle) -> Unreal-konvention som notes.md i Officer/Final_v5 (Y spejlet; FRotator fra kolonne-akser)."""
    S = Matrix.Diagonal((1, -1, 1))
    R = S @ M.to_3x3() @ S
    Ax, Ay, Az = R.col[0], R.col[1], R.col[2]
    yaw = math.degrees(math.atan2(Ax[1], Ax[0]))
    pitch = math.degrees(math.atan2(Ax[2], math.hypot(Ax[0], Ax[1])))
    sy = V((-math.sin(rad(yaw)), math.cos(rad(yaw)), 0))
    roll = math.degrees(math.atan2(Az.dot(sy), Ay.dot(sy)))
    t = M.translation
    return {'location_cm': [round(t.x, 2), round(-t.y, 2), round(t.z, 2)], 'rotation_PYR_deg': [round(pitch, 2), round(yaw, 2), round(roll, 2)], 'scale': [1.0, 1.0, 1.0]}


def prop_checks(rig, C):
    """Rekvisit-kontrol: spids/kugle i F-ramme (cm) i nogle nøgleframes + afstand haand-skaft."""
    st = C['st']
    pr = C['prop']
    if not pr:
        return None
    inv = Matrix(((0, -1, 0), (1, 0, 0), (0, 0, 1)))  # ubrugt
    res = {}
    Rinv = st.R.inverted()

    def toF(pr_):
        q = Rinv @ (pr_ - V((0, GROUND_RIG, 0)))
        return [round(q[0] + st.cf, 1), round(q[1] + st.cl, 1), round(q[2], 1)]
    series = []
    for P in C['Ps']:
        M = P[M_ + 'RightHand'] @ pr['socket']
        if pr['name'] == 'SM_Cannonball_12pdr':
            series.append(toF(M.translation))
        else:
            tip = M @ V((0, 0, STAFF_TIP_Z))
            series.append(toF(tip))
    res['prop_point_F_per_frame'] = series
    return res


# ---------------------------------------------------------------- forhaandsvisning
WM = Matrix(((0.01, 0, 0, 0), (0, 0, -0.01, 0), (0, 0.01, 0, 0), (0, 0, 0, 1)))


def tex_material(obj, png, color=(0.6, 0.6, 0.6, 1)):
    mat = bpy.data.materials.new('pv_' + obj.name[:20])
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    if png and Path(png).exists():
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(str(png))
        nt.links.new(tex.outputs[0], bsdf.inputs['Base Color'])
        nt.nodes.active = tex
    mat.diffuse_color = color
    obj.data.materials.clear()
    obj.data.materials.append(mat)


def import_obj(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == 'MESH']
    for o in new:
        if o.type in ('CAMERA', 'LIGHT', 'EMPTY') and o not in meshes:
            bpy.data.objects.remove(o)
    return meshes[0]


def setup_preview(rig):
    sc = rig.sc
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 560, 420
    sh = sc.display.shading
    sh.light = 'STUDIO'
    sh.color_type = 'TEXTURE'
    sh.show_shadows = True
    sc.world = bpy.data.worlds.new('w')
    sc.world.color = (0.82, 0.86, 0.9)
    sc.view_settings.view_transform = 'Standard'
    tex_material(rig.mesh, SOLDIER_TEX)
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
    cam.data.type = 'ORTHO'
    cam.data.clip_end = 100
    sc.collection.objects.link(cam)
    sc.camera = cam
    gun = import_obj(CANNON)
    if CANNON_TEX.exists():
        tex_material(gun, CANNON_TEX)
    props = {}
    for nm, tx in (('SM_Sponge', 'T_Sponge_BaseColor.png'), ('SM_Rammer', 'T_Rammer_BaseColor.png'), ('SM_Cannonball_12pdr', 'T_Cannonball_12pdr_BaseColor.png')):
        o = import_obj(PROPS / (nm + '.fbx'))
        tex_material(o, PROPS / tx)
        o.hide_render = True
        props[nm] = o
    bpy.ops.mesh.primitive_cube_add(size=1)
    g = bpy.context.object
    g.scale = (30, 30, 0.08)
    g.location = (0, 0, -1.04)
    gm = bpy.data.materials.new('gnd')
    gm.diffuse_color = (0.38, 0.46, 0.3, 1)
    g.data.materials.append(gm)
    return cam, gun, props


def place_gun(gun, st):
    k = 100.0 * GUN_SCALE
    A = Matrix(((0, -k, 0, 0), (k, 0, 0, 0), (0, 0, k, -GUN['zmin_blender'] * k), (0, 0, 0, 1)))
    Rm = T4(V((0, GROUND_RIG, 0)) - st.R @ V((st.cf, st.cl, 0))) @ R4(st.R)
    gun.matrix_world = WM @ Rm @ A


def compose_sheet(files, out, cols):
    code = ("import sys\nfrom PIL import Image, ImageDraw\nout=sys.argv[1];cols=int(sys.argv[2]);fs=sys.argv[3:]\n"
            "ims=[]\nfor a in fs:\n    f,l=a.split('::')\n    im=Image.open(f).convert('RGB')\n    ImageDraw.Draw(im).text((8,6),l,fill=(0,0,0))\n    ims.append(im)\n"
            "w,h=ims[0].size\nrows=(len(ims)+cols-1)//cols\ns=Image.new('RGB',(w*cols,h*rows),(255,255,255))\n"
            "[s.paste(im,((i%cols)*w,(i//cols)*h)) for i,im in enumerate(ims)]\ns.save(out)\n")
    tmp = CACHE / '_sheet.py'
    tmp.write_text(code)
    subprocess.run(['python', '-I', str(tmp), str(out), str(cols)] + files, check=True)


def render_clip(rig, cam, gun, props, C, outdir):
    st, sc = C['st'], rig.sc
    place_gun(gun, st)
    fc, width, side = C['cam']
    fc = fc * GUN_SCALE / 2.2
    width = width * max(0.95, GUN_SCALE / 2.2 * 1.15)
    cpt = WM @ st.pt((fc, 0, 108))
    n = len(C['Ps'])
    fr = [min(n - 1, int(round(x * (n - 1)))) for x in ((0.0, 0.18, 0.36, 0.54, 0.72, 0.9) if not C['loop'] else (0.0, 1 / 6, 2 / 6, 3 / 6, 4 / 6, 5 / 6))]
    sheets = []
    for view in ('side', 'top'):
        if view == 'side':
            lhat = (WM.to_3x3() @ st.R @ V((0, 1, 0))).normalized()
            d = -side * lhat
            cam.location = cpt - d * 10
            cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
        else:
            fh = (WM.to_3x3() @ st.R @ V((1, 0, 0))).normalized()
            lh = (WM.to_3x3() @ st.R @ V((0, 1, 0))).normalized()
            cpt_t = WM @ st.pt((fc, st.cl * 0.5, 108))
            cam.location = cpt_t + V((0, 0, 10))
            cam.rotation_euler = cols_to_mat([fh, lh, V((0, 0, 1))]).to_euler()
        cam.data.ortho_scale = width if view == 'side' else max(width, (abs(st.cl) + 140) / 75.0)
        files = []
        for f in fr:
            rig.apply(C['bases'][f])
            for o in props.values():
                o.hide_render = True
            pr = C['prop']
            if pr:
                o = props[pr['name']]
                vis = pr.get('visible')
                o.hide_render = bool(vis and not (vis[0] <= f <= vis[1]))
                o.matrix_world = WM @ (C['Ps'][f][M_ + 'RightHand'] @ pr['socket'])
            fn = str(outdir / f"{C['name']}_{view}_{f:03d}.png")
            sc.render.filepath = fn
            bpy.ops.render.render(write_still=True)
            files.append(fn + '::' + 'f=%d' % (f + 1))
        sheet = OUT / 'Preview' / f"A_Gun_{C['name']}_{view}_sheet.png"
        backup(sheet)
        compose_sheet(files, sheet, 3)
        sheets.append(sheet)
    return sheets


def fit_station(rig, nm, steps=((-60, 61, 15), (-30, 31, 10), (-40, 41, 20))):
    """FIT=Navn: soeger station (cf, cl, phi) omkring CLIPS[nm] med mindst haandfejl (kun afproevning)."""
    builder, (cf0, cl0, ph0) = CLIPS[nm]
    if os.environ.get('FITSTEPS'):
        steps = [tuple(int(x) for x in s.split(',')) for s in os.environ['FITSTEPS'].split(';')]
    best = None
    for dcf in range(*steps[0]):
        for dcl in range(*steps[1]):
            for dph in range(*steps[2]):
                st = Station(cf0 + dcf, cl0 + dcl, ph0 + dph)
                C = builder(rig, st)
                cost = 0.0
                n = C['N']
                for f in range(0, n, 5):
                    _, e = solve(rig, C['spec'](f))
                    cost += sum(v * v for v in e.values())
                if best is None or cost < best[0]:
                    best = (cost, cf0 + dcf, cl0 + dcl, ph0 + dph)
    print('FIT', nm, best)


# ---------------------------------------------------------------- hovedprogram
def main():
    if os.environ.get('FIT'):
        rig = Rig()
        for nm in os.environ['FIT'].split(','):
            fit_station(rig, nm)
        sys.exit(0)
    names = [n for n in CLIPS if not ONLY or n in ONLY]
    rig = Rig()
    results = {}
    for nm in names:
        C = evaluate(rig, nm)
        results[nm] = C
        if DEBUG:
            print('ERRS', nm, [(i, e.get('LeftHand', 0), e.get('RightHand', 0)) for i, e in enumerate(C['errs']) if e.get('LeftHand', 0) > 1 or e.get('RightHand', 0) > 1][:40])
        print('CLIP', nm, 'N', len(C['Ps']), 'maxerr', max_err(C), 'root', root_motion(C))
    if os.environ.get('DRY'):
        pass
    else:
        for nm, C in results.items():
            rig.apply(rig.basis_from_P({n: rig.restL[n] for n in rig.names}))
            C['clip'] = bake(rig, 'A_Gun_' + nm, C['bases'], OUT / f'A_Gun_{nm}.fbx')
    for nm, C in results.items():
        zs = []
        for f in range(0, len(C['Ps']), 3):
            rig.apply(C['bases'][f])
            zs.append(rig.mesh_minz() * 100.0 + 100.0)
        C['ground_cm'] = [round(min(zs), 2), round(max(zs), 2)]
    if not os.environ.get('SKIP_RENDER'):
        cam, gun, props = setup_preview(rig)
        fr = CACHE / 'frames'
        fr.mkdir(exist_ok=True)
        for nm, C in results.items():
            C['sheets'] = [str(s.name) for s in render_clip(rig, cam, gun, props, C, fr)]
            print('SHEET', C['sheets'])
        render_grips(rig, cam, gun, props, results, fr)
    if not os.environ.get('DRY'):
        write_outputs(rig, results)
    return rig, results


# ---------------------------------------------------------------- graebsdetaljer (naerbilleder)
def render_grips(rig, cam, gun, props, results, outdir):
    picks = [('Sponge', 30), ('Ram', 22), ('Load', 34)]
    files = []
    sc = rig.sc
    for nm, f in picks:
        if nm not in results:
            continue
        C = results[nm]
        st = C['st']
        place_gun(gun, st)
        rig.apply(C['bases'][f])
        pr = C['prop']
        for o in props.values():
            o.hide_render = True
        o = props[pr['name']]
        o.hide_render = False
        o.matrix_world = WM @ (C['Ps'][f][M_ + 'RightHand'] @ pr['socket'])
        ctr = WM @ C['Ps'][f][M_ + 'RightHand'].translation
        lhat = (WM.to_3x3() @ st.R @ V((0, 1, 0))).normalized()
        fh = (WM.to_3x3() @ st.R @ V((1, 0, 0))).normalized()
        for view in ('side', 'top'):
            if view == 'side':
                d = -C['cam'][2] * lhat
                cam.location = ctr - d * 10
                cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
            else:
                cam.location = ctr + V((0, 0, 10))
                cam.rotation_euler = cols_to_mat([fh, lhat, V((0, 0, 1))]).to_euler()
            cam.data.ortho_scale = 0.8
            fn = str(outdir / f'grip_{nm}_{view}.png')
            sc.render.filepath = fn
            bpy.ops.render.render(write_still=True)
            files.append(fn + '::' + f'{nm} f={f + 1} {view}')
    if files:
        sheet = OUT / 'Preview' / 'A_Gun_grips_closeup.png'
        backup(sheet)
        compose_sheet(files, sheet, 2)


# ---------------------------------------------------------------- rapport og notes
def rnd(x, n=1):
    return round(float(x), n)


def tip_summary(C):
    pr = C['prop']
    if not pr or pr['name'] == 'SM_Cannonball_12pdr':
        return None
    ser = C['prop_checks']['prop_point_F_per_frame']
    i = min(range(len(ser)), key=lambda k: ser[k][0])
    return {'deepest_tip_F_cm': ser[i], 'deepest_frame_1based': i + 1, 'muzzle_face_f_cm': rnd(MF), 'bore_axis_u_cm': rnd(MU),
            'tip_inside_muzzle_cm': rnd(MF - ser[i][0]), 'tip_offset_from_bore_axis_l_u_cm': [ser[i][1], rnd(ser[i][2] - MU)],
            'tip_at_start_F_cm': ser[0]}


def ev1(events):
    out = {}
    for k, v in events.items():
        if (k.endswith('_frame') or k.endswith('_frames') or k.endswith('_FIRE')) and 'swing' not in k and 'step' not in k:
            out[k] = [x + 1 for x in v] if isinstance(v, list) else v + 1
        else:
            out[k] = v
    return out


def write_outputs(rig, results):
    rep = {'fps': 30, 'gun_scale': GUN_SCALE, 'soldier_height_cm': 199, 'gun_game_scale_cm': {k: (rnd(v) if isinstance(v, float) else v) for k, v in GUN.items()}, 'clips': {}}
    for nm, C in results.items():
        C['prop_checks'] = prop_checks(rig, C)
        n = len(C['Ps'])
        e = {'file': f'A_Gun_{nm}.fbx', 'frames': n, 'seconds': rnd(n / 30.0 if C['loop'] else (n - 1) / 30.0, 2), 'loop': C['loop'],
             'first_equals_last': not C['loop'], 'station': C['st'].info(), 'max_ik_miss_cm': max_err(C), 'root_motion': root_motion(C),
             'foot_slide': foot_slide(C), 'mesh_lowest_point_vs_ground_cm_min_max': C['ground_cm'], 'events_1based': ev1(C['events'])}
        if nm == 'Push_Loop':
            e['stance_speed'] = stance_speed(C)
        if C['prop']:
            pr = C['prop']
            e['prop'] = {'asset': pr['name'], 'bone': 'RightHand', 'socket_unreal': ue_socket(pr['socket']), 'socket_blender_bone_local': [list(map(lambda x: round(x, 4), r)) for r in pr['socket']]}
            if 'zR' in pr:
                e['prop']['right_hand_on_shaft_z_cm'] = pr['zR']
                e['prop']['left_hand_on_shaft_z_cm'] = pr['zL']
            if 'visible' in pr:
                e['prop']['visible_frames_1based'] = [pr['visible'][0] + 1, pr['visible'][1] + 1]
            ts = tip_summary(C)
            if ts:
                e['prop']['check'] = ts
            if pr['name'] == 'SM_Cannonball_12pdr':
                rf = C['events']['ball_released_frame']
                e['prop']['ball_center_F_at_release_cm'] = C['prop_checks']['prop_point_F_per_frame'][rf]
        rep['clips'][nm] = e
    (OUT / 'report.json').write_text(json.dumps(rep, indent=1), encoding='utf-8')
    (OUT / 'notes.md').write_text(make_notes(rep), encoding='utf-8')


def make_notes(rep):
    G = rep['gun_game_scale_cm']
    c = rep['clips']
    L = []
    A = L.append
    A('# Kanonbesætning v1 (6-pund feltkanon) - bygget af Tools/Animation/build_gun_crew_clips_1864.py')
    A('')
    A('Blender 5.2 headless. Ikke importeret i Unreal (hand over til unreal-integration). 30 fps, på stedet, soldaternes skelet (41 Mixamo-knogler som DK_Infantry_1864 / SK_Human_1864, 199 cm hoej). '
      'Eksport som fanebaererens/officerens klip: armatur skala 0,01, akser -Z/Y, armatur + mesh i hver FBX (importer animation-only med roll 90). '
      'Klippene er ren procedural pose (IK) uden kilde-klip; historisk korrekthed (hvem står hvor, håndgreb, rytme) er et SKØN.')
    A('')
    A('**Hofte-hvilehøjde (rettet):** Hips i klippene er løftet %.3f cm i rigrummets op-akse, så Hips Lcl Translation svarer til A_Officer_Idle / A_Combat_*-klippene (ca. 93,5 cm). Uden dette sank figurerne ca. 98 cm. **Importen skal derfor køres med GUN_CREW_FIX_HIPS=0** (ingen ekstra tillæg). De gamle 2,2-klip i Crew/ har IKKE rettelsen.' % HIPS_REST_FIX_CM)
    A('')
    A('## Kanonens mål i spillets skala (Cannon.fbx x %.1f; højder over jorden = hjulenes underkant)' % rep['gun_scale'])
    A('- Kanon-ramme F: f = mod mundingen (Unreal-meshens +Y), l = kanonens venstre set bagfra (meshens +X), u = højde over jorden. Meshens origo ligger %.1f cm over jorden (hjulunderkant), så meshens Z = u - %.1f.' % (G['mesh_origin_height_above_ground'], G['mesh_origin_height_above_ground']))
    A('- **Mundingshøjde: %.1f cm** (boringens centrum, mundingsfladen ved f = %.1f cm fra meshens origo; mundingens ydre radius ca. %.1f cm).' % (G['muzzle_u'], G['muzzle_f'], G['muzzle_outer_radius']))
    A('- **Lavettens ende**: f = %.1f cm, overkant (håndtag) u = %.1f cm, halv bredde %.1f cm; sporet slutter på jorden.' % (G['trail_end_f'], G['trail_end_top_u'], G['trail_half_width']))
    A('- Hjulaksel: f = %.1f cm, u = %.1f cm, hjulradius %.1f cm, hjulets yderside l = +-%.1f cm. Kanonen er i alt %.0f x %.0f x %.0f cm (uskaleret mesh %s).' % (
        G['axle_f'], G['axle_u'], G['wheel_radius'], G['wheel_outer_l'], G['mesh_size_cm_unscaled'][1] * G['scale'], G['mesh_size_cm_unscaled'][0] * G['scale'], G['mesh_size_cm_unscaled'][2] * G['scale'], G['mesh_size_cm_unscaled']))
    A('- Fænghul/sigteknop (højeste punkt på øverste løb): f = %.1f cm, u = %.1f cm. Bagende af løbet (knappen) ca. f = %.1f cm.' % (G['vent_f'], G['vent_u'], G['cascabel_f']))
    if rep['gun_scale'] >= 2.0:
        A('- **Skalaproblem**: soldaten er 199 cm; mundingen sidder %.0f cm oppe (skulderhøjde 144 cm), fænghullet %.0f cm (over hovedet). Hjulene er %.0f cm i diameter. Kanonen er ca. 1,3 gange for stor i forhold til mandskabet. Se også Crew_scale1.6 for en sæt til skala 1,6 (GUN_SCALE i scriptet genberegner hele sættet).' % (G['muzzle_u'], G['vent_u'], 2 * G['wheel_radius']))
    else:
        A('- Skala %.1f (projektlederens valg): soldaten er 199 cm; mundingen sidder %.0f cm oppe (skulderhøjde 144 cm), fænghullet %.0f cm, hjuldiameter %.0f cm. Hænderne er ved sponge/ram ca. 128 cm over jorden (brysthøjde), og fænghullet kan nås med strakt arm. Stationer og baner er genfittet til denne skala.' % (rep['gun_scale'], G['muzzle_u'], G['vent_u'], 2 * G['wheel_radius']))
    A('')
    A('## Klip, stationer og frames (frames er 1-baserede som i FBX)')
    A('Station = hvor figurens jordpunkt (under hofterne) står i F (f, l i cm fra meshens origo) og hvilken vej den ser (grader fra mundingsretningen mod venstre). Unreal-værdi: i kanonmeshens lokale ramme (X = l, Y = f, Z = højde over meshens origo; blikretning som (x, y)). Stationerne er forslag, valgt så IK rammer; de er ikke koordineret mod andre figurer eller hjul-kollision i spillet.')
    A('')
    A('| Klip | Frames | Sek | Loop | Station F (f, l) / retning | Unreal (X, Y, Z) / blik (x, y) | Maks. IK-miss (cm) |')
    A('|---|---|---|---|---|---|---|')
    for nm, e in c.items():
        s = e['station']
        miss = max(e['max_ik_miss_cm'].values())
        A('| A_Gun_%s | %d | %.2f | %s | (%.0f, %.0f) / %.0f grader | (%.0f, %.0f, %.0f) / (%.3f, %.3f) | %.1f |' % (
            nm, e['frames'], e['seconds'], 'ja' if e['loop'] else 'nej (første = sidste frame)', s['F_ground_point_cm'][0], s['F_ground_point_cm'][1], s['heading_deg_from_muzzle_dir_toward_left'],
            s['unreal_gun_mesh_local_cm'][0], s['unreal_gun_mesh_local_cm'][1], s['unreal_gun_mesh_local_cm'][2], s['unreal_gun_mesh_forward_xy'][0], s['unreal_gun_mesh_forward_xy'][1], miss))
    A('')
    A('Roller: Sponge, Ram = nr. 1 til **højre** for mundingen (står foran mundingen, l < 0); Load = nr. 2 til **venstre** (l > 0); Prime = ved fænghullet på højre side; Aim = bag lavettens ende; Push_Loop = ved højre hjul; StandBy = valgfrit sted.')
    A('')
    A('## Hastighed, længde og rodbevægelse')
    A('Ingen klip bevæger sig frem (alle på stedet). Hofteknoglen (Hips) er roden; netto rodforskydning første til sidste frame er 0 for alle klip. Udsving (hofteknoglens max-min x / z / y, cm):')
    for nm, e in c.items():
        rm = e['root_motion']
        A('- A_Gun_%s: %d frames = %.2f s; udsving x/z/y = %s cm; netto = %s cm.' % (nm, e['frames'], e['seconds'], rm['excursion_x_z_y_cm'], rm['net_first_to_last_x_z_y_cm']))
    pl = c.get('Push_Loop')
    if pl:
        A('- **A_Gun_Push_Loop gangfart**: standfodens hastighed relativt til kroppen (median over frames med foden på jorden): venstre %s cm/s, højre %s cm/s (skridtlængde %.0f cm, 2 skridt pr. 1,2 s). Afspil med kanonens/mandens faktiske hastighed / dette tal for at undgå fødder der glider. Fødderne er flade (ingen hælløft), swing-højde 7 cm.' % (
            pl['stance_speed']['Left']['speed_cm_s_median'], pl['stance_speed']['Right']['speed_cm_s_median'], pl['events_1based']['step_length_cm']))
    A('')
    A('## Rekvisit-sockets (Unreal-konvention som Officer/Final_v5/notes.md: knogle RightHand, Location cm, Rotation P/Y/R grader, Scale 1)')
    A('Skala er 1,0 (rekvisitterne er i cm). Rekvisittens relative transform under socket = identitet. Blender-matrixen er i report.json (socket_blender_bone_local). Rekvisitternes origo: SM_Sponge / SM_Rammer ved græbspunktet 66,7 cm fra butten (hovedets spids z = +133,3 cm); kuglen ved sit centrum. Venstre hånd er IK på skaftet (ingen socket; den er aldrig fastgjort).')
    for nm, e in c.items():
        if 'prop' not in e:
            continue
        p = e['prop']
        s = p['socket_unreal']
        A('- **A_Gun_%s - %s**: Location (%.2f, %.2f, %.2f); Rotation P/Y/R (%.2f, %.2f, %.2f); Scale (1, 1, 1).' % (nm, p['asset'], *s['location_cm'], *s['rotation_PYR_deg']))
        if 'right_hand_on_shaft_z_cm' in p:
            A('  - Højre hånd griber skaftet ved prop-z = %.0f cm (overhåndsgreb, tommel mod hovedet), venstre hånd glider på skaftet ved z ca. %.0f cm (IK, ca. 35 cm foran højre).' % (p['right_hand_on_shaft_z_cm'], p['left_hand_on_shaft_z_cm']))
        if 'check' in p:
            k = p['check']
            A('  - Kontrol (målt på den eksporterede pose): skaftets spids er dybest ved frame %d: F = %s cm, dvs. %.0f cm inde i mundingen (mundingsflade f = %.0f), afstand fra boringens akse (l, u) = %s cm. Start/slut: spids ved F = %s (uden for mundingen).' % (
                k['deepest_frame_1based'], k['deepest_tip_F_cm'], k['tip_inside_muzzle_cm'], k['muzzle_face_f_cm'], k['tip_offset_from_bore_axis_l_u_cm'], k['tip_at_start_F_cm']))
        if 'visible_frames_1based' in p:
            A('  - Kuglen er synlig i hånden frame %d-%d. **Frame hvor den slippes/forsvinder ved mundingen: frame %d** (kuglens centrum ved slip: F = %s cm; mundingsflade f = %.0f, akse u = %.0f; dvs. ca. 6 cm inde i mundingen).' % (
                p['visible_frames_1based'][0], p['visible_frames_1based'][1], e['events_1based']['ball_released_frame'], p['ball_center_F_at_release_cm'], MF, MU))
    A('')
    A('## Hændelser (frames, 1-baserede)')
    for nm, e in c.items():
        ev = {k: v for k, v in e['events_1based'].items() if 'direction' not in k}
        if ev:
            A('- A_Gun_%s: %s' % (nm, json.dumps(ev, ensure_ascii=False)))
    A('')
    A('## Kontroltal (målt på de eksporterede poser)')
    for nm, e in c.items():
        fs = e['foot_slide']
        A('- A_Gun_%s: laveste mesh-punkt ift. jord (cm) min/max = %s; fødder på jorden: venstre %d/%d frames, højre %d/%d frames; ankelens største vandrette flytning pr. frame mens foden er på jorden: venstre %s cm, højre %s cm; maks. IK-miss %s cm.' % (
            nm, e['mesh_lowest_point_vs_ground_cm_min_max'], fs['Left']['on_ground_frames'], fs['Left']['of'], fs['Right']['on_ground_frames'], fs['Right']['of'], fs['Left']['max_horizontal_move_per_frame_cm'], fs['Right']['max_horizontal_move_per_frame_cm'], e['max_ik_miss_cm']))
    A('')
    A('## Preview')
    A('Preview/A_Gun_<klip>_side_sheet.png = 6 frames set fra siden (kamera på mandens side, orthografisk, kanon + jordlinje; mundingen mod højre når kameraet er på højre side). Preview/A_Gun_<klip>_top_sheet.png = samme 6 frames set oppefra (mundingen mod højre). Preview/A_Gun_grips_closeup.png = nærbilleder af håndgreb (Sponge f=31, Ram f=23, Load f=35). Billederne er Workbench-renders; rekvisitterne er placeret som i spillet (RightHand + socket).')
    A('')
    A('## Svagheder (ærlig liste)')
    if rep['gun_scale'] >= 2.0:
        A('- Kanon i skala 2,2 er for stor til 199 cm-soldaten (se ovenfor): hænderne er i hoved- til skulderhøjde ved sponge/ram/load, og **A_Gun_Prime når ikke helt fænghullet** (hånden er ca. %.0f cm kort selv med strakt arm, tåhævning og fremadlæning).' % c['Prime']['max_ik_miss_cm']['RightHand'])
    else:
        A('- A_Gun_Prime når fænghullet (IK-miss %.1f cm) men kun med strakt arm, tåhævning og fremadlæning; hånden er højt over skulderen. Sponge/Ram/Aim/Push er fittet til skala %.1f og kan ikke genbruges ved en anden skala uden genkørsel.' % (c['Prime']['max_ik_miss_cm']['RightHand'], rep['gun_scale']))
    A('- Sponge/Ram: skaftet føres svagt skråt (stigning %.0f grader før, ca. 60 %% heraf ved indføring). Hovedet er kun 25-35 cm inde i mundingen (200 cm lang stok mod %.0f cm løb i spillets skala); svabring til bunden er ikke vist.' % (D0, (G['muzzle_f'] - G['cascabel_f'])))
    A('- Hænderne er votte (kun Index1-3 og Thumb1-3 har vægte): fingre kan kun krummes som en blok, tommelfingeren er ikke bevaeget. Greb er derfor omtrentlige; skaftet går gennem håndfladen men kan skaere gennem fingerblokken. Se A_Gun_grips_closeup.png.')
    A('- Håndleds-vrid (sponge drejer ca. 110 grader) er delt 50/50 mellem underarm og hånd uden twist-knogler: mulig "slikpapir"-deformation i underarmen.')
    A('- Ladning/kugle gives ikke videre fra en anden figur: A_Gun_Load viser en modtagegestus i luften (frame 12-14) uden en partner. Tændrør og aftrækkersnor er ingen rekvisitter; Prime viser dem kun som håndbevægelse. Rekyl-reaktionen (frame 44-54) er en lille læning/hovedvending, ikke et trin tilbage.')
    A('- Aim: hovedet kigger op langs løbet, men øjenhøjden (ca. 150 cm i bøjning) er under løbets overkant (%.0f cm); kommandoeren ser altså i retning af løbet, ikke over det. Tegnet med hånden er en simpel haeven arm.' % G['breech_top_u'])
    A('- Push_Loop: fødder flade, ingen hælløft; hænder fast på hjulets bagkvadrant (hjulet drejer ikke i klippet - kanonen/hjulet skal føres af spillet; hænderne vil ikke følge eger). Hastigheden er fast (se ovenfor), ikke aflæst fra kanonen.')
    A('- Stationer, kollision med kanonens hjul/lavet og med andre mand er ikke testet i spillet. Alle sockets og stationer er ikke kontrolleret i Unreal (ingen import foretaget). Unreal-konverteringen af socket er den samme formel som officerens, ikke prøvet på disse rekvisitter.')
    A('- Referencen Kanon_Animationer_Begge_Rettet_Samlet.zip (R:\\Onedrive\\AI) er kun brugt som skitse (pakket ud i Saved/Codex/gun_crew/ref/zip_20261105: kun beskrivelse, kontrol og preview-billedet, intet kørt): sekvens fremrulning - svabring - ladning/indstoedning - retning - skud, fem mand på en række. Videoerne og kildekoden er ikke åbnet.')
    A('- Første preview-kørsel blev ved en fejl startet mens GPU\'en var optaget (94 %); resten blev kørt efter at GPU\'en var under 30 %.')
    return '\n'.join(L) + '\n'


if __name__ == '__main__':
    main()
