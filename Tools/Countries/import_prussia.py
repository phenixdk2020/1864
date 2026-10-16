"""Reproducerbar PR-import (Preussen, niveau B). Ingen build eller spilstart.

Kun PR-rækker aendres; alle andre rækker bevarer deres bytes (patch() fra SE/NO-importen roerer kun aendrede JSON-tokens).
Koeres EFTER export_neighbours.py og import_sweden_norway.py. Idempotent.
"""
import argparse
import copy
import hashlib
import json
import sys
from collections import deque
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_sweden_norway import read, patch, project  # genbrug: JSON-token-patch, LAEA, laesning

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = Path(r'C:\Users\allan\Documents\Codex\2026-10-12\l\outputs\Preussen-agent')
YEARS = [1825, 1851]
NATION = 'PR'
# Spillets egne statusstrenge ('estimated') bevares, hvor de findes; pakkens klassifikation staar i fieldStatus.
KEEP_EXISTING = {'status', 'populationStatus'}
# Spillerrettet tekst bevares; pakkens tekst gemmes ved siden af.
SIDE_TEXT = {'note': 'packageNote', 'period': 'packagePeriod'}
SCENARIO_START = {1825: (1825, 7), 1851: (1851, 7)}
ARCHIVE_NOTE = 'Countries/Prussia'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--package', type=Path, default=PACKAGE)
    ap.add_argument('--root', type=Path, default=ROOT)
    args = ap.parse_args()
    package = args.package / 'Spilimport'
    data = args.root / 'Data/Campaign1851'
    archive = data / 'Countries/Prussia'
    report_path = args.root / 'Docs/Import-Preussen-rapport.json'
    previous = json.loads(report_path.read_text(encoding='utf-8')) if report_path.exists() else {}
    report = {k: [] for k in ['added', 'updated', 'skippedNull', 'conflicts', 'deferred', 'valueConflicts', 'statusKept',
                              'coverage', 'mapping', 'lineEndings', 'archived', 'events', 'reachability']}

    def write(p, v):
        p.parent.mkdir(parents=True, exist_ok=True)
        raw = (json.dumps(v, ensure_ascii=False, indent=2, allow_nan=False) + '\n').encode('utf-8')
        if not p.exists() or p.read_bytes() != raw:
            p.write_bytes(raw)

    def preserve(src, dst):
        read(src) if src.suffix == '.json' else None
        dst.parent.mkdir(parents=True, exist_ok=True)
        if not dst.exists() or dst.read_bytes() != src.read_bytes():
            dst.write_bytes(src.read_bytes())
        report['archived'].append(str(dst.relative_to(data)).replace('\\', '/'))

    def nulls(v, path, bucket):
        if isinstance(v, dict):
            for k, c in v.items():
                if k != 'fieldStatus':
                    nulls(c, path + '/' + k, bucket)
        elif isinstance(v, list):
            for i, c in enumerate(v):
                nulls(c, path + '/' + str(i), bucket)
        elif v is None:
            bucket.append(path)

    def clean(v):
        # null er ukendt og skrives aldrig; begrundelsen bevares i fieldStatus
        if isinstance(v, dict):
            return {k: clean(c) for k, c in v.items() if c is not None}
        if isinstance(v, list):
            return [clean(c) for c in v if c is not None]
        return v

    def inherited_game(fs, key):
        info = fs.get(key) if isinstance(fs, dict) else None
        return isinstance(info, dict) and info.get('status') == 'spil' and str(info.get('source', '')).startswith('GAME-')

    def merge(old, new, loc, top=False):
        """Fletter pakkens ikke-null felter ind. null overskriver aldrig; spil-arvede tal der afviger fra spillet bevares."""
        result = copy.deepcopy(old)
        fs = new.get('fieldStatus') if isinstance(new.get('fieldStatus'), dict) else {}
        for k, v in new.items():
            if k == 'fieldStatus':
                result[k] = merge(result.get(k, {}), v, loc + '/fieldStatus') if isinstance(result.get(k), dict) else clean(v)
                continue
            if v is None:
                continue
            if top and k in SIDE_TEXT and result.get(k) is not None:
                if result[k] != v:
                    result[SIDE_TEXT[k]] = v
                continue
            if k in KEEP_EXISTING and result.get(k) is not None:
                if result[k] != v:
                    report['statusKept'].append(f'{loc}/{k}: spillets "{result[k]}" bevaret; pakke "{v}" (staar i fieldStatus)')
                continue
            cur = result.get(k)
            if isinstance(v, dict) and isinstance(cur, dict):
                result[k] = merge(cur, v, loc + '/' + k)
            elif isinstance(v, list) and isinstance(cur, list) and len(v) == len(cur) and all(isinstance(a, dict) and isinstance(b, dict) for a, b in zip(cur, v)):
                result[k] = [merge(a, b, f'{loc}/{k}/{i}') for i, (a, b) in enumerate(zip(cur, v))]
            elif k in result and cur != v and isinstance(v, (int, float)) and not isinstance(v, bool) and inherited_game(fs, k):
                report['valueConflicts'].append(f'{loc}/{k}: spillet {cur!r} bevaret; pakke {v!r} er markeret spil/GAME- og afviger')
            else:
                result[k] = clean(v)
        return result

    def upsert(rows, row, key, loc, top=False):
        ident = row[key]
        idx = next((i for i, r in enumerate(rows) if r.get(key) == ident), None)
        if idx is None:
            fresh = clean(row)
            rows.append(fresh)
            report['added'].append(f'{loc}:{ident}')
            return False
        for owner_key in ('owner', 'nation'):
            if owner_key in row and rows[idx].get(owner_key) not in (None, row[owner_key]):
                report['conflicts'].append(f'{loc}:{ident}: anden ejer/nation ({rows[idx][owner_key]}); sprunget over')
                return True
        merged = merge(rows[idx], row, f'{loc}:{ident}', top)
        if merged != rows[idx]:
            rows[idx] = merged
            report['updated'].append(f'{loc}:{ident}')
        return False

    # --- verificer og arkivér pakken (byteidentisk) ---------------------------------------------------------------
    manifest = read(package / 'Manifest.json')
    hashes = {Path(f['file']).name: f['sha256'] for f in manifest['files']}
    for name, digest in hashes.items():
        assert sha((package / name).read_bytes()) == digest, f'pakkehash afviger: {name}'
    overlay_names = sorted(hashes)
    for name in overlay_names:
        preserve(package / name, archive / ('GameReady' if name not in ('Data-status.json', 'Kildegrundlag.json', 'Skema-kontrol.json', 'Sources.json') else '') / name)
    for name in ['Manifest.json', 'Verifikation.json', 'README.md']:
        preserve(package / name, archive / name)
    for src in sorted((args.package / 'Research').glob('*.json')) + sorted((args.package / 'Research').glob('*.md')):
        preserve(src, archive / 'Research' / src.name)
    for name in ['Arbejdsstatus.md', 'Daekningsmatrix.md', 'Kilder.md', 'Preussen-landepakke-1825-1851.md']:
        preserve(args.package / name, archive / name)
    # Rene arkivfiler: kun tael null (ukendt), ikke aktiv data
    archive_nulls = {}
    for name in ['Army_1825.json', 'Army1851.json', 'Economy_1825.json', 'Economy_1851.json', 'Officers_1825.json', 'Officers1851.json',
                 'Population_1825.json', 'Population_1851.json', 'Population_1864.json', 'Trade_1851.json', 'Events.json']:
        bucket = []
        nulls(read(package / name), name, bucket)
        archive_nulls[name] = len(bucket)

    # --- aktive filer pr. scenarie --------------------------------------------------------------------------------
    extent = read(data / 'Denmark1851_Map.json')['extentKm']
    base_links = read(data / 'Denmark1851_Map.json')['links']
    base_names = [c['name'] for c in read(data / 'Denmark1851_Map.json')['cities'] + read(data / 'Denmark1851_Map.json')['foreignCities']]
    district_bin = data / 'South_Districts.bin'

    def locate(row):
        if row.get('lon') is None or row.get('lat') is None:
            return None
        x, y = project(row['lon'], row['lat'])
        return x, y, extent['xMin'] <= x < extent['xMax'] and extent['yMin'] < y <= extent['yMax']

    for year in YEARS:
        # cities_neighbours --------------------------------------------------------------------------------------
        path = data / f'cities_neighbours_{year}.json'
        old = read(path)
        new = copy.deepcopy(old)
        overlay = read(package / path.name)
        grid = new['ownerGrid']
        raw = (data / grid['file']).read_bytes()
        d_raster = district_bin.read_bytes() if district_bin.exists() else None
        for row in overlay['cities']:
            if row.get('owner') != NATION:
                continue
            nulls(row, f'{path.name}/cities/{row["name"]}', report['skippedNull'])
            skipped = upsert(new['cities'], row, 'name', path.name + '/cities', top=False)
            loc = locate(row)
            cell_ok = district_ok = None
            if loc and loc[2]:
                ix = int((loc[0] - extent['xMin']) / (extent['xMax'] - extent['xMin']) * grid['width'])
                iy = int((extent['yMax'] - loc[1]) / (extent['yMax'] - extent['yMin']) * grid['height'])
                cell_ok = raw[iy * grid['width'] + ix] == grid['ids'][NATION]
                if d_raster and row.get('district') and row['district'] in new['districtGrid']['ids']:
                    district_ok = int.from_bytes(d_raster[(iy * grid['width'] + ix) * 2:(iy * grid['width'] + ix) * 2 + 2], 'little') == new['districtGrid']['ids'][row['district']]
            report['coverage'].append(dict(scenario=str(year), kind='city', name=row['name'], insideExtent=bool(loc and loc[2]),
                                           ownerGridMatches=cell_ok, districtGridMatches=district_ok, onMap=True,
                                           km=[round(loc[0], 3), round(loc[1], 3)] if loc else None))
        outside = new.setdefault('outside', [])
        for row in overlay['outside']:
            if row.get('owner') != NATION:
                continue
            nulls(row, f'{path.name}/outside/{row["name"]}', report['skippedNull'])
            loc = locate(row)
            cand = copy.deepcopy(row)
            cand['onMap'] = False
            upsert(outside, cand, 'name', path.name + '/outside')
            report['coverage'].append(dict(scenario=str(year), kind='city', name=row['name'], insideExtent=bool(loc and loc[2]),
                                           ownerGridMatches=None, districtGridMatches=None, onMap=False,
                                           km=[round(loc[0], 3), round(loc[1], 3)] if loc else None))
            report['deferred'].append(f'{year}: {row["name"]}: uden for kortudsnittet (onMap=false), ingen vejforbindelse')
        for row in overlay['districts']:
            if row.get('nation') != NATION:
                continue
            nulls(row, f'{path.name}/districts/{row["id"]}', report['skippedNull'])
            upsert(new['districts'], row, 'id', path.name + '/districts')
        pr_cities = {c['name'] for c in overlay['cities'] if c.get('owner') == NATION}
        known = {c['name'] for c in new['cities']} | set(base_names)
        for link in overlay['links']:
            if link['a'] not in pr_cities and link['b'] not in pr_cities:
                continue
            existing = next((i for i, r in enumerate(new['links']) if {r['a'], r['b']} == {link['a'], link['b']}), None)
            if existing is None:
                if link['a'] in known and link['b'] in known and link.get('km'):
                    new['links'].append(clean(link))
                    report['added'].append(f"{year}: rute {link['a']}–{link['b']}")
                else:
                    report['deferred'].append(f"{year}: rute {link['a']}–{link['b']}: endepunkt/geometri mangler")
                continue
            merged = merge(new['links'][existing], link, f"{path.name}/links:{link['a']}-{link['b']}")
            if merged != new['links'][existing]:
                new['links'][existing] = merged
                report['updated'].append(f"{path.name}/links:{link['a']}-{link['b']}")
        for key, ident in (('nations', 'id'), ('territories', 'name')):
            for row in overlay[key]:
                if row.get('id', row.get('owner')) == NATION or row.get('owner') == NATION:
                    upsert(new[key], row, ident, f'{path.name}/{key}')
        for row in overlay.get('railways', []):
            if not any(t in pr_cities for t in row.get('towns', [])):
                continue
            nulls(row, f'{path.name}/railways/{row["name"]}', report['skippedNull'])
            upsert(new['railways'], row, 'name', path.name + '/railways')
        report['deferred'].append(f'{year}: ownerGrid/districtGrid-fieldStatus er faelles for alle nationer og importeres ikke')
        patch(path, old, new)
        # Economy_South er en laesbar spejling af distrikterne (valideres som ens); spejlingen foelger PR-rækkerne.
        mirror = data / f'Economy_South_{year}.json'
        if mirror.exists():
            mirror_old = read(mirror)
            mirror_new = copy.deepcopy(mirror_old)
            mirror_new['districts'] = copy.deepcopy(new['districts'])
            patch(mirror, mirror_old, mirror_new)

        # Nations -------------------------------------------------------------------------------------------------
        path = data / ('Nations_1825.json' if year == 1825 else 'Nations1851.json')
        old = read(path)
        new = copy.deepcopy(old)
        for row in read(package / path.name)['nations']:
            assert row['id'] == NATION
            nulls(row, f'{path.name}/nations/{row["id"]}', report['skippedNull'])
            upsert(new['nations'], row, 'id', path.name, top=True)
        patch(path, old, new)

        # Army_Neighbours -----------------------------------------------------------------------------------------
        path = data / f'Army_Neighbours_{year}.json'
        old = read(path)
        new = copy.deepcopy(old)
        towns = {c['name'] for c in read(data / f'cities_neighbours_{year}.json')['cities']}
        for row in read(package / path.name)['armies']:
            assert row['nation'] == NATION
            if row['town'] not in towns:
                report['deferred'].append(f"{year}: {row['id']}: samlingsby uden aktivt kort")
                continue
            nulls(row, f'{path.name}/armies/{row["id"]}', report['skippedNull'])
            upsert(new['armies'], row, 'id', path.name)
        patch(path, old, new)

    # --- events: analyseres her; aktiveres af Tools/Countries/activate_prussia_events.py (efter brugerens beslutning 2026-10-16) -----------------------
    events = read(package / 'Events.json')
    event_files = {y: data / ('Events_1825.json' if y == 1825 else 'Events.json') for y in YEARS}
    existing_ids = {y: ({e['id'] for e in read(event_files[y])} if event_files[y].exists() else set()) for y in YEARS}
    supported = ('spaending', 'stemning', 'mobiliseret', 'maegling', 'kasse')
    for event in events:
        reasons = []
        blocked = event.get('udeblivelse', {})
        if not any(blocked.get(k) for k in ('spaending', 'kasse', 'stemning')):
            reasons.append('udeblivelse uden ikke-nul spaending/kasse/stemning: afvises af ResetWar')
        for expr in (event.get('forudsaetninger', ''), event.get('blokeringer', '')):
            for atom in [a.strip() for a in expr.split(' og ') if a.strip()]:
                fact = atom.removeprefix('ikke ').split()[0]
                if not (fact in supported or fact.startswith(('forhold.', 'garant.', 'opinion.', 'forskning.', 'besat.', 'vaerk.'))):
                    reasons.append(f'ukendt faktum: {fact}')
        window = (event['vindue']['aar'], event['vindue']['maaned'])
        report['events'].append(dict(id=event['id'], window=f'{window[0]}-{window[1]:02d}',
                                     replacesDanishEventIn=[str(y) for y in YEARS if event['id'] in existing_ids[y]],
                                     beforeScenarioStart={str(y): window < SCENARIO_START[y] for y in YEARS},
                                     loaderRejects=reasons, activated=False))
        if event['id'] in existing_ids[1851] or event['id'] in existing_ids[1825]:
            report['conflicts'].append(f"Events: {event['id']} findes i dansk eventfil; flettes af activate_prussia_events.py (kun status/kilde tilfoejes den danske post)")
    report['deferred'].append(f'Events.json: {len(events)} hændelser arkiveret; aktiveres af activate_prussia_events.py (se eventActivation)')

    # --- rute-naetvaerk ---------------------------------------------------------------------------------------------
    for year in YEARS:
        cities = read(data / f'cities_neighbours_{year}.json')
        pr = [c['name'] for c in cities['cities'] if c['owner'] == NATION]
        adj = {True: {}, False: {}}  # True: faerger tilladt
        for link in base_links + cities['links']:
            for ferries in (True, False):
                if not ferries and link.get('ferryKm', 0) > 0:
                    continue
                adj[ferries].setdefault(link['a'], set()).add(link['b'])
                adj[ferries].setdefault(link['b'], set()).add(link['a'])

        def bfs(start, ferries):
            seen, queue = {start}, deque([start])
            while queue:
                n = queue.popleft()
                for m in adj[ferries].get(n, ()):
                    if m not in seen:
                        seen.add(m)
                        queue.append(m)
            return seen
        from_dk, land = bfs('København', True), bfs('Ratzeburg', False)
        outside_pr = [c['name'] for c in cities.get('outside', []) if c.get('owner') == NATION]
        report['reachability'].append(dict(
            scenario=str(year), prussianCities=len(pr),
            reachableFromKobenhavnWithFerries=[n for n in pr if n in from_dk],
            unreachableWithFerries=[n for n in pr if n not in from_dk],
            reachableOverLandFromRatzeburg=[n for n in pr if n in land],
            needFerry=[n for n in pr if n in from_dk and n not in land],
            offMapNoRoute=outside_pr))

    report['mapping'] = [
        dict(overlay='Nations_1825.json / Nations1851.json', schema='samme', loader='Campaign1851Nations.cpp:LoadNations', target='Nations_1825.json / Nations1851.json', action='PR-raekke flettet'),
        dict(overlay='cities_neighbours_<aar>.json', schema='samme', loader='Campaign1851Neighbours.cpp:LoadNeighbours', target='samme', action='PR-byer, -distrikter, -ruter, -territorium og -nation flettet; Koeslin i outside'),
        dict(overlay='Army_Neighbours_<aar>.json', schema='samme', loader='Campaign1851Neighbours.cpp:LoadNeighbours (haerdelen)', target='samme', action='PR-garnisoner og felthaer flettet'),
        dict(overlay='Army_1825.json / Army1851.json', schema='Army_1825.json / Army1851.json', loader='Campaign1851Army.cpp:LoadArmy (kun dansk pulje)', target=ARCHIVE_NOTE + '/GameReady/', action='arkiveret'),
        dict(overlay='Officers_1825.json / Officers1851.json', schema='Officers_1825.json / Officers1851.json', loader='Campaign1851Officers.cpp:LoadOfficers (kun dansk pulje)', target=ARCHIVE_NOTE + '/GameReady/', action='arkiveret'),
        dict(overlay='Economy_1825.json / Economy_1851.json', schema='Economy_1825.json', loader='Campaign1851Economy.cpp:EconomyValue (dansk; 1851 hardcoded)', target=ARCHIVE_NOTE + '/GameReady/', action='arkiveret (alle satser null)'),
        dict(overlay='Events.json', schema='Events.json / Events_1825.json', loader='Campaign1851War.cpp:ResetWar', target=ARCHIVE_NOTE + '/GameReady/', action='arkiveret; aktiveret af activate_prussia_events.py'),
        dict(overlay='Population_1825/1851/1864.json', schema='Population_1825.json', loader='Campaign1851Map.cpp:LoadData (dansk amtsoverlay)', target=ARCHIVE_NOTE + '/GameReady/', action='arkiveret'),
        dict(overlay='Trade_1851.json', schema='Trade_1851.json', loader='ingen', target=ARCHIVE_NOTE + '/GameReady/', action='arkiveret (alle totaler null)'),
        dict(overlay='Manifest, Data-status, Sources, Kildegrundlag, Skema-kontrol, Verifikation, README, Research', schema='dokumentation', loader='ingen', target=ARCHIVE_NOTE + '/', action='arkiveret'),
    ]
    for p in sorted(archive.rglob('*.json')):
        read(p)
    for p in data.glob('*.json'):
        if p.name.startswith(('cities_neighbours_', 'Army_Neighbours_', 'Nations', 'Economy_South_')):
            b = p.read_bytes()
            report['lineEndings'].append(dict(file=p.name, CRLF=b.count(b'\r\n'), LF=b.count(b'\n'), bareCR=b.count(b'\r') - b.count(b'\r\n')))
    report['archiveNulls'] = archive_nulls
    report['counts'] = {k: len(report[k]) for k in ['added', 'updated', 'skippedNull', 'conflicts', 'deferred', 'valueConflicts', 'statusKept']}
    if previous and not report['added'] and not report['updated']:
        print(json.dumps(report['counts']), '- ingen aendringer; rapporten fra foerste import bevaret')
        return
    write(report_path, report)
    print(json.dumps(report['counts']))


if __name__ == '__main__':
    main()
