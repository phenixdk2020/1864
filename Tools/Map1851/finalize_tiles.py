"""Scenarieafhængige distriktsfliser og grove udenlandske metadata."""
import copy
import re
from pathlib import Path
import numpy as np
from PIL import Image
from build_tiles import ROOT, TILE, read, write, digest, paste_legacy
from import_map_tiles import NOTE

def finalize(root):
    data=root/'Data/Campaign1851';m=read(data/'MapTiles.json')
    source=Image.open(root/'Data/Campaign1851/Denmark1851_Amter.png');source.load()
    foreign_ids=['HAN','MEC','MST','OLD','BRU','NL','BE','LU','SAX','SWEI','HK','HD','NAS','BAV','AT','POL','FIN','RU']
    text=(root/'Docs/Kortudvidelse-Research-2026-10-20.md').read_text(encoding='utf-8-sig')
    block=re.findall(r'```\s*\n(.*?)```',text.split('## 4. Nabolande',1)[1],re.S)[0]
    foreign=[]
    for nation,line in zip(foreign_ids,block.strip().splitlines()):
        name,capital,pop=line.split('|');foreign.append(dict(id=nation,name=name,capital=capital,population=round(float(pop)*1000000),populationStatus='skøn',populationNote=NOTE+' Ca. 1850; samme grove overslag i begge scenarier.',playable=False,economicStatus='skøn',economy={'annualRevenuePerHead':.2,'treasuryYears':3,'note':'Grov spilmodel, ikke historisk finansstatistik.'}))
    for code,name in [('FR','Frankrig'),('CH','Schweiz'),('GB','Storbritannien'),('FOREIGN','Øvrige fremmede områder')]:
        foreign.append(dict(id=code,name=name,population=None,populationStatus='skøn',populationNote='Intet befolkningstal i researchdokumentet; ukendt.',playable=False,economicStatus='skøn',economy={'annualRevenuePerHead':.2,'treasuryYears':3,'note':'Grov spilmodel uden befolkningstal.'}))
    for year in (1825,1851):
        path=data/f'cities_neighbours_full_{year}.json';value=read(path);g=value['districtGridFull'];ext=g['extentKm']
        grid=np.memmap(data/g['file'],dtype='<u2',mode='r',shape=(g['height'],g['width']))
        value['foreignOwners']=copy.deepcopy(foreign)
        value['amterTileEncoding']={'legacyRange':[0,255],'foreignDistrictOffset':256,'districtIds':g['ids'],'note':'I oprindeligt sydudsnit: uændrede danske amtspixels. Udenfor: 256 + districtGridFull-id, 0 for intet distrikt.'}
        for row in value['foreignOwners']:
            if not any(n['id']==row['id'] for n in value['nations']):value['nations'].append(copy.deepcopy(row))
        for tile in m['tiles']:
            b=tile['boundsKm'];xs=b['xMin']+(np.arange(512)+.5)/512*(b['xMax']-b['xMin']);ys=b['yMax']-(np.arange(512)+.5)/512*(b['yMax']-b['yMin'])
            ix=np.floor((xs-ext['xMin'])/.71).astype(int);iy=np.floor((ext['yMax']-ys)/.71).astype(int)
            ix=np.clip(ix,0,g['width']-1);iy=np.clip(iy,0,g['height']-1)
            raw=np.array(grid[np.ix_(iy,ix)],dtype=np.uint16);raw[raw>0]+=256
            image=Image.fromarray(raw);paste_legacy(image,source,tile['x'],tile['y'],2)
            name=f'Amter_{year}';dest=root/'Reference/Campaign1851/MapTiles'/f"Tile_{tile['x']}_{tile['y']}_{name}.png"
            image.save(dest);entry=dict(file=str(dest.relative_to(root)).replace('\\','/'),size=[512,512],sha256=digest(dest));tile['layers'][name]=entry
            if year==1851:
                alias=root/tile['layers']['Amter']['file'];image.save(alias);tile['layers']['Amter']['sha256']=digest(alias)
        write(path,value)
    m['legacyFiles']={str(p.relative_to(root)).replace('\\','/'):digest(p) for p in [data/'Denmark1851_Map.json',data/'Neighbours_Owners.bin',data/'South_Districts.bin']}
    m['layerScale'].update(Amter_1825=.5,Amter_1851=.5)
    m['amterEncoding']={'legacyRange':[0,255],'foreignDistrictOffset':256,'scenarioLayer':{'1825':'Amter_1825','1851':'Amter_1851'},'defaultLayerScenario':1851}
    write(data/'MapTiles.json',m)
    print('Scenarieafhængige distriktsfliser færdige.',flush=True)
if __name__=='__main__':finalize(ROOT)
