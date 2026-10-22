"""Regression af den danske maler, koordinatstøj og fysisk DEM-kodning."""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
import build_map as bm
from paint_tiles_v2 import WorldNoise
from etopo_pillow import EtopoPillow,encode_height
from build_tiles import ROOT

def validate(root=ROOT):
    fixtures=json.loads((root/'Tools/Map1851/painter_legacy_reference.json').read_text(encoding='utf-8-sig'))
    for row in fixtures:
        r=bm.Raster(0,30,0,30,96,80);lon,lat=r.grids();land=np.indices((80,96)).sum(axis=0)>55;regions=np.where(land,4,0)
        base=np.full((80,96,3),.2,np.float32) if row['seed']==1864 else None
        if row.get('scene')=='mixed-monarchy':
            r=bm.Raster(-200,400,100,700,96,80);lon,lat=r.grids();land=np.indices((80,96)).sum(0)>30;regions=np.where(land,1+np.indices((80,96))[1]//24,0).astype(np.uint8);base=None
        result=bm.paint(r,land,regions,lon,lat,row['seed'],base)
        assert [hashlib.sha256(x.tobytes()).hexdigest() for x in result]==row['hashes'],('dansk standardpainter ændret',row['seed'])
    left=WorldNoise(bm.Raster(-30,70,-20,80,100,100));right=WorldNoise(bm.Raster(20,120,-20,80,100,100))
    a=left.fbm((100,100),45,3,1869);b=right.fbm((100,100),45,3,1869)
    assert np.max(np.abs(a[:,50:]-b[:,:50]))<.0001,'støj afhænger af flisegrænse'
    a=left.worley((100,100),5,1868);b=right.worley((100,100),5,1868)
    assert np.array_equal(a[:,50:],b[:,:50]),'markmønster afhænger af flisegrænse'
    meters=np.array([-100,0,.5,100,1234.5,2999.9,3000,3500],np.float32)
    encoded=encode_height(meters,np.ones(meters.shape,bool));decoded=encoded.astype(float)/65535*3000
    assert np.max(np.abs(decoded-np.clip(meters,0,3000)))<=3000/65535/2+.0001
    files=list((root/'SourceAssets/ETOPO2022').glob('*.tif'))
    if files:
        reader=EtopoPillow(files)
        for tile in reader.tiles:
            with Image.open(tile['path']) as image:expected=image.getpixel((1800,1800))
            actual,edge=reader.sample(np.array([tile['west']+1800.5*tile['sx']]),np.array([tile['north']-1800.5*tile['sy']]))
            assert not edge[0] and abs(float(actual[0])-expected)<.001,tile['path']
        actual,_=reader.sample(np.array([14.999999,15.,15.000001]),np.full(3,60.00001))
        assert np.ptp(actual)<.1,('DEM-filsøm',actual)
    print('OK painter: dansk standardoutput byteidentisk (2 seeds × 2 scener × 4 lag), overlapstøj/markmønstre, DEM pixel-is-area, filkant og 0–3000 m til uint16.')
if __name__=='__main__':validate()
