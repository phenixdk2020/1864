"""Minimal binary FBX (7.x) reader: node tree with properties, arrays decompressed."""
import struct
import sys
import zlib

import numpy as np


class Node:
    __slots__ = ("name", "props", "children")

    def __init__(self, name, props, children):
        self.name, self.props, self.children = name, props, children

    def find(self, name):
        return [c for c in self.children if c.name == name]

    def first(self, name):
        r = self.find(name)
        return r[0] if r else None


def _read_prop(data, pos):
    t = chr(data[pos]); pos += 1
    if t == "Y": return struct.unpack_from("<h", data, pos)[0], pos + 2
    if t == "C": return bool(data[pos]), pos + 1
    if t == "I": return struct.unpack_from("<i", data, pos)[0], pos + 4
    if t == "F": return struct.unpack_from("<f", data, pos)[0], pos + 4
    if t == "D": return struct.unpack_from("<d", data, pos)[0], pos + 8
    if t == "L": return struct.unpack_from("<q", data, pos)[0], pos + 8
    if t in "SR":
        n = struct.unpack_from("<I", data, pos)[0]; pos += 4
        raw = data[pos:pos + n]
        return (raw.decode("utf-8", "replace") if t == "S" else raw), pos + n
    if t in "fdlib":
        count, enc, clen = struct.unpack_from("<III", data, pos); pos += 12
        raw = data[pos:pos + clen]; pos += clen
        if enc == 1:
            raw = zlib.decompress(raw)
        dt = {"f": "<f4", "d": "<f8", "l": "<i8", "i": "<i4", "b": "u1"}[t]
        return np.frombuffer(raw, dt, count), pos
    raise ValueError("prop type " + t)


def _read_node(data, pos, wide):
    if wide:
        end, nprops, plen = struct.unpack_from("<QQQ", data, pos); pos += 24
    else:
        end, nprops, plen = struct.unpack_from("<III", data, pos); pos += 12
    nlen = data[pos]; pos += 1
    if end == 0:
        return None, pos
    name = data[pos:pos + nlen].decode("ascii", "replace"); pos += nlen
    props = []
    for _ in range(nprops):
        p, pos = _read_prop(data, pos)
        props.append(p)
    children = []
    sentinel = 25 if wide else 13
    while pos < end - sentinel or (pos < end and end - pos > sentinel):
        child, pos = _read_node(data, pos, wide)
        if child is None:
            break
        children.append(child)
    return Node(name, props, children), end


def read(path):
    data = open(path, "rb").read()
    version = struct.unpack_from("<I", data, 23)[0]
    wide = version >= 7500
    pos = 27
    top = []
    while pos < len(data):
        node, pos = _read_node(data, pos, wide)
        if node is None:
            break
        top.append(node)
    return Node("root", [version], top)


if __name__ == "__main__":
    root = read(sys.argv[1])
    print("FBX version", root.props[0])
    objects = root.first("Objects")
    for g in objects.find("Geometry"):
        v = g.first("Vertices").props[0]
        idx = g.first("PolygonVertexIndex").props[0]
        polys = int((idx < 0).sum())
        layers = [c.name for c in g.children if c.name.startswith("Layer")]
        uv = g.first("LayerElementUV")
        print("Geometry", g.props[1], g.props[2], "verts", len(v) // 3, "polys", polys, "layers", sorted(set(layers)),
              "UV" if uv is not None else "no UV")
        vv = v.reshape(-1, 3)
        print("  bounds", vv.min(0).round(2), vv.max(0).round(2))
    for kind in ("Model", "Deformer", "Material", "Texture", "Video", "Pose", "AnimationStack"):
        nodes = objects.find(kind)
        if nodes:
            sub = {}
            for n in nodes:
                sub[n.props[2] if len(n.props) > 2 else ""] = sub.get(n.props[2] if len(n.props) > 2 else "", 0) + 1
            print(kind, len(nodes), sub)
    for t in objects.find("Texture"):
        fn = t.first("RelativeFilename") or t.first("FileName")
        print("  texture file:", fn.props[0] if fn else None)
    for vid in objects.find("Video"):
        content = vid.first("Content")
        print("  embedded video bytes:", len(content.props[0]) if content and content.props and isinstance(content.props[0], bytes) else 0)
    gs = root.first("GlobalSettings")
    if gs:
        for p in gs.first("Properties70").children:
            if p.props[0] in ("UpAxis", "FrontAxis", "CoordAxis", "UnitScaleFactor", "OriginalUnitScaleFactor"):
                print(" ", p.props[0], p.props[-1])
