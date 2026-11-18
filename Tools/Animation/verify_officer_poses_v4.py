import bpy,json
from pathlib import Path
R=Path('C:/Projekter/1864-Campaign');O=R/'SourceAssets/Units1864/Officer'
report={}
for kind in ['March_Skulder','Ordre','Hvil']:
 bpy.ops.wm.open_mainfile(filepath=str(O/f'Anim_v4/DK_{kind}_v4.blend'))
 s=bpy.context.scene;s.frame_set(1);bpy.context.view_layer.update()
 rig=next(o for o in s.objects if o.type=='ARMATURE')
 saber=s.objects['Saber_v4'];h=rig.matrix_world@rig.pose.bones['mixamorig:RightHand'].matrix
 offset=json.loads((O/'DK_Officer_1864_v4/metrics_v4.json').read_text())['saber']['matrix_blender_bone_local']
 from mathutils import Matrix,Vector
 expected=h@Matrix(offset)
 print('ATTACH',kind,list(saber.matrix_world.translation),list(expected.translation),max(abs(saber.matrix_world[i][j]-expected[i][j]) for i in range(4) for j in range(4)))
 print('ACTION',rig.animation_data.action.name,rig.animation_data.action.frame_range[:])
 s.render.engine='BLENDER_WORKBENCH';s.display.shading.color_type='TEXTURE'
 c=bpy.data.objects.new('ValidationCamera_v4',bpy.data.cameras.new('ValidationCamera_v4'));s.collection.objects.link(c);s.camera=c;c.location=(2,-6,2.1);c.rotation_euler=(Vector((0,0,.95))-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=2.6
 s.render.resolution_x=700;s.render.resolution_y=850;s.render.resolution_percentage=100;s.render.filepath=str(O/f'pose_check_{kind}_v4.png');bpy.ops.render.render(write_still=True)
