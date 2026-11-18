import bpy
from pathlib import Path
from mathutils import Vector
R=Path('C:/Projekter/1864-Campaign')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/Units1864/Saber.fbx'))
o=next(o for o in bpy.context.scene.objects if o.type=='MESH')
for a in [-.9,-.7,-.5,0,.5,.7,.9]:
 vs=[o.matrix_world@v.co for v in o.data.vertices if abs((o.matrix_world@v.co).x-a)<.04];print(a,[(min(v[i] for v in vs),max(v[i] for v in vs)) for i in [1,2]])
s=bpy.context.scene;s.render.engine='BLENDER_WORKBENCH';s.display.shading.color_type='TEXTURE'
c=bpy.data.objects.new('cam',bpy.data.cameras.new('cam'));s.collection.objects.link(c);s.camera=c;c.location=(0,-3,2);c.rotation_euler=(-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=2.3;s.render.resolution_x=1200;s.render.resolution_y=500;s.render.resolution_percentage=100;s.render.filepath=str(R/'SourceAssets/Units1864/Officer/prop_inspection_v4.png');bpy.ops.render.render(write_still=True)
