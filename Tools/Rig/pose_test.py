import json, struct, io
import numpy as np
from PIL import Image
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

d = np.load("Infantry_Rigged.glb.debug.npz")
world, faces, uv, joints, weights = d["world"], d["faces"], d["uv"], d["joints"], d["weights"]
G, parents, names = d["glob"], d["parents"], list(d["names"])
data = open("Infantry_Rigged.glb", "rb").read()
jl = struct.unpack_from("<I", data, 12)[0]; gj = json.loads(data[20:20+jl]); b = data[20+jl+8:]
bv = gj["bufferViews"][gj["images"][0]["bufferView"]]
tex = np.asarray(Image.open(io.BytesIO(b[bv["byteOffset"]:bv["byteOffset"]+bv["byteLength"]])).convert("RGB"))

def rot(axis, deg):
    a = np.radians(deg); c, s = np.cos(a), np.sin(a)
    m = np.eye(4)
    i, j = {"x": (1, 2), "y": (2, 0), "z": (0, 1)}[axis]
    m[i, i], m[i, j], m[j, i], m[j, j] = c, s, -s, c
    return m

def posed(rotations):
    local = [G[k] @ np.linalg.inv(G[parents[k]]) if parents[k] >= 0 else G[k] for k in range(len(G))]
    Gp = [None] * len(G)
    for k in range(len(G)):
        L = rotations.get(names[k], np.eye(4)) @ local[k]
        Gp[k] = L @ Gp[parents[k]] if parents[k] >= 0 else L
    skin = np.stack([np.linalg.inv(G[k]) @ Gp[k] for k in range(len(G))])
    v4 = np.c_[world, np.ones(len(world))]
    out = np.zeros_like(world)
    for slot in range(4):
        m = skin[joints[:, slot]]
        out += weights[:, slot, None] * np.einsum("vi,vij->vj", v4, m)[:, :3]
    return out

def render(ax, verts, az, title):
    P = verts[:, [0, 2, 1]]
    tri = P[faces]
    uvc = uv[faces].mean(1); h, w = tex.shape[:2]
    cols = tex[np.clip((uvc[:, 1] * h).astype(int), 0, h-1), np.clip((uvc[:, 0] * w).astype(int), 0, w-1)] / 255.0
    n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]); n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
    view = np.array([np.cos(np.radians(az)), np.sin(np.radians(az)), 0.1])
    shade = 0.45 + 0.55 * np.abs(n @ view)
    ax.add_collection3d(Poly3DCollection(tri, facecolors=np.clip(cols * shade[:, None], 0, 1), edgecolor="none"))
    lo = world[:, 1].min(); ax.set_xlim(-0.8, 0.8); ax.set_ylim(-0.8, 0.8); ax.set_zlim(lo, lo + 2.0); ax.set_box_aspect((1, 1, 1.25))
    ax.view_init(5, az); ax.set_axis_off(); ax.set_title(title)

print("joints:", names)
poses = {
    "arm X+60": {"mixamorig:LeftArm": rot("x", 60), "mixamorig:LeftUpLeg": rot("x", -45), "mixamorig:LeftLeg": rot("x", 80)},
    "arm Y+60": {"mixamorig:LeftArm": rot("y", 60), "mixamorig:RightUpLeg": rot("x", 45), "mixamorig:RightLeg": rot("x", 80)},
}
fig = plt.figure(figsize=(16, 8))
k = 1
for title, r in poses.items():
    v = posed(r)
    for az in (90, 0):
        ax = fig.add_subplot(1, 4, k, projection="3d"); k += 1
        render(ax, v, az, f"{title} ({'front' if az == 90 else 'side'})")
plt.tight_layout(); plt.savefig("pose_test.png", dpi=80)
