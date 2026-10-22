"""Immutable monarchy pixels, authoritative legacy Features/amter plus DK owner guard."""
from functools import lru_cache
import hashlib
import numpy as np
from PIL import Image
from scipy import ndimage
from build_tiles import LAYERS,TILE,read,digest

BORDER=64
SEA_SEAM=64

class Protection:
    def __init__(self,root):
        self.root=root
        with Image.open(root/LAYERS['Color'][0]) as im:self.colour=np.array(im)
        self.h,self.w=self.colour.shape[:2]
        with Image.open(root/LAYERS['Features'][0]) as im:f=np.array(im.crop((0,im.height//2,im.width,im.height)))
        with Image.open(root/LAYERS['Amter'][0]) as im:a=np.array(im.crop((0,im.height//2,im.width,im.height)))
        with Image.open(root/LAYERS['Height'][0]) as im:height=np.array(im.crop((0,im.height//2,im.width,im.height)))
        self.sea_full=(height==0)|~(self.colour[:,:,1].astype(np.int16)>self.colour[:,:,2].astype(np.int16)+3)
        base=read(root/'Data/Campaign1851/Denmark1851_Map.json');n=read(root/'Data/Campaign1851/cities_neighbours_1851.json');g=n['ownerGrid'];e=base['extentKm'];old=base['northExtension']['originalExtentKm']
        owner=np.fromfile(root/'Data/Campaign1851'/g['file'],np.uint8).reshape(g['height'],g['width'])
        xx=old['xMin']+(np.arange(a.shape[1])*2+1)*(old['xMax']-old['xMin'])/self.w
        yy=old['yMax']-(np.arange(a.shape[0])*2+1)*(old['yMax']-old['yMin'])/self.h
        cols=np.clip(((xx-e['xMin'])/(e['xMax']-e['xMin'])*g['width']).astype(int),0,g['width']-1)
        rows=np.clip(((e['yMax']-yy)/(e['yMax']-e['yMin'])*g['height']).astype(int),0,g['height']-1)
        guard=(owner[rows[:,None],cols[None,:]]==g['ids']['DK'])&~self.sea_full[::2,::2]
        self.half=(f[:,:,0]>0)|(a>0)|guard
        self.full=np.repeat(np.repeat(self.half,2,0),2,1)
        distance=ndimage.distance_transform_edt(~self.half)
        self.dist=ndimage.gaussian_filter((distance*2).astype(np.float32),1.5)
        small=np.asarray(Image.fromarray(self.colour).resize((a.shape[1],a.shape[0]),Image.Resampling.BOX)).astype(np.float32)
        valid=self.half&(small[:,:,1]>small[:,:,0]*.65)&(small[:,:,1]>small[:,:,2]+3)
        # Normalised convolution extends a continuous colour field. Nearest-seed
        # extension creates Voronoi spokes at corners; never copy nearest colours.
        den=ndimage.gaussian_filter(valid.astype(np.float32),24)
        mean=small[valid].mean(0)
        self.target=np.stack([np.divide(ndimage.gaussian_filter(small[:,:,c]*valid,24),den,out=np.full(den.shape,mean[c],np.float32),where=den>1e-7) for c in range(3)],axis=-1)
        self.report=dict(rule='Features.R>0 OR original amter>0 (Regions 1/2/3) OR legacy DK owner on legacy land; conservative 2x2 full-resolution expansion',halfPixels=int(self.half.sum()),fullPixels=int(self.full.sum()),featuresPixels=int((f[:,:,0]>0).sum()),amterPixels=int((a>0).sum()),ownerGuardPixels=int(guard.sum()),maskSHA256=hashlib.sha256(self.half.tobytes()).hexdigest(),blendPixels=BORDER,blendMethod='normalised Gaussian colour extension sigma=48px; smoothed distance feather; retained fine texture; no nearest-seed projection',seaSeamPixels=SEA_SEAM,regions={k:base['regions'][k] for k in ('1','2','3')},sources={str(p):digest(root/p) for p in [LAYERS['Features'][0],LAYERS['Amter'][0],LAYERS['Color'][0],LAYERS['Height'][0],'Data/Campaign1851/Denmark1851_Map.json','Data/Campaign1851/'+g['file']]})
    def sample(self,field,u,v,default=False):
        u,v=np.broadcast_arrays(u,v);inside=(u>=0)&(u<self.w)&(v>=0)&(v<self.h)
        out=np.full(u.shape,default,dtype=field.dtype)
        out[inside]=field[np.clip(v[inside].astype(int),0,self.h-1),np.clip(u[inside].astype(int),0,self.w-1)]
        return out
    def monarchy(self,u,v):return self.sample(self.full,u,v)
    def sea(self,u,v):return self.sample(self.sea_full,u,v)
    def distance(self,u,v):
        u,v=np.broadcast_arrays(u,v)
        if not np.any((u>=-BORDER)&(u<self.w+BORDER)&(v>=-BORDER)&(v<self.h+BORDER)):return np.full(u.shape,np.inf,np.float32)
        rows=np.clip((v+.5)/2-.5,0,self.half.shape[0]-1);cols=np.clip((u+.5)/2-.5,0,self.half.shape[1]-1)
        d=ndimage.map_coordinates(self.dist,[rows,cols],order=1,prefilter=False,mode='nearest')
        return d+np.hypot(np.maximum(np.maximum(-u,u-self.w+1),0),np.maximum(np.maximum(-v,v-self.h+1),0))
    def restore(self,image,source,tile,ratio,name):
        a=np.array(image);size=TILE//ratio;u=tile['x']*TILE+np.arange(size)[None,:]*ratio;v=tile['y']*TILE+np.arange(size)[:,None]*ratio
        u,v=np.broadcast_arrays(u,v);inside=(u>=0)&(u<self.w)&(v>=0)&(v<self.h)
        if not inside.any():return image
        ox=u[inside]//ratio;oy=v[inside]//ratio
        if source.height==8192//ratio:oy=oy+4096//ratio
        source_array=np.asarray(source);old=source_array[oy,ox]
        monarchy=self.monarchy(u,v);sea=self.sea(u,v);keep=(monarchy|sea)&inside
        if name=='Color':
            d=self.distance(u,v);blend=np.clip(1-d/BORDER,0,1);blend=blend*blend*(3-2*blend)
            selected=inside&~monarchy&~sea&(blend>0)
            if selected.any():
                rows=np.clip((v+.5)/2-.5,0,self.half.shape[0]-1);cols=np.clip((u+.5)/2-.5,0,self.half.shape[1]-1)
                target=np.stack([ndimage.map_coordinates(self.target[:,:,c],[rows,cols],order=1,prefilter=False,mode='nearest') for c in range(3)],axis=-1)
                fine=a.astype(np.float32)-ndimage.gaussian_filter(a.astype(np.float32),(6,6,0))
                destination=target+.75*fine
                a[selected]=np.clip(np.rint(a[selected]*(1-blend[selected,None])+destination[selected]*blend[selected,None]),0,255).astype(np.uint8)
            edge=np.minimum.reduce([u,v,self.w-1-u,self.h-1-v]);w=np.clip(edge/SEA_SEAM,0,1);w=w*w*(3-2*w)
            chosen=inside&sea&~monarchy
            if chosen.any():
                src=self.colour[np.clip(v,0,self.h-1),np.clip(u,0,self.w-1)]
                a[chosen]=np.rint(a[chosen]*(1-w[chosen,None])+src[chosen]*w[chosen,None]).astype(np.uint8)
            keep=monarchy&inside
        local=keep[inside]
        patch=a[inside];patch[local]=old[local];a[inside]=patch
        return Image.fromarray(a)

@lru_cache(maxsize=1)
def get(root):return Protection(root)
