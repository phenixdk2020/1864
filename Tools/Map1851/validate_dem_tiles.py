"""Kontrollér DEM-proveniens og højdesamples på alle nye fliser."""
from pathlib import Path
import json
import numpy as np
from PIL import Image
from etopo_pillow import EtopoPillow,encode_height
from build_tiles import ROOT,read,digest,TILE
from build_map import unproject

def validate(root=ROOT):
    from monarchy_mask import get
    protection=get(root)
    m=read(root/'Data/Campaign1851/MapTiles.json');assert not m.get('painterV2InProgress')
    assert m['painterV2']['relief']=='ETOPO2022'
    paths=[]
    for source in m['painterV2']['etopoSources']:
        path=root/source['file'];assert digest(path)==source['sha256'];paths.append(path)
    reader=EtopoPillow(paths);dx,dy=m['kmPerPixel']['x'],m['kmPerPixel']['y'];o=m['originKm'];checked=0;edge_count=0;maximum=0
    for tile in m['tiles']:
        rows,cols=np.meshgrid(np.arange(16,TILE,64),np.arange(16,TILE,64),indexing='ij');rows=rows.ravel();cols=cols.ravel()
        u=tile['x']*TILE+cols;v=tile['y']*TILE+rows
        original=(u>=0)&(u<3996)&(v>=0)&(v<4096)
        with Image.open(root/tile['layers']['Height']['file']) as im:height=np.array(im)
        with Image.open(root/tile['layers']['Features']['file']) as im:features=np.array(im)
        land=features[rows//2,cols//2,0]>0
        selected=~protection.monarchy(u,v)&~protection.sea(u,v)&land
        if not selected.any():continue
        x=o['x']+(u[selected]+.5)*dx;y=o['y']-(v[selected]+.5)*dy;lon,lat=unproject(x,y)
        meters,edge=reader.sample(lon,lat);expected=encode_height(meters,np.ones(meters.shape,bool))
        actual=height[rows[selected],cols[selected]]
        # Halvopløsnings-Features kan ramme en anden kystpixel; havnul holdes ude.
        valid=actual>0
        error=int(np.max(np.abs(actual[valid].astype(np.int32)-expected[valid].astype(np.int32)),initial=0));maximum=max(maximum,error)
        assert error<=1,(tile['x'],tile['y'])
        checked+=int(valid.sum());edge_count+=int(np.count_nonzero(edge&valid))
    result=dict(status='OK',sampledLandHeights=checked,edgeExtendedSamples=edge_count,maxUint16Error=maximum,tolerance=1,sourceCount=len(paths),encoding='round(clamp(metres,0,3000)/3000*65535)')
    print(json.dumps(result,ensure_ascii=False));return result
if __name__=='__main__':validate()
