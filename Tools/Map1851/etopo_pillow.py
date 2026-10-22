"""ETOPO GeoTIFF via Pillow: én 3600²-kilde ad gangen, kun udsnittet beholdes."""
from pathlib import Path
import numpy as np
from PIL import Image
Image.MAX_IMAGE_PIXELS=None
from scipy.ndimage import map_coordinates, distance_transform_edt

class EtopoPillow:
    def __init__(self,paths):
        self.tiles=[]
        for path in sorted(map(Path,paths)):
            with Image.open(path) as image:
                tie=image.tag_v2.get(33922);scale=image.tag_v2.get(33550)
                if not tie or not scale:raise ValueError('GeoTIFF mangler tiepoint/pixelskala: '+str(path))
                sx,sy=float(scale[0]),float(scale[1]);west=float(tie[3])-float(tie[0])*sx;north=float(tie[4])+float(tie[1])*sy
                self.tiles.append(dict(path=path,west=west,east=west+image.width*sx,north=north,south=north-image.height*sy,sx=sx,sy=sy,width=image.width,height=image.height))
        if not self.tiles:raise ValueError('Ingen lokale ETOPO-filer.')
        self.bounds=(min(t['west'] for t in self.tiles),max(t['east'] for t in self.tiles),min(t['south'] for t in self.tiles),max(t['north'] for t in self.tiles))
    def sample(self,lon,lat):
        lon,lat=np.broadcast_arrays(np.asarray(lon,np.float64),np.asarray(lat,np.float64))
        west,east,south,north=self.bounds
        # LAEA-rektanglets ekstra hjørner kan ligge uden for de seks GeoTIFF'er.
        # Her forlænges nærmeste DEM-kant eksplicit; aldrig analytisk højdeskøn.
        edge=(lon<west)|(lon>=east)|(lat<=south)|(lat>north)
        x=np.clip(lon,west+1e-9,east-1e-9);y=np.clip(lat,south+1e-9,north-1e-9)
        result=np.full(lon.shape,np.nan,np.float32)
        for tile in self.tiles:
            selected=(x>=tile['west'])&(x<tile['east'])&(y<=tile['north'])&(y>tile['south'])
            if not selected.any():continue
            # Pixel-is-area: tiepoint er hjørnet; samplecentre har offset 0,5.
            fx=(x[selected]-tile['west'])/tile['sx']-.5;fy=(tile['north']-y[selected])/tile['sy']-.5
            left=int(np.floor(fx.min()))-1;top=int(np.floor(fy.min()))-1
            right=int(np.ceil(fx.max()))+2;bottom=int(np.ceil(fy.max()))+2
            window=np.full((bottom-top,right-left),np.nan,np.float32)
            # Medtag nabofilens kantpixel, så bilineær sampling er kontinuerlig ved 15°/60°.
            for other in self.tiles:
                ox=round((other['west']-tile['west'])/tile['sx']);oy=round((tile['north']-other['north'])/tile['sy'])
                a=max(left,ox);b=max(top,oy);c=min(right,ox+other['width']);d=min(bottom,oy+other['height'])
                if a>=c or b>=d:continue
                with Image.open(other['path']) as image:
                    block=np.asarray(image.crop((a-ox,b-oy,c-ox,d-oy)),np.float32).copy()
                if not np.isfinite(block).all():raise ValueError('ETOPO-kilde indeholder ikke-endelige værdier: '+str(other['path']))
                window[b-top:d-top,a-left:c-left]=block
            missing=~np.isfinite(window)
            if missing.all():raise ValueError('Tomt ETOPO-vindue.')
            if missing.any():
                indices=distance_transform_edt(missing,return_distances=False,return_indices=True)
                window[missing]=window[tuple(indices[:,missing])]
            result[selected]=map_coordinates(window,[fy-top,fx-left],order=1,prefilter=False,mode='nearest')
        if not np.isfinite(result).all():raise ValueError('ETOPO indeholder nodata eller et hul mellem kildefilerne.')
        return result,edge

def encode_height(meters,land):
    return np.where(land,np.rint(np.clip(meters,0,3000)/3000*65535),0).astype(np.uint16)
