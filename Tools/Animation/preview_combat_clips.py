import bpy,sys,json,subprocess
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]
mesh=Path(sys.argv[sys.argv.index('--')+1])
output=root/'SourceAssets/InfantryCombat/Preview'
clips=json.loads((root/'SourceAssets/InfantryCombat/manifest.json').read_text())['clips']
allposes=[]
for clip in clips:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/InfantryCombat'/clip['file']))
 r=next(o for o in bpy.data.objects if o.type=='ARMATURE')
 poses=[]
 for f in range(1,clip['frames']+1,3):
  bpy.context.scene.frame_set(f)
  poses.append({b.name:b.matrix.copy() for b in r.pose.bones})
 allposes.append((clip['name'],poses))
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(mesh))
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
rig.animation_data_clear()
for im in bpy.data.images:
 if im.source=='FILE':
  im.filepath=str(mesh.parent/'Livgarden_Texture_0.png');im.reload()
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'
scene.display.shading.color_type='TEXTURE'
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.display.shading.background_type='WORLD'
scene.world=bpy.data.worlds.new('PreviewWorld')
scene.world.color=(.12,.14,.17)
scene.render.resolution_x=480;scene.render.resolution_y=480
scene.render.resolution_percentage=100
bpy.ops.object.camera_add(location=(3.3,-5,2.6))
camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,.8))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=3.3
scene.camera=camera
for name,poses in allposes:
 directory=output/name;directory.mkdir(exist_ok=True)
 for index,pose in enumerate(poses):
  for bone in rig.pose.bones:
   if bone.name in pose:
    bone.matrix=pose[bone.name];bpy.context.view_layer.update()
  scene.render.filepath=str(directory/f'{index:04d}.png')
  bpy.ops.render.render(write_still=True)
 print('PREVIEW_COMPLETE',name,flush=True)

