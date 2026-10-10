"""Skriv den eksplicitte sydlige datakilde; derefter køres export_neighbours.py.

Alle demografiske og militære tal er skøn, ikke verificerede mønstringslister.
"""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
BASE = json.loads((HERE / 'neighbours.json').read_text(encoding='utf-8'))
ROWS = '''Güstrow|12.176|53.797|MEC|8000|10000|1850
Parchim|11.849|53.426|MEC|5000|6500|1880
Ludwigslust|11.497|53.329|MEC|4000|5500|1846
Waren|12.685|53.52|MEC|4000|5500|1885
Neustrelitz|13.063|53.362|MST|5000|7500|1877
Neubrandenburg|13.261|53.557|MST|5000|7000|1864
Boizenburg|10.723|53.377|MEC|3000|4000|1846
Hagenow|11.193|53.431|MEC|3000|4000|1846
Grabow|11.566|53.279|MEC|2500|3500|1846
Bützow|11.982|53.848|MEC|3000|4500|1850
Schwaan|12.106|53.94|MEC|1600|2400|1850
Bad Kleinen|11.472|53.772|MEC|300|500|1848
Blankenberg|11.714|53.772|MEC|300|450|1850
Sternberg|11.825|53.713|MEC|2200|3000|1887
Crivitz|11.648|53.575|MEC|1600|2400|1898
Neukloster|11.685|53.866|MEC|1300|1900|1887
Gadebusch|11.116|53.701|MEC|2000|2700|1897
Grevesmühlen|11.183|53.862|MEC|2300|3300|1870
Rehna|11.049|53.78|MEC|1600|2200|1897
Doberan|11.9|54.106|MEC|1800|2500|1883
Warnemünde|12.086|54.176|MEC|1800|2300|1886
Ribnitz|12.432|54.243|MEC|2200|3000|1889
Marlow|12.571|54.154|MEC|1400|2000|9999
Tessin|12.466|54.027|MEC|1800|2600|1895
Teterow|12.575|53.775|MEC|3200|4300|1864
Malchin|12.764|53.739|MEC|3800|5200|1864
Stavenhagen|12.906|53.697|MEC|2000|3000|1864
Röbel|12.604|53.377|MEC|2600|3400|1899
Plau|12.262|53.458|MEC|2400|3200|1882
Lübz|12.029|53.463|MEC|2200|3000|1885
Dömitz|11.254|53.138|MEC|1300|1900|1873
Friedland|13.554|53.669|MST|4000|5000|1884
Anklam|13.69|53.856|PR|6000|9500|1863
Swinemünde|14.251|53.91|PR|3500|6500|1876
Stargard|15.041|53.337|PR|8500|14000|1846
Pasewalk|13.99|53.506|PR|4000|6000|1863
Demmin|13.041|53.908|PR|5000|7500|1877
Ueckermünde|14.045|53.737|PR|2800|4500|1884
Wolgast|13.774|54.053|PR|5000|6500|1863
Barth|12.727|54.365|PR|4000|5500|1888
Grimmen|13.041|54.112|PR|3000|4000|1877
Tribsees|12.757|54.095|PR|2200|3000|1895
Franzburg|12.884|54.185|PR|1400|1800|1895
Putbus|13.471|54.355|PR|800|1500|1889
Garz på Rügen|13.351|54.317|PR|1500|2000|1895
Sassnitz|13.644|54.516|PR|300|500|1891
Gartz|14.387|53.207|PR|3000|4200|1913
Prenzlau|13.86|53.316|PR|8000|12000|1863
Schwedt|14.284|53.06|PR|5500|8500|1873
Bergedorf|10.211|53.488|HH|2300|3500|1842
Lüneburg|10.407|53.25|HAN|10000|13000|1847
Bleckede|10.735|53.287|HAN|1400|2200|1895
Kolberg|15.576|54.176|PR|7000|11000|1859
Angermünde|13.998|53.014|PR|3000|5000|1843
Passow|14.116|53.143|PR|500|800|1843
Tantow|14.348|53.271|PR|400|700|1843'''


def seed():
    cities = []
    for line in ROWS.splitlines():
        name, lon, lat, nation, p25, p51, rail = line.split('|')
        cities.append(dict(name=name, lon=float(lon), lat=float(lat), owner=nation,
                           region={'MEC':'Mecklenburg-Schwerin','MST':'Mecklenburg-Strelitz','PR':'Pommern / Brandenburg','HH':'Hamborg','HAN':'Hannover'}[nation],
                           population={'1825':int(p25),'1851':int(p51)}, populationStatus='estimated',
                           harbour=name in ['Boizenburg','Dömitz','Warnemünde','Ribnitz','Anklam','Swinemünde','Ueckermünde','Wolgast','Barth','Sassnitz','Schwedt','Bleckede','Kolberg'],
                           fortress=name in ['Dömitz','Swinemünde','Kolberg'], garrison=0, guns=0, railwayYear=int(rail)))
    lookup = {c['name']:c for c in BASE['cities']+cities}
    cities.append(dict(name='Lauenburg',lon=10.555,lat=53.373,owner='DK',region='Lauenburg',
                       population={'1825':3500,'1851':4500},populationStatus='estimated',harbour=True,
                       fortress=False,garrison=0,guns=0,railwayYear=1853))
    cities.append(dict(name='Büchen',lon=10.616,lat=53.481,owner='DK',region='Lauenburg',
                       population={'1825':400,'1851':600},populationStatus='estimated',harbour=False,
                       fortress=False,garrison=0,guns=0,railwayYear=1846))
    # (id, name, seat, 1825 population, 1851 population, towns); clipped map districts.
    district_rows = [
        ('MEC-schwerin','Domanialamt Schwerin','Schwerin',43000,53000,['Schwerin','Crivitz','Sternberg','Bad Kleinen','Blankenberg']),
        ('MEC-hagenow','Domanialamt Hagenow','Hagenow',25000,32000,['Hagenow']),
        ('MEC-boizenburg','Domanialamt Boizenburg','Boizenburg',25000,30000,['Boizenburg']),
        ('MEC-gadebusch','Domanialamt Gadebusch','Gadebusch',28000,34000,['Gadebusch','Rehna']),
        ('MEC-grevesmuehlen','Domanialamt Grevesmühlen','Grevesmühlen',27000,32000,['Grevesmühlen']),
        ('MEC-wismar','Wismar og Domanialamt Mecklenburg','Wismar',27000,32000,['Wismar','Neukloster']),
        ('MEC-rostock','Rostock og Domanialamt Doberan','Rostock',44000,53000,['Rostock','Doberan','Warnemünde','Schwaan']),
        ('MEC-ribnitz','Domanialamt Ribnitz','Ribnitz',21000,27000,['Ribnitz','Marlow','Tessin']),
        ('MEC-buetzow','Domanialamt Bützow','Bützow',22000,28000,['Bützow']),
        ('MEC-guestrow','Domanialamt Güstrow','Güstrow',32000,39000,['Güstrow','Teterow']),
        ('MEC-stavenhagen','Domanialamt Stavenhagen','Stavenhagen',28000,34000,['Stavenhagen','Malchin']),
        ('MEC-waren','Domanialamt Waren','Waren',26000,32000,['Waren']),
        ('MEC-roebel','Domanialamt Röbel','Röbel',22000,27000,['Röbel']),
        ('MEC-plau','Domanialamt Plau','Plau',24000,30000,['Plau']),
        ('MEC-parchim','Parchim og Domanialamt Lübz','Parchim',31000,39000,['Parchim','Lübz']),
        ('MEC-ludwigslust','Domanialamt Ludwigslust / Grabow','Ludwigslust',27000,33000,['Ludwigslust','Grabow','Dömitz']),
        ('MST-stargard','Herrschaft Stargard (Strelitz)','Neustrelitz',62000,80000,['Neustrelitz','Neubrandenburg','Friedland']),
        ('PR-stralsund','Stadtkreis Stralsund','Stralsund',16000,20000,['Stralsund']),
        ('PR-franzburg','Kreis Franzburg','Franzburg',40000,49000,['Franzburg','Barth','Grimmen','Tribsees']),
        ('PR-greifswald','Kreis Greifswald','Greifswald',49000,64000,['Greifswald','Wolgast']),
        ('PR-ruegen','Kreis Rügen','Bergen på Rügen',32000,40000,['Bergen på Rügen','Putbus','Garz på Rügen','Sassnitz']),
        ('PR-demmin','Kreis Demmin','Demmin',38000,50000,['Demmin']),
        ('PR-anklam','Kreis Anklam','Anklam',30000,39000,['Anklam']),
        ('PR-usedom','Kreis Usedom-Wollin','Swinemünde',34000,47000,['Swinemünde']),
        ('PR-ueckermuende','Kreis Ueckermünde','Ueckermünde',30000,42000,['Ueckermünde','Pasewalk']),
        ('PR-randow','Kreis Randow (med Stettin)','Stettin',74000,105000,['Stettin','Gartz']),
        ('PR-saatzig','Kreis Saatzig (kortudsnit)','Stargard',28000,39000,['Stargard']),
        ('PR-prenzlau','Kreis Prenzlau (kortudsnit)','Prenzlau',44000,60000,['Prenzlau']),
        ('PR-angermuende','Kreis Angermünde (kortudsnit)','Angermünde',32000,44000,['Schwedt','Angermünde','Passow','Tantow']),
        ('PR-kolberg','Kreis Fürstenthum (kortudsnit ved Kolberg)','Kolberg',55000,70000,['Kolberg']),
        ('HH-city','Hamborg og Bergedorf','Hamborg',115000,138000,['Hamborg','Bergedorf']),
        ('LUB-city','Lübeck','Lübeck',28000,32000,['Lübeck']),
        ('HAN-lueneburg','Landdrostei Lüneburg (kortudsnit)','Lüneburg',55000,70000,['Lüneburg','Harburg','Bleckede']),
        ('HAN-stade','Landdrostei Stade (kortudsnit)','Stade',45000,56000,['Stade'])]
    districts, overrides = [], {}
    for did, name, seat, p25, p51, towns in district_rows:
        city = lookup[seat]; nation = city['owner']
        region = city['region']
        if nation == 'PR':
            region = 'Regierungsbezirk ' + ('Stralsund' if did in ['PR-stralsund','PR-franzburg','PR-greifswald','PR-ruegen'] else 'Potsdam' if did in ['PR-prenzlau','PR-angermuende'] else 'Köslin' if did=='PR-kolberg' else 'Stettin')
        districts.append(dict(id=did,name=name,seat=seat,nation=nation,region=region,lon=city['lon'],lat=city['lat'],
                              population={'1825':p25,'1851':p51},status='estimated',boundaryStatus='estimated-nearest-seat-clipped-by-owner',
                              ritterschaftShare=.4 if nation=='MEC' else 0,reserveShare=.03 if nation=='PR' else .02,yearlyClassShare=.009,capShare=.09,
                              ruralTax=.16,urbanTax=.36,grainPerHead=.38,cattlePerHead=.24,towns=towns))
        for town in towns:
            if town in [c['name'] for c in cities]: lookup[town]['district'] = did
            else: overrides[town] = dict(district=did,populationStatus='estimated')
    for town, men, guns in [('Dömitz',250,8),('Swinemünde',500,24),('Kolberg',1200,36)]:
        lookup[town].update(fortName=town+' fæstning',garrison=men,guns=guns)
    chains = [
        ['Lübeck','Grevesmühlen','Wismar','Neukloster','Rostock','Ribnitz','Barth','Stralsund'],
        ['Lübeck','Rehna','Gadebusch','Schwerin','Crivitz','Parchim','Lübz','Plau','Waren','Neubrandenburg','Friedland','Anklam','Ueckermünde','Stettin'],
        ['Hamborg','Bergedorf','Boizenburg','Hagenow','Ludwigslust','Grabow','Dömitz'],
        ['Schwerin','Bad Kleinen','Blankenberg','Bützow','Schwaan','Rostock','Warnemünde'],
        ['Bützow','Güstrow','Teterow','Malchin','Stavenhagen','Neubrandenburg'],
        ['Schwerin','Hagenow'],['Bad Kleinen','Wismar'],['Güstrow','Sternberg','Crivitz'],
        ['Rostock','Doberan','Wismar'],['Rostock','Tessin','Marlow','Ribnitz'],['Ludwigslust','Parchim'],
        ['Waren','Röbel','Neustrelitz','Neubrandenburg'],['Stralsund','Grimmen','Demmin','Neubrandenburg'],
        ['Stralsund','Franzburg','Tribsees','Demmin'],
        ['Greifswald','Anklam','Pasewalk','Prenzlau','Schwedt','Gartz','Stettin','Stargard','Kolberg'],
        ['Angermünde','Passow','Tantow','Stettin'],['Angermünde','Schwedt'],['Prenzlau','Angermünde'],
        ['Greifswald','Wolgast'],['Bergen på Rügen','Putbus','Garz på Rügen'],['Bergen på Rügen','Sassnitz'],
        ['Harburg','Lüneburg','Bleckede']]
    roads = [[a,b] for chain in chains for a,b in zip(chain,chain[1:])]
    roads += [['Ratzeburg','Gadebusch'],['Ratzeburg','Lauenburg'],['Lauenburg','Boizenburg'],['Bergedorf','Lauenburg'],['Bleckede','Boizenburg','Elben'],['Wolgast','Swinemünde','Peenestrom / Usedom']]
    roads += [['Bergedorf','Büchen'],['Büchen','Boizenburg'],['Ratzeburg','Büchen'],['Büchen','Lauenburg']]
    rails = [dict(name=name,via=via,opened=date) for name,via,date in [
        ('Berlin–Hamborg (kortudsnit)',['Hamborg','Bergedorf','Büchen','Boizenburg','Hagenow','Ludwigslust','Grabow'],'1846-12-15'),
        ('Hagenow–Schwerin',['Hagenow','Schwerin'],'1847-05-01'),
        ('Schwerin–Wismar',['Schwerin','Bad Kleinen','Wismar'],'1848-07-12'),
        ('Kleinen–Rostock',['Bad Kleinen','Blankenberg','Bützow','Schwaan','Rostock'],'1850-05-13'),
        ('Bützow–Güstrow',['Bützow','Güstrow'],'1850-05-13'),
        ('Stettin–Stargard',['Stettin','Stargard'],'1846-05-01'),
        ('Berlin–Stettin (kortudsnit)',['Angermünde','Passow','Tantow','Stettin'],'1843-08-15'),
        ('Harburg–Lüneburg',['Harburg','Lüneburg'],'1847-05-01'),
        ('Lübeck–Büchen',['Lübeck','Ratzeburg','Büchen'],'1851-10-15'),
        ('Büchen–Lauenburg',['Büchen','Lauenburg'],'1853-07-01')]]
    oob = {}
    for year in (1825,1851):
        units = []
        def unit(uid,name,nation,corps,arm,town,peace,war,count,guns=0,division=''):
            key = {'infantry':'companies','cavalry':'squadrons','artillery':'batteries'}[arm]
            units.append(dict(id=uid,name=name,nation=nation,corps=corps,division=division,arm=arm,town=town,
                              district=lookup[town].get('district',overrides.get(town,{}).get('district')),
                              chief='',chiefStatus='unknown',men=peace,mobilisedMen=war,guns=guns,status='estimated',
                              **{key:[dict(name=f'{k}. '+{'infantry':'Kompanie','cavalry':'Eskadron','artillery':'Batterie'}[arm],men=war//count,**({'guns':guns//count} if guns else {})) for k in range(1,count+1)]}))
        for nr,town in [(2,'Stettin'),(9,'Stargard'),(14,'Stralsund'),(21,'Stettin')]:
            unit(f'PR-IR{nr}',f'Infanterie-Regiment Nr. {nr}','PR','PR-pomerania','infantry',town,1400 if year==1825 else 1600,3000,12,division='3. Division' if nr in [2,9] else '4. Division')
        unit('PR-JG2','2. Jäger-Abteilung' if year==1825 else '2. Jäger-Bataillon','PR','PR-pomerania','infantry','Greifswald',400,600,4,division='Korpsjægere')
        unit('PR-KR2','Kürassier-Regiment Königin (Pommersches) Nr. 2','PR','PR-pomerania','cavalry','Pasewalk',400,600,4,division='Korpskavallerie')
        unit('PR-ART2','2. Artillerie-Brigade (feltdetachement)','PR','PR-pomerania','artillery','Stettin',600,1200,6,48,'Korpsartilleri')
        unit('PR-LW','Pommerske Landwehr-bataljoner (samlet skøn)','PR','PR-pomerania','infantry','Greifswald',0,3600 if year==1825 else 9600,24,division='Landwehr')
        unit('MEC-INF','Großherzoglich Mecklenburgisches Infanterie-Kontingent','MEC','MEC-field','infantry','Schwerin',1200,2400,12)
        unit('MEC-DR','Mecklenburgisches Chevauleger-Kontingent' if year==1825 else 'Mecklenburgisches Dragoner-Kontingent','MEC','MEC-field','cavalry','Ludwigslust',300,450 if year==1825 else 600,3 if year==1825 else 4)
        unit('MEC-ART','Mecklenburgische Feldbatterie','MEC','MEC-field','artillery','Schwerin',120,150,1,6)
        if year==1851:
            units[-3]['mobilisedMen']=2250
            # Three battalions / twelve companies; exact sum rather than rounded loss.
            for i, company in enumerate(units[-3]['companies']): company['men']=188 if i<6 else 187
        oob[str(year)] = units
    result = dict(note='Befolkninger, distriktsgrænser, økonomi og kamporden er markerede skøn. Ritterschaft er en overlejret grundherrschaft, ikke ét sammenhængende amt. Ingen verificeret rangliste eller opdigtede chefer.',
                  nation=dict(id='MST',name='Mecklenburg-Strelitz',colour=[156,123,166],lat=53.4,lon=13.1),
                  territories=[dict(name='Herrschaft Stargard (Strelitz; grænseskøn)',owner='MST',polygon=[[12.88,53.22],[13.33,53.2],[13.58,53.42],[13.56,53.72],[13.29,53.76],[12.89,53.6]])],
                  cities=cities,districts=districts,townOverrides=overrides,roads=roads,railways=rails,ordersOfBattle=oob,
                  chaussees=[dict(a='Lauenburg',b='Boizenburg',opened=1829),dict(a='Boizenburg',b='Hagenow',opened=1829),dict(a='Hagenow',b='Ludwigslust',opened=1829)],
                  outside=[dict(name='Köslin',owner='PR',lon=16.172,lat=54.194,population={'1825':5500,'1851':8500},district='Kreis Köslin',region='Regierungsbezirk Köslin',status='estimated')],
                  references1864=dict(note='Reference, ikke et scenarie eller uundgåelig offensiv. Ekspeditioner må ikke dobbelttælle II. korps. Mecklenburg følger ikke automatisk Preussen.',
                                      commands=['Gardekorps (Prinz August von Württemberg)','III. Armeekorps (Prinz Friedrich Karl)','Østrigsk VI. korps (Ludwig von Gablenz)']))
    result['sources'] = [
        dict(url='https://metropolregion.hamburg.de/natur-und-kultur-erleben/industriekultur-am-wasser/routen/schwerin/hauptbahnhof-eisenbahndirektion-11958',scope='Hagenow 1847, Wismar 1848, Rostock/Güstrow 1850, Lübeck 1870; ikke befolkningstal'),
        dict(url='https://onlinesammlung.museumsstiftung.de/detail/collection/0f3442a3-8efa-48eb-be81-352e39fa89ef',scope='Berlin–Stettin åbnet 15. august 1843'),
        dict(url='https://www.ortschroniken-mv.de/index.php/Geschichte_Stadt_und_Amt_Boizenburg/Elbe_in_Daten_bis_1918',scope='Hamborg–Berlin chaussé 1826–1829; bane 1846'),
        dict(url='https://www.geschichte-s-h.de/wp-content/uploads/2017/11/Mitteilungen93.pdf',scope='Lübeck–Büchen 1851, Büchen–Lauenburg 1853; andre oversigter blander disse åbninger sammen'),
        dict(url='https://kpbc.umk.pl/Content/236544/PDF/Gromadzenie_POPC_018_01.pdf',scope='Historisk garnisonsoversigt: Greifswalds 2. jægere fra 1821/1848, Pasewalks Königin-kürassierer; ikke komplet 1825/1851-kamporden'),
        dict(url='https://sbc.org.pl/en/dlibra/publication/389394/kurzgefa-te-stamm-und-rangliste-aller-regimenter-der-koniglich-preu-ischen-armee-von-deren-stiftung-an-bis-ende',scope='Katalog til primære ranglister. Årgangenes fulde korps-/chefsoplysninger er endnu ikke transskriberet.')]
    (HERE / 'south.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    for year in (1825,1851):
        path = HERE.parents[1] / 'Data/Campaign1851' / ('Nations_1825.json' if year==1825 else 'Nations1851.json')
        data = json.loads(path.read_text(encoding='utf-8-sig'))
        if not any(n['id']=='MST' for n in data['nations']):
            data['nations'].append(dict(id='MST',name='Mecklenburg-Strelitz',onMap=False,controller='ai',
                                       population=62000 if year==1825 else 80000,armyMen=700,railKm=0,growth=.7,taxPerHead=.12,
                                       relation=10,tradeValue=300,weights=[1,1,1,.2,.8,1,1,1],caution=.7,reserve=10000,
                                       group='tysk',status='estimated',note='Selvstændig stat; Herrschaft Stargard i kortudsnittet. Ratzeburg-landdelen er endnu ikke detaljeret.'))
            path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'Sydkilde: {len(cities)} nye byer; {len(districts)} distrikter')


if __name__ == '__main__':
    seed()
