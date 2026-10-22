"""Kompakte runtimehøjder og groft skovlag. Ingen downloads eller Unreal-start.
257x257 uint16-le pr. flise; 65x65 uint8 skov. Fælles rand samples
på samme globale pixelkoordinat (bilineært mellem pixelcentre).
"""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SIDE = 257


def generate():
    manifest_path = ROOT / "Data/Campaign1851/MapTiles.json"
    manifest_bytes = manifest_path.read_bytes()
    manifest_sha = hashlib.sha256(manifest_bytes).hexdigest()
    manifest = json.loads(manifest_bytes)
    assert not manifest.get("painterV2InProgress"), "Afslut flisemalingen først"
    assert manifest["tileSizePx"] == 1024
    assert manifest["heightEncoding"]["metersPerNormalizedUnit"] == 3000
    tiles = {(t["x"], t["y"]): t for t in manifest["tiles"]}
    cache = {}

    def source(key, layer):
        ck = (*key, layer)
        if ck not in cache:
            tile = tiles.get(key)
            if tile is None:
                return None
            with Image.open(ROOT / tile["layers"][layer]["file"]) as im:
                cache[ck] = np.asarray(im).copy()
        return cache[ck]

    def sample(gx, gy, layer):
        size = 1024 if layer == "Height" else 512
        ix, iy = np.floor(gx).astype(int), np.floor(gy).astype(int)
        out = np.zeros(gx.shape, dtype=np.float64)
        weights = np.zeros_like(out)
        for dy in (0, 1):
            for dx in (0, 1):
                xx, yy = ix+dx, iy+dy
                w = (1-np.abs(gx-xx))*(1-np.abs(gy-yy))
                keys = np.stack((xx//size, yy//size), axis=-1)
                for key in np.unique(keys.reshape(-1, 2), axis=0):
                    a = source(tuple(key), layer)
                    if a is None:
                        continue
                    mask = (keys[..., 0] == key[0]) & (keys[..., 1] == key[1])
                    val = a[yy[mask] % size, xx[mask] % size]
                    if layer == "Features":
                        val = val[:, 1]
                    out[mask] += val*w[mask]
                    weights[mask] += w[mask]
        return np.rint(np.divide(out, weights, out=out, where=weights>0))

    dest = ROOT / "Data/Campaign1851/MapTileHeights"
    dest.mkdir(exist_ok=True)
    entries = []
    # Trim decoded PNG cache after each tile; neighbours alone are needed.
    for key, tile in sorted(tiles.items()):
        axis = np.arange(SIDE)*4.0-0.5
        gx, gy = np.meshgrid(axis+key[0]*1024, axis+key[1]*1024)
        height = sample(gx, gy, "Height").astype("<u2")
        forest = sample(gx[::4, ::4]/2-0.25, gy[::4, ::4]/2-0.25, "Features").astype("u1")
        name = f"Tile_{key[0]}_{key[1]}"
        raw = height.tobytes()+forest.tobytes()
        (dest/(name+".bin")).write_bytes(raw)
        entries.append(dict(x=key[0], y=key[1], file="MapTileHeights/"+name+".bin", bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest()))
        cache.clear()
    assert hashlib.sha256(manifest_path.read_bytes()).hexdigest() == manifest_sha, "Manifest ændret under generering"
    index = dict(version=1, manifestSHA256=manifest_sha, heightSide=SIDE, forestSide=65, encoding="uint16-le + uint8 forest", tiles=entries)
    (dest/"Index.json").write_text(json.dumps(index, ensure_ascii=False, indent=2)+"\n", encoding="utf-8", newline="\r\n")
    print(f"Runtime: {len(entries)} fliser, {sum(t['bytes'] for t in entries)} bytes")


if __name__ == "__main__":
    generate()
