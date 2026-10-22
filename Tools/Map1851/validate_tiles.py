"""Statisk kontrol af fliser, originalpixels, scenarier og binære gitre."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
from build_tiles import ROOT, TILE, LAYERS, read, geometry, mask, Raster, layout, digest

def validate(root,coverage=True):
    data=root/'Data/Campaign1851';m=read(data/'MapTiles.json');seen=set();pixels={k:0 for k in LAYERS};tilemap={}
    assert not m.get('painterV2InProgress'),'malingen er ikke færdig'
    expected_extent=layout(root)[-1];assert m['extentKm']==expected_extent
    for name,sha in m.get('legacyFiles',{}).items():assert digest(root/name)==sha,name
    for tile in m['tiles']:
        key=(tile['x'],tile['y']);assert key not in seen,key;seen.add(key);tilemap[key]=tile
        gx,gy=key;dx,dy=m['kmPerPixel']['x'],m['kmPerPixel']['y'];origin=m['originKm'];b=tile['boundsKm']
        expected=dict(xMin=origin['x']+gx*TILE*dx,xMax=origin['x']+(gx+1)*TILE*dx,yMax=origin['y']-gy*TILE*dy,yMin=origin['y']-(gy+1)*TILE*dy)
        assert b==expected,key
        for name,entry in tile['layers'].items():
            path=root/entry['file'];assert digest(path)==entry['sha256'],path
            ratio=LAYERS['Amter' if name.startswith('Amter') else name][1]
            with Image.open(path) as image:
                assert image.size==(TILE//ratio,TILE//ratio)==tuple(entry['size'])
                if name.startswith('Amter'):assert image.mode in ('I;16','I')
                if name=='Height':assert image.mode in ('I;16','I')
    from monarchy_mask import get
    protection=get(root)
    assert protection.report==m['monarchyProtection']
    sea_pixels=0;foreign_pixels=0;repainted_foreign=0
    for name,meta in m['legacySources'].items():
        assert digest(root/meta['file'])==meta['sha256'],name
        ratio=meta['ratio'];size=TILE//ratio;ox,oy=meta['offsetPixels']
        with Image.open(root/meta['file']) as source:
            for sy in range(meta.get('preserveRect',[0,0,source.width,source.height])[1],source.height,size):
                for sx in range(0,source.width,size):
                    # Original source starts at aligned origin x=0; north extension at y=-4096.
                    gx=sx//size;gy=(sy+oy)//size;tile=tilemap[(gx,gy)]
                    w=min(size,source.width-sx);h=min(size,source.height-sy)
                    with Image.open(root/tile['layers'][name]['file']) as image:
                        a=np.asarray(source.crop((sx,sy,sx+w,sy+h)))
                        z=np.asarray(image.crop((0,0,w,h)))
                        uu=gx*TILE+np.arange(w)[None,:]*ratio;vv=gy*TILE+np.arange(h)[:,None]*ratio
                        protected=protection.monarchy(uu,vv)
                        assert np.array_equal(a[protected],z[protected]),(name,gx,gy,'monarchy')
                        pixels[name]+=int(protected.sum())
                        if name=='Color':
                            uu,vv=np.broadcast_arrays(uu,vv);edge=np.minimum.reduce([uu,vv,3995-uu,4095-vv])
                            sea=protection.sea(uu,vv)&~protected&(edge>=64)
                            assert np.array_equal(a[sea],z[sea]),('original sea outside seam',gx,gy)
                            sea_pixels+=int(sea.sum())
                            foreign=~protection.sea(uu,vv)&~protected
                            foreign_pixels+=int(foreign.sum())
                            repainted_foreign+=int(np.count_nonzero(foreign&np.any(a!=z,axis=-1)))
    assert foreign_pixels>100_000 and repainted_foreign/foreign_pixels>.95,('old foreign land not repainted',repainted_foreign,foreign_pixels)
    for year in (1825,1851):
        original=read(data/f'cities_neighbours_{year}.json');full=read(data/f'cities_neighbours_full_{year}.json')
        assert full['links'][:len(original['links'])]==original['links'],('gamle ruter',year)
        assert full['districts'][:len(original['districts'])]==original['districts'],('gamle distrikter',year)
        for before,after in zip(original['cities'],full['cities']):
            assert all(after.get(k)==v for k,v in before.items() if k!='onMap'),('gammel by',year,before['name'])
        for name,key,dtype in [('ejer','ownerGridFull',np.uint8),('distrikt','districtGridFull','<u2')]:
            meta=full[key];raw=np.fromfile(data/meta['file'],dtype);assert raw.size==meta['width']*meta['height']
            assert set(np.unique(raw))<={0,*meta['ids'].values()},(year,name)
            assert len(set(meta['ids'].values()))==len(meta['ids'])
            assert meta['cellKm']==.71 and meta['extentKm']['xMax']-meta['extentKm']['xMin']==meta['width']*.71
        dids=full['districtGridFull']['ids'];rows={d['id'] for d in full['districts']}
        assert set(dids)<=rows,(year,set(dids)-rows)
        assert all(c.get('populationStatus')=='skøn' for c in full['researchCities'])
        assert all(c.get('populationStatus')=='skøn' for c in full['cities'] if c['name'] in full['tileAddedCities'])
        assert set(full['ownerGridFull']['ids']) <= {n['id'] for n in full['nations']}
        assert all(d.get('populationStatus')=='skøn' for d in full['researchDistricts'])
        pr=[d for d in full['researchDistricts'] if d['nation']=='PR'];provinces={d['province'] for d in pr}
        if year==1825:assert {'Ostpreußen','Westpreußen'}<=provinces and 'Hohenzollern' not in provinces
        else:assert 'Preussen' in provinces and not {'Ostpreußen','Westpreußen'}&provinces
        names={c['name'] for c in full['cities']}
        base=read(data/'Denmark1851_Map.json');endpoints=names|{c['name'] for c in base['cities']+base['foreignCities']}
        assert all(e['a'] in endpoints and e['b'] in endpoints for e in full['links'])
        assert full['ownerGridFull']['width']<=4096 and full['ownerGridFull']['height']<=4096
        owner_meta=full['ownerGridFull'];district_meta=full['districtGridFull']
        owner=np.memmap(data/owner_meta['file'],dtype=np.uint8,mode='r',shape=(owner_meta['height'],owner_meta['width']))
        district=np.memmap(data/district_meta['file'],dtype='<u2',mode='r',shape=owner.shape)
        nations={d['id']:d['nation'] for d in full['districts']}
        for did,number in district_meta['ids'].items():
            expected_owner=owner_meta['ids'].get(nations[did])
            if expected_owner is not None:
                bad=int(np.count_nonzero((district==number)&(owner!=expected_owner)))
                assert bad==0,(year,did,'forkert ejer',bad)
        for tile in m['tiles']:
            with Image.open(root/tile['layers'][f'Amter_{year}']['file']) as image:
                codes=set(np.unique(np.asarray(image)).tolist());assert all(code<256 or code-256 in district_meta['ids'].values() for code in codes)
    from tile_seams import seam_metrics
    colour_seams=seam_metrics(root,m)
    from land_tone import statistics,TOLERANCE
    tone=statistics(root,m)
    assert tone['newLand']['count']>10_000_000,('region check must cover whole new land',tone)
    assert tone['maxMeanError']<=TOLERANCE,('region-wide RGB',tone)
    from land_tone import selection
    black_land=0
    for tile in m['tiles']:
        selected,_,_=selection(root,tile)
        if not selected.any():continue
        with Image.open(root/tile['layers']['Color']['file']) as image:a=np.asarray(image)
        black_land+=int(np.count_nonzero(selected&(a.max(2)<8)))
    assert black_land==0,('black placeholder/contrast clipping on new land',black_land)
    assert abs(tone['danish']['saturation']-tone['newLand']['saturation'])<=.06,('region-wide saturation',tone)
    for side,classes in colour_seams.items():
        for kind,row in classes.items():
            assert row['status']!='FAIL',(side,kind,row)
    assert m.get('painterV2',{}).get('function')=='build_map.paint'
    assert m['painterV2']['sourceSHA256']==digest(root/'Tools/Map1851/build_map.py')
    code=('build_map.py','build_tiles.py','paint_tiles_v2.py','tile_seams.py','etopo_pillow.py','dem_lakes.py','overview_background.py','land_tone.py','monarchy_mask.py')
    signature=hashlib.sha256((''.join(digest(root/'Tools/Map1851'/name) for name in code)+str(m['painterV2']['etopoSources'])+str(m['legacySources'])).encode()).hexdigest()
    assert m['painterV2']['signature']==signature,'kode/kilder ændret siden malingen'
    from validate_tile_edges import validate as validate_edges
    edge_report=validate_edges(root)
    for name in ('overview_preview_v5.png','tile_crop_northgermany_v5.png','tile_crop_kongeaa_v5.png','tile_crop_lauenburg_v5.png'):
        with Image.open(root/'Saved/Codex'/name) as preview:
            assert preview.height<=900 if name.startswith('overview') else preview.size==(1024,1024)
    checked=0
    if coverage:
        land=[ring for rings,_ in geometry(root,'ne_10m_land') for ring in rings];r=m['gridRange'];dx,dy=m['kmPerPixel']['x'],m['kmPerPixel']['y'];o=m['originKm']
        for gy in range(r['yMin'],r['yMaxExclusive']):
            for gx in range(r['xMin'],r['xMaxExclusive']):
                if (gx,gy) in seen:continue
                raster=Raster(o['x']+gx*TILE*dx,o['x']+(gx+1)*TILE*dx,o['y']-(gy+1)*TILE*dy,o['y']-gy*TILE*dy,TILE,TILE)
                assert not mask(raster,land).any(),('manglende landflise',gx,gy);checked+=1
    # Udeladte havfelter skal have malet baggrund, ikke konstante RGB-blokke.
    overview_meta=m['overview'];assert 'same coordinate noise' in overview_meta.get('seaBackground','')
    with Image.open(root/overview_meta['color']) as overview:
        e=m['extentKm'];ow,oh=overview.size;minimum_sea_std=float('inf')
        for gy in range(m['gridRange']['yMin'],m['gridRange']['yMaxExclusive']):
            for gx in range(m['gridRange']['xMin'],m['gridRange']['xMaxExclusive']):
                if (gx,gy) in seen:continue
                x0=round((m['originKm']['x']+gx*TILE*m['kmPerPixel']['x']-e['xMin'])/(e['xMax']-e['xMin'])*ow)
                y0=round((e['yMax']-m['originKm']['y']+gy*TILE*m['kmPerPixel']['y'])/(e['yMax']-e['yMin'])*oh)
                width=round(TILE*m['kmPerPixel']['x']/(e['xMax']-e['xMin'])*ow);height=round(TILE*m['kmPerPixel']['y']/(e['yMax']-e['yMin'])*oh)
                sea=np.asarray(overview.crop((x0+2,y0+2,x0+width-2,y0+height-2)))
                variation=float(sea.astype(float).std(axis=(0,1)).mean());minimum_sea_std=min(minimum_sea_std,variation)
                assert variation>.05,('ensfarvet havblok',gx,gy,variation)
    report=dict(status='OK',tileCount=len(seen),legacyPixelsCompared=pixels,monarchyPixelsCompared=pixels,unchangedInteriorSeaPixels=sea_pixels,originalForeignLandPixels=foreign_pixels,repaintedOriginalForeignPixels=repainted_foreign,omittedSeaTilesChecked=checked,scenarios=[1825,1851],colourSeams=colour_seams,landTone=tone,blackNewLandPixels=black_land,tileEdges=edge_report,minOmittedSeaStd=minimum_sea_std,note='Kun monarkiets pixels er beskyttede; oprindeligt hav uden for 64-pixel rektangelkant kontrolleret uændret; fremmed land genmalet.')
    print(json.dumps(report,ensure_ascii=False));return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=ROOT);p.add_argument('--skip-coverage',action='store_true');a=p.parse_args();validate(a.root,not a.skip_coverage)
