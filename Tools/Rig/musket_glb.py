"""Stand-alone musket (butt at origin, muzzle/bayonet along +Y, metres) as a glb with wood + steel materials."""
import json, struct, sys
import numpy as np

def box(y0, y1, w0, d0, w1=None, d1=None, cz=0.0):
    w1 = w0 if w1 is None else w1; d1 = d0 if d1 is None else d1
    ring = lambda y, w, d: [(-w/2, y, cz-d/2), (w/2, y, cz-d/2), (w/2, y, cz+d/2), (-w/2, y, cz+d/2)]
    lo, hi = ring(y0, w0, d0), ring(y1, w1, d1)
    quads = [lo[::-1], hi] + [[lo[i], lo[(i+1) % 4], hi[(i+1) % 4], hi[i]] for i in range(4)]
    v, t = [], []
    for q in quads:
        b = len(v); v += q; t += [[b, b+1, b+2], [b, b+2, b+3]]
    return np.array(v, np.float32), np.array(t, np.uint32)

def merge(parts):
    vs, ts, o = [], [], 0
    for v, t in parts:
        vs.append(v); ts.append(t + o); o += len(v)
    return np.concatenate(vs), np.concatenate(ts)

def normals(v, t):
    n = np.zeros_like(v); fn = np.cross(v[t[:, 1]] - v[t[:, 0]], v[t[:, 2]] - v[t[:, 0]])
    for c in range(3): np.add.at(n, t[:, c], fn)
    return (n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-9)).astype(np.float32)

wood = merge([box(0.00, 0.30, 0.045, 0.11, 0.040, 0.075), box(0.30, 0.48, 0.035, 0.050), box(0.48, 1.30, 0.032, 0.045, 0.028, 0.034)])
steel = merge([box(0.00, 0.02, 0.047, 0.112), box(0.50, 0.58, 0.012, 0.070), box(0.60, 1.45, 0.020, 0.020, 0.018, 0.018, cz=0.012),
               box(0.90, 0.92, 0.036, 0.040), box(1.18, 1.20, 0.032, 0.036), box(1.40, 1.95, 0.016, 0.006, 0.003, 0.002, cz=0.030)])

blobs, views, accs = [], [], []
def blob(raw, target):
    off = sum(len(b) for b in blobs); blobs.append(raw + b"\0" * ((-len(raw)) % 4))
    views.append({"buffer": 0, "byteOffset": off, "byteLength": len(raw), "target": target}); return len(views) - 1
def acc(a, comp, typ, target, mm=False):
    d = {"bufferView": blob(a.tobytes(), target), "componentType": comp, "count": int(len(a)), "type": typ}
    if mm: d["min"], d["max"] = a.min(0).tolist(), a.max(0).tolist()
    accs.append(d); return len(accs) - 1
prims = []
for mat, (v, t) in enumerate((wood, steel)):
    prims.append({"attributes": {"POSITION": acc(v, 5126, "VEC3", 34962, True), "NORMAL": acc(normals(v, t), 5126, "VEC3", 34962)},
                  "indices": acc(t.reshape(-1), 5125, "SCALAR", 34963), "material": mat})
g = {"asset": {"version": "2.0", "generator": "PROJECT 1864 musket"}, "scene": 0, "scenes": [{"nodes": [0]}],
     "nodes": [{"name": "SM_Musket", "mesh": 0}], "meshes": [{"name": "SM_Musket", "primitives": prims}],
     "materials": [{"name": "M_Musket_Wood", "pbrMetallicRoughness": {"baseColorFactor": [0.20, 0.09, 0.035, 1], "metallicFactor": 0, "roughnessFactor": 0.6}},
                   {"name": "M_Musket_Steel", "pbrMetallicRoughness": {"baseColorFactor": [0.55, 0.57, 0.6, 1], "metallicFactor": 0.9, "roughnessFactor": 0.35}}],
     "accessors": accs, "bufferViews": views, "buffers": [{"byteLength": sum(len(b) for b in blobs)}]}
js = json.dumps(g, separators=(",", ":")).encode(); js += b" " * ((-len(js)) % 4); bin_ = b"".join(blobs)
with open(sys.argv[1], "wb") as f:
    f.write(struct.pack("<III", 0x46546C67, 2, 28 + len(js) + len(bin_)))
    f.write(struct.pack("<II", len(js), 0x4E4F534A) + js); f.write(struct.pack("<II", len(bin_), 0x004E4942) + bin_)
print("musket written", sys.argv[1])
