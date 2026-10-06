"""Bake the user's existing rifle clips and a standing reload onto their original rig.
Run with Blender --background --python this_file -- <source-animation-directory>.
"""
import bpy, json, sys
from pathlib import Path

source=Path(sys.argv[sys.argv.index('--')+1])
project=Path(__file__).resolve().parents[2]
output=project/'SourceAssets/InfantryCombat'
output.mkdir(parents=True, exist_ok=True)
clips={'Idle':'Rifle Idle.fbx', 'Walk':'Walking with rifle.fbx',
       'Aim':'Rifle Aiming Idle.fbx', 'Fire':'Firing Rifle.fbx',
       'DeathFront':'Death From The Front.fbx',
       'DeathHeadshot':'Death From Front Headshot.fbx',
       'DeathBack':'Falling Back Death.fbx'}

def load(filename):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source/filename))
    return next(o for o in bpy.data.objects if o.type=='ARMATURE')

def sample(rig):
    action=rig.animation_data.action
    start,end=map(int,action.frame_range)
    frames=[]
    for frame in range(start,end+1):
        bpy.context.scene.frame_set(frame)
        frames.append({b.name:b.matrix_basis.copy() for b in rig.pose.bones})
    return frames

def bake(rig, name, frames):
    rig.animation_data_clear()
    rig.animation_data_create()
    rig.animation_data.action=bpy.data.actions.new('A_Combat_'+name)
    for frame, poses in enumerate(frames,1):
        for bone in rig.pose.bones:
            bone.rotation_mode='QUATERNION'
            bone.matrix_basis=poses[bone.name]
            bone.keyframe_insert('location', frame=frame)
            bone.keyframe_insert('rotation_quaternion', frame=frame)
            bone.keyframe_insert('scale', frame=frame)
    scene=bpy.context.scene
    scene.render.fps=30
    scene.frame_start=1
    scene.frame_end=len(frames)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active=rig
    path=output/('A_Combat_'+name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'ARMATURE'}, add_leaf_bones=False,
        bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0, axis_forward='-Y', axis_up='Z')
    return {'name':name,'file':path.name,'frames':len(frames), 'fps':30,
            'duration':(len(frames)-1)/30, 'bones':len(rig.data.bones)}

report=[]
for name,filename in clips.items():
    rig=load(filename)
    frames=sample(rig)
    if name in ('Idle','Walk','Aim','Fire'):
        # Movement is owned by the strategy unit; keep skeletal root in place.
        root=rig.pose.bones[0].name
        origin=frames[0][root].translation.copy()
        for poses in frames:
            position=poses[root].translation.copy()
            position.x=origin.x
            position.z=origin.z
            poses[root].translation=position
    report.append(bake(rig,name,frames))

idle_rig=load(clips['Idle'])
idle=sample(idle_rig)[0]
rig=load('Reload sitting.fbx')
reload=sample(rig)
lower={n for n in idle if any(part in n for part in ['Hips','UpLeg','Leg','Foot','Toe','Spine','Neck','Head'])}
frames=[]
for i,poses in enumerate(reload):
    # Preserve the stationary standing pelvis and legs. Blend upper-body motion
    # into and out of the same ready pose over twelve frames.
    weight=min(1.0,i/12,(len(reload)-1-i)/12)
    frame={}
    for name,pose in poses.items():
        if name in lower:
            frame[name]=idle[name].copy()
        else:
            loc0,rot0,scale0=idle[name].decompose()
            loc1,rot1,scale1=pose.decompose()
            from mathutils import Matrix
            frame[name]=Matrix.LocRotScale(loc0.lerp(loc1,weight),rot0.slerp(rot1,weight),scale0.lerp(scale1,weight))
    frames.append(frame)
report.append(bake(rig,'ReloadStanding',frames))
(output/'manifest.json').write_text(json.dumps({'source':str(source),'clips':report,
    'reload':'Upper-body motion from Reload sitting with standing Rifle Idle pelvis/legs; authored derivative, not new motion capture.'},indent=2))
print('COMBAT_ANIMATIONS_COMPLETE='+json.dumps(report))
