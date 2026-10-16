"""Aktiverer de preussiske events i scenariernes eventfiler (Events_1825.json, Events.json). Ingen build eller spilstart.

Kilde: Data/Campaign1851/Countries/Prussia/GameReady/Events.json (arkiveret, uaendret). Koeres EFTER import_prussia.py. Idempotent.
Regler (Docs/Import-Preussen.md, afsnittet "Begivenheder"; Docs/Events.md):
- Historiens udgangspunkt er ikke et manuskript: hver aktiv hændelse har vindue og sandsynlighed (spil) og forudsaetninger,
  som spillerens valg kan goere falske. Vinduets slutning (tilAar/tilMaaned) og `historik` er valgfri felter i loaderen.
- Hændelser, hvis vindue ligger foer scenariets start (1. juli), er historik: de afspilles ikke, men markeres `indtraf`.
- `london`, `execution`, `ultimatum` findes allerede som danske events med identiske betingelser, vindue, sandsynlighed,
  effekter og udeblivelse. De flettes: kun status/kilde (source, historyStatus, designStatus, note, fieldStatus) tilfoejes
  den danske post; dansk adfaerd og tekst er uaendret.
- Spaendingsfoelger er moderate (|x| <= 3, spil) og markeret `spil`.
"""
import argparse
import copy
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EVENT_FILE = {1825: 'Events_1825.json', 1851: 'Events.json'}
SCENARIO_START = {1825: (1825, 7), 1851: (1851, 7)}   # aar, maaned (1. i maaneden)
MERGED = ('london', 'execution', 'ultimatum')
HISTORY_1851 = ('pr_zollverein', 'pr_1848', 'pr_malmo', 'pr_berlinpeace', 'pr_olmuetz', 'pr_1851')
DESIGN = 'DESIGN'
FACTS = ('spaending', 'stemning', 'mobiliseret', 'maegling', 'kasse', 'krig')
FACT_PREFIXES = ('forhold.', 'garant.', 'opinion.', 'forskning.', 'besat.', 'vaerk.', 'begivenhed.', 'udeblevet.')
EFFECT_CAPS = {'spaending': 25, 'stemning': 10, 'kasse': 50000}


def fs(reason):
    return {'status': 'spil', 'source': DESIGN, 'reason': reason}


# Nye, spilbare PR-events. aar/maaned er fra arkivet; vinduets slutning, sandsynlighed, betingelser og foelger er spilvalg.
WHY_WINDOW = 'Vinduets slutning er et spilvalg: hændelsen forsøges hver måned i vinduet, og sandsynligheden gælder hele vinduet.'
WHY_EFFECT = 'Moderat spændingsfølge (højst 3 point) er et spilvalg, ikke en historisk størrelse.'
WHY_ABSENCE = 'Hvad der sker, når hændelsen udebliver, er et spilvalg: lille modsat spændingsændring.'
WHY_COND = 'Betingelserne er spilvalg: spillerens mobilisering, krigstilstand og relation til Preussen kan gøre dem falske.'
SPECS = {
    'pr_zollverein': dict(  # kun 1825: i 1851 er den historik
        scenarios=(1825,), window=(1834, 1, 1835, 6), p=0.8, pre='ikke mobiliseret og ikke krig', block='',
        eff={'spaending': 1}, ude={'spaending': -1}),
    'pr_hannover_zoll': dict(
        scenarios=(1825, 1851), window=(1851, 9, 1852, 12), p=0.75, pre='ikke mobiliseret og ikke krig', block='',
        eff={'spaending': -1}, ude={'spaending': 1}),
    'pr_roon1859': dict(
        scenarios=(1825, 1851), window=(1859, 12, 1860, 6), p=0.7, pre='forhold.PR < 50 og ikke krig', block='',
        eff={'spaending': 1}, ude={'spaending': -1}),
    'pr_roon1860': dict(
        scenarios=(1825, 1851), window=(1860, 1, 1862, 6), p=0.7, pre='forhold.PR < 50 og begivenhed.pr_roon1859 og ikke krig', block='',
        eff={'spaending': 3}, ude={'spaending': -1}),
}


def load(path):
    raw = path.read_bytes()
    return json.loads(raw.decode('utf-8-sig')), (b'\r\n' in raw)


def dump(value, crlf):
    text = json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
    return (text.replace('\n', '\r\n') if crlf else text).encode('utf-8')


def strip_status(value):
    if isinstance(value, dict):
        return {k: strip_status(v) for k, v in value.items() if k != 'fieldStatus'}
    return value


def window_end(event):
    w = event['vindue']
    return w.get('tilAar', w['aar']), w.get('tilMaaned', w['maaned'])


def check_expression(text, ids, label):
    for atom in [a.strip() for a in text.split(' og ') if a.strip()]:
        fact = atom.removeprefix('ikke ').split()[0]
        assert fact in FACTS or fact.startswith(FACT_PREFIXES), (label, 'ukendt faktum', fact)
        if fact.startswith(('begivenhed.', 'udeblevet.')):
            assert fact.split('.', 1)[1] in ids, (label, 'ukendt event-id', fact)


def build_future(archive, event_id, spec):
    base = archive[event_id]
    y0, m0, y1, m1 = spec['window']
    assert (base['vindue']['aar'], base['vindue']['maaned']) == (y0, m0), (event_id, 'vinduesstart afviger fra arkivet')
    effect = dict(spec['eff'])
    effect['fieldStatus'] = {k: fs(WHY_EFFECT) for k in spec['eff']}
    absence = dict(spec['ude'])
    absence['fieldStatus'] = {k: fs(WHY_ABSENCE) for k in spec['ude']}
    window = copy.deepcopy(base['vindue'])
    window['tilAar'], window['tilMaaned'] = y1, m1
    window['fieldStatus']['tilAar'] = fs(WHY_WINDOW)
    window['fieldStatus']['tilMaaned'] = fs(WHY_WINDOW)
    return {
        'id': event_id, 'navn': base['navn'], 'vindue': window, 'sandsynlighed': spec['p'],
        'forudsaetninger': spec['pre'], 'blokeringer': spec['block'], 'effekter': effect, 'udeblivelse': absence,
        'avis': base['avis'], 'log': base['log'], 'source': base['source'], 'historyStatus': base['historyStatus'],
        'designStatus': base['designStatus'],
        'note': 'Dato/emne historisk (kilde ' + base['source'] + '); vindue, sandsynlighed, betingelser og følger er egne spilvalg. '
                'Begivenheden kan udeblive, hvis betingelserne bliver falske (mobilisering, krig, relation til Preussen), og den afgøres ved vinduets udløb.',
        'fieldStatus': {'sandsynlighed': base['fieldStatus']['sandsynlighed'], 'forudsaetninger': fs(WHY_COND),
                        'blokeringer': fs('Ingen blokering er et spilvalg.')},
    }


def build_history(archive, event_id):
    base = copy.deepcopy(archive[event_id])
    base['historik'] = 'indtraf'
    base.pop('udeblivelse', None)
    if event_id == 'pr_zollverein':
        base.pop('effekter', None)    # arkivets eneste effekt var kasse 0
    base['note'] = ('Historik i scenarie 1851 (vinduet ligger før startdatoen 1. juli 1851): afspilles ikke, men indlæses som indtruffet, '
                    'så senere events kan henvise til det (begivenhed.' + event_id + '). Dato/emne historisk (kilde ' + base['source']
                    + '); betingelse og effekt er arkiverede spilvalg og bruges ikke.')
    return base


def merge_metadata(danish, archive_event):
    for key in ('vindue', 'sandsynlighed', 'forudsaetninger', 'blokeringer', 'effekter', 'udeblivelse', 'avis', 'log'):
        assert strip_status(danish[key]) == strip_status(archive_event[key]) or key in ('avis', 'log'), (danish['id'], key, 'PR-varianten afviger: flet manuelt')
    merged = copy.deepcopy(danish)
    for key in ('source', 'historyStatus', 'designStatus', 'note', 'fieldStatus'):
        merged.pop(key, None)
    merged['source'] = archive_event['source']
    merged['historyStatus'] = archive_event['historyStatus']
    merged['designStatus'] = archive_event['designStatus']
    merged['note'] = ('Delt med Preussen-importen (samme id): betingelse, vindue, sandsynlighed, effekter og udeblivelse er identiske med den arkiverede PR-variant, '
                      'så eventet er ikke duplikeret og dansk adfærd er uændret. Arkivnote: ' + archive_event['note'])
    merged['fieldStatus'] = copy.deepcopy(archive_event['fieldStatus'])
    return merged


def activate(root):
    data = root / 'Data/Campaign1851'
    archive_list, _ = load(data / 'Countries/Prussia/GameReady/Events.json')
    archive = {e['id']: e for e in archive_list}
    summary = []
    writes = []
    for year, name in EVENT_FILE.items():
        path = data / name
        events, crlf = load(path)
        danish = [e for e in events if not e['id'].startswith('pr_')]
        out = []
        for event in danish:
            out.append(merge_metadata(event, archive[event['id']]) if event['id'] in MERGED and year == 1851 else event)
        if year == 1851:
            assert {e['id'] for e in danish if e['id'] in MERGED} == set(MERGED)
        added = []
        if year == 1851:
            added += [build_history(archive, i) for i in HISTORY_1851]
        added += [build_future(archive, i, s) for i, s in SPECS.items() if year in s['scenarios']]
        ids = {e['id'] for e in out + added}
        assert len(ids) == len(out) + len(added), 'dublerede event-id'
        start = SCENARIO_START[year]
        for event in added:
            label = f'{name}:{event["id"]}'
            end = window_end(event)
            history = 'historik' in event
            assert history == (end < start), (label, 'før/efter scenariets start passer ikke til historik-feltet')
            for text in (event['forudsaetninger'], event['blokeringer']):
                check_expression(text, ids, label)
            for field in ('effekter', 'udeblivelse'):
                for k, v in event.get(field, {}).items():
                    if k != 'fieldStatus' and not history:   # historik afspilles aldrig; dens effekter er kun dokumentation
                        assert abs(v) <= EFFECT_CAPS[k] and abs(v) <= 3, (label, field, k, 'ikke moderat')
            if not history:
                assert event['udeblivelse'].get('spaending') or event['udeblivelse'].get('kasse'), (label, 'udeblivelse uden foelge')
            summary.append(dict(scenario=str(year), id=event['id'], mode='historik' if history else 'vindue',
                                window=f"{event['vindue']['aar']}-{event['vindue']['maaned']:02d}" + (
                                    f" .. {end[0]}-{end[1]:02d}" if end != (event['vindue']['aar'], event['vindue']['maaned']) else ''),
                                probability=event['sandsynlighed'], history=event.get('historik')))
        for event in out:
            if event['id'] in MERGED and year == 1851:
                summary.append(dict(scenario=str(year), id=event['id'], mode='flettet med dansk event', window=f"{event['vindue']['aar']}-{event['vindue']['maaned']:02d}",
                                    probability=event['sandsynlighed'], history=None))
        writes.append((path, dump(out + added, crlf)))
    for path, raw in writes:   # skriv først, når alle kontroller er bestået
        if not path.exists() or path.read_bytes() != raw:
            path.write_bytes(raw)
    # Rapport: hvilke events er nu aktive
    report_path = root / 'Docs/Import-Preussen-rapport.json'
    if report_path.exists():
        report = json.loads(report_path.read_text(encoding='utf-8'))
        active = {s['id'] for s in summary}
        for row in report.get('events', []):
            row['activated'] = row['id'] in active
        report['eventActivation'] = dict(
            script='Tools/Countries/activate_prussia_events.py',
            note='Events aktiveret efter brugerens beslutning 2026-10-16; se Docs/Import-Preussen.md, afsnittet Begivenheder.',
            events=summary)
        raw = (json.dumps(report, ensure_ascii=False, indent=2, allow_nan=False) + '\n').encode('utf-8')
        if report_path.read_bytes() != raw:
            report_path.write_bytes(raw)
    return summary


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, default=ROOT)
    args = ap.parse_args()
    summary = activate(args.root)
    for row in summary:
        print(row['scenario'], row['id'], row['mode'], row['window'], row['probability'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
