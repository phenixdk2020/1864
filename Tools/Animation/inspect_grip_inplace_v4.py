import bpy, json, tempfile
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2]
O=R/'SourceAssets/Units1864/Officer'
bpy.ops.wm.open_mainfile(filepath=str(O/'Anim_v4/DK_March_Skulder_Final_v4.blend'))
s=bpy.context.scene;s.frame_set(1);bpy.context.view_layer.update()
rig=next(o for o in s.objects if o.type=='ARMATURE')
for o in s.objects:
 print('OBJECT',o.name,o.type, 'bounds', [list(v) for v in o.bound_box] if o.type=='MESH' else '')
print('BONES',list(rig.pose.bones.keys()))
h=rig.matrix_world@rig.pose.bones['mixamorig:RightHand'].matrix
print('HAND', [list(row) for row in h])
for o in s.objects:
 if o.type!='MESH':continue
 if 'Saber' in o.name:
  for x in [-.1,-.05,0,.025,.05,.1,.2,.8]:
   v=[v.co for v in o.data.vertices if abs(v.co.x-x)<.012]
   print('SABER SLICE',x,len(v),[(min(p[i] for p in v),max(p[i] for p in v)) for i in range(3)] if v else [])
 if 'Officer' in o.name:
  dg=bpy.context.evaluated_depsgraph_get();ev=o.evaluated_get(dg);me=ev.to_mesh()
  ids=[v.index for v in o.data.vertices if sum(g.weight for g in v.groups if o.vertex_groups[g.group].name=='mixamorig:RightHand')>.85]
  vv=[h.inverted()@o.matrix_world@me.vertices[i].co for i in ids]
  print('HAND VERTS',len(vv),'range',[(min(p[i] for p in vv),max(p[i] for p in vv)) for i in range(3)],'mean',list(sum(vv,Vector())/len(vv)))
  ev.to_mesh_clear()
c=bpy.data.objects.new('inspectcam',bpy.data.cameras.new('inspectcam'));s.collection.objects.link(c);s.camera=c
t=s.objects['Saber_v4'].matrix_world.translation
c.location=t+Vector((-.3,-.6,.25));c.rotation_euler=(t-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=.32
s.render.engine='BLENDER_WORKBENCH';s.display.shading.color_type='TEXTURE';s.display.shading.show_cavity=True
s.render.resolution_x=1000;s.render.resolution_y=1000;s.render.resolution_percentage=100
s.render.filepath=str(Path(tempfile.gettempdir())/'officer_grip_before.png');bpy.ops.render.render(write_still=True)
