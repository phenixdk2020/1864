"""Statisk importkontrol: beskyttede bytes, JSON, kort, proveniens og idempotens."""
import ast
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path
from import_sweden_norway import ROOT, PACKAGE, read, spans
DATA=ROOT/'Data/Campaign1851'
def git_bytes(name):
    return subprocess.check_output(['git','cat-file','--filters','HEAD:Data/Campaign1851/'+name],cwd=ROOT)
def row_bytes(raw,key,index):
    text=raw.decode('utf-8-sig'); locations,_=spans(text);start,end=locations[(key,index)]
    return text[start:end].encode('utf-8')
def main():
    if (DATA/'Countries/LevelB.json').exists():
        from validate_level_b import validate
        validate('SE/NO')
        from validate_level_c import validate as validate_c
        validate_c()
        return
    checked=0
    for name,key in [('Nations_1825.json','nations'),('Nations1851.json','nations')]+[(f'{prefix}_{year}.json',key) for year in [1825,1851] for prefix,key in [('cities_neighbours','cities'),('Army_Neighbours','armies')]]:
        original=git_bytes(name); current=(DATA/name).read_bytes(); before=json.loads(original);after=json.loads(current)
        for i,row in enumerate(before[key]):
            nation=row.get('owner',row.get('nation',row.get('id')))
            if nation in ['SE','NO','PR']:continue  # SE/NO: egen import; PR: Tools/Countries/import_prussia.py (se validate_prussia.py)
            assert row==after[key][i],(name,i,'ændret fremmed række')
            assert row_bytes(original,key,i)==row_bytes(current,key,i),(name,i,'ændrede bytes')
            checked+=1
        # Every old city/army keeps its exact index and ID, including SE-south.
        identity='name' if key=='cities' else 'id'
        assert [r[identity] for r in before[key]]==[r[identity] for r in after[key][:len(before[key])]]
    before=json.loads(git_bytes('Trade_1851.json'));after=read(DATA/'Trade_1851.json')
    for key,value in before.items():
        if key=='areas':
            for country,row in value.items(): assert after[key][country]==row
        else:assert after[key]==value
    archive=DATA/'Countries/SwedenNorway';manifest=read(archive/'GameReady-manifest.json')
    for name,digest in manifest['hashes'].items():assert hashlib.sha256((archive/'GameReady'/name).read_bytes()).hexdigest()==digest,name
    parsed=0
    for path in DATA.rglob('*.json'):read(path);parsed+=1
    report=read(ROOT/'Docs/Import-Sverige-Norge-rapport.json')
    for year in ['1825','1851']:
        coverage=[r for r in report['coverage'] if r['scenario']==year]
        assert len([r for r in coverage if r['kind']=='district'])==42
        assert not any(r['onMap'] and not r['insideExtent'] for r in coverage)
        assert not any(r['onMap'] for r in coverage if r['kind']=='city' and r['nation']=='NO')
        cities=read(DATA/f'cities_neighbours_{year}.json')
        assert all(r.get('onMap') is False for r in cities['outside'] if r.get('owner') in ['SE','NO'])
        assert not any(r.get('district','').startswith(('SE-','NO-')) for r in cities['cities'])
    for path in [ROOT/'Tools/Countries/import_sweden_norway.py',Path(__file__),ROOT/'Tools/Map1851/validate_neighbours.py']:
        ast.parse(path.read_text(encoding='utf-8-sig'))
    # Recreate the initial import against HEAD in isolation; then repeat without changes.
    with tempfile.TemporaryDirectory(prefix='1864-SE-NO-') as directory:
        temporary=Path(directory);target=temporary/'Data/Campaign1851';target.mkdir(parents=True)
        names=['Denmark1851_Map.json','Neighbours_Owners.bin','South_Districts.bin','Economy_South_1825.json','Economy_South_1851.json','Nations_1825.json','Nations1851.json','Trade_1851.json']+[f'{prefix}_{year}.json' for year in [1825,1851] for prefix in ['cities_neighbours','Army_Neighbours']]
        for name in names:(target/name).write_bytes(git_bytes(name))
        command=['python',str(ROOT/'Tools/Countries/import_sweden_norway.py'),'--root',str(temporary),'--package',str(PACKAGE)]
        subprocess.run(command,check=True,capture_output=True)
        prussia=ROOT/'Tools/Countries/import_prussia.py'  # PR-importen koerer efter SE/NO; dens resultat er en del af de aktive filer
        pr_command=['python',str(prussia),'--root',str(temporary)]
        if prussia.exists():subprocess.run(pr_command,check=True,capture_output=True)
        initial=read(temporary/'Docs/Import-Sverige-Norge-rapport.json')
        first={str(p.relative_to(target)):hashlib.sha256(p.read_bytes()).hexdigest() for p in target.rglob('*') if p.is_file()}
        for name,digest in first.items():assert hashlib.sha256((DATA/name).read_bytes()).hexdigest()==digest,(name,'ikke reproducerbar')
        subprocess.run(command,check=True,capture_output=True)
        if prussia.exists():subprocess.run(pr_command,check=True,capture_output=True)
        second={str(p.relative_to(target)):hashlib.sha256(p.read_bytes()).hexdigest() for p in target.rglob('*') if p.is_file()}
        assert first==second,'Importen er ikke idempotent'
        repeated=read(temporary/'Docs/Import-Sverige-Norge-rapport.json')
        assert not repeated['added'] and not repeated['updated']
        # Preserve initial change counts; a second run's zeros are not the initial merge report.
        (ROOT/'Docs/Import-Sverige-Norge-rapport.json').write_bytes((json.dumps(initial,ensure_ascii=False,indent=2)+'\n').encode('utf-8'))
    print(f'OK: {checked} beskyttede rækker byteidentiske; {parsed} JSON; 30 eksporthashes; stabile indeks; reproducerbar og idempotent import.')
    print('Første import:',initial['counts'])
if __name__=='__main__':main()
