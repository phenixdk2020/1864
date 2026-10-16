"""Statisk kontrol af PR-importen: JSON, arkivhashes, urørte rækker, kortdækning, ruter, idempotens. Ingen build/spilstart."""
import ast
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path
from import_sweden_norway import ROOT, PACKAGE as SE_PACKAGE, read, spans
from import_prussia import PACKAGE as PR_PACKAGE

DATA = ROOT / 'Data/Campaign1851'
FILES = [('Nations_1825.json', 'nations'), ('Nations1851.json', 'nations')] + [
    (f'{prefix}_{year}.json', key) for year in (1825, 1851) for prefix, key in
    [('cities_neighbours', 'cities'), ('cities_neighbours', 'links'), ('cities_neighbours', 'districts'), ('cities_neighbours', 'nations'),
     ('cities_neighbours', 'outside'), ('cities_neighbours', 'territories'), ('cities_neighbours', 'railways'), ('Army_Neighbours', 'armies'),
     ('Economy_South', 'districts')]]


def git_bytes(name):
    return subprocess.check_output(['git', 'cat-file', '--filters', 'HEAD:Data/Campaign1851/' + name], cwd=ROOT)


def digest_tree(base):
    return {str(p.relative_to(base)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(base.rglob('*')) if p.is_file()}


def row_texts(raw, key):
    text = raw.decode('utf-8-sig')
    locations, _ = spans(text)
    count = len(json.loads(text)[key])
    return [text[slice(*locations[(key, i)])] for i in range(count)]


def is_pr(row, pr_cities):
    if 'a' in row:
        return row['a'] in pr_cities or row['b'] in pr_cities
    if 'towns' in row:
        return any(t in pr_cities for t in row['towns'])
    return 'PR' in (row.get('owner'), row.get('nation'), row.get('id'))


def main():
    parsed = 0
    for path in DATA.rglob('*.json'):
        read(path)
        parsed += 1
    report = read(ROOT / 'Docs/Import-Preussen-rapport.json')
    archive = DATA / 'Countries/Prussia'
    manifest = read(archive / 'Manifest.json')
    hashes = {Path(f['file']).name: f['sha256'] for f in manifest['files']}
    for name, digest in hashes.items():
        found = list(archive.rglob(name))
        assert found and hashlib.sha256(found[0].read_bytes()).hexdigest() == digest, name
    # Kortdækning og ruter (fra rapporten, genberegnet ved import)
    for year in ('1825', '1851'):
        cov = [r for r in report['coverage'] if r['scenario'] == year]
        assert not any(r['onMap'] and not (r['insideExtent'] and r['ownerGridMatches'] and r['districtGridMatches'] is not False) for r in cov)
        assert all(not r['onMap'] and not r['insideExtent'] for r in cov if r['name'] == 'Köslin')
        reach = next(r for r in report['reachability'] if r['scenario'] == year)
        assert not reach['unreachableWithFerries'] and reach['offMapNoRoute'] == ['Köslin']
    # Events: aktiveret af activate_prussia_events.py. Danske events er uændrede (kun status/kilde-metadata tilføjet) og står først i
    # samme rækkefølge; resten er pr_-events; CRLF bevaret.
    meta = {'source', 'historyStatus', 'designStatus', 'note', 'fieldStatus'}
    for name in ('Events.json', 'Events_1825.json'):
        head, current = json.loads(git_bytes(name)), json.loads((DATA / name).read_bytes())
        assert [e['id'] for e in current[:len(head)]] == [e['id'] for e in head], (name, 'danske events flyttet')
        for old, new in zip(head, current):
            assert {k: v for k, v in new.items() if k not in meta} == old, (name, old['id'], 'dansk event ændret ud over metadata')
        assert all(e['id'].startswith('pr_') for e in current[len(head):]), name
        raw = (DATA / name).read_bytes()
        assert raw.count(b'\r\n') == raw.count(b'\n') and raw.count(b'\r') == raw.count(b'\r\n'), (name, 'linjeskift')
    assert {e['id'] for e in report['events'] if e['activated']} == {e['id'] for e in report['events']}, 'alle PR-events er aktive'
    assert report['eventActivation']['events'], 'eventActivation mangler i rapporten'
    # Gengiv HEAD + SE/NO + PR i isolation: urørte rækker, reproducerbarhed, idempotens
    with tempfile.TemporaryDirectory(prefix='1864-PR-') as directory:
        tmp = Path(directory)
        target = tmp / 'Data/Campaign1851'
        target.mkdir(parents=True)
        names = ['Denmark1851_Map.json', 'Neighbours_Owners.bin', 'South_Districts.bin', 'Trade_1851.json', 'Events.json', 'Events_1825.json'] + [n for n, _ in FILES if n.count('_') or n.startswith('Nations')]
        for name in sorted(set(names)):
            (target / name).write_bytes(git_bytes(name))
        se = ['python', str(ROOT / 'Tools/Countries/import_sweden_norway.py'), '--root', str(tmp), '--package', str(SE_PACKAGE)]
        pr = ['python', str(ROOT / 'Tools/Countries/import_prussia.py'), '--root', str(tmp), '--package', str(PR_PACKAGE)]
        subprocess.run(se, check=True, capture_output=True)
        before = {name: (target / name).read_bytes() for name, _ in FILES}
        subprocess.run(pr, check=True, capture_output=True)
        activate = ['python', str(ROOT / 'Tools/Countries/activate_prussia_events.py'), '--root', str(tmp)]
        subprocess.run(activate, check=True, capture_output=True)
        first = digest_tree(target)
        for name, digest in first.items():
            assert hashlib.sha256((DATA / name).read_bytes()).hexdigest() == digest, (name, 'aktive filer er ikke PR-importens reproducerbare resultat')
        subprocess.run(pr, check=True, capture_output=True)
        subprocess.run(activate, check=True, capture_output=True)
        assert digest_tree(target) == first, 'PR-importen er ikke idempotent'
        initial = read(tmp / 'Docs/Import-Preussen-rapport.json')
        assert initial['counts'] == report['counts'], 'rapporten matcher ikke første import'
        checked = changed = 0
        for name, key in FILES:
            year = '1825' if '1825' in name else '1851'
            pr_cities = {c['name'] for c in read(target / f'cities_neighbours_{year}.json')['cities'] if c['owner'] == 'PR'}
            old_rows, new_rows = row_texts(before[name], key), row_texts((DATA / name).read_bytes(), key)
            old_json, new_json = json.loads(before[name])[key], json.loads((DATA / name).read_bytes())[key]
            assert len(old_rows) == len(new_rows)
            for i, (a, b) in enumerate(zip(old_rows, new_rows)):
                if is_pr(old_json[i], pr_cities):
                    changed += a != b
                    continue
                assert a == b, (name, key, i, 'urørt række ændret')
                checked += 1
            # Alt uden for rækkelisterne er uændret
            for k, v in json.loads(before[name]).items():
                if k != key and k in ('scenario', 'ownerGrid', 'districtGrid'):
                    assert v == json.loads((DATA / name).read_bytes())[k], (name, k)
            raw = (DATA / name).read_bytes()
            assert raw.count(b'\r\n') == raw.count(b'\n') and raw.count(b'\r') == raw.count(b'\r\n'), (name, 'linjeskift')
    for path in (Path(__file__), ROOT / 'Tools/Countries/import_prussia.py', ROOT / 'Tools/Countries/activate_prussia_events.py', ROOT / 'Tools/Map1851/validate_neighbours.py'):
        ast.parse(path.read_text(encoding='utf-8-sig'))
    print(f'OK: {checked} ikke-PR-rækker byteidentiske med HEAD+SE/NO-gengivelsen ({changed} PR-/PR-rute-rækker ændret); {parsed} JSON; {len(hashes)} pakkehashes; reproducerbar og idempotent.')
    print('Første import:', report['counts'])


if __name__ == '__main__':
    main()
