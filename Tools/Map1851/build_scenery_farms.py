"""Builds six more Danish/North-German farm types for the campaign map, reusing the helpers and the shared atlas of build_scenery_models.py.

Run headless:
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --python Tools/Map1851/build_scenery_farms.py

Output in SourceAssets/Campaign1851/Scenery/DK/: SM_Farm_<Type>.fbx (x6), SM_Farm_<Type>_Iso.png (renders),
scenery_farms.blend, T_SceneryDK_Atlas.png (cell (2,3) is now "timber_brick"; the other 15 cells are unchanged, so
the UVs of the eight earlier models still hold). Existing files are copied to _backup_<time> first.
Same conventions as build_scenery_models.py: 1 Blender unit = 1 m, FBX in cm, pivot at ground centre, +X = facade, Z up.
All dimensions are ESTIMATES from the approved reference images, not measured buildings.
"""
import math
import os
import shutil
import sys
import time

REPO = "C:/Projekter/1864-Campaign"
sys.path.insert(0, REPO + "/Tools/Map1851")

import bpy
import numpy as np
from mathutils import Vector

import build_scenery_models as B
from build_scenery_models import (Model, V, Z, X, Y, FULL, WIN, ARCH, WOOD, STONE, deco, wall_run, obox, fit_uv,
                                  thatch_ridge, chimney)

OUT = B.OUT
REFDIR = REPO + "/Reference/Campaign1851/Scenery/DK"
SHEET = REPO + "/Saved/Codex/scenery_dk_farm_models_sheet.png"


# ---------------------------------------------------------------- helpers
def v3(p):
    return V((p[0], p[1], p[2] if len(p) > 2 else 0.0))


def edge_frame(A, Bp):
    A, Bp = v3(A), v3(Bp)
    d = Bp - A
    L = d.length
    d = d / L
    return (A + Bp) / 2, V((d.y, -d.x, 0)), L        # outward normal for a CCW polygon; right_of(n) == d


def dec(m, o, n, s, z, w, h, tile):
    if tile == "window":
        deco(m, o, n, s, z, w, h, "window", WIN)
    elif tile == "arch":
        deco(m, o, n, s, z, w, h, "window", ARCH)
    elif tile == "peat":
        deco(m, o, n, s, z, w, h, "wood_stone", WOOD)
    else:
        deco(m, o, n, s, z, w, h, tile, FULL)


def put(m, A, Bp, items):
    o, n, L = edge_frame(A, Bp)
    for (s, z, w, h, tile) in items:
        dec(m, o, n, s - L / 2, z, w, h, tile)


def step(m, A, Bp, s, w=1.8, depth=0.9):
    o, n, L = edge_frame(A, Bp)
    c = o + n * (depth / 2) + (n.cross(Z)) * 0
    d = V((-n.y, n.x, 0))
    c = o + d * (s - L / 2) + n * (depth / 2)
    half = (depth / 2, w / 2, 0.12) if abs(n.x) > 0.5 else (w / 2, depth / 2, 0.12)
    obox(m, (c.x, c.y, 0.12), half, "wood_stone", STONE)


W = lambda s, z=1.7: (s, z, 0.8, 1.0, "window")
AR = lambda s: (s, 1.7, 0.45, 1.0, "arch")
G = lambda s, w=1.2, h=2.3: (s, 0.1 + h / 2, w, h, "door_green")
DO = lambda s, w=2.0, h=2.4: (s, 0.1 + h / 2, w, h, "door_ox")


def walls(m, poly, tile, H, seg=3.2, ph=0.0, gable_edges=(), zR=0.0, tri_tile=None, tri_v=(0.5, 1.0)):
    n = len(poly)
    for i in range(n):
        A, Bp = poly[i], poly[(i + 1) % n]
        o, nn, L = edge_frame(A, Bp)
        if ph:
            wall_run(m, o, nn, L, 0, ph, "wood_stone", seg=3.0, sub_=STONE)
            wall_run(m, o, nn, L, ph, H, tile, seg=seg)
        else:
            wall_run(m, o, nn, L, 0, H, tile, seg=seg)
        if i in gable_edges:
            a3, b3 = v3(A), v3(Bp)
            m.add([a3 + Z * H, b3 + Z * H, o + Z * zR], tri_tile or tile, FULL,
                  [(0, tri_v[0]), (1, tri_v[0]), (0.5, tri_v[1])], hint=nn)


def thatch_slope(m, pts, ref):
    ES, EE, RE, RS = pts
    n = max(1, int(math.ceil(max((EE - ES).length, (RE - RS).length) / 10.0 - 1e-6)))
    dd = (EE - ES).normalized()
    upv = RS - ES
    upv = (upv - dd * upv.dot(dd)).normalized()
    for i in range(n):
        t0, t1 = i / n, (i + 1) / n
        q = [ES + (EE - ES) * t0, ES + (EE - ES) * t1, RS + (RE - RS) * t1, RS + (RE - RS) * t0]
        m.add(q, "thatch", FULL, fit_uv(q, dd, upv, ref[0], ref[1]), hint=(0, 0, 1), smooth=True)


def eave_strip(m, pa, pb, hint):
    q = [pa - Z * 0.34, pb - Z * 0.34, pb, pa]
    m.add(q, "thatch", FULL, [(0, 0), (1, 0), (1, .15), (0, .15)], hint=hint)


def wing_roof(m, P0, P1, a, H, zR, oe, e0, e1, og=0.45, ref=(9.0, 6.0), ridge=(0.42, 0.4)):
    """Thatched roof over one wing (centre line P0->P1, half width a). End types: 'hip', 'gable' or ('c', inner_side)
    for a mitred corner with a neighbouring wing (inner_side = +1 left / -1 right of the wing direction)."""
    P0, P1 = v3(P0), v3(P1)
    d = (P1 - P0).normalized()
    l = Z.cross(d)
    s = (zR - H) / a
    ze = H - s * oe
    A = a + oe
    R, E = [], []
    for P, e, typ in ((P0, -d, e0), (P1, d, e1)):
        Ek = {}
        if typ == "hip":
            r = P - e * a
            for sg in (1, -1):
                Ek[sg] = P + e * oe + l * sg * A
        elif typ == "gable":
            r = P + e * og
            for sg in (1, -1):
                Ek[sg] = P + e * og + l * sg * A
        else:
            r = P
            for sg in (1, -1):
                Ek[sg] = P + e * A * (-1 if sg == typ[1] else 1) + l * sg * A
        R.append(r + Z * zR)
        E.append({k: p + Z * ze for k, p in Ek.items()})
    for sg in (1, -1):
        thatch_slope(m, [E[0][sg], E[1][sg], R[1], R[0]], ref)
        eave_strip(m, E[0][sg], E[1][sg], l * sg)
    for k, (typ, e) in enumerate(((e0, -d), (e1, d))):
        if typ == "hip":
            pts = [E[k][1], E[k][-1], R[k]]
            mid = (E[k][1] + E[k][-1]) / 2
            upv = R[k] - mid
            upv = (upv - l * upv.dot(l)).normalized()
            m.add(pts, "thatch", FULL, fit_uv(pts, l, upv, ref[0], ref[1]), hint=(0, 0, 1), smooth=True)
            eave_strip(m, E[k][1], E[k][-1], e) if k == 1 else eave_strip(m, E[k][-1], E[k][1], e)
    thatch_ridge(m, R[0], R[1], ridge[0], ridge[1])
    return ze


def cobbles(m, x0, x1, y0, y1, chunk=8.0):
    nx = max(1, int(round((x1 - x0) / chunk)))
    ny = max(1, int(round((y1 - y0) / chunk)))
    for i in range(nx):
        for j in range(ny):
            xa, xb = x0 + (x1 - x0) * i / nx, x0 + (x1 - x0) * (i + 1) / nx
            ya, yb = y0 + (y1 - y0) * j / ny, y0 + (y1 - y0) * (j + 1) / ny
            m.add([V((xa, ya, .04)), V((xb, ya, .04)), V((xb, yb, .04)), V((xa, yb, .04))], "cobbles", FULL,
                  [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0, 1))


def shift(m, dx, dy):
    d = V((dx, dy, 0))
    m.polys = [([p + d for p in pts], fuv, sm) for pts, fuv, sm in m.polys]


# common dimensions (metres, estimates)
WA = 3.5                  # half width of an ordinary wing (7 m)
HW, RISE, OE = 2.7, 3.4, 0.55
ZR = HW + RISE


# ---------------------------------------------------------------- models
def build_trelaenget():
    m = Model("SM_Farm_Trelaenget")
    poly = [(-12, -14), (12, -14), (12, -7), (-5, -7), (-5, 7), (12, 7), (12, 14), (-12, 14)]
    walls(m, poly, "timber_white", HW, seg=3.2)
    wing_roof(m, (12, -10.5), (-8.5, -10.5), WA, HW, ZR, OE, "hip", ("c", -1))
    wing_roof(m, (-8.5, -10.5), (-8.5, 10.5), WA, HW, ZR, OE, ("c", -1), ("c", -1))
    wing_roof(m, (-8.5, 10.5), (12, 10.5), WA, HW, ZR, OE, ("c", -1), "hip")
    P = poly
    put(m, P[0], P[1], [AR(4), DO(12), AR(20)])
    put(m, P[1], P[2], [W(3.5)])
    put(m, P[2], P[3], [DO(3.0), AR(6.0), DO(9.5), AR(12.5), W(15.5)])
    put(m, P[3], P[4], [W(2.2), W(4.6), G(7.0), W(9.4), W(11.8)])
    step(m, P[3], P[4], 7.0)
    put(m, P[4], P[5], [W(2.5), DO(6.0), AR(9.5), DO(13.0), AR(16.0)])
    put(m, P[5], P[6], [W(3.5)])
    put(m, P[6], P[7], [AR(4), DO(12), AR(20)])
    put(m, P[7], P[0], [W(4), W(8), W(12), W(16), W(20), W(24)])
    chimney(m, -8.5, 3.0, ZR - 1.0, ZR + 1.4, 1.0, 1.0, tile="white")
    cobbles(m, -5, 12, -7, 7)
    return m


def build_tolaenget():
    m = Model("SM_Farm_Tolaenget")
    poly = [(0, 0), (7, 0), (7, 19), (19, 19), (19, 26), (0, 26)]
    walls(m, poly, "timber_white", HW, seg=3.2)
    wing_roof(m, (3.5, 0), (3.5, 22.5), WA, HW, ZR, OE, "hip", ("c", -1))
    wing_roof(m, (3.5, 22.5), (19, 22.5), WA, HW, ZR, OE, ("c", -1), "hip")
    P = poly
    put(m, P[0], P[1], [W(3.5)])
    put(m, P[1], P[2], [W(2.0), W(4.6), G(8.0), W(11.2), W(14.0), W(16.8)])
    step(m, P[1], P[2], 8.0)
    put(m, P[2], P[3], [DO(2.8), AR(5.2), DO(8.0), AR(10.6)])
    put(m, P[3], P[4], [W(3.5)])
    put(m, P[4], P[5], [AR(4), DO(10), W(15)])
    put(m, P[5], P[0], [W(4), W(9), W(14), W(19), W(23)])
    chimney(m, 3.5, 7.0, ZR - 1.0, ZR + 1.4, 1.0, 1.0, tile="white")
    cobbles(m, 7, 19, 0, 19)
    shift(m, -9.5, -13.0)
    return m


def build_parallel():
    m = Model("SM_Farm_Parallel")
    dw = [(-12, -13), (-5, -13), (-5, 13), (-12, 13)]
    st = [(4, -11), (11, -11), (11, 11), (4, 11)]
    walls(m, dw, "timber_white", HW, seg=3.2)
    walls(m, st, "timber_white", HW, seg=3.2, gable_edges=(0, 2), zR=ZR)
    wing_roof(m, (-8.5, -13), (-8.5, 13), WA, HW, ZR, OE, "hip", "hip")
    wing_roof(m, (7.5, -11), (7.5, 11), WA, HW, ZR, OE, "gable", "gable")
    put(m, dw[0], dw[1], [W(3.5)])
    put(m, dw[1], dw[2], [W(2.2), W(5.2), W(8.2), G(13.0), W(17.8), W(20.8), W(23.8)])
    step(m, dw[1], dw[2], 13.0)
    put(m, dw[2], dw[3], [W(3.5)])
    put(m, dw[3], dw[0], [W(4), W(8), W(12), W(16), W(20), W(24)])
    put(m, st[0], st[1], [W(3.5, 1.8)])
    put(m, st[1], st[2], [AR(4), DO(11), AR(18)])
    put(m, st[2], st[3], [W(3.5, 1.8)])
    put(m, st[3], st[0], [DO(3.5), AR(7.0), DO(11.0), AR(14.5), DO(18.5)])
    chimney(m, -8.5, 3.0, ZR - 1.0, ZR + 1.4, 1.0, 1.0, tile="white")
    cobbles(m, -5, 4, -13, 13)
    return m


def build_langhus():
    m = Model("SM_Farm_VestjyskLanghus")
    a, Lh = 3.25, 19.0
    H, rise = 2.3, 3.2
    zR = H + rise
    poly = [(-a, -Lh), (a, -Lh), (a, Lh), (-a, Lh)]
    walls(m, poly, "brick", H, seg=3.2, ph=0.55, gable_edges=(0, 2), zR=zR, tri_tile="tar", tri_v=(0.1, 0.85))
    wing_roof(m, (0, -Lh), (0, Lh), a, H, zR, 0.5, "gable", "gable", og=0.45)
    # front (+X): dwelling at -Y, byre in the middle, barn at +Y; s runs along +Y from the dwelling end
    put(m, poly[1], poly[2], [W(2.2), W(4.8), G(7.4), W(10.2), W(12.6), G(16.5, 2.4, 2.2),
                              (21.0, 1.7, 0.5, 0.6, "arch"), G(24.5), (28.0, 1.7, 0.7, 0.9, "door_green"),
                              DO(31.5, 3.2, 2.4), (36.3, 1.45, 3.4, 1.7, "peat")])
    step(m, poly[1], poly[2], 7.4)
    put(m, poly[3], poly[0], [W(2.5), W(6.0), AR(14), DO(30, 2.6, 2.3), W(35.5)])
    put(m, poly[0], poly[1], [W(3.25, 1.6)])
    chimney(m, 0.0, -Lh + 5.5, zR - 0.8, zR + 1.3, 1.0, 1.0, tile="white")
    return m


def build_haubarg():
    m = Model("SM_Farm_Haubarg")
    a, hl = 12.0, 14.0
    H, rise, oe = 3.0, 9.4, 0.8
    zR = H + rise
    poly = [(-a, -hl), (a, -hl), (a, hl), (-a, hl)]
    walls(m, poly, "brick", H, seg=3.2, ph=0.6)
    wing_roof(m, (0, -hl), (0, hl), a, H, zR, oe, "hip", "hip", ref=(12.0, 10.0), ridge=(0.6, 0.55))
    # small entrance gable in front (x = +a), off-centre
    yc, hw_, xf, Hp, zp = -5.0, 2.8, a + 1.4, 3.6, 5.9
    s = rise / a
    xb = (zR - (zp + 0.15)) / s
    xe = (zR - Hp) / s
    brick_poly = [V((xf, yc - hw_, 0)), V((xf, yc + hw_, 0)), V((xf, yc + hw_, Hp)), V((xf, yc, zp)), V((xf, yc - hw_, Hp))]
    m.add(brick_poly, "brick", FULL, [(0, 0), (1, 0), (1, .75), (.5, 1), (0, .75)], hint=(1, 0, 0))
    for sg in (1, -1):
        y = yc + sg * hw_
        m.add([V((a - 0.2, y, 0)), V((xf, y, 0)), V((xf, y, Hp)), V((xe, y, Hp))], "brick", FULL, [(0, 0), (1, 0), (1, .75), (0, .75)], hint=(0, sg, 0))
        q = [V((xf + 0.35, yc + sg * (hw_ + 0.35), Hp - 0.1)), V((xf + 0.35, yc, zp + 0.1)), V((xb, yc, zp + 0.1)), V((xe, yc + sg * (hw_ + 0.35), Hp - 0.1))]
        m.add(q, "thatch", FULL, [(0, 0), (0, 1), (1, 1), (1, 0)] if sg > 0 else [(1, 0), (1, 1), (0, 1), (0, 0)], hint=(0.5, sg, 1), smooth=True)
    thatch_ridge(m, V((xf + 0.35, yc, zp + 0.1)), V((xb, yc, zp + 0.1)), 0.3, 0.3)
    dec(m, V((xf, yc, 0)), V((1, 0, 0)), 0, 0.1 + 1.5, 2.8, 3.0, "door_green")
    dec(m, V((xf, yc, 0)), V((1, 0, 0)), 0, 4.0, 0.8, 1.0, "window")
    P = poly
    put(m, P[0], P[1], [W(5, 1.6), W(9, 1.6), W(15, 1.6), W(19, 1.6)])
    put(m, P[1], P[2], [W(2.2, 1.6), W(5.0, 1.6), (15.5, 1.6, 0.8, 1.0, "window"), W(18.4, 1.6), W(21.2, 1.6), W(24.0, 1.6)])
    put(m, P[2], P[3], [W(3, 1.6), W(7, 1.6), G(11), W(15, 1.6), W(19, 1.6), G(21.5)])
    put(m, P[3], P[0], [W(4, 1.6), W(9, 1.6), W(14, 1.6), W(19, 1.6), W(24, 1.6)])
    return m


def build_niedersachsen():
    m = Model("SM_Farm_Niedersachsen")
    a, hl = 6.5, 14.0
    H, rise, oe, og = 3.6, 5.7, 0.5, 0.5
    zR = H + rise
    poly = [(-hl, -a), (hl, -a), (hl, a), (-hl, a)]
    walls(m, poly, "timber_brick", H, seg=3.2, gable_edges=(1, 3), zR=zR, tri_v=(0.5, 1.0))
    # hall house axis along X, entrance gable towards +X
    wing_roof(m, (-hl, 0), (hl, 0), a, H, zR, oe, "gable", "gable", og=og, ref=(10.0, 8.0), ridge=(0.45, 0.45))
    P = poly
    put(m, P[0], P[1], [W(3.0, 1.7), W(6.5, 1.7), W(10.0, 1.7), G(14.0), W(17.5, 1.7), W(21.0, 1.7), DO(24.5, 1.6, 2.3)])
    put(m, P[2], P[3], [W(3.5, 1.7), W(7.5, 1.7), W(11.5, 1.7), W(15.5, 1.7), W(19.5, 1.7), W(23.5, 1.7)])
    put(m, P[1], P[2], [W(1.4, 1.7), (6.5, 0.1 + 2.1, 3.4, 4.2, "door_green"), W(11.6, 1.7), W(6.5, 6.0)])
    put(m, P[3], P[0], [W(3.0, 1.7), W(10.0, 1.7), W(6.5, 5.6)])
    step(m, P[0], P[1], 14.0)
    # modest crossed gable boards (Giebelzier) above the front gable apex
    for sg in (1, -1):
        dd = V((0, sg * math.sin(math.radians(35)), math.cos(math.radians(35))))
        ey = X
        ez = dd.cross(ey)
        obox(m, (hl + og + 0.05, sg * 0.4 * math.sin(math.radians(35)) * 1.0, zR + 0.45), (0.75, 0.06, 0.04), "wood_stone", WOOD, axes=(dd, ey, ez), nobottom=False)
    chimney(m, 4.0, 0.0, zR - 1.0, zR + 1.4, 1.0, 1.0, tile="white")
    return m


BUILDERS = [("Trelaenget", build_trelaenget), ("Tolaenget", build_tolaenget), ("Parallel", build_parallel),
            ("VestjyskLanghus", build_langhus), ("Haubarg", build_haubarg), ("Niedersachsen", build_niedersachsen)]
BUDGET = 500
# (target z, ortho scale, azimuth) for the Iso renders
VIEW = {"Trelaenget": (2, 44, 38), "Tolaenget": (2, 40, 38), "Parallel": (2, 42, 38),
        "VestjyskLanghus": (2, 44, 38), "Haubarg": (4, 38, 38), "Niedersachsen": (3, 36, 38)}


def main():
    stamp = time.strftime("%Y%m%d-%H%M%S")
    olds = [f for f in os.listdir(OUT) if f.lower().endswith((".fbx", ".png", ".blend", ".md")) and os.path.isfile(OUT + "/" + f)]
    bk = f"{OUT}/_backup_{stamp}"
    os.makedirs(bk)
    for f in olds:
        shutil.copy2(OUT + "/" + f, bk + "/" + f)
    old_atlas = bpy.data.images.load(bk + "/T_SceneryDK_Atlas.png")
    old_atlas.colorspace_settings.name = "Non-Color"
    old_px = np.array(old_atlas.pixels[:], dtype=np.float32).reshape(2048, 2048, 4)
    bpy.data.images.remove(old_atlas)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 1.0
    img = B.build_atlas()
    img.colorspace_settings.name = "Non-Color"
    new_px = np.array(img.pixels[:], dtype=np.float32).reshape(2048, 2048, 4)
    img.colorspace_settings.name = "sRGB"
    S = B.S
    mask = np.ones((2048, 2048), bool)
    c, r = B.TILES["timber_brick"]
    rb = 3 - r
    mask[rb * S:(rb + 1) * S, c * S:(c + 1) * S] = False
    diff = np.abs(old_px[mask] - new_px[mask]).max()
    print("ATLAS: max difference outside the new cell =", diff)
    assert diff < 1e-6, "atlas changed outside the new cell"
    mat = B.make_material(img)

    objs, stats = {}, {}
    for key, fn in BUILDERS:
        m = fn()
        ob = m.to_object(mat)
        ob.name = "SM_Farm_" + key
        objs[key] = ob
        me = ob.data
        tris = sum(len(p.vertices) - 2 for p in me.polygons)
        stats[key] = tris
    for key, ob in objs.items():
        bpy.ops.object.select_all(action="DESELECT")
        ob.select_set(True)
        bpy.context.view_layer.objects.active = ob
        bpy.ops.export_scene.fbx(
            filepath=f"{OUT}/SM_Farm_{key}.fbx", use_selection=True, object_types={"MESH"}, global_scale=1.0,
            apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", use_space_transform=True, bake_space_transform=True,
            axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", use_mesh_modifiers=True, add_leaf_bones=False,
            use_custom_props=False, path_mode="AUTO", embed_textures=False, use_tspace=False)
    print("=== STATISTIK ===")
    for key in objs:
        lo, hi, unit = B.fbx_bbox_cm(f"{OUT}/SM_Farm_{key}.fbx")
        e = hi - lo
        print(f"{key}: tris={stats[key]} (budget {BUDGET}) FBX-bbox cm X*Y*Z = {e[0]:.0f} x {e[1]:.0f} x {e[2]:.0f} "
              f"min=({lo[0]:.0f},{lo[1]:.0f},{lo[2]:.0f}) max=({hi[0]:.0f},{hi[1]:.0f},{hi[2]:.0f}) unit={unit}")

    # scene for renders
    import bmesh
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=300.0)
    gm = bpy.data.meshes.new("Ground")
    bm.to_mesh(gm)
    bm.free()
    ground = bpy.data.objects.new("PreviewGround", gm)
    sc.collection.objects.link(ground)
    gmat = bpy.data.materials.new("PreviewGrey")
    gmat.use_nodes = True
    gb = gmat.node_tree.nodes["Principled BSDF"]
    gb.inputs["Base Color"].default_value = (0.36, 0.36, 0.38, 1)
    gb.inputs["Roughness"].default_value = 1.0
    gm.materials.append(gmat)
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    sun.data.energy = 3.6
    sun.data.angle = math.radians(4)
    d = Vector((-math.cos(math.radians(50)) * math.cos(math.radians(60)), -math.cos(math.radians(50)) * math.sin(math.radians(60)), -math.sin(math.radians(50))))
    sun.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    sc.collection.objects.link(sun)
    world = bpy.data.worlds.new("W")
    sc.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.36, 0.36, 0.38, 1)
    bg.inputs["Strength"].default_value = 1.0
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    sc.collection.objects.link(cam)
    B.setup_render(sc, 1024, 1024)
    bpy.ops.wm.save_as_mainfile(filepath=f"{OUT}/scenery_farms.blend", relative_remap=False)
    for key in objs:
        for k2, o2 in objs.items():
            o2.hide_render = (k2 != key)
        tz, ortho, az = VIEW[key]
        B.render_view(sc, cam, (0, 0, tz), ortho, f"{OUT}/SM_Farm_{key}_Iso.png", el=35.0, az=az)
    for o2 in objs.values():
        o2.hide_render = False

    # contact sheet: reference Iso | render, 3 rows x 2 pairs, 512 px panels
    def load(path):
        im = bpy.data.images.load(path)
        im.colorspace_settings.name = "Non-Color"
        im.scale(512, 512)
        a = np.array(im.pixels[:], dtype=np.float32).reshape(512, 512, 4)
        bpy.data.images.remove(im)
        return a
    sheet = np.ones((3 * 512, 4 * 512, 4), np.float32) * 0.1
    sheet[..., 3] = 1.0
    for i, (key, _) in enumerate(BUILDERS):
        row, col = i // 2, (i % 2) * 2
        ref = load(f"{REFDIR}/Farm_{key}_Iso.png")
        mod = load(f"{OUT}/SM_Farm_{key}_Iso.png")
        ry = (2 - row) * 512                   # image rows count from the bottom
        sheet[ry:ry + 512, col * 512:(col + 1) * 512] = ref
        sheet[ry:ry + 512, (col + 1) * 512:(col + 2) * 512] = mod
        sheet[ry:ry + 512, col * 512:col * 512 + 2] = 1.0
    out = bpy.data.images.new("sheet", 4 * 512, 3 * 512, alpha=False)
    out.colorspace_settings.name = "Non-Color"
    out.pixels.foreach_set(sheet.ravel())
    os.makedirs(os.path.dirname(SHEET), exist_ok=True)
    out.filepath_raw = SHEET
    out.file_format = "PNG"
    out.save()
    print("SHEET", SHEET)


if __name__ == "__main__":
    main()
