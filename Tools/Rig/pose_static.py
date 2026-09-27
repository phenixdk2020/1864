"""
Bake the rigged infantryman into a static "order arms" pose (arms down, musket upright in the
right hand, butt on the ground) and write a static glb for crowd instancing.

The figure is turned to face glTF +X, which Unreal's importer maps to the actor's forward (+X),
so AInfantryCompany can use it without a rotation offset.
"""
import io
import json
import struct
import sys

import numpy as np
from PIL import Image

RIGGED, DEBUG, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
ARM_HANG_DEG = float(sys.argv[4]) if len(sys.argv) > 4 else 7.0

d = np.load(DEBUG)
world, faces, uv, joints, weights = d["world"], d["faces"], d["uv"], d["joints"], d["weights"]
G, parents, names = d["glob"], d["parents"], [str(n) for n in d["names"]]

data = open(RIGGED, "rb").read()
jl = struct.unpack_from("<I", data, 12)[0]
gj = json.loads(data[20:20 + jl])
gbin = data[20 + jl + 8:]
ibv = gj["bufferViews"][gj["images"][0]["bufferView"]]
png = gbin[ibv["byteOffset"]:ibv["byteOffset"] + ibv["byteLength"]]


def rot(axis, deg):
    a = np.radians(deg)
    c, s = np.cos(a), np.sin(a)
    m = np.eye(4)
    i, j = {"x": (1, 2), "y": (2, 0), "z": (0, 1)}[axis]
    m[i, i], m[i, j], m[j, i], m[j, j] = c, s, -s, c
    return m


local = [G[k] @ np.linalg.inv(G[parents[k]]) if parents[k] >= 0 else G[k] for k in range(len(G))]


def rot_about_world_z(deg, pivot):
    """Row-vector matrix: rotate about the world forward axis (glTF +Z) through pivot."""
    a = np.radians(deg)
    r = np.eye(4)
    r[0, 0], r[0, 1], r[1, 0], r[1, 1] = np.cos(a), np.sin(a), -np.sin(a), np.cos(a)
    t0, t1 = np.eye(4), np.eye(4)
    t0[3, :3], t1[3, :3] = -pivot, pivot
    return t0 @ r @ t1


# Lower each upper arm in the body plane until the shoulder->hand line hangs ARM_HANG_DEG off vertical.
overrides = {}
for side, sign in (("Left", 1.0), ("Right", -1.0)):
    arm, hand_j = names.index(f"mixamorig:{side}Arm"), names.index(f"mixamorig:{side}Hand")
    shoulder, hand0 = G[arm][3, :3], G[hand_j][3, :3]
    v = hand0 - shoulder
    current = np.degrees(np.arctan2(abs(v[0]), -v[1]))  # angle from straight down, in the x-y plane
    delta = sign * -(current - ARM_HANG_DEG)
    print(f"{side} arm hangs {current:.1f} deg off vertical -> rotate {delta:.1f}")
    target = G[arm] @ rot_about_world_z(delta, shoulder)
    overrides[arm] = target @ np.linalg.inv(G[parents[arm]])

Gp = [None] * len(G)
for k in range(len(G)):
    L = overrides.get(k, local[k])
    Gp[k] = L @ Gp[parents[k]] if parents[k] >= 0 else L
skin = np.stack([np.linalg.inv(G[k]) @ Gp[k] for k in range(len(G))])

v4 = np.c_[world, np.ones(len(world))]
posed = np.zeros_like(world)
for slot in range(4):
    posed += weights[:, slot, None] * np.einsum("vi,vij->vj", v4, skin[joints[:, slot]])[:, :3]


def normals_of(verts, tris):
    n = np.zeros_like(verts)
    fn = np.cross(verts[tris[:, 1]] - verts[tris[:, 0]], verts[tris[:, 2]] - verts[tris[:, 0]])
    for c in range(3):
        np.add.at(n, tris[:, c], fn)
    return n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)


# ------------------------------------------------------------------ musket (metres, Y up)
hand = Gp[names.index("mixamorig:RightHand")][3, :3]
print(f"right hand at {hand.round(3)} (x = side, y = up, z = front)")
# Upright beside the right hand, a little outside the leg; butt on the ground.
base = np.array([hand[0] - np.sign(hand[0]) * 0.035, 0.0, hand[2] + 0.03])


def box(y0, y1, w0, d0, w1=None, d1=None, cx=0.0, cz=0.0):
    """Tapered box along Y. Returns (verts, tris) with flat faces (duplicated verts)."""
    w1 = w0 if w1 is None else w1
    d1 = d0 if d1 is None else d1
    ring = lambda y, w, dd: [(cx - w / 2, y, cz - dd / 2), (cx + w / 2, y, cz - dd / 2), (cx + w / 2, y, cz + dd / 2), (cx - w / 2, y, cz + dd / 2)]
    lo, hi = ring(y0, w0, d0), ring(y1, w1, d1)
    quads = [lo[::-1], hi] + [[lo[i], lo[(i + 1) % 4], hi[(i + 1) % 4], hi[i]] for i in range(4)]
    verts, tris = [], []
    for q in quads:
        b = len(verts)
        verts += q
        tris += [[b, b + 1, b + 2], [b, b + 2, b + 3]]
    return np.array(verts, np.float64) + base, np.array(tris, np.int64)


def merge(parts):
    verts, tris = [], []
    for v, t in parts:
        tris.append(t + sum(len(x) for x in verts))
        verts.append(v)
    return np.concatenate(verts), np.concatenate(tris)


# Danish rifled musket, ~1.45 m, with a 0.5 m socket bayonet.
wood = merge([
    box(0.00, 0.30, 0.045, 0.11, 0.040, 0.075),           # butt
    box(0.30, 0.48, 0.035, 0.050),                         # wrist
    box(0.48, 1.30, 0.032, 0.045, 0.028, 0.034),           # fore-stock
])
steel = merge([
    box(0.00, 0.02, 0.047, 0.112),                         # butt plate
    box(0.50, 0.58, 0.012, 0.070, cz=0.0),                 # lock plate
    box(0.60, 1.45, 0.020, 0.020, 0.018, 0.018, cz=0.012), # barrel (front of the stock)
    box(0.90, 0.92, 0.036, 0.040), box(1.18, 1.20, 0.032, 0.036),  # bands
    box(1.40, 1.95, 0.016, 0.006, 0.003, 0.002, cz=0.030), # bayonet blade, offset like a socket bayonet
])

# ------------------------------------------------------------------ face +X and write
turn = np.array([[0, 0, -1], [0, 1, 0], [1, 0, 0]], np.float64)  # row-vector R_y(+90): +Z -> +X


def out_mesh(verts, tris):
    v = verts @ turn
    return v.astype(np.float32), normals_of(v, tris).astype(np.float32), tris.astype(np.uint32)


body = out_mesh(posed, faces)
wood_m = out_mesh(*wood)
steel_m = out_mesh(*steel)

blobs, views, accessors = [], [], []


def blob(raw, target=None):
    offset = sum(len(b) for b in blobs)
    blobs.append(raw + b"\0" * ((-len(raw)) % 4))
    v = {"buffer": 0, "byteOffset": offset, "byteLength": len(raw)}
    if target:
        v["target"] = target
    views.append(v)
    return len(views) - 1


def acc(arr, comp, typ, target, minmax=False):
    a = {"bufferView": blob(arr.tobytes(), target), "componentType": comp, "count": int(len(arr)), "type": typ}
    if minmax:
        a["min"], a["max"] = arr.min(0).tolist(), arr.max(0).tolist()
    accessors.append(a)
    return len(accessors) - 1


def primitive(mesh, material, uvs=None):
    v, n, t = mesh
    attrs = {"POSITION": acc(v, 5126, "VEC3", 34962, True), "NORMAL": acc(n, 5126, "VEC3", 34962)}
    if uvs is not None:
        attrs["TEXCOORD_0"] = acc(uvs.astype(np.float32), 5126, "VEC2", 34962)
    return {"attributes": attrs, "indices": acc(t.reshape(-1), 5125, "SCALAR", 34963), "material": material}


prims = [primitive(body, 0, uv), primitive(wood_m, 1), primitive(steel_m, 2)]
img_view = blob(png)
gltf = {
    "asset": {"version": "2.0", "generator": "PROJECT 1864 pose_static (order arms)"},
    "scene": 0, "scenes": [{"nodes": [0]}],
    "nodes": [{"name": "SM_Infantry_Linje_OrderArms", "mesh": 0}],
    "meshes": [{"name": "SM_Infantry_Linje_OrderArms", "primitives": prims}],
    "materials": [
        {"name": "M_Infantry_Linje_Body", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 0.85}},
        {"name": "M_Musket_Wood", "pbrMetallicRoughness": {"baseColorFactor": [0.20, 0.09, 0.035, 1.0], "metallicFactor": 0.0, "roughnessFactor": 0.6}},
        {"name": "M_Musket_Steel", "pbrMetallicRoughness": {"baseColorFactor": [0.55, 0.57, 0.6, 1.0], "metallicFactor": 0.9, "roughnessFactor": 0.35}},
    ],
    "textures": [{"source": 0, "sampler": 0}],
    "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 33071, "wrapT": 33071}],
    "images": [{"bufferView": img_view, "mimeType": "image/png"}],
    "accessors": accessors, "bufferViews": views,
    "buffers": [{"byteLength": sum(len(b) for b in blobs)}],
}
js = json.dumps(gltf, separators=(",", ":")).encode()
js += b" " * ((-len(js)) % 4)
binary = b"".join(blobs)
with open(OUT, "wb") as f:
    f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(binary)))
    f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
    f.write(struct.pack("<II", len(binary), 0x004E4942) + binary)

np.savez(OUT + ".preview.npz", body=body[0], body_tris=body[2].reshape(-1, 3), uv=uv,
         wood=wood_m[0], wood_tris=wood_m[2].reshape(-1, 3), steel=steel_m[0], steel_tris=steel_m[2].reshape(-1, 3))
tex = np.asarray(Image.open(io.BytesIO(png)).convert("RGB"))
Image.fromarray(tex).save(OUT + ".tex.png")
allv = np.concatenate([body[0], wood_m[0], steel_m[0]])
print(f"wrote {OUT}: {sum(len(p[2]) // 3 for p in (body, wood_m, steel_m))} tris, bounds {allv.min(0).round(2)} .. {allv.max(0).round(2)}")
