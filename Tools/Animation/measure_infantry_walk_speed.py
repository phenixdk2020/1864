"""Measure low-foot backward travel relative to hips in world metres, weighted by stance duration.
Run with Blender -b -P; does not modify FBX files. Component scale is identity in EnsureVisualCount.
Forward is the principal horizontal axis of both feet relative to hips, oriented by the leg span.
Stance proxy: both endpoints in the lowest quarter of each foot's height range and travelling backwards.
"""
import bpy, json, math, hashlib
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[2]
results=[]
for clip in ['Walk','WalkShoulder']:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/InfantryCombat'/('A_Combat_'+clip+'.fbx')))
 arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
 sc=bpy.context.scene
 action=arm.animation_data.action
 lo,hi=map(int,action.frame_range)
 fps=sc.render.fps/sc.render.fps_base
 samples=[]
 for f in range(lo,hi+1):
  sc.frame_set(f)
  world=lambda n: arm.matrix_world @ arm.pose.bones['mixamorig:'+n].head
  right=world('RightUpLeg')-world('LeftUpLeg');right.z=0;right.normalize()
  samples.append({'f':f,'hips':list(world('Hips')),'left':list(world('LeftFoot')),'right':list(world('RightFoot')),'axis':list(Vector((0,0,1)).cross(right))})
 forward=sum((Vector(s['axis']) for s in samples),Vector()).normalized()
 # Principal stride axis avoids hip sway/twist bias in the anatomical forward estimate.
 offsets=[]
 for foot in ['left','right']:
  track=[Vector(s[foot])-Vector(s['hips']) for s in samples]
  centre=sum(track,Vector())/len(track)
  offsets.extend(v-centre for v in track)
 mx=sum(v.x for v in offsets)/len(offsets); my=sum(v.y for v in offsets)/len(offsets)
 xx=sum((v.x-mx)**2 for v in offsets); yy=sum((v.y-my)**2 for v in offsets); xy=sum((v.x-mx)*(v.y-my) for v in offsets)
 angle=0.5*math.atan2(2*xy,xx-yy)
 stride=Vector((math.cos(angle),math.sin(angle),0))
 if stride.dot(forward)<0: stride=-stride
 forward=stride
 feet=[]
 for foot in ['left','right']:
  heights=[s[foot][2] for s in samples]; cutoff=min(heights)+0.25*(max(heights)-min(heights))
  points=[(Vector(s[foot])-Vector(s['hips'])).dot(forward) for s in samples]
  segments=[]
  for i in range(len(samples)-1):
   v=(points[i+1]-points[i])*fps
   if heights[i]<=cutoff and heights[i+1]<=cutoff and v<0:
    segments.append({'from':lo+i,'to':lo+i+1,'backward_m':points[i]-points[i+1],'mps':-v})
  distance=sum(s['backward_m'] for s in segments); duration=len(segments)/fps
  feet.append({'foot':foot,'height_cutoff_m':cutoff,'stance_seconds':duration,'backward_m':distance,'cm_per_second':distance/duration*100,'segments':segments})
 result={'clip':clip,'source_sha256':hashlib.sha256((root/'SourceAssets/InfantryCombat'/('A_Combat_'+clip+'.fbx')).read_bytes()).hexdigest(),'method':'World-space foot-head backward travel relative to hips; lowest 25% foot-height range; principal stride axis after centring each foot track; distance / accepted interval duration','soldier_component_scale':1.0,'armature_scale':list(arm.scale),'scene_unit_scale':sc.unit_settings.scale_length,'frames':[lo,hi],'fps':fps,'cycle_seconds':(hi-lo)/fps,'forward':list(forward),'feet':feet,'cm_per_second':sum(f['backward_m'] for f in feet)/sum(f['stance_seconds'] for f in feet)*100}
 results.append(result)
 print('MEASURE',clip, 'cm/s',result['cm_per_second'], 'forward',list(forward))
(root/'SourceAssets/InfantryCombat/walk_speed_measurement.json').write_text(json.dumps(results,indent=2)+'\n')
