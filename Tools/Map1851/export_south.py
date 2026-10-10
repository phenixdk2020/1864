"""Scenariekilde for Syd for Danmark. Tal og distriktsgeometri er eksplicitte skøn.

Kaldes af export_neighbours.py; ingen Unreal-start. Kortets udsnit udvides ikke.
"""
import copy
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent


def enrich(source):
    """Bevar alle oprindelige indeks; tilføj kun sydlige byer og ruter."""
    source = copy.deepcopy(source)
    south = json.loads((HERE / 'south.json').read_text(encoding='utf-8'))
    existing = {c['name'] for c in source['cities']}
    for city in south['cities']:
        assert city['name'] not in existing, city['name']
        existing.add(city['name'])
        source['cities'].append(city)
    # Politisk adskilt fra Schwerin; nationens nye indeks tilføjes sidst.
    source['nations'].append(south['nation'])
    source['territories'].extend(south['territories'])
    source['outside'].extend(south['outside'])
    edges = {frozenset(r[:2]) for r in source['roads']}
    for road in south['roads']:
        if frozenset(road[:2]) not in edges:
            source['roads'].append(road)
            edges.add(frozenset(road[:2]))
    # Den gamle Rostock-linje via Wismar var en fejlagtig forenkling.
    source['railways'] = south['railways']
    source['south'] = south
    for city in source['cities']:
        if city['name'] in south['townOverrides']:
            city.update(south['townOverrides'][city['name']])
    return source


def scenario_districts(source, year, cities):
    districts = []
    for district in source['south']['districts']:
        row = copy.deepcopy(district)
        row['population'] = district['population'][str(year)]
        row['urban'] = sum(c['pop'] for c in cities if c.get('district') == row['id'])
        row['rural'] = max(0, row['population'] - row['urban'])
        row['reserve'] = round(row['population'] * row['reserveShare'])
        row['yearlyClass'] = round(row['population'] * row['yearlyClassShare'])
        row['taxAnnual'] = round(row['urban'] * row['urbanTax'] + row['rural'] * row['ruralTax'])
        row['grain'] = round(row['rural'] * row['grainPerHead'])
        row['cattle'] = round(row['rural'] * row['cattlePerHead'])
        row['ritterschaftPopulation'] = round(row['rural'] * row['ritterschaftShare'])
        row['domanialPopulation'] = row['rural'] - row['ritterschaftPopulation'] if row['nation']=='MEC' else 0
        row['treasuryStart'] = 3 * row['taxAnnual']
        row['recruitmentSystem'] = ('Almindelig værnepligt fra 1814, linje / reserve / Landwehr; det gamle kantonsystem er ikke uændret gældende i 1825'
                                    if row['nation']=='PR' else 'Mecklenburgsk konskription med lodtrækning og stedfortrædere; forenklet årgangsmodel'
                                    if row['nation'] in ('MEC','MST') else 'Forenklet lokal kontingentreserve')
        districts.append(row)
    return districts


def scenario_oob(source, year):
    return copy.deepcopy(source['south']['ordersOfBattle'][str(year)])
