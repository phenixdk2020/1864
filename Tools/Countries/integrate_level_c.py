"""Aktiver scenariepersoner og økonomiske spilskøn; arkiverne ændres ikke."""
import argparse
import json
from pathlib import Path


def career_reserves(nation, year):
    """Fiktiv reservepulje; alle aldre, evner og erfaringstal er spilskøn."""
    result = []
    for index, rank in enumerate(('Kaptajn', 'Major', 'Oberstløjtnant', 'Oberst', 'Generalmajor')):
        officer = dict(id=f'F_CAREER_{nation}_{year}_{index}',
                       name=f'Fiktiv reserveofficer {nation} {index + 1}', rank=rank,
                       born=year - 35 - index * 3, historical=False, experience=35 + index * 8,
                       note='Fiktiv person; alder, rang og evner er spilskøn.')
        officer.update({stat: 5 for stat in ('leadership', 'inspiration', 'initiative', 'tactical',
                                           'staff', 'discipline', 'aggression', 'composure', 'political', 'caution')})
        result.append(officer)
    return result


def integrate(root):
    data = Path(root) / 'Data/Campaign1851'
    def read(path):
        return json.loads(path.read_text(encoding='utf-8-sig'))
    def write(name, value):
        text = json.dumps(value, ensure_ascii=False, indent=2) + '\n'
        (data / name).write_bytes(text.replace('\n', '\r\n').encode('utf-8'))
    for year in (1825, 1851):
        nations = []
        for nation, folder in [('SE', 'SwedenNorway'), ('NO', 'SwedenNorway'), ('PR', 'Prussia')]:
            pack = data / 'Countries' / folder / 'GameReady'
            source = pack / ('Officers_1825.json' if year == 1825 else 'Officers1851.json')
            officers = [o for o in read(source)['generals']
                        if o.get('nation', 'NO' if o['id'].startswith('G_NO_') else 'SE') == nation]
            if nation == 'PR':
                name = 'Christian Günther von Bernstorff' if year == 1825 else 'Otto von Manteuffel'
                url = ('https://www.deutsche-biographie.de/gnd116147695.html' if year == 1825
                       else 'https://www.deutsche-biographie.de/sfz57925.html')
                ministers = [{'portfolio': 'Foreign', 'candidates': [{
                    'name': name, 'skill': 50, 'thrift': 50, 'caution': 50,
                    'role': 'Udenrigsminister', 'source': url}]}]
                if year == 1851:
                    ministers.append({'portfolio': 'War', 'candidates': [{
                        'name': 'August von Stockhausen', 'skill': 50, 'thrift': 50, 'caution': 50,
                        'role': 'Krigsminister; ikke feltchef',
                        'source': 'Countries/Prussia/GameReady/Officers1851.json'}]})
            else:
                ministers = read(pack / nation / f'Ministers_{year}.json')['portfolios']
            officers = [dict(officer, historical=True) for officer in officers] + career_reserves(nation, year)
            nations.append({'nation': nation, 'commander': officers[0]['id'],
                            'officerSource': source.relative_to(data).as_posix(),
                            'officers': officers, 'portfolios': ministers})
        write(f'ForeignPersonnel_{year}.json', {
            'scenario': str(year),
            'note': 'Historiske personer fra arkiverede landepakker. Egenskaber, erfaring og feltkommando er spilskøn, '
                    'ikke verificeret historisk rangering. Officerstats 1-10, ministerstats 0-100. '
                    'Ressortfordeling SE/NO er pakkens spilvalg; delte personer har flere ressorter. '
                    'PR: kun dokumenterede ressorter, resten ukendt. Ingen tvungen historisk udskiftning.',
            'nations': nations})
        danish = read(data / 'Economy_1825.json') if year == 1825 else {
            'grainPerHead': .6, 'cattlePerHead': .3, 'butterPerHead': .15, 'exportDuty': .02}
        write(f'ForeignEconomy_{year}.json', {
            'scenario': str(year), 'status': 'estimated',
            'note': 'ALLE produktions-/skattesatser, startkasser og underhold er spilskøn i fælles rd.-regneenhed, '
                    'ikke historiske valutakurser eller nationale budgettal. Kun aktive kortdistrikter indgår. '
                    'Produktion genbruger Danmarks scenarievalgte eksportmodel (høst, priser, eksporttold); '
                    'private producenters omsætning bliver ikke i sin helhed statslig indtægt. '
                    'Startkasse: sum af eksisterende distriktskasser (tre års grundskat). '
                    'Underhold pr. mand: dansk RaisedUpkeepPerMonth / våbenartens standardenhed; blandede korps regnes som infanteri (skøn). '
                    'Ubetalt andel giver op til 0,1 moraltab pr. måned; ingen gratis kredit. '
                    'Ingen danske reform-/forskningsbonusser overføres. Besat distrikt: nul indtægt; '
                    'sædebesættelse er forenklet skøn for hele distriktet. '
                    'Danmarks søkontrol reducerer fjendens eksport; unionen følger alliancebeslutningen.',
            'nations': [dict(nation=n, **{k: danish[k] for k in
                         ('grainPerHead', 'cattlePerHead', 'butterPerHead', 'exportDuty')})
                        for n in ('SE', 'NO', 'PR')]})


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    integrate(parser.parse_args().root)
