"""
Transfer a Mixamo rig (skeleton + skin weights) from the untextured high-poly Hunyuan mesh
onto the textured low-poly mesh, and write a skinned, textured glTF (.glb).

Conventions: FBX matrices are read as row-vector matrices (p' = p @ M, translation in the last row).
Output is glTF 2.0: metres, Y-up, character facing +Z (Mixamo's orientation).
"""
import json
import struct
import sys

import numpy as np
from scipy.spatial import cKDTree

from fbxread import read

RIG_FBX, TEX_GLB, OUT_GLB = sys.argv[1], sys.argv[2], sys.argv[3]
CM_TO_M = 0.01
# Textured glb local (x, y-up, z) -> Hunyuan FBX local (x, -z, y); measured median error 1.6 mm.
GLB_TO_FBX = np.array([[1, 0, 0], [0, 0, 1], [0, -1, 0]], np.float64)


def mat(node, name):
    return node.first(name).props[0].astype(np.float64).reshape(4, 4)


def props70(model):
    p = model.first("Properties70")
    return {c.props[0]: c.props[4:] for c in p.children} if p else {}


def translation(t):
    m = np.eye(4)
    m[3, :3] = t
    return m


# ------------------------------------------------------------------ read the rig
rig = read(RIG_FBX)
objects = rig.first("Objects")
models = {m.props[0]: m for m in objects.find("Model")}
conns = [c.props for c in rig.first("Connections").children]
# Scene hierarchy only: bones are also connected to their skin clusters, which must not count as parents.
parent_of = {c[1]: c[2] for c in conns if c[0] == "OO" and c[1] in models and (c[2] in models or c[2] == 0)}

bones = {mid: m for mid, m in models.items() if m.props[2] == "LimbNode"}
# Strip the "mixamorig:" namespace: Unreal's FBX import strips it from animation tracks, so the
# skeleton must use the bare names (Hips, Spine, RightHand ...) for the tracks to bind.
bone_name = {mid: m.props[1].split("\x00")[0].split(":")[-1] for mid, m in bones.items()}

geometry = objects.find("Geometry")[0]
shape_v = geometry.first("Vertices").props[0].reshape(-1, 3).astype(np.float64)

clusters = {}
for d in objects.find("Deformer"):
    if d.props[2] != "Cluster":
        continue
    bone = next(c[1] for c in conns if c[0] == "OO" and c[2] == d.props[0] and c[1] in bones)
    idx = d.first("Indexes").props[0] if d.first("Indexes") else np.zeros(0, np.int32)
    w = d.first("Weights").props[0] if d.first("Weights") else np.zeros(0)
    clusters[bone] = (idx, w, mat(d, "Transform"), mat(d, "TransformLink"))

# Mesh global at bind: Transform = MeshGlobal @ inv(Link)  =>  MeshGlobal = Transform @ Link.
mesh_globals = [t @ l for (_, _, t, l) in clusters.values()]
mesh_global = mesh_globals[0]
spread = max(np.abs(m - mesh_global).max() for m in mesh_globals)
print(f"mesh global consistency (max abs diff across clusters): {spread:.4g}")

# Bone order: parents first.
def depth(mid):
    d = 0
    while parent_of.get(mid) in bones:
        mid = parent_of[mid]
        d += 1
    return d

order = sorted(bones, key=lambda m: (depth(m), bone_name[m]))
index_of = {mid: i for i, mid in enumerate(order)}

# Bone global bind (cm, row convention). End bones without a cluster: parent global offset by Lcl Translation.
glob = {}
for mid in order:
    if mid in clusters:
        glob[mid] = clusters[mid][3]
    else:
        t = np.array(props70(bones[mid]).get("Lcl Translation", [0, 0, 0]), np.float64)
        glob[mid] = translation(t) @ glob[parent_of[mid]]

def to_metres(m):
    m = m.copy()
    m[3, :3] *= CM_TO_M
    return m

glob_m = {mid: to_metres(g) for mid, g in glob.items()}

# ------------------------------------------------------------------ weights on the high-poly mesh
nb = len(order)
dense = np.zeros((len(shape_v), nb), np.float32)
for mid, (idx, w, _, _) in clusters.items():
    dense[idx, index_of[mid]] += w
total = dense.sum(1, keepdims=True)
print(f"high-poly vertices without weights: {(total[:, 0] == 0).sum()} of {len(shape_v)}")
dense /= np.maximum(total, 1e-8)

# ------------------------------------------------------------------ read the textured mesh
data = open(TEX_GLB, "rb").read()
jlen = struct.unpack_from("<I", data, 12)[0]
gj = json.loads(data[20:20 + jlen])
gbin = data[20 + jlen + 8:]

def acc(i):
    a = gj["accessors"][i]
    bv = gj["bufferViews"][a["bufferView"]]
    comp = {5126: np.float32, 5125: np.uint32, 5123: np.uint16}[a["componentType"]]
    n = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[a["type"]]
    arr = np.frombuffer(gbin, comp, a["count"] * n, bv.get("byteOffset", 0) + a.get("byteOffset", 0))
    return arr.reshape(a["count"], n) if n > 1 else arr

prim = gj["meshes"][0]["primitives"][0]
tex_pos = acc(prim["attributes"]["POSITION"]).astype(np.float64)
tex_uv = acc(prim["attributes"]["TEXCOORD_0"]).astype(np.float32)
tex_nrm = acc(prim["attributes"]["NORMAL"]).astype(np.float64)
tex_idx = acc(prim["indices"]).astype(np.uint32)
img = gj["images"][0]
ibv = gj["bufferViews"][img["bufferView"]]
png = gbin[ibv.get("byteOffset", 0):ibv.get("byteOffset", 0) + ibv["byteLength"]]

# Into FBX mesh-local space, then to world bind (metres).
local = tex_pos @ GLB_TO_FBX
world = (np.c_[local, np.ones(len(local))] @ mesh_global)[:, :3] * CM_TO_M
# Put the feet on the ground: shift mesh and skeleton so the lowest vertex is at y = 0 (pivot between the feet).
ground = np.array([-(world[:, 0].min() + world[:, 0].max()) / 2, -world[:, 1].min(), -(world[:, 2].min() + world[:, 2].max()) / 2])
ground[0] = ground[2] = 0.0  # keep the Mixamo x/z origin (hips above the origin); only lift
world += ground
for mid in glob_m:
    glob_m[mid][3, :3] += ground
print(f"lifted to ground by {ground[1]:.3f} m")
rot = mesh_global[:3, :3] / np.linalg.norm(mesh_global[:3, :3], axis=1, keepdims=True)
normals = (tex_nrm @ GLB_TO_FBX) @ rot
normals /= np.linalg.norm(normals, axis=1, keepdims=True)

# ------------------------------------------------------------------ transfer weights (IDW over 6 nearest)
tree = cKDTree(shape_v)
dist, near = tree.query(local, k=6)
iw = 1.0 / np.maximum(dist, 1e-5) ** 2
iw /= iw.sum(1, keepdims=True)
w_tex = np.einsum("vk,vkb->vb", iw, dense[near])
top = np.argsort(-w_tex, axis=1)[:, :4]
top_w = np.take_along_axis(w_tex, top, axis=1)
top_w /= top_w.sum(1, keepdims=True)
print(f"transfer: median source distance {np.median(dist[:, 0]) * 100:.2f} cm (mesh units), max bones/vertex 4")

# ------------------------------------------------------------------ write glTF
blobs = []

def add_blob(arr, target=None):
    raw = arr.tobytes()
    offset = sum(len(b) for b in blobs)
    pad = (-len(raw)) % 4
    blobs.append(raw + b"\0" * pad)
    view = {"buffer": 0, "byteOffset": offset, "byteLength": len(raw)}
    if target:
        view["target"] = target
    views.append(view)
    return len(views) - 1

views, accessors = [], []

def add_acc(arr, comp, typ, target=None, minmax=False):
    view = add_blob(arr, target)
    a = {"bufferView": view, "componentType": comp, "count": int(arr.shape[0]), "type": typ}
    if minmax:
        a["min"] = np.atleast_1d(arr.min(0)).tolist()
        a["max"] = np.atleast_1d(arr.max(0)).tolist()
    accessors.append(a)
    return len(accessors) - 1

pos_a = add_acc(world.astype(np.float32), 5126, "VEC3", 34962, True)
nrm_a = add_acc(normals.astype(np.float32), 5126, "VEC3", 34962)
uv_a = add_acc(tex_uv, 5126, "VEC2", 34962)
jnt_a = add_acc(top.astype(np.uint16), 5123, "VEC4", 34962)
wgt_a = add_acc(top_w.astype(np.float32), 5126, "VEC4", 34962)
idx_a = add_acc(tex_idx, 5125, "SCALAR", 34963)
inv_bind = np.stack([np.linalg.inv(glob_m[mid]) for mid in order]).astype(np.float32)
ibm_a = add_acc(inv_bind.reshape(len(order), 16), 5126, "MAT4")
img_view = add_blob(np.frombuffer(png, np.uint8))

def quat_from_row(m):
    """Quaternion (x, y, z, w) of the rotation in a row-vector matrix (N,4,4) or (4,4)."""
    single = m.ndim == 2
    r = np.transpose(np.atleast_3d(m[None] if single else m)[:, :3, :3], (0, 2, 1))  # column-convention rotation
    r = r / np.linalg.norm(r, axis=1, keepdims=True)
    q = np.zeros((len(r), 4))
    tr = r[:, 0, 0] + r[:, 1, 1] + r[:, 2, 2]
    for i in range(len(r)):
        R = r[i]
        if tr[i] > 0:
            s = np.sqrt(tr[i] + 1.0) * 2
            q[i] = [(R[2, 1] - R[1, 2]) / s, (R[0, 2] - R[2, 0]) / s, (R[1, 0] - R[0, 1]) / s, 0.25 * s]
        elif R[0, 0] > R[1, 1] and R[0, 0] > R[2, 2]:
            s = np.sqrt(1.0 + R[0, 0] - R[1, 1] - R[2, 2]) * 2
            q[i] = [0.25 * s, (R[0, 1] + R[1, 0]) / s, (R[0, 2] + R[2, 0]) / s, (R[2, 1] - R[1, 2]) / s]
        elif R[1, 1] > R[2, 2]:
            s = np.sqrt(1.0 + R[1, 1] - R[0, 0] - R[2, 2]) * 2
            q[i] = [(R[0, 1] + R[1, 0]) / s, 0.25 * s, (R[1, 2] + R[2, 1]) / s, (R[0, 2] - R[2, 0]) / s]
        else:
            s = np.sqrt(1.0 + R[2, 2] - R[0, 0] - R[1, 1]) * 2
            q[i] = [(R[0, 2] + R[2, 0]) / s, (R[1, 2] + R[2, 1]) / s, 0.25 * s, (R[1, 0] - R[0, 1]) / s]
    q /= np.linalg.norm(q, axis=1, keepdims=True)
    for i in range(1, len(q)):  # keep neighbouring keys in the same hemisphere
        if np.dot(q[i], q[i - 1]) < 0:
            q[i] = -q[i]
    return q[0] if single else q


nodes = [{"name": "Infantry_Linje", "mesh": 0, "skin": 0}]
joint_nodes = []
for mid in order:
    parent = parent_of.get(mid)
    local_m = glob_m[mid] @ np.linalg.inv(glob_m[parent]) if parent in bones else glob_m[mid]
    nodes.append({"name": bone_name[mid], "translation": local_m[3, :3].astype(float).tolist(),
                  "rotation": quat_from_row(local_m).astype(float).tolist()})
    joint_nodes.append(len(nodes) - 1)
for mid in order:
    children = [index_of[c] + 1 for c in order if parent_of.get(c) == mid]
    if children:
        nodes[index_of[mid] + 1]["children"] = children
roots = [index_of[m] + 1 for m in order if parent_of.get(m) not in bones]

animations = []
if len(sys.argv) > 5:
    from fbx_anim import load_clip
    node_of = {bone_name[mid]: index_of[mid] + 1 for mid in order}
    for clip in sys.argv[5].split(","):
        times, locs = load_clip(sys.argv[4] + "/" + clip + ".fbx")
        t_acc = add_acc(times.astype(np.float32), 5126, "SCALAR", None, True)
        samplers, channels = [], []
        for name, mats in locs.items():
            if name not in node_of:
                continue
            for path, values, typ in (("translation", mats[:, 3, :3], "VEC3"), ("rotation", quat_from_row(mats), "VEC4")):
                samplers.append({"input": t_acc, "output": add_acc(values.astype(np.float32), 5126, typ, None), "interpolation": "LINEAR"})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": node_of[name], "path": path}})
        animations.append({"name": clip.replace(" ", ""), "samplers": samplers, "channels": channels})
        print(f"animation {clip}: {len(times)} frames, {len(channels) // 2} bones")

gltf = {
    "asset": {"version": "2.0", "generator": "PROJECT 1864 rig_transfer (Mixamo rig -> textured Hunyuan mesh)"},
    "scene": 0,
    "scenes": [{"nodes": [0] + roots}],
    "nodes": nodes,
    "meshes": [{"name": "Infantry_Linje", "primitives": [{
        "attributes": {"POSITION": pos_a, "NORMAL": nrm_a, "TEXCOORD_0": uv_a, "JOINTS_0": jnt_a, "WEIGHTS_0": wgt_a},
        "indices": idx_a, "material": 0}]}],
    "skins": [{"name": "mixamorig", "joints": joint_nodes, "inverseBindMatrices": ibm_a, "skeleton": roots[0]}],
    "materials": [{"name": "M_Infantry_Linje", "pbrMetallicRoughness": {
        "baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 0.85}}],
    "textures": [{"source": 0, "sampler": 0}],
    "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 33071, "wrapT": 33071}],
    "images": [{"bufferView": img_view, "mimeType": "image/png"}],
    "accessors": accessors,
    "bufferViews": views,
    "buffers": [{"byteLength": sum(len(b) for b in blobs)}],
}
if animations:
    gltf["animations"] = animations
js = json.dumps(gltf, separators=(",", ":")).encode()
js += b" " * ((-len(js)) % 4)
binary = b"".join(blobs)
with open(OUT_GLB, "wb") as f:
    f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(binary)))
    f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
    f.write(struct.pack("<II", len(binary), 0x004E4942) + binary)

np.savez(OUT_GLB + ".debug.npz", world=world, faces=tex_idx.reshape(-1, 3), uv=tex_uv, joints=top, weights=top_w,
         glob=np.stack([glob_m[m] for m in order]), parents=np.array([index_of.get(parent_of.get(m), -1) for m in order]),
         names=np.array([bone_name[m] for m in order]))
print(f"wrote {OUT_GLB}: {len(world)} verts, {len(tex_idx) // 3} tris, {len(order)} joints, "
      f"height {world[:, 1].max() - world[:, 1].min():.2f} m, facing check z-range {world[:, 2].min():.2f}..{world[:, 2].max():.2f}")
