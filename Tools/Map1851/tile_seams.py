"""V4: measure actual painted edges; the old foreign rectangle is no longer protected."""

import numpy as np

from PIL import Image

from build_tiles import ROOT,TILE,LAYERS,read,write,digest,project



WIDTH=4

CORNER=128

TOLERANCE=3.0



def land_pixels(a):return a[:,:,1].astype(np.int16)>a[:,:,2].astype(np.int16)+3



def crop_land(root,m,x0,y0,width,height):
    image=Image.new('L',(width,height))
    for t in m['tiles']:
        tx,ty=t['x']*TILE,t['y']*TILE;a=max(x0,tx);b=max(y0,ty);c=min(x0+width,tx+TILE);d=min(y0+height,ty+TILE)
        if a<c and b<d:
            with Image.open(root/t['layers']['Features']['file']) as im:f=np.asarray(im.resize((TILE,TILE),Image.Resampling.NEAREST))
            land=(f[:,:,0]>127)&(f[:,:,2]<128)
            image.paste(Image.fromarray((land*255).astype(np.uint8)).crop((a-tx,b-ty,c-tx,d-ty)),(a-x0,b-y0))
    return np.asarray(image)>0

def seam_metrics(root,m,width=None):
    w,h=3996,4096;n=WIDTH if width is None else width
    boxes={
      'west':((0,0,n,h),(-n,0,n,h),1),
      'east':((w-n,0,n,h),(w,0,n,h),1),
      'north':((0,0,w,n),(0,-n,w,n),0),
      'south':((0,h-n,w,n),(0,h,w,n),0)}
    report={}
    for side,(box_a,box_b,axis) in boxes.items():
        a=np.asarray(crop_native(root,m,*box_a));b=np.asarray(crop_native(root,m,*box_b))
        la=crop_land(root,m,*box_a);lb=crop_land(root,m,*box_b)
        if side in ('west','north'):b=np.flip(b,axis);lb=np.flip(lb,axis)
        else:a=np.flip(a,axis);la=np.flip(la,axis)
        if axis==1:a=a[CORNER:-CORNER];b=b[CORNER:-CORNER];la=la[CORNER:-CORNER];lb=lb[CORNER:-CORNER]
        else:a=a[:,CORNER:-CORNER];b=b[:,CORNER:-CORNER];la=la[:,CORNER:-CORNER];lb=lb[:,CORNER:-CORNER]
        present=np.any(a!=[18,38,62],axis=-1)&np.any(b!=[18,38,62],axis=-1)
        report[side]={}
        for kind,selected in [('land',present&la&lb),('sea',present&~la&~lb)]:
            x=a[selected].astype(float);y=b[selected].astype(float)
            if len(x)<256:report[side][kind]=dict(status='insufficient-samples',count=len(x));continue
            mean=float(np.abs(x.mean(0)-y.mean(0)).max());std=float(np.abs(x.std(0)-y.std(0)).max())
            report[side][kind]=dict(status='OK' if max(mean,std)<=TOLERANCE else 'FAIL',count=len(x),insideMean=x.mean(0).tolist(),outsideMean=y.mean(0).tolist(),maxMeanError=mean,maxStdError=std)
    return report

def calibrate(root,m):

    # All actual blending is restricted to the monarchy boundary in monarchy_mask.

    # Never mute or calibrate against the obsolete grey foreign rectangle.

    return seam_metrics(root,m)



def crop_native(root,m,x0,y0,width,height):

    image=Image.new('RGB',(width,height),(18,38,62))

    for t in m['tiles']:

        tx,ty=t['x']*TILE,t['y']*TILE

        a=max(x0,tx);b=max(y0,ty);c=min(x0+width,tx+TILE);d=min(y0+height,ty+TILE)

        if a<c and b<d:

            with Image.open(root/t['layers']['Color']['file']) as im:image.paste(im.crop((a-tx,b-ty,c-tx,d-ty)),(a-x0,b-y0))

    return image



def overview_and_previews(root,m):

    e=m['extentKm'];ow,oh=m['overview']['size']

    from overview_background import make_background

    overview=make_background(root,m);height=Image.new('I;16',(ow,oh))

    m['overview']['seaBackground']='build_map.paint, same coordinate noise and palette as tiles, blended world/detail scale, chunked'

    for t in m['tiles']:

        b=t['boundsKm'];x0=round((b['xMin']-e['xMin'])/(e['xMax']-e['xMin'])*ow);x1=round((b['xMax']-e['xMin'])/(e['xMax']-e['xMin'])*ow);y0=round((e['yMax']-b['yMax'])/(e['yMax']-e['yMin'])*oh);y1=round((e['yMax']-b['yMin'])/(e['yMax']-e['yMin'])*oh)

        for name,target in [('Color',overview),('Height',height)]:

            with Image.open(root/t['layers'][name]['file']) as im:target.paste(im.resize((x1-x0,y1-y0),Image.Resampling.NEAREST),(x0,y0))

    overview.save(root/m['overview']['color']);height.save(root/m['overview']['height'])

    out=root/'Saved/Codex';out.mkdir(parents=True,exist_ok=True)

    preview=overview.copy();preview.thumbnail((900,900),Image.Resampling.LANCZOS);preview.save(out/'overview_preview_v5.png')

    dx,dy=m['kmPerPixel']['x'],m['kmPerPixel']['y'];o=m['originKm']

    for name,lon,lat in [('tile_crop_northgermany_v5.png',10.8,53.8),('tile_crop_kongeaa_v5.png',9.1,55.4),('tile_crop_lauenburg_v5.png',10.55,53.43)]:

        x,y=project(lon,lat);u=round((x-o['x'])/dx);v=round((o['y']-y)/dy)

        crop_native(root,m,u-512,v-512,1024,1024).save(out/name)

