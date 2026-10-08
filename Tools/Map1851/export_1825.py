"""Export reviewed 1825 population overlays; no terrain generation or game execution."""
import argparse
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent


def export(out):
    cities = json.loads((HERE / 'cities_1825.json').read_text(encoding='utf-8'))
    amter = json.loads((HERE / 'amter_1825.json').read_text(encoding='utf-8'))
    names = {c['name']: c['pop'] for c in cities['cities']}
    assert len(names) == len(cities['cities'])
    assert len({a['id'] for a in amter['amter']}) == len(amter['amter'])
    for key in ('cities', 'foreignCities'):
        assert len({c['name'] for c in cities[key]}) == len(cities[key])
        assert all(isinstance(c['pop'], int) and c['pop'] > 0 for c in cities[key])
    for amt in amter['amter']:
        assert amt['urban'] == sum(names[t] for t in amt['towns'])
        assert amt['rural'] >= 0
        assert amt['population'] == amt['urban'] + amt['rural']
    map_file = out / 'Denmark1851_Map.json'
    if map_file.exists():
        base = json.loads(map_file.read_text(encoding='utf-8-sig'))
        for key in ('cities', 'foreignCities'):
            assert [c['name'] for c in cities[key]] == [c['name'] for c in base[key]]
        assert [a['id'] for a in amter['amter']] == [a['id'] for a in base['amter']]
    data = dict(scenario='1825', note=cities['note'], cities=cities['cities'],
                foreignCities=cities['foreignCities'], amter=amter['amter'])
    out.mkdir(parents=True, exist_ok=True)
    (out / 'Population_1825.json').write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=HERE.parents[1] / 'Data/Campaign1851')
    export(parser.parse_args().out)
