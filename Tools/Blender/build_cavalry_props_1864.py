"""Kavaleri-rekvisitter 1864: sabel og sadel til den riggede hest (SK_Horse_1864).

Koeres headless (Blender 5.2), aldrig i GUI, ingen import i Unreal:
  "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe" -b --python Tools/Blender/build_cavalry_props_1864.py -- [all|saber|saddle]

Kilder (aendres ikke): R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView/Items/
  Saber_1_textured.glb, Saddle_1_textured.glb (Saber.fbx / Sadel.fbx er de samme 40k-net; glb'en har teksturen),
  Horse_1_rigged_v3.blend (bones fra Horse_1_Armature_v2.001 + Horse_1_Mesh.001, samme par som SK_Horse_1864).

Output i SourceAssets/Units1864/Cavalry/Props/ (eksisterende filer gemmes foerst som *.prev.*):
  SM_Saber_1864.fbx + SM_Saber_1864_Texture_0.png          (<= 3.000 trekanter, tekstur 1024)
  SM_Saddle_1864.fbx + SM_Saddle_1864_Texture_0.png        (<= 8.000 trekanter, tekstur 1024)
  Preview/*.png (kontrolark), notes.md, report.json

Enheder: 1 Blender-enhed = 1 m. FBX gemmes som i hesteeksporten (apply_unit_scale, cm i Unreal).
Env-overstyring (sadelplacering/maal): SADDLE_S, SADDLE_CY, SADDLE_CZ, SADDLE_BY, SABER_TOTAL_CM, SABER_TRIS, SADDLE_TRIS.
"""
import json
import math
import os
import shutil
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

REPO = Path(__file__).resolve().parents[2]
ITEMS = "R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView/Items/"
SABER_GLB = ITEMS + "Saber_1_textured.glb"
SADDLE_GLB = ITEMS + "Saddle_1_textured.glb"
HORSE_BLEND = ITEMS + "Horse_1_rigged_v3.blend"
HORSE_FBX = REPO / "SourceAssets/Horse/Horse_1_rigged_v3.fbx"
SOLDIER_FBX = REPO / "SourceAssets/Units1864/DK_Infantry_1864.fbx"
SOLDIER_TEX = REPO / "SourceAssets/Units1864/DK_Infantry_1864_Texture_0.png"
OUT = REPO / "SourceAssets/Units1864/Cavalry/Props"
PREV = OUT / "Preview"
HORSE_RIG, HORSE_MESH = "Horse_1_Armature_v2.001", "Horse_1_Mesh.001"
HORSE_UE_LENGTH_CM = 219.0      # SK_Horse_1864 skaleres i koden til 219 cm lang (StrategyCavalryVisualComponent)


def env(name, default):
    return type(default)(os.environ.get(name, default))


# ---- maal og skoen (alle markeret SKOEN i notes) --------------------------------------------------------------------------
SABER_TOTAL_CM = env("SABER_TOTAL_CM", 95.0)    # opgave: ca. 95 cm; SKOEN for dansk/svensk let kavalerisabel 1850'erne
SABER_TRIS = env("SABER_TRIS", 2900)            # <= 3.000
SABER_TEX = 1024
SADDLE_TRIS = env("SADDLE_TRIS", 7800)          # <= 8.000
SADDLE_TEX = 1024
# sadelplacering (kilde-koordinater -> knoglerum): v_b = S * (src - (0, CY, CZ)) + (0, BY, 0)
SADDLE_S = env("SADDLE_S", 0.50)                # Blender-skala paa sadlen i forhold til kilden
SADDLE_CY = env("SADDLE_CY", 0.36)              # kildens sadelsaede (laveste punkt) i y -> knoglens origo
SADDLE_CZ = env("SADDLE_CZ", -0.05)             # kildens sadelunderside/skabrak-top i z -> knoglens origo + SADDLE_BZ
SADDLE_BY = env("SADDLE_BY", 0.03)              # forskydning af saedet fremad/bagud i knoglens Y (m)
STIRRUP_PUSH = env("STIRRUP_PUSH", 0.20)        # stigbojlernes udadgaaende forskydning i kildeenheder (x 0.5 = m)
SADDLE_BZ = env("SADDLE_BZ", 0.012)             # sadelunderside over knoglens origo (m); hestens ryg ligger ca. 1-2 cm over knoglen

REPORT = {}


# ---- generelt --------------------------------------------------------------------------------------------------------------
def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def backup(path):
    path = Path(path)
    if path.exists():
        shutil.copy2(path, path.with_name(path.stem + ".prev" + path.suffix))


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def world_bbox(obj):
    pts = np.array([(obj.matrix_world @ Vector(c))[:] for c in obj.bound_box])
    return pts.min(0), pts.max(0)


def mesh_np(obj):
    n = len(obj.data.vertices)
    a = np.empty(n * 3, dtype=np.float64)
    obj.data.vertices.foreach_get("co", a)
    return a.reshape(n, 3)


def set_mesh_np(obj, arr):
    obj.data.vertices.foreach_set("co", np.asarray(arr, dtype=np.float64).ravel())
    obj.data.update()


def load_glb_mesh(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == "MESH"]
    for o in new:
        if o.type != "MESH":
            bpy.data.objects.remove(o)
    obj = meshes[0]
    obj.parent = None
    obj.matrix_world = Matrix.Identity(4)
    src_img = None
    for m in obj.data.materials:
        if m and m.node_tree:
            for n in m.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image:
                    src_img = n.image
    return obj, src_img


def weld(obj, dist=0.0005):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=dist)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-7, edges=bm.edges)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def delete_faces(obj, drop_fn):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    dele = [f for f in bm.faces if drop_fn(f.calc_center_median())]
    bmesh.ops.delete(bm, geom=dele, context="FACES")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def decimate_to(obj, target):
    """Collapse-decimering (UV bevares af Blender) til hoejst `target` trekanter."""
    ratio = target / max(1, tri_count(obj))
    for _ in range(8):
        mod = obj.modifiers.new("Dec", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = min(1.0, ratio)
        mod.use_collapse_triangulate = True
        dg = bpy.context.evaluated_depsgraph_get()
        new = bpy.data.meshes.new_from_object(obj.evaluated_get(dg))
        obj.modifiers.remove(mod)
        old = obj.data
        obj.data = new
        bpy.data.meshes.remove(old)
        n = tri_count(obj)
        if n <= target:
            break
        ratio *= target / n * 0.98
    return tri_count(obj)


def shade(obj, angle_deg=60.0):
    for p in obj.data.polygons:
        p.use_smooth = True


def texture_to_png(src_img, out_png, size):
    """Skalerer kildetekstur til size x size og gemmer som RGB-PNG."""
    img = src_img.copy()
    img.scale(size, size)
    sc = bpy.context.scene
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGB"
    sc.render.image_settings.color_depth = "8"
    backup(out_png)
    img.save_render(str(out_png), scene=sc)
    bpy.data.images.remove(img)


def assign_material(obj, png_path, mat_name):
    mat = bpy.data.materials.new(mat_name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(str(png_path), check_existing=False)
    tex.image.colorspace_settings.name = "sRGB"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.75
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat


def export_fbx(obj, path, forward, up, bake_space):
    backup(path)
    for o in bpy.context.scene.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"MESH"}, global_scale=1.0, apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_NONE", use_space_transform=True, bake_space_transform=bake_space,
        axis_forward=forward, axis_up=up, mesh_smooth_type="FACE", use_mesh_modifiers=True, add_leaf_bones=False,
        use_custom_props=False, path_mode="RELATIVE", embed_textures=False)


def roundtrip(fbx, forward, up):
    """Importerer FBX'en igen (samme akser) og maaler -> kontrol af eksport/akser."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(fbx), axis_forward=forward, axis_up=up)
    new = [o for o in bpy.data.objects if o not in before and o.type == "MESH"][0]
    lo, hi = world_bbox(new)
    res = {"bbox_min_m": [round(float(x), 4) for x in lo], "bbox_max_m": [round(float(x), 4) for x in hi],
           "tris": tri_count(new), "uv_layers": len(new.data.uv_layers), "materials": [m.name for m in new.data.materials if m]}
    pts = mesh_np(new)
    bpy.data.objects.remove(new)
    return res, pts


# ---- kontrolark -------------------------------------------------------------------------------------------------------------
def setup_render(res):
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.light = "STUDIO"
    sc.display.shading.color_type = "TEXTURE"
    sc.display.shading.show_cavity = False
    sc.render.film_transparent = False
    w = bpy.data.worlds.new("PreviewWorld")
    w.color = (0.80, 0.80, 0.80)
    sc.world = w
    sc.display.shading.background_type = "WORLD"
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGB"
    sc.view_settings.view_transform = "Standard"


def render_view(path, loc, target, ortho=None, fov=None, clip=None, up_hint=(0, 0, 1)):
    sc = bpy.context.scene
    cam = bpy.data.objects.get("PrevCam")
    if cam is None:
        cam = bpy.data.objects.new("PrevCam", bpy.data.cameras.new("PrevCam"))
        sc.collection.objects.link(cam)
    sc.camera = cam
    cam.location = loc
    d = Vector(target) - Vector(loc)
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    if abs(d.normalized().dot(Vector((0, 0, 1)))) > 0.98:       # lodret blik: Y op i billedet
        cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    if ortho:
        cam.data.type = "ORTHO"
        cam.data.ortho_scale = ortho
    else:
        cam.data.type = "PERSP"
        cam.data.angle = math.radians(fov or 35)
    cam.data.clip_start = clip if clip else 0.01
    cam.data.clip_end = 50.0
    sc.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def read_png(path):
    img = bpy.data.images.load(str(path), check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)


def write_png(arr, path):
    h, w = arr.shape[:2]
    img = bpy.data.images.new("sheet", w, h, alpha=False)
    img.colorspace_settings.name = "sRGB"
    img.pixels.foreach_set(np.asarray(arr, dtype=np.float32).ravel())
    sc = bpy.context.scene
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGB"
    backup(path)
    img.save_render(str(path), scene=sc)
    bpy.data.images.remove(img)


def stitch(files, out, gap=6):
    tiles = [read_png(f) for f in files]
    h = max(t.shape[0] for t in tiles)
    cols = []
    for t in tiles:
        pad = np.ones((h - t.shape[0], t.shape[1], 4), dtype=np.float32)
        cols.append(np.concatenate([t, pad], axis=0))
        cols.append(np.zeros((h, gap, 4), dtype=np.float32) + np.array([0.15, 0.15, 0.15, 1.0], dtype=np.float32))
    write_png(np.concatenate(cols[:-1], axis=1), out)


def tmp_tile(name):
    return PREV / ("_tile_" + name + ".png")


# ---- sabel ------------------------------------------------------------------------------------------------------------------
def build_saber():
    clear_scene()
    obj, src_img = load_glb_mesh(SABER_GLB)
    obj.name = "SM_Saber_1864"
    weld(obj)
    P = mesh_np(obj)
    src_len = float(P[:, 0].max() - P[:, 0].min())
    s = (SABER_TOTAL_CM / 100.0) / src_len
    # kilde: haeft/knoglebue i -X, spids i +X, kaegge i -Z (knoglebue paa kaeggesiden), flad side vinkelret paa Y.
    # ny: klinge langs +Z, kaegge mod +X: (x', y', z') = (-(z - zc), y - yc, x - xc) * s;  origo = midt i grebet.
    xc, yc, zc = -0.81, 0.004, 0.0           # greb: x -0.92..-0.70 (midte -0.81); klingens midtlinje y 0.003, z ~ 0
    Q = np.stack([-(P[:, 2] - zc), P[:, 1] - yc, P[:, 0] - xc], axis=1) * s
    set_mesh_np(obj, Q)
    # akse-skiftet er en ren rotation (det = +1): vinding og normaler er uaendrede
    n0 = tri_count(obj)
    n1 = decimate_to(obj, SABER_TRIS)
    shade(obj)
    PREV.mkdir(parents=True, exist_ok=True)
    tex = OUT / "SM_Saber_1864_Texture_0.png"
    texture_to_png(src_img, tex, SABER_TEX)
    assign_material(obj, tex, "M_Saber_1864")
    fbx = OUT / "SM_Saber_1864.fbx"
    export_fbx(obj, fbx, "-Y", "Z", True)
    Qd = mesh_np(obj)
    lo, hi = Qd.min(0) * 100, Qd.max(0) * 100
    # klingestart: ved bunden af parerstangen (kilde x ~ -0.62 -> z' = (-0.62 + 0.81) * s)
    guard_z = (-0.62 - xc) * s * 100
    pommel_z = Qd[:, 2].min() * 100
    blade_len = hi[2] - guard_z
    # bredde af klingen midt paa (z' ~ 40 cm): udstrakt i x' (kaeglinje -> ryg) og tykkelse i y'
    mid = Qd[np.abs(Qd[:, 2] * 100 - 40) < 2.0] * 100
    REPORT["saber"] = {
        "source_len_m": round(src_len, 4), "scale": round(s, 5), "tris_before": n0, "tris": n1,
        "bbox_cm": {"min": [round(float(x), 2) for x in lo], "max": [round(float(x), 2) for x in hi]},
        "total_len_cm": round(float(hi[2] - lo[2]), 2), "blade_len_cm": round(float(blade_len), 2),
        "hilt_len_cm": round(float(guard_z - pommel_z), 2), "pivot_to_pommel_cm": round(float(-pommel_z), 2),
        "pivot_to_tip_cm": round(float(hi[2]), 2), "pivot_to_guard_cm": round(float(guard_z), 2),
        "blade_width_cm_at_40": round(float(mid[:, 0].max() - mid[:, 0].min()), 2),
        "blade_thick_cm_at_40": round(float(mid[:, 1].max() - mid[:, 1].min()), 2),
        "tip_offset_x_cm": round(float(Qd[Qd[:, 2].argmax(), 0] * 100), 2),
        "knuckle_bow_side": "+X" if Qd[:, 0].max() > -Qd[:, 0].min() else "-X", "texture_px": SABER_TEX,
        "fbx": str(fbx.relative_to(REPO)), "export_axes": "forward -Y, up Z, bake_space_transform (som Artillery/Props)"}
    rt, _ = roundtrip(fbx, "-Y", "Z")
    REPORT["saber"]["roundtrip"] = rt
    return obj


def import_soldier():
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(SOLDIER_FBX))
    new = [o for o in bpy.data.objects if o not in before]
    arm = [o for o in new if o.type == "ARMATURE"][0]
    mesh = [o for o in new if o.type == "MESH"][0]
    for o in new:
        if o.type in {"CAMERA", "LIGHT"}:
            bpy.data.objects.remove(o)
    lo, hi = world_bbox(mesh)
    arm.location.z -= lo[2]
    bpy.context.view_layer.update()
    lo, hi = world_bbox(mesh)
    img = bpy.data.images.load(str(SOLDIER_TEX), check_existing=False)
    for m in mesh.data.materials:
        if m and m.node_tree:
            for n in m.node_tree.nodes:
                if n.type == "TEX_IMAGE":
                    n.image = img
    return arm, mesh, float(hi[2] - lo[2])


def preview_saber(saber):
    setup_render((700, 1100))
    arm, mesh, h = import_soldier()
    lo, hi = world_bbox(mesh)
    ctr = (lo + hi) / 2
    # soldatens front: den side hvor mesh er bredest i dybden afgoeres af blikretningen; render fra -Y og drej om noedvendigt
    REPORT["soldier_height_cm"] = round(h * 100, 1)
    # sablen staar lodret ved siden af soldaten, grebet i haandhoejde (0,9 m), 0,45 m til hans hoejre side (set fra -Y: x+)
    saber.location = Vector((ctr[0] + 0.55, ctr[1], 0.92))
    PREV.mkdir(parents=True, exist_ok=True)
    tiles = []
    # (1) sabel + soldat, set fra siden (-Y), 2,2 m hoej ortho
    t = tmp_tile("saber_fig")
    render_view(t, (ctr[0] + 0.25, -6.0, 1.05), (ctr[0] + 0.25, 0, 1.05), ortho=2.35)
    tiles.append(t)
    # (2) sabel + soldat set fra +X
    t = tmp_tile("saber_fig_side")
    render_view(t, (8.0, ctr[1], 1.05), (0, ctr[1], 1.05), ortho=2.35)
    tiles.append(t)
    # (3) sabel alene: side (kaegge til hoejre) og front (flad side)
    arm.hide_render = True
    mesh.hide_render = True
    saber.location = Vector((0, 0, 0))
    t = tmp_tile("saber_edge")
    render_view(t, (0, -4.0, 0.33), (0, 0, 0.33), ortho=1.25)
    tiles.append(t)
    t = tmp_tile("saber_flat")
    render_view(t, (4.0, 0, 0.33), (0, 0, 0.33), ortho=1.25)
    tiles.append(t)
    # (4) greb i naerbillede (ca. 24 cm) fra -Y
    t = tmp_tile("saber_hilt")
    render_view(t, (0, -4.0, 0.0), (0, 0, 0.0), ortho=0.32)
    tiles.append(t)
    stitch(tiles, PREV / "Saber_check_sheet.png")
    for t in tiles:
        os.remove(t)
    bpy.data.objects.remove(arm)
    bpy.data.objects.remove(mesh)


# ---- sadel ------------------------------------------------------------------------------------------------------------------
def append_horse():
    with bpy.data.libraries.load(HORSE_BLEND, link=False) as (src, dst):
        dst.objects = [HORSE_RIG, HORSE_MESH]
    for o in dst.objects:
        bpy.context.scene.collection.objects.link(o)
    rig, mesh = bpy.data.objects[HORSE_RIG], bpy.data.objects[HORSE_MESH]
    bpy.context.view_layer.update()
    return rig, mesh


def bone_info(rig):
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    roll = rig.data.edit_bones["saddle_root"].roll
    bpy.ops.object.mode_set(mode="OBJECT")
    b = rig.data.bones["saddle_root"]
    M = rig.matrix_world @ b.matrix_local
    return {"head": rig.matrix_world @ b.head_local, "tail": rig.matrix_world @ b.tail_local, "roll": roll, "M": M,
            "parent": b.parent.name if b.parent else None, "length": b.length}


def horse_bvh(mesh):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = mesh.evaluated_get(dg)
    me = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.transform(mesh.matrix_world)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bvh = BVHTree.FromBMesh(bm)
    ev.to_mesh_clear()
    bm.free()
    return bvh


def inside_depth(bvh, p):
    """None hvis punktet er uden for hesten, ellers afstanden til overfladen (m). Paritetstest langs +Z."""
    n, o = 0, Vector(p)
    for _ in range(12):
        hit = bvh.ray_cast(o, Vector((0, 0, 1)))
        if hit[0] is None:
            break
        n += 1
        o = hit[0] + Vector((0, 0, 1e-4))
    if n % 2 == 0:
        return None
    near = bvh.find_nearest(Vector(p))
    return near[3] if near and near[0] is not None else None


def saddle_keep(c):
    x, y, z = c
    if y < -0.12:                                         # hovedtoej, tojler og forreste stropper
        return False
    if z < -0.28 and abs(x) < 0.20:                       # bugreb / midterstropper under saedet
        return False
    if z < -0.45 and not (abs(x) > 0.30 and 0.0 < y < 0.45):   # kun stigbojler beholdes langt nede
        return False
    return True


def build_saddle():
    clear_scene()
    rig, hmesh = append_horse()
    info = bone_info(rig)
    bvh = horse_bvh(hmesh)
    obj, src_img = load_glb_mesh(SADDLE_GLB)
    obj.name = "SM_Saddle_1864"
    weld(obj)
    delete_faces(obj, lambda c: not saddle_keep(c))
    n_cut = tri_count(obj)
    P = mesh_np(obj)
    # kilde -> knoglerum (kildens akser = knoglens: x = hestens venstre, -y = fremad, z = op)
    # stigbojlerne spredes udad (kildens stigbojler hænger inde i hestens tønde): kun under skabrakkens kant (z < -0,36)
    t = np.clip((-0.36 - P[:, 2]) / 0.14, 0.0, 1.0)
    P[:, 0] += np.sign(P[:, 0]) * STIRRUP_PUSH * (t * t * (3 - 2 * t))
    cvec = np.array([0.0, SADDLE_CY, SADDLE_CZ])
    Q = SADDLE_S * (P - cvec) + np.array([0.0, SADDLE_BY, SADDLE_BZ])
    set_mesh_np(obj, Q)
    # faces gemt inde i hesten (sadelunderside, skurer) fjernes: de ses aldrig og koster trekanter
    def buried(c):
        d = inside_depth(bvh, info["M"] @ Vector(c))
        return d is not None and d > 0.004
    n_pre = tri_count(obj)
    delete_faces(obj, buried)
    n_buried = n_pre - tri_count(obj)
    n_dec = decimate_to(obj, SADDLE_TRIS)
    shade(obj)
    tex = OUT / "SM_Saddle_1864_Texture_0.png"
    texture_to_png(src_img, tex, SADDLE_TEX)
    assign_material(obj, tex, "M_Saddle_1864")
    fbx = OUT / "SM_Saddle_1864.fbx"
    # Eksport med SAMME akser/indstillinger som hesten (Tools/Animation/export_horse_mesh_b1.py): -Z fwd, Y up,
    # ingen bake_space_transform -> sadlens vertices (i knoglerum) faar samme akseomregning som hestens knogler.
    export_fbx(obj, fbx, "-Z", "Y", False)
    Qd = mesh_np(obj)
    lo, hi = Qd.min(0), Qd.max(0)
    # placeret paa hesten (til kontrol): objektet faar knoglens matrix
    obj.matrix_world = info["M"]
    bpy.context.view_layer.update()
    # penetration: sadelvertices inde i hestens net
    W = np.array([(obj.matrix_world @ Vector(v))[:] for v in Qd])
    depths = []
    deep = []
    for p, q in zip(W, Qd):
        d = inside_depth(bvh, p)
        if d is not None:
            depths.append(d)
            if d > 0.05:
                deep.append(q)
    if deep:
        deep = np.array(deep) * 100
        for lo_z in range(-40, 10, 10):
            sel = deep[(deep[:, 2] >= lo_z) & (deep[:, 2] < lo_z + 10)]
            if len(sel):
                print("DEEPBAND z %d..%d n=%d |x| mean %.1f y range %.0f..%.0f depth-ish" % (lo_z, lo_z + 10, len(sel), np.abs(sel[:, 0]).mean(), sel[:, 1].min(), sel[:, 1].max()))
        print("DEEP_PEN n=%d bone-frame cm min %s max %s mean %s" % (len(deep), np.round(deep.min(0)), np.round(deep.max(0)), np.round(deep.mean(0))))
    # saedets hoejde over knoglen: traek en straale ned paa sadlen ved knoglens (x=0) og y i [-0.1,0.2]
    sb = BVHTree.FromObject(obj, bpy.context.evaluated_depsgraph_get())
    seat = {}
    for yb in (-0.10, -0.05, 0.0, 0.05, 0.10, 0.15, 0.20):
        hit = sb.ray_cast(Vector((0.0, yb, 1.0)), Vector((0, 0, -1)))     # BVH i objektets lokale (knogle-)rum
        seat[str(yb)] = None if hit[0] is None else round(float(hit[0].z) * 100, 2)
    hb = world_bbox(hmesh)
    hl = hb[1] - hb[0]
    k = HORSE_UE_LENGTH_CM / (hl[0] * 100)
    R3 = info["M"].to_3x3()
    REPORT["saddle"] = {
        "params": {"S": SADDLE_S, "CY": SADDLE_CY, "CZ": SADDLE_CZ, "BY": SADDLE_BY, "BZ": SADDLE_BZ},
        "tris_after_cut": n_cut, "tris_buried_removed": n_buried, "stirrup_push_src": STIRRUP_PUSH, "tris": n_dec, "texture_px": SADDLE_TEX,
        "bone": {"head_m": [round(x, 4) for x in info["head"]], "tail_m": [round(x, 4) for x in info["tail"]],
                 "length_m": round(info["length"], 4), "roll_rad": round(info["roll"], 5), "roll_deg": round(math.degrees(info["roll"]), 3),
                 "parent": info["parent"],
                 "axes_in_blender_world": {"x": [round(v, 3) for v in R3.col[0]], "y": [round(v, 3) for v in R3.col[1]],
                                           "z": [round(v, 3) for v in R3.col[2]]}},
        "horse_m": {"length_x": round(float(hl[0]), 4), "width_y": round(float(hl[1]), 4), "height_z": round(float(hl[2]), 4),
                    "bbox_min": [round(float(v), 4) for v in hb[0]], "bbox_max": [round(float(v), 4) for v in hb[1]]},
        "ue_scale_factor": round(k, 4), "horse_ue_length_cm": HORSE_UE_LENGTH_CM,
        "saddle_bbox_bone_frame_cm": {"min": [round(float(v) * 100, 1) for v in lo], "max": [round(float(v) * 100, 1) for v in hi]},
        "seat_surface_above_bone_cm_at_yb": seat,
        "penetration": {"verts_inside_horse": len(depths), "of": len(W),
                        "max_depth_cm": round(max(depths) * 100, 2) if depths else 0.0,
                        "p95_depth_cm": round(float(np.percentile(depths, 95)) * 100, 2) if depths else 0.0},
        "fbx": str(fbx.relative_to(REPO)), "export_axes": "forward -Z, up Y (som hesten), bake_space_transform False"}
    # roundtrip: genimporter sadlen OG hestens FBX; sadlens punkter i knoglerummet skal blive uaendrede
    saved = (rig, hmesh, obj)
    rt, pts = roundtrip(fbx, "-Z", "Y")
    rt["max_vertex_diff_vs_bone_frame_m"] = round(float(np.abs(np.sort(pts, axis=0) - np.sort(Qd, axis=0)).max()), 6)
    REPORT["saddle"]["roundtrip"] = rt
    return rig, hmesh, obj, info


def preview_saddle(rig, hmesh, saddle, info):
    setup_render((900, 900))
    PREV.mkdir(parents=True, exist_ok=True)
    head = info["head"]
    tiles = []
    ctr = Vector((head.x + 0.05, 0.0, head.z - 0.05))
    # (1) fra siden (-Y): hesten ser mod venstre
    t = tmp_tile("saddle_side")
    render_view(t, (ctr.x, -6, ctr.z), (ctr.x, 0, ctr.z), ortho=1.35)
    tiles.append(t)
    # (2) forfra (fra -X, hestens hoved skaeres vaek med near-clip)
    t = tmp_tile("saddle_front")
    render_view(t, (-6.0, 0, ctr.z), (0, 0, ctr.z), ortho=1.15, clip=5.9 + (-0.12))
    tiles.append(t)
    # (3) oppefra
    t = tmp_tile("saddle_top")
    render_view(t, (ctr.x, 0, 6.0), (ctr.x, 0, 0), ortho=1.35)
    tiles.append(t)
    # (4) 3/4 bagfra
    t = tmp_tile("saddle_q34")
    render_view(t, (head.x + 1.5, -1.6, head.z + 0.55), (head.x, 0, head.z - 0.08), fov=28)
    tiles.append(t)
    stitch(tiles, PREV / "Saddle_on_horse_sheet.png")
    for t in tiles:
        os.remove(t)
    # sadlen alene i knoglerummet (side, top) med akseretning
    for o in (rig, hmesh):
        o.hide_render = True
    saddle.matrix_world = Matrix.Identity(4)
    t1, t2, t3 = tmp_tile("sa_side"), tmp_tile("sa_front"), tmp_tile("sa_top")
    render_view(t1, (0, -5, 0.1), (0, 0, 0.1), ortho=0.95)
    render_view(t2, (-5, 0, 0.1), (0, 0, 0.1), ortho=0.95)
    render_view(t3, (0, 0, 5), (0, 0, 0), ortho=0.95)
    stitch([t1, t2, t3], PREV / "Saddle_alone_bone_frame_sheet.png")
    for t in (t1, t2, t3):
        os.remove(t)


# ---- noter -------------------------------------------------------------------------------------------------------------------
def write_notes():
    r = REPORT
    s, d = r["saber"], r["saddle"]
    b, hm, pen = d["bone"], d["horse_m"], d["penetration"]
    sb = d["saddle_bbox_bone_frame_cm"]
    seat = d["seat_surface_above_bone_cm_at_yb"]
    seat_mid = seat.get("0.0")
    seat_txt = ", ".join("%+d cm: %s" % (round(float(k) * 100), v) for k, v in seat.items())
    cloth_len = sb["max"][1] - sb["min"][1]
    notes = f"""# Kavaleri-rekvisitter 1864 (sabel og sadel)

Bygget af `Tools/Blender/build_cavalry_props_1864.py` (Blender 5.2 headless). Ikke importeret i Unreal (overdrages til unreal-integration).
Genkør: `blender -b --python Tools/Blender/build_cavalry_props_1864.py -- all` (eller `saber` / `saddle`). Eksisterende filer gemmes som `*.prev.*`.
Enhed: 1 Blender-enhed = 1 m; FBX'erne er i cm (apply_unit_scale), som hesteeksporten.

## Kilder (ændres ikke)
`R:\\Onedrive\\Dokumenter\\Unreal Projects\\Game1864\\Reference\\Units\\MultiView\\Items\\`
- `Saber_1_textured.glb` (samme net som `Saber.fbx`; 40.000 trekanter, tekstur 2048). glb'en bruges, fordi FBX'ens tekstur ikke kan læses af Blender.
- `Saddle_1_textured.glb` (samme net som `Sadel.fbx`; 40.000 trekanter, tekstur 2048). Kilden er et AI-genereret, sammensmeltet net med sadel, skabrak, tasker, oprullet kappe, stigbøjler, bugreb og et helt hovedtøj med tøjler.
- `Horse_1_rigged_v3.blend`: `Horse_1_Armature_v2.001` + `Horse_1_Mesh.001` (samme par som `SourceAssets/Horse/Horse_1_rigged_v3.fbx`, dvs. SK_Horse_1864; blendfilen har et dublet-par uden `.001`).

## 1. SM_Saber_1864.fbx + SM_Saber_1864_Texture_0.png
- Skala fra kilden: {s['scale']} (kilden er {s['source_len_m']} m lang). Samlet længde **{s['total_len_cm']} cm** (mål {SABER_TOTAL_CM:g} cm).
- Pivot (origin) = midt i grebet. Pommel {s['pivot_to_pommel_cm']} cm under origo, parerstang ca. {s['pivot_to_guard_cm']} cm over origo, spids {s['pivot_to_tip_cm']} cm over origo.
- Akser: **klinge langs lokal +Z (spids op), æggen mod +X**, flad side mod ±Y. Knoglebuen (håndbeskyttelsen) sidder på æggesiden ({s['knuckle_bow_side']}). Klingen er let krum; spidsen ligger {abs(s['tip_offset_x_cm'])} cm mod ryggen (-X).
- bbox (cm, i FBX): min {s['bbox_cm']['min']}, max {s['bbox_cm']['max']}.
- **Afvigelse:** klingen fra parerstang til spids er ca. **{s['blade_len_cm']} cm** (ønsket ca. 82). Kildens proportioner er greb ca. {s['hilt_len_cm']} cm inkl. pommel og klinge ca. 81 % af længden; ved 95 cm i alt giver det {s['blade_len_cm']} cm klinge. Jeg har ikke strakt klingen eller presset grebet sammen. Skal klingen være 82 cm, skal totallængden op på ca. 101 cm (`SABER_TOTAL_CM=101`), eller grebet kortes ned.
- Klingebredde ca. {s['blade_width_cm_at_40']} cm, tykkelse ca. {s['blade_thick_cm_at_40']} cm (kildenettet er tykt og uregelmæssigt; tykkelsen er større end en rigtig klinges).
- Trekanter: **{s['tris']}** (kilde 40.000, {s['tris_before']} efter svejsning); collapse-decimering, UV bevaret. Tekstur {s['texture_px']} x {s['texture_px']} (nedskaleret fra 2048). Materiale `M_Saber_1864` (kun BaseColor).
- FBX: forward -Y / up Z, bake_space_transform, apply_unit_scale (samme indstillinger som `SourceAssets/Artillery/Props`). Blender +Z = Unreal +Z.
- **Skøn (ikke verificeret kilde):** længde ca. 95 cm og klinge ca. 82 cm er opgavens skøn for en dansk/svensk let kavalerisabel i 1850'erne. Modellen er en generisk krum sabel med D-formet knoglebue, ikke en bestemt regulativmodel.
- Kontrolark: `Preview/Saber_check_sheet.png`, fra venstre: sabel ved soldaten (set fra siden), set fra +X, sabel alene fra æggesiden, fra fladen, greb i nærbillede. Soldaten er projektets `DK_Infantry_1864` ({r.get('soldier_height_cm', '?')} cm i spillet, ikke 180 cm); sablen står med grebet i ca. 92 cm højde ved siden af ham.

## 2. SM_Saddle_1864.fbx + SM_Saddle_1864_Texture_0.png
**Placering.** Sadlens vertices er gemt i `saddle_root`-knoglens eget koordinatsystem: origo = knoglens hoved, akserne er knoglens. Hænges sadlen på socket `saddle_root` uden relativ transform, sidder den på hesten i hvilepose.
- Knogle `saddle_root` (forælder {b['parent']}): hoved {b['head_m']} m, hale {b['tail_m']} m (Blender-verden; armaturet ligger i origo uden rotation), længde {b['length_m']} m, **roll {b['roll_rad']} rad ({b['roll_deg']} grader)**.
- Knoglens akser i Blender-verden: X = {b['axes_in_blender_world']['x']} (hestens venstre), Y = {b['axes_in_blender_world']['y']} (mod halen), Z = {b['axes_in_blender_world']['z']} (op). Hesten vender mod -X i Blender, så knoglens +Y peger bagud.
- Hesten i hvilepose (Blender-enheder = m): længde (X) {hm['length_x']}, bredde (Y) {hm['width_y']}, højde (Z, til ørespidserne) {hm['height_z']}; bbox {hm['bbox_min']} til {hm['bbox_max']}. Rygfladen bag manken ligger i z ca. 1,19-1,25 m, altså 1-2 cm over knoglens hoved (z 1,182 m).
- **Blender-skala på sadlen i forhold til kilden: {d['params']['S']}** (uniform). Kildens akser er knoglens akser (+X hestens venstre, -Y fremad, +Z op), så der er kun skala og forskydning, ingen rotation. Kildepunktet (x 0, y {d['params']['CY']}, z {d['params']['CZ']}) = sædets laveste punkt/skabrakkens top ligger i knoglerummet i (0, {d['params']['BY']}, {d['params']['BZ']}) m.
- **Skala i Unreal.** Sadlen er bygget i hestens egen skala (hesten er {hm['length_x'] * 100:.2f} cm lang i FBX'en). Er hesten i Unreal skaleret til {d['horse_ue_length_cm']:g} cm, skal sadlen have **samme faktor: {d['ue_scale_factor']}** ({d['horse_ue_length_cm']:g} / {hm['length_x'] * 100:.2f}) på alle tre akser. Hænges den som barn af hestens skeletal mesh component på socket `saddle_root`, arver den skalaen (relativ skala 1,0). Sættes den uafhængigt, bruges {d['ue_scale_factor']}.
- Sadlens bbox i knoglerummet (cm, i FBX, før Unreal-skalering): min {sb['min']}, max {sb['max']} (X = hestens venstre, Y = mod halen, Z = op). Skabrak + kappe er ca. {cloth_len:.0f} cm lang.
- **Sædets højde:** sædets overflade (midterlinjen, ved knoglens y = -10, -5, 0, 5, 10, 15, 20 cm; cm over knoglens hoved, uden Unreal-skala): {seat_txt}. Midt i sædet ca. {seat_mid} cm over knoglen. Rytteren sidder i koden `SaddleLiftCm` = 6 cm over knoglen (bar hest), så rytterens bækkenknogle skal løftes til ca. sæde + 8-10 cm (ca. {seat_mid + 9 if seat_mid else '?'} cm over knoglen), når sadlen sættes på; ikke ændret her. Stigbøjlernes trin ligger ca. {abs(sb['min'][2]):.0f} cm under knoglen.
- Penetration i hestens net (hvilepose): {pen['verts_inside_horse']} af {pen['of']} vertices, max {pen['max_depth_cm']} cm, 95-percentil {pen['p95_depth_cm']} cm.
- Trekanter: **{d['tris']}** (kilde 40.000; efter udskæring {d['tris_after_cut']}, heraf {d['tris_buried_removed']} fjernet fordi de lå gemt inde i hesten); collapse-decimering, UV bevaret. Tekstur {d['texture_px']} x {d['texture_px']} (nedskaleret fra 2048). UV-øerne for de bortskårne dele er ubrugte. Materiale `M_Saddle_1864`.
- FBX: forward -Z / up Y, uden bake_space_transform, apply_unit_scale: **samme indstillinger som hesteeksporten** (`Tools/Animation/export_horse_mesh_b1.py`), så sadlen får samme akseomregning som hestens knogler (sockettransformen = knoglens). Kun kontrolleret i Blender: genimport giver samme punkter i knoglerummet (maks. forskel {d['roundtrip']['max_vertex_diff_vs_bone_frame_m']} m). **Ikke kontrolleret i Unreal.** Hestens FBX vendes i spillet med HorseYaw; sadlen følger med, når den hænges på socket.

### Hvad er ændret i forhold til kilden
- Bortskåret: hovedtøj med nakkereb og tøjler, forreste stropper/brystreb (kilde-y < -0,12), midterstropper og bugreb under sædet (z < -0,28 ved |x| < 0,2), lavt hængende rester undtagen stigbøjler med læder. Beholdt: sadeltræ (pommel, sæde, cantle), skabrak (blå med rød kant), to sadeltasker, oprullet kappe bag cantle, læderstropper og stigbøjler. Hesten har sit eget hoved; ingen tøjler i dette asset.
- Stigbøjler og stigbøjlerem under skabrakkens kant er spredt {d['stirrup_push_src']} kildeenheder ud til siderne (kilden hænger dem inde i hestens tønde).
- Faces, der ligger inde i hestens net (sadelunderside), er fjernet.
- Kilden er uregelmæssig: kanter og tasker er grove, nogle strimler er rå. Ved kampafstand uden betydning.

## Skøn / forbehold
- Sabelmålene (95 / 82 cm) og sadlens skala ({d['params']['S']}) er skøn; skalaen er valgt, så skabrak og kappe dækker hestens flade ryg bag manken. Historisk korrekt kavalerisadel fra 1850'erne (model, remtøj) er ikke efterprøvet.
- Sadlen er højere end en rigtig sadel i forhold til hesten (sædet ca. {seat_mid} cm over knoglen); kildens form er bevaret frem for at flade den ud.
- Kontrolark: `Preview/Saddle_on_horse_sheet.png` (side, forfra med hestens hoved skåret væk, oppefra, 3/4 skråt) og `Preview/Saddle_alone_bone_frame_sheet.png` (sadlen alene i knoglerummet; origo i billedets midte; side/forfra/oppefra).
- Ingen animation og ingen deformation: sadlen følger kun knoglen (stift net).
"""
    (OUT / "notes.md").write_text(notes, encoding="utf-8")
    (OUT / "report.json").write_text(json.dumps(REPORT, indent=2, default=str), encoding="utf-8")


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    what = args[0] if args else "all"
    OUT.mkdir(parents=True, exist_ok=True)
    PREV.mkdir(parents=True, exist_ok=True)
    if what in ("all", "saber"):
        saber = build_saber()
        preview_saber(saber)
    if what in ("all", "saddle"):
        rig, hmesh, saddle, info = build_saddle()
        preview_saddle(rig, hmesh, saddle, info)
    if what == "all":
        write_notes()
    print("REPORT=" + json.dumps(REPORT, default=str))


main()
