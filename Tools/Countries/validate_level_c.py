"""Niveau C: kildeidentitet, scenarier, kontoadskillelse og statisk save-/motorkobling."""
import json
import math
import re
import shutil
import tempfile
from pathlib import Path
from integrate_level_c import integrate

ROOT = Path(__file__).resolve().parents[2]
PORTFOLIOS = ('Interior', 'PublicWorks', 'War', 'Transport', 'Intendance', 'Foreign', 'Navy', 'Finance')


def validate(root=ROOT):
    root = Path(root)
    data = root / 'Data/Campaign1851'
    def read(path):
        return json.loads(path.read_text(encoding='utf-8-sig'),
                          parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))
    parsed = sum(1 for path in data.rglob('*.json') if read(path) is not None)
    for year in (1825, 1851):
        personnel = read(data / f'ForeignPersonnel_{year}.json')
        economy = read(data / f'ForeignEconomy_{year}.json')
        assert personnel['scenario'] == economy['scenario'] == str(year)
        assert economy['status'] == 'estimated' and 'spilskøn' in economy['note']
        nations = {n['nation']: n for n in personnel['nations']}
        assert set(nations) == {n['nation'] for n in economy['nations']} == {'SE', 'NO', 'PR'}
        armies = read(data / f'Army_Neighbours_{year}.json')['armies']
        districts = read(data / f'cities_neighbours_{year}.json')['districts']
        national_data = read(data / ('Nations_1825.json' if year == 1825 else 'Nations1851.json'))['nations']
        assert next(n for n in national_data if n['id'] == 'PR')['relation'] == -20
        all_ids = set()
        for nation, entry in nations.items():
            officers = entry['officers']
            ids = {o['id'] for o in officers}
            assert len(ids) == len(officers) and not all_ids.intersection(ids)
            all_ids.update(ids)
            assert entry['commander'] in ids
            archived = read(data / entry['officerSource'])['generals']
            expected = [o for o in archived if o.get('nation', 'NO' if o['id'].startswith('G_NO_') else 'SE') == nation]
            assert [{k: v for k, v in o.items() if k != 'historical'} for o in officers if o['historical']] == expected, (year, nation, 'arkivpersoner ændret')
            for officer in officers:
                for stat in ('leadership', 'inspiration', 'initiative', 'tactical', 'staff', 'discipline', 'aggression', 'composure', 'political'):
                    assert 1 <= officer[stat] <= 10
            portfolios = {p['portfolio']: p for p in entry['portfolios']}
            assert len(portfolios) == len(entry['portfolios']) and set(portfolios) <= set(PORTFOLIOS)
            assert 'Foreign' in portfolios
            people = {}
            for portfolio in portfolios.values():
                for minister in portfolio['candidates']:
                    assert minister['name'] and minister['role']
                    stats = tuple(minister[k] for k in ('skill', 'thrift', 'caution'))
                    assert all(0 <= v <= 100 for v in stats)
                    assert minister['name'] not in people or people[minister['name']] == stats
                    people[minister['name']] = stats
            if nation in ('SE', 'NO'):
                source = data / f'Countries/SwedenNorway/GameReady/{nation}/Ministers_{year}.json'
                assert entry['portfolios'] == read(source)['portfolios']
            else:
                foreign = portfolios['Foreign']['candidates'][0]
                assert foreign['name'] == ('Christian Günther von Bernstorff' if year == 1825 else 'Otto von Manteuffel')
                assert foreign['source'].startswith('https://www.deutsche-biographie.de/')
            local = [d for d in districts if d['nation'] == nation]
            assert local and all(0 <= d['urban'] <= d['population'] for d in local)
            assert all(d['urbanTax'] >= 0 and d['ruralTax'] >= 0 for d in local)
            for army in armies:
                if army['nation'] != nation:
                    continue
                assert not army.get('officer') or army['officer']['id'] in ids
                if army['mobilisedMen'] > 0:
                    assert army['district'] in {d['id'] for d in local}
            parameters = next(n for n in economy['nations'] if n['nation'] == nation)
            assert all(math.isfinite(parameters[k]) and parameters[k] >= 0 for k in
                       ('grainPerHead', 'cattlePerHead', 'butterPerHead', 'exportDuty'))
        print(f'OK niveau C {year}: {len(all_ids)} officerer; SE/NO ressorter og PR udenrigsminister; 3 separate økonomier; relation -20.')
    # Regeneration must reproduce all four active files, without touching archive hashes.
    with tempfile.TemporaryDirectory(prefix='1864-level-c-') as folder:
        temporary = Path(folder)
        shutil.copytree(data / 'Countries', temporary / 'Data/Campaign1851/Countries')
        shutil.copy2(data / 'Economy_1825.json', temporary / 'Data/Campaign1851/Economy_1825.json')
        for _ in range(2):
            integrate(temporary)
            for prefix in ('ForeignPersonnel', 'ForeignEconomy'):
                for year in (1825, 1851):
                    name = f'{prefix}_{year}.json'
                    assert (data / name).read_bytes() == (temporary / 'Data/Campaign1851' / name).read_bytes(), name
    # Static wiring checks, not a substitute for compilation/runtime save tests.
    private = root / 'Source/Game1864/Private'
    source = (private / 'Campaign1851Neighbours.cpp').read_text(encoding='utf-8-sig')
    assert 'ForeignDraftCash -= SouthDraft * Campaign1851Army::ReplacementCostPerMan' in source
    assert 'ForeignDepotCash = FMath::Max' in source
    assert 'ForeignTreasuries.FindOrAdd(SouthDistrict.Nation) += ForeignRevenue / 12.0' in source
    assert 'NeighbourStage >= NeighbourArmy.MobilisationStage' in source
    assert 'ForeignCommander(SouthArmy->Nation, SouthArmy->OfficerId)' in source
    assert 'Campaign1851Army::StaffPaceFactor(ForeignChief->Stat' in source
    assert 'ResetForeignEconomies();' in source
    assert 'AddTransaction(' not in source, 'naboøkonomi må ikke debitere Danmark'
    save = (root / 'Source/Game1864/Public/Campaign1851SaveGame.h').read_text(encoding='utf-8-sig')
    assert 'CurrentVersion = 40' in save and 'UPROPERTY() TMap<FString, double> ForeignTreasuries;' in save
    controller = (private / 'Campaign1851PlayerController.cpp').read_text(encoding='utf-8-sig')
    assert 'Save->ForeignTreasuries = Map->GetForeignTreasuries();' in controller
    assert 'Save->SaveVersion >= 39' in controller and 'RestoreForeignTreasuries(Save->ForeignTreasuries)' in controller
    intel = (private / 'Campaign1851Intel.cpp').read_text(encoding='utf-8-sig')
    assert 'ForeignTreasuries.Find(C.Nation) && (!SouthSource || SouthSource->District.IsEmpty())' in intel
    assert '? C.StartMen : C.StartMen * 3 / 2;' in intel
    assert 'ForeignEconomies.Find(Army.Nation) ? 0 : Wanted' in source
    assert 'for (const FCampaign1851EnemyCorps& ForeignCorps : EnemyCorps)' in source
    # Requirement-level budget cases: zero funds, partial funds, retry, independent country accounts.
    cost_match = re.search(r'ReplacementCostPerMan\s*=\s*([\d.]+)',
                           (root / 'Source/Game1864/Public/Campaign1851Army.h').read_text(encoding='utf-8-sig'))
    cost = float(cost_match.group(1))
    treasury = {'DK': 150000., 'PR': 2.5 * cost, 'SE': 0., 'NO': 20. * cost}
    before = dict(treasury)
    def draft(nation, wanted, reserve):
        count = min(wanted, reserve, math.floor(treasury[nation] / cost))
        treasury[nation] -= count * cost
        return count
    assert draft('SE', 10, 10) == 0
    assert draft('PR', 10, 10) == 2
    treasury['PR'] += 3 * cost
    assert draft('PR', 8, 8) == 3
    assert treasury['DK'] == before['DK'] and treasury['NO'] == before['NO']
    assert json.loads(json.dumps(treasury)) == treasury
    print(f'OK niveau C: {parsed} JSON; genimport x2 byteidentisk; statisk kommando/betaling/save v40; budgetreference: nul/delvis/genforsøg og DK-adskillelse.')


if __name__ == '__main__':
    validate()
