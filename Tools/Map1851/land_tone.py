"""Streaming statistics and colour balance; immutable monarchy and legacy sea excluded."""
import numpy as np
from PIL import Image, PngImagePlugin
from build_tiles import TILE, LAYERS, digest

TOLERANCE=3.0

def selection(root,tile):
    with Image.open(root/tile['layers']['Features']['file']) as im:
        f=np.asarray(im.resize((TILE,TILE),Image.Resampling.NEAREST))
    u=tile['x']*TILE+np.arange(TILE)[None,:]
    v=tile['y']*TILE+np.arange(TILE)[:,None]
    from monarchy_mask import get
    return (f[:,:,0]>127)&(f[:,:,2]<128)&~get(root).monarchy(u,v)&~get(root).sea(u,v),u,v

def statistics(root,m):
    with Image.open(root/LAYERS['Color'][0]) as im:a=np.asarray(im)
    with Image.open(root/LAYERS['Amter'][0]) as im:
        am=np.asarray(im.crop((0,im.height//2,im.width,im.height)).resize((3996,4096),Image.Resampling.NEAREST))
    from monarchy_mask import get
    chosen=get(root).full&(a[:,:,1].astype(int)>a[:,:,2].astype(int)+3)
    reference=a[chosen].astype(float)
    total=np.zeros(3);squares=np.zeros(3);count=0;saturation=0.
    for t in m['tiles']:
        selected,_,_=selection(root,t)
        if not selected.any():continue
        with Image.open(root/t['layers']['Color']['file']) as im:b=np.asarray(im)[selected].astype(float)
        total+=b.sum(0);squares+=(b*b).sum(0);count+=len(b)
        saturation+=((b.max(1)-b.min(1))/np.maximum(b.max(1),1)).sum()
    mean=total/count;ref=reference.mean(0)
    sat=float(((reference.max(1)-reference.min(1))/np.maximum(reference.max(1),1)).mean())
    return dict(danish=dict(count=len(reference),mean=ref.tolist(),std=reference.std(0).tolist(),saturation=sat),newLand=dict(count=count,mean=mean.tolist(),std=np.sqrt(np.maximum(squares/count-mean*mean,0)).tolist(),saturation=saturation/count),maxMeanError=float(np.abs(mean-ref).max()),meanTolerance=TOLERANCE,saturationTolerance=.06)

def balance(root,m):
    # A continuous weight protects the monarchy and its exterior 64-pixel transition.
    for iteration in range(3):
        report=statistics(root,m)
        delta=np.array(report['danish']['mean'])-report['newLand']['mean']
        if np.abs(delta).max()<.65:break
        weight_sum=0.
        for t in m['tiles']:
            selected,u,v=selection(root,t)
            from monarchy_mask import get
            distance=get(root).distance(u,v)
            weight=np.clip(distance/64,0,1);weight=weight*weight*(3-2*weight)
            weight_sum+=weight[selected].sum()
        shift=delta*report['newLand']['count']/weight_sum
        for t in m['tiles']:
            selected,u,v=selection(root,t)
            if not selected.any():continue
            from monarchy_mask import get
            distance=get(root).distance(u,v)
            weight=np.clip(distance/64,0,1);weight=weight*weight*(3-2*weight)
            path=root/t['layers']['Color']['file']
            with Image.open(path) as im:a=np.array(im)
            a[selected]=np.clip(np.rint(a[selected].astype(float)+weight[selected,None]*shift),0,255).astype(np.uint8)
            info=PngImagePlugin.PngInfo();info.add(b'sRGB',b'\x00')
            Image.fromarray(a).save(path,pnginfo=info,compress_level=4)
            t['layers']['Color']['sha256']=digest(path)
        print('Landtone iteration',iteration+1,flush=True)
    report=statistics(root,m)
    assert report['maxMeanError']<=TOLERANCE,report
    return report
