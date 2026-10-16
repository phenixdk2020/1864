"""Reproducerbar SE/NO-import. Ingen build eller spilstart; urørte JSON-tokens bevares."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
PACKAGE = Path(r'C:\Users\allan\Documents\Codex\2026-10-12\l\outputs\Sverige-Norge-agent\Spilimport')
DECODER = json.JSONDecoder()
def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))
def spans(text, start=0, path=()):
    while text[start].isspace(): start += 1
    value, end = DECODER.raw_decode(text, start)
    result = {path:(start,end)}
    if isinstance(value,(dict,list)):
        pos = start+1
        for key in (value if isinstance(value,dict) else range(len(value))):
            while text[pos].isspace() or text[pos]==',': pos+=1
            if isinstance(value,dict):
                _,pos=DECODER.raw_decode(text,pos)
                while text[pos].isspace() or text[pos]==':': pos+=1
            children,pos=spans(text,pos,path+(key,))
            result.update(children)
    return result,end
def patch(p,old,new):
    raw=p.read_bytes(); text=raw.decode('utf-8-sig'); locations,_=spans(text); edits=[]
    nl='\r\n' if b'\r\n' in raw else '\n'
    def enc(v): return json.dumps(v,ensure_ascii=False,allow_nan=False,indent=2).replace('\n',nl)
    def visit(a,b,key=()):
        if a==b:return
        start,end=locations[key]
        if isinstance(a,dict) and isinstance(b,dict) and set(a)<=set(b):
            for k in a: visit(a[k],b[k],key+(k,))
            extra={k:b[k] for k in b if k not in a}
            if extra: edits.append((end-1,end-1,(',' if a else '')+nl+enc(extra)[1:-1].strip()+nl))
        elif isinstance(a,list) and isinstance(b,list) and len(a)<=len(b):
            for i,v in enumerate(a): visit(v,b[i],key+(i,))
            if len(b)>len(a): edits.append((end-1,end-1,(',' if a else '')+nl+(','+nl).join(enc(v) for v in b[len(a):])+nl))
        else: edits.append((start,end,enc(b)))
    visit(old,new)
    for start,end,value in sorted(edits,reverse=True): text=text[:start]+value+text[end:]
    assert json.loads(text)==new
    output=(b'\xef\xbb\xbf' if raw.startswith(b'\xef\xbb\xbf') else b'')+text.encode('utf-8')
    if raw!=output:p.write_bytes(output)
def project(lon,lat):
    lam,phi,phi0=map(math.radians,(lon-10,lat,52))
    k=math.sqrt(2/(1+math.sin(phi0)*math.sin(phi)+math.cos(phi0)*math.cos(phi)*math.cos(lam)))
    return [6371.0088*k*math.cos(phi)*math.sin(lam),6371.0088*k*(math.cos(phi0)*math.sin(phi)-math.sin(phi0)*math.cos(phi)*math.cos(lam))]
def main():
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('--package',type=Path,default=PACKAGE); ap.add_argument('--root',type=Path,default=ROOT); args=ap.parse_args()
    package=args.package; data=args.root/'Data/Campaign1851'; archive=data/'Countries/SwedenNorway'
    report={k:[] for k in ['added','updated','skippedNull','conflicts','deferred','coverage','mapping','lineEndings']}
    def write(p,v):
        p.parent.mkdir(parents=True,exist_ok=True); raw=(json.dumps(v,ensure_ascii=False,indent=2,allow_nan=False)+'\n').encode('utf-8')
        if not p.exists() or p.read_bytes()!=raw:p.write_bytes(raw)
    def preserve(src,dst):
        read(src); dst.parent.mkdir(parents=True,exist_ok=True)
        if not dst.exists() or dst.read_bytes()!=src.read_bytes():dst.write_bytes(src.read_bytes())
    def nulls(v,path):
        if v is None:report['skippedNull'].append(path)
        elif isinstance(v,dict):
            for k,c in v.items():nulls(c,path+'/'+k)
        elif isinstance(v,list):
            for i,c in enumerate(v):nulls(c,path+'/'+str(i))
    def merge(a,b):
        result=copy.deepcopy(a)
        for k,v in b.items():
            if v is None:continue
            result[k]=merge(result[k],v) if isinstance(v,dict) and isinstance(result.get(k),dict) else copy.deepcopy(v)
        return result
    def upsert(rows,row,key,location):
        idx=next((i for i,r in enumerate(rows) if r.get(key)==row[key]),None)
        if idx is None:rows.append(merge({},row)); report['added'].append(location+':'+str(row[key]))
        elif any(rows[idx].get(k) not in (None,row[k]) for k in ['owner','nation'] if k in row):
            report['conflicts'].append(location+':'+str(row[key])+': anden ejer/nation; sprunget over')
        elif rows[idx]!=merge(rows[idx],row):rows[idx]=merge(rows[idx],row); report['updated'].append(location+':'+str(row[key]))
    manifest=read(package/'GameReady-manifest.json')
    for name,meta in manifest['exports'].items():
        src=package/'GameReady'/name
        assert hashlib.sha256(src.read_bytes()).hexdigest()==manifest['hashes'][name],name
        preserve(src,archive/'GameReady'/name); nulls(read(src),name)
        target=name; loader='Ingen national loader; arkiv, ikke aktiv dansk data'
        if name.startswith('Nations'):loader='Campaign1851Nations.cpp:LoadNations'
        elif name.startswith('cities_') or 'Army_Neighbours' in name:target=Path(name).name; loader='Campaign1851Neighbours.cpp:LoadNeighbours'
        elif name.startswith('Trade_'):loader='Ingen C++-loader; kalibreringsdata'
        else:target='Countries/SwedenNorway/GameReady/'+name
        existing_loader=loader
        if '/Supply_' in name:existing_loader='Campaign1851Army.cpp:LoadArmy / ArmyEquipmentNumber (kun dansk pulje)'
        elif '/Army' in name and 'Army_Neighbours' not in name:existing_loader='Campaign1851Army.cpp:LoadArmy (kun dansk pulje)'
        elif '/Economy_' in name:existing_loader='Campaign1851Economy.cpp:EconomyValue (1825 DK; 1851 hardcoded)'
        elif '/Ministers_' in name:existing_loader='Campaign1851Ministers.cpp:LoadScenarioMinisters (kun 1825 DK)'
        elif name.startswith('Officers'):existing_loader='Campaign1851Officers.cpp:LoadOfficers (kun dansk pulje)'
        elif name.startswith('Population_'):existing_loader='Campaign1851Map.cpp:LoadData (1825-overlay; 1851 basekort)'
        report['mapping'].append({'overlay':name,'schema':meta['schema'],'target':target,'loader':loader,'existingLoader':existing_loader})
    for name in ['GameReady-manifest.json','GameReady-proveniens.json','Sources.json','Kildegrundlag.json','Data-status.json','Valuta.json','Union-regler.json','Events.json','Kortkrav.json','Runde7-cirkatal.json','Diplomati.json','Kamp-AI.json']:
        preserve(package/name,archive/name)
    for nation in ['SE','NO']:
        for src in sorted((package/nation).glob('*.json')):preserve(src,archive/'Research'/nation/src.name)
    extent=read(data/'Denmark1851_Map.json')['extentKm']
    def inside(row):
        if row.get('lon') is None or row.get('lat') is None:return False,None
        x,y=project(row['lon'],row['lat'])
        return extent['xMin']<=x<extent['xMax'] and extent['yMin']<y<=extent['yMax'],[x,y]
    for year in [1825,1851]:
        path=data/f'cities_neighbours_{year}.json'; old=read(path); new=copy.deepcopy(old); overlay=read(package/'GameReady'/path.name)
        grid=new['ownerGrid']; raw=(data/grid['file']).read_bytes(); names={r['name'] for r in new['cities']}
        outside=copy.deepcopy(new.get('outside',[]))
        points={r['name']:r for r in overlay['outside']+overlay['cities']}
        for name,row in points.items():
            if row.get('owner') not in ['SE','NO']:continue
            within,xy=inside(row); supported=False
            if within:
                ix=int((xy[0]-extent['xMin'])/(extent['xMax']-extent['xMin'])*grid['width']); iy=int((extent['yMax']-xy[1])/(extent['yMax']-extent['yMin'])*grid['height'])
                supported=raw[iy*grid['width']+ix]==grid['ids'].get(row['owner'],-1)
            active=supported and row.get('pop') is not None
            report['coverage'].append(dict(scenario=str(year),kind='city',name=name,nation=row['owner'],insideExtent=within,ownerGridMatches=supported,onMap=active,km=xy))
            candidate=copy.deepcopy(row); candidate['onMap']=active; candidate.pop('district',None)
            if active:
                if name not in names and (row.get('guns') is None or row.get('garrison') is None):report['conflicts'].append(f'{year}: {name}: garnison/kanoner ukendt');continue
                upsert(new['cities'],candidate,'name',path.name+'/cities')
            else:
                if name not in names:upsert(outside,candidate,'name',path.name+'/outside')
                report['deferred'].append(f'{year}: {name}: uden kortunderstøttelse eller scenarie-pop')
        new['outside']=outside
        regions={r['id']:r for r in read(package/'Kortkrav.json')['regions']}; districts=[]
        for row in overlay['districts']:
            if row.get('nation') not in ['SE','NO']:continue
            within,xy=inside(regions.get(row['id'],{})); districts.append({**row,'onMap':False})
            report['coverage'].append(dict(scenario=str(year),kind='district',name=row['id'],insideExtent=within,onMap=False,km=xy))
        write(archive/f'Districts_{year}.json',dict(scenario=str(year),note='Punktreference uden distriktsgrid. Status/kilder i GameReady-proveniens.',districts=districts))
        valid={r['name'] for r in new['cities']}
        for link in overlay['links']:
            if not link.get('km') or link['a'] not in valid or link['b'] not in valid:report['deferred'].append(f"{year}: rute {link['a']}–{link['b']}: geometri/endepunkt mangler");continue
            if not any({r['a'],r['b']}=={link['a'],link['b']} for r in new['links']):new['links'].append(link);report['added'].append(f"{year}: rute {link['a']}–{link['b']}")
        patch(path,old,new)
        path=data/('Nations_1825.json' if year==1825 else 'Nations1851.json');old=read(path);new=copy.deepcopy(old)
        for row in read(package/'GameReady'/path.name)['nations']:
            assert row['id'] in ['SE','NO'];upsert(new['nations'],row,'id',path.name)
        patch(path,old,new)
        path=data/f'Army_Neighbours_{year}.json';old=read(path);new=copy.deepcopy(old)
        for nation in ['SE','NO']:
            for row in read(package/f'GameReady/{nation}/Army_Neighbours_{year}.json')['armies']:
                assert row['nation']==nation
                if row['town'] not in valid:report['deferred'].append(f"{year}: {row['id']}: samlingsby uden aktivt kort");continue
                row=copy.deepcopy(row)
                if row['id']=='SE-field':row['id']='SE-south'
                upsert(new['armies'],row,'id',path.name)
        patch(path,old,new)
        path=data/f'Trade_{year}.json'; imported=read(package/'GameReady'/path.name)
        if path.exists():
            old=read(path);new=copy.deepcopy(old)
            for country in ['Sverige','Norge']:
                value=imported['areas'][country]; areas=new.setdefault('areas',{})
                if areas.get(country)!=value:report['updated' if country in areas else 'added'].append(path.name+'/areas:'+country)
                areas[country]=merge(areas.get(country,{}),value)
            patch(path,old,new)
        else:write(path,imported);report['added'].append(path.name)
    for p in sorted(archive.rglob('*.json')):read(p)
    for p in data.glob('*.json'):
        if p.name.startswith(('cities_neighbours_','Army_Neighbours_','Nations','Trade_')):
            raw=p.read_bytes();report['lineEndings'].append(dict(file=p.name,CRLF=raw.count(b'\r\n'),LF=raw.count(b'\n'),bareCR=raw.count(b'\r')-raw.count(b'\r\n')))
    report['counts']={k:len(report[k]) for k in ['added','updated','skippedNull','conflicts','deferred']}
    write(args.root/'Docs/Import-Sverige-Norge-rapport.json',report);print(json.dumps(report['counts']))
if __name__=='__main__':main()
