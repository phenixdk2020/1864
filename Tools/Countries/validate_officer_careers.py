"""Statisk officerskontrol; starter hverken Unreal eller et build."""
import json
import math
from pathlib import Path
from integrate_level_c import career_reserves

ROOT = Path(__file__).resolve().parents[2]
STATS = ('leadership', 'inspiration', 'initiative', 'tactical', 'staff', 'discipline',
         'aggression', 'composure', 'political', 'caution')
FIELDS = ('Nation', 'bHistorical', 'CareerStatus', 'RankMonths', 'CommandMonths',
          'BattlesServed', 'Wounds', 'CareerExperience')


def validate(root=ROOT):
    root = Path(root)
    data = root / 'Data/Campaign1851'
    def load(name):
        return json.loads((data / name).read_text(encoding='utf-8-sig'))
    for year in (1825, 1851):
        ids = set()
        packs = load(f'ForeignPersonnel_{year}.json')
        assert packs['scenario'] == str(year)
        assert {n['nation'] for n in packs['nations']} == {'SE', 'NO', 'PR'}
        danish = load('Officers_1825.json' if year == 1825 else 'Officers1851.json')
        groups = [('DK', danish['generals'])] + [(n['nation'], n['officers']) for n in packs['nations']]
        for nation, officers in groups:
            for officer in officers:
                assert officer['id'] not in ids, (year, officer['id'])
                ids.add(officer['id'])
                assert officer['name'] and officer['rank']
                assert isinstance(officer['historical'], bool)
                born = officer.get('born')
                assert born is None or isinstance(born, int) and 1700 <= born <= year - 16
                assert math.isfinite(officer['experience']) and 0 <= officer['experience'] <= 100
                limit = 100 if nation == 'DK' and year == 1825 else 10
                assert all(0 <= officer.get(stat, 5) <= limit for stat in STATS)
                if not officer['historical']:
                    label = officer.get('role', '') + officer.get('note', '') + officer['name']
                    assert 'fiktiv' in label.lower(), officer['id']
            if nation != 'DK':
                assert [o for o in officers if not o['historical']] == career_reserves(nation, year)
                archive = load(next(n['officerSource'] for n in packs['nations'] if n['nation'] == nation))['generals']
                for officer in officers:
                    if not officer['historical']:
                        continue
                    original = next(o for o in archive if o['id'] == officer['id'])
                    assert {k: v for k, v in officer.items() if k != 'historical'} == original
        print(f'OK officerskarrierer {year}: DK/SE/NO/PR, {len(ids)} unikke identiteter; historik og fiktive reserver.')
    public = root / 'Source/Game1864/Public'
    private = root / 'Source/Game1864/Private'
    save = (public / 'Campaign1851SaveGame.h').read_text(encoding='utf-8-sig')
    source = (private / 'Campaign1851Officers.cpp').read_text(encoding='utf-8-sig')
    assert 'CurrentVersion = 40' in save
    for field in FIELDS:
        assert f'S.{field} = O.{field};' in source
        assert f'O.{field} = S.{field};' in source
        assert f' {field}' in save
    assert 'O.Nation.IsEmpty()' in source and 'bCareerForeignSaved' in source
    assert 'MonthlyOfficerCareers();' in (private / 'Campaign1851Map.cpp').read_text(encoding='utf-8-sig')
    assert 'BattleOfficerCareer(ReportOfficer' in (private / 'Campaign1851Battles.cpp').read_text(encoding='utf-8-sig')
    assert 'career-officer|' in (private / 'Campaign1851Neighbours.cpp').read_text(encoding='utf-8-sig')
    assert 'LogCampaignDecision(CareerOfficer.Nation' in (private / 'Campaign1851Career.cpp').read_text(encoding='utf-8-sig')
    # Boundary reference: inactive officers unavailable, capped bonus, conditional promotion.
    def bonus(base, earned):
        return min(10, base + min(2, int(earned / 25)))
    assert bonus(5, 24.99) == 5 and bonus(5, 25) == 6 and bonus(5, 100) == 7
    assert bonus(9, 100) == 10
    def eligible(months, battles, xp, required, vacancy, active):
        return active and vacancy and xp >= required and (months >= 24 or months >= 12 and battles >= 3)
    assert eligible(24, 0, 40, 40, True, True)
    assert eligible(12, 3, 40, 40, True, True)
    assert not eligible(11, 99, 100, 40, True, True)
    assert not eligible(24, 3, 100, 40, False, True)
    assert not eligible(24, 3, 100, 40, True, False)
    print('OK save v40, ældre defaults, hookpunkter, proveniens og grænsetilfælde (statisk/reference).')


if __name__ == '__main__':
    validate()
