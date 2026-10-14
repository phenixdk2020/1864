"""Decimate the musket-reload props for the baked crowd (a few hundred triangles each).
blender -b -P decimate_reload_props.py -- <Kildeassets dir> <output dir>
"""
import bpy, sys
from pathlib import Path
src, out = [Path(a) for a in sys.argv[sys.argv.index('--') + 1:][:2]]
out.mkdir(parents=True, exist_ok=True)
targets = {'metal-rod': 700, 'wrapped-roll': 500, 'copper-cap': 300}   # triangles
report = {}
for name, tris in targets.items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(src / f'{name}.fbx'))
    obj = next(o for o in bpy.data.objects if o.type == 'MESH')
    before = len(obj.data.polygons)
    bpy.context.view_layer.objects.active = obj
    mod = obj.modifiers.new('dec', 'DECIMATE')
    mod.ratio = min(1.0, tris / max(1, before))
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(out / f'{name}_low.fbx'), use_selection=True, path_mode='COPY', embed_textures=True,
                             object_types={'MESH'}, axis_forward='-Y', axis_up='Z')
    report[name] = (before, len(obj.data.polygons))
print('DECIMATED', report)
