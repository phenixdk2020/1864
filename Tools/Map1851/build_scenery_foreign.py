"""Builds the campaign-map scenery sets for Sweden (SE), Norway (NO) and Prussia (PR), made like the Danish set.

Run headless:
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --python Tools/Map1851/build_scenery_foreign.py

Reuses the helpers (Model, wall_run, roof_gable, make_house, deco ...) and several complete builders of
build_scenery_models.py / build_scenery_farms.py. Each country gets ONE own atlas T_Scenery<CC>_Atlas.png (2048, 4x4 cells,
same size/layout idea as the Danish one) and the same eight role names as DK:
  SM_TownHouse_White, SM_TownHouse_Ochre, SM_TownHouse_Timber, SM_MerchantHouse, SM_Cottage, SM_Farm, SM_Church, SM_Windmill
Output per country: SourceAssets/Campaign1851/Scenery/<CC>/  (FBX x8, atlas, SM_<Role>_Iso.png renders, scenery_models.blend).
Contact sheet (reference Iso | model, 8 pairs per country row): Saved/Codex/scenery_foreign_models_sheet.png
Existing output is copied to _backup_<time> first. Not imported into Unreal (Content/Python/import_scenery_foreign.py does that).

Conventions as DK: 1 Blender unit = 1 m, FBX in cm (UnitScaleFactor 1), pivot at ground centre, +X = facade, Z up.
All dimensions and colours are ESTIMATES from the approved reference images, not measured buildings.

Technique: the DK builders hard-code tile names (e.g. "white", "ochre", "tile"). Here the tile layout and the painters are
replaced per country (same helper functions, new cell contents) and an ALIAS table maps a hard-coded name to another cell
(and optionally to a half cell) while a model is built, so the DK geometry builders can be reused unchanged.
"""
import math
import os
import shutil
import sys
import time

REPO = "C:/Projekter/1864-Campaign"
sys.path.insert(0, REPO + "/Tools/Map1851")

import bmesh
import bpy
import numpy as np
from mathutils import Vector

import build_scenery_models as B
import build_scenery_farms as F
from build_scenery_models import (Model, V, Z, X, Y, FULL, WIN, ARCH, WOOD, STONE, deco, wall_run, obox, fit_uv, right_of,
                                  roof_gable, ridge_seg, ridge_cap, chimney, arch_deco, thatch_ridge)

S = B.S
C = B.C
fbm, grain, yy, xx = B.fbm, B.grain, B.yy, B.xx
SCEN = REPO + "/SourceAssets/Campaign1851/Scenery"
REFROOT = REPO + "/Reference/Campaign1851/Scenery"
SHEET = REPO + "/Saved/Codex/scenery_foreign_models_sheet.png"
ROLES = ["TownHouse_White", "TownHouse_Ochre", "TownHouse_Timber", "MerchantHouse", "Cottage", "Farm", "Church", "Windmill"]

# ----------------------------------------------------------------------------------------------
# ALIAS: hard-coded tile name -> (real cell, half-cell or None). Active only while a model is built.
_ALIAS = {}
_orig_add = Model.add


def _add(self, pts, tile, sub_=FULL, uv=None, hint=None, smooth=False):
    a = _ALIAS.get(tile)
    if a:
        tile = a[0]
        if a[1] is not None and tuple(sub_) == tuple(FULL):
            sub_ = a[1]
    return _orig_add(self, pts, tile, sub_, uv, hint, smooth)


Model.add = _add
DOORS = {"door_green": ("doors", WIN), "door_ox": ("doors", ARCH)}      # two doors share one cell (left/right half)


# ================================================================================================
# TEXTURE PAINTERS (new ones; the rest are the DK painters of build_scenery_models.py)
def t_boards(col, seed, vertical=True, n=14, weather=0.0, plcol=(96, 90, 80), pl=0.07):
    """Painted board cladding. vertical: boards run up (Swedish/Norwegian church, falu-red); else horizontal clapboard."""
    idx = xx if vertical else yy
    bw = max(2, S // n)
    cell = idx // bw
    loc = idx % bw
    r = np.random.RandomState(seed).rand(S // bw + 3)[cell]
    T = np.ones((S, S, 3)) * C(*col)
    T *= (0.86 + 0.28 * r)[..., None]
    streak = fbm(seed + 3, 2, 70, 3) if vertical else fbm(seed + 3, 70, 2, 3)
    T *= (0.88 + 0.24 * streak)[..., None]
    if weather:
        patch = np.clip((fbm(seed + 4, 5, 5, 4) - 0.45) * 3.0, 0, 1) * weather
        T = T * (1 - patch[..., None]) + C(150, 142, 128) * patch[..., None]
    T[loc < 2] *= 0.5
    T[(loc >= 2) & (loc < 5)] *= 1.06
    T *= (1 - 0.2 * np.clip(1 - yy / S / 0.25, 0, 1) * fbm(seed + 9, 14, 14, 3))[..., None]
    T += grain(seed + 1, 0.02)[..., None]
    B.plinth(T, pl, seed, plcol)
    return T


def t_log(col, seed, rows=11, plcol=(104, 98, 88), pl=0.09):
    """Horizontal round logs with dark joints and cracks, stone foot."""
    bh = S // rows
    loc = yy % bh
    row = yy // bh
    r = np.random.RandomState(seed).rand(rows + 2)[row]
    T = np.ones((S, S, 3)) * C(*col)
    T *= (0.82 + 0.34 * r)[..., None]
    prof = np.sin(math.pi * (loc + 0.5) / bh)
    T *= (0.42 + 0.78 * prof)[..., None]
    T *= (0.82 + 0.36 * fbm(seed + 2, 4, 36, 3))[..., None]
    crack = (fbm(seed + 7, 6, 90, 2) > 0.74) & (loc > 6) & (loc < bh - 6)
    T[crack] *= 0.55
    T[loc < 3] *= 0.35
    T += grain(seed + 1, 0.025)[..., None]
    B.plinth(T, pl, seed, plcol)
    return T


def t_turf():
    """Roof turf: green-brown grass with dry patches and soil flecks."""
    n1 = fbm(201, 10, 10, 4)
    n2 = fbm(202, 40, 40, 3)
    n3 = fbm(203, 60, 10, 2)
    T = np.ones((S, S, 3)) * C(88, 112, 50)
    T *= (0.72 + 0.6 * n1)[..., None]
    T *= (0.82 + 0.36 * n2)[..., None]
    T *= (0.9 + 0.2 * n3)[..., None]
    dry = np.clip((fbm(204, 6, 6, 4) - 0.55) * 3.5, 0, 1) * 0.55
    T = T * (1 - dry[..., None]) + C(150, 138, 82) * dry[..., None]
    soil = np.clip((fbm(205, 22, 22, 3) - 0.68) * 5, 0, 1) * 0.6
    T = T * (1 - soil[..., None]) + C(86, 70, 48) * soil[..., None]
    T += grain(206, 0.05)[..., None]
    return T


def t_shingle(col, seed):
    """Wooden roof shingles in staggered rows (fine scale: the roof maps ~9 m onto one cell)."""
    rows, cw = 26, 14
    rh = S // rows
    row = yy // rh
    ly = yy % rh
    off = (row % 2) * (cw // 2) + (row * 5) % cw
    cell = (xx + off) // cw
    lx = (xx + off) % cw
    rs = np.random.RandomState(seed)
    rr = rs.rand(rows + 2, S // cw + 4)[row, cell]
    T = np.ones((S, S, 3)) * C(*col) * (0.68 + 0.64 * rr[..., None])
    T *= (0.5 + 0.5 * np.clip(ly / 6.0, 0, 1))[..., None]
    T[lx < 1.5] *= 0.6
    silver = np.clip((fbm(seed + 3, 6, 6, 4) - 0.5) * 3, 0, 1) * 0.4
    T = T * (1 - silver[..., None]) + C(142, 136, 126) * silver[..., None] * (0.7 + 0.5 * rr[..., None])
    T *= (0.85 + 0.3 * fbm(seed + 4, 12, 12, 3))[..., None]
    T += grain(seed + 5, 0.03)[..., None]
    return T


def t_lattice():
    """Post-mill sail: open wooden lattice (opaque, dark gap colour); no cloth."""
    u, v = xx / S, yy / S
    wood = C(176, 148, 108)
    T = np.ones((S, S, 3)) * C(120, 102, 80)
    lat = ((u * 5) % 1.0 < 0.14) | ((v * 26) % 1.0 < 0.16) | (u < 0.08) | (u > 0.94)
    T[lat] = (wood * (0.85 + 0.3 * fbm(101, 6, 6, 2)[..., None]))[lat]
    return T


def t_gravel():
    d1, d2, ident = B.voro(40, 211)
    base = C(150, 130, 100)[None, None, :] * (0.78 + 0.44 * fbm(212, 8, 8, 4)[..., None])
    st = C(176, 160, 130)[None, None, :] * (0.7 + 0.5 * B.idrand(ident, 213)[..., None])
    T = np.where((d1 < 0.34)[..., None], st, base)
    T = np.where(((d2 - d1) < 0.1)[..., None], T * 0.7, T)
    T += grain(214, 0.04)[..., None]
    return T


def door_cell(colA, colB):
    """One cell holding two doors (left half colA, right half colB). Each door painted full-cell by B.t_door logic, squeezed 2:1."""
    def one(col, band, seed):
        T = np.ones((S, S, 3)) * C(228, 218, 196)
        Wm, Hm = 1.2, 2.3
        v, XX, YY = B.sub(T, 0, 0, 1, 1, Wm, Hm)
        fr = C(236, 232, 220)
        v[...] = fr
        leaf = (XX > 0.08) & (XX < Wm - 0.08) & (YY > 0.02) & (YY < Hm - 0.08)
        tmp = np.zeros(v.shape)
        B.planks(tmp, XX, YY, Wm, Hm, C(*col), seed)
        v[leaf] = tmp[leaf]
        for ym in (0.4, 1.7):
            v[leaf & (abs(YY - ym) < 0.045)] = C(*band)
        v[leaf & (abs(XX - Wm / 2) < 0.012)] = C(*band)
        v[np.hypot(XX - (Wm - 0.22), YY - 1.0) < 0.04] = C(180, 150, 70)
        return T
    A = one(colA, tuple(int(c * 0.55) for c in colA), 71)
    Bd = one(colB, tuple(int(c * 0.55) for c in colB), 72)
    T = np.zeros((S, S, 3))
    T[:, :S // 2] = A[:, ::2]
    T[:, S // 2:] = Bd[:, ::2]
    return T


def gateway_barn():
    """NO merchant house: broad dark-green double barn doors in a pale frame (3.0 x 3.8 m, like the DK gateway cell)."""
    Wm, Hm = 3.0, 3.8
    T = np.ones((S, S, 3)) * C(214, 204, 184)
    v, XX, YY = B.sub(T, 0, 0, 1, 1, Wm, Hm)
    v[...] = C(214, 204, 184)
    frame = (XX > 0.1) & (XX < Wm - 0.1) & (YY < Hm - 0.15)
    v[frame] = C(234, 230, 218)
    leaf = (XX > 0.3) & (XX < Wm - 0.3) & (YY < Hm - 0.35)
    tmp = np.zeros(v.shape)
    B.planks(tmp, XX, YY, Wm, Hm, C(60, 80, 68), 82, 0.2)
    v[leaf] = tmp[leaf]
    v[leaf & (abs(XX - Wm / 2) < 0.03)] = C(18, 24, 20)
    for ym in (0.7, Hm - 0.9):
        v[leaf & (abs(YY - ym) < 0.08)] = C(30, 40, 34)
    for sgn in (-1, 1):
        v[leaf & B.seg_mask(XX, YY, Wm / 2, 0.8, Wm / 2 + sgn * 1.1, Hm - 1.0, 0.12)] = C(30, 40, 34)
    return T


def gateway_brick():
    """PR merchant house: arched double door in brick (same arch painter as DK, brick instead of ochre plaster)."""
    T = B.t_gateway()
    bg = np.all(np.abs(T - C(214, 170, 86)) < 1e-6, axis=2)
    return np.where(bg[..., None], B.t_brick() * 0.97, T)


def planks_weathered():
    return t_boards((120, 98, 74), 301, vertical=True, n=15, weather=0.55, plcol=(110, 104, 94), pl=0.0)


def t_plaster(col, seed, plcol=(96, 90, 80)):
    return B.wall_base(col, seed, plcol=plcol)


def t_brick_pr():
    """Backstein: slightly darker, coarser orange-red than the DK brick; mortar cream. No plinth (separate stone row)."""
    T = np.ones((S, S, 3)) * C(204, 194, 176)
    bh, bw = 16, 40
    row = yy // bh
    ly = yy % bh
    off = (row % 2) * (bw // 2)
    cx = (xx + off) % bw
    col = (xx + off) // bw
    rr = np.random.RandomState(161).rand(S // bh + 2, S // bw + 3)[row, col]
    br = C(160, 74, 52)[None, None, :] * (0.7 + 0.55 * rr[..., None])
    T = np.where(((ly >= 2) & (cx >= 2))[..., None], br, T)
    T *= (0.86 + 0.26 * fbm(162, 8, 8, 3))[..., None]
    return T


# ---------------------------------------------------------------- country definitions
def layout(names):
    return {n: (i % 4, i // 4) for i, n in enumerate(names)}


COUNTRIES = {}

COUNTRIES["SE"] = dict(
    tiles=["thatch", "tile", "shingle", "falu",
           "ochre_board", "grey_board", "white", "ochre",
           "timber_white", "window", "doors", "gateway",
           "wood_stone", "cobbles", "sail", "brick"],
    paint={
        "thatch": B.t_thatch, "tile": B.t_tile, "shingle": lambda: t_shingle((98, 84, 68), 311),
        "falu": lambda: t_boards((142, 44, 34), 321, True, 14, weather=0.1, plcol=(100, 96, 88)),
        "ochre_board": lambda: t_boards((206, 160, 62), 322, True, 14, weather=0.05, plcol=(96, 92, 84)),
        "grey_board": lambda: t_boards((112, 98, 84), 323, True, 14, weather=0.45, plcol=(96, 92, 84)),
        "white": B.t_white, "ochre": B.t_ochre, "timber_white": B.t_timber_white, "window": B.t_window,
        "doors": lambda: door_cell((52, 78, 56), (112, 40, 38)), "gateway": B.t_gateway, "wood_stone": B.t_wood_stone,
        "cobbles": B.t_cobbles, "sail": t_lattice, "brick": B.t_brick,
    },
    alias=dict(DOORS),
)
COUNTRIES["NO"] = dict(
    tiles=["tile", "shingle", "turf", "log",
           "log_grey", "board_white", "board_yellow", "board_red",
           "plaster", "window", "doors", "gateway",
           "wood_stone", "cobbles", "brick", "church_boards"],
    paint={
        "tile": B.t_tile, "shingle": lambda: t_shingle((64, 62, 62), 331), "turf": t_turf,
        "log": lambda: t_log((98, 78, 58), 341), "log_grey": lambda: t_log((108, 98, 86), 342, rows=11),
        "board_white": lambda: t_boards((222, 216, 200), 343, False, 30, weather=0.1),
        "board_yellow": lambda: t_boards((210, 164, 64), 344, False, 30, weather=0.05, plcol=(96, 92, 84)),
        "board_red": lambda: t_boards((150, 52, 40), 345, False, 30, weather=0.08, plcol=(96, 92, 84)),
        "plaster": lambda: t_plaster((214, 204, 184), 346),
        "window": B.t_window, "doors": lambda: door_cell((50, 64, 58), (146, 54, 42)), "gateway": gateway_barn,
        "wood_stone": B.t_wood_stone, "cobbles": t_gravel,
        "brick": lambda: B.wall_base((176, 170, 158), 347, plcol=(96, 92, 84)),
        "church_boards": lambda: t_boards((228, 224, 212), 348, True, 16, weather=0.08),
    },
    alias=dict(DOORS),
)
COUNTRIES["PR"] = dict(
    tiles=["thatch", "tile", "brick", "white",
           "ochre", "timber_ochre", "timber_white", "timber_brick",
           "window", "doors", "gateway", "plank",
           "wood_stone", "cobbles", "sail", "shingle"],
    paint={
        "thatch": B.t_thatch, "tile": B.t_tile, "brick": t_brick_pr, "white": B.t_white, "ochre": B.t_ochre,
        "timber_ochre": lambda: B.timber((226, 216, 194), (40, 32, 28), 355, 20, [0.5],
                                         [(0.0, 0.5, 0.5, 0.075), (0.5, 0.52, 1.0, 0.975)], 0.07),
        "timber_white": B.t_timber_white, "timber_brick": B.t_timber_brick, "window": B.t_window,
        "doors": lambda: door_cell((46, 72, 52), (86, 54, 42)), "gateway": gateway_brick, "plank": planks_weathered,
        "wood_stone": B.t_wood_stone, "cobbles": B.t_cobbles, "sail": B.t_sail, "shingle": lambda: t_shingle((104, 92, 80), 351),
    },
    alias=dict(DOORS),
)


def apply_country(cc):
    d = COUNTRIES[cc]
    B.TILES.clear()
    B.TILES.update(layout(d["tiles"]))
    B.PAINT.clear()
    B.PAINT.update(d["paint"])
    assert set(B.TILES) == set(B.PAINT), (set(B.TILES) ^ set(B.PAINT))
    B.ATLAS = f"{SCEN}/{cc}/T_Scenery{cc}_Atlas.png"
    B.OUT = f"{SCEN}/{cc}"


# ================================================================================================
# GEOMETRY HELPERS
def corner_boards(m, a, b, H, tile="white", w=0.3):
    """White corner boards on a house with ridge along Y (half depth a along X, half length b along Y)."""
    for sx in (1, -1):
        for sy in (1, -1):
            p = [V((sx * (a + 0.02), sy * b, 0)), V((sx * (a + 0.02), sy * (b - w), 0)),
                 V((sx * (a + 0.02), sy * (b - w), H)), V((sx * (a + 0.02), sy * b, H))]
            m.add(p, tile, FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(sx, 0, 0))
            q = [V((sx * a, sy * (b + 0.02), 0)), V((sx * (a - w), sy * (b + 0.02), 0)),
                 V((sx * (a - w), sy * (b + 0.02), H)), V((sx * a, sy * (b + 0.02), H))]
            m.add(q, tile, FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, sy, 0))


def house(name, L, D, H, rise, wall, roof, seg, front, back, gable, door, corner=None, **kw):
    m = B.make_house(name, L, D, H, rise, wall, roof, seg, None, front, back, gable, door, **kw)
    if corner:
        corner_boards(m, D / 2, L / 2, H, corner)
    return m


def emit(dst, src, dx, dy, ang_deg, dz=0.0):
    """Copy the polygons of model `src` into `dst`, rotated about Z by ang_deg and moved by (dx, dy, dz)."""
    ca, sa = math.cos(math.radians(ang_deg)), math.sin(math.radians(ang_deg))
    for pts, fuv, sm in src.polys:
        q = [V((p.x * ca - p.y * sa + dx, p.x * sa + p.y * ca + dy, p.z + dz)) for p in pts]
        dst.polys.append((q, fuv, sm))


def beam(m, p0, p1, w=0.2, tile="wood_stone", sub_=WOOD):
    p0, p1 = Vector(p0), Vector(p1)
    ex = (p1 - p0).normalized()
    ey = Z.cross(ex) if abs(ex.z) < 0.99 else X
    ey = ey.normalized()
    ez = ex.cross(ey)
    obox(m, (p0 + p1) / 2, ((p1 - p0).length / 2, w, w), tile, sub_, axes=(ex, ey, ez), nobottom=False)


def clip_poly(poly, axis, val, keep_ge):
    out = []
    n = len(poly)
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        ia = a[axis] >= val if keep_ge else a[axis] <= val
        ib = b[axis] >= val if keep_ge else b[axis] <= val
        if ia:
            out.append(a)
        if ia != ib:
            t = (val - a[axis]) / (b[axis] - a[axis])
            out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    return out


def region_face(m, o, n, poly, tile, sub_=FULL, cell=3.2):
    """Planar face with outward normal n, polygon given as (s, z) with s along right_of(n) from ground point o.
    Split into cells of <= `cell` metres so a small tile (brick) repeats at a believable scale."""
    n = V(n)
    r = right_of(n)
    o = V(o)
    ys = [p[0] for p in poly]
    zs = [p[1] for p in poly]
    y0, y1, z0, z1 = min(ys), max(ys), min(zs), max(zs)
    ny = max(1, int(math.ceil((y1 - y0) / cell - 1e-6)))
    nz = max(1, int(math.ceil((z1 - z0) / cell - 1e-6)))
    for i in range(ny):
        ya, yb = y0 + (y1 - y0) * i / ny, y0 + (y1 - y0) * (i + 1) / ny
        for j in range(nz):
            za, zb = z0 + (z1 - z0) * j / nz, z0 + (z1 - z0) * (j + 1) / nz
            pc = clip_poly(poly, 0, ya, True)
            pc = clip_poly(pc, 0, yb, False) if pc else pc
            pc = clip_poly(pc, 1, za, True) if pc else pc
            pc = clip_poly(pc, 1, zb, False) if pc else pc
            ded = []
            for p in pc:
                if not ded or abs(p[0] - ded[-1][0]) + abs(p[1] - ded[-1][1]) > 1e-5:
                    ded.append(p)
            if len(ded) > 1 and abs(ded[0][0] - ded[-1][0]) + abs(ded[0][1] - ded[-1][1]) < 1e-5:
                ded.pop()
            if len(ded) < 3:
                continue
            area = 0.5 * abs(sum(ded[k][0] * ded[(k + 1) % len(ded)][1] - ded[(k + 1) % len(ded)][0] * ded[k][1] for k in range(len(ded))))
            if area < 1e-3:
                continue
            pts = [o + r * p[0] + Z * p[1] for p in ded]
            uv = [((p[0] - ya) / (yb - ya), (p[1] - za) / (zb - za)) for p in ded]
            m.add(pts, tile, sub_, uv, hint=n)


def brick_face(m, o, n, hw, top, ph=0.6, z_from=None):
    """Brick face between -hw..hw. `top` = list of (s, z) top-boundary points left->right (wall top / gable profile).
    Stone plinth row (ph) below the brick unless z_from is given (then the face starts at z_from, no plinth)."""
    n = V(n)
    if z_from is None:
        region_face(m, o, n, [(-hw, 0), (hw, 0), (hw, ph), (-hw, ph)], "wood_stone", STONE, cell=3.0)
        base = ph
    else:
        base = z_from
    poly = [(-hw, base)] + list(top) + [(hw, base)]
    region_face(m, o, n, poly, "brick")


def stepped_profile(hw, z0, n, sh, sw, ztop):
    prof = [(-hw, z0)]
    for i in range(n):
        prof.append((-hw + i * sw, z0 + (i + 1) * sh))
        prof.append((-hw + (i + 1) * sw, z0 + (i + 1) * sh))
    p = hw - n * sw
    prof += [(-p, ztop), (p, ztop)]
    for i in range(n - 1, -1, -1):
        prof.append((hw - (i + 1) * sw, z0 + (i + 1) * sh))
        prof.append((hw - i * sw, z0 + (i + 1) * sh))
    prof.append((hw, z0))
    return prof


def treads(m, o, n, prof, depth, tile="tile"):
    n = V(n)
    r = right_of(n)
    o = V(o)
    for (ya, za), (yb, zb) in zip(prof[:-1], prof[1:]):
        if abs(za - zb) < 1e-6 and abs(ya - yb) > 1e-3:
            q = [o + r * ya + Z * za, o + r * yb + Z * za, o + r * yb + Z * za - n * depth, o + r * ya + Z * za - n * depth]
            m.add(q, tile, FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0, 1))


# ================================================================================================
# SE / NO CHURCH WITH SPIRE
def build_spire_church(wall, wall_seg, plinth_h, roof, tri_tile, porch=False, spire_h=23.5):
    m = Model("SM_Church")
    ta, ty0, ty1, th = 3.3, -13.4, -7.4, 10.4
    na, ny0, ny1, nh, nR = 4.6, -7.4, 8.1, 6.0, 11.6
    ca, cy0, cy1, ch, cR = 3.5, 8.1, 14.8, 5.6, 9.8
    tym, nym, cym = (ty0 + ty1) / 2, (ny0 + ny1) / 2, (cy0 + cy1) / 2
    W = lambda o, n, ln, h: wall_run(m, o, n, ln, 0, h, wall, seg=wall_seg, plinth_h=plinth_h)
    for sg in (1, -1):
        W(V((sg * ta, tym, 0)), V((sg, 0, 0)), ty1 - ty0, th)
        W(V((sg * na, nym, 0)), V((sg, 0, 0)), ny1 - ny0, nh)
        W(V((sg * ca, cym, 0)), V((sg, 0, 0)), cy1 - cy0, ch)
    W(V((0, ty0, 0)), V((0, -1, 0)), 2 * ta, th)
    W(V((0, ty1, 0)), V((0, 1, 0)), 2 * ta, th)
    for (a_, yv, hh, rr) in ((na, ny1, nh, nR), (ca, cy1, ch, cR)):
        n = V((0, 1, 0))
        o = V((0, yv, 0))
        W(o, n, 2 * a_, hh)
        r = right_of(n)
        m.add([o + r * (-a_) + Z * hh, o + r * a_ + Z * hh, o + Z * rr], tri_tile, FULL, [(0, .6), (1, .6), (.5, .98)], hint=n)
    roof_gable(m, na, ny0 + 0.02, ny1 + 0.4, nh, nR, 0.35, roof, ref=(10, 8), fas=0.14, ftile=(wall if wall_seg == 0 else tri_tile, FULL), gfas=(False, True))
    roof_gable(m, ca, cy0 + 0.02, cy1 + 0.35, ch, cR, 0.3, roof, ref=(8, 7), fas=0.14, ftile=(tri_tile, FULL), gfas=(False, True))
    # tower: skirt roof, louvre lantern, bell-cast base and pyramid spire
    zs0, zl0, zl1 = th - 0.4, th + 0.6, th + 3.6
    hs, hl = ta + 0.35, 2.2
    for sg in (1, -1):                     # skirt roof towards +/-X
        p = [V((sg * hs, ty0 - 0.35, zs0)), V((sg * hs, ty1 + 0.35, zs0)), V((sg * hl, tym + hl, zl0)), V((sg * hl, tym - hl, zl0))]
        m.add(p, roof, FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(sg, 0, 1))
    for sg in (1, -1):                     # towards +/-Y
        p = [V((-hs, tym + sg * (hs - ta + (ty1 - ty0) / 2 + 0.0), zs0)), V((hs, tym + sg * ((ty1 - ty0) / 2 + 0.35), zs0)),
             V((hl, tym + sg * hl, zl0)), V((-hl, tym + sg * hl, zl0))]
        p[0] = V((-hs, tym + sg * ((ty1 - ty0) / 2 + 0.35), zs0))
        m.add(p, roof, FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, sg, 1))
    obox(m, (0, tym, (zl0 + zl1) / 2), (hl, hl, (zl1 - zl0) / 2), "shingle", FULL)
    for k in range(4):
        ang = math.radians(90 * k)
        nvec = V((math.cos(ang), math.sin(ang), 0))
        deco(m, V((0, tym, 0)) + nvec * hl, nvec, 0, (zl0 + zl1) / 2, 0.9, 2.2, "window", ARCH)
    zb1, zb2 = zl1, zl1 + 1.3
    hb = 2.7
    for sg in (1, -1):
        for (a0, a1) in (((hb, hl)),):
            pass
    ring_lo = [V((hb, tym - hb, zb1)), V((hb, tym + hb, zb1)), V((-hb, tym + hb, zb1)), V((-hb, tym - hb, zb1))]
    ring_hi = [V((hl, tym - hl, zb2)), V((hl, tym + hl, zb2)), V((-hl, tym + hl, zb2)), V((-hl, tym - hl, zb2))]
    apex = V((0, tym, spire_h))
    for i in range(4):
        a_, b_ = ring_lo[i], ring_lo[(i + 1) % 4]
        c_, d_ = ring_hi[(i + 1) % 4], ring_hi[i]
        mid = (a_ + b_) / 2 - V((0, tym, 0))
        m.add([a_, b_, c_, d_], roof, FULL, [(0, 0), (1, 0), (1, .3), (0, .3)], hint=mid + Z * 0.4)
        m.add([d_, c_, apex], roof, FULL, [(0, .3), (1, .3), (.5, 1)], hint=mid + Z * 0.6)
    obox(m, (0, tym, spire_h + 0.4), (0.07, 0.07, 0.6), "wood_stone", WOOD, nobottom=False)
    obox(m, (0, tym, spire_h + 0.65), (0.07, 0.4, 0.07), "wood_stone", WOOD, nobottom=False)
    # windows: arched, on walls only
    for sg in (1, -1):
        n = V((sg, 0, 0))
        for yv in (-4.6, -0.1, 4.4):
            arch_deco(m, n * na, n, sg * yv, 3.0, 0.9, 2.0)
        arch_deco(m, n * ca, n, sg * 11.5, 3.0, 0.9, 2.0)
        arch_deco(m, n * ta, n, sg * tym, 7.2, 0.5, 1.4)
    arch_deco(m, V((0, ty1, 0)), V((0, 1, 0)), 0.0, 8.0, 0.5, 1.4)
    arch_deco(m, V((0, cy1, 0)), V((0, 1, 0)), 0.0, 3.2, 0.9, 2.0)
    if porch:
        pa, py0 = 1.5, ty0 - 2.3
        for sg in (1, -1):
            wall_run(m, V((sg * pa, (py0 + ty0) / 2, 0)), V((sg, 0, 0)), ty0 - py0, 0, 3.0, wall, seg=wall_seg, plinth_h=plinth_h)
        n = V((0, -1, 0))
        o = V((0, py0, 0))
        wall_run(m, o, n, 2 * pa, 0, 3.0, wall, seg=wall_seg, plinth_h=plinth_h)
        r = right_of(n)
        m.add([o + r * (-pa) + Z * 3.0, o + r * pa + Z * 3.0, o + Z * 4.5], tri_tile, FULL, [(0, .6), (1, .6), (.5, .98)], hint=n)
        roof_gable(m, pa, py0 - 0.2, ty0 + 0.1, 3.0, 4.5, 0.25, roof, ref=(5, 5), fas=0.1, ftile=(tri_tile, FULL), gfas=(True, False))
        deco(m, o, n, 0.0, 1.4, 1.2, 2.3, "door_ox", FULL)
        obox(m, (0, py0 - 0.8, 0.12), (0.9, 0.7, 0.12), "wood_stone", STONE)
    else:
        deco(m, V((0, ty0, 0)), V((0, -1, 0)), 0.0, 1.35, 1.4, 2.4, "door_green", FULL)
        obox(m, (0, ty0 - 0.8, 0.12), (0.9, 0.7, 0.12), "wood_stone", STONE)
    return m


# ================================================================================================
# POST MILL (SE lattice sails, PR canvas sails)
def build_postmill(wall, roof, sail):
    m = Model("SM_Windmill")
    hx, hy = 2.7, 2.2
    z0b, zt, zr = 5.2, 10.2, 12.4
    hz = 8.4
    # trestle: post, crossed ground beams, quarter bars, stone footings
    obox(m, (0, 0, z0b / 2), (0.38, 0.38, z0b / 2), "wood_stone", WOOD)
    beam(m, V((-4.0, 0, 0.45)), V((4.0, 0, 0.45)), 0.3)
    beam(m, V((0, -4.0, 0.5)), V((0, 4.0, 0.5)), 0.3)
    for (ex, ey) in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        beam(m, V((ex * 3.2, ey * 3.2, 0.75)), V((ex * 0.3, ey * 0.3, 3.6)), 0.17)
        obox(m, (ex * 3.9, ey * 3.9, 0.2), (0.55, 0.55, 0.2), "wood_stone", STONE)
    beam(m, V((0, 0, 3.4)), V((hx - 0.2, 0, z0b - 0.1)), 0.0001) if False else None
    # body: boards, saddle roof along X
    for sg in (1, -1):
        n = V((sg, 0, 0))
        p = [V((sg * hx, -hy, z0b)), V((sg * hx, hy, z0b)), V((sg * hx, hy, zt)), V((sg * hx, 0, zr - 0.1)), V((sg * hx, -hy, zt))]
        m.add(p, wall, FULL, [(0, 0), (1, 0), (1, .75), (.5, 1.0), (0, .75)], hint=n)
        q = [V((-hx, sg * hy, z0b)), V((hx, sg * hy, z0b)), V((hx, sg * hy, zt)), V((-hx, sg * hy, zt))]
        m.add(q, wall, FULL, [(0, 0), (1, 0), (1, .75), (0, .75)], hint=(0, sg, 0))
    k = (zr - zt) / hy
    ze = zt - 0.35 * k
    for sg in (1, -1):
        p = [V((hx + 0.3, sg * (hy + 0.35), ze)), V((-hx - 0.3, sg * (hy + 0.35), ze)), V((-hx - 0.3, 0, zr)), V((hx + 0.3, 0, zr))]
        m.add(p, roof, FULL, fit_uv(p, X, Vector((0, -sg, k)).normalized(), 9.0, 6.0), hint=(0, sg, 1))
        m.add([V((hx + 0.3, sg * (hy + 0.35), ze - 0.16)), V((-hx - 0.3, sg * (hy + 0.35), ze - 0.16)), V((-hx - 0.3, sg * (hy + 0.35), ze)),
               V((hx + 0.3, sg * (hy + 0.35), ze))], roof, FULL, [(0, 0), (1, 0), (1, .2), (0, .2)], hint=(0, sg, 0))
    # back: door + ladder + tail pole
    deco(m, V((-hx, 0, 0)), -X, -1.1, z0b + 1.15, 1.0, 1.9, "door_ox", FULL)
    lad = [V((-hx - 0.12, 0.6, z0b + 0.2)), V((-hx - 0.12, 1.6, z0b + 0.2)), V((-hx - 4.4, 1.6, 0.05)), V((-hx - 4.4, 0.6, 0.05))]
    m.add(lad, "sail", FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(-0.4, 0, 1))
    m.add(lad, "sail", FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0.4, 0, -1))
    beam(m, V((-hx - 0.1, -0.5, z0b + 0.6)), V((-hx - 7.8, -0.5, 0.45)), 0.2)
    for sg in (1, -1):
        for zz in (7.2, 8.8):
            B.deco(m, V((0, sg * hy, 0)), V((0, sg, 0)), 0.0 if zz > 8 else (1.2 * sg), zz, 0.7, 0.9, "window", WIN)
    # hub, stocks, sails
    sx = hx + 0.9
    obox(m, V((hx + 0.35, 0, hz)), (0.65, 0.34, 0.34), "wood_stone", WOOD)
    R = 10.2
    for phi in (45, 135):
        d = V((0, math.cos(math.radians(phi)), math.sin(math.radians(phi))))
        ex, ey = d, X
        obox(m, V((sx, 0, hz)), (R, 0.14, 0.18), "wood_stone", WOOD, axes=(ex, ey, ex.cross(ey)), nobottom=False)
    for kk in range(4):
        phi = math.radians(45 + 90 * kk)
        d = V((0, math.cos(phi), math.sin(phi)))
        p = V((0, -math.sin(phi), math.cos(phi)))
        c0 = V((sx + 0.16, 0, hz)) + d * 1.8 + p * 0.15
        c1 = c0 + d * 7.4
        quad = [c0, c1, c1 + p * 2.4, c0 + p * 2.4]
        m.add(quad, sail, FULL, [(0, 0), (0, 1), (1, 1), (1, 0)], hint=X)
        m.add([q - X * 0.03 for q in quad], sail, FULL, [(0, 0), (0, 1), (1, 1), (1, 0)], hint=-X)
    return m


# ================================================================================================
# NO: TUN FARM, WATERMILL
def log_house(name, L, D, H, rise, wall, front, door, back=None, gable=None, win=(0.8, 1.0), chim=(None, 0), oe=0.45, og=0.3):
    return B.make_house(name, L, D, H, rise, wall, "turf", 0.0, None, front, back or {}, gable or {}, door, chim=chim,
                        door_tile="door_ox", plinth_h=0.3, roof_ref=(9.0, 6.0), win_size=win, oe=oe, og=og)


def build_tun_farm():
    m = Model("SM_Farm")
    ALIAS_LOCAL = {"tile": ("turf", None), "white": ("log", None), "brick": ("brick", None)}
    old = dict(_ALIAS)
    _ALIAS.update(ALIAS_LOCAL)
    stue = log_house("stue", 9.4, 5.8, 2.7, 2.4, "log", {1.5: [-3.0, 3.1]}, (0.2, 2.0), back={1.5: [-2.5, 2.5]}, gable={1.5: [0.0]},
                     chim=(0.4, 1.0))
    fjos = log_house("fjos", 8.0, 5.2, 2.2, 2.2, "log_grey", {1.3: [-2.8]}, (1.3, 1.8), back={}, gable={})
    stab = log_house("stabbur", 4.8, 4.2, 2.3, 2.2, "log_grey", {}, None, gable={1.5: [0.0]})
    lathe = log_house("redskap", 7.0, 4.6, 2.0, 2.0, "log_grey", {}, (-1.5, 1.7))
    emit(m, stue, -12.0, 0, 0)
    emit(m, fjos, 0, 12.5, -90)
    emit(m, stab, 0, -12.5, 90, dz=1.1)
    emit(m, lathe, 12.0, 0, 180)
    _ALIAS.clear()
    _ALIAS.update(old)
    # stabbur: stone piers, stair, door
    for sx in (-1, 1):
        for sy in (-1, 1):
            obox(m, (sx * 1.6, -12.5 + sy * 1.7, 0.6), (0.4, 0.4, 0.6), "wood_stone", STONE)
    deco(m, V((0, -12.5 + 2.1, 0)), V((0, 1, 0)), 0.0, 1.1 + 1.15, 1.0, 1.9, "door_ox", FULL)
    m.add([V((-0.5, -10.4 + 0.1, 1.15)), V((0.5, -10.4 + 0.1, 1.15)), V((0.5, -10.4 + 2.0, 0.05)), V((-0.5, -10.4 + 2.0, 0.05))],
          "wood_stone", WOOD, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0.4, 1))
    # yard
    m.add([V((-9.0, -9.0, 0.04)), V((9.0, -9.0, 0.04)), V((9.0, 9.0, 0.04)), V((-9.0, 9.0, 0.04))], "cobbles", FULL,
          [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0, 1))
    return m


def build_watermill():
    m = Model("SM_Windmill")
    ALIAS_LOCAL = {"tile": ("turf", None), "white": ("log", None)}
    old = dict(_ALIAS)
    _ALIAS.update(ALIAS_LOCAL)
    h = B.make_house("mill", 7.2, 5.4, 4.0, 2.8, "log", "turf", 0.0, None, {}, {2.2: [-2.4]}, {2.2: [0.0]}, None,
                     chim=(None, 0), plinth_h=0.3, roof_ref=(9.0, 6.0), win_size=(0.8, 1.0), oe=0.5, og=0.4)
    _ALIAS.clear()
    _ALIAS.update(old)
    pz = 2.6
    emit(m, h, 0, 0, 0, dz=pz)
    for sx in (-1, 1):
        for sy in (-1, 1):
            obox(m, (sx * 2.2, sy * 3.0, pz / 2), (0.65, 0.65, pz / 2), "wood_stone", STONE)
    deco(m, V((2.7, 0, 0)), X, 0.0, pz + 1.3, 1.2, 2.2, "door_ox", FULL)
    deco(m, V((2.7, 0, 0)), X, -2.2, pz + 2.1, 0.8, 1.0, "window", WIN)
    m.add([V((3.9, -0.6, pz + 0.05)), V((3.9, 0.6, pz + 0.05)), V((7.5, 0.6, 0.05)), V((7.5, -0.6, 0.05))], "wood_stone", WOOD,
          [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0.4, 0, 1))
    # flume on trestles, running along -Y towards the mill wall, open channel
    zf = 6.3
    yA, yB = -17.0, -3.0
    wf, df = 0.7, 0.55
    m.add([V((-wf, yA, zf)), V((wf, yA, zf)), V((wf, yB, zf + 0.3)), V((-wf, yB, zf + 0.3))], "wood_stone", WOOD, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0, 1))
    for sg in (-1, 1):
        m.add([V((sg * wf, yA, zf)), V((sg * wf, yB, zf + 0.3)), V((sg * wf, yB, zf + 0.3 + df)), V((sg * wf, yA, zf + df))],
              "wood_stone", WOOD, [(0, 0), (1, 0), (1, .5), (0, .5)], hint=(-sg, 0, 0))
        m.add([V((sg * wf, yA, zf)), V((sg * wf, yB, zf + 0.3)), V((sg * wf, yB, zf + 0.3 + df)), V((sg * wf, yA, zf + df))],
              "wood_stone", WOOD, [(0, 0), (1, 0), (1, .5), (0, .5)], hint=(sg, 0, 0))
    for yv in (-15.5, -10.0, -4.5):
        zt = zf + (yv - yA) / (yB - yA) * 0.3
        for sg in (-1, 1):
            beam(m, V((sg * 0.6, yv, 0.05)), V((sg * 0.6, yv, zt)), 0.14)
        beam(m, V((-0.6, yv, zt - 0.2)), V((0.6, yv, zt - 0.2)), 0.12)
    # sluice tail: board dropping to the (invisible) wheel pit
    beam(m, V((0, -3.0, zf + 0.3)), V((0, -1.6, zf - 1.4)), 0.1)
    ALIAS_LOCAL.clear()
    return m


# ================================================================================================
# PR: BRICK CHURCH, STEPPED-GABLE MERCHANT HOUSE
def build_church_brick():
    m = Model("SM_Church")
    ta, ty0, ty1, th, tR = 4.6, -14.6, -7.4, 13.0, 17.4
    na, ny0, ny1, nh, nR = 4.6, -7.4, 8.1, 6.2, 12.0
    ca, cy0, cy1, ch, cR = 3.6, 8.1, 14.8, 5.6, 10.2
    tym, nym, cym = (ty0 + ty1) / 2, (ny0 + ny1) / 2, (cy0 + cy1) / 2
    SL = 0.6
    for sg in (1, -1):
        brick_face(m, V((sg * ta, tym, 0)), V((sg, 0, 0)), (ty1 - ty0) / 2, [(-(ty1 - ty0) / 2, th), ((ty1 - ty0) / 2, th)])
        brick_face(m, V((sg * na, nym, 0)), V((sg, 0, 0)), (ny1 - ny0) / 2, [(-(ny1 - ny0) / 2, nh), ((ny1 - ny0) / 2, nh)])
        brick_face(m, V((sg * ca, cym, 0)), V((sg, 0, 0)), (cy1 - cy0) / 2, [(-(cy1 - cy0) / 2, ch), ((cy1 - cy0) / 2, ch)])
    # stepped tower gables (front: wall + gable in one face; inner face and back face above the nave)
    prof = stepped_profile(ta, th, 4, 1.35, 0.8, th + 6.0)
    brick_face(m, V((0, ty0, 0)), V((0, -1, 0)), ta, prof)
    brick_face(m, V((0, ty0 + SL, 0)), V((0, 1, 0)), ta, prof, z_from=th)
    brick_face(m, V((0, ty1, 0)), V((0, 1, 0)), ta, prof, z_from=nh)
    brick_face(m, V((0, ty1 - SL, 0)), V((0, -1, 0)), ta, prof, z_from=th)
    treads(m, V((0, ty0, 0)), V((0, -1, 0)), prof, -SL)
    treads(m, V((0, ty1, 0)), V((0, 1, 0)), prof, SL)
    for (a_, yv, hh, rr) in ((na, ny1, nh, nR), (ca, cy1, ch, cR)):
        brick_face(m, V((0, yv, 0)), V((0, 1, 0)), a_, [(-a_, hh), (0, rr), (a_, hh)])
    roof_gable(m, ta, ty0 + SL, ty1 - SL, th, tR, 0.2, "tile", ref=(8, 8), fas=0.14, ftile=("tile", FULL), gfas=(False, False))
    roof_gable(m, na, ny0 + 0.02, ny1 + 0.4, nh, nR, 0.35, "tile", ref=(10, 8), fas=0.14, ftile=("tile", FULL), gfas=(False, True))
    roof_gable(m, ca, cy0 + 0.02, cy1 + 0.35, ch, cR, 0.3, "tile", ref=(8, 7), fas=0.14, ftile=("tile", FULL), gfas=(False, True))
    for (y0, y1, zr) in ((ty0 + SL, ty1 - SL, tR), (ny0, ny1 + 0.4, nR), (cy0, cy1 + 0.35, cR)):
        ridge_seg(m, V((0, y0, zr)), V((0, y1, zr)), 0.26, 0.22, "tile", FULL)
    for sg in (1, -1):
        n = V((sg, 0, 0))
        for yv in (-4.6, -0.1, 4.4):
            arch_deco(m, n * na, n, sg * yv, 3.2, 0.9, 2.0)
        arch_deco(m, n * ca, n, sg * 11.5, 3.1, 0.9, 2.0)
        for yv in (tym - 1.6, tym + 1.6):
            arch_deco(m, n * ta, n, sg * yv, 10.2, 0.7, 1.7)
        arch_deco(m, n * ta, n, sg * tym, 6.5, 0.4, 1.3)
    for sx in (-1.6, 1.6):
        arch_deco(m, V((0, ty0, 0)), V((0, -1, 0)), sx, 10.2, 0.7, 1.7)
    arch_deco(m, V((0, cy1, 0)), V((0, 1, 0)), 0.0, 3.3, 0.9, 2.0)
    deco(m, V((0, ty0, 0)), V((0, -1, 0)), 0.0, 1.5, 1.8, 2.9, "door_green", FULL)
    return m


def build_merchant_stepped():
    m = Model("SM_MerchantHouse")
    a, b, H = 6.0, 4.3, 7.0
    n_st, sh, sw = 4, 1.25, 0.95
    k = sh / sw
    zR = H + b * k
    prof = stepped_profile(b, H, n_st, sh, sw, zR)
    brick_face(m, V((a, 0, 0)), X, b, prof, ph=0.6)
    brick_face(m, V((-a, 0, 0)), -X, b, [(-b, H), (0, zR), (b, H)], ph=0.6)
    for sg in (1, -1):
        brick_face(m, V((0, sg * b, 0)), V((0, sg, 0)), a, [(-a, H), (a, H)], ph=0.6)
    treads(m, V((a, 0, 0)), X, prof, 0.4)
    ze = H - 0.35 * k
    for sg in (1, -1):
        p = [V((a, sg * (b + 0.35), ze)), V((-a - 0.3, sg * (b + 0.35), ze)), V((-a - 0.3, 0, zR)), V((a, 0, zR))]
        m.add(p, "tile", FULL, fit_uv(p, X, Vector((0, -sg, k)).normalized(), 9.0, 6.0), hint=(0, sg, 1))
        m.add([V((a, sg * (b + 0.35), ze - 0.16)), V((-a - 0.3, sg * (b + 0.35), ze - 0.16)), V((-a - 0.3, sg * (b + 0.35), ze)),
               V((a, sg * (b + 0.35), ze))], "tile", FULL, [(0, .3), (1, .3), (1, .45), (0, .45)], hint=(0, sg, 0))
    ridge_seg(m, V((-a - 0.3, 0, zR)), V((a, 0, zR)), 0.25, 0.2, "tile", FULL)
    deco(m, V((a, 0, 0)), X, 0, 0.6 + 1.9, 3.0, 3.8, "gateway", FULL)
    for sy in (-3.1, 3.1):
        deco(m, V((a, 0, 0)), X, sy, 2.2, 0.9, 1.3, "window", WIN)
    for sy in (-3.0, 0.0, 3.0):
        deco(m, V((a, 0, 0)), X, sy, 5.0, 0.9, 1.4, "window", WIN)
    for sy in (-2.3, 2.3):
        deco(m, V((a, 0, 0)), X, sy, 7.7, 0.8, 1.3, "window", WIN)
    deco(m, V((a, 0, 0)), X, 0, 9.5, 1.1, 1.9, "door_green", FULL)
    obox(m, (a + 0.55, 0, zR - 0.7), (0.55, 0.07, 0.07), "wood_stone", WOOD)
    for sg in (1, -1):
        n = V((0, sg, 0))
        for zz in (2.2, 4.9):
            for s_ in (-4.0, -1.4, 1.4, 4.0):
                deco(m, V((0, sg * b, 0)), n, s_, zz, 0.9, 1.3, "window", WIN)
    for zz in (2.4, 5.0):
        for s_ in (-2.2, 2.2):
            deco(m, V((-a, 0, 0)), -X, s_, zz, 0.9, 1.3, "window", WIN)
    chimney(m, -2.5, 0.0, zR - 1.0, zR + 1.5, 1.0, 1.0)
    return m


# ================================================================================================
# PER-COUNTRY MODEL LISTS: (role, builder, extra alias)
def models_SE():
    return [
        ("TownHouse_White", lambda: house("SM_TownHouse_White", 11.0, 7.6, 5.4, 3.4, "white", "tile", 0.0,
                                          {1.9: [-4.0, -2.2, 2.2, 4.0], 4.4: [-4.0, -2.2, 0.0, 2.2, 4.0]},
                                          {1.9: [-3.5, 0.0, 3.5], 4.4: [-3.5, 0.0, 3.5]},
                                          {1.9: [-1.9, 1.9], 4.4: [-1.9, 1.9], 7.4: [0.0]}, (0.0, 2.3), chim=(0.0, 1.0),
                                          roof_ref=(9.0, 6.0)), {}),
        ("TownHouse_Ochre", lambda: house("SM_TownHouse_Ochre", 11.0, 7.6, 5.8, 3.5, "ochre_board", "tile", 3.0,
                                          {1.9: [-4.0, -2.2, 2.2, 4.0], 4.6: [-4.0, -2.2, 0.0, 2.2, 4.0]},
                                          {1.9: [-3.5, 0.0, 3.5], 4.6: [-3.5, 0.0, 3.5]},
                                          {1.9: [-1.9, 1.9], 4.6: [-1.9, 1.9], 7.6: [0.0]}, (0.0, 2.3), corner="white",
                                          chim=(0.4, 1.0), roof_ref=(9.0, 6.0)), {}),
        ("TownHouse_Timber", lambda: house("SM_TownHouse_Timber", 11.5, 7.0, 4.6, 3.4, "grey_board", "tile", 3.0,
                                           {1.9: [-4.0, -2.0, 2.0, 4.0]}, {1.9: [-3.5, 0.0, 3.5]},
                                           {1.9: [-1.8, 1.8], 5.6: [0.0]}, (0.0, 2.2), door_tile="door_ox", corner="white",
                                           chim=(-0.2, 1.0), roof_ref=(9.0, 6.0)), {"tile": ("shingle", None)}),
        ("MerchantHouse", B.build_merchant, {}),
        ("Cottage", lambda: house("SM_Cottage", 10.4, 6.0, 2.7, 3.2, "falu", "tile", 3.2,
                                  {1.5: [-2.8, 2.8]}, {1.5: [-2.8, 0.0, 2.8]}, {1.5: [0.0]}, (0.0, 2.0), corner="white",
                                  chim=(0.3, 1.0), win_size=(0.8, 1.0), oe=0.5, roof_ref=(9.0, 6.0)), {"tile": ("shingle", None)}),
        ("Farm", B.build_farm, {"tar": ("grey_board", None)}),
        ("Church", lambda: build_spire_church("white", 0.0, 0.5, "shingle", "white"), {}),
        ("Windmill", lambda: build_postmill("grey_board", "shingle", "sail"), {}),
    ]


def models_NO():
    return [
        ("TownHouse_White", lambda: house("SM_TownHouse_White", 12.0, 7.0, 3.3, 3.5, "board_white", "tile", 3.0,
                                          {2.0: [-4.6, -2.5, 3.1, 4.9]}, {2.0: [-3.5, 0.0, 3.5]},
                                          {2.0: [-1.6, 1.6], 4.6: [0.0]}, (0.4, 1.7), door_tile="door_ox", chim=(-0.2, 1.0),
                                          roof_ref=(9.0, 6.0)), {"white": ("board_white", None)}),
        ("TownHouse_Ochre", lambda: house("SM_TownHouse_Ochre", 11.0, 7.5, 6.0, 3.5, "board_yellow", "tile", 3.0,
                                          {2.0: [-4.0, -2.2, 2.2, 4.0], 4.8: [-4.0, -2.2, 0.0, 2.2, 4.0]},
                                          {2.0: [-4.0, -1.5, 1.5, 4.0], 4.8: [-4.0, -1.5, 1.5, 4.0]},
                                          {2.0: [-1.8, 1.8], 4.8: [-1.8, 1.8], 7.5: [0.0]}, (0.0, 2.3), corner="white",
                                          chim=(0.4, 1.0), roof_ref=(9.0, 6.0)), {"white": ("board_white", None)}),
        ("TownHouse_Timber", lambda: house("SM_TownHouse_Timber", 12.0, 7.0, 5.6, 3.3, "board_red", "tile", 3.0,
                                           {1.9: [-4.4, -2.3, 2.3, 4.4], 4.7: [-4.4, -2.3, 0.0, 2.3, 4.4]},
                                           {1.9: [-3.5, 0.0, 3.5], 4.7: [-3.5, 0.0, 3.5]},
                                           {1.9: [-1.8, 1.8], 4.7: [-1.8, 1.8]}, (0.0, 2.3), door_tile="door_green",
                                           corner="white", chim=(-0.5, 1.0), roof_ref=(9.0, 6.0)),
         {"white": ("board_white", None)}),
        ("MerchantHouse", B.build_merchant, {"ochre": ("plaster", None), "white": ("board_white", None)}),
        ("Cottage", lambda: house("SM_Cottage", 9.8, 6.4, 2.6, 2.6, "log", "tile", 3.3,
                                  {1.4: [-3.1, 3.3]}, {1.4: [-2.6, 2.6]}, {1.4: [0.0]}, (0.6, 1.9), door_tile="door_ox",
                                  chim=(0.3, 1.0), win_size=(0.8, 1.0), oe=0.55, og=0.45, roof_ref=(9.0, 6.0)),
         {"tile": ("turf", None), "white": ("log", None)}),
        ("Farm", build_tun_farm, {}),
        ("Church", lambda: build_spire_church("church_boards", 3.0, 0.0, "shingle", "church_boards", porch=True, spire_h=24.0), {}),
        ("Windmill", build_watermill, {}),
    ]


def models_PR():
    return [
        ("TownHouse_White", B.build_white, {}),
        ("TownHouse_Ochre", B.build_ochre, {}),
        ("TownHouse_Timber", B.build_timber, {}),
        ("MerchantHouse", build_merchant_stepped, {}),
        ("Cottage", B.build_cottage, {}),
        ("Farm", F.build_niedersachsen, {}),
        ("Church", build_church_brick, {}),
        ("Windmill", lambda: build_postmill("plank", "shingle", "sail"), {}),
    ]


MODELS = {"SE": models_SE, "NO": models_NO, "PR": models_PR}
BUDGET = 520


# ================================================================================================
def export_fbx(ob, path):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"}, global_scale=1.0,
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", use_space_transform=True, bake_space_transform=True,
        axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", use_mesh_modifiers=True, add_leaf_bones=False,
        use_custom_props=False, path_mode="AUTO", embed_textures=False, use_tspace=False)


def build_country(cc, sc_state):
    out = f"{SCEN}/{cc}"
    stamp = time.strftime("%Y%m%d-%H%M%S")
    os.makedirs(out, exist_ok=True)
    olds = [f for f in os.listdir(out) if f.lower().endswith((".fbx", ".png", ".blend", ".md")) and os.path.isfile(out + "/" + f)]
    if olds:
        bk = f"{out}/_backup_{stamp}"
        os.makedirs(bk)
        for f in olds:
            shutil.copy2(out + "/" + f, bk + "/" + f)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 1.0
    apply_country(cc)
    img = B.build_atlas()
    mat = B.make_material(img)
    objs, tris = {}, {}
    for role, fn, extra in MODELS[cc]():
        _ALIAS.clear()
        _ALIAS.update(COUNTRIES[cc]["alias"])
        _ALIAS.update(extra)
        try:
            m = fn()
        finally:
            _ALIAS.clear()
        ob = m.to_object(mat)
        ob.name = "SM_" + role
        objs[role] = ob
        tris[role] = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    for role, ob in objs.items():
        export_fbx(ob, f"{out}/SM_{role}.fbx")
    print(f"=== STATISTIK {cc} ===")
    stats = {}
    for role, ob in objs.items():
        lo, hi, unit = B.fbx_bbox_cm(f"{out}/SM_{role}.fbx")
        e = hi - lo
        stats[role] = (tris[role], e)
        flag = "" if 100 <= tris[role] <= BUDGET else "   <-- outside 100-500"
        print(f"{cc} {role}: tris={tris[role]} bbox cm X*Y*Z = {e[0]:.0f} x {e[1]:.0f} x {e[2]:.0f} "
              f"min=({lo[0]:.0f},{lo[1]:.0f},{lo[2]:.0f}) max=({hi[0]:.0f},{hi[1]:.0f},{hi[2]:.0f}) unit={unit}{flag}")
    # preview scene
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
    gb.inputs["Base Color"].default_value = (0.5, 0.5, 0.5, 1)
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
    bg.inputs["Color"].default_value = (0.5, 0.5, 0.5, 1)
    bg.inputs["Strength"].default_value = 1.0
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    sc.collection.objects.link(cam)
    B.setup_render(sc, 768, 768)
    # side by side in the blend (all eight), then one render per model
    xs = {r: (i % 4) * 42.0 for i, r in enumerate(objs)}
    ys = {r: (i // 4) * 42.0 for i, r in enumerate(objs)}
    home = {r: ob.location.copy() for r, ob in objs.items()}
    for r, ob in objs.items():
        ob.location = (xs[r], ys[r], 0)
    bpy.ops.wm.save_as_mainfile(filepath=f"{out}/scenery_models.blend", relative_remap=False)
    for r, ob in objs.items():
        ob.location = home[r]
    for role, ob in objs.items():
        for r2, o2 in objs.items():
            o2.hide_render = (r2 != role)
        tris_, e = stats[role]
        ortho = max(e[0], e[1], e[2] * 1.25) / 100.0 * 1.2
        render_view(sc, cam, (0, 0, e[2] / 200.0 * 0.9), ortho, f"{out}/SM_{role}_Iso.png")
    for o2 in objs.values():
        o2.hide_render = False
    return stats


def render_view(sc, cam, target, ortho, path):
    B.render_view(sc, cam, target, ortho, path, el=35.0, az=38.0)


def load_square(path, size):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    im.scale(size, size)
    a = np.array(im.pixels[:], dtype=np.float32).reshape(size, size, 4)
    bpy.data.images.remove(im)
    return a


def contact_sheet(countries, panel=256):
    rows = len(countries)
    sheet = np.ones((rows * panel, 16 * panel, 4), np.float32) * 0.1
    sheet[..., 3] = 1.0
    for ri, cc in enumerate(countries):
        refs = {
            "SE": ["TownHouse_White_SE", "TownHouse_Ochre_SE", "TownHouse_Timber_SE", "MerchantHouse_SE", "Cottage_Parstuga_SE",
                   "Farm_Firlaenget_Skane", "Church_Village_SE", "Windmill_Post_SE"],
            "NO": ["TownHouse_White_NO", "TownHouse_Ochre_NO", "TownHouse_Timber_NO", "MerchantHouse_NO", "Cottage_Laft_NO",
                   "Farm_Tun_NO", "Church_Wooden_NO", "Windmill_Watermill_NO"],
            "PR": ["TownHouse_White_PR", "TownHouse_Ochre_PR", "TownHouse_Timber_PR", "MerchantHouse_PR", "Cottage_Kate_PR",
                   "Farm_Hallenhaus_PR", "Church_Backstein_PR", "Windmill_Bock_PR"],
        }[cc]
        ry = (rows - 1 - ri) * panel
        for i, role in enumerate(ROLES):
            ref = load_square(f"{REFROOT}/{cc}/{refs[i]}_Iso.png", panel)
            mod = load_square(f"{SCEN}/{cc}/SM_{role}_Iso.png", panel)
            sheet[ry:ry + panel, (2 * i) * panel:(2 * i + 1) * panel] = ref
            sheet[ry:ry + panel, (2 * i + 1) * panel:(2 * i + 2) * panel] = mod
            sheet[ry:ry + panel, (2 * i) * panel:(2 * i) * panel + 2] = 1.0
    out = bpy.data.images.new("sheet", 16 * panel, rows * panel, alpha=False)
    out.colorspace_settings.name = "Non-Color"
    out.pixels.foreach_set(sheet.ravel())
    os.makedirs(os.path.dirname(SHEET), exist_ok=True)
    out.filepath_raw = SHEET
    out.file_format = "PNG"
    out.save()
    print("SHEET", SHEET)


def main():
    only = [a for a in sys.argv[sys.argv.index("--") + 1:]] if "--" in sys.argv else []
    ccs = only or ["SE", "NO", "PR"]
    allstats = {}
    for cc in ccs:
        allstats[cc] = build_country(cc, None)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    contact_sheet(["SE", "NO", "PR"] if not only else ccs)
    return allstats


if __name__ == "__main__":
    main()
