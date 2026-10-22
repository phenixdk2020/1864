"""Statisk kontrol af runtimefliser. Starter ikke Unreal og ?ndrer ikke kilderne."""
import hashlib
import json
from functools import lru_cache
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def validate(root=ROOT):
    data = root / "Data/Campaign1851"
    manifest_path = data / "MapTiles.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    index = json.loads((data / "MapTileHeights/Index.json").read_text(encoding="utf-8"))
    assert index["manifestSHA256"] == hashlib.sha256(manifest_path.read_bytes()).hexdigest(), "Runtime-data er for?ldede"
    assert index["heightSide"] == 257 and index["forestSide"] == 65
    tiles = {(t["x"], t["y"]): t for t in manifest["tiles"]}
    runtime = {}
    total = 0
    for entry in index["tiles"]:
        key = entry["x"], entry["y"]
        assert key in tiles and key not in runtime
        raw = (data / entry["file"]).read_bytes()
        assert len(raw) == entry["bytes"] == 257*257*2+65*65
        assert hashlib.sha256(raw).hexdigest() == entry["sha256"], entry["file"]
        runtime[key] = (np.frombuffer(raw[:257*257*2], dtype="<u2").reshape(257, 257), np.frombuffer(raw[257*257*2:], dtype="u1").reshape(65, 65))
        total += len(raw)
    assert set(runtime) == set(tiles)
    assert total < 80_000_000, "Codex 21-runtime overskrider 80 MB"
    edges = 0
    for (x, y), (height, forest) in runtime.items():
        for neighbour, side in (((x+1,y), 0), ((x,y+1), 1)):
            if neighbour not in runtime:
                continue
            nh, nf = runtime[neighbour]
            assert np.array_equal(height[:, -1] if side == 0 else height[-1, :], nh[:, 0] if side == 0 else nh[0, :]), (x,y,neighbour,"h?jdes?m")
            assert np.array_equal(forest[:, -1] if side == 0 else forest[-1, :], nf[:, 0] if side == 0 else nf[0, :]), (x,y,neighbour,"skovs?m")
            # LOD1/2 must share precisely the same edge samples at common vertices.
            for stride in (2,4):
                a = height[::stride, -1] if side == 0 else height[-1, ::stride]
                b = nh[::stride, 0] if side == 0 else nh[0, ::stride]
                assert np.array_equal(a,b)
            edges += 1

    @lru_cache(maxsize=12)
    def image(key, layer):
        if key not in tiles:
            return None
        with Image.open(root / tiles[key]["layers"][layer]["file"]) as im:
            return np.asarray(im).copy()

    # Independent scalar reference: bilinear source sampling at physical vertices.
    # Includes every edge/corner plus an interior sample lattice in every tile.
    samples = 0
    for (tx,ty), (height,forest) in runtime.items():
        for layer, target, side, step in (("Height",height,1024,4),("Features",forest,512,8)):
            coords = set((i,j) for i in range(0,len(target),8) for j in range(0,len(target),8))
            coords.update((i,j) for i in range(len(target)) for j in (0,len(target)-1))
            coords.update((i,j) for i in (0,len(target)-1) for j in range(len(target)))
            for iy,ix in coords:
                gx,gy = tx*side+ix*step-0.5, ty*side+iy*step-0.5
                sx,sy = int(np.floor(gx)),int(np.floor(gy))
                value = weight = 0.0
                for dy in (0,1):
                    for dx in (0,1):
                        px,py = sx+dx,sy+dy
                        source = image((px//side,py//side),layer)
                        if source is None:
                            continue
                        w = (1-abs(gx-px))*(1-abs(gy-py))
                        v = source[py%side,px%side]
                        value += (int(v[1]) if layer == "Features" else int(v))*w
                        weight += w
                expected = round(value/weight) if weight else 0
                assert int(target[iy,ix]) == expected, (tx,ty,layer,ix,iy,int(target[iy,ix]),expected)
                samples += 1
    grids = []
    for year in (1825,1851):
        scenario = json.loads((data/f"cities_neighbours_full_{year}.json").read_text(encoding="utf-8"))
        for field,itemsize in (("ownerGridFull",1),("districtGridFull",2)):
            grid = scenario[field]
            assert 0 < grid["width"] <= 4096 and 0 < grid["height"] <= 4096
            assert (data/grid["file"]).stat().st_size == grid["width"]*grid["height"]*itemsize
            ext = grid["extentKm"]
            # Roundtrip vertices using this raster's extent, not the Danish extent.
            for x,y in ((0,0),(grid["width"]//2,grid["height"]//2),(grid["width"]-1,grid["height"]-1)):
                kx = ext["xMin"]+(x+0.5)/grid["width"]*(ext["xMax"]-ext["xMin"])
                ky = ext["yMax"]-(y+0.5)/grid["height"]*(ext["yMax"]-ext["yMin"])
                assert int((kx-ext["xMin"])/(ext["xMax"]-ext["xMin"])*grid["width"]) == x
                assert int((ext["yMax"]-ky)/(ext["yMax"]-ext["yMin"])*grid["height"]) == y
            grids.append(dict(scenario=year,grid=field,width=grid["width"],height=grid["height"]))
    report = dict(status="OK",manifestSHA256=index["manifestSHA256"],tiles=len(runtime),sharedEdges=edges,sourceSamples=samples,runtimeBytes=total,grids=grids)
    (data / "MapTileRuntime_Validation.json").write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8",newline="\r\n")
    print(json.dumps(report,ensure_ascii=False))
    return report


if __name__ == "__main__":
    validate()
