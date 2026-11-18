import bpy,json
from pathlib import Path
script=Path(__file__).with_name('fit_officer_grip_v4.py')
scope={'__file__':str(script)}
exec(script.read_text(encoding='utf-8-sig').split('for nat in NATIONS:run(nat)')[0],scope)
from collections import Counter
for nat in scope['NATIONS']:
 for label in scope['LABELS']:
  bpy.ops.wm.open_mainfile(filepath=str(scope['TMP']/f'{nat}_{label}.blend'))
  scene=bpy.context.scene;scene.frame_set(1);scope['update']()
  body=scene.objects[f'{nat}_Officer_1864_v4'];saber=scene.objects['Saber_v4']
  bv,bf=scope['body_geometry'](body)
  print('DIAG',nat,label,json.dumps(scope['intersection'](bv,bf,saber)),json.dumps(scope['measured_cavity'](body,saber)),flush=True)
  ev=body.evaluated_get(bpy.context.evaluated_depsgraph_get());me=ev.to_mesh();me.calc_loop_triangles();saber.data.calc_loop_triangles()
  pf=[tuple(t.vertices) for t in saber.data.loop_triangles];pv=[saber.matrix_world@v.co for v in saber.data.vertices]
  tree=scope['BVHTree'].FromPolygons(bv,bf,all_triangles=True);pt=scope['BVHTree'].FromPolygons(pv,pf,all_triangles=True);pairs=tree.overlap(pt)
  print('MATERIALCUTS',Counter(me.polygons[me.loop_triangles[a].polygon_index].material_index for a,c in pairs))
  for mat in [0,1]:
   subset=[(a,c) for a,c in pairs if me.polygons[me.loop_triangles[a].polygon_index].material_index==mat]
   if subset:print('RANGES',mat,[(min(saber.data.vertices[i].co[k] for a,c in subset for i in pf[c]),max(saber.data.vertices[i].co[k] for a,c in subset for i in pf[c])) for k in range(3)])
