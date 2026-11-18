"""Kompagniofficer (kaptajn) v5, 2026-11: meshes + klip paa soldaternes skelet SK_Human_1864.
    blender -b -P build_officer_clips_1864.py -- [<out dir>]        (default SourceAssets/Units1864/Officer/Final_v5)
Input (aendres ikke): Final_v4/{NAT}_Officer_1864_v4/*.fbx, Anim_v4/{NAT}_{March_Skulder,Ordre,Hvil}_Final_v4.blend, Final_v4/Props_v4/*.fbx,
  soldaternes klip SourceAssets/InfantryCombat/A_Combat_{WalkShoulder,DeathFront,SabreDraw,SabreSheath}.fbx
  og den raa Mixamo Walking.fbx (kun til den frit svingende hoejre arm i WalkSheathed; env WALKING_FBX).
Metode: officerens knogler er identiske med DK_Infantry_1864 (navne, foraeldre, bindepose, roll). Pose overfoeres fra soldatens klip ved at kopiere hver
  knogles lokale rotation (relativt til foraelder), Hips tager soldatens absolutte position; arme/haender kommer fra Codex' holdte poser (matrix_basis).
  Armaturet eksporteres med skala 0.01 og uden forskydning som soldaternes (v4 havde 0.009 + 0.902 m), saa mesh og klip har samme enheder som soldaten.
Klip (30 fps): A_Officer_Walk (30 frames, ben/hofte/ryg/hoved = soldatens WalkShoulder, saebel paa skulderen), A_Officer_WalkSheathed (samme ben, hoejre arm
  fri, venstre haand paa hjaltet ved venstre hofte), A_Officer_Idle (90), A_Officer_Order (60), A_Officer_Death (soldatens DeathFront, indblandet fra Hvil).
Output: SK_Officer_1864_{DK,SE,PR,NO}.fbx, A_Officer_*.fbx, SM_Officer_{Saber,Scabbard}.fbx, report.json, Preview/ (billeder).
v5.1 (2026-11-05): Idle/Order staar nu paa jorden som Walk (jordniveau = toeknogle i stance, ikke laveste mesh-punkt); nye klip A_Officer_SabreDraw /
  A_Officer_SabreSheath (37 frames) med IK til officerens eget faeste (haanden = skedens transform @ inverse(saebelsocket), saa der er 0 spring ved graebsframen).
Env: ONLY=Idle,Order,Death,SabreDraw,SabreSheath (kun disse klip skrives; uden ONLY skrives alt, ogsaa meshes), SKIP_RENDER=1 (ingen forhaandsbilleder), HILT_X (m i prop-rum, default -0.03), POLE=x,y,z (venstre albue), LHAND_OFF=x,y,z (cm, i Hips-rum).
"""
import bpy, json, math, os, sys, shutil, subprocess
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion

args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
OFF = PROJECT / 'SourceAssets/Units1864/Officer'
V4 = OFF / 'Final_v4'
ANIM = OFF / 'Anim_v4'
COMBAT = PROJECT / 'SourceAssets/InfantryCombat'
OUT = Path(args[0]) if args else OFF / 'Final_v5'
CACHE = PROJECT / 'Saved/Codex/officer_v5'
WALKING = Path(os.environ.get('WALKING_FBX', r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer\Walking.fbx'))
NATIONS = ['DK', 'SE', 'PR', 'NO']
HILT_X = float(os.environ.get('HILT_X', '-0.03'))
POLE = Vector([float(x) for x in os.environ.get('POLE', '0.8,-0.25,-0.55').split(',')])
LHAND_OFF = Vector([float(x) for x in os.environ.get('LHAND_OFF', '0,0,0').split(',')])
SOL_SPEED_CM_S = 137.70097  # konstant i StrategyInfantryVisualComponent (InfantryShoulderWalkClipCmPerSecond)
M_ = 'mixamorig:'
ONLY = [x for x in os.environ.get('ONLY', '').split(',') if x]


def want(name):
    return not ONLY or name in ONLY

for d in (OUT, OUT / 'Preview', CACHE):
    d.mkdir(parents=True, exist_ok=True)


def short(n):
    return n.split(':')[-1]


def exp_settings(path, anim):
    kw = dict(filepath=str(path), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, use_mesh_modifiers=False,
              bake_anim=anim, axis_forward='-Z', axis_up='Y')
    if anim:
        kw.update(bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False, bake_anim_simplify_factor=0, bake_anim_force_startend_keying=True)
    bpy.ops.export_scene.fbx(**kw)


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def first_arm():
    return next(o for o in bpy.data.objects if o.type == 'ARMATURE')


def set_action_slot(arm):
    act = arm.animation_data.action
    try:
        arm.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    return act


def depth(pb):
    n = 0
    while pb.parent:
        pb = pb.parent
        n += 1
    return n


# ---------------------------------------------------------------- kilde-klip (soldat) -> armaturrum-matricer pr. frame
def load_clip_matrices(fbx, frames=None):
    """Importerer et soldaterklip og returnerer ({frame_index: {bone: Matrix i armaturrum}}, parent-dict, [world-mesh-minz], arm-info)."""
    clear()
    bpy.ops.import_scene.fbx(filepath=str(fbx))
    arm = first_arm()
    act = set_action_slot(arm)
    sc = bpy.context.scene
    f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
    parents = {pb.name: (pb.parent.name if pb.parent else None) for pb in arm.pose.bones}
    out = []
    mesh = next((o for o in bpy.data.objects if o.type == 'MESH'), None)
    minz = []
    for f in range(f0, f1 + 1):
        sc.frame_set(f)
        bpy.context.view_layer.update()
        out.append({pb.name: pb.matrix.copy() for pb in arm.pose.bones})
        if mesh is not None and frames == 'mesh':
            ev = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
            me = ev.to_mesh()
            minz.append(min((ev.matrix_world @ v.co).z for v in me.vertices))
            ev.to_mesh_clear()
    return out, parents, minz


# ---------------------------------------------------------------- officer-rig
class Rig:
    def __init__(self, nation='DK'):
        clear()
        bpy.ops.import_scene.fbx(filepath=str(V4 / f'{nation}_Officer_1864_v4' / f'{nation}_Officer_1864_v4.fbx'))
        self.arm = first_arm()
        self.mesh = next(o for o in bpy.data.objects if o.type == 'MESH')
        self.arm.scale = (0.01, 0.01, 0.01)
        self.arm.location = (0, 0, 0)
        bpy.context.view_layer.update()
        self.sc = bpy.context.scene
        self.sc.render.fps = 30
        for pb in self.arm.pose.bones:
            pb.rotation_mode = 'QUATERNION'
        self.order = sorted(self.arm.pose.bones, key=depth)
        self.names = [pb.name for pb in self.order]
        self.par = {pb.name: (pb.parent.name if pb.parent else None) for pb in self.order}
        self.restL = {b.name: b.matrix_local.copy() for b in self.arm.data.bones}
        self.rrel = {}
        for b in self.arm.data.bones:
            self.rrel[b.name] = (b.parent.matrix_local.inverted() @ b.matrix_local) if b.parent else b.matrix_local.copy()
        self.blen = {b.name: b.length for b in self.arm.data.bones}
        self.kids = {n: [] for n in self.names}
        for n, p in self.par.items():
            if p:
                self.kids[p].append(n)

    # P: dict bone -> armaturrum-matrix  <->  basis: dict bone -> matrix_basis
    def basis_from_P(self, P):
        out = {}
        for n in self.names:
            p = self.par[n]
            loc = (P[p].inverted() @ P[n]) if p else P[n]
            out[n] = self.rrel[n].inverted() @ loc
        return out

    def P_from_basis(self, basis):
        P = {}
        for n in self.names:
            p = self.par[n]
            P[n] = (P[p] if p else Matrix.Identity(4)) @ self.rrel[n] @ basis[n]
        return P

    def rest_basis(self):
        return {n: Matrix.Identity(4) for n in self.names}

    def head_tail(self, P, n):
        m = P[n]
        return m.translation.copy(), (m @ Vector((0, self.blen[n], 0))).copy()

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

    def minz_of(self, basis):
        self.apply(basis)
        return self.mesh_minz()

    def build_P(self, rules, hips_dy=0.0):
        """rules: bone -> ('sol', srcP) kopierer lokal rotation fra kilde (Hips: absolut) | ('basis', basis_dict) | ellers hvile."""
        res = {}
        for n in self.names:
            p = self.par[n]
            r = rules.get(n)
            if r and r[0] == 'sol' and n in r[1]:
                src = r[1]
                if p is None:
                    res[n] = Matrix.Translation((0, hips_dy, 0)) @ src[n].copy()
                elif p in src:
                    ls = (src[p].inverted() @ src[n]).to_3x3()
                    T = (res[p] @ self.rrel[n]).translation
                    res[n] = Matrix.Translation(T) @ (res[p].to_3x3() @ ls).to_4x4()
                else:
                    res[n] = res[p] @ self.rrel[n]
            elif r and r[0] == 'basis' and n in r[1]:
                res[n] = (res[p] if p else Matrix.Identity(4)) @ self.rrel[n] @ r[1][n]
            else:
                res[n] = (res[p] if p else Matrix.Identity(4)) @ self.rrel[n]
        return res


def rebuild_subtree(rig, P, root, newP_root):
    """Saetter P[root]=newP_root og flytter alle efterkommere med uaendret lokal pose."""
    old = {n: P[n].copy() for n in P}
    P[root] = newP_root
    stack = list(rig.kids[root])
    while stack:
        n = stack.pop()
        p = rig.par[n]
        P[n] = P[p] @ (old[p].inverted() @ old[n])
        stack.extend(rig.kids[n])


def aim(rig, P, bone, new_dir):
    h, t = rig.head_tail(P, bone)
    cur = (t - h).normalized()
    rot = cur.rotation_difference(new_dir.normalized()).to_matrix()
    M = P[bone]
    new = Matrix.Translation(M.translation) @ (rot @ M.to_3x3()).to_4x4()
    rebuild_subtree(rig, P, bone, new)


def two_bone_ik(rig, P, upper, lower, hand, target, pole):
    S = P[upper].translation.copy()
    l1, l2 = rig.blen[upper], rig.blen[lower]
    d_vec = target - S
    d = min(d_vec.length, (l1 + l2) * 0.999)
    u = d_vec.normalized()
    p = (pole - pole.dot(u) * u).normalized()
    a = (l1 * l1 - l2 * l2 + d * d) / (2 * d)
    h = math.sqrt(max(l1 * l1 - a * a, 0.0))
    E = S + u * a + p * h
    aim(rig, P, upper, E - S)
    aim(rig, P, lower, target - P[lower].translation)
    return E


def quat_blend(A, B, w):
    """Blander to matrix_basis (loc lineaert, rotation slerp)."""
    la, qa, sa = A.decompose()
    lb, qb, sb = B.decompose()
    return Matrix.LocRotScale(la.lerp(lb, w), qa.slerp(qb, w), sa.lerp(sb, w))


def smooth(x):
    x = max(0.0, min(1.0, x))
    return x * x * (3 - 2 * x)


# ---------------------------------------------------------------- hold-poser (Codex) som matrix_basis
def dump_poses():
    """Trækker matrix_basis for frame 1 ud af Anim_v4-blendfilerne (egen Blender-proces) -> CACHE/pose_{NAT}_{pose}.json."""
    code = ("import bpy,json,sys\n"
            "out=sys.argv[sys.argv.index('--')+1]\n"
            "a=bpy.data.objects['Armature'];sc=bpy.context.scene\n"
            "sc.frame_set(1);bpy.context.view_layer.update()\n"
            "json.dump({pb.name:[list(map(float,r)) for r in pb.matrix_basis] for pb in a.pose.bones},open(out,'w'))\n")
    script = CACHE / '_dump_pose.py'
    script.write_text(code)
    res = {}
    for nat in NATIONS:
        for pose in ('March_Skulder', 'Ordre', 'Hvil'):
            out = CACHE / f'pose_{nat}_{pose}.json'
            if not out.exists():
                subprocess.run([bpy.app.binary_path, '-b', str(ANIM / f'{nat}_{pose}_Final_v4.blend'), '--python', str(script), '--', str(out)],
                               check=True, capture_output=True)
            res[(nat, pose)] = {k: Matrix(v) for k, v in json.loads(out.read_text()).items()}
    return res


def socket_matrices(nat='DK'):
    d = json.loads((V4 / f'{nat}_Officer_1864_v4' / 'verification_v4.json').read_text())
    return Matrix(d['saber_socket']['matrix_blender_bone_local']), Matrix(d['scabbard_socket']['matrix_blender_bone_local'])


LEFT = ['Left' + b for b in ('Shoulder', 'Arm', 'ForeArm', 'Hand')] + ['LeftHand%s%d' % (f, i) for f in ('Thumb', 'Index', 'Middle', 'Ring', 'Pinky') for i in range(1, 5)]
RIGHT = [n.replace('Left', 'Right') for n in LEFT]
ARMS = [M_ + n for n in LEFT + RIGHT]
ARMS = [n for n in ARMS]
BODY = [M_ + n for n in ('Hips', 'Spine', 'Spine1', 'Spine2', 'Neck', 'Head', 'HeadTop_End', 'LeftUpLeg', 'LeftLeg', 'LeftFoot', 'LeftToeBase', 'LeftToe_End',
                          'RightUpLeg', 'RightLeg', 'RightFoot', 'RightToeBase', 'RightToe_End')]


def compose_walk(rig, solP, poses_basis, rawP=None, sheathed=False, scab_M=None, dy=0.0):
    """Én frame: krop fra soldat (solP), arme fra holdt pose; sheathed: hoejre arm + venstre skulder fra raa gang (rawP), venstre haand paa hjaltet (IK)."""
    rules = {n: ('sol', solP) for n in BODY}
    for n in ARMS:
        rules[n] = ('basis', poses_basis)
    if sheathed:
        for n in ('RightShoulder', 'RightArm', 'RightForeArm', 'RightHand', 'LeftShoulder'):
            rules[M_ + n] = ('sol', rawP)
    P = rig.build_P(rules, dy)
    if sheathed:
        hilt = (P[M_ + 'Hips'] @ scab_M @ Vector((HILT_X, 0, 0))) + LHAND_OFF
        two_bone_ik(rig, P, M_ + 'LeftArm', M_ + 'LeftForeArm', M_ + 'LeftHand', hilt, POLE)
    return rig.basis_from_P(P)


def bake(rig, name, frames_basis, path):
    arm = rig.arm
    arm.animation_data_clear()
    arm.animation_data_create()
    act = bpy.data.actions.new(name)
    arm.animation_data.action = act
    try:
        arm.animation_data.action_slot = act.slots[0] if act.slots else act.slots.new('OBJECT', arm.name)
    except Exception:
        pass
    for i, basis in enumerate(frames_basis, 1):
        for pb in arm.pose.bones:
            pb.matrix_basis = basis[pb.name]
            pb.keyframe_insert('location', frame=i)
            pb.keyframe_insert('rotation_quaternion', frame=i)
            pb.keyframe_insert('scale', frame=i)
    rig.sc.frame_start, rig.sc.frame_end = 1, len(frames_basis)
    for o in bpy.data.objects:
        o.select_set(False)
    arm.select_set(True)
    rig.mesh.select_set(True)
    bpy.context.view_layer.objects.active = arm
    exp_settings(path, True)
    return {'name': name, 'file': Path(path).name, 'frames': len(frames_basis), 'fps': 30}


def foot_stance_speed(rig, basis_frames):
    """Stance-fodens hastighed (cm/s) i kroppens ramme: frames hvor foeddens knogle er inden for 1,2 cm af sit eget minimum (flad fod paa jorden);
    median af -dz/dt (armaturrum z = fremad). Hofterne er laast i x/z, saa det er klippets rod-hastighed."""
    res = {}
    P_all = [rig.P_from_basis(b) for b in basis_frames]
    n = len(P_all)
    for side in ('Left', 'Right'):
        y = [P[M_ + side + 'Foot'].translation.y for P in P_all]
        z = [P[M_ + side + 'Foot'].translation.z for P in P_all]
        ymin = min(y)
        v = []
        for i in range(n):
            j = (i + 1) % n
            if y[i] < ymin + 1.2 and y[j] < ymin + 1.2 and j != 0:
                v.append(-(z[j] - z[i]) * 30.0)
        v.sort()
        res[side] = {'flat_frames': len(v), 'speed_cm_s_median': round(v[len(v) // 2], 2) if v else None,
                     'speed_cm_s_min_max': [round(v[0], 1), round(v[-1], 1)] if v else None}
    return res


# ---------------------------------------------------------------- forhaandsvisning
def setup_render(rig):
    sc = rig.sc
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = 420, 560
    sc.render.film_transparent = False
    sh = sc.display.shading
    sh.light = 'STUDIO'
    sh.color_type = 'TEXTURE'
    sh.show_shadows = True
    sc.world = bpy.data.worlds.new('w')
    sc.world.color = (0.82, 0.86, 0.9)
    sc.view_settings.view_transform = 'Standard'
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = 2.5
    sc.collection.objects.link(cam)
    sc.camera = cam
    return cam


def add_props(rig, saber_M, scab_M):
    props = []
    for nm, M in (('Saber_v4', saber_M), ('Scabbard_v4', scab_M)):
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=str(V4 / 'Props_v4' / f'{nm}.fbx'))
        new = [o for o in bpy.data.objects if o not in before and o.type == 'MESH']
        props.append((new[0], 'RightHand' if nm.startswith('Saber') else 'Hips', M))
        for o in bpy.data.objects:
            if o not in before and o.type != 'MESH':
                bpy.data.objects.remove(o)
    return props


def place_props(rig, props, basis, on_scabbard=False):
    """on_scabbard: saebel hos skeden (Hips-socket) i stedet for i haanden."""
    P = rig.P_from_basis(basis)
    for ob, bone, M in props:
        if ob.name.startswith('Saber') and on_scabbard:
            bone, M = M_ + 'Hips', props[1][2]
        else:
            bone = M_ + bone
        ob.parent = None
        ob.matrix_world = rig.arm.matrix_world @ P[bone] @ M


def render_frames(rig, cam, props, frames, tag, outdir, ground_z, on_scabbard=False, views=('side',), attach=None):
    sc = rig.sc
    if 'gnd' not in bpy.data.objects:
        bpy.ops.mesh.primitive_plane_add(size=4.0, location=(0, 0, ground_z))
        g = bpy.context.object
        g.name = 'gnd'
        g.color = (0.45, 0.5, 0.4, 1)
        sc.display.shading.single_color = (0.45, 0.5, 0.4)
    files = []
    for vi, view in enumerate(views):
        if view == 'side':
            cam.location, cam.rotation_euler = (6, 0, ground_z + 0.95), (math.radians(90), 0, math.radians(90))
        elif view == 'front':
            cam.location, cam.rotation_euler = (0, -6, ground_z + 0.95), (math.radians(90), 0, 0)
        elif view == 'q34h':
            cam.location, cam.rotation_euler = (4.2, -4.2, ground_z + 0.95), (math.radians(90), 0, math.radians(45))
        else:
            cam.location, cam.rotation_euler = (4.2, -4.2, ground_z + 1.5), (math.radians(75), 0, math.radians(45))
        cam.data.clip_end = 50
        for k, basis in enumerate(frames):
            rig.apply(basis)
            place_props(rig, props, basis, on_scabbard if attach is None else attach[k])
            sc.render.filepath = str(outdir / f'{tag}_{view}_{k:02d}.png')
            bpy.ops.render.render(write_still=True)
            files.append(sc.render.filepath)
    return files


def compose_sheet(files, out, cols):
    code = ("import sys\nfrom PIL import Image\nout=sys.argv[1];cols=int(sys.argv[2]);fs=sys.argv[3:]\n"
            "ims=[Image.open(f).convert('RGB') for f in fs]\nw,h=ims[0].size\nrows=(len(ims)+cols-1)//cols\n"
            "s=Image.new('RGB',(w*cols,h*rows),(255,255,255))\n"
            "[s.paste(im,((i%cols)*w,(i//cols)*h)) for i,im in enumerate(ims)]\ns.save(out)\n")
    tmp = CACHE / '_sheet.py'
    tmp.write_text(code)
    subprocess.run(['python', '-I', str(tmp), str(out), str(cols)] + files, check=True)


# ---------------------------------------------------------------- sabelklip med IK til officerens eget faeste (v5.1)
def base_P(rig, basis, lift, twist=0.0, lean=0.0):
    """Armaturrum-poser fra matrix_basis, hofte loeftet 'lift' cm; valgfri torso-drejning (twist om lodret akse, lean om fremad-akse) fra Spine."""
    P = rig.P_from_basis(basis)
    hips = M_ + 'Hips'
    P[hips] = Matrix.Translation((0, lift, 0)) @ P[hips]
    for n in rig.names[1:]:
        P[n] = P[rig.par[n]] @ rig.rrel[n] @ basis[n]
    if twist or lean:
        M = P[M_ + 'Spine']
        rot = Matrix.Rotation(math.radians(twist), 3, 'Y') @ Matrix.Rotation(math.radians(lean), 3, 'Z')
        rebuild_subtree(rig, P, M_ + 'Spine', Matrix.Translation(M.translation) @ (rot @ M.to_3x3()).to_4x4())
    return P


def rigid(M):
    return Matrix.Translation(M.translation) @ M.to_3x3().normalized().to_4x4()


def rot_lerp(A, B, w):
    qa, qb = A.to_3x3().normalized().to_quaternion(), B.to_3x3().normalized().to_quaternion()
    return qa.slerp(qb, w).to_matrix().to_4x4()


def pose_lerp(A, B, w):
    """Stiv interpolation (position lerp, rotation slerp) mellem to matricer."""
    R = rot_lerp(A, B, w)
    R.translation = A.translation.lerp(B.translation, w)
    return R


class Sabre:
    """Geometri for officerens egen saebel: haanden er altid saebel-socket-relativ, saa haanden ved graebsframen = skedens transform @ inverse(saebelsocket)."""

    DMAX = float(os.environ.get('DMAX', '80'))              # cm saebelen traekkes langs skedeaksen foer den svinges fri
    TWIST = float(os.environ.get('TWIST', '-20'))           # grader torso-drejning (negativ = vendt mod hoejre, saa hoejre skulder naermer sig skeden bag hoften)
    LEAN = float(os.environ.get('LEAN', '-8'))             # grader sidelaens (negativ = overkroppen mod venstre)
    VIA = Vector([float(x) for x in os.environ.get('VIA', '10,112,22').split(',')])   # mellempunkt for haandens vej fra hvile til faeste (cm, armaturrum)
    POLE_G = Vector([float(x) for x in os.environ.get('POLE_G', '-0.6,-0.7,-0.3').split(',')])
    POLE_L = Vector([float(x) for x in os.environ.get('POLE_L', '0.8,-0.25,-0.55').split(',')])
    CLAV = float(os.environ.get('CLAV', '0.4'))             # 0..1: hvor meget hoejre nøgleben svinges mod maalet (skulderen skydes frem over kroppen)
    TIP_DEV = float(os.environ.get('TIP_DEV', '3.0'))         # cm: max sideværts afvigelse af klingespidsen inde i skeden under udtraek
    ALPHA_MAX = math.radians(float(os.environ.get('ALPHA_MAX', '30')))
    FWD = Vector([float(x) for x in os.environ.get('FWD', '-0.5,0.0,1.0').split(',')])   # retning hjaltet drejes mod (fremad/indad), armaturrum
    THROAT_X = 0.115                                        # skedens mund i prop-rum (m)

    def __init__(self, rig, ph, po, saber_M, scab_M, lift):
        self.rig, self.ph, self.po, self.sM, self.cM, self.lift = rig, ph, po, saber_M, scab_M, lift
        R = M_ + 'RightHand'
        self.Ph, self.Po = base_P(rig, ph, lift), base_P(rig, po, lift)
        self.scabT = self.Ph[M_ + 'Hips'] @ scab_M
        self.Hg = rigid(self.scabT @ saber_M.inverted())            # haand ved graebet
        self.axis = (self.scabT.to_3x3() @ Vector((1, 0, 0))).normalized()   # saebelens x = mod spidsen
        self.T_end = rigid(self.Po[R] @ saber_M @ self.scabT.inverted())     # stiv transform: skede-position -> Order-position
        self.H0 = rigid(self.Ph[R])
        self.pole_h = self.Ph[M_ + 'RightForeArm'].translation - self.Ph[M_ + 'RightArm'].translation
        self.pole_o = self.Po[M_ + 'RightForeArm'].translation - self.Po[M_ + 'RightArm'].translation
        self.scale = scab_M.to_scale()[0]
        self.throat = self.scabT @ Vector((self.THROAT_X, 0, 0))
        self.hilt = self.scabT @ Vector((HILT_X, 0, 0))
        self.blade_out_cm = (0.856 - self.THROAT_X) * self.scale   # traekkelaengde hvor spidsen forlader munden
        # basis for arme: krop = Hvil, hoejre fingre = Order (graebet)
        self.base = dict(ph)
        for n in rig.names:
            if n.startswith(M_ + 'RightHand') and n != R:
                self.base[n] = po[n]

    def T_pull(self, d):
        """Stiv transform skede-position -> saebelposition efter udtraek d cm. Saeblen drejes om skedens mund (hilt fremad/indad), saa spidsen
        i skeden aldrig afviger mere end TIP_DEV cm fra skedeaksen (tip-afvigelse = resterende klingelaengde i skeden * sin(vinkel))."""
        T = Matrix.Translation(-self.axis * d)
        l_in = max(self.blade_out_cm - d, 0.0)
        a = self.ALPHA_MAX if l_in < 1e-6 else min(math.asin(min(1.0, self.TIP_DEV / l_in)), self.ALPHA_MAX)
        a *= smooth(d / 15.0)   # ingen drejning ved d=0 (graebs-/slip-framen skal ligge praecist paa skedens transform)
        h = -self.axis
        f = self.FWD - self.FWD.dot(h) * h
        h2 = (h + math.tan(a) * f.normalized()).normalized()
        R = h.rotation_difference(h2).to_matrix().to_4x4()
        M = self.throat
        return Matrix.Translation(M) @ R @ Matrix.Translation(-M) @ T

    def hand_target(self, phase, x):
        """phase: approach (x=0 hvile .. 1 graeb), pull (x=0 .. 1 -> DMAX), swing (x=0 .. 1 -> Order). Returnerer (haandmatrix, pole)."""
        if phase == 'approach':
            p0, p1, v = self.H0.translation, self.Hg.translation, self.VIA
            H = rot_lerp(self.H0, self.Hg, x)
            H.translation = (1 - x) ** 2 * p0 + 2 * (1 - x) * x * v + x * x * p1
            return H, self.pole_h.lerp(self.POLE_G, x)
        if phase == 'pull':
            return rigid(self.T_pull(self.DMAX * x) @ self.Hg), self.POLE_G.lerp(self.pole_o, 0.25 * x)
        Hp = rigid(self.T_pull(self.DMAX) @ self.Hg)
        return pose_lerp(Hp, rigid(self.T_end @ self.Hg), x), self.POLE_G.lerp(self.pole_o, 0.25 + 0.75 * x)

    def frame(self, hand, pole, twist_w, left_w_throat, left_w_hilt):
        rig = self.rig
        P = base_P(rig, self.base, self.lift, self.TWIST * twist_w, self.LEAN * twist_w)
        R = M_ + 'RightHand'
        cl = M_ + 'RightShoulder'
        if self.CLAV > 0:
            ch, ct = rig.head_tail(P, cl)
            cur = (ct - ch).normalized()
            want_dir = (hand.translation - ch).normalized()
            aim(rig, P, cl, cur.lerp(want_dir, self.CLAV * twist_w))
        two_bone_ik(rig, P, M_ + 'RightArm', M_ + 'RightForeArm', R, hand.translation, pole)
        got = P[R].translation.copy()
        rebuild_subtree(rig, P, R, Matrix.Translation(got) @ hand.to_3x3().to_4x4())
        err = (got - hand.translation).length
        basis = rig.basis_from_P(P)
        # venstre arm: hvile -> skedens mund -> hjalt (som WalkSheathed)
        Lb = {}
        for key, tgt, w in (('t', self.throat + Vector((5.0, 0, 0)) + LHAND_OFF, left_w_throat), ('h', self.hilt + LHAND_OFF, left_w_hilt)):
            if w <= 0:
                continue
            P2 = {k: v.copy() for k, v in P.items()}
            two_bone_ik(rig, P2, M_ + 'LeftArm', M_ + 'LeftForeArm', M_ + 'LeftHand', tgt, self.POLE_L)
            Lb[key] = rig.basis_from_P(P2)
        for n in ARMS:
            if n.startswith(M_ + 'Left') and n in basis:
                m = basis[n]
                if 't' in Lb:
                    m = quat_blend(m, Lb['t'][n], left_w_throat)
                if 'h' in Lb:
                    m = quat_blend(m, Lb['h'][n], left_w_hilt)
                basis[n] = m
        return basis, err


DRAW_GRIP = 12      # 0-baseret frame: haanden har graebet, saeblen skifter fra skede til haand
SHEATH_REL = 24     # 0-baseret frame: saeblen er helt i skeden, haanden slipper


def draw_params(i, gi=DRAW_GRIP, pull_end=25, last=36):
    """Draw: 0..gi hand naermer sig, gi..pull_end traek langs skeden, pull_end..last sving til Order."""
    if i <= gi:
        ph_, x = 'approach', smooth(i / gi)
        tw = x
    elif i <= pull_end:
        ph_, x = 'pull', smooth((i - gi) / (pull_end - gi))
        tw = 1.0
    else:
        ph_, x = 'swing', smooth((i - pull_end) / (last - pull_end))
        tw = 1.0 - x
    wl = smooth((i - 3) / 9.0) * (1.0 - smooth((i - 26) / 10.0))
    return ph_, x, tw, wl, 0.0


def sheath_params(i, rel=SHEATH_REL, sw_end=10, last=36):
    """Sheath: 0..sw_end sving fra Order mod skedens akse, sw_end..rel skub ind, rel..last haanden hjem + venstre haand paa hjalt."""
    if i <= sw_end:
        ph_, x = 'swing', 1.0 - smooth(i / sw_end)
        tw = 1.0 - x
    elif i <= rel:
        ph_, x = 'pull', 1.0 - smooth((i - sw_end) / (rel - sw_end))
        tw = 1.0
    else:
        ph_, x = 'approach', 1.0 - smooth((i - rel) / (last - rel))
        tw = x
    wt = smooth((i - 1) / 8.0) * (1.0 - smooth((i - rel) / 8.0))
    wh = smooth((i - rel) / (last - rel))
    return ph_, x, tw, wt, wh


def build_sabre_clip(sab, kind, n=37):
    out, errs, hands = [], [], []
    for i in range(n):
        ph_, x, tw, wt, wh = (draw_params if kind == 'draw' else sheath_params)(i)
        H, pole = sab.hand_target(ph_, x)
        b, e = sab.frame(H, pole, tw, wt, wh)
        out.append(b)
        errs.append(round(e, 2))
        hands.append(H)
    return out, errs, hands


def draw_hline(files, row, rgb=(230, 30, 30)):
    code = ('import sys\nfrom PIL import Image, ImageDraw\nrow=int(sys.argv[1]);fs=sys.argv[2:]\n'
            'for f in fs:\n    im=Image.open(f).convert("RGB");d=ImageDraw.Draw(im);d.line([(0,row),(im.size[0],row)],fill=%r,width=2);im.save(f)\n' % (rgb,))
    tmp = CACHE / '_hline.py'
    tmp.write_text(code)
    subprocess.run(['python', '-I', str(tmp), str(row)] + list(files), check=True)



def sabre_report(rig, sab, saber_M, scab_M, draw, draw_err, sheath_c, sheath_err, poses):
    R = M_ + 'RightHand'
    G = Vector((0.015, 0, 0))            # midten af graebet i prop-rum (m)
    res = {'blade_out_cm': round(sab.blade_out_cm, 1), 'twist_deg': sab.TWIST, 'lean_deg': sab.LEAN, 'clavicle_k': sab.CLAV, 'dmax_cm': sab.DMAX,
           'scab_axis': [round(x, 3) for x in sab.axis]}
    sh = sab.Ph[M_ + 'RightArm'].translation
    res['arm_reach_cm'] = round(rig.blen[M_ + 'RightArm'] + rig.blen[M_ + 'RightForeArm'], 1)
    res['wrist_to_shoulder_at_grip_cm_without_body_help'] = round((sab.Hg.translation - sh).length, 1)
    for nm, fr, errs, gi in (('draw', draw, draw_err, DRAW_GRIP), ('sheath', sheath_c, sheath_err, SHEATH_REL)):
        info = {'frames': len(fr), 'seconds': round(len(fr) / 30.0, 3), 'ik_wrist_err_cm_max': max(errs), 'ik_wrist_err_cm_by_frame': errs}
        d_series, ang_series, off_series = [], [], []
        for i, b in enumerate(fr):
            P = base_P(rig, b, 0.0)
            S = P[R] @ saber_M
            o = S.translation
            d_series.append(round(-(o - sab.scabT.translation).dot(sab.axis), 1))
            ax = (S.to_3x3() @ Vector((1, 0, 0))).normalized()
            ang_series.append(round(math.degrees(ax.angle(sab.axis)), 1))
            dv = (o - sab.scabT.translation)
            off_series.append(round((dv - dv.dot(sab.axis) * sab.axis).length, 1))
        tip_in, tip_lat = [], []
        for i, b_ in enumerate(fr):
            P_ = base_P(rig, b_, 0.0)
            tp = (P_[R] @ saber_M) @ Vector((0.856, 0, 0))
            rel = tp - sab.throat
            ax_ = rel.dot(sab.axis)
            tip_in.append(round(ax_, 1))
            tip_lat.append(round((rel - ax_ * sab.axis).length, 1))
        info['tip_depth_in_scabbard_cm_by_frame (positiv = spidsen er i skeden)'] = tip_in
        info['tip_lateral_from_scabbard_axis_cm_by_frame'] = tip_lat
        inside = [(tip_in[i] > 0.0 and tip_lat[i] <= sab.TIP_DEV + 0.2) for i in range(len(fr))]   # spidsen er i skeden (dybde > 0 og hoejst TIP_DEV fra aksen)
        if nm == 'draw':
            info['tip_leaves_scabbard_first_frame_0based'] = next(i for i in range(DRAW_GRIP, len(fr)) if not inside[i])
            info['tip_leaves_scabbard_second'] = round(info['tip_leaves_scabbard_first_frame_0based'] / 30.0, 3)
        else:
            k = max(i for i in range(len(fr)) if not inside[i] and i < SHEATH_REL) + 1
            info['tip_enters_scabbard_first_frame_0based'] = k
            info['tip_enters_scabbard_second'] = round(k / 30.0, 3)
        info['saber_pulled_out_cm_by_frame'] = d_series
        info['saber_axis_angle_vs_scabbard_deg_by_frame'] = ang_series
        info['saber_lateral_offset_vs_scabbard_cm_by_frame'] = off_series
        P = base_P(rig, fr[gi], 0.0)
        Sh, Ss = P[R] @ saber_M, P[M_ + 'Hips'] @ scab_M
        info['grip_frame_0based'] = gi
        info['grip_frame_1based_blender'] = gi + 1
        info['grip_second'] = round(gi / 30.0, 4)
        info['grip_saber_transform_pos_diff_cm'] = round((Sh.translation - Ss.translation).length, 4)
        info['grip_saber_transform_rot_diff_deg'] = round(math.degrees(Sh.to_3x3().normalized().to_quaternion().rotation_difference(Ss.to_3x3().normalized().to_quaternion()).angle), 4)
        info['grip_centre_hand_vs_scabbard_cm'] = round(((Sh @ G) - (Ss @ G)).length, 4)
        info['grip_wrist_bone_to_hilt_point_cm'] = round((P[R].translation - (Ss @ Vector((HILT_X, 0, 0)))).length, 1)
        tail = P[R] @ Vector((0, rig.blen[R], 0))
        info['grip_palm_proxy_to_hilt_point_cm'] = round((tail - (Ss @ Vector((HILT_X, 0, 0)))).length, 1)
        res[nm] = info
    # kalibrering: samme proxy i Order-posen (haanden holder saeblen korrekt)
    Po = sab.Po
    Sh = Po[R] @ saber_M
    res['order_pose_palm_proxy_to_own_hilt_point_cm'] = round(((Po[R] @ Vector((0, rig.blen[R], 0))) - (Sh @ Vector((HILT_X, 0, 0)))).length, 1)
    res['order_pose_wrist_to_own_hilt_point_cm'] = round((Po[R].translation - (Sh @ Vector((HILT_X, 0, 0)))).length, 1)
    # endeposer
    e = base_P(rig, draw[-1], 0.0)
    res['draw_last_vs_order_pose_max_bone_dist_cm'] = round(max((e[n].translation - sab.Po[n].translation).length for n in rig.names), 2)
    e = base_P(rig, sheath_c[-1], 0.0)
    res['sheath_last_vs_hvil_right_arm_max_dist_cm'] = round(max((e[M_ + n].translation - sab.Ph[M_ + n].translation).length for n in ('RightShoulder', 'RightArm', 'RightForeArm', 'RightHand')), 2)
    res['sheath_last_left_hand_to_hilt_cm'] = round((e[M_ + 'LeftHand'].translation - sab.hilt).length, 2)
    e0 = base_P(rig, draw[0], 0.0)
    res['draw_first_vs_hvil_max_bone_dist_cm'] = round(max((e0[n].translation - sab.Ph[n].translation).length for n in rig.names), 3)
    e0 = base_P(rig, sheath_c[0], 0.0)
    res['sheath_first_vs_order_max_bone_dist_cm'] = round(max((e0[n].translation - sab.Po[n].translation).length for n in rig.names), 3)
    leg = [M_ + n for n in ('LeftUpLeg', 'LeftLeg', 'LeftFoot', 'LeftToeBase', 'RightUpLeg', 'RightLeg', 'RightFoot', 'RightToeBase', 'Hips')]
    mv = 0.0
    for fr in (draw, sheath_c):
        for b in fr:
            P = base_P(rig, b, 0.0)
            mv = max(mv, max((P[n].translation - sab.Ph[n].translation).length for n in leg))
    res['legs_hips_max_move_vs_idle_cm'] = round(mv, 3)
    return res


# ---------------------------------------------------------------- hovedprogram
def main():
    report = {'speed_const_cm_s': SOL_SPEED_CM_S}
    poses = dump_poses()
    saber_M, scab_M = socket_matrices('DK')
    # 0) pose-forskel mellem nationer
    diffs = {}
    for pose in ('March_Skulder', 'Ordre', 'Hvil'):
        mx = 0
        for nat in NATIONS[1:]:
            for b, m in poses[('DK', pose)].items():
                mx = max(mx, max(abs(m[i][j] - poses[(nat, pose)][b][i][j]) for i in range(4) for j in range(4)))
        diffs[pose] = mx
    report['pose_diff_vs_DK'] = diffs

    # 1) kilder fra soldaten
    solM, solPar, sol_minz = load_clip_matrices(COMBAT / 'A_Combat_WalkShoulder.fbx', frames='mesh')
    death, deathPar, _ = load_clip_matrices(COMBAT / 'A_Combat_DeathFront.fbx')
    raw = None
    if WALKING.exists():
        raw, _, _ = load_clip_matrices(WALKING)
    report['soldier_walk_frames'] = len(solM)
    report['soldier_walk_mesh_minz_cm'] = round(min(sol_minz) * 100, 2)
    report['death_src_frames'] = len(death)

    # 2) mesh pr. nation (nyt armatur: skala 0.01, ingen forskydning)
    for nat in (NATIONS if not ONLY else []):
        rig = Rig(nat)
        bb = [rig.mesh.matrix_world @ Vector(c) for c in rig.mesh.bound_box]
        report.setdefault('mesh_bbox_m', {})[nat] = [round(min(v[i] for v in bb), 3) for i in range(3)] + [round(max(v[i] for v in bb), 3) for i in range(3)]
        for o in bpy.data.objects:
            o.select_set(False)
        rig.arm.select_set(True)
        rig.mesh.select_set(True)
        bpy.context.view_layer.objects.active = rig.arm
        exp_settings(OUT / f'SK_Officer_1864_{nat}.fbx', False)
        tex = V4 / f'{nat}_Officer_1864_v4' / f'{nat}_Officer_1864_Texture_0_v4.png'
        if tex.exists():
            shutil.copyfile(tex, OUT / f'SK_Officer_1864_{nat}_Texture_0.png')
    for src, dst in ((('Saber_v4', 'SM_Officer_Saber'), ('Scabbard_v4', 'SM_Officer_Scabbard')) if not ONLY else ()):
        shutil.copyfile(V4 / 'Props_v4' / f'{src}.fbx', OUT / f'{dst}.fbx')

    # 3) klip (DK-meshen er med i klip-FBX'erne, som ved fanebaereren)
    rig = Rig('DK')
    pm, po, ph = poses[('DK', 'March_Skulder')], poses[('DK', 'Ordre')], poses[('DK', 'Hvil')]
    N = len(solM)
    # Officerens skelet er soldatens (SK_Human_1864): med soldatens hofte-position (dy=0) staar han paa samme niveau som soldaterne i spillet.
    # Jordniveau = laveste punkt af hans saaler i marchen; de holdte poser loeftes saa deres saaler rammer samme niveau.
    dy = 0.0
    walk0 = [compose_walk(rig, solM[i], pm) for i in range(N)]
    report['walk_ground_z_cm'] = round(min(rig.minz_of(b) for b in walk0) * 100, 2)   # (gammel definition: laveste mesh-punkt i hele gangen, inkl. udliggere)
    # v5.1: jordniveau = tåknoglens hoejde i stance (median af den laveste tå pr. frame). Idle/Order/Sabre-klip staar paa samme tåhøjde som Walk.
    Pw = [rig.P_from_basis(b) for b in walk0]
    toe_lo = sorted(min(P[M_ + 'LeftToeBase'].translation.y, P[M_ + 'RightToeBase'].translation.y) for P in Pw)
    toe_ground = toe_lo[len(toe_lo) // 2]
    ground_cm = None
    report['walk_toe_ground_cm'] = round(toe_ground, 2)
    report['walk_lowest_toe_range_cm'] = [round(toe_lo[0], 2), round(toe_lo[-1], 2)]
    report['walk_hips_y_range_cm'] = [round(min(P[M_ + 'Hips'].translation.y for P in Pw), 2), round(max(P[M_ + 'Hips'].translation.y for P in Pw), 2)]
    walk = walk0
    sheath = None
    if raw is not None:
        sheath = [compose_walk(rig, solM[i], pm, rawP=raw[i], sheathed=True, scab_M=scab_M, dy=dy) for i in range(N)]
    report['walk_minz_cm'] = [round(min(rig.minz_of(b) for b in walk) * 100, 2), round(max(rig.minz_of(b) for b in walk) * 100, 2)]
    report['walk_foot_stance'] = foot_stance_speed(rig, walk)
    hips_xz = [(rig.P_from_basis(b)[M_ + 'Hips'].translation.x, rig.P_from_basis(b)[M_ + 'Hips'].translation.z) for b in walk]
    report['walk_root_motion_cm'] = [round(max(h[k] for h in hips_xz) - min(h[k] for h in hips_xz), 3) for k in (0, 1)]

    def held(pose_basis, frames, amp):
        Pr = rig.P_from_basis(pose_basis)
        lift = toe_ground - min(Pr[M_ + 'LeftToeBase'].translation.y, Pr[M_ + 'RightToeBase'].translation.y)
        out = []
        for i in range(frames):
            ph_ = 2 * math.pi * i / frames
            P = rig.P_from_basis(pose_basis)
            P[M_ + 'Hips'] = Matrix.Translation((0, lift, 0)) @ P[M_ + 'Hips']
            for n in rig.names[1:]:
                p = rig.par[n]
                P[n] = P[p] @ rig.rrel[n] @ pose_basis[n]
            for n, a in ((M_ + 'Spine1', 0.7 * amp), (M_ + 'Spine2', 0.9 * amp), (M_ + 'Head', -0.6 * amp)):
                M = P[n]
                rot = Matrix.Rotation(math.radians(a * math.sin(ph_)), 3, 'X')
                rebuild_subtree(rig, P, n, Matrix.Translation(M.translation) @ (rot @ M.to_3x3()).to_4x4())
            out.append(rig.basis_from_P(P))
        return out, lift

    idle, idle_lift = held(ph, 90, 1.0)
    order, order_lift = held(po, 60, 0.4)
    report['idle_lift_cm'], report['order_lift_cm'] = round(idle_lift, 2), round(order_lift, 2)
    for nm, fr in (('idle', idle), ('order', order)):
        pb = base_P(rig, fr[0], 0.0)
        report[nm + '_hips_y_cm'] = round(pb[M_ + 'Hips'].translation.y, 2)
        report[nm + '_toe_y_cm_rel_walk_ground'] = round(min(pb[M_ + 'LeftToeBase'].translation.y, pb[M_ + 'RightToeBase'].translation.y) - toe_ground, 2)
        report[nm + '_mesh_minz_cm'] = round(rig.minz_of(fr[0]) * 100, 2)
    ground_cm = rig.minz_of(idle[0]) * 100   # sole-niveau (til forhaandsvisning)
    report['ground_sole_z_cm'] = round(ground_cm, 2)
    # saebelspids i Hvil (saeblen i haanden, tip nedad): hoejde over sål-niveau
    sab = Sabre(rig, ph, po, saber_M, scab_M, idle_lift)
    sabre_tip_hvil = (sab.Ph[M_ + 'RightHand'] @ saber_M @ Vector((0.856, 0, 0))).y - ground_cm
    report['saber_tip_hvil_above_sole_cm'] = round(sabre_tip_hvil, 2)

    # death: soldatens DeathFront, indblandet fra Hvil-posen (arme laengst)
    dstart = idle[0]
    dpose = []
    for i in range(len(death)):
        dpose.append(rig.basis_from_P(rig.build_P({n: ('sol', death[i]) for n in rig.names})))
    wb = lambda i, T: smooth(i / T)
    dfinal = []
    for i, b in enumerate(dpose):
        fr = {}
        for n in rig.names:
            T = 14 if n in ARMS else 8
            fr[n] = quat_blend(dstart[n], b[n], wb(i, T))
        dfinal.append(fr)
    report['death_first_frame_blend'] = 'Hvil-pose -> soldat DeathFront over 8 (krop) / 14 (arme) frames'
    report['death_src_hips_start_cm'] = [round(x, 2) for x in death[0][M_ + 'Hips'].translation]
    report['death_src_hips_end_cm'] = [round(x, 2) for x in death[-1][M_ + 'Hips'].translation]

    # sabelklip med IK til officerens eget faeste
    draw, draw_err, draw_hands = build_sabre_clip(sab, 'draw')
    sheath_c, sheath_err, sheath_hands = build_sabre_clip(sab, 'sheath')
    report['sabre_ik'] = sabre_report(rig, sab, saber_M, scab_M, draw, draw_err, sheath_c, sheath_err, poses)
    report['death_mesh_minz_cm'] = [round(min(rig.minz_of(b) for b in dfinal) * 100, 2), round(rig.minz_of(dfinal[0]) * 100, 2)]
    P0 = base_P(rig, dfinal[0], 0.0)
    P1 = base_P(rig, idle[0], 0.0)
    report['death_first_vs_idle_max_bone_dist_cm'] = round(max((P0[n].translation - P1[n].translation).length for n in rig.names), 3)

    clips = []
    for nm, fr in (('Walk', walk), ('WalkSheathed', sheath), ('Idle', idle), ('Order', order), ('Death', dfinal), ('SabreDraw', draw), ('SabreSheath', sheath_c)):
        if fr is None or not want(nm):
            continue
        clips.append(bake(rig, 'A_Officer_' + nm, fr, OUT / f'A_Officer_{nm}.fbx'))
    report['clips_written_this_run'] = [c['name'] for c in clips]
    report['clips'] = [{'name': 'A_Officer_' + nm, 'file': f'A_Officer_{nm}.fbx', 'frames': len(fr), 'fps': 30}
                       for nm, fr in (('Walk', walk), ('WalkSheathed', sheath), ('Idle', idle), ('Order', order), ('Death', dfinal), ('SabreDraw', draw), ('SabreSheath', sheath_c)) if fr is not None]

    # validering: stangretninger i officeren (walk) mod soldaten
    err = 0
    for i in (0, 7, 15, 22):
        Po = rig.P_from_basis(walk[i])
        for n in BODY:
            for c in rig.kids[n]:
                if c not in BODY or n not in solM[i] or c not in solM[i]:
                    continue
                v1 = (Po[c].translation - Po[n].translation).normalized()
                v2 = (solM[i][c].translation - solM[i][n].translation).normalized()
                err = max(err, math.degrees(v1.angle(v2)))
    report['walk_bone_dir_err_deg_max'] = round(err, 2)

    # 4) sabre/skede-klip: passer SabreDraw/SabreSheath paa officeren?
    report['sabre_clips'] = {}
    for cname in ('A_Combat_SabreDraw', 'A_Combat_SabreSheath', 'A_Combat_SabreSheath2', 'A_Combat_SabreWithdraw'):
        fp = COMBAT / f'{cname}.fbx'
        if not fp.exists():
            continue
        src, _, _ = load_clip_matrices(fp)
        rig2 = Rig('DK')
        info = {'frames': len(src), 'bones_in_file': len(src[0])}
        rows = []
        for i, sp in enumerate(src):
            P = rig2.build_P({n: ('sol', sp) for n in rig2.names})
            hilt = (P[M_ + 'Hips'] @ scab_M @ Vector((HILT_X, 0, 0)))
            rh = P[M_ + 'RightHand'].translation
            # haandfladens centrum ca. 0.5 haandled-laengder fra haandledet langs knoglen
            palm = P[M_ + 'RightHand'] @ Vector((0, rig2.blen[M_ + 'RightHand'] * 1.0, 0))
            rows.append((i, round((rh - hilt).length, 1), round((palm - hilt).length, 1)))
        dmin = min(r[2] for r in rows)
        near = [r[0] for r in rows if r[2] <= dmin + 4.0]
        info['min_hand_to_hilt_cm'] = dmin
        info['frame_of_min_0based'] = min(rows, key=lambda r: r[2])[0]
        info['frames_within_4cm_of_min_0based'] = [near[0], near[-1]] if near else None
        info['dist_series_palm_to_hilt'] = [r[2] for r in rows]
        report['sabre_clips'][cname] = info
    # 5) forhaandsvisning
    if not os.environ.get('SKIP_RENDER'):
        rig = Rig('DK')
        cam = setup_render(rig)
        props = add_props(rig, saber_M, scab_M)
        gz = ground_cm / 100.0
        prev = OUT / 'Preview'
        frm = prev / '_frames'
        frm.mkdir(exist_ok=True)
        row_ground = int(round(280 + 0.95 * 560 / 2.5))     # jordlinje (soleniveau) i side-billeder: ortho 2.5 m over 560 px, kamera 0,95 m over jorden
        if not ONLY:
            wf = [walk[i] for i in range(0, 30, 4)]
            f_walk = render_frames(rig, cam, props, wf, 'walk', frm, gz, views=('side',))
            compose_sheet(f_walk, prev / 'Officer_Walk_side_sheet.png', 4)
            f_walk3 = render_frames(rig, cam, props, wf[::2], 'walk', frm, gz, views=('front', 'q34'))
            compose_sheet(f_walk3, prev / 'Officer_Walk_front_q34_sheet.png', 4)
            if sheath:
                sf = [sheath[i] for i in range(0, 30, 4)]
                f_sh = render_frames(rig, cam, props, sf, 'sheath', frm, gz, on_scabbard=True, views=('side',))
                compose_sheet(f_sh, prev / 'Officer_WalkSheathed_side_sheet.png', 4)
                f_sh3 = render_frames(rig, cam, props, sf[::2], 'sheath', frm, gz, on_scabbard=True, views=('front', 'q34'))
                compose_sheet(f_sh3, prev / 'Officer_WalkSheathed_front_q34_sheet.png', 4)
        if want('Idle') or want('Order') or want('Death'):
            firsts = [idle[0], order[0], dfinal[0], dfinal[len(dfinal) // 2], dfinal[-1]]
            f_f = render_frames(rig, cam, props, firsts, 'first', frm, gz, views=('side',))
            draw_hline(f_f, row_ground)
            compose_sheet(f_f, prev / 'Officer_Idle_Order_Death_sheet.png', 5)
            f_f3 = render_frames(rig, cam, props, [idle[0], order[0]], 'firstq', frm, gz, views=('front', 'q34h'))
            compose_sheet(f_f3, prev / 'Officer_Idle_Order_q34_sheet.png', 2)
            f_f2 = render_frames(rig, cam, props, [idle[0], order[0]], 'firsts', frm, gz, views=('side',))
            draw_hline(f_f2, row_ground)
            compose_sheet(f_f2, prev / 'Officer_Idle_Order_ground_sheet.png', 2)
        for nm, fr, idx, att in (('SabreDraw', draw, (0, 6, 12, 18, 26, 36), [i < DRAW_GRIP for i in range(37)]),
                                 ('SabreSheath', sheath_c, (0, 8, 14, 20, 24, 36), [i >= SHEATH_REL for i in range(37)])):
            if not want(nm):
                continue
            fl = []
            for v in ('side', 'front', 'q34h'):
                fl += render_frames(rig, cam, props, [fr[i] for i in idx], nm + '_' + str(v), frm, gz, views=(v,), attach=[att[i] for i in idx])
            draw_hline([f for f in fl if '_side_' in f], row_ground)
            compose_sheet(fl, prev / f'Officer_{nm}_sheet.png', len(idx))
    if ONLY and (OUT / 'report.json').exists():     # delkoersel: behold felter fra tidligere fuld koersel (meshes mv.)
        old = json.loads((OUT / 'report.json').read_text())
        old.update(report)
        report = old
    (OUT / 'report.json').write_text(json.dumps(report, indent=1))
    print('OFFICER_BUILD_DONE', json.dumps({k: v for k, v in report.items() if k != 'sabre_clips'}))
    print('SABRE_CLIPS', json.dumps({k: {kk: vv for kk, vv in v.items() if kk != 'dist_series_palm_to_hilt'} for k, v in report['sabre_clips'].items()}))


if __name__ == '__main__':
    main()
