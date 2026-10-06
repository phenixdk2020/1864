import bpy, sys
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]
mesh=Path(sys.argv[sys.argv.index('--')+1])
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/InfantryCombat/A_Combat_ReloadStanding.fbx'))
clip_rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
poses=[]
for f in (1,70,140,207):
    bpy.context.scene.frame_set(f)
    poses.append({b.name:b.matrix.copy() for b in clip_rig.pose.bones})
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(mesh))
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
rig.animation_data_clear()
for image in bpy.data.images:
    if image.source=='FILE':
        image.filepath=str(mesh.parent/'Livgarden_Texture_0.png')
        image.reload()
for obj in bpy.data.objects:
    if obj.type=='MESH':
        obj.animation_data_clear()
print('PREVIEW_RIG',rig.name,'bones',len(rig.pose.bones))
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.samples=24
scene.render.resolution_x=600
scene.render.resolution_y=800
scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('PreviewWorld')
scene.world.color=(.22,.22,.22)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.025))
floor=bpy.context.object
mat=bpy.data.materials.new('PreviewFloor')
mat.diffuse_color=(.16,.18,.2,1)
floor.data.materials.append(mat)
bpy.ops.object.light_add(type='AREA',location=(3,4,5))
bpy.context.object.data.energy=800
bpy.context.object.data.shape='DISK'
bpy.context.object.data.size=4
bpy.ops.object.camera_add(location=(2.7,-4,1.8))
camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,.95))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.lens=58
scene.camera=camera
output=root/'SourceAssets/InfantryCombat/Preview'
output.mkdir(exist_ok=True)
for index,pose in enumerate(poses):
    for bone in rig.pose.bones:
        if bone.name in pose:
            bone.matrix=pose[bone.name]
            bpy.context.view_layer.update()
    bpy.context.view_layer.update()
    scene.render.filepath=str(output/f'ReloadStanding_{index}.png')
    bpy.ops.render.render(write_still=True)
