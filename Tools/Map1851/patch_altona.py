"""Altona and its ground back in Holsten (2026-11-05).

Natural Earth's boundaries are modern: Hamborg has included Altona since 1937, so the generated land mask
(Denmark1851_Features.png, R = monarchy land) and the owner grids (Neighbours_Owners_Full_<scenario>.bin) gave Altona,
Ottensen and the Elbe bank to Pinneberg to Hamborg/Hannover. Altona had no 3D town, no farms and no garrison plot.

This script marks the land north of the Elbe between Wedel and the old Hamborg boundary (Nobistor, about 9.958 E) as
the monarchy's again: R = 255 in the features mask, owner DK and district 0 ("sea or Danish") in both scenarios.
The Elbe line is the game's own (Data/Campaign1851/Hydro1851.json, "Elben"), so the river stays water.
Boundaries are an estimate (historic Altona/Pinneberg west of Hamborg), not surveyed. The script is idempotent;
run it again after build_map.py or export_neighbours.py regenerate those files:

    python -I Tools/Map1851/patch_altona.py
"""
import json
import math
import pathlib

import numpy as np
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA = ROOT / "Data" / "Campaign1851"
LON_WEST, LON_EAST, LAT_NORTH = 9.66, 9.958, 53.72   # Wedel ... Nobistor (Hamborg boundary), north to the Danish land
BANK = 0.004                                           # degrees north of the Elbe's centre line (about 450 m)


def forward(lat, lon, lat0=52.0, lon0=10.0, r=6371.0088):
    """The map's Lambert azimuthal equal-area projection (km), as Campaign1851Map.h."""
    lam = np.radians(lon - lon0); phi = np.radians(lat); phi0 = math.radians(lat0)
    k = np.sqrt(2.0 / (1.0 + math.sin(phi0) * np.sin(phi) + math.cos(phi0) * np.cos(phi) * np.cos(lam)))
    return r * k * np.cos(phi) * np.sin(lam), r * k * (math.cos(phi0) * np.sin(phi) - math.sin(phi0) * np.cos(phi) * np.cos(lam))


def inverse(x, y, lat0=52.0, lon0=10.0, r=6371.0088):
    phi0 = math.radians(lat0)
    rho = np.maximum(np.hypot(x, y), 1e-9)
    c = 2.0 * np.arcsin(np.clip(rho / (2.0 * r), -1, 1))
    lat = np.degrees(np.arcsin(np.cos(c) * math.sin(phi0) + y * np.sin(c) * math.cos(phi0) / rho))
    lon = lon0 + np.degrees(np.arctan2(x * np.sin(c), rho * math.cos(phi0) * np.cos(c) - y * math.sin(phi0) * np.sin(c)))
    return lat, lon


def elbe_lat(lon):
    hydro = json.loads((DATA / "Hydro1851.json").read_text(encoding="utf-8-sig"))
    elbe = next(r for r in hydro["rivers"] if r["name"] == "Elben")["points"]
    pts = sorted((p[1], p[0]) for p in elbe)                # (lon, lat), west to east
    return np.interp(lon, [p[0] for p in pts], [p[1] for p in pts])


def area(lat, lon):
    return (lon >= LON_WEST) & (lon <= LON_EAST) & (lat <= LAT_NORTH) & (lat >= elbe_lat(lon) + BANK)


def cell_grid(extent, w, h):
    """Centres of a north-to-south grid in km, as lat/lon."""
    xs = extent["xMin"] + (np.arange(w) + 0.5) / w * (extent["xMax"] - extent["xMin"])
    ys = extent["yMax"] - (np.arange(h) + 0.5) / h * (extent["yMax"] - extent["yMin"])
    return xs, ys


def patch_features():
    path = DATA / "Denmark1851_Features.png"
    extent = json.loads((DATA / "Denmark1851_Map.json").read_text(encoding="utf-8-sig"))["extentKm"]
    img = np.array(Image.open(path).convert("RGB"))
    h, w = img.shape[:2]
    xs, ys = cell_grid(extent, w, h)
    # Only the window round Altona is tested (the whole image is large).
    x0, y0 = forward(np.array(LAT_NORTH + 0.05), np.array(LON_WEST - 0.1))
    x1, y1 = forward(np.array(53.45), np.array(LON_EAST + 0.1))
    cols = np.where((xs >= min(x0, x1)) & (xs <= max(x0, x1)))[0]
    rows = np.where((ys >= min(y0, y1)) & (ys <= max(y0, y1)))[0]
    gx, gy = np.meshgrid(xs[cols], ys[rows])
    lat, lon = inverse(gx, gy)
    mask = area(lat, lon)
    window = img[rows[0]:rows[-1] + 1, cols[0]:cols[-1] + 1]
    changed = int((mask & (window[..., 0] <= 127)).sum())
    window[..., 0][mask] = 255
    Image.fromarray(img).save(path)
    print(f"features: {changed} pixels to monarchy land")


def patch_owners(scenario):
    meta = json.loads((DATA / f"cities_neighbours_full_{scenario}.json").read_text(encoding="utf-8-sig"))
    owner, district = meta["ownerGridFull"], meta["districtGridFull"]
    w, h, dk = owner["width"], owner["height"], owner["ids"]["DK"]
    owners = np.fromfile(DATA / owner["file"], np.uint8).reshape(h, w)
    districts = np.fromfile(DATA / district["file"], "<u2").reshape(district["height"], district["width"])
    assert districts.shape == owners.shape and owner["extentKm"] == district["extentKm"]
    xs, ys = cell_grid(owner["extentKm"], w, h)
    x0, y0 = forward(np.array(LAT_NORTH + 0.05), np.array(LON_WEST - 0.1))
    x1, y1 = forward(np.array(53.45), np.array(LON_EAST + 0.1))
    cols = np.where((xs >= min(x0, x1)) & (xs <= max(x0, x1)))[0]
    rows = np.where((ys >= min(y0, y1)) & (ys <= max(y0, y1)))[0]
    gx, gy = np.meshgrid(xs[cols], ys[rows])
    lat, lon = inverse(gx, gy)
    mask = area(lat, lon)
    win_o = owners[rows[0]:rows[-1] + 1, cols[0]:cols[-1] + 1]
    win_d = districts[rows[0]:rows[-1] + 1, cols[0]:cols[-1] + 1]
    changed = int((mask & (win_o != dk)).sum())
    win_o[mask] = dk
    win_d[mask] = 0
    owners.tofile(DATA / owner["file"])
    districts.tofile(DATA / district["file"])
    print(f"owners {scenario}: {changed} cells to DK")


if __name__ == "__main__":
    patch_features()
    for scenario in ("1825", "1851"):
        patch_owners(scenario)
