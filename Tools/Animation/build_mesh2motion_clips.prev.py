"""Mesh2Motion (CC0) -> vores skeletter, 2026-11. Retarget af udvalgte klip til hesten (SK_Horse_1864) og soldatens skelet (SK_Human_1864).
    blender -b -P Tools/Animation/build_mesh2motion_clips.py -- [<out dir>]      (default SourceAssets/Mesh2Motion)
Kilde (kun laest): D:/pinokio/api/mesh2motion/app/static/{animations/horse-animations.glb, human-base-animations.glb, human-addon-animations.glb}
  (CC0, se D:/pinokio/api/mesh2motion/app/LICENSE-CC0.MD). Maal: SourceAssets/Horse/Horse_1_rigged_v3.fbx og SourceAssets/Units1864/DK_Infantry_1864.fbx.
Env: PART=horse|human (default begge), ONLY=Idle2,Eat,... (kun disse klip; hest: Idle2,Eat,TurnLeft,TurnRight,Death,Run; menneske: navne uden A_M2M_),
  SKIP_RENDER=1 (ingen kontrolark), DEBUG_SRC=1 (kilden vises ved siden af i kontrolarket), M2M_DIR (kilde-rod), HORSE_S (hestens skala, default pelvis-hoejde-forhold).
Eksisterende outputfiler flyttes til <navn>.prev.<ext> foer de overskrives (ogsaa notes.md og ark).
Metode: kilde og maal sammenlignes i verdensrum (kilden drejes saa den vender som maalet). Skeletterne er ikke ens (andre knogletal / T-pose mod A-pose),
  saa kroppen overfoeres som verdens-rotationsdelta (hest) eller som retningsmatch (menneskets T-pose-arme), og benene faar IK mod fodens/ankelens
  forskydning fra hvile skaleret med benlaengde-forholdet, saa foedderne staar paa jorden uden at glide. Hoften faar kildens forskydning fra hvile x skala.
  En lodret kontakt-korrektion holder mesh-bunden paa samme niveau som kildens mesh-bund (doede dyr/mennesker ligger paa jorden).
  Alle klip er 30 fps; loops har ingen dublet-slutframe; ingen rodbevaegelse (alt paa stedet).
"""
import bpy, json, math, os, shutil, subprocess, sys
from pathlib import Path
import numpy as np
from mathutils import Vector, Matrix, Quaternion

args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PROJECT = Path(__file__).resolve().parents[2]
M2M = Path(os.environ.get('M2M_DIR', r'D:\pinokio\api\mesh2motion\app\static'))
OUT = Path(args[0]) if args else PROJECT / 'SourceAssets/Mesh2Motion'
CACHE = PROJECT / 'Saved/Codex/mesh2motion'
HORSE_FBX = PROJECT / 'SourceAssets/Horse/Horse_1_rigged_v3.fbx'
HORSE_REF = PROJECT / 'Reference/Animations/Horse'
SOLDIER = PROJECT / 'SourceAssets/Units1864/DK_Infantry_1864.fbx'
SOLDIER_TEX = PROJECT / 'SourceAssets/Units1864/DK_Infantry_1864_Texture_0.png'
HORSE_TEX = PROJECT / 'SourceAssets/Units1864/Horse_Texture_0.png'
ONLY = [x for x in os.environ.get('ONLY', '').split(',') if x]
PART = os.environ.get('PART', 'all')
SKIP_RENDER = bool(os.environ.get('SKIP_RENDER'))
DEBUG_SRC = bool(os.environ.get('DEBUG_SRC'))
FPS = 30
V = Vector
for d in (OUT / 'Horse', OUT / 'Human', OUT / 'Preview', CACHE):
    d.mkdir(parents=True, exist_ok=True)

HORSE_CLIPS = {   # udfil: (kilde-klip, loop, kontakt-korrektion af kroppens mesh-bund)
    'A_Horse_Idle2': ('Idle', True, False),
    'A_Horse_Eat': ('Eating', True, False),
    'A_Horse_TurnLeft': ('Turn_Left', True, False),
    'A_Horse_TurnRight': ('Turn_Right', True, False),
    'A_Horse_Death': ('Death', False, True),
    'A_Horse_Run': ('Run', True, False),
}
BASE, ADDON = 'human-base-animations.glb', 'human-addon-animations.glb'
HUMAN_CLIPS = {   # navn efter A_M2M_ : (kilde-fil, kilde-klip, loop)
    'Push': (BASE, 'Push', True),
    'Walk_Carry': (BASE, 'Walk_Carry', True),
    'Driving': (BASE, 'Driving', True),
    'Sitting_Idle': (BASE, 'Sitting_Idle', True),
    'Fixing_Kneeling': (BASE, 'Fixing_Kneeling', True),
    'Kneeling_Tired': (ADDON, 'Kneeling Tired', True),
    'Crouch_Idle': (BASE, 'Crouch_Idle', True),
    'Death_A': (ADDON, 'Death_A', False),
    'Death_B': (ADDON, 'Death_B', False),
    'Death_C': (ADDON, 'Death_C', False),
    'Death_D': (BASE, 'Death_D', False),
    'Idle_Sword': (BASE, 'Idle_Sword', True),
}


def backup(path):
    path = Path(path)
    if path.exists():
        shutil.copyfile(path, path.with_name(path.stem + '.prev' + path.suffix))


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def depth(pb):
    n = 0
    while pb.parent:
        pb = pb.parent
        n += 1
    return n


def import_new(fn, **kw):
    before = set(bpy.data.objects)
    fn(**kw)
    return [o for o in bpy.data.objects if o not in before]


def set_action(arm, act):
    arm.animation_data_create()
    arm.animation_data.action = act
    try:
        arm.animation_data.action_slot = act.slots[0]
    except Exception:
        pass


def action_key_frames(act):
    keys = set()
    for l in act.layers:
        for s in l.strips:
            for cb in s.channelbags:
                for fc in cb.fcurves:
                    for k in fc.keyframe_points:
                        keys.add(round(k.co[0], 4))
    return sorted(keys)


def goto(sc, f):
    fl = math.floor(f)
    sc.frame_set(fl, subframe=f - fl)


def mesh_minz(obj):
    """Laveste z (verden) af det evaluerede mesh."""
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = ev.to_mesh()
    n = len(me.vertices)
    co = np.empty(n * 3, dtype=np.float32)
    me.vertices.foreach_get('co', co)
    co = co.reshape(n, 3)
    M = np.array(ev.matrix_world)
    z = co @ M[2, :3] + M[2, 3]
    ev.to_mesh_clear()
    return float(z.min())


def smooth_series(a, win=2):
    n = len(a)
    return [sum(a[max(0, i - win):min(n, i + win + 1)]) / (min(n, i + win + 1) - max(0, i - win)) for i in range(n)]


def quat_path_interp(qs, fr, u):
    """qs: kilde-kvaternioner ved broeker fr (voksende); returnerer slerp ved u."""
    if u <= fr[0]:
        return qs[0].copy()
    if u >= fr[-1]:
        return qs[-1].copy()
    for i in range(len(fr) - 1):
        if fr[i] <= u <= fr[i + 1]:
            w = (u - fr[i]) / max(fr[i + 1] - fr[i], 1e-9)
            return qs[i].slerp(qs[i + 1], w)
    return qs[-1].copy()


def compose_sheet(files, out, cols):
    code = ("import sys\nfrom PIL import Image, ImageDraw\nout=sys.argv[1];cols=int(sys.argv[2]);fs=sys.argv[3:]\n"
            "ims=[]\nfor a in fs:\n    f,l=a.split('::')\n    im=Image.open(f).convert('RGB')\n    ImageDraw.Draw(im).text((8,6),l,fill=(0,0,0))\n    ims.append(im)\n"
            "w,h=ims[0].size\nrows=(len(ims)+cols-1)//cols\ns=Image.new('RGB',(w*cols,h*rows),(255,255,255))\n"
            "[s.paste(im,((i%cols)*w,(i//cols)*h)) for i,im in enumerate(ims)]\ns.save(out)\n")
    tmp = CACHE / '_sheet.py'
    tmp.write_text(code)
    backup(out)
    subprocess.run(['python', '-I', str(tmp), str(out), str(cols)] + files, check=True)


def tex_material(obj, png, color=(0.6, 0.6, 0.6, 1)):
    m = bpy.data.materials.new('m_' + obj.name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes.get('Principled BSDF')
    if png and Path(png).exists():
        img = bpy.data.images.load(str(png))
        tn = nt.nodes.new('ShaderNodeTexImage')
        tn.image = img
        nt.links.new(tn.outputs['Color'], bsdf.inputs['Base Color'])
    else:
        bsdf.inputs['Base Color'].default_value = color
    m.diffuse_color = color
    obj.data.materials.clear()
    obj.data.materials.append(m)


def setup_preview(w=420 * (2 if DEBUG_SRC else 1), h=340):
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sh = sc.display.shading
    sh.light = 'STUDIO'
    sh.color_type = 'TEXTURE'
    sh.show_shadows = True
    sc.world = bpy.data.worlds.new('w')
    sc.world.color = (0.82, 0.86, 0.9)
    sc.view_settings.view_transform = 'Standard'
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
    cam.data.type = 'ORTHO'
    cam.data.clip_end = 100
    sc.collection.objects.link(cam)
    sc.camera = cam
    bpy.ops.mesh.primitive_cube_add(size=1)
    g = bpy.context.object
    g.name = 'GROUND'
    g.scale = (30, 30, 0.02)
    g.location = (0, 0, -0.01)
    gm = bpy.data.materials.new('gnd')
    gm.diffuse_color = (0.38, 0.46, 0.3, 1)
    g.data.materials.append(gm)
    return cam


def aim_cam(cam, target, direction, scale):
    d = V(direction).normalized()
    cam.location = V(target) - d * 20
    cam.rotation_euler = d.to_track_quat('-Z', 'Y' if abs(d.z) < 0.9 else 'X').to_euler()
    cam.data.ortho_scale = scale


def preview_indices(n, loop):
    if loop:
        return [min(n - 1, int(round(k * n / 6.0))) for k in range(6)]
    return [int(round(k * (n - 1) / 5.0)) for k in range(6)]


# ================================================================== HEST
class HorseRT:
    """Retarget Mesh2Motion-hesten -> vores heste-rig (Horse_1_rigged_v3.fbx)."""
    DIRECT = {'pelvis': 'hips', 'shoulder.L': 'front_scapula_l', 'shoulder.R': 'front_scapula_r', 'ear.L': 'ear_1_l', 'ear.R': 'ear_1_r', 'jaw': 'mouth'}
    SPINE_T = ['pelvis', 'spine_01', 'spine_02', 'chest', 'neck_01', 'neck_02', 'head']
    SPINE_S = ['hips', 'spine_1', 'spine_2', 'spine_3', 'spine_4', 'spine_5', 'spine_6', 'head']
    TAIL_T = ['tail_01', 'tail_02', 'tail_03']
    TAIL_S = ['tail_1', 'tail_2', 'tail_3', 'tail_4']
    # vores pastern-knogle -> kildens knogle der begynder i samme led (fetlock) og har samme retning
    PASTERN_SRC = {'front_pastern.L': 'front_leg_ankle_l', 'front_pastern.R': 'front_leg_ankle_r',
                   'hind_pastern.L': 'back_leg_foot_l', 'hind_pastern.R': 'back_leg_foot_r'}

    def __init__(self):
        self.hind_xy = float(os.environ.get('HIND_XY', '1.0'))   # ekstra vandret skala for bagbenene
        self.leg_xy = 1.0     # vandret skala af fodforskydningen (Run bruger mindre: bagbenet strakte sig ellers for langt bagud for meshet)
        clear()
        objs = import_new(bpy.ops.import_scene.fbx, filepath=str(HORSE_FBX))
        self.T = next(o for o in objs if o.type == 'ARMATURE')
        self.mesh = next(o for o in objs if o.type == 'MESH')
        sobjs = import_new(bpy.ops.import_scene.gltf, filepath=str(M2M / 'animations/horse-animations.glb'))
        self.S = next(o for o in sobjs if o.type == 'ARMATURE')
        self.smesh = next(o for o in sobjs if o.type == 'MESH' and o.name == 'Horse')
        for o in sobjs:
            if o.type == 'MESH' and o is not self.smesh:
                bpy.data.objects.remove(o)
        self.sc = bpy.context.scene
        self.sc.render.fps = FPS
        self.S.rotation_mode = 'XYZ'
        self.S.rotation_euler = (0, 0, -math.pi / 2)    # kilden ser mod -Y, vores hest mod -X
        bpy.context.view_layer.update()
        self.WS0 = self.S.matrix_world.copy()    # fast (kun drejning), saa debug-forskydning af kilden ikke paavirker beregningen
        self.tb = {b.name: (b.head_local.copy(), b.tail_local.copy()) for b in self.T.data.bones}   # kopier: Bone-referencer er ugyldige efter edit-mode
        self.sb = {b.name: (b.head_local.copy(), b.tail_local.copy()) for b in self.S.data.bones}
        self.Wt = {b.name: self.T.matrix_world @ b.matrix_local for b in self.T.data.bones}
        self.Ws = {b.name: self.WS0 @ b.matrix_local for b in self.S.data.bones}
        self.s = float(os.environ.get('HORSE_S', self.Wt['pelvis'].translation.z / self.Ws['hips'].translation.z))
        self.add_ik()
        for nm in ('saddle_root', 'harness_root', 'bridle_root'):   # som de eksisterende A_Horse_*-klip (36 deform-knogler): disse tre er ikke deform
            if nm in self.T.data.bones:
                self.T.data.bones[nm].use_deform = False
        for pb in self.T.pose.bones:
            pb.rotation_mode = 'QUATERNION'
        self.order = sorted(self.T.pose.bones, key=depth)
        self.names = [pb.name for pb in self.order]
        self.par = {pb.name: (pb.parent.name if pb.parent else None) for pb in self.order}
        self.rrel = {b.name: ((b.parent.matrix_local.inverted() @ b.matrix_local) if b.parent else b.matrix_local.copy()) for b in self.T.data.bones}
        self.ctrl_rest = {n: self.T.data.bones[n].matrix_local.copy() for n in self.ctrl}
        self.fr_spine_t = self.fractions([self.tb[n][0] for n in self.SPINE_T], None)
        self.fr_spine_s = self.fractions([self.sb[n][0] for n in self.SPINE_S], None)
        self.fr_tail_t = self.fractions([self.tb[n][0] for n in self.TAIL_T], self.tb['tail_03'][1])
        self.fr_tail_s = self.fractions([self.sb[n][0] for n in self.TAIL_S], self.sb['tail_leaf'][0])
        print('HORSE s=%.4f' % self.s, 'spine fr T', [round(x, 2) for x in self.fr_spine_t], 'S', [round(x, 2) for x in self.fr_spine_s])

    @staticmethod
    def fractions(heads, end):
        pts = [V(p) for p in heads] + ([V(end)] if end is not None else [])
        cum = [0.0]
        for a, b in zip(pts[:-1], pts[1:]):
            cum.append(cum[-1] + (b - a).length)
        return [c / cum[-1] for c in cum[:len(heads)]]

    def add_ik(self):
        arm = self.T
        bpy.context.view_layer.objects.active = arm
        bpy.ops.object.mode_set(mode='EDIT')
        self.ctrl = {}
        for pn in self.PASTERN_SRC:
            pastern = arm.data.edit_bones[pn]
            b = arm.data.edit_bones.new('CTRL_' + pn)
            b.head, b.tail, b.roll = pastern.head.copy(), pastern.tail.copy(), pastern.roll
            b.use_deform = False
            self.ctrl['CTRL_' + pn] = pn
        bpy.ops.object.mode_set(mode='OBJECT')
        for cn, pn in self.ctrl.items():
            kind = 'front' if pn.startswith('front') else 'hind'
            side = pn.split('.')[1]
            cannon = arm.pose.bones[f'{kind}_cannon.{side}']
            ik = cannon.constraints.new('IK')
            ik.target, ik.subtarget = arm, cn
            ik.chain_count = 3
            ik.use_stretch = False
            ik.iterations = 128
            for pb in (cannon, cannon.parent, cannon.parent.parent):
                pb.ik_stretch = 0
            orient = arm.pose.bones[pn].constraints.new('COPY_ROTATION')
            orient.target, orient.subtarget = arm, cn
            orient.target_space = 'POSE'
            orient.owner_space = 'POSE'

    def src_pose(self):
        return {pb.name: self.WS0 @ pb.matrix for pb in self.S.pose.bones}

    def delta(self, sp, sname):
        return (sp[sname].to_3x3() @ self.Ws[sname].to_3x3().inverted()).to_quaternion()

    def solve(self, sp, dz=0.0):
        """Returnerer ({knogle: matrix_basis}, {ctrl: matrix_basis})."""
        Rw = {}
        for tn, sn in self.DIRECT.items():
            Rw[tn] = self.delta(sp, sn).to_matrix() @ self.Wt[tn].to_3x3()
        ds = [self.delta(sp, sn) for sn in self.SPINE_S[1:]]
        frs = self.fr_spine_s[1:]
        for tn, u in zip(self.SPINE_T[1:], self.fr_spine_t[1:]):
            Rw[tn] = quat_path_interp(ds, frs, u).to_matrix() @ self.Wt[tn].to_3x3()
        # hale: retningsmatch (kildens hale hviler vandret, vores haenger ned, saa et delta ville give en forkert retning)
        qs = [sp[sn].to_quaternion() for sn in self.TAIL_S]
        for tn, u in zip(self.TAIL_T, self.fr_tail_t):
            q = quat_path_interp(qs, self.fr_tail_s[:len(qs)], u)
            d_s = q @ V((0, 1, 0))
            Rt0 = self.Wt[tn].to_3x3()
            d_t = Rt0 @ V((0, 1, 0))
            sw = d_t.rotation_difference(d_s)
            Rw[tn] = Quaternion().slerp(sw, float(os.environ.get('TAIL_W', '0.0'))).to_matrix() @ Rt0   # 0 = halen foelger kun baekkenet: enhver ekstra hale-drejning aabner halemeshets skjulte flader (sorte plader/graa stang ved bagbenet)
        P = {}
        for n in self.names:
            p = self.par[n]
            base = (P[p] @ self.rrel[n]) if p else self.rrel[n]
            if n == 'pelvis':
                d = sp['hips'].translation - self.Ws['hips'].translation
                pos = self.Wt['pelvis'].translation + self.s * d + V((0, 0, dz))
                P[n] = Matrix.Translation(pos) @ Rw[n].to_4x4()
            elif n in Rw:
                P[n] = Matrix.Translation(base.translation) @ Rw[n].to_4x4()
            else:
                P[n] = base
        basis = {}
        for n in self.names:
            p = self.par[n]
            loc = (P[p].inverted() @ P[n]) if p else P[n]
            basis[n] = self.rrel[n].inverted() @ loc
        cb = {}
        for cn, pn in self.ctrl.items():
            sn = self.PASTERN_SRC[pn]
            d = sp[sn].translation - self.Ws[sn].translation
            k_xy = self.leg_xy * (self.hind_xy if pn.startswith('hind') else 1.0)
            d = V((d.x * k_xy, d.y * k_xy, d.z))
            pos = self.Wt[pn].translation + self.s * d + V((0, 0, dz))
            R = (self.delta(sp, sn).to_matrix() if os.environ.get('PASTERN_ROT', '1') == '1' else Matrix.Identity(3)) @ self.Wt[pn].to_3x3()
            cb[cn] = self.ctrl_rest[cn].inverted() @ (Matrix.Translation(pos) @ R.to_4x4())
        return basis, cb

    def apply(self, basis, cb):
        for pb in self.T.pose.bones:
            pb.matrix_basis = cb[pb.name] if pb.name in cb else basis[pb.name]
        bpy.context.view_layer.update()

    def build(self, out_name, src_name, loop, contact):
        act_s = bpy.data.actions[src_name]
        set_action(self.S, act_s)
        keys = action_key_frames(act_s)
        frames = keys[:-1] if loop else keys
        n = len(frames)
        self.T.animation_data_clear()
        sps, smin = [], []
        for f in frames:
            goto(self.sc, f)
            sps.append(self.src_pose())
            smin.append(mesh_minz(self.smesh))
        omin = []
        for sp in sps:
            self.apply(*self.solve(sp, 0.0))
            omin.append(mesh_minz(self.mesh))
        raw = [self.s * a - b for a, b in zip(smin, omin)]
        dz = smooth_series(raw, 2)
        dz = [0.0 if abs(v) < 0.006 else v for v in dz]
        dz = [max(-0.06, min(0.30, v)) for v in dz]
        if not contact:
            dz = [0.0] * n
        self.T.animation_data_create()
        act = bpy.data.actions.new(out_name)
        self.T.animation_data.action = act
        try:
            self.T.animation_data.action_slot = act.slots[0] if act.slots else act.slots.new('OBJECT', self.T.name)
        except Exception:
            pass
        info = {'name': out_name, 'src': src_name, 'frames': n, 'loop': loop, 'dz_applied_cm': [round(min(dz) * 100, 1), round(max(dz) * 100, 1)],
                'raw_dz_cm': [round(min(raw) * 100, 1), round(max(raw) * 100, 1)], 'src_frames': [frames[0], frames[-1]]}
        for i, sp in enumerate(sps, 1):
            basis, cb = self.solve(sp, dz[i - 1])
            self.apply(basis, cb)
            for pb in self.T.pose.bones:
                pb.keyframe_insert('location', frame=i)
                pb.keyframe_insert('rotation_quaternion', frame=i)
        self.sc.frame_start, self.sc.frame_end = 1, n
        return act, n, info

    def export(self, act, path):
        backup(path)
        arm = self.T
        arm.animation_data.action = act
        for o in bpy.data.objects:
            o.select_set(False)
        arm.select_set(True)
        bpy.context.view_layer.objects.active = arm
        bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE'}, use_armature_deform_only=True,
                                 add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                                 bake_anim_simplify_factor=0.0, bake_anim_step=1.0, bake_anim_force_startend_keying=True,
                                 apply_unit_scale=True, axis_forward='-Z', axis_up='Y')

    def preview(self, act, n, name, loop, top=False):
        sc = self.sc
        self.T.animation_data.action = act
        cam = bpy.data.objects['cam']
        fr = preview_indices(n, loop)
        views = [('side', V((0, 1, -0.05)), V((-0.1 + (1.7 if DEBUG_SRC else 0), 0, 0.8)), 2.6 + (3.4 if DEBUG_SRC else 0))]
        if top:
            views.append(('top', V((0, 0.001, -1)), V((-0.1, 0, 0.5)), 3.4 + (3.4 if DEBUG_SRC else 0)))
        sheets = []
        for vn, d, tgt, scale in views:
            aim_cam(cam, tgt, d, scale)
            files = []
            for i in fr:
                sc.frame_set(i + 1)
                if DEBUG_SRC:
                    set_action(self.S, bpy.data.actions[self.cur_src])
                    goto(sc, self.cur_frames[i])
                fn = str(CACHE / f'{name}_{vn}_{i:03d}.png')
                sc.render.filepath = fn
                bpy.ops.render.render(write_still=True)
                files.append(fn + '::' + f'f={i + 1}')
            sheet = OUT / 'Preview' / f'{name}_{vn}_sheet.png'
            compose_sheet(files, sheet, 3)
            sheets.append(sheet)
        return sheets


def measure_horse(path):
    """Laeser en eksporteret hesteklip-FBX igen og maaler frames, rod-bevaegelse, pelvis-hoejde, hov-stance-hastighed og -glid."""
    clear()
    bpy.ops.import_scene.fbx(filepath=str(path))
    r = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = r.animation_data.action
    try:
        r.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    s, e = map(int, act.frame_range)
    hooves = ['front_hoof.L', 'front_hoof.R', 'hind_hoof.L', 'hind_hoof.R']
    series = {h: [] for h in hooves}
    pel, root = [], []
    names = [b.name for b in r.data.bones]
    for fr in range(s, e + 1):
        bpy.context.scene.frame_set(fr)
        root.append(r.matrix_world @ r.pose.bones['root'].head)
        pel.append((r.matrix_world @ r.pose.bones['pelvis'].head).z)
        for h in hooves:
            series[h].append(r.matrix_world @ r.pose.bones[h].head)
    stance, slide = [], []
    for h in hooves:
        zs = [p.z for p in series[h]]
        zmin = min(zs)
        run = []
        for i, z in enumerate(zs + [99.0]):
            if z < zmin + 0.012:
                run.append(i)
            else:
                if len(run) >= 3:
                    xs = [series[h][j].x for j in run]
                    ys = [series[h][j].y for j in run]
                    dur = (run[-1] - run[0]) / FPS
                    stance.append(math.hypot(xs[-1] - xs[0], ys[-1] - ys[0]) / dur * 100.0)
                    dev = 0.0
                    for j, x, y in zip(run, xs, ys):
                        a = (j - run[0]) / max(run[-1] - run[0], 1)
                        dev = max(dev, math.hypot(x - (xs[0] + a * (xs[-1] - xs[0])), y - (ys[0] + a * (ys[-1] - ys[0]))) * 100.0)
                    slide.append(dev)
                run = []
    n = e - s + 1
    rm = root[-1] - root[0]
    return {'frames': n, 'fps': FPS, 'length_s': round((n - 1) / FPS, 3), 'bones': len(names),
            'root_motion_cm': [round(rm.x * 100, 2), round(rm.y * 100, 2), round(rm.z * 100, 2)],
            'root_travel_cm': round(max(math.hypot(p.x - root[0].x, p.y - root[0].y) for p in root) * 100, 2),
            'pelvis_z_range_cm': [round(min(pel) * 100, 1), round(max(pel) * 100, 1)],
            'stance_hoof_speed_cm_s_mean': round(sum(stance) / len(stance), 1) if stance else 0.0,
            'stance_hoof_speed_cm_s_max': round(max(stance), 1) if stance else 0.0,
            'stance_runs': len(stance), 'stance_hoof_dev_cm_max': round(max(slide), 2) if slide else 0.0,
            'hoof_z_min_cm': round(min(min(p.z for p in series[h]) for h in hooves) * 100, 1),
            'bone_names': names}


HORSE_REJECTED = set()   # klip der ikke er gode nok laegges i Horse/_afvist/ (ingen lige nu)


def horse_path(out_name):
    d = OUT / 'Horse' / ('_afvist' if out_name in HORSE_REJECTED else '')
    d.mkdir(parents=True, exist_ok=True)
    return d / (out_name + '.fbx')


def make_notes(res):
    L = []
    A = L.append
    A('# Mesh2Motion-klip paa vores skeletter (retarget i Blender)')
    A('')
    A('Genereret af `Tools/Animation/build_mesh2motion_clips.py` (genkoerbar; tidligere filer gemmes som `.prev`). **Ikke importeret i Unreal** - det goer unreal-integration.')
    A('')
    A('- **Kilde og licens:** Mesh2Motion, CC0 (`D:/pinokio/api/mesh2motion/app/LICENSE-CC0.MD`): `static/animations/horse-animations.glb`, `human-base-animations.glb`, `human-addon-animations.glb`. Kildefilerne er kun laest.')
    A('- **Maal:** hest = `SourceAssets/Horse/Horse_1_rigged_v3.fbx` (SK_Horse_1864, 36 deform-knogler som de eksisterende A_Horse_*-klip); menneske = `SourceAssets/Units1864/DK_Infantry_1864.fbx` (SK_Human_1864, 41 Mixamo-knogler, uaendret).')
    A('- **30 fps.** Menneskeklippene er resamplet fra 24 fps; hesteklippene var 30 fps. Loops har **ingen dublet-slutframe** (sidste frame foerer direkte over i foerste).')
    A('- **Alt paa stedet:** ingen rodbevaegelse (hestens `root` / soldatens `Hips` flytter sig ikke i xy ud over klippets egen kropsbevaegelse).')
    A('- **Akser/import:** hest = samme eksport som horse_gaits_blender.py (axis_forward -Z, axis_up Y, kun deform-knogler) -> **roll 0** ved import (HORSE_CLIP_ROLL). Menneske = samme eksport som A_Gun_*/A_Officer_* (armatur + mesh i FBX) -> samme import som de klip (roll 90, ingen ekstra hofte-tillaeg: Hips er loeftet 98,079 cm, saa toeknoglen staar paa 0; mesh-sulen ligger da ca. 1,9 cm under 0).')
    A('- **Ikke relevant her:** ingen klip holder en riffel (skulder-gevaer-reglen er ikke i spil); Walk_Carry har tomme baere-haender.')
    A('')
    A('## Metode')
    A('- Hest: kroppen (baekken, rygsoejle, hals, hoved, ore, kaebe, skuldre) som verdens-rotationsdelta fra Mesh2Motion-hesten (kaede-broeker for rygsoejle/hale); halen foelger kun baekkenet (stille). Benene: IK (som horse_gaits_blender.py) mod kildens fetlock-forskydning fra hvile x 0,81 (hoejdeforhold), saa hovene staar paa jorden. Baekkenet = hvile + 0,81 x kildens forskydning.')
    A('- Menneske: T-pose -> A-pose, hver knogles retning/vridning overfoeres (verdens-orientering + fast hvile-forskel), ben via 2-knogle-IK mod ankel-forskydning fra hvile x 0,937 (benlaengde-forhold), hofte = hvile + 0,937 x kildens forskydning. Kun tommel og pegefinger findes paa soldaten.')
    A('- Lodret kontakt-korrektion (kun naar forskellen er over 0,6 cm): mesh-bunden holdes paa samme niveau som kildens mesh-bund (doede/liggende figurer ligger paa jorden). Hest: kun Death.')
    A('')
    A('## Hest (SourceAssets/Mesh2Motion/Horse)')
    A('| Klip | Kilde-klip | Frames | Laengde (n-1)/30 | Cyklus n/30 | Loop | Baekkenhoejde cm | Rodbevaegelse cm | Bemaerkning |')
    A('|---|---|---|---|---|---|---|---|---|')
    for k, v in res.get('horse', {}).items():
        m = v['measure']
        rem = {'A_Horse_Idle2': 'staar; hov-glid max %s cm' % m['stance_hoof_dev_cm_max'],
               'A_Horse_Eat': 'graesning, hovene stille; mulen gaar ca. 5 cm ned i jorden (graes)',
               'A_Horse_TurnLeft': 'trin paa stedet (hoven drejer rundt i kontakt, glid op til %s cm); selve kursaendringen goer aktoeren' % m['stance_hoof_dev_cm_max'],
               'A_Horse_TurnRight': 'som TurnLeft (glid op til %s cm)' % m['stance_hoof_dev_cm_max'],
               'A_Horse_Death': 'staar, falder om paa siden og ligger (ingen loop); baekkenet ender ca. %s cm over jorden' % m['pelvis_z_range_cm'][0],
               'A_Horse_Run': 'kort galop/rask galop-cyklus; stance-hov-hastighed (= rodhastighed hvis aktoeren flyttes) %s cm/s, mellem A_Horse_Canter 357 og A_Horse_Gallop 757; bagbenet naar 99 %% af fuld laengde' % m['stance_hoof_speed_cm_s_mean']}[k]
        A('| %s%s | %s | %d | %.3f s | %.3f s | %s | %s-%s | %s | %s |' % (k, ' (afvist)' if k in HORSE_REJECTED else '', v['src'], v['frames'], m['length_s'], v['frames'] / 30.0,
                                                                 'ja' if v['loop'] else 'nej', m['pelvis_z_range_cm'][0], m['pelvis_z_range_cm'][1], m['root_motion_cm'], rem))
    A('')
    A('Hestens hvile-baekkenhoejde er 105,8 cm (A_Horse_Stand 105,5-106,0); Idle2 ligger 104,3-104,9 cm.')
    A('')
    A('## Menneske (SourceAssets/Mesh2Motion/Human)')
    A('| Klip | Kilde-klip | Frames | Laengde (n-1)/30 | Cyklus n/30 | Loop | Hoftehoejde cm min-max (foerste frame) | Hofte x/y foerste til sidste frame cm | Stance-fod | Bemaerkning |')
    A('|---|---|---|---|---|---|---|---|---|---|')
    nh = {'Push': 'skubber, laenet frem; ingen genstand', 'Walk_Carry': 'GANG: langsom baerende gang, arme foran kroppen (tomme)', 'Driving': 'siddende, haender paa et rat (ingen rat/stol i klippet)',
          'Sitting_Idle': 'siddende paa usynlig stol', 'Fixing_Kneeling': 'staar, knaeler og arbejder (156 kilde-frames)', 'Kneeling_Tired': 'knaelende traet, kort loop',
          'Crouch_Idle': 'hug', 'Death_A': 'staar, falder paa knae, forover paa jorden', 'Death_B': 'hop og fald forover, ligger paa siden', 'Death_C': 'baglaens fald med rulle',
          'Death_D': 'saenker sig, falder bagover og ligger', 'Idle_Sword': 'staar parat (ingen vaaben i klippet)'}
    for k, v in res.get('human', {}).items():
        m = v['measure']
        nm = k.replace('A_M2M_', '')
        sp = 'median %s cm/s, glid %s cm' % (m['stance_foot_speed_cm_s_median'], m['stance_foot_dev_cm_max']) if m['stance_runs'] else '-'
        A('| %s | %s | %d | %.3f s | %.3f s | %s | %s-%s (%s) | %s / %s | %s | %s |' % (k, v['src'], v['frames'], m['length_s'], v['frames'] / 30.0, 'ja' if v['loop'] else 'nej',
          m['hips_height_cm'][0], m['hips_height_cm'][1], m['hips_height_cm'][2], m['root_motion_cm'][0], m['root_motion_cm'][1], sp, nh.get(nm, '')))
    A('')
    A('Death_A og Death_D flytter hoften hhv. ca. 65 og 43 cm i xy (selve faldet); alle andre klip bliver paa stedet.')
    A('')
    A('Hoftehoejde = Hips-knoglens hoejde over toeknoglens nulniveau efter eksport (staaende hvile ca. 94 cm som A_Officer_Idle). De lave vaerdier (siddende 59, knaelende 46-48, hug 53) er klippenes egen kropsholdning; Death/Fixing starter staaende (87-93 cm) og ender liggende/knaelende.')
    A('')
    if 'A_M2M_Walk_Carry' in res.get('human', {}):
        w = res['human']['A_M2M_Walk_Carry']
        A('**Gangklip (hastighed):** kun `A_M2M_Walk_Carry`. Rodhastighed (= stance-fodens hastighed, saa foedderne ikke glider hvis aktoeren flyttes med denne fart): **%s cm/s**, laengde %s s (%d frames, cyklus %.3f s), rodbevaegelse i klippet 0 (paa stedet), fodglid maks %s cm. Til sammenligning gaar infanteriets march-klip 137,7 cm/s.' % (
            w['measure']['stance_foot_speed_cm_s_median'], w['measure']['length_s'], w['frames'], w['frames'] / 30.0, w['measure']['stance_foot_dev_cm_max']))
        A('')
    A('## Kontrolark (Preview/)')
    A('Et ark pr. klip: `Preview/<klip>_sheet.png` (menneske) og `Preview/<klip>_side_sheet.png` (hest; turn-klip ogsaa `_top_sheet.png`). 6 frames, loops jaevnt fordelt over cyklussen. Mesh uden hesteens/soldatens endelige materiale i UE.')
    A('')
    A('## Kendte begraensninger')
    A('- **A_Horse_Run:** brugbar som en hurtig kantergang/let galop (ca. 487 cm/s stance-hov-hastighed), men IKKE en erstatning for A_Horse_Gallop (757 cm/s); sammenlign i spillet foer brug.')
    A('- **Mesh-artefakter ved bagbenet:** i klip hvor venstre bagben staar anderledes end i hvile ses moerke planker bag hasen og en graa spids ved hoven (Eat, Turn, Run, Death). Det er skinning/hale-mesh i Horse_1_rigged_v3 og ses ogsaa i de eksisterende klip (A_Horse_Gallop, kontrolleret i Blender); retargetingen er ikke aarsagen.')
    A('- Hestens hale er stille (foelger kun baekkenet): enhver ekstra hale-drejning (selv 25 %) aabner halemeshets skjulte flader til sorte plader/en graa stang ved bagbenet. Kilden svinger halen; det er udeladt.')
    A('- Turn-klippene drejer ikke hesten (kilden er ogsaa paa stedet).')
    A('- Haender/fingre: kun tommel og pegefinger; haandpositioner kan afvige nogle cm fra kilden (kortere arme). Ingen rekvisitter (rat, stol, skubbet genstand).')
    A('- Death_B: toeknoglen ligger kortvarigt ca. 3 cm under nulniveau; Death_C har en luftfase.')
    (OUT / 'notes.md').write_text('\n'.join(L), encoding='utf-8')


def run_horse():
    rt = HorseRT()
    results = {}
    setup_preview()
    tex_material(rt.mesh, HORSE_TEX, (0.45, 0.30, 0.2, 1))
    if DEBUG_SRC:
        rt.S.location = (3.4, 0, 0)
        tex_material(rt.smesh, None, (0.3, 0.3, 0.6, 1))
    else:
        rt.smesh.hide_render = True
        rt.smesh.hide_viewport = True
    for out_name, (src_name, loop, contact) in HORSE_CLIPS.items():
        short = out_name.replace('A_Horse_', '')
        if ONLY and short not in ONLY and out_name not in ONLY:
            continue
        rt.leg_xy = float(os.environ.get('RUN_LEG_XY', '1.0')) if 'Run' in out_name else 1.0
        act, n, info = rt.build(out_name, src_name, loop, contact)
        rt.cur_src, rt.cur_frames = src_name, (action_key_frames(bpy.data.actions[src_name])[:n])
        rt.export(act, horse_path(out_name))
        if not SKIP_RENDER:
            sheets = rt.preview(act, n, out_name, loop, top=('Turn' in out_name))
            info['sheets'] = [str(s).replace(chr(92), '/') for s in sheets]
        results[out_name] = info
        print('HORSE CLIP', out_name, info)
    return results


def finish_horse(results):
    for out_name, info in results.items():
        info['measure'] = measure_horse(horse_path(out_name))
        print('HORSE MEASURE', out_name, {k: v for k, v in info['measure'].items() if k != 'bone_names'})
    return results


# ================================================================== MENNESKE
M_ = 'mixamorig:'
T4 = Matrix.Translation


def R4(R3):
    return R3.to_4x4()


def build_hmap():
    m = {'Hips': 'pelvis', 'Spine': 'spine_01', 'Spine1': 'spine_02', 'Spine2': 'spine_03', 'Neck': 'neck_01', 'Head': 'head', 'HeadTop_End': 'head_leaf'}
    for S, s in (('Left', 'l'), ('Right', 'r')):
        m[S + 'Shoulder'] = f'clavicle_{s}'
        m[S + 'Arm'] = f'upperarm_{s}'
        m[S + 'ForeArm'] = f'lowerarm_{s}'
        m[S + 'Hand'] = f'hand_{s}'
        for i in (1, 2, 3):
            m[f'{S}HandThumb{i}'] = f'thumb_0{i}_{s}'
            m[f'{S}HandIndex{i}'] = f'index_0{i}_{s}'
        m[S + 'HandThumb4'] = f'thumb_04_leaf_{s}'
        m[S + 'HandIndex4'] = f'index_04_leaf_{s}'
        m[S + 'UpLeg'] = f'thigh_{s}'
        m[S + 'Leg'] = f'calf_{s}'
        m[S + 'Foot'] = f'foot_{s}'
        m[S + 'ToeBase'] = f'ball_{s}'
        m[S + 'Toe_End'] = f'ball_leaf_{s}'
    return {M_ + k: v for k, v in m.items()}


HMAP = build_hmap()


def rebuild_subtree(rig, P, root, newP):
    old = {n: P[n].copy() for n in P}
    P[root] = newP
    stack = list(rig.kids[root])
    while stack:
        n = stack.pop()
        p = rig.par[n]
        P[n] = P[p] @ (old[p].inverted() @ old[n])
        stack.extend(rig.kids[n])


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


class HumanRT:
    def __init__(self):
        clear()
        objs = import_new(bpy.ops.import_scene.fbx, filepath=str(SOLDIER))
        self.T = next(o for o in objs if o.type == 'ARMATURE')
        self.mesh = next(o for o in objs if o.type == 'MESH')
        for o in objs:
            if o.type in ('CAMERA', 'LIGHT'):
                bpy.data.objects.remove(o)
        self.sc = bpy.context.scene
        self.sc.render.fps = FPS
        bpy.context.view_layer.update()
        self.MW = self.T.matrix_world.copy()
        self.Rarm = self.MW.to_3x3().normalized()          # armatur -> verden (kun drejning)
        self.Rarm_inv = self.Rarm.inverted()
        self.Sc = self.MW.to_scale()[0]                    # cm -> m (0.01)
        for pb in self.T.pose.bones:
            pb.rotation_mode = 'QUATERNION'
        self.order = sorted(self.T.pose.bones, key=depth)
        self.names = [pb.name for pb in self.order]
        self.par = {pb.name: (pb.parent.name if pb.parent else None) for pb in self.order}
        self.restL = {b.name: b.matrix_local.copy() for b in self.T.data.bones}
        self.rrel = {b.name: ((b.parent.matrix_local.inverted() @ b.matrix_local) if b.parent else b.matrix_local.copy()) for b in self.T.data.bones}
        self.blen = {b.name: b.length for b in self.T.data.bones}
        self.kids = {n: [] for n in self.names}
        for n, p in self.par.items():
            if p:
                self.kids[p].append(n)
        self.G = mesh_minz(self.mesh)       # jordniveau (verden, m) = mesh-bund i hvile
        self.hips_fix = float(os.environ.get('HIPS_FIX', '98.079'))   # som A_Gun_*/A_Officer_*: Hips Lcl op saa TOEKNOGLEN (ikke mesh-sulen) staar paa z=0; sulen ligger da ca. 1,9 cm under 0
        print('HUMAN ground G=%.4f m, hips fix %.3f cm' % (self.G, self.hips_fix), 'hips rest rig', tuple(round(x, 2) for x in self.restL[M_ + 'Hips'].translation))
        self.src = None

    def load_src(self, fname):
        if self.src is not None:
            for o in self.src['objs']:
                if o.name in bpy.data.objects:
                    bpy.data.objects.remove(o)
            for a in list(bpy.data.actions):
                bpy.data.actions.remove(a)
        objs = import_new(bpy.ops.import_scene.gltf, filepath=str(M2M / 'animations' / fname))
        arm = next(o for o in objs if o.type == 'ARMATURE')
        mesh = next(o for o in objs if o.type == 'MESH' and o.name == 'Mannequin')
        for o in objs:
            if o.type == 'MESH' and o is not mesh:
                bpy.data.objects.remove(o)
        bpy.context.view_layer.update()
        self.src = {'arm': arm, 'mesh': mesh, 'objs': [arm, mesh]}
        self.Ws = {b.name: arm.matrix_world @ b.matrix_local for b in arm.data.bones}
        self.WS0 = arm.matrix_world.copy()
        self.Rres = {}
        for tn, sn in HMAP.items():
            Rs = self.Ws[sn].to_3x3()
            Rt = self.Rarm @ self.restL[tn].to_3x3()
            ds, dt = Rs @ V((0, 1, 0)), Rt @ V((0, 1, 0))
            Q = ds.rotation_difference(dt).to_matrix()
            self.Rres[tn] = Rs.inverted() @ Q.inverted() @ Rt
        hip_t = self.restL[M_ + 'LeftUpLeg'].translation
        ank_t = self.restL[M_ + 'LeftFoot'].translation
        lt = (hip_t - ank_t).length * self.Sc
        ls = (self.Ws['thigh_l'].translation - self.Ws['foot_l'].translation).length
        self.s = float(os.environ.get('HUMAN_S', lt / ls))
        print('HUMAN s=%.4f (leg %.3f / %.3f)' % (self.s, lt, ls))

    def src_pose(self):
        arm = self.src['arm']
        return {pb.name: self.WS0 @ pb.matrix for pb in arm.pose.bones}

    def to_rig_vec(self, v):
        return self.Rarm_inv @ v / self.Sc

    def solve(self, sp, dz=0.0):
        """Returnerer (P: knogle -> armaturrum-matrix, basis: knogle -> matrix_basis)."""
        Rt = {}
        for tn, sn in HMAP.items():
            Rt[tn] = self.Rarm_inv @ (sp[sn].to_3x3() @ self.Rres[tn])
        hips = M_ + 'Hips'
        dH = sp['pelvis'].translation - self.Ws['pelvis'].translation
        hpos = self.restL[hips].translation + self.s * self.to_rig_vec(dH) + V((0, dz / self.Sc, 0))
        P = {}
        for n in self.names:
            p = self.par[n]
            base = (P[p] @ self.rrel[n]) if p else self.rrel[n]
            if n == hips:
                P[n] = T4(hpos) @ R4(Rt[n])
            elif n in Rt:
                P[n] = T4(base.translation) @ R4(Rt[n])
            else:
                P[n] = base
        for side, s in (('Left', 'l'), ('Right', 'r')):
            up, lo, ft, tb, te = [M_ + side + x for x in ('UpLeg', 'Leg', 'Foot', 'ToeBase', 'Toe_End')]
            dA = sp['foot_' + s].translation - self.Ws['foot_' + s].translation
            target = self.restL[ft].translation + self.s * self.to_rig_vec(dA) + V((0, dz / self.Sc, 0))
            hipw, kneew, ankw = sp['thigh_' + s].translation, sp['calf_' + s].translation, sp['foot_' + s].translation
            ax = ankw - hipw
            ax = ax.normalized() if ax.length > 1e-6 else V((0, 0, 1))
            perp = (kneew - hipw) - ((kneew - hipw).dot(ax)) * ax
            pole = self.to_rig_vec(perp + V((0, -1, 0)) * 0.03).normalized()
            two_bone_ik(self, P, up, lo, target, pole)
            P[ft] = T4(P[ft].translation) @ R4(Rt[ft])
            P[tb] = T4((P[ft] @ self.rrel[tb]).translation) @ R4(Rt[tb])
            P[te] = T4((P[tb] @ self.rrel[te]).translation) @ R4(Rt[te])
        basis = {}
        for n in self.names:
            p = self.par[n]
            loc = (P[p].inverted() @ P[n]) if p else P[n]
            basis[n] = self.rrel[n].inverted() @ loc
        return P, basis

    def apply(self, basis):
        for pb in self.T.pose.bones:
            pb.matrix_basis = basis[pb.name]
        bpy.context.view_layer.update()

    def build(self, name, src_name, loop):
        act_s = bpy.data.actions[src_name]
        arm_s = self.src['arm']
        set_action(arm_s, act_s)
        keys = action_key_frames(act_s)
        K = len(keys) - 1                      # intervaller ved 24 fps
        dur = K / 24.0
        n = int(round(dur * FPS)) if loop else int(round(dur * FPS)) + 1
        denom = n if loop else (n - 1)
        fr = [keys[0] + k * K / denom for k in range(n)]
        sps, smin = [], []
        for f in fr:
            goto(self.sc, f)
            sps.append(self.src_pose())
            smin.append(mesh_minz(self.src['mesh']))
        omin = []
        for sp in sps:
            P, basis = self.solve(sp, 0.0)
            self.apply(basis)
            omin.append(mesh_minz(self.mesh))
        raw = [self.G + self.s * a - b for a, b in zip(smin, omin)]
        dz = smooth_series(raw, 2)
        dz = [0.0 if abs(v) < 0.006 else v for v in dz]
        dz = [max(-0.05, min(0.35, v)) for v in dz]
        bases = []
        for sp, d in zip(sps, dz):
            P, basis = self.solve(sp, d)
            bases.append(basis)
        info = {'name': name, 'src': src_name, 'frames': n, 'loop': loop, 'src_frames_24fps': K + (0 if loop else 1),
                'dz_applied_cm': [round(min(dz) * 100, 1), round(max(dz) * 100, 1)], 'raw_dz_cm': [round(min(raw) * 100, 1), round(max(raw) * 100, 1)]}
        return bases, info, fr

    def with_hips_fix(self, bases):
        h = M_ + 'Hips'
        out = []
        for b in bases:
            nb = dict(b)
            P = self.rrel[h] @ b[h]
            nb[h] = self.rrel[h].inverted() @ (T4(V((0, self.hips_fix, 0))) @ P)
            out.append(nb)
        return out

    def export(self, name, bases, path):
        bases = self.with_hips_fix(bases)
        backup(path)
        arm = self.T
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
        self.sc.frame_start, self.sc.frame_end = 1, len(bases)
        for o in bpy.data.objects:
            o.select_set(False)
        arm.select_set(True)
        self.mesh.select_set(True)
        bpy.context.view_layer.objects.active = arm
        bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, use_mesh_modifiers=False,
                                 bake_anim=True, axis_forward='-Z', axis_up='Y', bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                                 bake_anim_simplify_factor=0, bake_anim_force_startend_keying=True)
        arm.animation_data_clear()

    def preview(self, name, bases, loop, fr_src):
        sc = self.sc
        cam = bpy.data.objects['cam']
        idx = preview_indices(len(bases), loop)
        aim_cam(cam, V((0.7 if DEBUG_SRC else 0, 0, self.G + 0.9)), V((-1, 0.9, -0.12)), 2.3 + (1.8 if DEBUG_SRC else 0))
        files = []
        arm_s = self.src['arm']
        for i in idx:
            self.apply(bases[i])
            if DEBUG_SRC:
                goto(sc, fr_src[i])
            fn = str(CACHE / f'{name}_{i:03d}.png')
            sc.render.filepath = fn
            bpy.ops.render.render(write_still=True)
            files.append(fn + '::' + f'f={i + 1}')
        sheet = OUT / 'Preview' / f'{name}_sheet.png'
        compose_sheet(files, sheet, 3)
        return sheet


def measure_human(path):
    clear()
    bpy.ops.import_scene.fbx(filepath=str(path))
    r = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = r.animation_data.action
    try:
        r.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    s, e = map(int, act.frame_range)
    names = [b.name for b in r.data.bones]
    hips, ank, toe = [], {'L': [], 'R': []}, {'L': [], 'R': []}
    for fr in range(s, e + 1):
        bpy.context.scene.frame_set(fr)
        W = r.matrix_world
        hips.append(W @ r.pose.bones[M_ + 'Hips'].head)
        for k, side in (('L', 'Left'), ('R', 'Right')):
            ank[k].append(W @ r.pose.bones[M_ + side + 'Foot'].head)
            toe[k].append(W @ r.pose.bones[M_ + side + 'ToeBase'].head)
    n = e - s + 1
    zmin_toe = min(p.z for k in toe for p in toe[k])
    speeds, devs = [], []
    for k in ('L', 'R'):
        zs = [p.z for p in ank[k]]
        z0 = min(zs)
        run = []
        for i, z in enumerate(zs + [99.0]):
            if z < z0 + 0.012:
                run.append(i)
            else:
                if len(run) >= 3:
                    xs = [ank[k][j].x for j in run]
                    ys = [ank[k][j].y for j in run]
                    dur = (run[-1] - run[0]) / FPS
                    speeds.append(math.hypot(xs[-1] - xs[0], ys[-1] - ys[0]) / dur * 100.0)
                    dev = 0.0
                    for j, x, y in zip(run, xs, ys):
                        a = (j - run[0]) / max(run[-1] - run[0], 1)
                        dev = max(dev, math.hypot(x - (xs[0] + a * (xs[-1] - xs[0])), y - (ys[0] + a * (ys[-1] - ys[0]))) * 100.0)
                    devs.append(dev)
                run = []
    rm = hips[-1] - hips[0]
    hz = [p.z for p in hips]
    return {'frames': n, 'fps': FPS, 'length_s': round((n - 1) / FPS, 3), 'bones': len(names),
            'root_motion_cm': [round(rm.x * 100, 2), round(rm.y * 100, 2), round(rm.z * 100, 2)],
            'hips_travel_cm_xy': round(max(math.hypot(p.x - hips[0].x, p.y - hips[0].y) for p in hips) * 100, 1),
            'hips_height_cm': [round(min(hz) * 100, 1), round(max(hz) * 100, 1), round(hz[0] * 100, 1)],
            'toe_z_min_cm': round(zmin_toe * 100, 1), 'stance_runs': len(speeds),
            'stance_foot_speed_cm_s_median': round(sorted(speeds)[len(speeds) // 2], 1) if speeds else 0.0,
            'stance_foot_speed_cm_s_max': round(max(speeds), 1) if speeds else 0.0,
            'stance_foot_dev_cm_max': round(max(devs), 2) if devs else 0.0,
            'bone_names': names}


def run_human():
    rt = HumanRT()
    results = {}
    setup_preview(420 * (2 if DEBUG_SRC else 1), 420)
    tex_material(rt.mesh, SOLDIER_TEX)
    bpy.data.objects['GROUND'].location.z = rt.G - 0.01
    cur_file = None
    for name, (fname, src_name, loop) in HUMAN_CLIPS.items():
        out_name = 'A_M2M_' + name
        if ONLY and name not in ONLY and out_name not in ONLY:
            continue
        if fname != cur_file:
            rt.load_src(fname)
            if DEBUG_SRC:
                rt.src['arm'].location = (1.4, 0, 0)
                tex_material(rt.src['mesh'], None, (0.3, 0.3, 0.7, 1))
            else:
                rt.src['mesh'].hide_render = True
            cur_file = fname
        bases, info, fr = rt.build(out_name, src_name, loop)
        if not SKIP_RENDER:
            info['sheet'] = str(rt.preview(out_name, bases, loop, fr)).replace(chr(92), '/')
        rt.export(out_name, bases, OUT / 'Human' / (out_name + '.fbx'))
        results[out_name] = info
        print('HUMAN CLIP', out_name, info)
    return results


def finish_human(results):
    for out_name, info in results.items():
        info['measure'] = measure_human(OUT / 'Human' / (out_name + '.fbx'))
        print('HUMAN MEASURE', out_name, {k: v for k, v in info['measure'].items() if k != 'bone_names'})
    return results


def main():
    res = {}
    if PART in ('all', 'horse'):
        res['horse'] = run_horse()
    if PART in ('all', 'human'):
        res['human'] = run_human()
    if PART in ('all', 'horse'):
        res['horse'] = finish_horse(res['horse'])
    if PART in ('all', 'human'):
        res['human'] = finish_human(res['human'])
    (CACHE / 'results.json').write_text(json.dumps(res, indent=1, default=str))
    if PART == 'all':
        backup(OUT / 'notes.md')
        make_notes(res)
    print('M2M DONE')


if __name__ == '__main__':
    main()
