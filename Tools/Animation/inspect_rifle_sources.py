import bpy, json, sys
from pathlib import Path
source=Path(sys.argv[sys.argv.index('--')+1])
report=[]
for name in ['Rifle Idle.fbx', 'Reload sitting.fbx', 'Walking with rifle.fbx']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source/name))
    rigs=[o for o in bpy.data.objects if o.type=='ARMATURE']
    rig=rigs[0]
    action=rig.animation_data.action
    report.append({'file':name,'rig':rig.name,'scale':list(rig.scale),'fps':bpy.context.scene.render.fps,'range':list(action.frame_range),'bones':[b.name for b in rig.data.bones]})
print('ANIMATION_INSPECTION='+json.dumps(report))
