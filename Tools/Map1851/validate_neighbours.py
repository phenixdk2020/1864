"""Statisk kontrol af nabodata; starter hverken Unreal eller spillet."""
import ast
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / 'Data/Campaign1851'

def read(name):
    return json.loads((DATA/name).read_text(encoding='utf-8-sig'))

base = read('Denmark1851_Map.json')
base_names = [c['name'] for c in base['cities']+base['foreignCities']]
extent = base['extentKm']
for year in (1825,1851):
    cities = read(f'cities_neighbours_{year}.json')
    armies = read(f'Army_Neighbours_{year}.json')
    nations = read('Nations_1825.json' if year==1825 else 'Nations1851.json')['nations']
    nation_ids = [n['id'] for n in nations]
    assert len(nation_ids)==len(set(nation_ids)), 'Dublerede nationer'
    assert cities['scenario']==armies['scenario']==str(year)
    rows = cities['cities']
    names = [c['name'] for c in rows]
    assert len(names)==len(set(names))
    grid = cities['ownerGrid']
    raw = (DATA/grid['file']).read_bytes()
    assert len(raw)==grid['width']*grid['height']
    assert set(raw) <= {0,*grid['ids'].values()}
    assert set(grid['ids']) <= set(nation_ids)
    all_names = set(base_names+names)
    for town in rows:
        assert town['owner'] in nation_ids and town['pop']>0
        assert town['name'] not in [c['name'] for c in base['cities']]
        assert town['garrison']>=0 and town['guns']>=0
        lam,phi,phi0 = math.radians(town['lon']-10),math.radians(town['lat']),math.radians(52)
        k=math.sqrt(2/(1+math.sin(phi0)*math.sin(phi)+math.cos(phi0)*math.cos(phi)*math.cos(lam)))
        x=6371.0088*k*math.cos(phi)*math.sin(lam)
        y=6371.0088*k*(math.cos(phi0)*math.sin(phi)-math.sin(phi0)*math.cos(phi)*math.cos(lam))
        ix=int((x-extent['xMin'])/(extent['xMax']-extent['xMin'])*grid['width'])
        iy=int((extent['yMax']-y)/(extent['yMax']-extent['yMin'])*grid['height'])
        assert raw[iy*grid['width']+ix]==grid['ids'][town['owner']], town['name']
    for link in cities['links']:
        assert link['a'] in all_names and link['b'] in all_names
        assert len(link['km'])>=2
        assert all(math.isfinite(x) and math.isfinite(y) and extent['xMin']<=x<=extent['xMax'] and extent['yMin']<=y<=extent['yMax'] for x,y in link['km'])
        length = sum(math.dist(a,b) for a,b in zip(link['km'],link['km'][1:]))
        assert abs(length-link['roadKm']-link['ferryKm'])<0.002
        if link['ferryKm']>0: assert link['ferry'] and link['roadKm']==0
    ids = [a['id'] for a in armies['armies']]
    assert len(ids)==len(set(ids))
    for army in armies['armies']:
        assert army['nation'] in nation_ids and army['town'] in names
        assert 0<=army['men']<=army['mobilisedMen']
    assert sum(a['garrison'] for a in armies['armies'])==len(rows)
    if year==1825:
        assert not cities['railways'] and not any(c['railway'] for c in rows)
        assert not any('rail' in link for link in cities['links'])
    print(f'CAMPAIGN-1851|neighbours|valideret|{year}|{len(rows)} byer|{len(cities["links"])} ruter|{len(ids)} hære')
for script in ('export_neighbours.py','validate_neighbours.py'):
    ast.parse((ROOT/'Tools/Map1851'/script).read_text(encoding='utf-8'))
