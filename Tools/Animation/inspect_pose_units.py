import bpy,json,sys
for f in sys.argv[sys.argv.index('--')+1:]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=f)
    r=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    bpy.context.scene.frame_set(1)
    b=r.pose.bones['mixamorig:Hips']
    print('POSE_UNITS',f,'obj',r.matrix_world,'rest',b.bone.matrix_local,'basis',b.matrix_basis,'worldhead',r.matrix_world@b.head)
