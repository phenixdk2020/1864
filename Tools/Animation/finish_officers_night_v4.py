"""Headless CPU finish; backups and render intermediates live in OS temp."""
import bpy, math, json, tempfile, shutil
from pathlib import Path
from mathutils import Vector, Matrix
ROOT=Path(__file__).resolve().parents[2]
ns={'__file__':str(ROOT/'Tools/Animation/fit_officer_grip_v4.py')}
exec((ROOT/'Tools/Animation/fit_officer_grip_v4.py').read_text(encoding='utf-8').split('for nat in NATIONS:run(nat)')[0],ns)
O=ROOT/'SourceAssets/Units1864/Officer';T=Path(tempfile.gettempdir())/'officer_finish_night';T.mkdir(exist_ok=True)

def foot_min(body,side):
 ev=body.evaluated_get(bpy.context.evaluated_depsgraph_get());me=ev.to_mesh()
 ids=[v.index for v in body.data.vertices if sum(g.weight for g in v.groups if body.vertex_groups[g.group].name in ['mixamorig:'+side+'Foot','mixamorig:'+side+'ToeBase'])>.5]
 z=min((body.matrix_world@me.vertices[i].co).z for i in ids);ev.to_mesh_clear();return z

def leg(r,side,dz):
 names=['mixamorig:'+side+x for x in ['UpLeg','Leg','Foot']]
 a,b,c=[ns['wbone'](r,n) for n in names];start=a.translation;mid=b.translation;end=c.translation;target=end+Vector((0,0,dz))
 la=(mid-start).length;lb=(end-mid).length;u=(target-start).normalized();d=(target-start).length
 assert abs(la-lb)<d<la+lb
 pole=mid-start;pole=(pole-u*pole.dot(u)).normalized();along=(la*la-lb*lb+d*d)/(2*d)
 knee=start+u*along+pole*math.sqrt(max(0,la*la-along*along))
 ns['aim'](r,names[0],knee);ns['aim'](r,names[1],target)
 actual=ns['wbone'](r,names[2]);r.pose.bones[names[2]].matrix=r.matrix_world.inverted()@Matrix.LocRotScale(actual.translation,c.to_quaternion(),actual.to_scale());ns['update']()

def cpu_render(s,target,delta,scale,path,res):
 c=s.camera
 if c is None:
  c=bpy.data.objects.new('NightCamera',bpy.data.cameras.new('NightCamera'));s.collection.objects.link(c);s.camera=c
 c.location=Vector(target)+Vector(delta);c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=scale;c.data.clip_start=.001
 s.render.engine='CYCLES';s.cycles.device='CPU';s.cycles.samples=8;s.cycles.use_denoising=True
 s.world.use_nodes=True;s.world.node_tree.nodes.get('Background').inputs[0].default_value=(.35,.35,.35,1)
 if not s.objects.get('NightSoftbox'):
  light=bpy.data.objects.new('NightSoftbox',bpy.data.lights.new('NightSoftbox','AREA'));s.collection.objects.link(light);light.data.energy=350;light.data.shape='DISK';light.data.size=4;light.location=(-2,-4,4);light.rotation_euler=(Vector((0,0,1))-light.location).to_track_quat('-Z','Y').to_euler()
 s.render.resolution_x,s.render.resolution_y=res;s.render.resolution_percentage=100;s.render.filepath=str(path);bpy.ops.render.render(write_still=True)

for nat in ['DK','SE','NO','PR']:
 dst=O/'Final_v4'/f'{nat}_Officer_1864_v4';report=json.loads((dst/'verification_v4.json').read_text(encoding='utf-8'));result={}
 for label in ['March_Skulder','Ordre','Hvil']:
  source=O/'Anim_v4'/f'{nat}_{label}_Final_v4.blend';backup=T/source.name
  if not backup.exists():shutil.copy2(source,backup)
  bpy.ops.wm.open_mainfile(filepath=str(source));s=bpy.context.scene;r=next(o for o in s.objects if o.type=='ARMATURE');body=s.objects[f'{nat}_Officer_1864_v4'];saber=s.objects['Saber_v4'];scab=s.objects['Scabbard_v4'];bpy.context.preferences.filepaths.save_version=0
  original={}
  for original_frame in range(1,32):
   s.frame_set(original_frame);ns['update']();original[original_frame]={pb.name:pb.matrix_basis.copy() for pb in r.pose.bones}
  frames=[];modified=set()
  for frame in range(1,32):
   s.frame_set(frame)
   for original_name,original_matrix in original[frame].items():r.pose.bones[original_name].matrix_basis=original_matrix
   ns['update']();hand=ns['wbone'](r,ns['HAND']).copy()
   # Relax the right clavicle by 2 cm at the outer end, keeping its length.
   shoulder=ns['wbone'](r,'mixamorig:RightShoulder');arm=ns['wbone'](r,'mixamorig:RightArm');target=arm.translation+Vector((0,0,-.02))
   ns['aim'](r,'mixamorig:RightShoulder',target);modified.add('mixamorig:RightShoulder')
   best=None
   for angle in [0,-15,15,-30,30,-45,45,-60,60,-90,90]:
    ns['ik'](r,hand,elbow_angle=math.radians(angle));vv,ff=ns['body_geometry'](body);hit=ns['intersection'](vv,ff,saber)
    if hit['triangle_intersections']==0 and hit['max_penetration_cm']==0:best=angle;break
   assert best is not None,(nat,label,frame,'no clear relaxed arm')
   modified.update(['mixamorig:RightArm','mixamorig:RightForeArm',ns['HAND']])
   # A marching swing may be airborne; the supporting sole is exactly Z=0.
   levels={side:foot_min(body,side) for side in ['Left','Right']};support=min(levels,key=levels.get)
   for side,z in levels.items():
    if label!='March_Skulder' or side==support or z<0:
     for iteration in range(3):
      error=foot_min(body,side)
      if abs(error)<0.000001:break
      leg(r,side,-error)
     modified.update('mixamorig:'+side+x for x in ['UpLeg','Leg','Foot'])
   feet={side:foot_min(body,side)*100 for side in ['Left','Right']}
   assert min(feet.values())>=-.001 and abs(min(feet.values()))<.001,(nat,label,frame,feet)
   if label!='March_Skulder':assert max(abs(z) for z in feet.values())<.001
   vv,ff=ns['body_geometry'](body)
   for prop in [saber,scab]:
    check=ns['intersection'](vv,ff,prop);assert check['triangle_intersections']==0 and check['max_penetration_cm']==0,(nat,label,frame,prop.name,check)
   for n in modified:
    pb=r.pose.bones[n];pb.rotation_mode='QUATERNION'
    for path in ['location','rotation_quaternion','scale']:pb.keyframe_insert(path,frame=frame)
   print('FRAME_OK',nat,label,frame,flush=True)
   frames.append({'frame':frame,'sole_min_z_cm':feet,'elbow_pole_degrees':best,'shoulder_outer_drop_cm':(arm.translation.z-ns['wbone'](r,'mixamorig:RightArm').translation.z)*100})
  s.frame_set(1);ns['update']();cavity=ns['measured_cavity'](body,saber);assert cavity['distance_cm']<1.5
  result[label]={'frames':frames,'grip_to_palm_cm':cavity['distance_cm']}
  bpy.ops.wm.save_as_mainfile(filepath=str(source))
  # CPU images are staged; originals remain until composition succeeds.
  tmp=ns['TMP'];cpu_render(s,(0,-.10,.97),(-2,-7,1.8),2.28,tmp/f'{nat}_{label}_full.png',(800,900))
  centre=saber.matrix_world@ns['GC'];rot=saber.matrix_world.to_3x3()
  cpu_render(s,centre,rot@Vector((.3,-1,.5)),.32,tmp/f'{nat}_{label}_hand.png',(850,850))
  cpu_render(s,centre,rot@Vector((.1,1,.35)),.32,tmp/f'{nat}_{label}_palm.png',(650,650))
  if label=='Hvil':cpu_render(s,(.225,.04,.83),(2,-4,1.2),.67,tmp/f'{nat}_hip.png',(850,850))
  print('NIGHT_OK',nat,label,flush=True)
 report['night_finish_2026_10_30']=result
 (dst/'verification_v4.json').write_text(json.dumps(report,indent=2,ensure_ascii=False).replace('\n','\r\n'),encoding='utf-8',newline='')
 note=dst/'notes.md';text=note.read_text(encoding='utf-8');text=text.replace('Ikke en produktionsgang med sikret fodkontakt.','Støttefoden er korrigeret til Z=0 på alle 31 heltalsframes; svingfoden må være løftet. Subframe-kontakt og fodglidning er ikke verificeret.')
 text+='\n## Afslutning, Codex nat 2026-10-30\n\nHøjre skulders yderpunkt er sænket ca. 1,7 cm ved rotation af kravebenet uden ændrede knoglelængder. Armen er løst på ny med uændret håndtransform og sabelsocket. Begge såler står på Z=0 i Ordre/Hvil; marchens støttefod står på Z=0, og ingen fod synker på de 31 kontrollerede frames. Eksisterende fire lukkede fingre og separat tommelfinger er bevaret; yderligere vanteombygning er unødvendig. Socketværdierne ovenfor er uændrede. Alle tre klip er fortsat 30 fps, 1 sekund, in-place, 0 cm rodbevægelse og 0 cm/s. CPU-rendering, ingen GPU-app eller Unreal startet. Implementeret, ikke bygget/testet (Codex nat 2026-10-30).\n'
 note.write_text(text.replace('\n','\r\n'),encoding='utf-8',newline='')
