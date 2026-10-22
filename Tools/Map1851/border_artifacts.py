"""Independent border diagnostics: unsupported thin lines and discontinuous colour extension."""
import numpy as np
from PIL import Image
from scipy import ndimage
from build_tiles import ROOT,TILE,read,geometry,mask
from build_map import Raster,project
from monarchy_mask import get
from paint_tiles_v2 import hydro_mask


def line_components(rgb,allowed,water):
    """Find thin dark/blue straight marks, excluding source/authorised water."""
    a=rgb.astype(np.float32)
    luminance=a@np.array([.2126,.7152,.0722],np.float32)
    dark=ndimage.grey_closing(luminance,size=(7,7))-luminance
    blue=(a[:,:,2]>a[:,:,1]+6)&(a[:,:,1]>a[:,:,0]+8)
    candidates=allowed&~ndimage.binary_dilation(water,iterations=5)&((dark>18)|blue)
    labels,n=ndimage.label(candidates,np.ones((3,3)))
    found=[]
    for i,box in enumerate(ndimage.find_objects(labels),1):
        if box is None:continue
        yy,xx=np.nonzero(labels[box]==i)
        if len(xx)<24:continue
        points=np.column_stack((xx,yy));values=np.linalg.eigvalsh(np.cov(points.T))
        length=float(np.ptp(points,axis=0).max());linearity=float(values[1]/max(values.sum(),1e-6))
        if length>=40 and linearity>.995:
            found.append(dict(pixels=len(xx),length=length,linearity=linearity,box=[box[1].start,box[0].start,box[1].stop,box[0].stop]))
    return found


def brown_components(rgb,allowed):
    # The unmuted land palette is green/yellow. The old corner projection
    # leaked political brown into broad red-dominant fans, unlike fine fields.
    excess=ndimage.gaussian_filter(rgb[:,:,0].astype(float)-rgb[:,:,1].astype(float),2)
    labels,n=ndimage.label(allowed&(excess>8),np.ones((3,3)))
    found=[]
    for i,box in enumerate(ndimage.find_objects(labels),1):
        if box is None:continue
        chosen=labels[box]==i;count=int(chosen.sum())
        if count>=64:found.append(dict(pixels=count,maxRedExcess=float(excess[box][chosen].max()),box=[box[1].start,box[0].start,box[1].stop,box[0].stop]))
    return found


def field_slope(target,chosen):
    # Half-resolution field: derivative is reported per original full pixel.
    maxima=[]
    for axis in (0,1):
        diff=np.abs(np.diff(target,axis=axis)).max(2)*.5
        both=(chosen[1:]&chosen[:-1]) if axis==0 else (chosen[:,1:]&chosen[:,:-1])
        maxima.append(float(diff[both].max()) if both.any() else 0.)
    return max(maxima)


def self_test():
    a=np.full((160,160,3),[80,90,44],np.uint8)
    allow=np.ones((160,160),bool);water=np.zeros_like(allow)
    assert not line_components(a,allow,water)
    a[80,10:150]=[22,43,66]
    assert line_components(a,allow,water),'thin-line detector missed injected unsupported blue line'
    water[80,10:150]=True
    assert not line_components(a,allow,water),'authorised source hydro must remain allowed'
    yy,xx=np.indices((100,100));target=np.full((100,100,3),80.,np.float32)
    target[(yy>50)&(xx>yy)]=50
    assert field_slope(target,np.ones((100,100),bool))>1.,'wedge detector missed injected colour-field wedge'
    a=np.full((100,100,3),[80,90,44],np.uint8);a[(yy>50)&(xx>yy)]=[95,70,40]
    assert brown_components(a,np.ones((100,100),bool)),'brown-fan detector missed injected wedge'


def validate(root=ROOT,manifest=None):
    self_test()
    m=manifest or read(root/'Data/Campaign1851/MapTiles.json');p=get(root)
    band=(p.dist>0)&(p.dist<=64)&~p.half
    slope=field_slope(p.target,band)
    assert slope<=1.,('discontinuous/radial monarchy colour extension',slope)
    rivers=[];ne=root/'SourceAssets/NaturalEarth/ne_10m_rivers_lake_centerlines/ne_10m_rivers_lake_centerlines.shp'
    if ne.exists():rivers=[ring for rings,_ in geometry(root,'ne_10m_rivers_lake_centerlines') for ring in rings]
    land=[ring for rings,_ in geometry(root,'ne_10m_land') for ring in rings]
    checked=[];pixels=0;failures=[];wedges=[]
    for tile in m['tiles']:
        if not(-1<=tile['x']<=4 and -1<=tile['y']<=4):continue
        u,v=np.meshgrid(tile['x']*TILE+np.arange(TILE),tile['y']*TILE+np.arange(TILE))
        distance=p.distance(u,v);near=(distance>0)&(distance<=128)&~p.monarchy(u,v)
        if not near.any():continue
        b=tile['boundsKm'];r=Raster(b['xMin'],b['xMax'],b['yMin'],b['yMax'],TILE,TILE)
        _,expected=hydro_mask(r,None,rivers)
        # Only original water exempts additional marks. Generated Features.B is
        # deliberately NOT trusted: a stray hydro raster would mark itself valid.
        sea=p.sea(u,v);protected=p.monarchy(u,v)
        with Image.open(root/tile['layers']['Color']['file']) as im:a=np.asarray(im)
        expected_sea=~mask(r,land)
        allowed=near&~ndimage.binary_dilation(sea|expected_sea|protected,iterations=5)
        found=line_components(a,allowed,expected|sea)
        brown=brown_components(a,allowed&(distance>8)&(distance<64))
        if brown:wedges.append(dict(tile=[tile['x'],tile['y']],fans=brown))
        checked.append([tile['x'],tile['y']]);pixels+=int(allowed.sum())
        if found:failures.append(dict(tile=[tile['x'],tile['y']],lines=found))
    assert checked and pixels>1000
    assert not failures,('unsupported straight border lines',failures)
    assert not wedges,('brown corner/wedge smudges',wedges)
    return dict(status='OK',borderTiles=checked,foreignBorderPixels=pixels,unsupportedStraightLines=0,brownWedgeComponents=0,maxColourFieldSlopePerPixel=slope,slopeTolerance=1.,negativeControls='injected unsupported blue line, brown image wedge and triangular colour-field discontinuity rejected; authorised hydro accepted',coverage='All tiles intersecting the 128px exterior monarchy band, including Elbe/Lauenburg, Trave/Mecklenburg, Hamburg/Luebeck and Fehmarn; Kongeaa is internal protected monarchy.')

if __name__=='__main__':
    import json
    print(json.dumps(validate(),ensure_ascii=False))
