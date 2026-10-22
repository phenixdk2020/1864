"""Find systematiske farvebrud ved nabofliser og originalrektanglet."""
from functools import lru_cache
import json
import numpy as np
from PIL import Image
from build_tiles import ROOT,read,TILE,LAYERS
from tile_seams import crop_native,land_pixels

def validate(root=ROOT):
    m=read(root/'Data/Campaign1851/MapTiles.json');assert not m.get('painterV2InProgress')
    tiles={(t['x'],t['y']):t for t in m['tiles']}
    @lru_cache(maxsize=18)
    def pixels(key):
        with Image.open(root/tiles[key]['layers']['Color']['file']) as image:return np.array(image,dtype=np.int16)
    maximum=0.;count=0
    for key in sorted(tiles,key=lambda k:(k[1],k[0])):
        a=pixels(key)
        for offset,vertical in [((1,0),False),((0,1),True)]:
            other=(key[0]+offset[0],key[1]+offset[1])
            if other not in tiles:continue
            b=pixels(other)
            if vertical:
                jump=np.abs(a[-1]-b[0]);normal=(np.abs(a[-1]-a[-2]).mean()+np.abs(b[1]-b[0]).mean())*.5
            else:
                jump=np.abs(a[:,-1]-b[:,0]);normal=(np.abs(a[:,-1]-a[:,-2]).mean()+np.abs(b[:,1]-b[:,0]).mean())*.5
            mean=float(jump.mean());maximum=max(maximum,mean);count+=1
            assert mean<=max(5.,normal*1.8+1.),('flisesøm',key,other,mean,float(normal))
    w,h=3996,4096
    boundary={
        'west':(crop_native(root,m,-1,0,1,h),crop_native(root,m,0,0,1,h)),
        'east':(crop_native(root,m,w,0,1,h),crop_native(root,m,w-1,0,1,h)),
        'north':(crop_native(root,m,0,-1,w,1),crop_native(root,m,0,0,w,1)),
        'south':(crop_native(root,m,0,h,w,1),crop_native(root,m,0,h-1,w,1))}
    rows={}
    for side,(image,inside) in boundary.items():
        new=np.asarray(image);original=np.asarray(inside);same=land_pixels(new)==land_pixels(original)
        present=np.any(new!=[18,38,62],axis=-1);selected=same&present
        differences=np.abs(new.astype(np.int16)-original.astype(np.int16))[selected]
        mean=float(differences.mean()) if len(differences) else 0.
        assert mean<=3.,('repainted rectangle edge',side,mean)
        rows[side]=dict(comparedPixels=int(selected.sum()),meanError=mean)
    from monarchy_mask import get
    from land_tone import selection
    from scipy import ndimage
    protection=get(root);total=np.zeros(3);signed=np.zeros(3);border_count=0;local_tones=[]
    for tile in m['tiles']:
        if not(-1<=tile['x']<=3 and -1<=tile['y']<=4):continue
        selected,u,v=selection(root,tile);u,v=np.broadcast_arrays(u,v)
        distance=protection.distance(u,v);chosen=selected&(distance>0)&(distance<=8)
        if not chosen.any():continue
        rr=np.clip((v[chosen]+.5)/2-.5,0,protection.half.shape[0]-1);cc=np.clip((u[chosen]+.5)/2-.5,0,protection.half.shape[1]-1)
        target=np.stack([ndimage.map_coordinates(protection.target[:,:,c],[rr,cc],order=1,prefilter=False,mode='nearest') for c in range(3)],axis=1)
        with Image.open(root/tile['layers']['Color']['file']) as image:actual=np.asarray(image)[chosen].astype(float)
        # v5 deliberately retains fine field texture. Compare mean colour to
        # the smooth reference; per-pixel absolute deviations include that grain.
        delta=actual-target
        total+=np.abs(delta).sum(0);signed+=delta.sum(0);border_count+=len(actual)
        mean_error=np.abs(delta.mean(0))
        if len(actual)>=256:assert mean_error.max()<=3.,('monarchy exterior tile colour',tile['x'],tile['y'],mean_error.tolist())
        local_tones.append(dict(tile=[tile['x'],tile['y']],count=len(actual),meanRGBError=mean_error.tolist()))
    assert border_count>256,('monarchy transition sample count',border_count)
    error=(total/border_count).tolist()
    mean_error=np.abs(signed/border_count).tolist()
    assert max(mean_error)<=3.,('monarchy exterior mean colour transition',mean_error)
    monarchy_border=dict(count=border_count,meanAbsoluteRGBError=error,meanRGBError=mean_error,localTones=local_tones,tolerance=3.,note='First 8 foreign pixels: mean colour compared to continuous original-land reference per tile and globally; absolute per-pixel error includes retained field grain. Protected pixels checked independently.')
    from border_artifacts import validate as validate_artifacts
    artifacts=validate_artifacts(root,m)
    result=dict(borderArtifacts=artifacts,status='OK',adjacentTilePairs=count,maxMeanEdgeJump=maximum,originalBoundary=rows,monarchyBoundary=monarchy_border)
    print(json.dumps(result,ensure_ascii=False));return result
if __name__=='__main__':validate()
