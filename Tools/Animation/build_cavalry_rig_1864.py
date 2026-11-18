"""Rytterfigurer v1 (2026-11): fem Hunyuan-figurer (A-pose, 40k trekanter, ca. 2,0 m) rigget til soldaternes skelet SK_Human_1864, til rytteren paa den riggede hest.

    blender -b -P build_cavalry_rig_1864.py -- [build|preview|all] [SE_Husar PR_Husar PR_Dragon SE_Dragon DK_Dragon]      (default: all, alle fem)

Samme metode som build_artillery_rig_1864.py (armatur uroert, nearest-surface-vaegt-transfer fra soldatens mesh, arm-tilpasning, haandled efter tekstur,
haender = kun Hand-knoglen, kantudglatning, maks. 4 pr. vertex), men med rytterfigurer og rytterklip til kontrolarket.
Input (aendres ikke): glb-versionerne ved siden af Reference/Units/MultiView/*Cavalry/*_textured.glb (samme mesh som de angivne FBX; DK_Dragon-FBX'ens tekstur er
  ikke laesbar i Blender, glb er); SourceAssets/Units1864/DK_Infantry_1864.fbx (soldatens armatur + mesh = skelet, bindepose, vaegtdonor).
Output: SourceAssets/Units1864/Cavalry/Final_v1/SK_Cavalry_1864_{NAVN}.fbx (+ _Texture_0.png), report.json, Preview/Cavalry_{NAVN}_sheet.png + _zoom.png.
Preview: A-pose (hvile), A_Combat_CavWalk fr. 14, CavGallop fr. 8, CavCharge fr. 8, CavSabreDraw fr. 10 (pose overfoert som officerens: lokal rotation pr. knogle),
  forfra (oeverst) og fra siden (nederst); zoom-ark: hoved/haender (hvile og galop), sædet (gallop, fra siden), skuldre/haender i CavSabreDraw.
Env: HAND_MODE=rigid|transfer, SMOOTH=<iterationer>, RENDER=cycles|workbench, SAMPLES=<n>, BASELINE=1 (soldatens eget mesh i de samme klip = reference).
Eksisterende filer i Final_v1 gemmes som *.prev.* foer de overskrives.
"""
import bpy, bmesh, json, math, os, shutil, sys
from pathlib import Path
from mathutils import Vector, Matrix, Quaternion
from mathutils.bvhtree import BVHTree

ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
MODE = next((a for a in ARGS if a in ('build', 'preview', 'all')), 'all')
ALL_MODELS = ['SE_Husar', 'PR_Husar', 'PR_Dragon', 'SE_Dragon', 'DK_Dragon']
NATS_ARG = [a for a in ARGS if a in ALL_MODELS]
NATIONS = NATS_ARG or ALL_MODELS
PROJECT = Path(__file__).resolve().parents[2]
SRC = Path(os.environ.get('CAV_SRC', r'R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView'))
SOLDIER = PROJECT / 'SourceAssets/Units1864/DK_Infantry_1864.fbx'
OUT = PROJECT / 'SourceAssets/Units1864/Cavalry/Final_v1'
PREV = OUT / 'Preview'
HAND_MODE = os.environ.get('HAND_MODE', 'rigid')
WRIST_PCT = float(os.environ.get('WRIST_PCT', '0.05'))
ARM_FIT = os.environ.get('ARM_FIT', '1') == '1'
ARM_FIT_ITER = int(os.environ.get('ARM_FIT_ITER', '2'))
SMOOTH = int(os.environ.get('SMOOTH', '2'))
RENDER = os.environ.get('RENDER', 'cycles')
SAMPLES = int(os.environ.get('SAMPLES', '24'))
M_ = 'mixamorig:'
MODEL_GLB = {
    'SE_Husar': 'Swedish Cavalry/SE_Husar_1864_Apose_textured.glb',
    'PR_Husar': 'Prussisk Cavalry/PR_Husar_1864_Apose_textured.glb',
    'PR_Dragon': 'Prussisk Cavalry/PR_Dragon_1864_Apose_textured.glb',
    'SE_Dragon': 'Swedish Cavalry/SE_Dragon_1864_Apose_textured.glb',
    'DK_Dragon': 'Danish Cavalry/DK_Dragon_1864_textured.glb',
}
CLIPS = {  # navn -> (fil, frames (Blender-frames; klippene har frame 2..n))
    'Walk': ('SourceAssets/InfantryCombat/A_Combat_CavWalk.fbx', [14]),
    'Gallop': ('SourceAssets/InfantryCombat/A_Combat_CavGallop.fbx', [8]),
    'Charge': ('SourceAssets/InfantryCombat/A_Combat_CavCharge.fbx', [8]),
    'SabreDraw': ('SourceAssets/InfantryCombat/A_Combat_CavSabreDraw.fbx', [10]),
}
# zoom-ark: (tag, azimut, zone, ortho-bredde m); zone 'top' = overkrop/hoved, 'seat' = hofte/laar
ZOOM = [('Hvile', 0, 'top', 1.15), ('Gallop8', 0, 'top', 1.15), ('Gallop8', 90, 'top', 1.15), ('Gallop8', 90, 'seat', 1.5), ('Charge8', 90, 'seat', 1.5), ('SabreDraw10', 0, 'top', 1.15)]
OUT.mkdir(parents=True, exist_ok=True)
PREV.mkdir(parents=True, exist_ok=True)


def keep_prev(path):
    path = Path(path)
    if path.exists():
        dst = path.with_name(path.stem + '.prev' + path.suffix)
        shutil.copyfile(path, dst)


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 30


def load(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    return [o for o in bpy.data.objects if o not in before]


def world_bounds(pts):
    return [min(p[i] for p in pts) for i in range(3)], [max(p[i] for p in pts) for i in range(3)]


def tri_count(me):
    me.calc_loop_triangles()
    return len(me.loop_triangles)


# ------------------------------------------------------------------ build
def donor_bvh(sol):
    """BVH over soldatens mesh i verdensrum + vaegte pr. vertex (dict gruppe->vaegt)."""
    me = sol.data
    names = {vg.index: vg.name.replace(M_, '') for vg in sol.vertex_groups}
    wts = [{names[g.group]: g.weight for g in v.groups} for v in me.vertices]
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    mw = sol.matrix_world
    verts = [mw @ v.co for v in bm.verts]
    tris = [[v.index for v in f.verts] for f in bm.faces]
    bm.free()
    return BVHTree.FromPolygons(verts, tris), verts, tris, wts


def bary(p, a, b, c):
    v0, v1, v2 = b - a, c - a, p - a
    d00, d01, d11 = v0.dot(v0), v0.dot(v1), v1.dot(v1)
    d20, d21 = v2.dot(v0), v2.dot(v1)
    den = d00 * d11 - d01 * d01
    if abs(den) < 1e-14:
        return 1.0, 0.0, 0.0
    v = (d11 * d20 - d01 * d21) / den
    w = (d00 * d21 - d01 * d20) / den
    return 1.0 - v - w, v, w


def build_one(nat):
    clear()
    objs = load(SOLDIER)
    rig = next(o for o in objs if o.type == 'ARMATURE')
    sol = next(o for o in objs if o.type == 'MESH')
    bvh, dverts, dtris, dwts = donor_bvh(sol)
    ref_bones = {b.name: (b.matrix_local.copy(), b.parent.name if b.parent else None) for b in rig.data.bones}
    sol_min = min(p.z for p in dverts)
    sol_max = max(p.z for p in dverts)
    sol_cx = (min(p.x for p in dverts) + max(p.x for p in dverts)) / 2

    # --- modellen (glb) til verdensrum, meter, Z op
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(SRC / MODEL_GLB[nat]))
    new = [o for o in bpy.data.objects if o not in before and o.type == 'MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in new:
        o.select_set(True)
    bpy.context.view_layer.objects.active = new[0]
    if len(new) > 1:
        bpy.ops.object.join()
    mod = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    me = mod.data
    co = [v.co.copy() for v in me.vertices]
    lo, hi = world_bounds(co)
    height = hi[2] - lo[2]
    # kun translation: soler paa soldatens soelniveau, midte i x som soldatens (y uroert)
    shift = Vector((sol_cx - (lo[0] + hi[0]) / 2, 0.0, sol_min - lo[2]))
    for v in me.vertices:
        v.co += shift
    me.update()
    info = {'nation': nat, 'height_m': round(height, 4), 'soldier_height_m': round(sol_max - sol_min, 4), 'shift_m': [round(x, 4) for x in shift],
            'triangles': tri_count(me), 'vertices': len(me.vertices)}

    # --- vaegt-transfer: nearest surface. Foerst tilpasses donorens arme (pose) til modellens, saa haender der haenger taet ved laarene ikke faar laarvaegte.
    def transfer(verts, tris):
        tree = BVHTree.FromPolygons(verts, tris)
        out, far_ = [], 0.0
        for v in me.vertices:
            loc, nrm, idx, dist = tree.find_nearest(v.co)
            a_, b_, c_ = (verts[i] for i in tris[idx])
            u, vv, w = bary(loc, a_, b_, c_)
            acc = {}
            for k, i in zip((u, vv, w), tris[idx]):
                for g, x in dwts[i].items():
                    acc[g] = acc.get(g, 0.0) + k * x
            out.append(acc)
            far_ = max(far_, dist)
        return out, far_

    def region_idx(wlist, name):
        return [i for i, acc in enumerate(wlist) if acc and max(acc, key=acc.get) == name and acc[name] > 0.7]

    def med(pts):
        return Vector(tuple(sorted(p[k] for p in pts)[len(pts) // 2] for k in range(3)))

    W, far = transfer(dverts, dtris)
    fit_log = {}
    fit_world = {}
    if ARM_FIT:
        dep = bpy.context.evaluated_depsgraph_get
        donor_sets = {n: region_idx(dwts, n) for n in ('LeftForeArm', 'LeftHand', 'RightForeArm', 'RightHand')}
        for it in range(ARM_FIT_ITER):
            targets = {}
            for n in donor_sets:
                idx_ = region_idx(W, n)
                targets[n] = med([me.vertices[i].co for i in idx_]) if idx_ else None
            for side in ('Left', 'Right'):
                for step in range(3):
                    for bone_name, cen_name in ((side + 'Arm', side + 'ForeArm'), (side + 'ForeArm', side + 'Hand')):
                        pb = rig.pose.bones[M_ + bone_name]
                        bpy.context.view_layer.update()
                        ev = sol.evaluated_get(dep())
                        em = ev.to_mesh()
                        cur = [ev.matrix_world @ v.co for v in em.vertices]
                        ev.to_mesh_clear()
                        cen = med([cur[i] for i in donor_sets[cen_name]])
                        J = rig.matrix_world @ pb.head
                        t = targets[cen_name]
                        if t is None:
                            continue
                        q = (cen - J).rotation_difference(t - J)
                        Rw = Matrix.Translation(J) @ q.to_matrix().to_4x4() @ Matrix.Translation(-J)
                        pb.matrix = rig.matrix_world.inverted() @ Rw @ rig.matrix_world @ pb.matrix
            bpy.context.view_layer.update()
            ev = sol.evaluated_get(dep())
            em = ev.to_mesh()
            cur = [ev.matrix_world @ v.co for v in em.vertices]
            ev.to_mesh_clear()
            W, far = transfer(cur, dtris)
            fit_log[f'iter{it}'] = {n: [round(x, 3) for x in (med([cur[i] for i in donor_sets[n]]))] for n in donor_sets}
        fit_log['targets'] = {n: ([round(x, 3) for x in t] if t else None) for n, t in targets.items()}
        bpy.context.view_layer.update()
        rwi = rig.matrix_world.inverted()
        for pb in rig.pose.bones:   # tilpasningsposens transform pr. knogle i verdensrum (donor-hvile -> modellens arme); bruges til at 'af-poserer' modellen
            fit_world[pb.name.replace(M_, '')] = rig.matrix_world @ pb.matrix @ pb.bone.matrix_local.inverted() @ rwi
        for pb in rig.pose.bones:
            pb.matrix_basis = Matrix.Identity(4)
        bpy.context.view_layer.update()
    info['arm_fit'] = fit_log
    info['max_nearest_dist_m'] = round(far, 4)

    # --- bro-flader: Hunyuan-figurerne er ét sammensvejset net; haender/underarme der hviler mod laar, pung eller baelte er svejset fast og rives ved rytterposerne
    #     (kanter 20-50 cm laengere). Foer udglatning fjernes flader med en kant mellem en underarms-/haand-vertex (Hand+ForeArm > 0,5) og en vertex med
    #     arm-andel (Arm+ForeArm+Hand) under BRIDGE (0,3): albue/haand mod krop/laar. Skulder og overarm er urort. Udglatningen gaar kun over bevarede kanter.
    def fr(acc, side):
        return sum(x for g, x in acc.items() if g.startswith(side + 'Hand') or g == side + 'ForeArm')

    def cr(acc, side):
        return fr(acc, side) + acc.get(side + 'Arm', 0.0)
    keep_polys = []
    cut = {'Left': 0, 'Right': 0}
    BR = float(os.environ.get('BRIDGE', '0.3'))
    for p_ in me.polygons:
        vs = list(p_.vertices)
        bad = None
        for side in ('Left', 'Right'):
            for ia in range(len(vs)):
                ja, jb = vs[ia], vs[(ia + 1) % len(vs)]
                if (fr(W[ja], side) > 0.5 and cr(W[jb], side) < BR) or (fr(W[jb], side) > 0.5 and cr(W[ja], side) < BR):
                    bad = side
                    break
            if bad:
                break
        if bad:
            cut[bad] += 1
        else:
            keep_polys.append(p_)
    info['bridge_faces_removed'] = cut

    # --- kantudglatning (paa sammensvejste positioner: UV-split vertices med samme position faar altid identiske vaegte, ellers reves sømmene)
    key = {}
    gid = [0] * len(me.vertices)
    for v in me.vertices:
        gid[v.index] = key.setdefault((round(v.co.x, 5), round(v.co.y, 5), round(v.co.z, 5)), len(key))
    ng = len(key)
    gW = [None] * ng
    for i, acc in enumerate(W):
        if gW[gid[i]] is None:
            gW[gid[i]] = dict(acc)
    gadj = [set() for _ in range(ng)]
    for p_ in keep_polys:
        vv = list(p_.vertices)
        for k_ in range(len(vv)):
            a_, b_ = gid[vv[k_]], gid[vv[(k_ + 1) % len(vv)]]
            if a_ != b_:
                gadj[a_].add(b_)
                gadj[b_].add(a_)
    for _ in range(SMOOTH):
        nw = []
        for i, acc in enumerate(gW):
            res = {g: 0.5 * x for g, x in acc.items()}
            nb = gadj[i]
            if nb:
                k = 0.5 / len(nb)
                for j in nb:
                    for g, x in gW[j].items():
                        res[g] = res.get(g, 0.0) + k * x
            nw.append(res)
        gW = nw
    W = [dict(gW[gid[i]]) for i in range(len(me.vertices))]

    # --- haandled efter tekstur: hud (skin) ved haanden = haandknoglen; manchet/aermer proximalt for haandleddet = underarm (ikke haand)
    import numpy as np
    img = None
    for m_ in me.materials:
        if m_ and m_.use_nodes:
            for n_ in m_.node_tree.nodes:
                if n_.type == 'TEX_IMAGE' and n_.image:
                    img = n_.image
    skin = np.zeros(len(me.vertices), dtype=bool)
    if img is not None:
        iw, ih = img.size
        px = np.empty(iw * ih * 4, dtype=np.float32)
        img.pixels.foreach_get(px)
        px = px.reshape(ih, iw, 4)
        uvl = me.uv_layers.active.data
        acc_c = np.zeros((len(me.vertices), 3), dtype=np.float64)
        cnt = np.zeros(len(me.vertices))
        for li, lp in enumerate(me.loops):
            u_, v_ = uvl[li].uv
            x_, y_ = int(min(max(u_, 0), 0.9999) * iw), int(min(max(v_, 0), 0.9999) * ih)
            acc_c[lp.vertex_index] += px[y_, x_, :3]
            cnt[lp.vertex_index] += 1
        col = acc_c / np.maximum(cnt, 1)[:, None]
        r_, g_, b_ = col[:, 0], col[:, 1], col[:, 2]
        skin = (r_ > 0.12) & (g_ / np.maximum(r_, 1e-6) > 0.38) & (g_ / np.maximum(r_, 1e-6) < 0.88) & (b_ / np.maximum(r_, 1e-6) > 0.22) & (b_ / np.maximum(r_, 1e-6) < 0.85)
    wrist_log = {}
    for side in ('Left', 'Right'):
        fi, hi_ = region_idx(W, side + 'ForeArm'), region_idx(W, side + 'Hand')
        if not fi or not hi_:
            continue
        cf = med([me.vertices[i].co for i in fi])
        ch = med([me.vertices[i].co for i in hi_])
        ax = (ch - cf).normalized()
        sg = 1.0 if side == 'Left' else -1.0
        T, LAT, DH = {}, {}, {}
        for v in me.vertices:
            d_ = v.co - cf
            t_ = d_.dot(ax)
            T[v.index] = t_
            LAT[v.index] = (d_ - ax * t_).length
            DH[v.index] = (v.co - ch).length
        t_h = (ch - cf).dot(ax)
        sk = [T[i] for i in range(len(me.vertices)) if skin[i] and LAT[i] < 0.08 and DH[i] < 0.25 and sg * me.vertices[i].co.x > 0.05 and T[i] > t_h - 0.16]
        if len(sk) < 50:
            continue
        sk.sort()
        t_w = min(max(sk[int(len(sk) * WRIST_PCT)], t_h - 0.10), t_h - 0.05)   # hudens start, begraenset til 5-10 cm foer haandens tyngdepunkt (stoej i tekstur)
        pj = rig.matrix_world @ rig.data.bones[M_ + side + 'Hand'].head_local
        t_p = (pj - cf).dot(ax)
        lat_p = (pj - cf - ax * t_p).length
        t_b = min(max(t_p, t_w + 0.025), t_w + 0.06)   # midten af haandled-overgangen: ved soldatens haandled-led (mindst hudens start + 2,5 cm), ellers rives vaegtgraensen ved bukning
        n_cuff = n_hand = 0
        for v in me.vertices:
            i = v.index
            if LAT[i] > 0.10 or DH[i] > 0.30 or sg * v.co.x < 0.05:
                continue
            t_ = T[i]
            acc = W[i]
            if t_ < t_b + 0.025:
                hw = acc.pop(side + 'Hand', 0.0)
                for g in [g for g in acc if g.startswith(side + 'HandThumb') or g.startswith(side + 'HandIndex')]:
                    hw += acc.pop(g)
                if hw > 0:
                    acc[side + 'ForeArm'] = acc.get(side + 'ForeArm', 0.0) + hw
                    n_cuff += 1
            if t_ > t_b - 0.025 and HAND_MODE == 'rigid' and DH[i] < 0.25:
                h = min(1.0, (t_ - (t_b - 0.025)) / 0.05)
                h = h * h * (3 - 2 * h)
                for g in list(acc):
                    acc[g] *= (1 - h)
                acc[side + 'Hand'] = acc.get(side + 'Hand', 0.0) + h
                n_hand += 1
        wrist_log[side] = {'skin_t_percentiles_m': [round(sk[int(len(sk) * q)], 3) for q in (0.02, 0.05, 0.1, 0.2, 0.3, 0.5)], 'wrist_t_m': round(t_w, 3), 'pivot_t_m': round(t_p, 3), 'blend_centre_t_m': round(t_b, 3), 'pivot_lateral_m': round(lat_p, 3), 'hand_t_m': round(t_h, 3), 'skin_samples': len(sk), 'cuff_vertices_moved_to_forearm': n_cuff, 'hand_vertices': n_hand}
    info['wrist'] = wrist_log
    info['skin_vertices'] = int(skin.sum())

    # --- haender: kun haandknoglen (vanter), eller behold fingervaegte
    hand_fix = 0
    if HAND_MODE == 'rigid':
        for acc in W:
            for side in ('Left', 'Right'):
                fingers = [g for g in acc if g.startswith(side + 'HandThumb') or g.startswith(side + 'HandIndex')]
                if fingers:
                    s = sum(acc.pop(g) for g in fingers)
                    acc[side + 'Hand'] = acc.get(side + 'Hand', 0.0) + s
                    hand_fix += 1
    info['hand_mode'] = HAND_MODE
    info['hand_vertices_merged_to_hand_bone'] = hand_fix

    # --- normaliser, maks. 4
    nz = 0
    for acc in W:
        items = sorted(acc.items(), key=lambda kv: -kv[1])[:4]
        s = sum(x for _, x in items)
        if s <= 1e-9:
            items, s = [('Hips', 1.0)], 1.0
            nz += 1
        acc.clear()
        acc.update({g: x / s for g, x in items if x / s > 0.005})
        s2 = sum(acc.values())
        for g in list(acc):
            acc[g] /= s2
    info['vertices_without_weights_fallback_hips'] = nz

    # --- af-poser: modellens arme/haender flyttes fra deres A-pose til soldatens hvilearme (inverse af tilpasningsposen, vaegtet), saa haand/underarm ligger paa soldatens knogler
    #     (ellers sidder haanden op til 14 cm ved siden af RightHand-knoglen, og saebelen i haanden flyder). UNPOSE=0 slaar det fra.
    import numpy as np
    unp = {}
    if fit_world and os.environ.get('UNPOSE', '1') == '1':
        Fi = {g: np.array(m.inverted()) for g, m in fit_world.items() if np.abs(np.array(m) - np.eye(4)).max() > 1e-6}
        moved = 0.0
        for i, v in enumerate(me.vertices):
            ws = {g: x for g, x in W[i].items() if g in Fi}
            if not ws:
                continue
            tot = sum(W[i].values())
            M = np.eye(4) * (1.0 - sum(ws.values()) / tot)
            for g, x in ws.items():
                M = M + Fi[g] * (x / tot)
            q = M @ np.array([v.co.x, v.co.y, v.co.z, 1.0])
            nv = Vector((q[0], q[1], q[2]))
            moved = max(moved, (nv - v.co).length)
            v.co = nv
        me.update()
        info['unpose_max_vertex_move_cm'] = round(moved * 100, 1)
        info['unpose_bones'] = sorted(Fi)
    # --- rytter-kontrol af vaegte (hvile): hat/hoved (z > 0,80 m = over hagen), hoejre haand
    hz = [i for i, v in enumerate(me.vertices) if v.co.z > 0.80]
    head_ok = [i for i in hz if W[i].get('Head', 0) + W[i].get('HeadTop_End', 0) > 0.95]
    bad_z = [round(me.vertices[i].co.z, 3) for i in hz if i not in set(head_ok)]
    info['hat_region_vertices'] = len(hz)
    info['hat_region_head_weight_over_95pct'] = round(len(head_ok) / max(len(hz), 1), 4)
    info['hat_region_lowest_non_head_z_m'] = min(bad_z) if bad_z else None
    rh = region_idx(W, 'RightHand')
    if rh:
        hc = Vector((sum(me.vertices[i].co.x for i in rh) / len(rh), sum(me.vertices[i].co.y for i in rh) / len(rh), sum(me.vertices[i].co.z for i in rh) / len(rh)))
        hb = rig.matrix_world @ rig.data.bones[M_ + 'RightHand'].head_local
        info['right_hand_mesh_centroid_to_bone_cm_rest'] = round((hc - hb).length * 100, 1)
        info['right_hand_vertices'] = len(rh)
        bl = rig.data.bones[M_ + 'RightHand'].matrix_local.inverted() @ (rig.matrix_world.inverted() @ hc)
        info['right_hand_centroid_in_RightHand_bone_space_cm'] = [round(x, 2) for x in bl]
        sidx = [i for i, acc in enumerate(dwts) if sum(x for g, x in acc.items() if g.startswith('RightHand')) > 0.5]
        sc_ = Vector((sum(dverts[i].x for i in sidx) / len(sidx), sum(dverts[i].y for i in sidx) / len(sidx), sum(dverts[i].z for i in sidx) / len(sidx)))
        info['soldier_right_hand_centroid_in_bone_space_cm'] = [round(x, 2) for x in (rig.data.bones[M_ + 'RightHand'].matrix_local.inverted() @ (rig.matrix_world.inverted() @ sc_))]
    # drejning: taaerne frem? (laveste 1 % af vertices, gennemsnits-y mod soldatens taa = -y)
    low = sorted(me.vertices, key=lambda v: v.co.z)[:max(50, len(me.vertices) // 100)]
    info['feet_mean_y_m'] = round(sum(v.co.y for v in low) / len(low), 3)

    # --- ind i soldatens mesh-objekt (samme noder/transform/modifier som soldatens FBX)
    # smaa oer, som bro-snittet har skaaret fri (snore, remme), faar ellers kun et enkelt knogle-vaegt-sæt og svæver; dem under ISLAND flader droppes
    par = list(range(ng))

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    for p_ in keep_polys:
        vv = [gid[i] for i in p_.vertices]
        for k_ in vv[1:]:
            ra, rb = find(vv[0]), find(k_)
            if ra != rb:
                par[ra] = rb
    comp = {}
    for p_ in keep_polys:
        r_ = find(gid[p_.vertices[0]])
        comp[r_] = comp.get(r_, 0) + 1
    ISLAND = int(os.environ.get('ISLAND', '150'))
    small = {r_: n_ for r_, n_ in comp.items() if n_ < ISLAND}
    info['islands_dropped'] = {'components': len(small), 'faces': sum(small.values()), 'largest': max(small.values()) if small else 0, 'main_component_faces': max(comp.values())}
    keep_polys = [p_ for p_ in keep_polys if find(gid[p_.vertices[0]]) not in small]
    used = sorted({i for p_ in keep_polys for i in p_.vertices})
    remap = {old_i: new_i for new_i, old_i in enumerate(used)}
    info['faces_kept'] = len(keep_polys)
    info['vertices_kept'] = len(used)
    info['loose_vertices_dropped'] = len(me.vertices) - len(used)
    W = [W[i] for i in used]
    inv = sol.matrix_world.inverted()
    newme = bpy.data.meshes.new('SK_Cavalry_1864_' + nat)
    newme.from_pydata([inv @ me.vertices[i].co for i in used], [], [tuple(remap[i] for i in p_.vertices) for p_ in keep_polys])
    uvsrc = me.uv_layers.active
    uv = newme.uv_layers.new(name='UVMap')
    newme.update(calc_edges=True)
    # loops i from_pydata kan have anden raekkefoelge end me.loops: byg uv via polygon-loop-indeks
    for p_old, p_new in zip(keep_polys, newme.polygons):
        for lo_, ln_ in zip(p_old.loop_indices, p_new.loop_indices):
            uv.data[ln_].uv = uvsrc.data[lo_].uv
    for p in newme.polygons:
        p.use_smooth = True
    mat = me.materials[0] if me.materials else None
    newme.materials.append(mat)
    old = sol.data
    sol.data = newme
    bpy.data.meshes.remove(old)
    sol.name = 'SK_Cavalry_1864_' + nat
    for vg in list(sol.vertex_groups):
        sol.vertex_groups.remove(vg)
    for b in rig.data.bones:
        sol.vertex_groups.new(name=b.name)
    vgs = {vg.name: vg for vg in sol.vertex_groups}
    for i, acc in enumerate(W):
        for g, x in acc.items():
            vgs[M_ + g if not g.startswith(M_) else g].add([i], x, 'REPLACE')
    bpy.data.objects.remove(mod, do_unlink=True)
    # navne i soldatens FBX er uden praefiks i vertex groups ('Hips'), saa soldatens egne gruppenavne er brugt som noegler; map til knogle
    return rig, sol, info, ref_bones


def export_fbx(rig, sol, nat):
    path = OUT / f'SK_Cavalry_1864_{nat}.fbx'
    keep_prev(path)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    sol.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
                             use_mesh_modifiers=False, bake_anim=False, axis_forward='-Z', axis_up='Y')
    return path


def save_texture(sol, nat):
    img = None
    for m in sol.data.materials:
        if m and m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == 'TEX_IMAGE' and n.image:
                    img = n.image
    dst = OUT / f'SK_Cavalry_1864_{nat}_Texture_0.png'
    keep_prev(dst)
    if img is None:
        return None, None
    old = img.filepath_raw
    img.filepath_raw = str(dst)
    img.file_format = 'PNG'
    img.save()
    return str(dst.name), list(img.size)


def verify(nat, path):
    """Importer den eksporterede FBX og sammenlign skelettet med soldatens + vaegte."""
    clear()
    ref = load(SOLDIER)
    rr = next(o for o in ref if o.type == 'ARMATURE')
    refb = {b.name: (rr.matrix_world @ b.matrix_local, b.parent.name if b.parent else None) for b in rr.data.bones}
    got = load(path)
    arm = next(o for o in got if o.type == 'ARMATURE')
    mesh = next(o for o in got if o.type == 'MESH')
    res = {'bones': len(arm.data.bones), 'armature_scale': [round(x, 5) for x in arm.scale], 'armature_location': [round(x, 5) for x in arm.location]}
    mx = 0.0
    names_ok = set(refb) == {b.name for b in arm.data.bones}
    par_ok = True
    for b in arm.data.bones:
        if b.name not in refb:
            continue
        m = arm.matrix_world @ b.matrix_local
        r = refb[b.name][0]
        mx = max(mx, max(abs(m[i][j] - r[i][j]) for i in range(3) for j in range(4)))
        if (b.parent.name if b.parent else None) != refb[b.name][1]:
            par_ok = False
    res['names_match_SK_Human'] = names_ok
    res['parents_match'] = par_ok
    res['max_bind_matrix_dev'] = round(mx, 7)
    pts = [mesh.matrix_world @ v.co for v in mesh.data.vertices]
    lo, hi = world_bounds(pts)
    res['mesh_bounds_m'] = [round(x, 3) for x in lo + hi]
    res['mesh_triangles'] = tri_count(mesh.data)
    mi = 0
    bad = 0
    for v in mesh.data.vertices:
        mi = max(mi, len(v.groups))
        s = sum(g.weight for g in v.groups)
        if abs(s - 1.0) > 1e-3:
            bad += 1
    res['max_influences'] = mi
    res['vertices_weight_sum_not_1'] = bad
    res['vertex_groups'] = len(mesh.vertex_groups)
    res['mesh_object_parent'] = mesh.parent.name if mesh.parent else None
    res['modifiers'] = [m.type for m in mesh.modifiers]
    return res


def build():
    report = {}
    for nat in NATIONS:
        rig, sol, info, _ = build_one(nat)
        tex, size = save_texture(sol, nat)
        path = export_fbx(rig, sol, nat)
        info['texture'] = tex
        info['texture_size'] = size
        info['fbx'] = path.name
        info['verify'] = verify(nat, path)
        report[nat] = info
        print('BUILT', nat, json.dumps(info))
    rp = OUT / 'report.json'
    old = json.loads(rp.read_text(encoding='utf-8')) if rp.exists() else {}
    keep_prev(rp)
    old.update(report)
    rp.write_text(json.dumps(old, indent=2), encoding='utf-8')


# ------------------------------------------------------------------ preview
def depth(pb):
    d = 0
    while pb.parent:
        pb = pb.parent
        d += 1
    return d


def clip_pose(rig, clip_arm, frame):
    """Overfoer en klips pose (lokal rotation relativt til foraelder) til rig; Hips = rig-hvile + klippets forskydning."""
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()
    order = sorted(rig.pose.bones, key=depth)
    cp = {pb.name: pb for pb in clip_arm.pose.bones}
    Pw = {}
    for pb in order:
        src = cp.get(pb.name)
        db = pb.bone
        rest_loc = (db.parent.matrix_local.inverted() @ db.matrix_local) if db.parent else db.matrix_local
        if src is None:
            pb.matrix_basis = Matrix.Identity(4)
            continue
        sb = src.bone
        if src.parent:
            s_loc = src.parent.matrix.inverted() @ src.matrix
            s_rest = sb.parent.matrix_local.inverted() @ sb.matrix_local
        else:
            s_loc = src.matrix
            s_rest = sb.matrix_local
        # delta i lokal bone-frame: rest^-1 @ pose
        # Rytterklippenes bindepose er sædet, ikke soldatens: som i Unreal (spor = absolut lokal transform) bruges klippets lokale transform
        # direkte, udtrykt relativt til MODELLENS bindepose. (For klip med soldatens bindepose er dette det samme som s_rest^-1 @ s_loc.)
        delta = rest_loc.inverted() @ s_loc
        basis = Matrix.Identity(4)
        basis = Quaternion(delta.to_quaternion()).to_matrix().to_4x4()
        if not src.parent:
            basis.translation = delta.translation
        pb.matrix_basis = basis
    bpy.context.view_layer.update()


def setup_render(scene):
    if RENDER == 'workbench':
        scene.render.engine = 'BLENDER_WORKBENCH'
        scene.display.shading.light = 'STUDIO'
        scene.display.shading.color_type = 'TEXTURE'
    else:
        scene.render.engine = 'CYCLES'
        scene.cycles.device = 'CPU'
        scene.cycles.samples = SAMPLES
        scene.cycles.use_denoising = False
    scene.render.resolution_x = int(360 * float(os.environ.get('RES_SCALE', '1')))
    scene.render.resolution_y = int(520 * float(os.environ.get('RES_SCALE', '1')))
    scene.render.image_settings.file_format = 'PNG'
    w = bpy.data.worlds.new('W')
    w.use_nodes = True
    bg = w.node_tree.nodes['Background']
    bg.inputs[0].default_value = (0.62, 0.64, 0.68, 1)
    bg.inputs[1].default_value = 1.0
    scene.world = w
    sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN'))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
    scene.collection.objects.link(sun)


def render_view(scene, cam, centre, az_deg, ortho, tag, out):
    az = math.radians(az_deg)
    cam.location = centre + Vector((math.sin(az) * 6, -math.cos(az) * 6, 0.3))
    cam.rotation_euler = (centre - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = ortho
    scene.render.filepath = str(out)
    bpy.ops.render.render(write_still=True)


BASELINE = os.environ.get('BASELINE') == '1'   # maal samme kantstraekning paa soldatens eget mesh (reference for, hvad der er normalt)


def preview():
    for nat in (['SOLDIER'] if BASELINE else NATIONS):
        clear()
        objs = load(SOLDIER if BASELINE else OUT / f'SK_Cavalry_1864_{nat}.fbx')
        rig = next(o for o in objs if o.type == 'ARMATURE')
        mesh = next(o for o in objs if o.type == 'MESH')
        # tekstur fra siden af
        tex = OUT / f'SK_Cavalry_1864_{nat}_Texture_0.png'
        for m in ([] if BASELINE else mesh.data.materials):
            m.use_nodes = True
            nt = m.node_tree
            for n in list(nt.nodes):
                if n.type == 'TEX_IMAGE':
                    nt.nodes.remove(n)
            bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
            ti = nt.nodes.new('ShaderNodeTexImage')
            ti.image = bpy.data.images.load(str(tex))
            nt.links.new(ti.outputs['Color'], bsdf.inputs['Base Color'])
            bsdf.inputs['Roughness'].default_value = 0.7
        for pb in rig.pose.bones:
            pb.rotation_mode = 'QUATERNION'
        import numpy as np
        vg_name = {vg.index: vg.name.replace(M_, '') for vg in mesh.vertex_groups}
        mw0 = mesh.matrix_world
        rh_idx = np.array([v.index for v in mesh.data.vertices if sum(g.weight for g in v.groups if vg_name[g.group] == 'RightHand') > 0.5], dtype=np.int64)
        lh_idx = np.array([v.index for v in mesh.data.vertices if sum(g.weight for g in v.groups if vg_name[g.group] == 'LeftHand') > 0.5], dtype=np.int64)
        hat_idx = np.array([v.index for v in mesh.data.vertices if (mw0 @ v.co).z > 0.80], dtype=np.int64)
        if BASELINE:
            rh_idx = None

        def hat_expect(rg, rest_pts):
            """Hvor hat-vertices ville vaere, hvis de sad fast paa Head-knoglen (rigid): pose-matrix * bind-matrix^-1."""
            pb_ = rg.pose.bones[M_ + 'Head']
            T = rg.matrix_world @ pb_.matrix @ pb_.bone.matrix_local.inverted() @ rg.matrix_world.inverted()
            Tn = np.array(T)
            return (np.c_[rest_pts, np.ones(len(rest_pts))] @ Tn.T)[:, :3]
        scene = bpy.context.scene
        setup_render(scene)
        cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam'))
        cam.data.type = 'ORTHO'
        scene.collection.objects.link(cam)
        scene.camera = cam
        shots = [('Hvile', None, None)]
        clip_arms = {}
        for name, (rel, frames) in CLIPS.items():
            new = load(PROJECT / rel)
            carm = next(o for o in new if o.type == 'ARMATURE')
            act = carm.animation_data.action
            try:
                carm.animation_data.action_slot = act.slots[0]   # Blender 5: FBX 7700-klip faar ellers ikke deres action-slot
            except Exception:
                pass
            for o in new:
                if o.type == 'MESH':
                    o.hide_render = True
                    o.hide_viewport = True
            carm.hide_render = True
            clip_arms[name] = carm
            for f in frames:
                shots.append((f'{name}{f}', name, f))
        import numpy as np
        rest_pos = None
        metrics, zooms = {}, []
        tiles = []
        for tag, name, f in shots:
            if name:
                clip_pose(rig, clip_arms[name], f)
            else:
                for pb in rig.pose.bones:
                    pb.matrix_basis = Matrix.Identity(4)
                bpy.context.view_layer.update()
            bone_dev = None
            if name:
                ca_ = clip_arms[name]
                devs = [((rig.matrix_world @ pb.head) - (ca_.matrix_world @ ca_.pose.bones[pb.name].head)).length for pb in rig.pose.bones if pb.name in ca_.pose.bones]
                bone_dev = round(max(devs) * 100, 2)
            ev = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
            me = ev.to_mesh()
            pts = [ev.matrix_world @ v.co for v in me.vertices]
            ev.to_mesh_clear()
            lo, hi = world_bounds(pts)
            arr = np.array([tuple(p_) for p_ in pts], dtype=np.float32)
            if rest_pos is None:
                rest_pos = arr
                ed = np.array([tuple(e.vertices) for e in mesh.data.edges], dtype=np.int64)
                rest_len = np.linalg.norm(rest_pos[ed[:, 0]] - rest_pos[ed[:, 1]], axis=1) + 1e-6
            else:
                ln = np.linalg.norm(arr[ed[:, 0]] - arr[ed[:, 1]], axis=1)
                ratio = ln / rest_len
                metrics[tag] = {'edge_stretch_max': round(float(ratio.max()), 2), 'edge_stretch_p9999': round(float(np.percentile(ratio, 99.99)), 2),
                                'edges_over_2x': int((ratio > 2.0).sum()), 'edge_growth_max_cm': round(float((ln - rest_len).max()) * 100, 1),
                                'edges_growing_over_2cm': int(((ln - rest_len) > 0.02).sum()),
                                'worst_edge_rest_mid_m': [round(float(x), 3) for x in (rest_pos[ed[int((ln - rest_len).argmax()), 0]] + rest_pos[ed[int((ln - rest_len).argmax()), 1]]) / 2],
                                'worst_edge_rest_len_cm': round(float(rest_len[int((ln - rest_len).argmax())]) * 100, 2), 'bounds_m': [round(x, 2) for x in lo + hi]}
            if rh_idx is not None and len(rh_idx):
                hc = arr[rh_idx].mean(axis=0)
                hb = rig.matrix_world @ rig.pose.bones[M_ + 'RightHand'].head
                hd = {'right_hand_mesh_to_bone_cm': round(float(np.linalg.norm(hc - np.array(tuple(hb)))) * 100, 1)}
                lh_ = np.array(tuple(rig.matrix_world @ rig.pose.bones[M_ + 'LeftHand'].head))
                hd['left_hand_mesh_to_bone_cm'] = round(float(np.linalg.norm(arr[lh_idx].mean(axis=0) - lh_)) * 100, 1)
                hd['hat_max_dev_from_head_bone_cm'] = round(float(np.linalg.norm(arr[hat_idx] - hat_expect(rig, rest_pos[hat_idx]), axis=1).max()) * 100, 1) if len(hat_idx) else None
                hd['bone_head_max_dev_vs_clip_cm'] = bone_dev
                if tag in metrics:
                    metrics[tag].update(hd)
                else:
                    metrics['rest_hand'] = hd
            if os.environ.get('DIAG') and tag == 'Gallop8':
                order_ = np.argsort(-(ln - rest_len))[:int(os.environ.get('DIAG'))]
                for k_ in order_:
                    a_, b_ = int(ed[k_, 0]), int(ed[k_, 1])
                    wa = {vg_name[g.group]: round(g.weight, 2) for g in mesh.data.vertices[a_].groups}
                    wb = {vg_name[g.group]: round(g.weight, 2) for g in mesh.data.vertices[b_].groups}
                    print('DIAG', round(float((ln - rest_len)[k_]) * 100, 1), 'cm', [round(float(x), 3) for x in rest_pos[a_]], wa, [round(float(x), 3) for x in rest_pos[b_]], wb)
            if bone_dev is not None and tag in metrics:
                metrics[tag]['bone_head_max_dev_vs_clip_cm'] = bone_dev
            centre = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2))
            ortho = max(hi[2] - lo[2], (hi[0] - lo[0]) * 1.3) * 1.12
            for ztag, zaz, zone, zortho in ZOOM:
                if ztag != tag:
                    continue
                rx, ry = scene.render.resolution_x, scene.render.resolution_y
                scene.render.resolution_x = scene.render.resolution_y = 600
                if zone == 'top':
                    zc = Vector((centre.x, centre.y, hi[2] - 0.50))
                else:
                    hp = rig.matrix_world @ rig.pose.bones[M_ + 'Hips'].head
                    zc = Vector((centre.x, centre.y, hp.z - 0.10))
                zp = PREV / f'_zoom_{nat}_{tag}_{zaz}_{zone}.png'
                render_view(scene, cam, zc, zaz, zortho, tag, zp)
                zooms.append(zp)
                scene.render.resolution_x, scene.render.resolution_y = rx, ry
            for az, tg in ((0, 'F'), (90, 'S')):
                p = PREV / f'_tmp_{nat}_{tag}_{tg}.png'
                render_view(scene, cam, centre, az, ortho, tag, p)
                tiles.append((tag, tg, p))
        # kontrolark (numpy; ingen PIL i Blenders python)
        arrs = []
        for t, g, p in tiles:
            im = bpy.data.images.load(str(p))
            w, h = im.size
            a = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)
            arrs.append(a)
        cols = len(shots)
        sheet = np.ones((h * 2, w * cols, 4), dtype=np.float32) * 0.65
        for i, a in enumerate(arrs):
            c, r = i // 2, i % 2
            sheet[(1 - r) * h:(2 - r) * h, c * w:(c + 1) * w, :] = a   # blender pixels: nederste raekke foerst; F oeverst, S nederst
        out = bpy.data.images.new('sheet', w * cols, h * 2, alpha=False)
        out.pixels = sheet.ravel().tolist()
        out.filepath_raw = str(PREV / f'Cavalry_{nat}_sheet.png')
        out.file_format = 'PNG'
        out.save()
        for _, _, p in tiles:
            os.remove(p)
        # zoom-ark (overkrop: haender, krave, skuldre)
        zi = [bpy.data.images.load(str(p)) for p in zooms]
        zw, zh = zi[0].size
        za = np.concatenate([np.array(i.pixels[:], dtype=np.float32).reshape(zh, zw, 4) for i in zi], axis=1)
        zo = bpy.data.images.new('zoom', za.shape[1], za.shape[0], alpha=False)
        zo.pixels = za.ravel().tolist()
        zo.filepath_raw = str(PREV / f'Cavalry_{nat}_zoom.png')
        zo.file_format = 'PNG'
        zo.save()
        for p in zooms:
            os.remove(p)
        rp = OUT / 'report.json'
        rep = json.loads(rp.read_text(encoding='utf-8')) if rp.exists() else {}
        rep.setdefault('soldier_baseline' if BASELINE else nat, {})['preview_metrics'] = metrics
        rp.write_text(json.dumps(rep, indent=2), encoding='utf-8')
        print('PREVIEW', nat, shots)


if MODE in ('build', 'all'):
    build()
if MODE in ('preview', 'all'):
    preview()
