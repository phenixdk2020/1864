"""DEM-afledte søkonturer med grove navngivne søområder som søgeguide."""
import numpy as np
from PIL import Image,ImageDraw
from scipy import ndimage
from build_map import project

class DemLakeMasks:
    def __init__(self,reader,guides):
        self.layers=[];self.report=[];step=1/240
        for name,points in guides:
            a=np.array(points);west=np.floor((a[:,0].min()-.8)/step)*step;east=np.ceil((a[:,0].max()+.8)/step)*step
            north=np.ceil((a[:,1].max()+.35)/step)*step;south=np.floor((a[:,1].min()-.35)/step)*step
            width=round((east-west)/step);height=round((north-south)/step)
            lon,lat=np.meshgrid(west+(np.arange(width)+.5)*step,north-(np.arange(height)+.5)*step)
            heights,_=reader.sample(lon,lat)
            guide=Image.new('L',(width,height));ImageDraw.Draw(guide).polygon([((x-west)/step,(north-y)/step) for x,y in points],fill=255)
            guide=np.asarray(guide)>0
            levels,counts=np.unique(np.round(heights[guide],1),return_counts=True);index=int(np.argmax(counts));level=float(levels[index])
            candidate=np.abs(heights-level)<.25;labels,count=ndimage.label(candidate)
            intersections=np.bincount(labels[guide].ravel(),minlength=count+1)
            keep=np.flatnonzero(intersections>=10);keep=keep[keep!=0]
            water=np.isin(labels,keep) if int(counts[index])>=32 else np.zeros_like(candidate)
            # Små falske enkeltfelter fjernes uden at gøre søens bred til en polygon.
            sizes=np.bincount(labels.ravel(),minlength=count+1);water&=sizes[labels]>=20
            outline_lon=np.array([west,east,east,west]);outline_lat=np.array([south,south,north,north]);px,py=project(outline_lon,outline_lat)
            self.layers.append(dict(name=name,west=west,north=north,step=step,data=water.astype(np.float32),bounds=(float(px.min()),float(px.max()),float(py.min()),float(py.max()))))
            self.report.append(dict(name=name,levelMeters=level,demWaterCells=int(water.sum()),status='DEM-flat-surface-with-seed-region',note='Kontur fra ETOPO-plan flade; søgeområde og klassifikation er ikke verificeret hydrologi.'))
    def rasterize(self,raster):
        result=np.zeros((raster.h,raster.w),bool);coordinates=None
        for layer in self.layers:
            b=layer['bounds']
            if b[1]<raster.x_min or b[0]>raster.x_max or b[3]<raster.y_min or b[2]>raster.y_max:continue
            if coordinates is None:coordinates=raster.grids()
            lon,lat=coordinates;x=(lon-layer['west'])/layer['step']-.5;y=(layer['north']-lat)/layer['step']-.5
            result|=ndimage.map_coordinates(layer['data'],[y,x],order=1,prefilter=False,mode='constant',cval=0)>.5
        return result
