"""Evaluate Mixamo FBX animation curves into per-bone local matrices (row-vector convention, metres)."""
import numpy as np

from fbxread import read

KTIME = 46186158000.0  # FBX ticks per second
CM_TO_M = 0.01


def euler_xyz_row(deg):
    """FBX eEulerXYZ (R = Rz @ Ry @ Rx in column convention), returned as row-vector matrices (N,4,4)."""
    x, y, z = np.radians(deg[:, 0]), np.radians(deg[:, 1]), np.radians(deg[:, 2])
    cx, sx, cy, sy, cz, sz = np.cos(x), np.sin(x), np.cos(y), np.sin(y), np.cos(z), np.sin(z)
    n = len(deg)
    rx = np.zeros((n, 3, 3)); rx[:, 0, 0] = 1; rx[:, 1, 1] = cx; rx[:, 1, 2] = -sx; rx[:, 2, 1] = sx; rx[:, 2, 2] = cx
    ry = np.zeros((n, 3, 3)); ry[:, 1, 1] = 1; ry[:, 0, 0] = cy; ry[:, 0, 2] = sy; ry[:, 2, 0] = -sy; ry[:, 2, 2] = cy
    rz = np.zeros((n, 3, 3)); rz[:, 2, 2] = 1; rz[:, 0, 0] = cz; rz[:, 0, 1] = -sz; rz[:, 1, 0] = sz; rz[:, 1, 1] = cz
    col = rz @ ry @ rx
    out = np.tile(np.eye(4), (n, 1, 1))
    out[:, :3, :3] = np.transpose(col, (0, 2, 1))
    return out


def translation_row(t):
    out = np.tile(np.eye(4), (len(t), 1, 1))
    out[:, 3, :3] = t
    return out


def load_clip(path, fps=30.0):
    """Returns (times[F], {bone_name: local_row[F,4,4]}) with bone names stripped of their namespace."""
    root = read(path)
    obj = root.first("Objects")
    conns = [c.props for c in root.first("Connections").children]
    models = {m.props[0]: m for m in obj.find("Model") if m.props[2] == "LimbNode"}
    nodes = {n.props[0]: n for n in obj.find("AnimationCurveNode")}
    curves = {n.props[0]: n for n in obj.find("AnimationCurve")}

    def props70(m):
        p = m.first("Properties70")
        return {c.props[0]: np.array(c.props[4:], np.float64) for c in p.children} if p else {}

    channels = {}  # (model, prop) -> {axis: (times, values)}
    t_end = 0.0
    for c in conns:
        if c[0] == "OP" and c[1] in nodes and c[2] in models:
            chan = {}
            for cc in conns:
                if cc[0] == "OP" and cc[2] == c[1] and cc[1] in curves:
                    cv = curves[cc[1]]
                    kt = cv.first("KeyTime").props[0] / KTIME
                    kv = cv.first("KeyValueFloat").props[0].astype(np.float64)
                    chan[cc[3]] = (kt, kv)
                    t_end = max(t_end, float(kt.max()))
            channels[(c[2], c[3])] = chan

    frames = int(round(t_end * fps)) + 1
    times = np.arange(frames) / fps

    def sample(mid, prop, default):
        chan = channels.get((mid, prop), {})
        out = np.tile(default, (frames, 1)).astype(np.float64)
        for i, axis in enumerate(("d|X", "d|Y", "d|Z")):
            if axis in chan:
                kt, kv = chan[axis]
                out[:, i] = np.interp(times, kt, kv)
        return out

    locals_ = {}
    for mid, m in models.items():
        name = m.props[1].split("\x00")[0].split(":")[-1]
        p = props70(m)
        t = sample(mid, "Lcl Translation", p.get("Lcl Translation", np.zeros(3))) * CM_TO_M
        r = sample(mid, "Lcl Rotation", p.get("Lcl Rotation", np.zeros(3)))
        pre = euler_xyz_row(np.tile(p.get("PreRotation", np.zeros(3)), (frames, 1)))
        # Column convention: L = T * Rpre * R  ->  row convention: R_row @ Rpre_row @ T_row.
        locals_[name] = euler_xyz_row(r) @ pre @ translation_row(t)
    return times, locals_
