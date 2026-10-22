"""Fliseadapter til den oprindelige maler. Koordinatfast støj, halo og relief."""
import math
import numpy as np
from scipy import ndimage
from scipy.spatial import cKDTree
from PIL import Image, ImageDraw
from build_map import paint, project, unproject, Raster, REGION_KINGDOM
from build_tiles import geometry, mask, relief, paste_legacy, LAYERS, TILE, read, write, digest, ROOT

HALO=128
SEED=1864

def hashed(x,y,seed):
    value=np.sin(x*127.1+y*311.7+seed*74.7)*43758.5453
    return (value-np.floor(value)).astype(np.float32)

class WorldNoise:
    """Samme fysiske støjskala som sydkortets 4096 rækker, uden fliselokale frø."""
    def __init__(self,raster,reference_km=558.2832617202233,reference_pixels=4096):
        self.r=raster;self.reference_km=reference_km;self.reference_pixels=reference_pixels
        self.x,self.y=raster.xy_grids()
        self.x=self.x.astype(np.float32);self.y=self.y.astype(np.float32)
    def fbm(self,shape,scale,octaves,seed,persistence=.5):
        # Støjens mindste bølgelængde er ca. 11 originalpixels. Sample på et
        # globalt justeret 2-pixelgitter; genopskalér før den uændrede maler.
        step=2 if self.r.km_per_px<.3 else 1
        columns=np.arange(math.ceil((shape[1]-1)/step)+1)*step+.5
        rows=np.arange(math.ceil((shape[0]-1)/step)+1)*step+.5
        xs=(self.r.x_min+columns*(self.r.x_max-self.r.x_min)/self.r.w).astype(np.float32)
        ys=(self.r.y_max-rows*(self.r.y_max-self.r.y_min)/self.r.h).astype(np.float32)
        xbase,ybase=np.meshgrid(xs,ys)
        total=np.zeros(xbase.shape,np.float32);amp=1.;norm=0.
        for octave in range(octaves):
            spacing=self.reference_km/(scale*2**octave)
            x=xbase/spacing;y=ybase/spacing
            xmin=math.floor(float(x.min()))-3;ymin=math.floor(float(y.min()))-3
            xmax=math.ceil(float(x.max()))+4;ymax=math.ceil(float(y.max()))+4
            gx,gy=np.meshgrid(np.arange(xmin,xmax),np.arange(ymin,ymax))
            grid=hashed(gx,gy,seed+octave*19)
            sample=ndimage.map_coordinates(grid,[y-ymin,x-xmin],order=3,prefilter=False,mode='nearest')
            total+=sample*amp;norm+=amp;amp*=persistence
        rows,cols=np.indices(shape,dtype=np.float32)
        result=ndimage.map_coordinates(total/norm,[rows/step,cols/step],order=1,prefilter=False,mode='nearest')
        return np.clip(.5+(result-.5)*2.2,0,1)
    def worley(self,shape,spacing_px,seed):
        spacing=spacing_px*self.reference_km/self.reference_pixels
        x=self.x/spacing;y=self.y/spacing
        xmin=math.floor(float(x.min()))-3;ymin=math.floor(float(y.min()))-3
        xmax=math.ceil(float(x.max()))+4;ymax=math.ceil(float(y.max()))+4
        gx,gy=np.meshgrid(np.arange(xmin,xmax),np.arange(ymin,ymax))
        points=np.column_stack(((gx+hashed(gx,gy,seed)).ravel(),(gy+hashed(gx,gy,seed+1)).ravel()))
        values=hashed(gx,gy,seed+2).ravel()
        _,index=cKDTree(points).query(np.column_stack((x.ravel(),y.ravel())),workers=1)
        return values[index].reshape(shape)

class JoinedNoise:
    """Ved originalkanten bruges verdensarkets grovere støjskala; fuld detalje nås over 50 km."""
    def __init__(self,raster,meta):
        self.high=WorldNoise(raster);self.low=WorldNoise(raster,3000,3072)
        x,y=self.high.x,self.high.y;o=meta['originKm'];dx,dy=meta['kmPerPixel']['x'],meta['kmPerPixel']['y']
        sx=np.maximum(np.maximum(o['x']-x,x-(o['x']+3996*dx)),0)
        sy=np.maximum(np.maximum(y-o['y'],o['y']-4096*dy-y),0)
        self.weight=np.clip(1-np.hypot(sx,sy)/50,0,1)
        self.weight=self.weight*self.weight*(3-2*self.weight)
    def fbm(self,shape,scale,octaves,seed,persistence=.5):
        a=self.high.fbm(shape,scale,octaves,seed,persistence)
        b=self.low.fbm(shape,scale,octaves,seed+6,persistence)
        return a*(1-self.weight)+b*self.weight
    def worley(self,shape,spacing_px,seed):
        a=self.high.worley(shape,spacing_px,seed);b=self.low.worley(shape,spacing_px,seed+6)
        return a*(1-self.weight)+b*self.weight

def near_legacy(tile,meta):
    o=meta['originKm'];dx,dy=meta['kmPerPixel']['x'],meta['kmPerPixel']['y'];b=tile['boundsKm']
    x=max(o['x']-(b['xMax']+HALO*dx),b['xMin']-HALO*dx-(o['x']+3996*dx),0)
    y=max(o['y']-4096*dy-(b['yMax']+HALO*dy),b['yMin']-HALO*dy-o['y'],0)
    return math.hypot(x,y)<50


def estimated_meters(xx,yy,lon,lat):
    """Visuelle skøn, ikke DEM: bjergkæder og særskilte tyske højlande."""
    noise=(np.sin(xx*.041+np.sin(yy*.026))+np.cos(yy*.035-xx*.019))*.18+1
    base=18+18*(np.sin(xx*.008)*np.cos(yy*.009)+1)
    mountains=np.zeros_like(xx)
    chain=[(7.4,59.1,1100),(8.2,61.2,2050),(10.6,62.5,1700),(12.7,64.2,1300),(15.5,66.6,1600),(18.4,68.4,1700),(22.0,69.6,850)]
    for a,b in zip(chain,chain[1:]):
        ax,ay=project(*a[:2]);bx,by=project(*b[:2]);vx,vy=bx-ax,by-ay
        t=np.clip(((xx-ax)*vx+(yy-ay)*vy)/(vx*vx+vy*vy),0,1)
        dist=(xx-ax-t*vx)**2+(yy-ay-t*vy)**2
        mountains=np.maximum(mountains,(a[2]+(b[2]-a[2])*t)*np.exp(-dist/(2*48**2)))
    for lon0,lat0,peak,sx,sy,angle in [(10.6,51.8,1080,27,16,0),(16.,50.7,1450,115,22,-.18),(6.4,50.3,650,42,32,0),(8.4,51.2,740,45,30,0)]:
        cx,cy=project(lon0,lat0);u=(xx-cx)*math.cos(angle)+(yy-cy)*math.sin(angle);v=-(xx-cx)*math.sin(angle)+(yy-cy)*math.cos(angle)
        mountains=np.maximum(mountains,peak*np.exp(-.5*((u/sx)**2+(v/sy)**2)))
    return base+mountains*noise

# Kun fallback-geometri; lokale Natural Earth-hydrolag har prioritet.
LAKES=[
 ('Vänern',[(12.35,58.4),(12.5,59.1),(13.1,59.4),(13.65,59.1),(13.55,58.55),(12.8,58.35)]),
 ('Vättern',[(14.12,57.8),(14.25,58.55),(14.8,58.9),(15.05,58.78),(14.65,58.18),(14.35,57.78)]),
 ('Mälaren',[(15.8,59.38),(16.45,59.6),(17.4,59.65),(17.95,59.35),(17.15,59.25),(16.5,59.4)]),
 ('Hjälmaren',[(15.3,59.18),(15.55,59.33),(16.15,59.28),(16.1,59.12),(15.5,59.05)]),
 ('Siljan',[(14.55,60.65),(14.55,60.95),(14.9,61.05),(15.1,60.8),(14.9,60.68)]),
 ('Storsjön',[(14.2,62.95),(14.25,63.35),(14.6,63.3),(14.7,63.1),(14.45,63.0)]),
 ('Mjøsa',[(10.45,60.45),(10.42,60.8),(10.38,61.12),(10.5,61.1),(10.75,60.8),(11.1,60.6),(10.9,60.48)]),
 ('Randsfjorden',[(10.18,60.24),(10.1,60.75),(10.25,60.85),(10.3,60.5),(10.3,60.25)]),
 ('Tyrifjorden',[(10.1,59.93),(10.,60.12),(10.25,60.18),(10.43,60.05),(10.25,59.92)]),
 ('Päijänne',[(25.25,61.2),(25.35,61.85),(25.65,62.2),(25.85,62.12),(25.7,61.55),(25.55,61.2)]),
 ('Inari',[(27.3,68.65),(27.3,69.15),(28.3,69.3),(28.6,69.05),(28.,68.65)]),
 ('Saimaa',[(27.5,61.15),(27.8,61.8),(28.5,62.2),(29.2,62.15),(29.5,61.8),(28.8,61.5),(28.4,61.1)])]
RIVERS=[('Glomma',[(11.5,62.6),(11.1,61.5),(11.5,60.8),(11.1,60.2),(11.2,59.5),(10.95,59.15)]),
 ('Göta älv',[(12.3,58.38),(12.3,58.1),(12.1,57.9),(11.9,57.7)]),
 ('Dalälven',[(13.5,61.9),(14.8,61.0),(15.5,60.5),(16.4,60.25),(17.4,60.5)]),
 ('Rhinen',[(8.3,49.),(8.2,49.9),(7.6,50.35),(7.1,50.73),(6.96,50.94),(6.75,51.35),(6.6,51.8),(5.8,51.9)]),
 ('Elben',[(14.2,50.8),(13.7,51.05),(12.7,51.6),(12.1,51.9),(11.65,52.13),(11.68,52.3),(11.85,52.43),(11.95,52.6),(12.08,52.70),(12.05,52.82),(11.75,53.0),(11.57,53.02),(11.45,53.04),(11.26,53.14),(11.18,53.17),(11.05,53.18),(10.99,53.24),(10.9,53.28),(10.79,53.29),(10.76,53.36),(10.7,53.38),(10.59,53.37),(10.51,53.37),(10.37,53.43),(10.25,53.44),(10.1,53.50),(9.99,53.54),(9.88,53.54),(9.78,53.55),(9.68,53.57),(9.56,53.59),(9.48,53.63),(9.4,53.67),(9.3,53.72),(9.15,53.79),(9.,53.84),(8.7,53.9)]),
 ('Oder',[(18.1,49.9),(17.,50.5),(16.3,51.2),(14.7,52.),(14.6,52.6),(14.3,53.2),(14.6,53.7)]),
 ('Wisła',[(19.9,50.05),(21.,51.2),(21.,52.2),(19.7,52.5),(18.6,53.),(18.8,53.6),(18.9,54.3)])]

def hydro_mask(raster,local_lakes=None,local_rivers=None):
    lake=np.zeros((raster.h,raster.w),bool)
    if hasattr(local_lakes,'rasterize'):
        lake=local_lakes.rasterize(raster)
    elif local_lakes:
        lake=mask(raster,local_lakes)
    else:
        for _,points in LAKES:
            lake|=raster.fill([[points]])
    image=Image.new('L',(raster.w,raster.h));draw=ImageDraw.Draw(image)
    if local_rivers:
        lines=[points for points,_,_ in local_rivers]
        for points in lines:
            px=(points[:,0]-raster.x_min)/(raster.x_max-raster.x_min)*raster.w;py=(raster.y_max-points[:,1])/(raster.y_max-raster.y_min)*raster.h
            draw.line(list(zip(px,py)),fill=255,width=2)
    else:
        for _,points in RIVERS:
            lon,lat=np.array(points).T;px,py=raster.to_px(lon,lat);draw.line(list(zip(px,py)),fill=255,width=2)
    return lake,np.asarray(image)>0

def render_tile(root,tile,land_shapes,etopo,sources,local_lakes=None,local_rivers=None):
    b=tile['boundsKm'];m=read(root/'Data/Campaign1851/MapTiles.json');dx,dy=m['kmPerPixel']['x'],m['kmPerPixel']['y']
    raster=Raster(b['xMin']-HALO*dx,b['xMax']+HALO*dx,b['yMin']-HALO*dy,b['yMax']+HALO*dy,TILE+2*HALO,TILE+2*HALO)
    land=mask(raster,land_shapes);lakes,rivers=hydro_mask(raster,local_lakes,local_rivers);land&=~lakes
    if etopo:
        from etopo_pillow import encode_height
        xx,yy=raster.xy_grids();longitude,latitude=unproject(xx,yy)
        meters,edge=etopo.sample(longitude,latitude);height=encode_height(meters,land)
        tile['demEdgeExtendedLandPixels']=int(np.count_nonzero(edge[HALO:HALO+TILE,HALO:HALO+TILE]&land[HALO:HALO+TILE,HALO:HALO+TILE]))
    else:
        height,meters,_,_=relief(raster,land,etopo)

    lon,lat=raster.grids();regions=np.where(land,REGION_KINGDOM,0).astype(np.uint8)
    context=WorldNoise(raster)
    colour,_,forest,farmland=paint(raster,land,regions,lon,lat,SEED,context=context,height_m=height.astype(np.float32)/65535*3000,edge_fade=False)
    if hasattr(context,'weight'):
        from PIL import ImageFilter
        # Originalens kant er en opskalering af 0,977 km-verdensarket: samme
        # fysiske 0,6-pixel penselbredde, gradvist over i den detaljerede pensel.
        softened=np.asarray(Image.fromarray(colour).filter(ImageFilter.GaussianBlur(.6*(3000/3072)/raster.km_per_px)))
        weight=context.weight[:,:,None]
        colour=np.clip(colour*(1-weight)+softened*weight,0,255).astype(np.uint8)
    # Floder holdes særskilt fra havets 10 km kystskygge.
    colour[rivers&land]=(colour[rivers&land].astype(np.float32)*.2+np.array([28,58,82])*.8).astype(np.uint8)
    region=np.s_[HALO:HALO+TILE,HALO:HALO+TILE]
    colour=colour[region];height=height[region];land=land[region];forest=forest[region];farmland=farmland[region];water=(lakes|rivers)[region]
    features=np.stack((land.astype(np.uint8)*255,(np.clip(forest,0,1)*255).astype(np.uint8),water.astype(np.uint8)*255),axis=-1)
    detail=np.zeros((TILE,TILE,4),np.uint8);detail[:,:,3]=(farmland*255).astype(np.uint8)
    layers={'Color':Image.fromarray(colour),'Height':Image.fromarray(height),'Features':Image.fromarray(features).resize((512,512),Image.Resampling.NEAREST),'DetailMask':Image.fromarray(detail).resize((512,512),Image.Resampling.BILINEAR)}
    from PIL import PngImagePlugin
    for name,image in layers.items():
        from monarchy_mask import get
        image=get(root).restore(image,sources[name],tile,LAYERS[name][1],name)
        path=root/tile['layers'][name]['file'];info=PngImagePlugin.PngInfo()
        if name=='Color':info.add(b'sRGB',b'\x00')
        image.save(path,pnginfo=info,compress_level=4);tile['layers'][name]['sha256']=digest(path)
    return tile

def repaint(root=ROOT,workers=2):
    """Højst to uafhængige fliser ad gangen; ingen højopløst helkort-allokering."""
    from concurrent.futures import ThreadPoolExecutor
    import copy,hashlib
    from tile_seams import calibrate,overview_and_previews,seam_metrics
    root=__import__('pathlib').Path(root);m=read(root/'Data/Campaign1851/MapTiles.json')
    from monarchy_mask import get
    protection=get(root)
    m['monarchyProtection']=protection.report
    sources={k:Image.open(root/p) for k,(p,_) in LAYERS.items() if k!='Amter'}
    for image in sources.values():image.load()
    land=[ring for rings,_ in geometry(root,'ne_10m_land') for ring in rings]
    local_lakes=[];local_rivers=[]
    ne=root/'SourceAssets/NaturalEarth'
    for kind,target in [('ne_10m_lakes',local_lakes),('ne_10m_rivers_lake_centerlines',local_rivers)]:
        if (ne/kind/(kind+'.shp')).exists():target.extend(ring for rings,_ in geometry(root,kind) for ring in rings)
    etopo=list((root/'SourceAssets/ETOPO2022').glob('*.tif'))
    if etopo:
        from etopo_pillow import EtopoPillow
        etopo=EtopoPillow(etopo)
        if not local_lakes:
            from dem_lakes import DemLakeMasks
            local_lakes=DemLakeMasks(etopo,LAKES)
    dem_sources=[dict(file=str(t['path'].relative_to(root)).replace('\\','/'),sha256=digest(t['path'])) for t in etopo.tiles] if etopo else []
    signature=hashlib.sha256((''.join(digest(root/'Tools/Map1851'/name) for name in ('build_map.py','build_tiles.py','paint_tiles_v2.py','tile_seams.py','etopo_pillow.py','dem_lakes.py','overview_background.py','land_tone.py','monarchy_mask.py'))+str(dem_sources)+str(m['legacySources'])).encode()).hexdigest()
    paint_layers=('Color','Height','Features','DetailMask')
    def cache_valid(tile):
        return all(digest(root/tile['layers'][name]['file'])==tile['layers'][name]['sha256'] for name in paint_layers)
    if m.get('painterV2',{}).get('signature')==signature and not m.get('painterV2InProgress') and all(cache_valid(t) for t in m['tiles']):
        previews=('overview_preview_v5.png','tile_crop_northgermany_v5.png','tile_crop_kongeaa_v5.png','tile_crop_lauenburg_v5.png')
        if not all((root/'Saved/Codex'/name).exists() for name in previews):overview_and_previews(root,m)
        print('Malede lag og kildesignatur er uændrede; intet genmales.',flush=True)
        return m
    tiles=[copy.deepcopy(t) for t in m['tiles'] if (t.get('paintV2Signature')!=signature or not cache_valid(t))]
    positions={(t['x'],t['y']):i for i,t in enumerate(m['tiles'])}
    def job(tile):
        tile=render_tile(root,tile,land,etopo,sources,local_lakes,local_rivers)
        tile['paintV2Signature']=signature
        return tile
    m['painterV2InProgress']={'signature':signature,'pending':len(tiles)}
    write(root/'Data/Campaign1851/MapTiles.json',m)

    with ThreadPoolExecutor(max_workers=max(1,min(2,workers))) as pool:
        for i,tile in enumerate(pool.map(job,tiles),1):
            m['tiles'][positions[(tile['x'],tile['y'])]]=tile
            if i%10==0:
                m['painterV2InProgress']['pending']=len(tiles)-i;write(root/'Data/Campaign1851/MapTiles.json',m)
                print(f'Malet {i}/{len(tiles)} fliser',flush=True)
    from land_tone import balance
    region_tone=balance(root,m)
    metrics=calibrate(root,m)
    m.pop('painterV2InProgress',None)
    m['painterV2']=dict(version=5,signature=signature,etopoSources=dem_sources,function='build_map.paint',seed=SEED,edgeFade=False,haloPixels=HALO,legacyTextureTransitionKm=0,noise='world-anchored, original 558.283 km reference scale',sourceSHA256=digest(root/'Tools/Map1851/build_map.py'),relief='ETOPO2022' if etopo else 'skøn: Scandes, Harz, Sudeterne, Eifel, Sauerland; flade lavlande',hydro={'lakes':'ETOPO flat-surface extraction with estimated seed regions' if hasattr(local_lakes,'report') else 'Natural Earth' if local_lakes else 'skøn: lokale navngivne referencepolygoner','rivers':'Natural Earth' if local_rivers else 'skøn: lokale referencepolylinjer'},seamTolerance=3.0,seamStripPixels=4,seamMetrics=metrics)
    m['heightEncoding'].update(note='Fælles 3 km-skala; monarkiets pixels uændrede. Fremmed land: ETOPO i meter.',metersPerNormalizedUnit=3000,rounding='nearest, clipped 0..3000 m; sea=0',source='NOAA NCEI ETOPO 2022 Ice Surface, DOI 10.25921/fd45-gt74',demCoverageExtension='nearest DEM edge outside source coverage; requested lon/lat box fully covered',edgeExtendedLandPixels=sum(t.get('demEdgeExtendedLandPixels',0) for t in m['tiles']))
    m['painterV2']['landTone']=region_tone
    m['painterV2']['palette']='Danish unmuted; region-wide RGB mean balanced to original monarchy land'
    m['painterV2']['hydro']['lakeExtraction']=local_lakes.report if hasattr(local_lakes,'report') else []
    m['reliefStatus']=m['painterV2']['relief'];overview_and_previews(root,m);write(root/'Data/Campaign1851/MapTiles.json',m)
    failures=[(s,k,row) for s,classes in metrics.items() for k,row in classes.items() if row['status']=='FAIL']
    print('Sømresultater:',__import__('json').dumps(metrics,ensure_ascii=False),flush=True)
    if failures:raise AssertionError(f'Farvesøm uden for tolerance: {[(s,k) for s,k,_ in failures]}')
    print('Maler v5 og previews færdige.',flush=True)

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',type=__import__('pathlib').Path,default=ROOT);parser.add_argument('--workers',type=int,default=2);args=parser.parse_args();repaint(args.root,args.workers)
