"""Minimal ESRI shapefile reader (polygons, polylines, dBASE attributes). No dependencies."""
import struct


def read_dbf(path):
    with open(path, "rb") as f:
        data = f.read()
    n_records, header_len, record_len = struct.unpack("<IHH", data[4:12])
    fields = []
    pos = 32
    while data[pos] != 0x0D:
        name = data[pos:pos + 11].split(b"\0")[0].decode("latin-1")
        ftype = chr(data[pos + 11])
        length = data[pos + 16]
        fields.append((name, ftype, length))
        pos += 32
    records = []
    for i in range(n_records):
        off = header_len + i * record_len + 1  # skip deletion flag
        rec = {}
        for name, ftype, length in fields:
            raw = data[off:off + length].rstrip(b"\0 ")
            off += length
            try:
                text = raw.decode("utf-8").strip()
            except UnicodeDecodeError:
                text = raw.decode("cp1252", errors="replace").strip()
            if ftype in "NF":
                try:
                    rec[name] = float(text) if text else None
                except ValueError:
                    rec[name] = None
            else:
                rec[name] = text
        records.append(rec)
    return records


def read_shp(path):
    """Returns a list of shapes; each shape is a list of parts; each part a list of (lon, lat)."""
    with open(path, "rb") as f:
        data = f.read()
    shapes = []
    pos = 100
    while pos < len(data):
        _, content_len = struct.unpack(">II", data[pos:pos + 8])
        pos += 8
        shape_type = struct.unpack("<i", data[pos:pos + 4])[0]
        if shape_type in (3, 5):  # polyline, polygon
            n_parts, n_points = struct.unpack("<ii", data[pos + 36:pos + 44])
            parts_idx = struct.unpack(f"<{n_parts}i", data[pos + 44:pos + 44 + 4 * n_parts])
            pts_off = pos + 44 + 4 * n_parts
            coords = struct.unpack(f"<{2 * n_points}d", data[pts_off:pts_off + 16 * n_points])
            points = list(zip(coords[0::2], coords[1::2]))
            bounds = list(parts_idx) + [n_points]
            shapes.append([points[bounds[k]:bounds[k + 1]] for k in range(n_parts)])
        else:
            shapes.append([])
        pos += content_len * 2
    return shapes


def read(path_no_ext):
    return list(zip(read_shp(path_no_ext + ".shp"), read_dbf(path_no_ext + ".dbf")))
