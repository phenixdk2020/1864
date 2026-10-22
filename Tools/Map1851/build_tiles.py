"""Byg lokale LAEA-fliser og scenarieoverlays uden Unreal eller netadgang."""
import argparse
import copy
import hashlib
import json
import math
import sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, PngImagePlugin
from scipy.spatial import cKDTree
from build_map import project, unproject, Raster
from shp import iter_read
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Tools/Countries'))
from import_map_tiles import import_research, NOTE
from integrate_level_b import route

TILE=1024
LAYERS={'Color':('Reference/Campaign1851/Denmark1851_Color.png',1),
 'Height':('Data/Campaign1851/Denmark1851_Height.png',1),
 'Features':('Data/Campaign1851/Denmark1851_Features.png',2),
 'Amter':('Data/Campaign1851/Denmark1851_Amter.png',2),
 'DetailMask':('Reference/Campaign1851/Denmark1851_DetailMask.png',2)}
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes((json.dumps(value,ensure_ascii=False,indent=2,allow_nan=False)+'\r\n').replace('\r\n','\n').replace('\n','\r\n').encode('utf-8'))
def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()
def geometry(root,kind):
    result=[]
    path=root/'SourceAssets/NaturalEarth'/kind/kind
    for parts,record in iter_read(str(path)):
        rings=[]
        for ring in parts:
            a=np.asarray(ring)
            if len(a)<3 or a[:,0].max()<3 or a[:,0].min()>34 or a[:,1].max()<47 or a[:,1].min()>73:continue
            x,y=project(a[:,0],a[:,1]);points=np.column_stack((x,y))
            area=np.sum(a[:,0]*np.roll(a[:,1],1)-np.roll(a[:,0],1)*a[:,1])
            rings.append((points,(x.min(),x.max(),y.min(),y.max()),area>=0))
        if rings:result.append((rings,record))
    return result

def mask(raster,shapes):
    image=Image.new('L',(raster.w,raster.h));draw=ImageDraw.Draw(image)
    for points,b,outer in shapes:
        if b[1]<raster.x_min or b[0]>raster.x_max or b[3]<raster.y_min or b[2]>raster.y_max:continue
        px=(points[:,0]-raster.x_min)/(raster.x_max-raster.x_min)*raster.w
        py=(raster.y_max-points[:,1])/(raster.y_max-raster.y_min)*raster.h
        draw.polygon(list(zip(px,py)),fill=255 if outer else 0)
    return np.asarray(image)>0

def layout(root):
    meta=read(root/'Data/Campaign1851/Denmark1851_Map.json');old=meta['extentKm'];south=meta['northExtension']['originalExtentKm']
    with Image.open(root/LAYERS['Color'][0]) as im:w,h=im.size
    dx=(south['xMax']-south['xMin'])/w;dy=(south['yMax']-south['yMin'])/h
    # Dense perimeter sampling includes the curved south-edge minimum near lon 10.
    lon=np.linspace(4.5,31.5,27001);lat=np.linspace(49,71.3,22301)
    x,y=project(np.concatenate((lon,lon,np.full_like(lat,4.5),np.full_like(lat,31.5))),np.concatenate((np.full_like(lon,49),np.full_like(lon,71.3),lat,lat)))
    left=math.floor((min(x.min(),old['xMin'])-south['xMin'])/(TILE*dx))
    right=math.ceil((max(x.max(),old['xMax'])-south['xMin'])/(TILE*dx))
    top=math.floor((south['yMax']-max(y.max(),old['yMax']))/(TILE*dy))
    bottom=math.ceil((south['yMax']-min(y.min(),old['yMin']))/(TILE*dy))
    extent=dict(xMin=south['xMin']+left*TILE*dx,xMax=south['xMin']+right*TILE*dx,yMin=south['yMax']-bottom*TILE*dy,yMax=south['yMax']-top*TILE*dy)
    return meta,south,dx,dy,left,right,top,bottom,extent

def relief(raster,land,etopo):
    xx,yy=raster.xy_grids();lon,lat=unproject(xx,yy)
    from paint_tiles_v2 import estimated_meters
    meters=estimated_meters(xx,yy,lon,lat)
    if etopo:
        from etopo_pillow import EtopoPillow
        reader=etopo if hasattr(etopo,'sample') else EtopoPillow(etopo)
        meters,_=reader.sample(lon,lat)
    # Nye områder: 0–3000 m; eksisterende pixels har uændret visuel skala.
    from etopo_pillow import encode_height
    height=encode_height(meters,land)
    return height,meters,xx,yy

def paint_color(raster,land,meters):
    """Verdenskoordinatbaseret farve/skov med den oprindelige kortpalet."""
    from scipy import ndimage
    xx,yy=raster.xy_grids()
    def noise(scale,seed):
        x=xx/scale;y=yy/scale;ix=np.floor(x);iy=np.floor(y);fx=x-ix;fy=y-iy
        fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
        def hashed(a,b):
            v=np.sin(a*127.1+b*311.7+seed*74.7)*43758.5453
            return v-np.floor(v)
        return (hashed(ix,iy)*(1-fx)+hashed(ix+1,iy)*fx)*(1-fy)+(hashed(ix,iy+1)*(1-fx)+hashed(ix+1,iy+1)*fx)*fy
    broad=noise(22,3);woods=noise(5,5)*.65+noise(1.3,9)*.35
    forest=np.clip((woods-.53)*5,0,1)*land
    patch=noise(.55,4)
    colour=np.stack((84+36*broad+15*patch,112+14*broad+10*patch,48+14*broad+10*patch),axis=-1)
    colour=colour*(1-forest[...,None]*.88)+np.array([38,62,32])*forest[...,None]*.88
    gy,gx=np.gradient(meters);shade=np.clip(.90+(-gx+gy)*.012,.65,1.2)
    colour*=shade[...,None]
    grey=colour.mean(axis=2,keepdims=True)
    colour=(colour*.3+grey*.7)*.72+np.array([9,8,5])
    if land.any() and (~land).any():
        coast=ndimage.distance_transform_edt(~land)*raster.km_per_px
        inland=ndimage.distance_transform_edt(land)*raster.km_per_px
    else:coast=np.full(land.shape,100.);inland=np.full(land.shape,100.)
    shore=np.clip(1-inland/.4,0,1)*land
    colour=colour*(1-shore[...,None]*.5)+np.array([160,155,125])*shore[...,None]*.5
    depth=np.clip(coast/10,0,1)**.7
    sea=np.array([52,92,104])*(1-depth[...,None])+np.array([18,38,62])*depth[...,None]
    sea*=.94+.12*noise(9,7)[...,None]
    return np.clip(np.where(land[...,None],colour,sea),0,255).astype(np.uint8),forest

def paste_legacy(image,source,gx,gy,ratio):
    # Grid coordinates are relative to original southern raster's north-west edge.
    offset_y=-4096//ratio if source.height==8192//ratio else 0
    sx=gx*TILE//ratio;sy=gy*TILE//ratio-offset_y
    left=max(0,sx);top=max(4096//ratio if source.height==8192//ratio else 0,sy);right=min(source.width,sx+image.width);bottom=min(source.height,sy+image.height)
    if left<right and top<bottom:image.paste(source.crop((left,top,right,bottom)),(left-sx,top-sy))

def owners(raster,countries,ids,year):
    grid=np.zeros((raster.h,raster.w),np.uint8)
    # Period German states: coarse nearest-capital correction inside modern Germany.
    anchors=[('PR',13.4,52.52),('PR',14.55,53.43),('PR',11.63,52.13),('PR',7.63,51.96),('PR',6.96,50.94),('PR',6.64,49.76),('PR',11.03,50.98),('HAN',9.73,52.37),('HAN',7.48,53.37),('MEC',11.41,53.63),('MST',13.07,53.36),('OLD',8.21,53.14),('BRU',10.52,52.27),('SAX',13.74,51.05),('SWEI',11.33,50.98),('HK',9.48,51.31),('HD',8.65,49.87),('NAS',8.24,50.08),('BAV',11.58,48.14)]
    points=np.array([project(lon,lat) for _,lon,lat in anchors]);values=np.array([ids[n] for n,_,_ in anchors],np.uint8)
    xx,yy=raster.xy_grids()
    for rings,rec in countries:
        code=rec.get('ADM0_A3');nation={'DNK':'DK','SWE':'SE','NOR':'NO','FIN':'FIN','RUS':'RU','EST':'RU','LVA':'RU','LTU':'RU','NLD':'NL','BEL':'BE','LUX':'LU','CZE':'AT','AUT':'AT','FRA':'FR','CHE':'CH','GBR':'GB'}.get(code)
        if not nation and code not in ('DEU','POL'):continue
        selected=mask(raster,rings)
        if code=='DEU':
            _,ix=cKDTree(points).query(np.column_stack((xx[selected],yy[selected])));grid[selected]=values[ix]
        elif code=='POL':
            lon,lat=unproject(xx,yy)
            # Duchy of Posen, Pomerania, Silesia, East/West Prussia; cheap historical envelope.
            pr=(lon<18.5)|((lat>53.15)&(lon<22.8))|((lat<51.35)&(lon<18.8))
            grid[selected&pr]=ids['PR'];grid[selected&~pr]=ids['POL']
        else:
            grid[selected]=ids[nation]
            if code in ('LTU','RUS'):
                lon,lat=unproject(xx,yy)
                pr=((lon<22.85)&(lon>19)&(lat<55.4)&(lat>54.2))|((lon<21.55)&(lon>20.5)&(lat<55.95)&(lat>55.2))
                grid[selected&pr]=ids['PR']
    return grid

def grids(root,extent,countries,meta):
    data=root/'Data/Campaign1851';oldext=meta['extentKm'];old=read(data/'cities_neighbours_1825.json')
    ids=copy.deepcopy(old['ownerGrid']['ids'])
    for nation in ['BRU','NL','BE','LU','SAX','SWEI','HK','HD','NAS','BAV','AT','POL','FIN','RU','FR','CH','GB','FOREIGN']:
        ids.setdefault(nation,max(ids.values())+1)
    width=math.ceil((extent['xMax']-extent['xMin'])/.71);height=math.ceil((extent['yMax']-extent['yMin'])/.71)
    # Exact .71 cells; grid extent may extend less than one cell beyond tile extent.
    gext=dict(extent,xMax=extent['xMin']+width*.71,yMin=extent['yMax']-height*.71)
    raster=Raster(gext['xMin'],gext['xMax'],gext['yMin'],gext['yMax'],width,height)
    print('Gitre',width,height,flush=True)
    base=owners(raster,countries,ids,1825)
    legacy=np.fromfile(data/'Neighbours_Owners.bin',np.uint8).reshape(old['ownerGrid']['height'],old['ownerGrid']['width'])
    legacy_d=np.fromfile(data/'South_Districts.bin','<u2').reshape(legacy.shape)
    xs=gext['xMin']+(np.arange(width)+.5)*.71;ys=gext['yMax']-(np.arange(height)+.5)*.71
    ix=np.floor((xs-oldext['xMin'])/(oldext['xMax']-oldext['xMin'])*legacy.shape[1]).astype(int)
    iy=np.floor((oldext['yMax']-ys)/(oldext['yMax']-oldext['yMin'])*legacy.shape[0]).astype(int)
    validx=(ix>=0)&(ix<legacy.shape[1]);validy=(iy>=0)&(iy<legacy.shape[0]);oldsel=np.ix_(validy,validx)
    base[oldsel]=legacy[np.ix_(iy[validy],ix[validx])]
    outputs={}
    for year in (1825,1851):
        original=read(data/f'cities_neighbours_{year}.json');value=import_research(root,year,original,project);grid=base.copy()
        # Hohenzollern: small estimated envelope; independent in 1825, Prussian in 1851.
        for y0 in range(max(0,height-256),height,128):
            lon,lat=unproject(xs[None,:],ys[y0:y0+128,None])
            hohenzollern=((lon-9.1)/.25)**2+((lat-48.24)/.3)**2<=1
            grid[y0:y0+128][hohenzollern]=ids['PR'] if year==1851 else ids['FOREIGN']
        dids=copy.deepcopy(original['districtGrid']['ids'])
        for d in value['districts']:dids.setdefault(d['id'],max(dids.values(),default=0)+1)
        dg=np.zeros((height,width),'<u2')
        # Bounded row blocks for nearest-seat allocation; no giant coordinate stack.
        candidates=value['researchDistricts']+[d for d in value['districts'] if d['nation'] not in ('SE','NO','PR')]
        for nation,oid in ids.items():
            seats=[d for d in candidates if d['nation']==nation and 'lon' in d and 'lat' in d and d['id']!='SE-stockholm-city']
            if not seats:continue
            tree=cKDTree([project(d['lon'],d['lat']) for d in seats]);numbers=np.array([dids[d['id']] for d in seats],np.uint16)
            for y0 in range(0,height,128):
                sel=grid[y0:y0+128]==oid;yy,xx=np.where(sel)
                if len(xx):
                    _,index=tree.query(np.column_stack((xs[xx],ys[y0+yy])));dg[y0+yy,xx]=numbers[index]
        # Stockholm stad and län share a seat: estimated 3 km urban core, län outside.
        stad=next(d for d in value['researchDistricts'] if d['id']=='SE-stockholm-city')
        stadx,stady=project(stad['lon'],stad['lat'])
        for y0 in range(0,height,128):
            citycore=(xs[None,:]-stadx)**2+(ys[y0:y0+128,None]-stady)**2<=9
            selection=citycore&(grid[y0:y0+128]==ids['SE'])
            dg[y0:y0+128][selection]=dids['SE-stockholm-city']
        # Preserve legacy district assignments exactly at their corresponding world cells.
        dg[oldsel]=legacy_d[np.ix_(iy[validy],ix[validx])]
        for c in value['cities']:
            if 'lon' not in c or 'lat' not in c:continue
            x,y=project(c['lon'],c['lat']);c['onMap']=bool(gext['xMin']<=x<gext['xMax'] and gext['yMin']<y<=gext['yMax'])
            if c['name'] in value['tileAddedCities'] and c['onMap']:
                cy,cx=int((gext['yMax']-y)/.71),int((x-gext['xMin'])/.71)
                grid[cy,cx]=ids[c['owner']];dg[cy,cx]=dids[c['district']]
        value['ownerGridFull']=dict(file=f'Neighbours_Owners_Full_{year}.bin',width=width,height=height,ids=ids,extentKm=gext,cellKm=.71,dtype='uint8',rowOrder='north-to-south',boundaryStatus='Natural Earth 10m; tyske stater nærmeste hovedstad, polsk/preussisk grænse groft skøn; ældre udsnit bevaret')
        value['districtGridFull']=dict(file=f'Districts_Full_{year}.bin',width=width,height=height,ids=dids,extentKm=gext,cellKm=.71,dtype='uint16-le',rowOrder='north-to-south',zeroMeaning='Hav eller dansk amt/groft fremmed område uden udenlandsk distrikt')
        grid.tofile(data/value['ownerGridFull']['file']);dg.tofile(data/value['districtGridFull']['file'])
        # Extend only new city connections; all old route coordinates remain untouched.
        points={c['name']:c for c in value['cities'] if c.get('onMap')}
        linked=set(points)-set(value['tileAddedCities'])
        for name in sorted(value['tileAddedCities']):
            c=points.get(name)
            if not c:continue
            choices=[n for n in linked if points[n].get('owner')==c['owner']]
            if not choices:linked.add(name);continue
            start=project(c['lon'],c['lat']);other=min(choices,key=lambda n:math.dist(start,project(points[n]['lon'],points[n]['lat'])))
            end=project(points[other]['lon'],points[other]['lat'])
            # Existing A* algorithm on a bounded corridor; avoid dense arrays for the whole map per route.
            cells=[(int((gext['yMax']-p[1])/.71),int((p[0]-gext['xMin'])/.71)) for p in (start,end)]
            y0=max(0,min(p[0] for p in cells)-48);y1=min(height,max(p[0] for p in cells)+49)
            x0=max(0,min(p[1] for p in cells)-48);x1=min(width,max(p[1] for p in cells)+49)
            sub=grid[y0:y1,x0:x1];ex=dict(xMin=gext['xMin']+x0*.71,xMax=gext['xMin']+x1*.71,yMax=gext['yMax']-y0*.71,yMin=gext['yMax']-y1*.71)
            allowed={ids['SE'],ids['NO']} if c['owner'] in ('SE','NO') else {ids[n] for n in ('PR','HAN','MEC','MST','BRU','HK','HD','NAS','SAX','SWEI','OLD','BAV')}
            line=route(sub,start,end,allowed,ex);ferry=line is None;line=line or [start,end]
            transit=set()
            if not ferry:
                for px,py in line:
                    number=int(grid[min(height-1,max(0,int((gext['yMax']-py)/.71))),min(width-1,max(0,int((px-gext['xMin'])/.71)))])
                    transit.update(n for n,oid in ids.items() if oid==number and n!=c['owner'])
            length=sum(math.dist(a,b) for a,b in zip(line,line[1:]))
            value['links'].append(dict(a=name,b=other,km=[[float(x),float(y)] for x,y in line],roadKm=0 if ferry else round(length,3),ferryKm=round(length,3) if ferry else 0,ferry='Skønnet bådforbindelse' if ferry else '',transitOwners=sorted(transit),requiresForeignTransit=bool(transit),status='estimated',note='Skøn: A* i afgrænset landkorridor; båd hvis ingen landrute findes. Ikke verificeret postrute.'))
            linked.add(name)
        path=data/f'cities_neighbours_full_{year}.json';write(path,value)
        outputs[str(year)]={'file':str(path.relative_to(root)).replace('\\','/'),'ownerGrid':value['ownerGridFull'],'districtGrid':value['districtGridFull']}
        print('Scenarie',year,'nye byer',len(value['tileAddedCities']),'researchdistrikter',len(value['researchDistricts']),flush=True)
    return outputs

def build(root,finish_data_only=False):
    data=root/'Data/Campaign1851';out=root/'Reference/Campaign1851/MapTiles';out.mkdir(parents=True,exist_ok=True)
    meta,south,dx,dy,left,right,top,bottom,extent=layout(root)
    land_shapes=geometry(root,'ne_10m_land');countries=geometry(root,'ne_10m_admin_0_countries')
    sources={k:Image.open(root/p) for k,(p,_) in LAYERS.items()}
    # Decode legacy rasters once (~130 MB); new raster allocations are strictly tile-sized.
    for im in sources.values():im.load()
    etopo=list((root/'SourceAssets/ETOPO2022').glob('*.tif'))
    manifest=dict(version=1,projection=meta['projection'],tileSizePx=TILE,kmPerPixel={'x':dx,'y':dy},originKm={'x':south['xMin'],'y':south['yMax']},rowOrder='north-to-south',gridRange={'xMin':left,'xMaxExclusive':right,'yMin':top,'yMaxExclusive':bottom},extentKm=extent,geographicRequest={'lonMin':4.5,'lonMax':31.5,'latMin':49,'latMax':71.3},layerScale={k:1/r for k,(_,r) in LAYERS.items()},heightEncoding={'type':'uint16','seaLevelNormalized':0,'newAreaMetersPerNormalizedUnit':3000,'note':'Eksisterende pixels er visuelt relief uden verificeret meterskala; nye områder 0–3000 m.'},reliefStatus='ETOPO2022 + oprindeligt dansk relief' if etopo else 'skøn: analytisk Scandes/Sudeter-relief, ingen målte højdedata',tiles=[],legacySources={})
    for k,(p,r) in LAYERS.items():
        im=sources[k];manifest['legacySources'][k]=dict(file=p,sha256=digest(root/p),size=list(im.size),mode=im.mode,ratio=r,offsetPixels=[0,-4096//r if im.height==8192//r else 0],preserveRect=[0,4096//r if im.height==8192//r else 0,im.width,im.height])
    for gy in range(top,bottom):
        for gx in range(left,right):
            bounds=dict(xMin=south['xMin']+gx*TILE*dx,xMax=south['xMin']+(gx+1)*TILE*dx,yMax=south['yMax']-gy*TILE*dy,yMin=south['yMax']-(gy+1)*TILE*dy)
            if finish_data_only:
                paths={name:out/f'Tile_{gx}_{gy}_{name}.png' for name in LAYERS}
                if not paths['Color'].exists():continue
                tile={'x':gx,'y':gy,'boundsKm':bounds,'layers':{},'resumed':True}
                for name,path in paths.items():
                    with Image.open(path) as image:size=list(image.size)
                    tile['layers'][name]=dict(file=str(path.relative_to(root)).replace('\\','/'),size=size,sha256=digest(path))
                manifest['tiles'].append(tile)
                continue
            raster=Raster(bounds['xMin'],bounds['xMax'],bounds['yMin'],bounds['yMax'],TILE,TILE)
            land=mask(raster,[ring for rings,_ in land_shapes for ring in rings])
            legacy=0<=gx*TILE<3996 and gy*TILE<4096 and (gy+1)*TILE>-4096
            if not land.any() and not legacy:continue
            height,meters,xx,yy=relief(raster,land,etopo)
            color,forest=paint_color(raster,land,meters)
            features=np.stack((land.astype(np.uint8)*255,(forest*255).astype(np.uint8),np.zeros_like(land,np.uint8)),axis=-1)
            layers={'Color':Image.fromarray(color),'Height':Image.fromarray(height),'Features':Image.fromarray(features).resize((512,512),Image.Resampling.NEAREST),'Amter':Image.new('L',(512,512)),'DetailMask':Image.new('RGBA',(512,512))}
            tile={'x':gx,'y':gy,'boundsKm':bounds,'layers':{},'landPixels':int(land.sum()),'legacyCoverage':legacy}
            for name,image in layers.items():
                ratio=LAYERS[name][1];paste_legacy(image,sources[name],gx,gy,ratio)
                path=out/f'Tile_{gx}_{gy}_{name}.png';info=PngImagePlugin.PngInfo()
                if name=='Color':info.add(b'sRGB',b'\x00')
                image.save(path,pnginfo=info,compress_level=6)
                tile['layers'][name]=dict(file=str(path.relative_to(root)).replace('\\','/'),size=list(image.size),sha256=digest(path))
            manifest['tiles'].append(tile)
        print('Fliserække',gy,'antal',len(manifest['tiles']),flush=True)
    # Overview composed in bounded tile-sized chunks; only the ~1 km overview is resident.
    ow=math.ceil((extent['xMax']-extent['xMin']));oh=math.ceil((extent['yMax']-extent['yMin']))
    overview=Image.new('RGB',(ow,oh),(18,38,62));overview_h=Image.new('I;16',(ow,oh))
    for tile in manifest['tiles']:
        b=tile['boundsKm'];x0=round((b['xMin']-extent['xMin'])/(extent['xMax']-extent['xMin'])*ow);x1=round((b['xMax']-extent['xMin'])/(extent['xMax']-extent['xMin'])*ow);y0=round((extent['yMax']-b['yMax'])/(extent['yMax']-extent['yMin'])*oh);y1=round((extent['yMax']-b['yMin'])/(extent['yMax']-extent['yMin'])*oh)
        for name,target in [('Color',overview),('Height',overview_h)]:
            with Image.open(root/tile['layers'][name]['file']) as im:target.paste(im.resize((x1-x0,y1-y0),Image.Resampling.NEAREST),(x0,y0))
    overview.save(out/'Overview_Color.png');overview_h.save(out/'Overview_Height.png')
    manifest['overview']=dict(color='Reference/Campaign1851/MapTiles/Overview_Color.png',height='Reference/Campaign1851/MapTiles/Overview_Height.png',size=[ow,oh],extentKm=extent)
    manifest['scenarios']=grids(root,extent,countries,meta)
    write(data/'MapTiles.json',manifest)
    # Add only opt-in keys to current scenario files; legacy loader fields stay exact.
    from import_sweden_norway import patch
    for year in (1825,1851):
        path=data/f'cities_neighbours_{year}.json';before=read(path);after=copy.deepcopy(before)
        after.update(mapTiles='MapTiles.json',fullScenario=f'cities_neighbours_full_{year}.json',ownerGridFull=manifest['scenarios'][str(year)]['ownerGrid'],districtGridFull=manifest['scenarios'][str(year)]['districtGrid'])
        patch(path,before,after)
    print('Færdig',len(manifest['tiles']),'fliser',flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',type=Path,default=ROOT);parser.add_argument('--finish-data-only',action='store_true',help='Genbrug færdige fliser; kræver efterfølgende fuld dækningsvalidering.');args=parser.parse_args();build(args.root,args.finish_data_only)
    from finalize_tiles import finalize
    finalize(args.root)
    from paint_tiles_v2 import repaint
    repaint(args.root)
