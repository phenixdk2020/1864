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


def iter_shp(path):
    """Read one polygon record at a time; northern extension must not retain the whole world."""
    with open(path, "rb") as stream:
        stream.seek(100)
        while True:
            header = stream.read(8)
            if not header:
                break
            if len(header) != 8:
                raise ValueError("Truncated shapefile record header")
            _, content_len = struct.unpack(">II", header)
            data = stream.read(content_len * 2)
            if len(data) != content_len * 2:
                raise ValueError("Truncated shapefile record")
            shape_type = struct.unpack_from("<i", data)[0]
            if shape_type not in (3, 5):
                yield []
                continue
            n_parts, n_points = struct.unpack_from("<ii", data, 36)
            parts_idx = struct.unpack_from(f"<{n_parts}i", data, 44)
            pts_off = 44 + 4 * n_parts
            coords = struct.unpack_from(f"<{2 * n_points}d", data, pts_off)
            points = list(zip(coords[0::2], coords[1::2]))
            bounds = list(parts_idx) + [n_points]
            yield [points[bounds[k]:bounds[k + 1]] for k in range(n_parts)]


def read_shp(path):
    """Compatibility wrapper for existing full-map generators."""
    return list(iter_shp(path))


def iter_read(path_no_ext):
    """Geometry streams by record; DBF attributes are small and retain their record order."""
    return zip(iter_shp(path_no_ext + ".shp"), read_dbf(path_no_ext + ".dbf"))


def read(path_no_ext):
    return list(zip(read_shp(path_no_ext + ".shp"), read_dbf(path_no_ext + ".dbf")))
