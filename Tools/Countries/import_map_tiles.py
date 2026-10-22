"""Lokale researchtabeller til flisekortet; eksisterende rækker bevares."""
import copy
import re
import unicodedata
NOTE='Skøn fra Docs/Kortudvidelse-Research-2026-10-20.md; ikke kildeverificeret.'
def slug(s):
    s=s.lower().replace('ø','o').replace('æ','ae').replace('ß','ss')
    return re.sub('[^a-z0-9]+','-',unicodedata.normalize('NFKD',s).encode('ascii','ignore').decode()).strip('-')
def import_research(root,year,original,project):
    text=(root/'Docs/Kortudvidelse-Research-2026-10-20.md').read_text(encoding='utf-8-sig')
    result=copy.deepcopy(original); cities=result['cities']; districts=result['districts']; research=[]
    section=text.split('## 2. Byer',1)[1].split('## 3.',1)[0]
    for nation,block in zip(('SE','NO','PR'),re.findall(r'```\s*\n(.*?)```',section,re.S)):
        for line in block.strip().splitlines():
            name,lat,lon,p25,p51,district=line.split('|')
            research.append(dict(name=name,owner=nation,lat=float(lat),lon=float(lon),pop=round(float(p25 if year==1825 else p51)*1000),researchDistrict=district,populationStatus='skøn',populationNote=NOTE,onMap=True,railway=False))
    positions={c['name']:(c['lon'],c['lat']) for c in research}
    positions.update({'Vänersborg':(12.32,58.38),'Mariestad':(13.82,58.71),'Fjaler':(5.40,61.36),'Steinkjer':(11.50,64.02),'Gumbinnen':(22.20,54.59),'Marienwerder':(18.93,53.74),'Köslin':(16.17,54.19),'Oppeln':(17.93,50.67),'Merseburg':(12.,51.35),'Arnsberg':(8.06,51.40),'Sigmaringen':(9.22,48.09)})
    additions=[]
    def add(nation,name,seat,pop,province=None,did=None,note=''):
        lon,lat=positions[seat]; did=did or nation+'-tile-'+slug(name)
        row=dict(id=did,name=name,seat=seat,nation=nation,region=province or nation,lon=lon,lat=lat,population=pop,populationStatus='skøn',populationNote=NOTE+note,boundaryStatus='skøn: nærmeste sæde inden for ejerområdet',status='estimated')
        if province:row['province']=province
        if not any(d['id']==did for d in districts):districts.append(copy.deepcopy(row))
        additions.append(row)
    block=re.findall(r'```\s*\n(.*?)```',text.split('### Sverige: län',1)[1],re.S)[0]
    seids='stockholm-city stockholm uppsala sodermanland ostergotland jonkoping kronoberg kalmar gotland blekinge kristianstad malmohus halland goteborg-bohus alvsborg skaraborg varmland orebro vastmanland kopparberg gavleborg vasternorrland jamtland vasterbotten norrbotten'.split()
    for line,did in zip(block.strip().splitlines(),seids):
        name,seat,p25,p51=line.split('|');add('SE',name,seat,int(p25 if year==1825 else p51),did='SE-'+did)
    nos=[('Smaalenene','Moss'),('Akershus','Christiania'),('Hedemarken','Hamar'),('Christians','Lillehammer'),('Buskerud','Drammen'),('Jarlsberg og Larvik','Tønsberg'),('Bratsberg','Skien'),('Nedenes','Arendal'),('Lister og Mandal','Kristiansand'),('Stavanger','Stavanger'),('Søndre Bergenhus','Bergen'),('Nordre Bergenhus','Fjaler'),('Romsdal','Molde'),('Søndre Trondhjem','Trondhjem'),('Nordre Trondhjem','Steinkjer'),('Nordland','Bodø'),('Tromsø','Tromsø'),('Finnmarken','Vardø')]
    total=1050000 if year==1825 else 1426000
    for i,(name,seat) in enumerate(nos):add('NO',name,seat,total//18+(i<total%18),note=' Nationaltotal ligeligt fordelt på 18 amter; 1851 interpoleret 1845–1855.')
    block=re.findall(r'```\s*\n(.*?)```',text.split('### Preussen: provins',1)[1],re.S)[0];spec=[]
    for line in block.strip().splitlines():
        province,_,seats=line.split('|')
        if province=='Hohenzollern':
            if year==1825:continue
            seats='Sigmaringen'
        if year==1851 and province in ('Ostpreußen','Westpreußen'):province='Preussen'
        for seat in seats.split(';'):spec.append((province,seat.split(' (')[0].strip()))
    total=12300000 if year==1825 else 16300000
    for i,(province,seat) in enumerate(spec):add('PR',seat,seat,total//len(spec)+(i<total%len(spec)),province,note=' Nationaltotal ligeligt fordelt på Regierungsbezirke; ikke distriktsfolketælling.')
    names={c['name'] for c in cities};outside={c['name']:c for c in result.get('outside',[])};added=[]
    for row in research:
        candidates=[d for d in additions if d['nation']==row['owner']]
        exact=[d for d in candidates if slug(d['name'])==slug(row['researchDistrict'].replace(' o. ',' och '))]
        row['district']=min(exact or candidates,key=lambda d:sum((a-b)**2 for a,b in zip(project(d['lon'],d['lat']),project(row['lon'],row['lat']))))['id']
        aliases=[c for c in cities if c.get('owner')==row['owner'] and c.get('lon') is not None and c.get('lat') is not None and sum((a-b)**2 for a,b in zip(project(c['lon'],c['lat']),project(row['lon'],row['lat'])))<4]
        if row['name'] not in names and aliases:
            row['legacyName']=min(aliases,key=lambda c:sum((a-b)**2 for a,b in zip(project(c['lon'],c['lat']),project(row['lon'],row['lat']))))['name']
        if row['name'] not in names and not aliases:
            merged=copy.deepcopy(outside.get(row['name'],{}));merged.update(row);cities.append(merged);names.add(row['name']);added.append(row['name'])
    for district in additions:
        seat=district['seat'];alias=next((c.get('legacyName',c['name']) for c in research if c['name']==seat),seat)
        district['seatCityName']=alias
        if alias not in names:
            lon,lat=positions[seat]
            cities.append(dict(name=seat,owner=district['nation'],lon=lon,lat=lat,pop=0,populationStatus='skøn',populationNote=NOTE+' Intet bytal i research; 0 er ukendt, ikke faktisk nul.',district=district['id'],onMap=True,railway=False))
            names.add(seat);added.append(seat)
    result['outside']=[c for c in result.get('outside',[]) if c['name'] not in added]
    result.update(researchDistricts=additions,researchCities=research,tileAddedCities=added,tileDataNote=NOTE+' Eksisterende rækker bevaret; researchDistricts indeholder nye skøn.')
    return result
