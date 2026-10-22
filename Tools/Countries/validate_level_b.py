"""Niveau B: bevar dansk geografi, verificer aktive data og genimport i isolation."""
import ast
import hashlib
import json
import math
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
import numpy as np
from PIL import Image
from import_sweden_norway import ROOT, PACKAGE, read, spans
from import_prussia import PACKAGE as PR_PACKAGE

DATA=ROOT/'Data/Campaign1851'

def head(name):
 return subprocess.check_output(['git','cat-file','--filters','HEAD:Data/Campaign1851/'+name],cwd=ROOT)

def digest_tree(path):
 result={}
 for p in sorted(path.rglob('*')):
  if p.is_file():
   with p.open('rb') as f:result[str(p.relative_to(path))]=hashlib.file_digest(f,'sha256').hexdigest()
 return result

def validate(nation):
 count=0
 for p in DATA.rglob('*.json'):read(p);count+=1
 base=read(DATA/'Denmark1851_Map.json');original=json.loads(head('Denmark1851_Map.json'))
 for key in original:
  if key not in ('extentKm','extent','sizeKm','textureSize'):assert base[key]==original[key],('dansk kortdata',key)
 assert base['extentKm']['yMin']==original['extentKm']['yMin']
 assert base['extentKm']['xMin']==original['extentKm']['xMin'] and base['extentKm']['xMax']==original['extentKm']['xMax']
 extended_in_head='northExtension' in original
 assert math.isclose(base['extentKm']['yMax'],original['extentKm']['yMax'] if extended_in_head else original['extentKm']['yMax']*2-original['extentKm']['yMin'],rel_tol=0,abs_tol=1e-9)
 # Compare image pixels sequentially, not all decoded images at once.
 import io
 for name in ('Denmark1851_Amter.png','Denmark1851_Features.png','Denmark1851_Height.png'):
  with Image.open(io.BytesIO(head(name))) as old_image,Image.open(DATA/name) as new_image:
   assert new_image.size==(old_image.width,old_image.height if extended_in_head else old_image.height*2)
   assert new_image.crop((0,0 if extended_in_head else old_image.height,new_image.width,new_image.height)).tobytes()==old_image.tobytes(),name
 old_owner=np.frombuffer(head('Neighbours_Owners.bin'),dtype='u1')
 owner=np.frombuffer((DATA/'Neighbours_Owners.bin').read_bytes(),dtype='u1')
 assert np.array_equal(owner[0 if extended_in_head else len(old_owner):],old_owner),'det oprindelige ejergrid ændret'
 old_district=np.frombuffer(head('South_Districts.bin'),dtype='<u2')
 district=np.frombuffer((DATA/'South_Districts.bin').read_bytes(),dtype='<u2')
 protected=(old_owner!=2) # SE får nu egne distrikter; hav/DK/tyske distrikter er beskyttede.
 assert np.array_equal(district[0 if extended_in_head else len(old_district):][protected],old_district[protected])
 for year in (1825,1851):
  c=read(DATA/f'cities_neighbours_{year}.json');a=read(DATA/f'Army_Neighbours_{year}.json')
  old_c=json.loads(head(f'cities_neighbours_{year}.json'))
  assert [r['name'] for r in c['cities'][:len(old_c['cities'])]]==[r['name'] for r in old_c['cities']]
  for key in ('cities','districts','links','nations','railways','territories'):
   old_rows=old_c[key];new_rows=c[key]
   for i,row in enumerate(old_rows):
    if key=='cities' and row['owner'] in ('SE','NO'):continue
    assert new_rows[i]==row,(year,key,i,'eksisterende række ændret')
  ids=c['districtGrid']['ids'];assert len(ids)==len(set(ids.values()))
  names={r['name'] for r in c['cities']};edges={n:set() for n in names}
  for e in c['links']:
   assert e['a'] in names or any(r['name']==e['a'] for r in base['cities']+base['foreignCities'])
   assert len(e['km'])>=2 and abs(sum(__import__('math').dist(x,y) for x,y in zip(e['km'],e['km'][1:]))-e['roadKm']-e['ferryKm'])<.002
   edges.setdefault(e['a'],set()).add(e['b']);edges.setdefault(e['b'],set()).add(e['a'])
  reached=set();pending=['Malmø']
  while pending:
   town=pending.pop()
   if town in reached:continue
   reached.add(town);pending.extend(edges.get(town,set())-reached)
  nordic=[r for r in c['cities'] if r['owner'] in ('SE','NO')]
  assert all(r.get('onMap') and r['name'] in reached and r.get('district') in ids for r in nordic)
  assert any(r['owner']=='NO' and 'Oslo' in r['name'] for r in nordic)
  assert not {r['name'] for r in c['outside']}&{r['name'] for r in nordic}
  assert all(r.get('onMap') is not True for r in c['outside'] if r.get('owner') in ('SE','NO'))
  for n in ('SE','NO','PR'):
   fields=[r for r in a['armies'] if r['nation']==n and not r['garrison'] and r['mobilisedMen']>0]
   assert fields and all(r.get('district') in ids and r['mobilisationDays']>0 and r['mobilisationFullDays']>=r['mobilisationDays'] for r in fields)
   assert all(r['town'] in names for r in fields)
   if n=='PR':assert all(r.get('chief') and r.get('officer') and r.get('division') and r.get('corpsName') for r in fields)
  nations=read(DATA/('Nations_1825.json' if year==1825 else 'Nations1851.json'))['nations']
  assert next(r for r in nations if r['id']=='PR')['relation']==-20
  events=read(DATA/('Events_1825.json' if year==1825 else 'Events.json'))
  for event in events:
   if event['id'].startswith('pr_') and not event.get('historik'):
    assert 0<event['sandsynlighed']<1 and event['forudsaetninger']
  print(f'OK {year}: {len(nordic)} SE/NO-byer, {sum(r["nation"] in ("SE","NO") for r in c["districts"])} distrikter; ingen isolerede byer; PR-OOB/officerer/mobilisering; relation -20.')
 # Original research exports stay byte-identical to their manifests.
 se=DATA/'Countries/SwedenNorway';manifest=read(se/'GameReady-manifest.json')
 for name,digest in manifest['hashes'].items():assert hashlib.sha256((se/'GameReady'/name).read_bytes()).hexdigest()==digest,name
 pr=DATA/'Countries/Prussia';manifest_pr=read(pr/'Manifest.json')
 for item in manifest_pr['files']:
  found=list(pr.rglob(Path(item['file']).name));assert found and hashlib.sha256(found[0].read_bytes()).hexdigest()==item['sha256']
 for path in (ROOT/'Tools/Countries').glob('*.py'):ast.parse(path.read_text(encoding='utf-8-sig'))
 # Full import pipeline, twice, in a disk-backed temporary tree. Never mutate the working files.
 with tempfile.TemporaryDirectory(prefix='1864-level-b-') as folder:
  tmp=Path(folder);target=tmp/'Data/Campaign1851';shutil.copytree(DATA,target)
  initial=digest_tree(target)
  commands=[['import_sweden_norway.py','--package',str(PACKAGE)],['import_prussia.py','--package',str(PR_PACKAGE)],['activate_prussia_events.py'],['integrate_level_b.py'],['integrate_level_c.py']]
  for iteration in range(2):
   for command in commands:
    run=subprocess.run([sys.executable,str(ROOT/'Tools/Countries'/command[0]),*command[1:],'--root',str(tmp)],capture_output=True,text=True)
    assert run.returncode==0,(command[0],run.stdout[-1000:],run.stderr[-2000:])
   current=digest_tree(target)
   assert initial==current,('genimport ændrer aktive data',iteration,[k for k,v in current.items() if initial.get(k)!=v][:12])
 print(f'OK {nation}: {count} JSON; danske indeks/geografi/pixels og beskyttede gridceller bevaret; arkivhashes OK; fuld genimport x2 byte-idempotent.')

if __name__=='__main__':validate('SE/NO+PR')
