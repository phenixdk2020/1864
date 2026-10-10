"""Eksportér naboer uden build/spilstart. Historiske grænser og styrker er skøn.

Ejergrid: uint8, række 0 mod nord, samme LAEA-udsnit som Map.json.
Natural Earth er kun land/kyst; periodens ejergrænser kommer fra neighbours.json.
"""
import argparse
import heapq
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from export_south import enrich, scenario_districts, scenario_oob

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def project(lon, lat):
    lam, phi, phi0 = np.radians(lon - 10), np.radians(lat), math.radians(52)
    k = np.sqrt(2 / (1 + math.sin(phi0) * np.sin(phi) + math.cos(phi0) * np.cos(phi) * np.cos(lam)))
    return 6371.0088 * k * np.cos(phi) * np.sin(lam), 6371.0088 * k * (math.cos(phi0) * np.sin(phi) - math.sin(phi0) * np.cos(phi) * np.cos(lam))


def export(out, natural_earth=None):
    base = json.loads((out / 'Denmark1851_Map.json').read_text(encoding='utf-8-sig'))
    source = enrich(json.loads((HERE / 'neighbours.json').read_text(encoding='utf-8')))
    extent = base['extentKm']
    x0, x1, y0, y1 = (extent[k] for k in ('xMin', 'xMax', 'yMin', 'yMax'))
    w, h = 768, round(768 * (y1-y0)/(x1-x0))

    def pixel(lon, lat):
        x, y = project(lon, lat)
        return (x-x0)/(x1-x0)*w, (y1-y)/(y1-y0)*h

    def cell(lon, lat):
        x, y = pixel(lon, lat)
        return min(w-1, max(0, int(x))), min(h-1, max(0, int(y)))

    def km(x, y):
        return [round(x0+(x+.5)/w*(x1-x0), 3), round(y1-(y+.5)/h*(y1-y0), 3)]

    own = np.asarray(Image.open(out / 'Denmark1851_Features.png').convert('RGB').resize((w,h), Image.Resampling.NEAREST))[...,0] > 127
    height = np.asarray(Image.open(out / 'Denmark1851_Height.png').resize((w,h), Image.Resampling.NEAREST))
    land = (height >= 0.002 * 65535) | own  # uint16: runtime IsSea threshold; explicit fallback
    coast_source = 'Eksisterende højde-/Features-raster; kystafgrænsning er tilnærmet'
    if natural_earth:
        from shp import read
        canvas = Image.new('L', (w,h))
        draw = ImageDraw.Draw(canvas)
        for parts, _ in read(str(natural_earth / 'ne_10m_land' / 'ne_10m_land')):
            for ring in parts:
                if len(ring) < 3:
                    continue
                area = sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(ring,ring[1:]+ring[:1]))
                draw.polygon([pixel(*p) for p in ring], fill=255 if area < 0 else 0)
        land = (np.asarray(canvas)>127) | own
        coast_source = 'Natural Earth 10m land (public domain), monarkiets eksisterende maske har forrang'

    ids = {n['id']: i+1 for i,n in enumerate(source['nations'])}
    # Jutland/islands occupy the north-west; foreign polygons override this default.
    owners = Image.new('L',(w,h),ids['DK'])
    draw = ImageDraw.Draw(owners)
    for territory in source['territories']:
        draw.polygon([pixel(*p) for p in territory['polygon']], fill=ids[territory['owner']])
    grid = np.array(owners)
    grid[~land] = 0
    grid[own] = ids['DK']
    # Historic river enclaves are below raster resolution: keep a small land-only city centre.
    for c in source['cities']:
        x,y = cell(c['lon'],c['lat'])
        r = 2
        patch = grid[max(0,y-r):y+r+1,max(0,x-r):x+r+1]
        patch_land = land[max(0,y-r):y+r+1,max(0,x-r):x+r+1]
        patch_own = own[max(0,y-r):y+r+1,max(0,x-r):x+r+1]
        patch[patch_land & ~patch_own] = ids[c['owner']]
        # Named foreign centres are authoritative at the coast and in the modern
        # Schleswig-Holstein mask (Luebeck was a free city, not Danish territory).
        grid[y,x] = ids[c['owner']]
    assert not np.any(land & (grid == 0)), 'Land mangler ejer'
    grid.tofile(out / 'Neighbours_Owners.bin')

    # District geometry is an explicit nearest-seat estimate, clipped to nation ownership.
    # It must never create Danish districts or change the northern layer.
    district_grid = np.zeros((h, w), dtype='<u2')
    yy, xx = np.indices((h, w))
    for nation in {d['nation'] for d in source['south']['districts']}:
        candidates = [(i+1, d) for i,d in enumerate(source['south']['districts']) if d['nation']==nation]
        distance = np.full((h,w), np.inf)
        for number, district in candidates:
            px, py = pixel(district['lon'],district['lat'])
            squared = (xx-px)**2 + (yy-py)**2
            selected = (grid==ids[nation]) & (squared<distance)
            district_grid[selected] = number
            distance[selected] = squared[selected]
    district_ids = {d['id']:i+1 for i,d in enumerate(source['south']['districts'])}
    for city in source['cities']:
        if city.get('district'):
            cx, cy = cell(city['lon'],city['lat'])
            district_grid[cy,cx] = district_ids[city['district']]
    district_grid.tofile(out / 'South_Districts.bin')

    all_cities = {c['name']: c for c in base['cities']+base['foreignCities']+source['cities']}
    links = []
    # Grid A*: land-only, bounded to this small raster. Ferries must be explicit.
    dry = ndimage.binary_dilation(land, iterations=1)
    def route(a,b,ferry):
        start, end = cell(a['lon'],a['lat']), cell(b['lon'],b['lat'])
        if ferry:
            return [list(project(a['lon'],a['lat'])), list(project(b['lon'],b['lat']))]
        queue, costs, previous = [(0,start)], {start:0}, {}
        while queue:
            _, at = heapq.heappop(queue)
            if at == end:
                path = [at]
                while path[-1] != start:
                    path.append(previous[path[-1]])
                points = [list(project(a['lon'],a['lat']))]+[km(*p) for p in reversed(path)]+[list(project(b['lon'],b['lat']))]
                return points
            for dx,dy in ((1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)):
                nx,ny = at[0]+dx,at[1]+dy
                if not (0<=nx<w and 0<=ny<h) or (not dry[ny,nx] and (nx,ny)!=end):
                    continue
                if dx and dy and (not dry[at[1],nx] or not dry[ny,at[0]]):
                    continue
                nxt = nx,ny
                cost = costs[at]+math.hypot(dx,dy)
                if cost >= costs.get(nxt,float('inf')):
                    continue
                costs[nxt],previous[nxt] = cost,at
                heapq.heappush(queue,(cost+math.hypot(nx-end[0],ny-end[1]),nxt))
        raise ValueError(f'Ingen landrute: {a["name"]}–{b["name"]}')

    for row in source['roads']:
        a,b = (all_cities[n] for n in row[:2])
        ferry = row[2] if len(row)>2 else ''
        points = route(a,b,ferry)
        length = sum(math.dist(a,b) for a,b in zip(points,points[1:]))
        links.append(dict(a=a['name'],b=b['name'],km=points,roadKm=0 if ferry else round(length,3),ferryKm=round(length,3) if ferry else 0,ferry=ferry))

    for year in (1825,1851):
        nations = {n['id'] for n in json.loads((out / ('Nations_1825.json' if year==1825 else 'Nations1851.json')).read_text(encoding='utf-8'))['nations']}
        cities = []
        for c in source['cities']:
            x,y = project(c['lon'],c['lat'])
            assert x0 <= x <= x1 and y0 <= y <= y1, c['name']
            assert c['owner'] in nations
            cities.append(dict(c, pop=c['population'][str(year)], railway=year>=c.get('railwayYear',9999)))
        scenario_links = [dict(link) for link in links]
        for link in scenario_links:
            link['chaussee'] = any(year>=r['opened'] and {link['a'],link['b']}=={r['a'],r['b']} for r in source['south']['chaussees'])
        railways = []
        for rail in source['railways']:
            if year < int(rail['opened'][:4]):
                continue
            points = []
            for a,b in zip(rail['via'],rail['via'][1:]):
                link = next(link for link in scenario_links if {link['a'],link['b']} == {a,b})
                path = link['km'] if link['a']==a else list(reversed(link['km']))
                link['rail'] = link['km']
                points.extend(path if not points else path[1:])
            railways.append(dict(rail, towns=rail['via'], stations=rail['via'], km=points))
        districts = scenario_districts(source,year,cities)
        data = dict(scenario=str(year),note=source['note'],cities=cities,links=scenario_links,
                    nations=source['nations'],territories=source['territories'],
                    ownerGrid=dict(file='Neighbours_Owners.bin',width=w,height=h,ids=ids,coastSource=coast_source),
                    outside=source['outside'],railways=railways,districts=districts,
                    districtGrid=dict(file='South_Districts.bin',width=w,height=h,ids=district_ids,status='estimated'))
        (out / f'cities_neighbours_{year}.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        armies = []
        for c in cities:
            garrison = dict(id='garrison-'+c['name'],name='Garnison i '+c['name'],nation=c['owner'],town=c['name'],garrison=True,men=c['garrison'],mobilisedMen=c['garrison'],guns=c['guns'],district=c.get('district',''))
            if c.get('district') and c['fortress']:
                gunmen = min(c['garrison']//2, c['guns']*8)
                garrison.update(status='estimated',chief='',chiefStatus='unknown',
                                companies=[dict(name='Fæstningsinfanteri',men=c['garrison']-gunmen)],
                                batteries=[dict(name='Fæstningsartilleri',men=gunmen,guns=c['guns'])])
            armies.append(garrison)
        for army in source['armies']:
            # Keep old aggregate IDs as migration anchors, without double-counting the regiments.
            detailed = army['id'] in ('PR-pomerania','MEC-field')
            armies.append(dict(army,men=0 if detailed else army['peaceMen'][str(year)],mobilisedMen=0 if detailed else army['warMen'][str(year)],legacyAggregate=detailed))
        for unit in scenario_oob(source,year):
            armies.append(dict(unit,garrison=False))
        (out / f'Army_Neighbours_{year}.json').write_text(json.dumps(dict(scenario=str(year),note=source['note'],armies=armies),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        economy = dict(scenario=str(year),note=source['south']['note'],currency='rd. sølv, sammenlignelig spilregneenhed; ikke historisk valutakurs',districts=districts)
        (out / f'Economy_South_{year}.json').write_text(json.dumps(economy,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'CAMPAIGN-1851|neighbours|{len(cities)} byer|{len(links)} ruter|grid={w}x{h}|{coast_source}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,default=ROOT / 'Data/Campaign1851')
    parser.add_argument('--natural-earth',type=Path)
    args = parser.parse_args()
    export(args.out,args.natural_earth)
