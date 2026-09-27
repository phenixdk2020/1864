import sys
import numpy as np
from PIL import Image
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
p = np.load(sys.argv[1] + ".preview.npz"); tex = np.asarray(Image.open(sys.argv[1] + ".tex.png"))
h, w = tex.shape[:2]
def parts():
    tri = p["body"][p["body_tris"]]; uvc = p["uv"][p["body_tris"]].mean(1)
    yield tri, tex[np.clip((uvc[:, 1] * h).astype(int), 0, h-1), np.clip((uvc[:, 0] * w).astype(int), 0, w-1)] / 255.0
    yield p["wood"][p["wood_tris"]], np.tile([0.32, 0.16, 0.06], (len(p["wood_tris"]), 1))
    yield p["steel"][p["steel_tris"]], np.tile([0.7, 0.72, 0.75], (len(p["steel_tris"]), 1))
fig = plt.figure(figsize=(15, 8))
for k, az in enumerate([0, 45, 90, 180]):   # az 0 = looking from +X (the figure's front)
    ax = fig.add_subplot(1, 4, k + 1, projection="3d")
    for tri, col in parts():
        T = tri[:, :, [0, 2, 1]]
        n = np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0]); n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
        view = np.array([np.cos(np.radians(az)), np.sin(np.radians(az)), 0.1])
        ax.add_collection3d(Poly3DCollection(T, facecolors=np.clip(col * (0.45 + 0.55 * np.abs(n @ view))[:, None], 0, 1), edgecolor="none"))
    ax.set_xlim(-0.7, 0.7); ax.set_ylim(-0.7, 0.7); ax.set_zlim(0, 2.1); ax.set_box_aspect((1, 1, 1.5))
    ax.view_init(5, az); ax.set_axis_off(); ax.set_title(f"az {az}")
plt.tight_layout(); plt.savefig(sys.argv[2], dpi=80)
