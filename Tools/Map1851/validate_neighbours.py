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
    source = json.loads((ROOT/'Tools/Map1851/neighbours.json').read_text(encoding='utf-8'))
    # Stable legacy prefix; Swedish values now checked against the imported package.
    sweden_package = DATA / 'Countries/SwedenNorway/GameReady'
    sweden_overlay = json.loads((sweden_package/f'cities_neighbours_{year}.json').read_text(encoding='utf-8-sig'))
    sweden_rows = {c['name']: c for c in sweden_overlay['cities']}
    assert [c['name'] for c in rows[:len(source['cities'])]] == [c['name'] for c in source['cities']]
    for original in source['cities']:
        if original['owner'] != 'SE': continue
        actual = next(c for c in rows if c['name']==original['name'])
        expected = {**original, **{k:v for k,v in sweden_rows[original['name']].items() if v is not None and k != 'district'}}
        def assert_overlay(actual_value, expected_value):
            for overlay_key, overlay_value in expected_value.items():
                if overlay_value is None: continue
                if isinstance(overlay_value, dict): assert_overlay(actual_value[overlay_key], overlay_value)
                else: assert actual_value[overlay_key]==overlay_value, (original['name'],overlay_key)
        assert_overlay(actual, expected)
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
    for original in source['armies']:
        if original['nation']!='SE': continue
        actual = next(a for a in armies['armies'] if a['id']==original['id'])
        sweden_armies = json.loads((sweden_package/f'SE/Army_Neighbours_{year}.json').read_text(encoding='utf-8-sig'))['armies']
        package_id = 'SE-field' if original['id']=='SE-south' else original['id']
        expected = next(a for a in sweden_armies if a['id']==package_id)
        assert actual['men']==expected['men'] and actual['mobilisedMen']==expected['mobilisedMen']
    southern = json.loads((ROOT/'Tools/Map1851/south.json').read_text(encoding='utf-8'))
    assert 40 <= len(southern['cities']) <= 60
    districts = cities['districts']
    economy = read(f'Economy_South_{year}.json')
    assert economy['scenario']==str(year) and economy['districts']==districts
    dids = [d['id'] for d in districts]
    assert len(dids)==len(set(dids))
    district_grid = cities['districtGrid']
    import numpy as np
    raster = np.frombuffer((DATA/district_grid['file']).read_bytes(),dtype='<u2')
    assert len(raster)==len(raw) and set(raster)<={0,*district_grid['ids'].values()}
    for district in districts:
        assert district['nation'] in nation_ids and district['seat'] in names
        assert district['status']=='estimated' and district['boundaryStatus'].startswith('estimated')
        assert district['urban']==sum(c['pop'] for c in rows if c.get('district')==district['id'])
        assert district['population']>=district['urban']>0
        assert district['rural']==district['population']-district['urban']
        assert district['reserve']==round(district['population']*district['reserveShare'])
        assert district['yearlyClass']==round(district['population']*district['yearlyClassShare'])
        assert 0<=district['reserveShare']<=district['capShare'] and 0<district['yearlyClassShare']<district['capShare']
        assert district['taxAnnual']==round(district['urban']*district['urbanTax']+district['rural']*district['ruralTax'])
        selected = raster==district_grid['ids'][district['id']]
        assert selected.any(), district['id']
        assert all(owner==grid['ids'][district['nation']] for owner in np.frombuffer(raw,dtype='u1')[selected])
    assert not any(raster[i] for i,owner in enumerate(raw) if owner in (0,grid['ids']['DK'],grid['ids']['SE']))
    for town in rows:
        if not town.get('district'): continue
        assert town['district'] in dids
        assert town['owner']==next(d['nation'] for d in districts if d['id']==town['district'])
        # District centre must agree with the card, including coastal raster corrections.
        from export_neighbours import project
        x,y=project(town['lon'],town['lat'])
        ix=int((x-extent['xMin'])/(extent['xMax']-extent['xMin'])*grid['width'])
        iy=int((extent['yMax']-y)/(extent['yMax']-extent['yMin'])*grid['height'])
        assert raster[iy*grid['width']+ix]==district_grid['ids'][town['district']], town['name']
    for army in armies['armies']:
        if not army.get('corps'): continue
        assert army['district'] in dids and army['status']=='estimated'
        assert army['nation']==next(d['nation'] for d in districts if d['id']==army['district'])
        subunits = army[{'infantry':'companies','cavalry':'squadrons','artillery':'batteries'}[army['arm']]]
        assert sum(u['men'] for u in subunits)==army['mobilisedMen']
        assert sum(u.get('guns',0) for u in subunits)==army['guns']
        assert army['chief'] or army['chiefStatus']=='unknown'
    for army in armies['armies']:
        if army['garrison'] and army.get('companies'):
            assert sum(u['men'] for u in army['companies']+army['batteries'])==army['men']
            assert sum(u['guns'] for u in army['batteries'])==army['guns']
    for parent in ('PR-pomerania','MEC-field'):
        anchor=next(a for a in armies['armies'] if a['id']==parent)
        assert anchor['men']==anchor['mobilisedMen']==0 and anchor['legacyAggregate']
        expected = (18000 if year==1825 else 24000) if parent=='PR-pomerania' else 3000
        assert sum(a['mobilisedMen'] for a in armies['armies'] if a.get('corps')==parent)==expected
        units=[a for a in armies['armies'] if a.get('corps')==parent]
        local=[d for d in districts if d['nation']==units[0]['nation']]
        needed=sum(a['mobilisedMen']-a['men'] for a in units)
        assert needed<=sum(d['reserve'] for d in local)
        assert needed*10<=sum(d['treasuryStart'] for d in local)
    # All southern towns must have a path into the road network at the Danish border.
    neighbours={n:set() for n in all_names}
    for link in cities['links']:
        neighbours[link['a']].add(link['b']); neighbours[link['b']].add(link['a'])
    reached, pending=set(),['Ratzeburg']
    while pending:
        n=pending.pop()
        if n in reached: continue
        reached.add(n); pending.extend(neighbours[n]-reached)
    assert all(c['name'] in reached for c in southern['cities'])
    for railway in cities['railways']:
        assert int(railway['opened'][:4])<=year
        assert all(n in all_names for n in railway['via'])
    assert sum(a['garrison'] for a in armies['armies'])==len(rows)
    if year==1825:
        assert not cities['railways'] and not any(c['railway'] for c in rows)
        assert not any('rail' in link for link in cities['links'])
    print(f'CAMPAIGN-1851|neighbours|valideret|{year}|{len(rows)} byer|{len(cities["links"])} ruter|{len(ids)} hære')
for script in ('export_neighbours.py','validate_neighbours.py','export_south.py','south_seed.py'):
    ast.parse((ROOT/'Tools/Map1851'/script).read_text(encoding='utf-8'))
