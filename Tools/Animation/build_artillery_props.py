"""Bygger tre artilleri-props (kanonkugle, ladestok, visker) som lavpoly spilmodeller.

Koeres headless:
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --python Tools/Animation/build_artillery_props.py

Ingen import i Unreal. Output: SourceAssets/Artillery/Props/
  SM_Cannonball_12pdr / SM_Rammer / SM_Sponge  (.fbx, .blend, BaseColor/Roughness PNG)
  props_preview.png, props_detail.png, README.md

Enheder: 1 Blender-enhed = 1 m. FBX eksporteres med apply_unit_scale, saa Unreal faar cm.
Maal der er skoen (ikke verificeret kilde) er markeret ESTIMAT.
"""
import math
import os
import shutil
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

REPO = "C:/Projekter/1864-Campaign"
OUT = REPO + "/SourceAssets/Artillery/Props"
REF = "R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView/Items/"
PHOTO_BALL = REF + "Vejrbidt jernkanonkugle i n\u00e6rbillede.png"
PHOTO_ROD = REF + "Ladestok.png"
PHOTO_SPONGE = REF + "Visker.png"

BALL_DIAM_CM = 11.7      # ESTIMAT: dansk 12-pds glatloebet kugle
ROD_LEN_CM = 200.0       # ESTIMAT: ladestoklaengde
SPONGE_LEN_CM = 200.0    # ESTIMAT: viskerstok inkl. hoved
SHAFT_SPONGE_CM = 4.0    # skaftdiameter (opgave)
HEAD_SPONGE_CM = 14.0    # hoveddiameter (opgave)
SEGS = 16                # radiale segmenter paa drejede props

STATS = {}


# ----------------------------------------------------------------- billeder
def load_rgb(path):
    """Returnerer float32 (H,W,3) 0..1, raa vaerdier (sRGB-kodet), rakke foerst (top foerst)."""
    img = bpy.data.images.load(path)
    img.colorspace_settings.name = "Non-Color"
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)[::-1, :, :3].copy()


def save_png(arr_top_first, path, srgb):
    h, w = arr_top_first.shape[:2]
    if arr_top_first.ndim == 2:
        arr_top_first = np.repeat(arr_top_first[:, :, None], 3, axis=2)
    rgba = np.ones((h, w, 4), dtype=np.float32)
    rgba[:, :, :3] = np.clip(arr_top_first, 0, 1)
    img = bpy.data.images.new(os.path.basename(path), w, h, alpha=False)
    img.colorspace_settings.name = "Non-Color"  # skriv raa vaerdier uden konvertering
    img.pixels.foreach_set(rgba[::-1].ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def bilerp(img, x, y):
    h, w = img.shape[:2]
    x = np.clip(x, 0, w - 1.001)
    y = np.clip(y, 0, h - 1.001)
    x0 = np.floor(x).astype(int)
    y0 = np.floor(y).astype(int)
    fx = (x - x0)[..., None]
    fy = (y - y0)[..., None]
    a = img[y0, x0] * (1 - fx) + img[y0, x0 + 1] * fx
    b = img[y0 + 1, x0] * (1 - fx) + img[y0 + 1, x0 + 1] * fx
    return a * (1 - fy) + b * fy


def resize(img, w, h):
    two = img.ndim == 2
    if two:
        img = img[:, :, None]
    img = fft_blur(img, 0.5 * img.shape[0] / h)  # kun brugt paa tileable data (periodisk)
    ys = (np.arange(h) + 0.5) * img.shape[0] / h - 0.5 + 2
    xs = (np.arange(w) + 0.5) * img.shape[1] / w - 0.5 + 2
    img = np.pad(img, ((2, 2), (2, 2), (0, 0)), mode="wrap")  # tileable: ingen kantfejl
    X, Y = np.meshgrid(xs, ys)
    out = bilerp(img, X, Y)
    return out[:, :, 0] if two else out


def fft_blur(img, sigma, sigma_y=None):
    """Periodisk gaussisk sløring (img er tileable)."""
    h, w = img.shape[:2]
    sy = sigma if sigma_y is None else sigma_y
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.fftfreq(w)[None, :]
    k = np.exp(-2 * (math.pi ** 2) * ((sigma * fx) ** 2 + (sy * fy) ** 2))
    if img.ndim == 2:
        return np.real(np.fft.ifft2(np.fft.fft2(img) * k))
    return np.stack([np.real(np.fft.ifft2(np.fft.fft2(img[:, :, c]) * k)) for c in range(img.shape[2])], axis=2)


def make_tileable(img):
    h, w = img.shape[:2]
    wx = np.sin(np.pi * (np.arange(w) + 0.5) / w) ** 2
    wy = np.sin(np.pi * (np.arange(h) + 0.5) / h) ** 2
    m = (wy[:, None] * wx[None, :])[:, :, None]
    shifted = np.roll(np.roll(img, h // 2, axis=0), w // 2, axis=1)
    out = img * m + shifted * (1 - m)
    # gendan kontrast (blandingen daemper varians)
    mu = img.mean(axis=(0, 1))
    sd = img.std(axis=(0, 1)) / np.maximum(out.std(axis=(0, 1)), 1e-6)
    return (out - mu) * sd + mu


# ------------------------------------------------------------------- scene
def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    s = bpy.context.scene
    s.unit_settings.system = "METRIC"
    s.unit_settings.scale_length = 1.0
    s.unit_settings.length_unit = "CENTIMETERS"


def make_material(name, base_path, rough_path):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    tb = nt.nodes.new("ShaderNodeTexImage")
    tb.image = bpy.data.images.load(base_path)
    tb.image.colorspace_settings.name = "sRGB"
    tr = nt.nodes.new("ShaderNodeTexImage")
    tr.image = bpy.data.images.load(rough_path)
    tr.image.colorspace_settings.name = "Non-Color"
    tb.location = (-500, 150)
    tr.location = (-500, -150)
    nt.links.new(tb.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(tr.outputs["Color"], bsdf.inputs["Roughness"])
    bsdf.inputs["Metallic"].default_value = 0.0
    return mat


def finish_mesh(bm, name, mat):
    """Triangulering, glat shading med haard kant over 40 grader, UV-lag bevares."""
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method="FIXED", ngon_method="BEAUTY")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    lim = math.radians(40)
    for e in bm.edges:
        if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > lim:
            e.smooth = False
        else:
            e.smooth = True
    for f in bm.faces:
        f.smooth = True
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(mat)
    return ob


def tri_count(ob):
    ob.data.calc_loop_triangles()
    return len(ob.data.loop_triangles)


def apply_all(ob):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


# --------------------------------------------------------------------- kugle
def build_cannonball():
    reset_scene()
    photo = load_rgb(PHOTO_BALL)
    h, w = photo.shape[:2]
    # kuglens centrum/radius ud fra fotoet (ca. 627,627 / 560 px)
    cx, cy, R = 627, 627, 560
    half = 400  # 0,71 R: lav forvraengning
    patch = photo[cy - half:cy + half, cx - half:cx + half]
    lum_raw = patch @ np.array([0.299, 0.587, 0.114], dtype=np.float32)
    low = fft_blur(patch, 45)
    patch = np.clip(patch / np.maximum(low, 0.02) * patch.mean(axis=(0, 1)), 0, 1)  # fjern kuglens skygge/glans
    patch = patch[40:-40, 40:-40]       # skaer kant med sloerings-artefakter vaek
    lum_raw = lum_raw[40:-40, 40:-40]
    patch = make_tileable(patch)
    lum_raw = make_tileable(lum_raw[:, :, None])[:, :, 0]
    flat = np.clip(patch, 0, 1)
    # roughness: hoej, lidt lavere paa blanke pletter (lys over lokalt gennemsnit)
    hp = lum_raw - fft_blur(lum_raw, 25)
    hp = np.clip(hp / (3 * hp.std() + 1e-6), -1, 1)
    rough = 0.88 - 0.16 * np.clip(hp, 0, 1) + 0.04 * np.clip(-hp, 0, 1)
    # 1024x1024 tekstur: 4 x 2 fliser (UV-kugle: u = 36,8 cm, v = 18,4 cm), jf. README
    tw, th = 256, 512
    base_tile = resize(flat, tw, th)
    rough_tile = resize(rough, tw, th)
    base = np.tile(base_tile, (2, 4, 1))
    rgh = np.tile(rough_tile, (2, 4))
    bp = OUT + "/T_Cannonball_12pdr_BaseColor.png"
    rp = OUT + "/T_Cannonball_12pdr_Roughness.png"
    save_png(base, bp, True)
    save_png(rgh, rp, False)
    STATS["ball_tex"] = (1024, 1024)
    mat = make_material("M_Cannonball_12pdr", bp, rp)

    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=24, v_segments=12, radius=BALL_DIAM_CM / 200.0, calc_uvs=True)
    ob = finish_mesh(bm, "SM_Cannonball_12pdr", mat)
    apply_all(ob)
    return ob


# ------------------------------------------------------- drejet profil (foto)
def trace_profile(photo, thr=45, clamp_from=None, clamp_fn=None):
    """Pr. raekke: venstre/hoejre kant af objektet mod baggrund. Returnerer y0,y1,radius(px),cx(px)."""
    a = photo * 255.0
    H, W, _ = a.shape
    bg = np.concatenate([a[:, :100], a[:, -100:]], axis=1).mean(axis=1)
    d = np.sqrt(((a - bg[:, None, :]) ** 2).sum(2))
    m = d > thr
    rows = np.where(m.sum(1) > 3)[0]
    y0, y1 = int(rows.min()), int(rows.max())
    ys = np.arange(y0, y1 + 1)
    left = np.zeros(len(ys))
    right = np.zeros(len(ys))
    for i, y in enumerate(ys):
        xs = np.where(m[y])[0]
        left[i], right[i] = xs.min(), xs.max()

    def med(v, k=7):
        p = np.pad(v, k // 2, mode="edge")
        return np.array([np.median(p[i:i + k]) for i in range(len(v))])

    left, right = med(left), med(right)
    r = (right - left + 1) / 2.0
    c = (right + left) / 2.0
    if clamp_from is not None:  # skygge paa gulvet forurener nederste raekker: brug haandsat afrunding
        i0 = clamp_from - y0
        c[i0:] = c[i0]
        r[i0:] = np.minimum(r[i0:], clamp_fn(ys[i0:]))
    return y0, y1, r, c


def rdp(pts, tol):
    pts = np.asarray(pts, dtype=float)
    keep = np.zeros(len(pts), bool)
    keep[0] = keep[-1] = True
    stack = [(0, len(pts) - 1)]
    while stack:
        a, b = stack.pop()
        if b <= a + 1:
            continue
        p, q = pts[a], pts[b]
        d = q - p
        n = np.hypot(*d)
        seg = pts[a + 1:b] - p
        dist = np.abs(seg[:, 0] * d[1] - seg[:, 1] * d[0]) / n if n > 0 else np.hypot(seg[:, 0], seg[:, 1])
        i = int(np.argmax(dist))
        if dist[i] > tol:
            keep[a + 1 + i] = True
            stack += [(a, a + 1 + i), (a + 1 + i, b)]
    return np.where(keep)[0]


def simplify(zr_cm, target_pts, tol0=0.05):
    """Mindste RDP-tolerance (cm) der giver <= target_pts profilpunkter."""
    tol = tol0
    while True:
        idx = rdp(zr_cm, tol)
        if len(idx) <= target_pts:
            return idx, tol
        tol *= 1.15


def lathe_mesh(prof, ys_rows, y0, y1, mat, name, segs=SEGS):
    """prof: liste (z_m, r_m, y_raekke). Rotationsflade om Z med egne UV (u rundt, v = fotoraekke)."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    n = len(prof)
    rings = []
    for z, r, y in prof:
        if r < 1e-7:
            rings.append([bm.verts.new((0, 0, z))] * 1)
        else:
            rings.append([bm.verts.new((r * math.cos(2 * math.pi * k / segs), r * math.sin(2 * math.pi * k / segs), z))
                          for k in range(segs)])

    def vv(y):
        return min(1.0, max(0.0, 1.0 - (y - y0) / float(y1 - y0)))

    for i in range(n - 1):
        A, Bv = rings[i], rings[i + 1]
        va, vb = vv(prof[i][2]), vv(prof[i + 1][2])
        for k in range(segs):
            k2 = (k + 1) % segs
            u0, u1 = k / segs, (k + 1) / segs
            if len(A) == 1:
                vs = [(A[0], (u0 + u1) / 2, va), (Bv[k], u0, vb), (Bv[k2], u1, vb)]
            elif len(Bv) == 1:
                vs = [(A[k], u0, va), (Bv[0], (u0 + u1) / 2, vb), (A[k2], u1, va)]
            else:
                vs = [(A[k], u0, va), (Bv[k], u0, vb), (Bv[k2], u1, vb), (A[k2], u1, va)]
            try:
                f = bm.faces.new([t[0] for t in vs])
            except ValueError:
                continue
            for lp, t in zip(f.loops, vs):
                lp[uv].uv = (t[1], t[2])
    # flade-normal udad: sikr orientering efter recalc
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def cyl_texture(photo, y0, y1, cx_row, r_row, tw, th, gain=0.25, ye0=None, ye1=None):
    """Cylindrisk projektion: front-halvdelen af fotoet foldes spejlet rundt om omkredsen (u=0,5 = fotoets midte)."""
    j = (np.arange(th) + 0.5) / th
    yy = ye0 + j * (ye1 - ye0)
    rows = np.arange(y0, y1 + 1)
    cxs = np.interp(yy, rows, cx_row)
    rs = np.interp(yy, rows, r_row) * 0.92
    u = (np.arange(tw) + 0.5) / tw
    th_a = 2 * math.pi * (u - 0.5)
    th_a = np.where(np.abs(th_a) > math.pi / 2, np.sign(th_a) * (math.pi - np.abs(th_a)), th_a)
    X = cxs[:, None] + rs[:, None] * (th_a * 2 / math.pi)[None, :]   # lineaer i vinkel: ingen udtrækning ved kanten
    Y = np.repeat(yy[:, None], tw, axis=1)
    tex = bilerp(photo, X, Y)
    # fladfelt: fjern bagt belysningsgradient paa tvaers (raekke-gennemsnit) og over stoerre omraader
    row_mean = fft_blur(tex, 1000.0, 20.0)
    loc = fft_blur(tex, 60.0, 20.0)
    tex = tex / np.maximum(loc, 0.02) * row_mean
    return np.clip(tex, 0, 1), yy


def lum(a):
    return a @ np.array([0.299, 0.587, 0.114], dtype=np.float32)


# --------------------------------------------------------------------- ladestok
def build_rammer():
    reset_scene()
    photo = load_rgb(PHOTO_ROD)
    y0, y1, r_px, cx_px = trace_profile(photo, thr=110)
    ye0, ye1 = y0 + 6, y1 - 6
    n_rows = y1 - y0 + 1
    s = ROD_LEN_CM / (n_rows - 1)           # cm pr. fotoraekke (foto-proportioner bevares)
    z_cm = (y1 - (np.arange(y0, y1 + 1))) * s
    r_cm = r_px * s
    zr = np.stack([z_cm, r_cm], axis=1)
    idx, tol = simplify(zr, 18)
    STATS["rammer_rdp_tol_cm"] = tol
    pivot = ROD_LEN_CM / 3.0                # grebspunkt 1/3 fra den nedre (spaltede) ende
    prof = []
    # luk begge ender med flad kappe via akse-punkt
    first, last = idx[0], idx[-1]
    prof.append(((z_cm[first] - pivot) / 100, 0.0, y0 + first))
    for i in idx:
        prof.append(((z_cm[i] - pivot) / 100, r_cm[i] / 100, y0 + i))
    prof.append(((z_cm[last] - pivot) / 100, 0.0, y0 + last))
    STATS["rammer_shaft_cm"] = float(np.median(r_cm) * 2)
    STATS["rammer_profile"] = [(round(p[0] * 100, 1), round(p[1] * 100, 2)) for p in prof]

    # tekstur 256 x 2048
    tex, yy = cyl_texture(photo, y0, y1, cx_px, r_px, 256, 2048, ye0=ye0, ye1=ye1)
    base = tex
    lm = lum(tex)
    hp = lm - fft_blur(lm, 6)
    k = np.clip(hp / (3 * hp.std() + 1e-6), -1, 1)
    rough = 0.78 - 0.1 * k
    bp = OUT + "/T_Rammer_BaseColor.png"
    rp = OUT + "/T_Rammer_Roughness.png"
    save_png(base, bp, True)
    save_png(np.clip(rough, 0.6, 0.9), rp, False)
    STATS["rammer_tex"] = (256, 2048)
    mat = make_material("M_Rammer", bp, rp)

    bm = lathe_mesh(prof, None, ye0, ye1, mat, "SM_Rammer")
    # slids i den nedre ende: kasse gennem hele diameteren langs X
    ob = finish_mesh(bm, "SM_Rammer", mat)
    zb = (z_cm[last] - pivot) / 100          # nederste z (m)
    slot = bpy.data.objects.new("slot", bpy.data.meshes.new("slot"))
    sb = bmesh.new()
    bmesh.ops.create_cube(sb, size=1.0)
    sb.to_mesh(slot.data)
    sb.free()
    bpy.context.scene.collection.objects.link(slot)
    slot.scale = (0.2, 0.009, 0.09)                          # 20 cm x 0,9 cm x 9 cm
    slot.location = (0, 0, zb + 0.09 / 2 - 0.005)           # 8,5 cm dyb slids
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action="DESELECT")
    slot.select_set(True)
    bpy.context.view_layer.objects.active = slot
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    mod = ob.modifiers.new("slot", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.object = slot
    mod.solver = "EXACT"
    bpy.context.view_layer.objects.active = ob
    bpy.ops.object.modifier_apply(modifier="slot")
    bpy.data.objects.remove(slot)
    # slidsvaegge har ingen UV fra kasse: peg paa et moerkt teksel ved den nedre ende
    me = ob.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uvl = bm.loops.layers.uv.active
    for f in bm.faces:
        if all(abs(l[uvl].uv.x) < 1e-6 and abs(l[uvl].uv.y) < 1e-6 for l in f.loops) or \
                all(l[uvl].uv.length < 1e-6 for l in f.loops):
            for l in f.loops:
                l[uvl].uv = (0.5, 0.03)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method="BEAUTY", ngon_method="BEAUTY")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    lim = math.radians(40)
    for e in bm.edges:
        e.smooth = not (len(e.link_faces) == 2 and e.calc_face_angle(0.0) > lim)
    for f in bm.faces:
        f.smooth = True
    bm.to_mesh(me)
    bm.free()
    apply_all(ob)
    return ob


# ----------------------------------------------------------------------- visker
def build_sponge():
    reset_scene()
    photo = load_rgb(PHOTO_SPONGE)
    ball_fn = lambda yv: 32.0 * np.sqrt(np.clip(1 - ((yv - 1440.0) / 37.0) ** 2, 0, 1)) + 0.5
    y0, y1, r_px, cx_px = trace_profile(photo, thr=80, clamp_from=1440, clamp_fn=ball_fn)
    ye0, ye1 = y0 + 6, y1 - 6
    n_rows = y1 - y0 + 1
    s = SPONGE_LEN_CM / (n_rows - 1)
    ys = np.arange(y0, y1 + 1)
    z_cm = (y1 - ys) * s
    # radius: hoved og skaft skaleres hver for sig til opgavens maal (fotoets skaft er proportionelt tykkere)
    Y_HEAD_END = 362   # foto: overgang uld -> messingringen
    head_px = np.median(r_px[(ys > 90) & (ys < 330)]) * 2
    shaft_px = np.median(r_px[(ys > 520) & (ys < 1100)]) * 2
    k_head = HEAD_SPONGE_CM / head_px
    k_shaft = SHAFT_SPONGE_CM / shaft_px
    r_cm = np.where(ys < Y_HEAD_END, r_px * k_head, r_px * k_shaft)
    # kaleten foran/under messingringen: ringen (59 px) maa ikke blive tyndere end skaftet
    zr = np.stack([z_cm, r_cm], axis=1)
    idx, tol = simplify(zr, 34)
    # sikr et haardt trin ved ulden/ringen (to punkter ved samme foto-overgang)
    idx = sorted(set(idx.tolist()) | {int(Y_HEAD_END - y0) - 1, int(Y_HEAD_END - y0) + 2})
    STATS["sponge_rdp_tol_cm"] = tol
    pivot = SPONGE_LEN_CM / 3.0
    prof = [((z_cm[idx[0]] - pivot) / 100, 0.0, y0 + idx[0])]
    for i in idx:
        prof.append(((z_cm[i] - pivot) / 100, r_cm[i] / 100, y0 + i))
    prof.append(((z_cm[idx[-1]] - pivot) / 100, 0.0, y0 + idx[-1]))
    STATS["sponge_shaft_cm"] = float(shaft_px * k_shaft)
    STATS["sponge_head_cm"] = float(head_px * k_head)
    STATS["sponge_profile"] = [(round(p[0] * 100, 1), round(p[1] * 100, 2)) for p in prof]

    tw, th = 512, 2048
    tex, yy = cyl_texture(photo, y0, y1, cx_px, r_px, tw, th, ye0=ye0, ye1=ye1)
    lm = lum(tex)
    hp = lm - fft_blur(lm, 4)
    k = np.clip(hp / (3 * hp.std() + 1e-6), -1, 1)
    rough = np.empty((th, tw), dtype=np.float32)
    for j, y in enumerate(yy):
        if y < Y_HEAD_END:
            r0, amp = 0.96, 0.03       # uld
        elif y < 450:
            r0, amp = 0.40, 0.18       # messingring
        elif y < 1310:
            r0, amp = 0.72, 0.12       # traeskaft
        elif y < 1385:
            r0, amp = 0.40, 0.18       # nedre messingring
        else:
            r0, amp = 0.38, 0.18       # messingkugle
        rough[j] = r0 - 0.5 * amp * k[j]
    bp = OUT + "/T_Sponge_BaseColor.png"
    rp = OUT + "/T_Sponge_Roughness.png"
    save_png(tex, bp, True)
    save_png(np.clip(rough, 0.2, 1.0), rp, False)
    STATS["sponge_tex"] = (tw, th)
    mat = make_material("M_Sponge", bp, rp)

    bm = lathe_mesh(prof, None, ye0, ye1, mat, "SM_Sponge")
    ob = finish_mesh(bm, "SM_Sponge", mat)
    apply_all(ob)
    return ob


# -------------------------------------------------------------------- eksport
def export_prop(ob, name):
    blend = f"{OUT}/{name}.blend"
    fbx = f"{OUT}/{name}.fbx"
    bpy.ops.wm.save_as_mainfile(filepath=blend, relative_remap=True)
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(
        filepath=fbx, use_selection=True, object_types={"MESH"}, global_scale=1.0,
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", use_space_transform=True,
        bake_space_transform=True, axis_forward="-Y", axis_up="Z", mesh_smooth_type="EDGE",
        use_mesh_modifiers=True, add_leaf_bones=False, use_custom_props=False, path_mode="AUTO",
        embed_textures=False, use_tspace=False)
    STATS[name + "_tris"] = tri_count(ob)
    STATS[name + "_verts"] = len(ob.data.vertices)
    STATS[name + "_sharp"] = sum(1 for e in ob.data.edges if e.use_edge_sharp)


def fbx_bbox(path):
    """Laeser raa FBX-vertices (som Unreal ser dem) + UnitScaleFactor."""
    from io_scene_fbx import parse_fbx
    root, _ = parse_fbx.parse(path)
    pts = None
    unit = None

    def walk(e):
        nonlocal pts, unit
        if e.id == b"Vertices" and pts is None:
            pts = np.array(e.props[0], dtype=float).reshape(-1, 3)
        if e.id == b"P" and e.props and e.props[0] == b"UnitScaleFactor":
            unit = e.props[4]
        if e.id == b"P" and e.props and e.props[0] in (b"Lcl Scaling", b"Lcl Rotation"):
            print("FBXPROP", path, e.props[0], e.props[4:7])
        for c in e.elems:
            walk(c)
    walk(root)
    lo, hi = pts.min(axis=0), pts.max(axis=0)
    return lo, hi, unit


# --------------------------------------------------------------------- preview
def render_preview():
    reset_scene()
    names = ["SM_Cannonball_12pdr", "SM_Rammer", "SM_Sponge"]
    objs = []
    for n in names:
        with bpy.data.libraries.load(f"{OUT}/{n}.blend", link=False) as (src, dst):
            dst.objects = [n]
        o = bpy.data.objects[n]
        bpy.context.scene.collection.objects.link(o)
        objs.append(o)
    ball, ram, spo = objs
    ball.location = (-0.45, 0, BALL_DIAM_CM / 200.0)
    ram.location = (0.05, 0, ROD_LEN_CM / 300.0)
    spo.location = (0.55, 0, SPONGE_LEN_CM / 300.0)
    sc = bpy.context.scene
    # gulv + baggrund
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=4.0)
    fl = bpy.data.meshes.new("floor")
    bm.to_mesh(fl)
    bm.free()
    flo = bpy.data.objects.new("floor", fl)
    sc.collection.objects.link(flo)
    fm = bpy.data.materials.new("floor")
    fm.use_nodes = True
    fm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.30, 0.30, 0.30, 1)
    fm.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
    flo.data.materials.append(fm)
    # 1 m maalestok
    def box(name, size, loc, col):
        b = bmesh.new()
        bmesh.ops.create_cube(b, size=1.0)
        m = bpy.data.meshes.new(name)
        b.to_mesh(m)
        b.free()
        o = bpy.data.objects.new(name, m)
        o.scale = size
        o.location = loc
        mt = bpy.data.materials.new(name)
        mt.use_nodes = True
        mt.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = col
        mt.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.8
        o.data.materials.append(mt)
        sc.collection.objects.link(o)
    for i in range(10):
        c = (0.9, 0.9, 0.9, 1) if i % 2 == 0 else (0.05, 0.05, 0.05, 1)
        box(f"bar{i}", (0.03, 0.03, 0.1), (-1.25, 0, 0.05 + 0.1 * i), c)
    box("tick0", (0.15, 0.02, 0.01), (-1.25, 0, 0.005), (1, 1, 1, 1))
    box("tick1", (0.15, 0.02, 0.01), (-1.25, 0, 1.0), (1, 1, 1, 1))
    tcu = bpy.data.curves.new("lbl", "FONT")
    tcu.body = "1 m"
    tcu.size = 0.09
    to = bpy.data.objects.new("lbl", tcu)
    to.location = (-1.17, -0.03, 0.96)
    to.rotation_euler = (math.radians(90), 0, 0)
    tm = bpy.data.materials.new("lblm")
    tm.use_nodes = True
    tm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (1, 1, 1, 1)
    to.data.materials.append(tm)
    sc.collection.objects.link(to)
    # lys
    ld = bpy.data.lights.new("sun", "SUN")
    ld.energy = 2.0
    ld.angle = math.radians(20)
    lo = bpy.data.objects.new("sun", ld)
    lo.rotation_euler = (math.radians(55), 0, math.radians(-30))
    sc.collection.objects.link(lo)
    ld2 = bpy.data.lights.new("fill", "SUN")
    ld2.energy = 0.6
    lo2 = bpy.data.objects.new("fill", ld2)
    lo2.rotation_euler = (math.radians(70), 0, math.radians(50))
    sc.collection.objects.link(lo2)
    sc.world = bpy.data.worlds.new("w")
    sc.world.use_nodes = True
    sc.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.30, 0.30, 0.30, 1)
    sc.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    sc.view_settings.view_transform = "Standard"
    # kamera
    cd = bpy.data.cameras.new("cam")
    cd.type = "ORTHO"
    cam = bpy.data.objects.new("cam", cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    cam.rotation_euler = (math.radians(90), 0, 0)
    sc.render.resolution_x, sc.render.resolution_y = 1200, 800
    sc.render.film_transparent = False
    sc.render.image_settings.file_format = "PNG"
    for eng in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT", "BLENDER_WORKBENCH"):
        try:
            sc.render.engine = eng
            break
        except TypeError:
            continue
    STATS["render_engine"] = sc.render.engine

    def shot(path, center, scale, w=1200, h=800):
        cd.ortho_scale = scale
        cam.location = (center[0], -6, center[1])
        sc.render.resolution_x, sc.render.resolution_y = w, h
        sc.render.filepath = path
        bpy.ops.render.render(write_still=True)

    shot(OUT + "/props_preview.png", (-0.3, 1.05), 3.6)
    # detaljer: kugle, visker-hoved, ladestok-top (400 x 400 hver) -> 1200 x 400
    tiles = []
    tmp = OUT + "/_tile.png"
    specs = [((-0.45, 0.0585), 0.16), ((0.55, 1.78), 0.42), ((0.05, 1.82), 0.42)]
    for c, sca in specs:
        shot(tmp, c, sca, 400, 400)
        im = bpy.data.images.load(tmp)
        px = np.empty(400 * 400 * 4, dtype=np.float32)
        im.pixels.foreach_get(px)
        tiles.append(px.reshape(400, 400, 4))
        bpy.data.images.remove(im)
    os.remove(tmp)
    det = np.concatenate(tiles, axis=1)
    di = bpy.data.images.new("detail", 1200, 400, alpha=False)
    di.colorspace_settings.name = "Non-Color"
    di.pixels.foreach_set(det.ravel())
    di.filepath_raw = OUT + "/props_detail.png"
    di.file_format = "PNG"
    di.save()


def write_readme(bb):
    S = STATS
    b = bb
    def line(n):
        lo, hi, unit = b[n]
        d = hi - lo
        return (f"{d[0]:.1f} x {d[1]:.1f} x {d[2]:.1f} cm (X x Y x Z); min "
                f"({lo[0]:.1f}, {lo[1]:.1f}, {lo[2]:.1f}), max ({hi[0]:.1f}, {hi[1]:.1f}, {hi[2]:.1f})")
    txt = f"""# Artilleri-props (kanonkugle, ladestok, visker)

Genereret af `Tools/Animation/build_artillery_props.py` (Blender 5.2, headless). Ikke importeret i Unreal.
Eksisterende filer overskrives ved genkoersel; tag backup foer en ny version.

| Prop | Trekanter | Tekstur (BaseColor + Roughness) | Pivot |
|---|---|---|---|
| SM_Cannonball_12pdr | {S['SM_Cannonball_12pdr_tris']} | {S['ball_tex'][0]}x{S['ball_tex'][1]} | kuglens centrum |
| SM_Rammer | {S['SM_Rammer_tris']} | {S['rammer_tex'][0]}x{S['rammer_tex'][1]} | graebspunkt 66,7 cm fra den nedre (slidsede) ende |
| SM_Sponge | {S['SM_Sponge_tris']} | {S['sponge_tex'][0]}x{S['sponge_tex'][1]} | graebspunkt 66,7 cm fra den nedre (messingkugle-) ende |

## Maal (laest fra den eksporterede FBX, centimeter som Unreal ser dem)
- SM_Cannonball_12pdr: {line('SM_Cannonball_12pdr')}
- SM_Rammer: {line('SM_Rammer')}
- SM_Sponge: {line('SM_Sponge')}
- FBX UnitScaleFactor: {b['SM_Rammer'][2]} (1 = cm).

## Enheder, akser, eksport
- I Blender er 1 enhed = 1 m. FBX eksporteres med `apply_unit_scale`, saa vaerdierne i filen er centimeter.
- FBX-akser: forward `-Y`, up `Z` (ingen ekstra rotation paa objektet). Ladestok og visker har laengdeaksen langs +Z i Blender
  og dermed ogsaa +Z i Unreal (staaende lodret). Hoved-enden (ladestokkens kappe / viskerens uldhoved) peger mod +Z.
- Alle transforms er applied, trekantet i mesh'en, kun mesh i FBX (ingen lys/kameraer/armatur).
- Glat shading med haard kant ved vinkler over 40 grader (ferrul-trin, endekapper, slids). FBX-smoothing: EDGE.

## Teksturer
- Kugle: sømløs 1024x1024. Midterste 720x720 px af fotoet (0,71 R, lav forvraengning) goeres tileable (offset-blanding),
  lys-gradient/spejling fjernes (hoejpas), og flisen lægges 4 x 2 gange paa UV-kuglen (u = 36,8 cm, v = 18,4 cm)
  saa kornstoerrelsen er ens i begge retninger. Roughness 0,7-0,95 (lavere paa blanke pletter). Polerne kan vise let
  sammentrækning af kornet. Metallic 0 (ingen glans ud over fotoet).
- Ladestok/visker: cylindrisk projektion af fotoets forside; forsiden spejles om omkredsen (u = 0,5 er fotoets midte),
  saa der er ingen sømme. Fotoets belysning er delvist bagt ind (mild kantkorrektion). Visker: en atlas, et materiale.
  Roughness efter materiale: uld ~0,96, messing ~0,4, trae ~0,72, mere variation fra fotoets lys.

## Estimater / antagelser (ikke verificerede kilder)
- Kugle 11,7 cm diameter: ESTIMAT for dansk 12-pds glatloebet kugle.
- Ladestok 200 cm lang og visker 200 cm lang: ESTIMAT. Fotoet ligner en tynd jernstang; fotoets proportioner er bevaret
  og skaleret til et kanonredskab (skaft ca. {S['rammer_shaft_cm']:.1f} cm).
- Visker: skaft {S['sponge_shaft_cm']:.1f} cm og hoved {S['sponge_head_cm']:.1f} cm er sat efter opgaven (12-pds kaliber ca. 12 cm + uld);
  fotoets skaft er forholdsmæssigt tykkere, saa hoved og skaft er skaleret hver for sig. Ulden er en glat cylinder (ikke tuft-geometri).
- Ladestokkens slids (ca. 0,9 cm bred, 8,5 cm dyb) er modelleret som snit gennem den nedre ende.
- Preview: `props_preview.png` (alle tre + 1 m maalestok), `props_detail.png` (kugle, viskerhoved, ladestokshoved).
"""
    with open(OUT + "/README.md", "w", encoding="utf-8") as f:
        f.write(txt)


def main():
    os.makedirs(OUT, exist_ok=True)
    # backup af tidligere version
    existing = [f for f in os.listdir(OUT) if f.lower().endswith((".fbx", ".blend", ".png", ".md"))] if os.path.isdir(OUT) else []
    if existing:
        import time
        bdir = f"{OUT}/_backup_{time.strftime('%Y%m%d_%H%M%S')}"
        os.makedirs(bdir)
        for f in existing:
            shutil.copy2(f"{OUT}/{f}", f"{bdir}/{f}")
    export_prop(build_cannonball(), "SM_Cannonball_12pdr")
    export_prop(build_rammer(), "SM_Rammer")
    export_prop(build_sponge(), "SM_Sponge")
    bb = {n: fbx_bbox(f"{OUT}/{n}.fbx") for n in ("SM_Cannonball_12pdr", "SM_Rammer", "SM_Sponge")}
    render_preview()
    write_readme(bb)
    for k, v in STATS.items():
        print("STAT", k, v)
    for n, (lo, hi, u) in bb.items():
        print("BBOX", n, np.round(lo, 2), np.round(hi, 2), "unit", u)


main()
