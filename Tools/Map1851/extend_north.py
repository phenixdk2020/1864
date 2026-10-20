"""Tilføj nordligt raster uden at resample det danske kort. Ingen Unreal-start."""
import copy
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw


def extend(out, natural_earth):
    from build_map import Raster
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Countries"))
    from import_sweden_norway import patch
    from shp import iter_read as read
    out = Path(out)
    path = out / 'Denmark1851_Map.json'
    original = path.read_bytes()
    meta = json.loads(original)
    if 'northExtension' in meta:
        print('Nordudvidelse findes allerede; ingen ændringer.')
        return
    old = copy.deepcopy(meta['extentKm'])
    span = old['yMax'] - old['yMin']
    north = old['yMax'] + span
    neighbour = json.loads((out / 'cities_neighbours_1825.json').read_text(encoding='utf-8-sig'))
    width, height = neighbour['ownerGrid']['width'], neighbour['ownerGrid']['height']
    raster = Raster(old['xMin'], old['xMax'], old['yMax'], north, width, height)
    # Stream land shapes; loading the unused worldwide admin-1 dataset costs much more memory.
    land = np.zeros((height, width), dtype=bool)
    for shape, _ in read(str(Path(natural_earth) / 'ne_10m_land/ne_10m_land')):
        land |= raster.fill([shape])
    country_grid = np.zeros((height, width), np.uint8)
    ids = neighbour['ownerGrid']['ids']
    ids['NO'] = max(ids.values()) + 1
    for shapes, record in read(str(Path(natural_earth) / 'ne_10m_admin_0_countries/ne_10m_admin_0_countries')):
        nation = {'SWE': 'SE', 'NOR': 'NO'}.get(record.get('ADM0_A3'))
        if nation:
            country_grid[raster.fill([shapes]) & land] = ids[nation]
    # Coast geometry is Natural Earth; terrain relief is a visual estimate, not elevation data.
    for name in ('Denmark1851_Height.png', 'Denmark1851_Features.png', 'Denmark1851_Amter.png'):
        image = Image.open(out / name)
        if name.endswith('Height.png'):
            top = Image.fromarray((land.astype(np.uint16) * 1800)).resize(image.size, Image.Resampling.NEAREST)
        else:
            top = Image.new(image.mode, image.size, 0)
        result = Image.new(image.mode, (image.width, image.height * 2))
        result.paste(top, (0, 0)); result.paste(image, (0, image.height))
        result.save(out / name)
    for name, dtype in [('Neighbours_Owners.bin', np.uint8), ('South_Districts.bin', '<u2')]:
        raw = np.frombuffer((out / name).read_bytes(), dtype=dtype).reshape(height, width)
        top = country_grid if dtype == np.uint8 else np.zeros_like(raw)
        np.concatenate([top, raw]).tofile(out / name)
    meta['extentKm']['yMax'] = north
    meta['sizeKm']['height'] = round(north - old['yMin'], 2)
    if isinstance(meta['textureSize'], dict):
        meta['textureSize']['height'] *= 2
    else:
        meta['textureSize'][1] *= 2
    # Geographic bounding box is descriptive; exact raster bounds are extentKm.
    meta['extent']['latMax'] = 63.0
    meta['northExtension'] = {'originalExtentKm': old, 'originalMapSHA256': hashlib.sha256(original).hexdigest(),
                              'originalGridHeight': height, 'note': 'Sydlige pixels bevaret; nordligt relief er skøn. Kyst: Natural Earth 10m.'}
    # Only top-level scalar metadata changed. Avoid a token-span tree for the huge road array.
    text = original.decode('utf-8-sig')
    import re
    for field in ('extentKm', 'extent', 'sizeKm', 'textureSize'):
        pattern = r'("' + field + r'"\s*:\s*)\{[^}]*\}'
        text, count = re.subn(pattern, lambda match: match.group(1) + json.dumps(meta[field], ensure_ascii=False), text, count=1)
        assert count == 1, field
    newline = '\r\n' if b'\r\n' in original else '\n'
    end = text.rfind('}')
    text = text[:end] + ',' + newline + '"northExtension": ' + json.dumps(meta['northExtension'], ensure_ascii=False) + newline + text[end:]
    assert json.loads(text) == meta
    path.write_bytes(text.encode('utf-8'))
    for year in (1825, 1851):
        path = out / f'cities_neighbours_{year}.json'
        value = json.loads(path.read_text(encoding='utf-8-sig'))
        original_value = copy.deepcopy(value)
        for field in ('ownerGrid', 'districtGrid'):
            value[field]['height'] *= 2
        value['ownerGrid']['ids']['NO'] = ids['NO']
        patch(path, original_value, value)
    print(f'Nordudvidelse: ejergrid {width}x{height * 2}; danske pixels bevaret.')
