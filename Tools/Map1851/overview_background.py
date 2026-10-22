"""Mal oversigtsbaggrund med den oprindelige verdensark-palet, også i udeladte havfelter."""
import numpy as np
from PIL import Image
from build_map import Raster,paint,REGION_KINGDOM

def make_background(root,m):
    from build_tiles import geometry,mask
    from paint_tiles_v2 import WorldNoise,JoinedNoise
    e=m['extentKm'];ow,oh=m['overview']['size'];dx=(e['xMax']-e['xMin'])/ow;dy=(e['yMax']-e['yMin'])/oh;halo=16
    full=Raster(e['xMin']-halo*dx,e['xMax']+halo*dx,e['yMin']-halo*dy,e['yMax']+halo*dy,ow+2*halo,oh+2*halo)
    land=mask(full,[ring for rings,_ in geometry(root,'ne_10m_land') for ring in rings])
    output=Image.new('RGB',(ow,oh))
    for y in range(0,oh,256):
        for x in range(0,ow,256):
            w=min(256,ow-x);h=min(256,oh-y)
            raster=Raster(e['xMin']+(x-halo)*dx,e['xMin']+(x+w+halo)*dx,e['yMax']-(y+h+halo)*dy,e['yMax']-(y-halo)*dy,w+2*halo,h+2*halo)
            water_context=land[y:y+h+2*halo,x:x+w+2*halo];lon,lat=raster.grids()
            context=WorldNoise(raster)
            colours,_,_,_=paint(raster,water_context,np.where(water_context,REGION_KINGDOM,0).astype(np.uint8),lon,lat,1864,context=context,height_m=np.zeros(water_context.shape,np.float32),edge_fade=False)
            output.paste(Image.fromarray(colours[halo:halo+h,halo:halo+w]),(x,y))
    return output
