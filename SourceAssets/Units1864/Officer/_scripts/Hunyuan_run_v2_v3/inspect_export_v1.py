"""CPU-rasterisering af uændret eksport; ingen meshforenkling."""
import os
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
import json
import sys
import shutil
from pathlib import Path
import numpy as np
import trimesh
from PIL import Image, ImageDraw

run = Path(__file__).resolve().parent
nat = sys.argv[1]
result = json.loads((run / (nat + '_attempt_256_v1') / 'result_v1.json').read_text(encoding='utf-8'))
destination = run.parent / (nat + '_Officer_1864_v1.glb')
if destination.exists():
    raise FileExistsError(destination)
with open(result[1]['value'], 'rb') as src, destination.open('xb') as dst:
    shutil.copyfileobj(src, dst)
scene = trimesh.load(destination, force='scene', process=False)
meshes = list(scene.geometry.values())
qa = run.parent / (nat + '_Officer_1864_v1')
qa.mkdir(exist_ok=False)
info = {'nation': nat, 'triangles': sum(len(m.faces) for m in meshes), 'textures': [], 'meshes': []}
for index, mesh in enumerate(meshes):
    material = mesh.visual.material
    tex = getattr(material, 'baseColorTexture', None)
    if tex is not None:
        texpath = run.parent / (nat + '_Officer_1864_Texture_' + str(index) + '_v1.png')
        with texpath.open('xb') as f:
            tex.save(f, format='PNG')
        info['textures'].append({'file': texpath.name, 'size': list(tex.size)})
    edges = mesh.edges_sorted
    unique, counts = np.unique(edges, axis=0, return_counts=True)
    info['meshes'].append({'triangles': len(mesh.faces), 'boundary_edges_raw': int(np.sum(counts == 1))})
    # UV-sømme giver duplikerede vertices. Kun en kopi svejses til topologikontrol.
    welded = mesh.copy()
    welded.merge_vertices(merge_tex=True, merge_norm=True)
    _, counts = np.unique(welded.edges_sorted, axis=0, return_counts=True)
    info['meshes'][-1]['boundary_edges_welded'] = int(np.sum(counts == 1))
    info['meshes'][-1]['watertight_welded'] = bool(welded.is_watertight)

width, height = 720, 1000
views = [('Front', 0), ('Venstre', 90), ('Back', 180), ('Hoegre', 270)]
panels = []
allv = np.vstack([m.vertices for m in meshes])
center = (allv.min(axis=0) + allv.max(axis=0)) / 2
scale = (height - 100) / np.ptp(allv[:, 1])
for name, angle in views:
    canvas = np.full((height, width, 3), 225, dtype=np.uint8)
    depthbuf = np.full((height, width), -np.inf)
    a = np.deg2rad(angle)
    ca, sa = np.cos(a), np.sin(a)
    for mesh in meshes:
        vertices = mesh.vertices - center
        vx = vertices[:, 0] * ca - vertices[:, 2] * sa
        vz = vertices[:, 0] * sa + vertices[:, 2] * ca
        xy = np.column_stack((width/2 + vx*scale, height/2 - vertices[:,1]*scale))
        texture = getattr(mesh.visual.material, 'baseColorTexture', None)
        pixels = np.array(texture.convert('RGB')) if texture is not None else None
        uv = getattr(mesh.visual, 'uv', None)
        for fi, face in enumerate(mesh.faces):
            p = xy[face]
            xmin, ymin = np.maximum(np.floor(p.min(axis=0)).astype(int), [0,0])
            xmax, ymax = np.minimum(np.ceil(p.max(axis=0)).astype(int), [width-1,height-1])
            if xmax < xmin or ymax < ymin:
                continue
            den = (p[1,1]-p[2,1])*(p[0,0]-p[2,0]) + (p[2,0]-p[1,0])*(p[0,1]-p[2,1])
            if abs(den) < 1e-8:
                continue
            yy, xx = np.mgrid[ymin:ymax+1, xmin:xmax+1]
            x, y = xx+0.5, yy+0.5
            w0 = ((p[1,1]-p[2,1])*(x-p[2,0])+(p[2,0]-p[1,0])*(y-p[2,1]))/den
            w1 = ((p[2,1]-p[0,1])*(x-p[2,0])+(p[0,0]-p[2,0])*(y-p[2,1]))/den
            w2 = 1-w0-w1
            z = w0*vz[face[0]] + w1*vz[face[1]] + w2*vz[face[2]]
            target = depthbuf[ymin:ymax+1,xmin:xmax+1]
            mask = (w0>=-1e-5)&(w1>=-1e-5)&(w2>=-1e-5)&(z>target)
            if not mask.any():
                continue
            if pixels is not None and uv is not None:
                tuv = w0[...,None]*uv[face[0]]+w1[...,None]*uv[face[1]]+w2[...,None]*uv[face[2]]
                tx = np.clip(np.rint(tuv[...,0]*(pixels.shape[1]-1)).astype(int),0,pixels.shape[1]-1)
                ty = np.clip(np.rint((1-tuv[...,1])*(pixels.shape[0]-1)).astype(int),0,pixels.shape[0]-1)
                colors = pixels[ty,tx]
            else:
                colors = np.full((*mask.shape,3),165,dtype=np.uint8)
            shade = 0.65 + 0.35*abs(mesh.face_normals[fi,0]*sa+mesh.face_normals[fi,2]*ca)
            region = canvas[ymin:ymax+1,xmin:xmax+1]
            region[mask] = np.clip(colors[mask]*shade,0,255).astype(np.uint8)
            target[mask] = z[mask]
    panel = Image.fromarray(canvas)
    ImageDraw.Draw(panel).text((20,20), nat+' '+name, fill=(0,0,0))
    panels.append(panel)
sheet = Image.new('RGB', (width*4,height))
for index,panel in enumerate(panels):
    sheet.paste(panel,(index*width,0))
with (qa / 'preview_v1.png').open('xb') as f:
    sheet.save(f,format='PNG')
with (qa / 'metrics_v1.json').open('x',encoding='utf-8') as f:
    json.dump(info,f,indent=2)
print(json.dumps(info),flush=True)
