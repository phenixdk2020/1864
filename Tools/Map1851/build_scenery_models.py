"""Bygger otte danske kampagnekort-bygninger som lavpoly spilmodeller med EET fælles teksturatlas.

Koeres headless:
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --python Tools/Map1851/build_scenery_models.py

Ingen import i Unreal. Output: SourceAssets/Campaign1851/Scenery/DK/
  SM_<Type>.fbx (x8), T_SceneryDK_Atlas.png, scenery_models.blend,
  scenery_models_preview.png, scenery_models_closeup.png, README.md (skrives i haanden)

Enheder: 1 Blender-enhed = 1 m. FBX eksporteres med apply_unit_scale + FBX_SCALE_NONE (vertices x100, UnitScaleFactor 1 = cm), saa Unreal faar cm.
Pivot ved jordcentrum, +X = facade (gadesiden), Z op. Eksisterende output flyttes til _backup_<tid>.
Maal og farver er skoen efter referencebillederne (ESTIMAT), ikke maalt efter virkelige bygninger.
"""
import math
import os
import shutil
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Vector

REPO = "C:/Projekter/1864-Campaign"
OUT = REPO + "/SourceAssets/Campaign1851/Scenery/DK"
ATLAS = OUT + "/T_SceneryDK_Atlas.png"
S = 512                      # felt-stoerrelse i atlas (4 x 4 felter = 2048)
PAD = 3.0 / 2048.0           # UV-indrykning mod bloeding

Z = Vector((0, 0, 1))
X = Vector((1, 0, 0))
Y = Vector((0, 1, 0))
V = Vector

# ----------------------------------------------------------------------------------------------
# ATLAS-LAYOUT: navn -> (kolonne, raekke fra TOPPEN)
TILES = {
    "thatch": (0, 0), "tile": (1, 0), "ochre": (2, 0), "white": (3, 0),
    "timber_ochre": (0, 1), "timber_white": (1, 1), "brick": (2, 1), "window": (3, 1),
    "door_green": (0, 2), "door_ox": (1, 2), "gateway": (2, 2), "tar": (3, 2),
    "sail": (0, 3), "cobbles": (1, 3), "timber_brick": (2, 3), "wood_stone": (3, 3),   # (2,3) was "grass" (unused by models) until 2026-10-23
}
# under-felter (u0,v0,u1,v1) i et felt
FULL = (0, 0, 1, 1)
WIN = (0, 0, .5, 1)          # window-feltet: venstre halvdel = sprosset rektangulaer rude
ARCH = (.5, 0, 1, 1)         # hoejre halvdel = spidsbuet/rundbuet kirkevindue
WOOD = (0, .5, 1, 1)         # wood_stone: oeverste halvdel = vejrbidt traeplanke
STONE = (0, 0, 1, .5)        # nederste halvdel = markstensmur

yy, xx = np.mgrid[0:S, 0:S]   # yy = raekke (v op), xx = soejle


def C(r, g, b):
    return np.array([r, g, b], float) / 255.0


def vn(cy, cx, seed):
    """Periodisk value-noise, 0..1, S x S."""
    r = np.random.RandomState(seed).rand(cy, cx)
    ys = np.arange(S) * cy / S
    xs = np.arange(S) * cx / S
    y0 = ys.astype(int)
    x0 = xs.astype(int)
    fy = ys - y0
    fx = xs - x0
    fy = fy * fy * (3 - 2 * fy)
    fx = fx * fx * (3 - 2 * fx)
    y1 = (y0 + 1) % cy
    x1 = (x0 + 1) % cx
    a = r[np.ix_(y0, x0)]
    b = r[np.ix_(y0, x1)]
    c = r[np.ix_(y1, x0)]
    d = r[np.ix_(y1, x1)]
    FY = fy[:, None]
    FX = fx[None, :]
    return (a * (1 - FX) + b * FX) * (1 - FY) + (c * (1 - FX) + d * FX) * FY


def fbm(seed, cy=6, cx=6, octs=4):
    t = np.zeros((S, S))
    amp = 1.0
    tot = 0.0
    for i in range(octs):
        t += amp * vn(cy * 2 ** i, cx * 2 ** i, seed + i * 17)
        tot += amp
        amp *= 0.5
    return t / tot


def grain(seed, amt=0.05):
    return (np.random.RandomState(seed).rand(S, S) - 0.5) * 2 * amt


def voro(n, seed):
    """Voronoi: (d1, d2, id) i celle-enheder, n x n jitrede celler, periodisk."""
    rs = np.random.RandomState(seed)
    P = rs.rand(n, n, 2)
    gy = yy / S * n
    gx = xx / S * n
    cy = gy.astype(int)
    cx = gx.astype(int)
    d1 = np.full((S, S), 9e9)
    d2 = np.full((S, S), 9e9)
    ident = np.zeros((S, S), int)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            ny = cy + dy
            nx = cx + dx
            pt = P[ny % n, nx % n]
            d = np.hypot(gx - (nx + pt[..., 1]), gy - (ny + pt[..., 0]))
            ident_new = ((ny % n) * n + (nx % n))
            closer = d < d1
            d2 = np.where(closer, d1, np.minimum(d2, d))
            ident = np.where(closer, ident_new, ident)
            d1 = np.where(closer, d, d1)
    return d1, d2, ident


def idrand(ident, seed):
    r = np.random.RandomState(seed).rand(int(ident.max()) + 1)
    return r[ident]


def blend(T, mask, col, k=1.0):
    m = mask[..., None] * k if mask.ndim == 2 else mask
    T[...] = T * (1 - m) + np.asarray(col, float) * m


def sub(T, u0, v0, u1, v1, Wm, Hm):
    x0, x1 = int(u0 * S), int(u1 * S)
    y0, y1 = int(v0 * S), int(v1 * S)
    h, w = y1 - y0, x1 - x0
    Xm = (np.arange(w) + 0.5) / w * Wm
    Ym = (np.arange(h) + 0.5) / h * Hm
    XX, YY = np.meshgrid(Xm, Ym)
    return T[y0:y1, x0:x1], XX, YY


def seg_mask(XX, YY, x0, y0, x1, y1, w):
    dx, dy = x1 - x0, y1 - y0
    L2 = dx * dx + dy * dy
    t = np.clip(((XX - x0) * dx + (YY - y0) * dy) / L2, 0, 1)
    return np.hypot(XX - (x0 + t * dx), YY - (y0 + t * dy)) < w / 2


def plinth(T, h=0.07, seed=3, col=(88, 84, 76)):
    """Mork stensokkel nederst i et vaegfelt."""
    n = fbm(seed, 12, 12, 3)
    edge = h + (n - 0.5) * 0.025
    m = (yy / S < edge).astype(float)
    d1, d2, ident = voro(18, seed)
    stone = C(*col)[None, None, :] * (0.75 + 0.5 * idrand(ident, seed)[..., None]) * (1 - 0.35 * (d2 - d1 < 0.12)[..., None])
    blend(T, m, stone)


# ---------------------------------------------------------------- felter
def t_thatch():
    """Straa: fine uregelmaessige lodrette straa, graabrunt, lysere vejrbidte pletter, moerke mosstrimler nedefra."""
    fine = fbm(1, 2, 160, 2)
    fine2 = fbm(2, 3, 90, 3)
    mid = fbm(3, 6, 40, 3)
    patch = fbm(4, 5, 5, 4)
    T = np.ones((S, S, 3)) * C(116, 98, 72)
    T *= (0.72 + 0.56 * fine)[..., None]
    T *= (0.8 + 0.4 * fine2)[..., None]
    T *= (0.92 + 0.16 * mid)[..., None]
    light = np.clip((patch - 0.5) * 3.0, 0, 1) * 0.45
    T = T * (1 - light[..., None]) + C(158, 146, 118) * light[..., None] * (0.7 + 0.5 * fine2)[..., None]
    moss = fbm(5, 2, 70, 3)
    mm = np.clip(1.0 - yy / S / 0.42, 0, 1) * np.clip((moss - 0.42) * 3.5, 0, 1)
    T = T * (1 - mm[..., None] * 0.7) + C(58, 66, 42) * mm[..., None] * 0.7
    T *= (0.8 + 0.2 * np.clip(yy / S * 2.0, 0, 1))[..., None]
    T += grain(6, 0.05)[..., None]
    return T


def t_tile():
    rows, rh, cw = 16, S // 16, 24
    T = np.zeros((S, S, 3))
    row = yy // rh
    ly = yy % rh
    off = (row % 2) * (cw // 2)
    cxp = (xx + off) % cw
    col = (xx + off) // cw
    rs = np.random.RandomState(11)
    rr = rs.rand(rows + 1, S // cw + 3)
    r1 = rr[row, col]
    base = C(178, 70, 44)[None, None, :] * (0.78 + 0.42 * r1[..., None]) + C(30, 24, 8) * (rs.rand(rows + 1, S // cw + 3)[row, col])[..., None] * 0.5
    T[:] = base
    rib = 0.88 + 0.12 * np.cos(cxp / cw * 2 * math.pi)
    T *= rib[..., None]
    sh = np.clip((ly - 0) / 7.0, 0, 1) * 0.5 + 0.5          # skyggekant i bunden af hver raekke
    T *= sh[..., None]
    T *= np.where(cxp < 1.5, 0.72, 1.0)[..., None]
    T *= (0.85 + 0.3 * fbm(12, 8, 8, 4))[..., None]
    T *= (0.9 + 0.1 * (ly / rh))[..., None]
    T += grain(13, 0.03)[..., None]
    return T


def wall_base(col, seed, band=None, pl=0.07, plcol=(80, 76, 68)):
    n = fbm(seed, 8, 8, 4)
    n2 = fbm(seed + 5, 3, 3, 3)
    T = np.ones((S, S, 3)) * C(*col)
    T *= (0.88 + 0.22 * n)[..., None]
    T *= (0.95 + 0.1 * n2)[..., None]
    # fugtige pletter nederst
    T *= (1 - 0.22 * np.clip(1 - yy / S / 0.25, 0, 1) * fbm(seed + 9, 14, 14, 3))[..., None]
    T += grain(seed + 1, 0.02)[..., None]
    if band:
        b = (abs(yy / S - band[0]) < band[1] / 2).astype(float)
        blend(T, b, C(*band[2]) * (0.9 + 0.2 * n[..., None]), 0.85)
    plinth(T, pl, seed, plcol)
    return T


def t_ochre():
    return wall_base((214, 170, 86), 21, band=(0.5, 0.012, (232, 212, 160)), plcol=(76, 72, 66))


def t_white():
    return wall_base((226, 216, 192), 31, plcol=(96, 90, 80))


def timber(infill, post, seed, pw, rails, braces, pl=0.07):
    T = wall_base(infill, seed, pl=pl, plcol=(96, 92, 84))
    pwu = pw / S
    wood = C(*post)[None, None, :] * (0.8 + 0.4 * fbm(seed + 3, 2, 60, 2)[..., None])
    u = xx / S
    v = yy / S
    m = np.zeros((S, S))
    for pu in (0.0, 0.5, 1.0):
        m = np.maximum(m, (abs(u - pu) < pwu * (0.5 if pu in (0.0, 1.0) else 0.5)))
    m = np.maximum(m, (v > 0.975))
    m = np.maximum(m, (abs(v - (pl + 0.012)) < 0.016))
    for r in rails:
        m = np.maximum(m, (abs(v - r) < pwu * 0.45))
    m = m.astype(float)
    m[v < pl] = 0
    for (a, b, c, d) in braces:
        mk = seg_mask(u * 1.0, v * 1.0, a, b, c, d, pwu * 0.9).astype(float)
        mk[v < pl] = 0
        m = np.maximum(m, mk)
    # skygge ved siden af tommerne (relief)
    sh = np.zeros((S, S))
    sh[:, 2:] = m[:, :-2]
    sh[:-2, :] = np.maximum(sh[:-2, :], m[2:, :])
    blend(T, (sh * (1 - m)), C(60, 52, 40), 0.35)
    blend(T, m, wood)
    return T


def t_timber_ochre():
    return timber((220, 172, 90), (34, 30, 28), 41, 20, [0.5], [(0.0, 0.5, 0.5, 0.075 + 0.0), (0.5, 0.52, 1.0, 0.975)], 0.07)


def t_timber_white():
    return timber((228, 218, 196), (74, 58, 46), 51, 15, [0.62], [(0.5, 0.075, 0.0, 0.62), (0.5, 0.62, 1.0, 0.975)], 0.07)


def t_brick():
    T = np.ones((S, S, 3)) * C(206, 196, 178)       # moertel
    bh, bw = 16, 40
    row = yy // bh
    ly = yy % bh
    off = (row % 2) * (bw // 2)
    cx = (xx + off) % bw
    col = (xx + off) // bw
    rs = np.random.RandomState(61)
    rr = rs.rand(S // bh + 2, S // bw + 3)[row, col]
    br = C(150, 68, 48)[None, None, :] * (0.75 + 0.5 * rr[..., None])
    inside = (ly >= 2) & (cx >= 2)
    T = np.where(inside[..., None], br, T)
    T *= (0.88 + 0.2 * fbm(62, 8, 8, 3))[..., None]
    return T


def t_window():
    T = np.ones((S, S, 3)) * C(228, 218, 196)
    # --- sprosset rektangulaer rude 1.0 x 1.4 m (venstre halvdel)
    v, XX, YY = sub(T, 0, 0, .5, 1, 1.0, 1.4)
    fr = C(238, 234, 222)
    v[...] = fr
    sill = YY < 0.11
    v[sill] = C(150, 146, 138)
    gl = (XX > 0.07) & (XX < 0.93) & (YY > 0.15) & (YY < 1.33)
    grad = (0.7 + 0.5 * (YY / 1.4))[..., None]
    v[gl] = (C(42, 54, 68) * grad)[gl]
    refl = gl & (abs((XX - 0.2) * 1.4 - (YY - 0.4) * 0.5 - 0.0) < 0.09) & (XX < 0.5)
    v[refl] = v[refl] * 0.6 + C(120, 138, 156) * 0.4
    for xm in (0.5,):
        v[(abs(XX - xm) < 0.022) & gl] = fr
    for ym in (0.15 + (1.33 - 0.15) / 3, 0.15 + 2 * (1.33 - 0.15) / 3):
        v[(abs(YY - ym) < 0.022) & gl] = fr
    edge = gl & ((XX < 0.1) | (YY > 1.30))
    v[edge] = v[edge] * 0.55          # indre skygge
    # --- rundbuet kirkevindue 0.9 x 2.0 m (hoejre halvdel)
    v, XX, YY = sub(T, .5, 0, 1, 1, 0.9, 2.0)
    wallc = C(228, 218, 196)
    v[...] = wallc
    rad = 0.45
    cy = 2.0 - rad
    inb = lambda r: ((XX > 0.45 - r) & (XX < 0.45 + r) & (YY < cy)) | (np.hypot(XX - 0.45, YY - cy) < r)
    outer = inb(0.45)
    inner = inb(0.30) & (YY > 0.2)
    v[outer] = C(196, 184, 160)
    v[(YY < 0.1)] = C(170, 160, 142)
    glass = C(36, 44, 56) * (0.7 + 0.5 * (YY / 2.0))[..., None]
    v[inner] = glass[inner]
    lead = inner & ((abs(((XX + YY) * 6.0) % 1.0 - 0.5) > 0.44) | (abs(((XX - YY) * 6.0) % 1.0 - 0.5) > 0.44))
    v[lead] = C(18, 20, 24)
    return T


def planks(v, XX, YY, Wm, Hm, col, seed, pw=0.17):
    n = fbm(seed, 2, 24, 2)[:v.shape[0], :v.shape[1]] if False else None
    cell = (XX / pw).astype(int)
    r = np.random.RandomState(seed).rand(64)[cell % 64]
    v[...] = (np.array(col) * (0.75 + 0.5 * r)[..., None])
    gap = (XX % pw) < 0.012
    v[gap] = v[gap] * 0.35


def t_door(variant):
    T = np.ones((S, S, 3)) * C(228, 218, 196)
    Wm, Hm = 1.2, 2.3
    v, XX, YY = sub(T, 0, 0, 1, 1, Wm, Hm)
    fr = C(236, 232, 220)
    v[...] = fr
    leaf = (XX > 0.08) & (XX < Wm - 0.08) & (YY > 0.02) & (YY < Hm - 0.08)
    if variant == "green":
        leaf = leaf & (YY < Hm - 0.5)
        tr = (XX > 0.08) & (XX < Wm - 0.08) & (YY >= Hm - 0.5) & (YY < Hm - 0.08)
        v[tr] = C(30, 38, 46)
        v[tr & ((abs(XX - Wm / 2) < 0.02) | (abs(XX - Wm / 4) < 0.02) | (abs(XX - 3 * Wm / 4) < 0.02))] = fr
        col = C(52, 80, 60)
    else:
        col = C(104, 36, 38)
    tmp = np.zeros(v.shape)
    planks(tmp, XX, YY, Wm, Hm, col, 71 if variant == "green" else 72)
    v[leaf] = tmp[leaf]
    if variant == "green":
        for ym in (0.5, 1.2, 1.7):
            v[leaf & (abs(YY - ym) < 0.05)] = C(30, 44, 36)
        v[leaf & (abs(XX - Wm / 2) < 0.015)] = C(20, 30, 24)
    else:
        for ym in (0.4, 1.7):
            band = leaf & (abs(YY - ym) < 0.045)
            v[band] = C(34, 24, 22)
        v[leaf & (abs(XX - 0.9) < 0.01) & (YY < 1.5)] = C(30, 20, 20)
    knob = np.hypot(XX - (Wm - 0.22), YY - 1.0) < 0.04
    v[knob] = C(180, 150, 70)
    return T


def t_gateway():
    Wm, Hm = 3.0, 3.8
    T = np.ones((S, S, 3)) * C(214, 170, 86)
    v, XX, YY = sub(T, 0, 0, 1, 1, Wm, Hm)
    v[...] = C(214, 170, 86) * (0.9 + 0.2 * fbm(81, 6, 6, 3)[:v.shape[0], :v.shape[1]][..., None] if False else 1.0)
    rad = 1.4
    cy = Hm - rad - 0.1

    def shape(r):
        return ((XX > Wm / 2 - r) & (XX < Wm / 2 + r) & (YY < cy)) | (np.hypot(XX - Wm / 2, YY - cy) < r)
    stone = shape(rad)
    inner = shape(rad - 0.28)
    brick = C(196, 184, 156)
    v[stone] = brick * (0.9 + 0.1 * np.sin(YY * 7)[..., None])[stone]
    # buens kiler
    ang = np.arctan2(YY - cy, XX - Wm / 2)
    keystone = stone & ~inner & (YY > cy) & (abs(np.sin(ang * 6)) < 0.08)
    v[keystone] = C(120, 112, 96)
    # sokkel i bunden
    v[(YY < 0.5) & ~inner & stone] = C(110, 106, 98)
    tmp = np.zeros(v.shape)
    planks(tmp, XX, YY, Wm, Hm, C(60, 78, 66), 82, 0.24)
    v[inner] = tmp[inner]
    v[inner & (abs(XX - Wm / 2) < 0.025)] = C(18, 24, 20)
    for ym in (0.6, cy - 0.2):
        v[inner & (abs(YY - ym) < 0.07)] = C(30, 40, 34)
    for sgn in (-1, 1):
        mk = inner & seg_mask(XX, YY, Wm / 2, 0.7, Wm / 2 + sgn * 1.0, cy - 0.3, 0.1)
        v[mk] = C(30, 40, 34)
    return T


def t_tar():
    n1 = fbm(91, 3, 30, 3)
    pw = 36
    cell = xx // pw
    r = np.random.RandomState(92).rand(30)[cell % 30]
    T = np.ones((S, S, 3)) * C(104, 78, 54)
    T *= (0.75 + 0.5 * r)[..., None]
    T *= (0.82 + 0.36 * n1)[..., None]
    lx = (xx % pw)
    T[lx < 2] *= 0.45
    T[(lx >= 2) & (lx < 5)] *= 1.12
    T[(yy % 170) < 3] *= 0.7
    T += grain(93, 0.025)[..., None]
    return T


def t_sail():
    u = xx / S
    v = yy / S
    wood = C(184, 154, 112)
    gap = C(138, 118, 92)
    T = np.ones((S, S, 3)) * gap
    lat = ((u * 5) % 1.0 < 0.14) | ((v * 26) % 1.0 < 0.16)
    T[lat] = wood * (0.85 + 0.3 * fbm(101, 6, 6, 2)[lat][..., None])
    cloth = (u > 0.1) & (u < 0.97) & (v > 0.03) & (v < 0.72 + 0.14 * (1 - u)) & (v < 0.97)
    cl = C(238, 228, 196) * (0.9 + 0.12 * fbm(102, 10, 10, 3))[..., None]
    cl *= (1 - 0.12 * (np.sin(u * 40 + v * 6) > 0.8))[..., None]
    T[cloth] = cl[cloth]
    T[(u < 0.08)] = wood * 0.7
    T[(u > 0.975)] = wood * 0.75
    return T


def t_cobbles():
    d1, d2, ident = voro(34, 111)
    base = C(128, 120, 106)[None, None, :] * (0.7 + 0.5 * idrand(ident, 112)[..., None])
    T = base * (1 - 0.15 * fbm(113, 6, 6, 3)[..., None]) * (0.88 + 0.12 * np.clip(d1 * 1.6, 0, 1))[..., None]
    gap = (d2 - d1) < 0.12
    T = np.where(gap[..., None], C(70, 64, 54) * (0.8 + 0.4 * fbm(114, 20, 20, 2))[..., None], T)
    # lidt jord/graes mellem
    patch = fbm(115, 4, 4, 3) > 0.66
    T = np.where((patch & gap)[..., None], C(92, 108, 54), T)
    return T


def t_grass():
    n = fbm(121, 10, 10, 4)
    T = np.ones((S, S, 3)) * C(106, 142, 66) * (0.8 + 0.4 * n)[..., None]
    T += grain(122, 0.05)[..., None]
    T[..., 2] *= 0.9
    return T


def t_timber_brick():
    """Fachwerk with muted red-brick infill (Niedersachsen hall house); oak frame like timber(), stone plinth at the bottom."""
    T = np.ones((S, S, 3)) * C(206, 196, 178)
    bh, bw = 16, 40
    row = yy // bh
    ly = yy % bh
    off = (row % 2) * (bw // 2)
    cx = (xx + off) % bw
    col = (xx + off) // bw
    rr = np.random.RandomState(141).rand(S // bh + 2, S // bw + 3)[row, col]
    br = C(176, 92, 70)[None, None, :] * (0.78 + 0.4 * rr[..., None])
    T = np.where(((ly >= 2) & (cx >= 2))[..., None], br, T)
    T *= (0.9 + 0.18 * fbm(142, 8, 8, 3))[..., None]
    pl = 0.07
    u, v = xx / S, yy / S
    pwu = 22 / S
    m = np.zeros((S, S))
    for pu in (0.0, 0.5, 1.0):
        m = np.maximum(m, abs(u - pu) < pwu * 0.5)
    m = np.maximum(m, v > 0.975)
    m = np.maximum(m, abs(v - (pl + 0.012)) < 0.016)
    m = np.maximum(m, abs(v - 0.55) < pwu * 0.45)
    m = m.astype(float)
    m[v < pl] = 0
    for (a, b, c, d) in ((0.5, 0.075, 0.0, 0.55), (0.5, 0.55, 1.0, 0.975)):
        mk = seg_mask(u, v, a, b, c, d, pwu * 0.9).astype(float)
        mk[v < pl] = 0
        m = np.maximum(m, mk)
    sh = np.zeros((S, S))
    sh[:, 2:] = m[:, :-2]
    sh[:-2, :] = np.maximum(sh[:-2, :], m[2:, :])
    blend(T, sh * (1 - m), C(50, 38, 30), 0.4)
    wood = C(62, 48, 38)[None, None, :] * (0.8 + 0.4 * fbm(143, 2, 60, 2)[..., None])
    blend(T, m, wood)
    plinth(T, pl, 144, (96, 92, 84))
    return T


def t_wood_stone():
    T = np.zeros((S, S, 3))
    # oeverst: vejrbidt traeplanke (vandrette aarer)
    n = fbm(131, 40, 3, 3)
    wood = C(116, 94, 70)[None, None, :] * (0.6 + 0.8 * n)[..., None]
    wood[(yy % 32) < 2] *= 0.45
    T[:] = wood
    # nederst: markstenssmur
    d1, d2, ident = voro(9, 132)
    st = C(124, 116, 102)[None, None, :] * (0.7 + 0.5 * idrand(ident, 133)[..., None]) * (0.9 + 0.1 * np.clip(d1, 0, 1))[..., None]
    st = np.where(((d2 - d1) < 0.1)[..., None], C(66, 62, 54), st)
    lower = yy < S // 2
    T[lower] = st[lower]
    return T


PAINT = {"thatch": t_thatch, "tile": t_tile, "ochre": t_ochre, "white": t_white, "timber_ochre": t_timber_ochre,
         "timber_white": t_timber_white, "brick": t_brick, "window": t_window, "door_green": lambda: t_door("green"),
         "door_ox": lambda: t_door("ox"), "gateway": t_gateway, "tar": t_tar, "sail": t_sail, "cobbles": t_cobbles,
         "timber_brick": t_timber_brick, "wood_stone": t_wood_stone}


def build_atlas():
    A = np.zeros((4 * S, 4 * S, 3))
    for name, (c, r) in TILES.items():
        t = np.clip(PAINT[name](), 0, 1)
        rb = 3 - r
        A[rb * S:(rb + 1) * S, c * S:(c + 1) * S] = t
    rgba = np.ones((4 * S, 4 * S, 4), np.float32)
    rgba[..., :3] = A
    img = bpy.data.images.new("T_SceneryDK_Atlas", 4 * S, 4 * S, alpha=False)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = ATLAS
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)
    img = bpy.data.images.load(ATLAS)
    img.colorspace_settings.name = "sRGB"
    img.pack() if False else None
    return img


# ----------------------------------------------------------------------------------------------
# MESH-BYGGER
def tile_rect(tile, sub_=FULL):
    c, r = TILES[tile]
    rb = 3 - r
    u0 = (c + sub_[0]) / 4.0 + PAD
    u1 = (c + sub_[2]) / 4.0 - PAD
    v0 = (rb + sub_[1]) / 4.0 + PAD
    v1 = (rb + sub_[3]) / 4.0 - PAD
    return u0, v0, u1, v1


def newell(pts):
    n = Vector((0, 0, 0))
    for i in range(len(pts)):
        a, b = pts[i], pts[(i + 1) % len(pts)]
        n.x += (a.y - b.y) * (a.z + b.z)
        n.y += (a.z - b.z) * (a.x + b.x)
        n.z += (a.x - b.x) * (a.y + b.y)
    return n


def fit_uv(pts, ax, ay, ru=0.0, rv=0.0):
    us = [p.dot(ax) for p in pts]
    vs = [p.dot(ay) for p in pts]
    u0, u1, v0, v1 = min(us), max(us), min(vs), max(vs)
    su = max(u1 - u0, ru, 1e-6)
    sv = max(v1 - v0, rv, 1e-6)
    return [((u - u0) / su, (v - v0) / sv) for u, v in zip(us, vs)]


def right_of(n):
    return Z.cross(n).normalized()


def wall_uv(pts, n, a_lo, a_hi, z_lo, z_hi):
    r = right_of(n)
    return [((p.dot(r) - a_lo) / (a_hi - a_lo), (p.z - z_lo) / (z_hi - z_lo)) for p in pts]


class Model:
    def __init__(self, name):
        self.name = name
        self.polys = []

    def add(self, pts, tile, sub_=FULL, uv=None, hint=None, smooth=False):
        pts = [Vector(p) for p in pts]
        if uv is None:
            n = newell(pts).normalized()
            ax = Y if abs(n.x) > abs(n.y) and abs(n.x) >= abs(n.z) else X
            ay = Z if abs(n.z) < 0.7 else Y
            uv = fit_uv(pts, ax, ay)
        uv = list(uv)
        if hint is not None and newell(pts).dot(Vector(hint)) < 0:
            pts.reverse()
            uv.reverse()
        u0, v0, u1, v1 = tile_rect(tile, sub_)
        fuv = [(u0 + min(max(a, 0), 1) * (u1 - u0), v0 + min(max(b, 0), 1) * (v1 - v0)) for a, b in uv]
        self.polys.append((pts, fuv, smooth))

    def to_object(self, scene_mat):
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new("UVMap")
        smooth_verts = []
        smooth_faces = []
        for pts, fuv, sm in self.polys:
            vs = [bm.verts.new(p) for p in pts]
            try:
                f = bm.faces.new(vs)
            except ValueError:
                continue
            for lp, uvv in zip(f.loops, fuv):
                lp[uvl].uv = uvv
            if sm:
                smooth_verts.extend(vs)
                smooth_faces.append(f)
        if smooth_verts:
            bmesh.ops.remove_doubles(bm, verts=smooth_verts, dist=1e-4)
            for f in smooth_faces:
                if f.is_valid:
                    f.smooth = True
        bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method="BEAUTY", ngon_method="EAR_CLIP")
        me = bpy.data.meshes.new(self.name)
        bm.to_mesh(me)
        bm.free()
        me.materials.append(scene_mat)
        for p in me.polygons:
            if not p.use_smooth:
                p.use_smooth = False
        ob = bpy.data.objects.new(self.name, me)
        bpy.context.scene.collection.objects.link(ob)
        return ob


# ---------------------------------------------------------------- hjaelpere
def obox(m, c, half, tile, sub_=FULL, axes=(X, Y, Z), ref=(0, 0), nobottom=True, hide=()):
    c = Vector(c)
    for i in range(3):
        for s in (1, -1):
            if nobottom and i == 2 and s == -1:
                continue
            if (i, s) in hide:
                continue
            n = axes[i] * s
            j, k = [q for q in range(3) if q != i]
            pts = [c + n * half[i] + axes[j] * a * half[j] + axes[k] * b * half[k] for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
            if i == 2:
                uv = fit_uv(pts, axes[0], axes[1], ref[0], ref[1])
            else:
                other_h = axes[2] if k == 2 else axes[j]
                uv = fit_uv(pts, axes[j] if k == 2 else axes[j], axes[k], ref[0], ref[1])
            m.add(pts, tile, sub_, uv, hint=n)


def deco(m, o, n, s, z, w, h, tile, sub_, off=0.02):
    r = right_of(n)
    c = Vector(o) + r * s + Z * z + n * off
    p = [c - r * w / 2 - Z * h / 2, c + r * w / 2 - Z * h / 2, c + r * w / 2 + Z * h / 2, c - r * w / 2 + Z * h / 2]
    m.add(p, tile, sub_, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=n)


def wall_run(m, o, n, length, z0, z1, tile, seg=0.0, plinth_h=0.0, sub_=FULL):
    """Vaeg centreret paa o (ground), normal n, laengde length. seg>0: opdel i felter (bindingsvaerk)."""
    r = right_of(n)
    o = Vector(o)
    nseg = max(1, int(math.ceil(length / seg - 1e-6))) if seg else 1
    for i in range(nseg):
        a0 = -length / 2 + length * i / nseg
        a1 = -length / 2 + length * (i + 1) / nseg
        p0, p1 = o + r * a0, o + r * a1
        if plinth_h:
            q = [p0 + Z * z0, p1 + Z * z0, p1 + Z * (z0 + plinth_h), p0 + Z * (z0 + plinth_h)]
            m.add(q, tile, sub_, [(0, 0), (1, 0), (1, 0.07), (0, 0.07)], hint=n)
            q = [p0 + Z * (z0 + plinth_h), p1 + Z * (z0 + plinth_h), p1 + Z * z1, p0 + Z * z1]
            m.add(q, tile, sub_, [(0, 0.07), (1, 0.07), (1, 1), (0, 1)], hint=n)
        else:
            q = [p0 + Z * z0, p1 + Z * z0, p1 + Z * z1, p0 + Z * z1]
            m.add(q, tile, sub_, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=n)
    return nseg


def bay_centres(length, seg):
    nseg = max(1, int(math.ceil(length / seg - 1e-6)))
    bw = length / nseg / 2.0
    return [-length / 2 + (i + 0.5) * bw for i in range(nseg * 2)]


def roof_gable(m, a, y0, y1, H, zR, oe, tile, ref=(7.0, 6.0), smooth=False, fas=0.16, ftile=None, og0=0, og1=0, gfas=(True, True)):
    """Saddeltag med rygning langs Y. Planen gaar gennem (a, H); eavens overhang oe."""
    s = (zR - H) / a
    ze = H - s * oe
    updir = {+1: Vector((-1, 0, s)).normalized(), -1: Vector((1, 0, s)).normalized()}
    for sg in (1, -1):
        p = [V((sg * (a + oe), y0, ze)), V((sg * (a + oe), y1, ze)), V((0, y1, zR)), V((0, y0, zR))]
        m.add(p, tile, FULL, fit_uv(p, Y, updir[sg], ref[0], ref[1]), hint=(sg, 0, 1), smooth=smooth)
        if fas:
            ft = ftile or (tile, FULL)
            q = [V((sg * (a + oe), y0, ze - fas)), V((sg * (a + oe), y1, ze - fas)), V((sg * (a + oe), y1, ze)), V((sg * (a + oe), y0, ze))]
            m.add(q, ft[0], ft[1], [(0, 0), (1, 0), (1, .15), (0, .15)] if ft[0] == "thatch" else [(0, .3), (1, .3), (1, .45), (0, .45)], hint=(sg, 0, 0))
            for yy_, ysg, gf in ((y0, -1, gfas[0]), (y1, 1, gfas[1])):
                if not gf:
                    continue
                q = [V((sg * (a + oe), yy_, ze)), V((0, yy_, zR)), V((0, yy_, zR - fas)), V((sg * (a + oe), yy_, ze - fas))]
                m.add(q, ft[0], ft[1], [(0, .3), (1, .3), (1, .45), (0, .45)], hint=(0, ysg, 0))
    return ze


def ridge_cap(m, y0, y1, zR, w=0.22, h=0.18, tile="wood_stone", sub_=WOOD):
    r0, r1 = V((0, y0, zR + h)), V((0, y1, zR + h))
    for sg in (1, -1):
        p = [V((sg * w, y0, zR)), V((sg * w, y1, zR)), r1, r0]
        m.add(p, tile, sub_, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(sg, 0, 1))
    for yv, sgy in ((y0, -1), (y1, 1)):
        m.add([V((-w, yv, zR)), V((w, yv, zR)), V((0, yv, zR + h))], tile, sub_, [(0, 0), (1, 0), (.5, 1)], hint=(0, sgy, 0))


def chimney(m, cx, cy, zb, zt, w=0.9, d=0.9, tile="brick", cap=True):
    obox(m, (cx, cy, (zb + zt) / 2), (w / 2, d / 2, (zt - zb) / 2), tile, FULL, ref=(0, 0))
    if cap:
        obox(m, (cx, cy, zt + 0.07), (w / 2 + 0.08, d / 2 + 0.08, 0.07), "wood_stone", STONE)


def slope_z(a, H, zR, x):
    return H + (zR - H) * (1 - abs(x) / a)


def gable_walls(m, a, b_pos, H, zR, tile, seg=0.0, gcols=None, tri_v=(0.62, 0.96), plinth_h=0.0):
    """Gavlvaegge +/-Y: rektangel + trekant."""
    for sgn in (1, -1):
        n = V((0, sgn, 0))
        o = V((0, sgn * b_pos, 0))
        wall_run(m, o, n, 2 * a, 0, H, tile, seg=seg, plinth_h=plinth_h)
        r = right_of(n)
        tri = [o + r * (-a) + Z * H, o + r * a + Z * H, o + Z * zR]
        m.add(tri, tile, FULL, [(0, tri_v[0]), (1, tri_v[0]), (0.5, tri_v[1])], hint=n)


# ================================================================================================
# MODELLER
def make_house(name, L, D, H, rise, wall_tile, roof_tile, seg, floors_z, front_w, back_w, gable_w, door,
               chim=(0.0, 1.0), door_tile="door_green", thatch=False, plinth_h=0.45, gable_attic=True, gable_ref=(7, 6),
               roof_ref=(7.0, 6.0), halfhip=False, extra=None, win_size=(1.0, 1.4), oe=0.45, og=0.35):
    m = Model(name)
    a, b = D / 2, L / 2
    zR = H + rise
    ph = plinth_h
    # sokkel
    obox(m, (0, 0, ph / 2), (a + 0.06, b + 0.06, ph / 2), "wood_stone", STONE, ref=(4, ph))
    # vaegge
    walls = [("F", V((1, 0, 0)), L), ("B", V((-1, 0, 0)), L)]
    for key, n, length in walls:
        wall_run(m, n * a, n, length, 0, H, wall_tile, seg=seg)
    if not halfhip:
        gable_walls(m, a, b, H, zR, wall_tile, seg=seg, tri_v=(0.6, 0.98) if "timber" not in wall_tile else (0.5, 1.0))
    # tag
    if halfhip:
        ze = roof_halfhip(m, a, b, H, zR, oe, og, wall_tile, roof_tile, seg)
    else:
        ze = roof_gable(m, a, -(b + og), b + og, H, zR, oe, roof_tile, ref=roof_ref, smooth=thatch, fas=0.34 if thatch else 0.16,
                        ftile=("thatch", FULL) if thatch else ("white", FULL))
        if thatch:
            ridge_cap(m, -(b + og), b + og, zR, 0.28, 0.2)
        else:
            ridge_cap(m, -(b + og), b + og, zR, 0.25, 0.2, "tile", FULL)
    # vinduer
    ws, hs = win_size
    for key, n, length in walls:
        lst = front_w if key == "F" else back_w
        bw = bay_centres(length, seg) if seg else None
        for fz, items in lst.items():
            for it in items:
                s = bw[it] if (seg and isinstance(it, int)) else it
                deco(m, n * a, n, s, fz, ws, hs, "window", WIN)
    for sgn in (1, -1):
        n = V((0, sgn, 0))
        bw = bay_centres(D, seg) if seg else None
        for fz, items in gable_w.items():
            for it in items:
                s = bw[it] if (seg and isinstance(it, int)) else it
                if fz > H - 0.5 and not halfhip:
                    # loftvindue: tjek at det er under tagfladen
                    wsz, hsz = ws * 0.8, hs * 0.8
                else:
                    wsz, hsz = ws, hs
                deco(m, V((0, sgn * b, 0)), n, s, fz, wsz, hsz, "window", WIN)
    # doer
    if door:
        n = V((1, 0, 0))
        bw = bay_centres(L, seg) if seg else None
        s = bw[door[0]] if (seg and isinstance(door[0], int)) else door[0]
        dh = door[1] if len(door) > 1 else 2.3
        deco(m, n * a, n, s, ph + 0.05 + dh / 2, 1.2, dh, door_tile, FULL, off=0.02)
        obox(m, (a + 0.6, s * 1.0, 0.13), (0.55, 0.85, 0.13), "wood_stone", STONE)
    # skorsten
    cy_, _ = chim
    if cy_ is not None:
        cyy = cy_ * b * 0.55 if abs(cy_) <= 1 else cy_
        chimney(m, 0.0, cyy, zR - 0.7, zR + 1.5)
    if extra:
        extra(m, a, b, H, zR)
    return m


def roof_halfhip(m, a, b, H, zR, oe, og, wall_tile, roof_tile, seg):
    s = (zR - H) / a
    ze = H - s * oe
    yE = b + og
    zH = H + 0.58 * (zR - H)
    r = b - 0.55
    xh = a - (zH - H) / s
    k = (zR - zH) / (yE - r)
    zb = zH + (yE - b) * k
    xa = a - (zb - H) / s
    for sgn in (1, -1):
        n = V((0, sgn, 0))
        o = V((0, sgn * b, 0))
        rr = right_of(n)
        p = [o + rr * (-a), o + rr * a, o + rr * a + Z * H, o + rr * xa + Z * zb, o - rr * xa + Z * zb, o - rr * a + Z * H]
        m.add(p, wall_tile, FULL, [(0, 0), (1, 0), (1, 0.7), (0.5 + xa / (2 * a), 0.98), (0.5 - xa / (2 * a), 0.98), (0, 0.7)], hint=n)
    ref = (7.0, 6.0)

    def bulge(pa, pb, out, depth=0.14):
        mid_a, mid_b = pa + out * depth - Z * 0.17, pb + out * depth - Z * 0.17
        low_a, low_b = pa + out * 0.02 - Z * 0.4, pb + out * 0.02 - Z * 0.4
        m.add([pa, pb, mid_b, mid_a], "thatch", FULL, [(0, .1), (1, .1), (1, .2), (0, .2)], hint=out + Z * 0.3, smooth=True)
        m.add([mid_a, mid_b, low_b, low_a], "thatch", FULL, [(0, 0), (1, 0), (1, .1), (0, .1)], hint=out - Z * 0.3, smooth=True)
    for sg in (1, -1):
        up = Vector((-sg, 0, s)).normalized()
        p = [V((sg * (a + oe), -yE, ze)), V((sg * (a + oe), yE, ze)), V((sg * xh, yE, zH)), V((0, r, zR)), V((0, -r, zR)), V((sg * xh, -yE, zH))]
        m.add(p, roof_tile, FULL, fit_uv(p, Y, up, ref[0], ref[1]), hint=(sg, 0, 1), smooth=True)
        bulge(V((sg * (a + oe), -yE, ze)), V((sg * (a + oe), yE, ze)), V((sg, 0, 0)))
        for ys in (1, -1):
            q = [V((sg * (a + oe), ys * yE, ze)), V((sg * xh, ys * yE, zH)), V((sg * xh, ys * yE, zH - 0.3)), V((sg * (a + oe), ys * yE, ze - 0.3))]
            m.add(q, "thatch", FULL, [(0, .1), (1, .1), (1, .2), (0, .2)], hint=(0, ys, 0), smooth=True)
    for sgn in (1, -1):
        p = [V((-xh, sgn * yE, zH)), V((xh, sgn * yE, zH)), V((0, sgn * r, zR))]
        dirv = Vector((0, -sgn, k)).normalized()
        m.add(p, roof_tile, FULL, fit_uv(p, X, dirv, ref[0], ref[1]), hint=(0, sgn, 1), smooth=True)
        bulge(V((-xh, sgn * yE, zH)), V((xh, sgn * yE, zH)), V((0, sgn, 0)), 0.1)
    thatch_ridge(m, V((0, -r, zR)), V((0, r, zR)), 0.42, 0.42)
    return ze


def build_ochre():
    return make_house("SM_TownHouse_Ochre", 11.0, 7.5, 6.0, 3.5, "ochre", "tile", 0.0, None,
                      {2.0: [-4.0, -1.5, 4.0], 5.0: [-4.0, -1.5, 1.5, 4.0]},
                      {2.0: [-4.0, -1.5, 1.5, 4.0], 5.0: [-4.0, -1.5, 1.5, 4.0]},
                      {2.0: [-1.8, 1.8], 5.0: [-1.8, 1.8], 7.5: [0.0]}, (1.5,), chim=(0.4, 1.0), roof_ref=(9.0, 6.0))


def build_white():
    return make_house("SM_TownHouse_White", 12.0, 7.0, 3.3, 3.5, "white", "tile", 0.0, None,
                      {2.0: [-4.6, -2.5, -0.1, 3.1, 4.9]}, {2.0: [-3.5, 0.0, 3.5]},
                      {2.0: [-1.6, 1.6], 4.6: [0.0]}, (1.7,), chim=(-0.2, 1.0), door_tile="door_ox", roof_ref=(9.0, 6.0))


def build_timber():
    return make_house("SM_TownHouse_Timber", 12.0, 7.0, 5.6, 3.3, "timber_ochre", "tile", 3.0, None,
                      {1.9: [1, 3, 4, 6], 4.7: [0, 2, 4, 6]},
                      {1.9: [1, 3, 5, 7], 4.7: [1, 3, 5, 7]},
                      {1.9: [1, 2], 4.7: [1, 2]}, (2,), chim=(-0.5, 1.0), door_tile="door_ox", roof_ref=(9.0, 6.0))


def thatch_ridge(m, p0, p1, w=0.42, h=0.42):
    """Rundet, tyk halmrygning (5-punkts profil, bloed skygge)."""
    p0, p1 = Vector(p0), Vector(p1)
    d = (p1 - p0).normalized()
    sd = Z.cross(d).normalized()
    prof = [(-w, -0.08), (-w * 0.85, h * 0.6), (0.0, h), (w * 0.85, h * 0.6), (w, -0.08)]
    for i in range(4):
        (s0, z0), (s1, z1) = prof[i], prof[i + 1]
        q = [p0 + sd * s0 + Z * z0, p1 + sd * s0 + Z * z0, p1 + sd * s1 + Z * z1, p0 + sd * s1 + Z * z1]
        mid = sd * (s0 + s1) / 2 + Z * ((z0 + z1) / 2 + 0.2)
        m.add(q, "thatch", FULL, [(0, 0.3 + i * 0.1), (1, 0.3 + i * 0.1), (1, 0.4 + i * 0.1), (0, 0.4 + i * 0.1)], hint=mid, smooth=True)
    for pe, sg in ((p0, -1), (p1, 1)):
        pts = [pe + sd * s_ + Z * z_ for (s_, z_) in prof]
        m.add(pts, "thatch", FULL, [(0.5 + s_ / (2 * w), 0.4 + z_) for (s_, z_) in prof], hint=d * sg, smooth=True)


def ridge_seg(m, p0, p1, w=0.25, h=0.2, tile="wood_stone", sub_=WOOD):
    p0, p1 = Vector(p0), Vector(p1)
    d = (p1 - p0).normalized()
    sd = Z.cross(d).normalized()
    for sg in (1, -1):
        q = [p0 + sd * sg * w, p1 + sd * sg * w, p1 + Z * h, p0 + Z * h]
        m.add(q, tile, sub_, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(sd * sg + Z))


def dormer(m, x0, a, H, zR, xd, w, hf, roof_tile, face_tile, hatch=True, smooth=False, bx=1.0):
    """Kvist paa +X-taghalvdelen. x0 = rygningens x, a = halvdybde, xd = frontens afstand fra rygning."""
    def sz(xl):
        return H + (zR - H) * (1 - xl / a)
    z0 = sz(xd)
    zE = z0 + hf
    xe = a * (1 - (zE - H) / (zR - H))
    zr = zE + 0.9
    xb = a * (1 - (zr - H) / (zR - H))
    P = lambda xl, y, z: V((x0 + xl, y, z))
    fsub = FULL if face_tile != "wood_stone" else WOOD
    front = [P(xd, -w, z0 - 0.15), P(xd, w, z0 - 0.15), P(xd, w, zE), P(xd, -w, zE)]
    m.add(front, face_tile, fsub, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(1, 0, 0))
    if hatch:
        deco(m, V((x0 + xd, 0, (z0 + zE) / 2 - 0.07)), X, 0, 0, w * 1.5, hf - 0.25, "door_green", FULL, off=0.02)
    for sg in (1, -1):
        m.add([P(xd, sg * w, z0 - 0.15), P(xd, sg * w, zE), P(xe, sg * w, zE)], "wood_stone", WOOD, [(0, 0), (1, 0), (.5, 1)], hint=(0, sg, 0))
        q = [P(xd + 0.2, sg * (w + 0.2), zE - 0.1), P(xd + 0.2, 0, zr + 0.1), P(xb, 0, zr), P(xe, sg * w, zE)]
        m.add(q, roof_tile, FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0.5, sg, 1), smooth=smooth)
    m.add([P(xd + 0.2, -(w + 0.2), zE - 0.1), P(xd + 0.2, w + 0.2, zE - 0.1), P(xd + 0.2, 0, zr + 0.1)], face_tile, fsub, [(0, 0.6), (1, 0.6), (.5, 1)], hint=(1, 0, 0))
    if bx:
        obox(m, P(xd + 0.55, 0, zr - 0.05), (0.55, 0.07, 0.07), "wood_stone", WOOD)


def build_merchant():
    def extra(m, a, b, H, zR):
        deco(m, V((a, 0, 0)), X, 0, 0.5 + 1.9, 3.0, 3.8, "gateway", FULL, off=0.02)
        deco(m, V((-a, 0, 0)), -X, 0, 0.5 + 1.9, 3.0, 3.8, "gateway", FULL, off=0.02)
        dormer(m, 0.0, a, H, zR, 2.9, 0.85, 1.9, "tile", "ochre", hatch=True)
        for sg in (1, -1):
            chimney(m, 0.0, sg * (b - 0.9), zR - 0.8, zR + 1.6, 1.0, 1.0)
        for sg in (1, -1):
            obox(m, (a + 0.08, sg * (b - 0.05), 3.4), (0.04, 0.04, 3.4), "wood_stone", STONE)
    return make_house("SM_MerchantHouse", 12.0, 8.6, 7.2, 4.6, "ochre", "tile", 0.0, None,
                      {2.1: [-5.0, -3.0, 3.0, 5.0], 5.4: [-5.0, -3.0, -1.0, 1.0, 3.0, 5.0]},
                      {2.1: [-4.5, -1.5, 1.5, 4.5], 5.4: [-4.5, -1.5, 1.5, 4.5]},
                      {2.1: [-2.2, 2.2], 5.4: [-2.2, 2.2], 9.1: [0.0]}, None, chim=(None, 0), roof_ref=(9.0, 6.0),
                      win_size=(1.05, 1.6), extra=extra)


def build_cottage():
    return make_house("SM_Cottage", 10.0, 5.8, 2.4, 3.0, "timber_white", "thatch", 3.2, None,
                      {1.5: [1, 3, 6]}, {1.5: [1, 4, 6]}, {1.5: [1]}, (4, 1.85), chim=(0.3, 1.0), door_tile="door_green",
                      thatch=True, plinth_h=0.45, halfhip=True, win_size=(0.8, 1.0), oe=0.5, og=0.45, roof_ref=(9, 6))


def build_farm():
    m = Model("SM_Farm")
    Xo, Yo, w = 15.0, 17.5, 7.5
    Xi, Yi = Xo - w, Yo - w
    H = 2.9
    rise = 3.7
    zR = H + rise
    a = w / 2
    s = rise / a
    oe, ie = 0.6, 0.5
    xr, yr = Xo - a, Yo - a
    zo = H - s * oe
    zi = H - s * ie
    tile, ref = "thatch", (9.0, 6.0)
    for (cx, cy, hx, hy) in ((Xo, 0, 0.06, Yo + 0.06), (-Xo, 0, 0.06, Yo + 0.06), (0, Yo, Xo + 0.06, 0.06), (0, -Yo, Xo + 0.06, 0.06)):
        obox(m, (cx, cy, 0.2), (hx, hy, 0.2), "wood_stone", STONE, ref=(4, 0.4))
    outer = [(V((1, 0, 0)), V((Xo, 0, 0)), 2 * Yo), (V((-1, 0, 0)), V((-Xo, 0, 0)), 2 * Yo),
             (V((0, 1, 0)), V((0, Yo, 0)), 2 * Xo), (V((0, -1, 0)), V((0, -Yo, 0)), 2 * Xo)]
    inner = [(V((-1, 0, 0)), V((Xi, 0, 0)), 2 * Yi), (V((1, 0, 0)), V((-Xi, 0, 0)), 2 * Yi),
             (V((0, -1, 0)), V((0, Yi, 0)), 2 * Xi), (V((0, 1, 0)), V((0, -Yi, 0)), 2 * Xi)]
    for n, o, ln in outer + inner:
        wall_run(m, o, n, ln, 0, H, "timber_white", seg=3.2)

    def slope(p0, p1, p2, p3, hint):
        d = (p1 - p0).normalized()
        upv = p3 - p0
        upv = (upv - d * upv.dot(d)).normalized()
        m.add([p0, p1, p2, p3], tile, FULL, fit_uv([p0, p1, p2, p3], d, upv, ref[0], ref[1]), hint=hint, smooth=True)
    xo, yo = Xo + oe, Yo + oe
    for p0, p1, p2, p3, h in (
            (V((xo, -yo, zo)), V((xo, yo, zo)), V((xr, yr, zR)), V((xr, -yr, zR)), (1, 0, 1)),
            (V((-xo, yo, zo)), V((-xo, -yo, zo)), V((-xr, -yr, zR)), V((-xr, yr, zR)), (-1, 0, 1)),
            (V((xo, yo, zo)), V((-xo, yo, zo)), V((-xr, yr, zR)), V((xr, yr, zR)), (0, 1, 1)),
            (V((-xo, -yo, zo)), V((xo, -yo, zo)), V((xr, -yr, zR)), V((-xr, -yr, zR)), (0, -1, 1))):
        slope(p0, p1, p2, p3, h)
    xi, yi = Xi - ie, Yi - ie
    for p0, p1, p2, p3, h in (
            (V((xi, yi, zi)), V((xi, -yi, zi)), V((xr, -yr, zR)), V((xr, yr, zR)), (-1, 0, 1)),
            (V((-xi, -yi, zi)), V((-xi, yi, zi)), V((-xr, yr, zR)), V((-xr, -yr, zR)), (1, 0, 1)),
            (V((-xi, yi, zi)), V((xi, yi, zi)), V((xr, yr, zR)), V((-xr, yr, zR)), (0, -1, 1)),
            (V((xi, -yi, zi)), V((-xi, -yi, zi)), V((-xr, -yr, zR)), V((xr, -yr, zR)), (0, 1, 1))):
        slope(p0, p1, p2, p3, h)
    corners = [V((xr, yr, zR)), V((-xr, yr, zR)), V((-xr, -yr, zR)), V((xr, -yr, zR))]
    for i in range(4):
        thatch_ridge(m, corners[i], corners[(i + 1) % 4], 0.42, 0.4)

    def strip(pa, pb, hint):
        q = [pa - Z * 0.34, pb - Z * 0.34, pb, pa]
        m.add(q, "thatch", FULL, [(0, 0), (1, 0), (1, .15), (0, .15)], hint=hint)
    for (x0, y0, zz, sgn) in ((xo, yo, zo, 1), (xi, yi, zi, -1)):
        pts = [V((x0, -y0, zz)), V((x0, y0, zz)), V((-x0, y0, zz)), V((-x0, -y0, zz))]
        for i in range(4):
            pa, pb = pts[i], pts[(i + 1) % 4]
            strip(pa, pb, ((pa + pb) / 2).normalized() * sgn)
    dormer(m, xr, a, H, zR, 2.6, 1.3, 1.7, "thatch", "wood_stone", hatch=True, smooth=True)

    def row(n, o, items):
        for (s_, z_, w_, h_, t_) in items:
            deco(m, o, n, s_, z_, w_, h_, t_, WIN if t_ == "window" else FULL, off=0.02)
    W = lambda s_: (s_, 1.7, 0.8, 1.0, "window")
    D = lambda s_, t_="door_ox": (s_, 1.55, 1.3, 2.2, t_)
    G = lambda s_: (s_, 1.7, 1.5, 2.5, "door_green")
    row(V((1, 0, 0)), V((Xo, 0, 0)), [W(-14), W(-10), W(-6), W(-2.5), W(6), W(10), W(14), G(-1.0), G(1.0)])
    row(V((-1, 0, 0)), V((-Xo, 0, 0)), [D(-10), D(10), W(-4), W(0), W(4)])
    row(V((0, 1, 0)), V((0, Yo, 0)), [W(-8), W(-4), W(0), W(4), W(8), D(12)])
    row(V((0, -1, 0)), V((0, -Yo, 0)), [D(-10), D(-4), W(3), W(8)])
    row(V((-1, 0, 0)), V((Xi, 0, 0)), [W(-7), W(-3), W(3), W(7), G(-1.0), G(1.0), D(5.2), D(-5.2)])
    row(V((1, 0, 0)), V((-Xi, 0, 0)), [D(-7), D(-3), D(3), D(7)])
    row(V((0, -1, 0)), V((0, Yi, 0)), [W(-5), W(-2), W(2), W(5)])
    row(V((0, 1, 0)), V((0, -Yi, 0)), [D(-4), D(4), W(0)])
    chimney(m, -xr, 3.0, zR - 0.9, zR + 1.3, 1.0, 1.0, tile="white")
    for ix in (0, 1):
        for iy in (0, 1):
            x0, x1 = -Xi + ix * Xi, ix * Xi
            y0, y1 = -Yi + iy * Yi, iy * Yi
            m.add([V((x0, y0, 0.04)), V((x1, y0, 0.04)), V((x1, y1, 0.04)), V((x0, y1, 0.04))], "cobbles", FULL, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0, 1))
    m.add([V((Xo, -3.0, 0.035)), V((Xo + 4.0, -3.0, 0.035)), V((Xo + 4.0, 3.0, 0.035)), V((Xo, 3.0, 0.035))], "cobbles", FULL, [(0, 0), (.5, 0), (.5, .5), (0, .5)], hint=(0, 0, 1))
    # broend
    wx, wy = -1.5, 1.5
    n8 = 8
    r0, r1, hh = 0.85, 0.55, 0.9
    for i in range(n8):
        a0, a1 = (i + 0.5) * 2 * math.pi / n8, (i + 1.5) * 2 * math.pi / n8
        c0, c1 = V((math.cos(a0), math.sin(a0), 0)), V((math.cos(a1), math.sin(a1), 0))
        ctr = V((wx, wy, 0))
        P0, P1 = ctr + c0 * r0, ctr + c1 * r0
        Q0, Q1 = ctr + c0 * r1, ctr + c1 * r1
        m.add([P0 + Z * 0.04, P1 + Z * 0.04, P1 + Z * hh, P0 + Z * hh], "wood_stone", STONE, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(c0 + c1))
        m.add([P0 + Z * hh, P1 + Z * hh, Q1 + Z * hh, Q0 + Z * hh], "wood_stone", STONE, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=(0, 0, 1))
        m.add([ctr + Z * 0.4, Q1 + Z * 0.4, Q0 + Z * 0.4], "tar", FULL, [(0.5, 0.5), (1, 0.5), (1, 1)], hint=(0, 0, 1))
    for sg in (-1, 1):
        obox(m, (wx, wy + sg * 0.75, 1.1), (0.07, 0.07, 1.1), "wood_stone", WOOD)
    ax_ = V((math.cos(0.5), 0, math.sin(0.5)))
    az_ = V((-math.sin(0.5), 0, math.cos(0.5)))
    obox(m, (wx + 0.2, wy, 2.15), (1.7, 0.05, 0.06), "wood_stone", WOOD, axes=(ax_, Y, az_))
    # goedningsbunke
    dx, dy = 4.5, -5.0
    rings = [(1.9, 0.0), (1.7, 0.45), (1.0, 0.85), (0.0, 1.05)]
    for k in range(len(rings) - 1):
        ra, za = rings[k]
        rb, zb = rings[k + 1]
        for i in range(n8):
            a0, a1 = i * 2 * math.pi / n8, (i + 1) * 2 * math.pi / n8
            p = [V((dx + ra * math.cos(a0), dy + ra * math.sin(a0) * 0.8, za)), V((dx + ra * math.cos(a1), dy + ra * math.sin(a1) * 0.8, za)),
                 V((dx + rb * math.cos(a1), dy + rb * math.sin(a1) * 0.8, zb)), V((dx + rb * math.cos(a0), dy + rb * math.sin(a0) * 0.8, zb))]
            uvs = [(i / n8, k / 3.0), ((i + 1) / n8, k / 3.0), ((i + 1) / n8, (k + 1) / 3.0), (i / n8, (k + 1) / 3.0)]
            if rb == 0:
                p = p[:3]
                uvs = uvs[:3]
            m.add(p, "thatch", FULL, uvs, hint=(p[0] + p[1]) / 2 - V((dx, dy, 0)) + Z * 0.3, smooth=True)
    return m


# ---------------------------------------------------------------------------------- kirke
def stepped_slab(m, y_out, y_in, sgn_out, hw=5.1, z0=15.5):
    prof = [(hw, z0), (hw, z0 + 1.7), (hw - 0.9, z0 + 1.7), (hw - 0.9, z0 + 3.3), (hw - 1.8, z0 + 3.3), (hw - 1.8, z0 + 4.8),
            (hw - 2.7, z0 + 4.8), (hw - 2.7, z0 + 6.1), (hw - 3.6, z0 + 6.1), (hw - 3.6, z0 + 6.9)]
    poly = [(-hw, z0)] + prof + [(-x, z) for (x, z) in reversed(prof)]
    for yv, sg in ((y_out, sgn_out), (y_in, -sgn_out)):
        pts = [V((x, yv, z)) for (x, z) in poly]
        m.add(pts, "white", FULL, fit_uv(pts, X, Z), hint=(0, sg, 0))
    n = len(poly)
    for i in range(1, n):
        a_, b_ = poly[i], poly[(i + 1) % n]
        dx, dz = b_[0] - a_[0], b_[1] - a_[1]
        q = [V((a_[0], y_out, a_[1])), V((b_[0], y_out, b_[1])), V((b_[0], y_in, b_[1])), V((a_[0], y_in, a_[1]))]
        m.add(q, "white", FULL, [(0, 0), (1, 0), (1, .1), (0, .1)], hint=(dz, 0, -dx))
    # smaa orange teglhaetter paa trinnenes afsatser
    ym = (y_out + y_in) / 2
    th = abs(y_out - y_in) / 2 + 0.08
    for (xa, za), (xb, zb) in zip(prof[1:], prof[2:]):
        if abs(za - zb) < 1e-6 and za > z0 + 0.1:
            xc = (xa + xb) / 2
            for sx in (1, -1):
                obox(m, (sx * xc, ym, za + 0.05), (abs(xa - xb) / 2 + 0.06, th, 0.05), "tile", FULL)
    zt = prof[-1][1]
    obox(m, (0, ym, zt + 0.05), (prof[-1][0] + 0.06, th, 0.05), "tile", FULL)


def arch_deco(m, o, n, s, z, w, h, off=0.02):
    deco(m, o, n, s, z, w, h, "window", ARCH, off=off)


def build_church():
    m = Model("SM_Church")
    ta, ty0, ty1, th, tR = 5.0, -15.0, -7.4, 15.5, 20.6
    na, ny0, ny1, nh, nR = 4.8, -7.4, 8.1, 6.5, 13.6
    ca, cy0, cy1, ch, cR = 4.0, 8.1, 15.0, 6.3, 11.6
    SL = 0.65
    tym, nym, cym = (ty0 + ty1) / 2, (ny0 + ny1) / 2, (cy0 + cy1) / 2
    PH = 0.5
    for sg in (1, -1):
        wall_run(m, V((sg * ta, tym, 0)), V((sg, 0, 0)), ty1 - ty0, 0, th, "white", plinth_h=PH)
        wall_run(m, V((sg * na, nym, 0)), V((sg, 0, 0)), ny1 - ny0, 0, nh, "white", plinth_h=PH)
        wall_run(m, V((sg * ca, cym, 0)), V((sg, 0, 0)), cy1 - cy0, 0, ch, "white", plinth_h=PH)
    wall_run(m, V((0, ty0, 0)), V((0, -1, 0)), 2 * ta, 0, th, "white", plinth_h=PH)
    wall_run(m, V((0, ty1, 0)), V((0, 1, 0)), 2 * ta, 0, th, "white", plinth_h=PH)
    for (a_, yv, hh, rr) in ((na, ny1, nh, nR), (ca, cy1, ch, cR)):
        n = V((0, 1, 0))
        o = V((0, yv, 0))
        wall_run(m, o, n, 2 * a_, 0, hh, "white", plinth_h=PH)
        r = right_of(n)
        m.add([o + r * (-a_) + Z * hh, o + r * a_ + Z * hh, o + Z * rr], "white", FULL, [(0, .6), (1, .6), (.5, .98)], hint=n)
    # taarnets egen saddeltag (roede tegl) mellem de to kamtakker, med rygningsbaand
    roof_gable(m, ta, ty0 + SL, ty1 - SL, th, tR, 0.3, "tile", ref=(8, 8), fas=0.14, ftile=("white", FULL), gfas=(False, False))
    roof_gable(m, na, ny0 + 0.02, ny1 + 0.4, nh, nR, 0.35, "tile", ref=(10, 8), fas=0.14, ftile=("white", FULL), gfas=(False, True))
    roof_gable(m, ca, cy0 + 0.02, cy1 + 0.35, ch, cR, 0.3, "tile", ref=(8, 7), fas=0.14, ftile=("white", FULL), gfas=(False, True))
    for (y0, y1, zr) in ((ty0 + SL, ty1 - SL, tR), (ny0, ny1 + 0.4, nR), (cy0, cy1 + 0.35, cR)):
        ridge_seg(m, V((0, y0, zr)), V((0, y1, zr)), 0.26, 0.22, "tile", FULL)
    stepped_slab(m, ty0, ty0 + SL, -1)
    stepped_slab(m, ty1, ty1 - SL, 1)
    # vinduer: alle paa vaegge. o har y=0 paa +/-X-flader, saa s = ren y-koordinat (x sg for -X-fladen)
    for sg in (1, -1):
        n = V((sg, 0, 0))
        for yv in (-4.6, -0.1, 4.4):
            arch_deco(m, n * na, n, sg * yv, 3.3, 0.9, 2.0)
        arch_deco(m, n * ca, n, sg * 11.5, 3.3, 0.9, 2.0)
        for yv in (tym - 1.5, tym + 1.5):
            arch_deco(m, n * ta, n, sg * yv, 13.0, 0.9, 2.0)
        arch_deco(m, n * ta, n, sg * tym, 8.8, 0.4, 1.3)
    for sx in (-1.8, 1.8):
        arch_deco(m, V((0, ty0, 0)), V((0, -1, 0)), sx, 13.0, 0.9, 2.0)
    arch_deco(m, V((0, cy1, 0)), V((0, 1, 0)), 0.0, 3.4, 0.9, 2.0)
    deco(m, V((0, ty0, 0)), V((0, -1, 0)), 0.0, 1.45, 1.7, 2.8, "door_green", FULL, off=0.02)
    return m


# ---------------------------------------------------------------------------------- moelle
def oct_pt(R, k, z):
    ang = math.radians(22.5 + 45 * k)
    return V((R * math.cos(ang), R * math.sin(ang), z))


def lathe_oct(m, rings, tile, sub_=FULL, vr=(0, 1), smooth=False):
    z0, zN = rings[0][1], rings[-1][1]
    for i in range(len(rings) - 1):
        (Ra, za), (Rb, zb) = rings[i], rings[i + 1]
        v0 = vr[0] + (za - z0) / max(zN - z0, 1e-6) * (vr[1] - vr[0])
        v1 = vr[0] + (zb - z0) / max(zN - z0, 1e-6) * (vr[1] - vr[0])
        for k in range(8):
            a_, b_ = oct_pt(Ra, k, za), oct_pt(Ra, k + 1, za)
            c_, d_ = oct_pt(Rb, k + 1, zb), oct_pt(Rb, k, zb)
            hint = (a_ + b_) / 2 * Vector((1, 1, 0))
            hint = hint.normalized() + Z * (Ra - Rb) * 0.1
            if Rb < 1e-6:
                m.add([a_, b_, V((0, 0, zb))], tile, sub_, [(0, v0), (1, v0), (.5, v1)], hint=hint, smooth=smooth)
            else:
                m.add([a_, b_, c_, d_], tile, sub_, [(0, v0), (1, v0), (1, v1), (0, v1)], hint=hint, smooth=smooth)


def ring_apo(rings):
    c22 = math.cos(math.radians(22.5))

    def f(z):
        for (Ra, za), (Rb, zb) in zip(rings[:-1], rings[1:]):
            if za <= z <= zb:
                return (Ra + (Rb - Ra) * (z - za) / (zb - za)) * c22
        return rings[-1][0] * c22
    return f


def mill_deco(m, k, s, z, w, h, tile, sub_, apo_fn, off=0.02):
    ang = math.radians(45 * k)
    n = V((math.cos(ang), math.sin(ang), 0))
    r = right_of(n)
    pts = []
    for (ds, dz) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        zz = z + dz * h / 2
        pts.append(n * (apo_fn(zz) + off) + r * (s + ds * w / 2) + Z * zz)
    m.add(pts, tile, sub_, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=n)


def build_windmill():
    m = Model("SM_Windmill")
    drum = [(4.7, 0.0), (4.64, 0.9), (4.4, 4.0)]
    body = [(3.5, 4.3), (2.55, 12.0)]
    dA, bA = ring_apo(drum), ring_apo(body)
    lathe_oct(m, drum[:2], "wood_stone", STONE)
    lathe_oct(m, drum[1:], "white", FULL, vr=(0.07, 0.95))
    # galleri: gulv + aaben rail (stolper og to haandlister, malet traae)
    gz0, gz1, gR = 4.0, 4.3, 6.0
    for k in range(8):
        for (zz, hint, Rin) in ((gz1, (0, 0, 1), 3.5), (gz0, (0, 0, -1), 4.4)):
            m.add([oct_pt(Rin, k, zz), oct_pt(Rin, k + 1, zz), oct_pt(gR, k + 1, zz), oct_pt(gR, k, zz)], "wood_stone", WOOD,
                  [(0, 0), (1, 0), (1, 1), (0, 1)], hint=hint)
    lathe_oct(m, [(gR, gz0), (gR, gz1)], "wood_stone", WOOD)
    rr = gR - 0.15
    for zc in (gz1 + 1.0, gz1 + 0.5):
        for k in range(8):
            a_, b_ = oct_pt(rr, k, zc), oct_pt(rr, k + 1, zc)
            q = [a_ - Z * 0.06, b_ - Z * 0.06, b_ + Z * 0.06, a_ + Z * 0.06]
            hint = (a_ + b_) * Vector((1, 1, 0))
            m.add(q, "wood_stone", WOOD, [(0, 0), (1, 0), (1, .2), (0, .2)], hint=hint)
            m.add(q, "wood_stone", WOOD, [(0, 0), (1, 0), (1, .2), (0, .2)], hint=-hint)
    for k in range(8):
        pc = oct_pt(rr, k, 0)
        ang = math.radians(22.5 + 45 * k)
        t = V((-math.sin(ang), math.cos(ang), 0))
        rad = V((math.cos(ang), math.sin(ang), 0))
        q = [V((pc.x, pc.y, gz1)) - t * 0.09, V((pc.x, pc.y, gz1)) + t * 0.09, V((pc.x, pc.y, gz1 + 1.05)) + t * 0.09, V((pc.x, pc.y, gz1 + 1.05)) - t * 0.09]
        m.add(q, "wood_stone", WOOD, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=rad)
        m.add(q, "wood_stone", WOOD, [(0, 0), (1, 0), (1, 1), (0, 1)], hint=-rad)
    # konsoller
    for k in range(8):
        ang = math.radians(45 * k)
        n = V((math.cos(ang), math.sin(ang), 0))
        pA = n * 4.3 + Z * 2.6
        pB = n * 5.7 + Z * 4.0
        d = (pB - pA)
        ex = d.normalized()
        ey = Z.cross(n).normalized()
        ez = ex.cross(ey)
        obox(m, (pA + pB) / 2, (d.length / 2, 0.11, 0.09), "wood_stone", WOOD, axes=(ex, ey, ez))
    lathe_oct(m, body, "tar", FULL)
    lathe_oct(m, [(2.9, 11.9), (2.45, 13.0), (1.3, 14.6), (0.0, 15.5)], "tar", FULL)
    lathe_oct(m, [(0.22, 15.45), (0.3, 15.75), (0.0, 16.2)], "wood_stone", WOOD)
    mill_deco(m, 0, 0, 1.35, 1.3, 2.4, "door_green", FULL, dA)
    for k in (1, 7, 3, 5):
        mill_deco(m, k, 0, 2.2, 0.8, 1.0, "window", WIN, dA)
    mill_deco(m, 0, 0, 5.45, 1.0, 2.0, "door_ox", FULL, bA)
    for zz in (7.6, 9.3, 11.0):
        mill_deco(m, 0, 0, zz, 0.6, 0.85, "window", WIN, bA)
    for k in (2, 6):
        mill_deco(m, k, 0, 7.5, 0.6, 0.85, "window", WIN, bA)
    # vinger: aksel, to krydsende stokke, fire gitter-/sejlfelter
    hz = 11.8
    obox(m, V((2.95, 0, hz)), (0.45, 0.32, 0.32), "wood_stone", WOOD)
    sx = 3.5
    for phi in (45, 135):
        d = V((0, math.cos(math.radians(phi)), math.sin(math.radians(phi))))
        ex, ey = d, X
        ez = ex.cross(ey)
        obox(m, V((sx, 0, hz)), (9.8, 0.14, 0.18), "wood_stone", WOOD, axes=(ex, ey, ez), nobottom=False)
    for kk in range(4):
        phi = math.radians(45 + 90 * kk)
        d = V((0, math.cos(phi), math.sin(phi)))
        p = V((0, -math.sin(phi), math.cos(phi)))
        c0 = V((sx + 0.16, 0, hz)) + d * 1.8 + p * 0.15
        c1 = c0 + d * 7.0
        quad = [c0, c1, c1 + p * 2.4, c0 + p * 2.4]
        m.add(quad, "sail", FULL, [(0, 0), (0, 1), (1, 1), (1, 0)], hint=X)
        m.add([q - X * 0.03 for q in quad], "sail", FULL, [(0, 0), (0, 1), (1, 1), (1, 0)], hint=-X)
    return m


# ================================================================================================
BUILDERS = [("TownHouse_Ochre", build_ochre), ("TownHouse_White", build_white), ("TownHouse_Timber", build_timber),
            ("MerchantHouse", build_merchant), ("Cottage", build_cottage), ("Farm", build_farm),
            ("Church", build_church), ("Windmill", build_windmill)]
BUDGET = {"TownHouse_Ochre": 400, "TownHouse_White": 400, "TownHouse_Timber": 400, "Cottage": 400, "MerchantHouse": 600,
          "Farm": 900, "Church": 700, "Windmill": 600}


def make_material(img):
    mat = bpy.data.materials.new("M_SceneryDK_Atlas")
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = img
    tex.interpolation = "Linear"
    tex.extension = "EXTEND"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.92
    for key in ("Specular IOR Level", "Specular"):
        if key in bsdf.inputs:
            bsdf.inputs[key].default_value = 0.1
            break
    return mat


def fbx_bbox_cm(path):
    from io_scene_fbx import parse_fbx
    root, _ = parse_fbx.parse(path)
    pts = []
    unit = []

    def walk(e):
        if e.id == b"Vertices":
            pts.append(np.array(e.props[0], dtype=float).reshape(-1, 3))
        if e.id == b"P" and e.props and e.props[0] == b"UnitScaleFactor":
            unit.append(e.props[4])
        for c in e.elems:
            walk(c)
    walk(root)
    allp = np.vstack(pts)
    return allp.min(axis=0), allp.max(axis=0), (unit[0] if unit else None)


def setup_render(sc, w, h):
    for eng in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT", "BLENDER_WORKBENCH"):
        try:
            sc.render.engine = eng
            break
        except TypeError:
            continue
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.render.resolution_percentage = 100
    sc.render.image_settings.file_format = "PNG"
    sc.view_settings.view_transform = "Standard"
    try:
        sc.eevee.taa_render_samples = 64
    except Exception:
        pass
    return sc.render.engine


def look_at_cam(cam, target, el_deg, az_deg, dist=400.0):
    el, az = math.radians(el_deg), math.radians(az_deg)
    pos = Vector(target) + Vector((math.cos(el) * math.cos(az), math.cos(el) * math.sin(az), math.sin(el))) * dist
    cam.location = pos
    cam.rotation_euler = (Vector(target) - pos).to_track_quat("-Z", "Y").to_euler()


def render_view(sc, cam, target, ortho, out, el=40.0, az=28.0):
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = ortho
    cam.data.clip_start, cam.data.clip_end = 1.0, 2000.0
    look_at_cam(cam, target, el, az)
    sc.camera = cam
    sc.render.filepath = out
    bpy.ops.render.render(write_still=True)


def main():
    stamp = time.strftime("%Y%m%d-%H%M%S")
    os.makedirs(OUT, exist_ok=True)
    olds = [f for f in os.listdir(OUT) if f.lower().endswith((".fbx", ".png", ".blend")) and not os.path.isdir(OUT + "/" + f)]
    if olds:
        bk = f"{OUT}/_backup_{stamp}"
        os.makedirs(bk)
        for f in olds + (["README.md"] if os.path.exists(OUT + "/README.md") else []):
            shutil.copy2(OUT + "/" + f, bk + "/" + f)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 1.0
    img = build_atlas()
    mat = make_material(img)
    objs = {}
    stats = {}
    for key, fn in BUILDERS:
        m = fn()
        ob = m.to_object(mat)
        ob.name = "SM_" + key
        objs[key] = ob
    for key, ob in objs.items():
        me = ob.data
        tris = sum(len(p.vertices) - 2 for p in me.polygons)
        bb = [ob.matrix_world @ Vector(c) for c in ob.bound_box]
        lo = Vector((min(b.x for b in bb), min(b.y for b in bb), min(b.z for b in bb)))
        hi = Vector((max(b.x for b in bb), max(b.y for b in bb), max(b.z for b in bb)))
        stats[key] = (tris, lo, hi)
    # FBX
    for key, ob in objs.items():
        bpy.ops.object.select_all(action="DESELECT")
        ob.select_set(True)
        bpy.context.view_layer.objects.active = ob
        bpy.ops.export_scene.fbx(
            filepath=f"{OUT}/SM_{key}.fbx", use_selection=True, object_types={"MESH"}, global_scale=1.0,
            apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", use_space_transform=True, bake_space_transform=True,
            axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE", use_mesh_modifiers=True, add_leaf_bones=False,
            use_custom_props=False, path_mode="AUTO", embed_textures=False, use_tspace=False)
    print("=== STATISTIK ===")
    for key, (tris, lo, hi) in stats.items():
        lo_c, hi_c, unit = fbx_bbox_cm(f"{OUT}/SM_{key}.fbx")
        ext = hi_c - lo_c
        print(f"{key}: tris={tris} (budget {BUDGET[key]}) FBX-bbox cm X*Y*Z = {ext[0]:.0f} x {ext[1]:.0f} x {ext[2]:.0f}  "
              f"min=({lo_c[0]:.0f},{lo_c[1]:.0f},{lo_c[2]:.0f}) max=({hi_c[0]:.0f},{hi_c[1]:.0f},{hi_c[2]:.0f}) unit={unit}")
    # layout til blend + preview
    back_x, front_x = -26.0, 14.0
    place = {"TownHouse_Ochre": (back_x, -40), "TownHouse_White": (back_x, -24), "TownHouse_Timber": (back_x, -8),
             "MerchantHouse": (back_x, 10), "Cottage": (front_x, -52), "Farm": (front_x - 4, -22),
             "Church": (front_x, 28), "Windmill": (front_x, 62)}
    for key, (px, py) in place.items():
        objs[key].location = (px, py, 0)
    # grund
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=300.0)
    gm = bpy.data.meshes.new("Ground")
    bm.to_mesh(gm)
    bm.free()
    ground = bpy.data.objects.new("PreviewGround", gm)
    sc.collection.objects.link(ground)
    gmat = bpy.data.materials.new("PreviewGreen")
    gmat.use_nodes = True
    gb = gmat.node_tree.nodes["Principled BSDF"]
    gb.inputs["Base Color"].default_value = (0.20, 0.34, 0.09, 1)
    gb.inputs["Roughness"].default_value = 1.0
    gm.materials.append(gmat)
    # lys
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
    bg.inputs["Color"].default_value = (0.62, 0.72, 0.88, 1)
    bg.inputs["Strength"].default_value = 1.0
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    sc.collection.objects.link(cam)
    eng = setup_render(sc, 1600, 900)
    print("RENDER ENGINE", eng)
    for ob in objs.values():
        ob.select_set(False)
    bpy.ops.wm.save_as_mainfile(filepath=f"{OUT}/scenery_models.blend", relative_remap=False)
    img_path = ATLAS
    render_view(sc, cam, (0, 2, 4), 140.0, f"{OUT}/scenery_models_preview.png")
    # naerbillede: kirke + cottage + moelle
    keep = {"Church": (0, -8), "Cottage": (6, 20), "Windmill": (6, 44)}
    saved = {k: objs[k].location.copy() for k in objs}
    for k, o in objs.items():
        if k not in keep:
            o.hide_render = True
    for k, (px, py) in keep.items():
        objs[k].location = (px, py, 0)
    objs["MerchantHouse"].rotation_euler = (0, 0, 0)
    render_view(sc, cam, (0, 14, 6), 80.0, f"{OUT}/scenery_models_closeup.png")
    for k, o in objs.items():
        o.hide_render = False
        o.location = saved[k]
    return stats


if __name__ == "__main__":
    main()
