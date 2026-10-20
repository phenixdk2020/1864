"""Aktive scenariedata. Nye fordelinger, ruter og mobiliseringstider er spilskøn."""
import argparse, copy, heapq, json, math
from pathlib import Path
import numpy as np
from import_sweden_norway import ROOT, read, patch, project

def route(grid, start, end, ids, extent):
    """A* with bounded dense arrays; avoid a Python dictionary per raster cell."""
    height, width = grid.shape
    dx = (extent['xMax'] - extent['xMin']) / width
    dy = (extent['yMax'] - extent['yMin']) / height
    def cell(point):
        return int((extent['yMax'] - point[1]) / dy), int((point[0] - extent['xMin']) / dx)
    origin, goal = cell(start), cell(end)
    origin_id, goal_id = origin[0] * width + origin[1], goal[0] * width + goal[1]
    cost = np.full(height * width, np.inf)
    previous = np.full(height * width, -1, dtype=np.int32)
    closed = np.zeros(height * width, dtype=bool)
    cost[origin_id] = 0.0
    queue = [(0.0, origin_id)]
    while queue:
        _, current_id = heapq.heappop(queue)
        if closed[current_id]:
            continue
        closed[current_id] = True
        current = divmod(current_id, width)
        if current_id == goal_id:
            cells = [goal_id]
            while cells[-1] != origin_id:
                cells.append(int(previous[cells[-1]]))
            cells.reverse()
            return [start] + [[extent['xMin'] + (i % width + .5) * dx,
                              extent['yMax'] - (i // width + .5) * dy] for i in cells[1:-1]] + [end]
        for sy, sx in ((-1,0),(1,0),(0,-1),(0,1),(-1,-1),(-1,1),(1,-1),(1,1)):
            y, x = current[0] + sy, current[1] + sx
            if not (0 <= y < height and 0 <= x < width) or int(grid[y,x]) not in ids:
                continue
            if sy and sx and (int(grid[y,current[1]]) not in ids or int(grid[current[0],x]) not in ids):
                continue
            node_id = y * width + x
            distance = cost[current_id] + math.hypot(sx * dx, sy * dy)
            if not closed[node_id] and distance < cost[node_id]:
                cost[node_id] = distance
                previous[node_id] = current_id
                heapq.heappush(queue, (distance + math.hypot((x-goal[1])*dx, (y-goal[0])*dy), node_id))
    return None

def integrate(root):
 data=root/'Data/Campaign1851';archive=data/'Countries/SwedenNorway';base=read(data/'Denmark1851_Map.json');extent=base['extentKm']
 if 'northExtension' not in base:raise ValueError('Kør build_map.py --extend-north først')
 anchors={r['id']:r for r in read(archive/'Kortkrav.json')['regions']};summary=[]
 for year in (1825,1851):
  path=data/f'cities_neighbours_{year}.json';old=read(path);value=copy.deepcopy(old);overlay=read(archive/f'GameReady/cities_neighbours_{year}.json')
  w,h=value['ownerGrid']['width'],value['ownerGrid']['height'];grid=np.frombuffer((data/'Neighbours_Owners.bin').read_bytes(),dtype=np.uint8).copy().reshape(h,w);ids=value['ownerGrid']['ids']
  def inside(r):
   if r.get('lon') is None or r.get('lat') is None:return False
   x,y=project(r['lon'],r['lat']);return extent['xMin']<=x<extent['xMax'] and extent['yMin']<y<=extent['yMax']
  def cell(r):
   x,y=project(r['lon'],r['lat']);return int((extent['yMax']-y)/(extent['yMax']-extent['yMin'])*h),int((x-extent['xMin'])/(extent['xMax']-extent['xMin'])*w)
  if not any(r['id']=='NO' for r in value['nations']):value['nations'].append(dict(id='NO',name='Norge',colour=[145,65,65],lat=60.2,lon=9.3,note='Spilvalg: kortfarve og labelpunkt. Union med Sverige, separate nationale puljer.'))
  districts=[copy.deepcopy(r) for r in overlay['districts'] if inside(anchors.get(r['id'],{})) and r.get('population',0)]
  points={r['name']:copy.deepcopy(r) for r in overlay['cities']+overlay['outside'] if r.get('owner') in ('SE','NO') and inside(r)}
  for r in points.values():
   if r.get('pop') is None:r['pop']=r.get('population',{}).get(str(year))
   if not r.get('district') or r['district'] not in {d['id'] for d in districts}:
    r['district']=min((d for d in districts if d['nation']==r['owner']),key=lambda d:math.dist(project(r['lon'],r['lat']),project(anchors[d['id']]['lon'],anchors[d['id']]['lat'])))['id']
  for d in districts:
   members=[r for r in points.values() if r['district']==d['id']];unknown=[r for r in members if not r.get('pop')]
   # Skøn: amtets kildebaserede urbane pulje fordeles ligeligt på ukendte bytal.
   remaining=max(len(unknown),d['urban']-sum(r.get('pop') or 0 for r in members))
   for i,r in enumerate(unknown):r.update(pop=remaining//len(unknown)+(i<remaining%len(unknown)),populationStatus='estimated-allocation-of-district-urban-total',populationNote='Skøn, ikke byfolketælling: amtets urbane pulje fordelt ligeligt på punkter uden bytal.')
   if not members:continue
   if d['seat'] not in points:d['seat']=min(members,key=lambda r:math.dist(project(r['lon'],r['lat']),project(anchors[d['id']]['lon'],anchors[d['id']]['lat'])))['name']
   d.update(lon=anchors[d['id']]['lon'],lat=anchors[d['id']]['lat'],boundaryStatus='estimated-nearest-seat-clipped-by-owner',reserveShare=.02,yearlyClassShare=.009,capShare=.09,ruralTax=.16,urbanTax=.36,note='Geometri, skatte- og rekrutteringssatser er spilskøn; befolkning fra landepakken.',towns=[r['name'] for r in members])
  districts=[d for d in districts if d['seat'] in points]
  for d in districts:
   d['sourceStatus']=d['status'];d['status']='estimated';d['urban']=sum(r['pop'] for r in points.values() if r['district']==d['id']);d['rural']=d['population']-d['urban'];d['reserve']=round(d['population']*d['reserveShare']);d['yearlyClass']=round(d['population']*d['yearlyClassShare']);d['taxAnnual']=round(d['urban']*d['urbanTax']+d['rural']*d['ruralTax']);d['treasuryStart']=3*d['taxAnnual']
  garrisons={a['town']:a for n in ('SE','NO') for a in read(archive/f'GameReady/{n}/Army_Neighbours_{year}.json')['armies'] if a.get('garrison')};names={r['name']:i for i,r in enumerate(value['cities'])}
  for r in points.values():
   r['onMap']=True;r.setdefault('railway',False);g=garrisons.get(r['name'],{})
   for field,source in [('garrison','men'),('guns','guns')]:
    if r.get(field) is None:r[field]=g.get(source,0)
   r['militaryNote']='Pakkens cirkamodel; manglende garnison skønnet til ingen fast styrke.'
   if r['name'] in names:value['cities'][names[r['name']]].update({k:v for k,v in r.items() if v is not None})
   else:value['cities'].append(r)
   grid[cell(r)]=ids[r['owner']]
  value['outside']=[r for r in value['outside'] if r['name'] not in points]
  for d in districts:
   index=next((i for i,r in enumerate(value['districts']) if r['id']==d['id']),None)
   if index is None:value['districts'].append(d)
   else:value['districts'][index]=d
  dids=value['districtGrid']['ids']
  for d in value['districts']:dids.setdefault(d['id'],len(dids)+1)
  raster=np.frombuffer((data/'South_Districts.bin').read_bytes(),dtype='<u2').copy().reshape(h,w);yy,xx=np.indices((h,w))
  for n in ('SE','NO'):
   distance=np.full((h,w),np.inf)
   for d in districts:
    if d['nation']!=n:continue
    y,x=cell(d);squared=(yy-y)**2+(xx-x)**2;selected=(grid==ids[n])&(squared<distance);raster[selected]=dids[d['id']];distance[selected]=squared[selected]
  for r in points.values():raster[cell(r)]=dids[r['district']]
  allpoints={r['name']:r for r in value['cities']};connected={'Malmø'}
  def flood():
   while True:
    before=len(connected)
    for e in value['links']:
     if e['a'] in connected or e['b'] in connected:connected.update((e['a'],e['b']))
    if len(connected)==before:break
  flood()
  while set(points)-connected:
   a,b=min(((a,b) for a in sorted(set(points)-connected) for b in sorted(connected) if b in allpoints and allpoints[b]['owner'] in ('SE','NO')),key=lambda p:(math.dist(project(points[p[0]]['lon'],points[p[0]]['lat']),project(allpoints[p[1]]['lon'],allpoints[p[1]]['lat'])),p))
   start,end=project(points[a]['lon'],points[a]['lat']),project(allpoints[b]['lon'],allpoints[b]['lat']);line=route(grid,start,end,{ids['SE'],ids['NO']},extent);ferry=line is None
   if ferry:line=[start,end]
   length=sum(math.dist(x,y) for x,y in zip(line,line[1:]));value['links'].append(dict(a=a,b=b,km=line,roadKm=0 if ferry else round(length,3),ferryKm=round(length,3) if ferry else 0,ferry='Skønnet bådforbindelse' if ferry else '',chaussee=False,status='estimated',note='Spilskøn; A* på landmaske eller eksplicit bådforbindelse, ikke verificeret postrute.'));flood()
  patch(path,old,value);grid.tofile(data/'Neighbours_Owners.bin');raster.tofile(data/'South_Districts.bin')
  path=data/f'Economy_South_{year}.json';old=read(path);new=copy.deepcopy(old);new['districts']=value['districts'];patch(path,old,new)
  nation_path=data/('Nations_1825.json' if year==1825 else 'Nations1851.json');nation_old=read(nation_path);nation_new=copy.deepcopy(nation_old)
  next(r for r in nation_new['nations'] if r['id']=='PR')['relation']=-20
  patch(nation_path,nation_old,nation_new)
  path=data/f'Army_Neighbours_{year}.json';old=read(path);army=copy.deepcopy(old);army_ids={a['id'] for a in army['armies']}
  for n in ('SE','NO'):
   for r in read(archive/f'GameReady/{n}/Army_Neighbours_{year}.json')['armies']:
    if r['id']=='SE-field':continue
    if r['town'] in points and r['id'] not in army_ids:army['armies'].append(copy.deepcopy(r));army_ids.add(r['id'])
  for r in points.values():
   gid='garrison-'+r['name']
   if gid not in army_ids:army['armies'].append(dict(id=gid,name='Garnison i '+r['name'],nation=r['owner'],town=r['name'],garrison=True,men=r['garrison'],mobilisedMen=r['garrison'],guns=r['guns'],note='Spilsk?n fra byens garnisonsmodel.'));army_ids.add(gid)
  officerfile='Officers_1825.json' if year==1825 else 'Officers1851.json';officers=read(data/'Countries/Prussia/GameReady'/officerfile)['generals']
  for r in army['armies']:
   if r['nation'] in ('SE','NO') and r['town'] in points:r['district']=points[r['town']]['district']
   if r['nation'] not in ('SE','NO','PR'):continue
   r['mobilisationDays']=0 if r['garrison'] else 7 if r['nation']=='PR' else 14;r['mobilisationFullDays']=0 if r['garrison'] else 42 if year==1825 else 28;r['mobilisationNote']='Spilskøn: samling, reserveindkaldelse, fuld feltstyrke; nationale puljer adskilt.'
   if r['nation']=='PR' and not r['garrison']:r.update(officer=copy.deepcopy(officers[0]),officerMarchFactor=.9+float(officers[0]['staff'])*.02,chief=officers[0]['name'],chiefStatus='estimated-field-assignment',division='3. Division' if r['id'] in ('PR-IR2','PR-IR9') else '4. Division',corpsName='II. Armeekorps (pommersk feltdetachement)',organisationNote='Perioderamme fra Prussia/GameReady/Army; feltfordeling og chef er spilskøn.')
  patch(path,old,army);summary.append(dict(scenario=str(year),cities=len(points),districts=len(districts),unreachable=sorted(set(points)-connected)))
 report=data/'Countries/LevelB.json';content=(json.dumps(dict(note='Statisk integration; ingen build eller spiltest.',union=dict(members=['SE','NO'],sharedForeignPolicy=True,separateArmies=True,separateManpower=True,separateTreasuries=True,source='SwedenNorway/Union-regler.json'),scenarios=summary),ensure_ascii=False,indent=2)+'\n').encode()
 if not report.exists() or report.read_bytes()!=content:report.write_bytes(content)
 print(json.dumps(summary,ensure_ascii=False))

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=ROOT);integrate(p.parse_args().root)
