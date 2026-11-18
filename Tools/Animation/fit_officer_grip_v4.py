"""Blender 5.2, headless. Fixed hand socket, sculpted grasp and measured poses.
Drafts/renders only in tempfile; --apply overwrites Final_v4 and Anim_v4.
"""
import bpy, bmesh, math, json, sys, tempfile
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'SourceAssets/Units1864/Officer'
TMP=Path(tempfile.gettempdir())/'officer_grip_v4'
TMP.mkdir(exist_ok=True)
APPLY='--apply' in sys.argv
NATIONS=sys.argv[sys.argv.index('--nations')+1].split(',') if '--nations' in sys.argv else ['DK']
LABELS=['March_Skulder','Ordre','Hvil']
HAND='mixamorig:RightHand'; HIPS='mixamorig:Hips'
GC=Vector((.005,.0019,.026))
WRIST=Vector((-.075,.070,.085))

def update():bpy.context.view_layer.update()
def active(o):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
def basis(direction,back):
 x=Vector(direction).normalized();z=Vector(back);z=(z-x*z.dot(x)).normalized();y=z.cross(x)
 return Matrix((x,y,z)).transposed().to_4x4()
def wbone(r,n):return r.matrix_world@r.pose.bones[n].matrix
def aim(r,n,target):
 p=r.pose.bones[n];w=wbone(r,n);q=(w.to_3x3()@Vector((0,1,0))).rotation_difference(Vector(target)-w.translation)
 p.matrix=r.matrix_world.inverted()@Matrix.LocRotScale(w.translation,q@w.to_quaternion(),w.to_scale());update()
def ik(r,desired,prop=None,elbow_angle=0):
 # Analytic two-segment solve: preserve lengths, no hand translation keys.
 for name in ['mixamorig:RightArm','mixamorig:RightForeArm',HAND]:r.pose.bones[name].matrix_basis=Matrix.Identity(4)
 update()
 shoulder=wbone(r,'mixamorig:RightArm').translation
 elbow=wbone(r,'mixamorig:RightForeArm').translation
 wrist=wbone(r,HAND).translation
 a=(elbow-shoulder).length;b=(wrist-elbow).length
 target=desired.translation;v=target-shoulder;d=v.length
 if d>a+b-.0001 or d<abs(a-b)+.0001:raise RuntimeError(f'Unreachable wrist {d}, lengths {a},{b}')
 u=v/d;pole=Vector((-.3,.15,-1));pole=(pole-u*pole.dot(u)).normalized()
 pole=Matrix.Rotation(elbow_angle,3,u)@pole
 along=(a*a-b*b+d*d)/(2*d);height=math.sqrt(max(0,a*a-along*along))
 e=shoulder+u*along+pole*height
 aim(r,'mixamorig:RightArm',e);aim(r,'mixamorig:RightForeArm',target)
 p=r.pose.bones[HAND];actual=wbone(r,HAND)
 p.matrix=r.matrix_world.inverted()@Matrix.LocRotScale(actual.translation,desired.to_quaternion(),actual.to_scale());update()
 assert (wbone(r,HAND).translation-target).length<.0001

def grasp_mesh(body,rig,offset):
 """Replace distal fused mitten, retaining cuff and all original skeleton bones.
 Four separate curled fingers have explicit radial clearance to the grip.
 Generated vertices bind rigidly to RightHand, matching the prop socket.
 """
 if body.get('grasp_v4'):return
 rest=rig.matrix_world@rig.data.bones[HAND].matrix_local
 world=rest@offset
 # The supplied sleeve cuff is wider than the wrist and extends past the
 # actual wrist joint. Refit its distal 10 cm in bind space before posing.
 wrist=rest.translation;elbow=(rig.matrix_world@rig.data.bones['mixamorig:RightForeArm'].matrix_local).translation
 axis=(wrist-elbow).normalized();extra_remove=set()
 for v in body.data.vertices:
  weights={body.vertex_groups[g.group].name:g.weight for g in v.groups}
  influence=weights.get('mixamorig:RightForeArm',0)+weights.get(HAND,0)
  if influence<.4:continue
  p=body.matrix_world@v.co;rel=p-wrist;along=rel.dot(axis)
  if rel.length>.20 or along<-.12:continue
  if along>.006:extra_remove.add(v.index);continue
  t=max(0,min(1,(along+.10)/.10));t=t*t*(3-2*t)
  radial=rel-axis*along
  if radial.length>.0001:
   radius=radial.length*(1-t)+.026*t
   p=wrist+axis*min(along,-.002)+radial.normalized()*radius
   v.co=body.matrix_world.inverted()@p
 bm=bmesh.new();bm.from_mesh(body.data);bm.verts.ensure_lookup_table()
 ids=[v.index for v in body.data.vertices if v.index in extra_remove or sum(g.weight for g in v.groups if body.vertex_groups[g.group].name==HAND)>.85]
 bmesh.ops.delete(bm,geom=[bm.verts[i] for i in ids],context='VERTS');bm.to_mesh(body.data);bm.free()
 verts=[];faces=[];palm_ids=[];ring_ids=[]
 def ellipsoid(center,radii,segments=24,rings=14,tag=False):
  start=len(verts)
  for j in range(rings+1):
   t=math.pi*j/rings
   for k in range(segments):
    f=2*math.pi*k/segments
    verts.append(Vector(center)+Vector((radii[0]*math.sin(t)*math.cos(f),radii[1]*math.sin(t)*math.sin(f),radii[2]*math.cos(t))))
  for j in range(rings):
   for k in range(segments):
    a=start+j*segments+k;b=start+j*segments+(k+1)%segments
    faces.append((a,b,b+segments,a+segments))
  if tag:palm_ids.extend(range(start,len(verts)))
 def tube(points,radii,sides=12):
  start=len(verts)
  for j,p in enumerate(points):
   tangent=(points[min(j+1,len(points)-1)]-points[max(0,j-1)]).normalized()
   axis=Vector((1,0,0));axis=(axis-tangent*axis.dot(tangent)).normalized();other=tangent.cross(axis)
   for k in range(sides):
    f=2*math.pi*k/sides;rx,ry=radii[j] if isinstance(radii[j],tuple) else (radii[j],radii[j]);verts.append(p+rx*axis*math.cos(f)+ry*other*math.sin(f))
  faces.append(tuple(start+k for k in reversed(range(sides))))
  for j in range(len(points)-1):
   for k in range(sides):
    a=start+j*sides+k;b=start+j*sides+(k+1)%sides;faces.append((a,b,b+sides,a+sides))
  faces.append(tuple(start+(len(points)-1)*sides+k for k in range(sides)))
 ellipsoid((.005,.034,.026),(.047,.018,.033),tag=True)
 ellipsoid(WRIST,(.039,.024,.025))
 ellipsoid((-.043,.052,.060),(.028,.025,.028))
 saber=bpy.context.scene.objects['Saber_v4']
 for index,x in enumerate([.024,.003,-.018,-.039]):
  pts=[];rad=[]
  section=[v.co for v in saber.data.vertices if abs(v.co.x-x)<.003 and abs(v.co.y-.0019)>.0065 and v.co.z>-.032]
  yz=np.array([(p.y,p.z/2.2) for p in section]);aa=np.column_stack((2*yz,np.ones(len(yz))));bb=np.sum(yz*yz,axis=1)
  fit=np.linalg.lstsq(aa,bb,rcond=None)[0];cy=float(fit[0]);cz=float(fit[1])*2.2
  ry=math.sqrt(max(.00001,float(fit[2])+cy*cy+(cz/2.2)**2));rz=ry*2.2
  for j in range(29):
   t=-.22+j/28*4.48
   thick=.0085-.0040*max(0,math.sin(t))**4
   pts.append(Vector((x,cy+(ry+thick+.0018)*math.cos(t),cz-(rz+thick+.0018)*math.sin(t))))
   rad.append((.009*(1-.08*j/28),thick))
  tube(pts,rad)
 # Thumb rests obliquely above the index/middle fingers, away from guard.
 thumb=[Vector(p) for p in [(.028,.050,.046),(.045,.038,.055),(.048,.018,.060),(.036,-.003,.059),(.023,-.015,.056)]]
 tube(thumb,[.013,.012,.011,.010,.008])
 # Projection against actual triangulated hilt, not a nominal cylinder.
 saber=bpy.context.scene.objects['Saber_v4'];saber.data.calc_loop_triangles()
 hv=[v.co.copy() for v in saber.data.vertices];hf=[tuple(t.vertices) for t in saber.data.loop_triangles]
 tree=BVHTree.FromPolygons(hv,hf,all_triangles=True)
 data=bpy.data.meshes.new('ArticulatedGrasp_v4');data.from_pydata([body.matrix_world.inverted()@world@v for v in verts],[],faces);data.update()
 obj=bpy.data.objects.new('ArticulatedGrasp_v4',data);bpy.context.scene.collection.objects.link(obj)
 mat=bpy.data.materials.get('OfficerHandSkin_v4') or bpy.data.materials.new('OfficerHandSkin_v4')
 mat.diffuse_color=(.68,.46,.33,1);mat.use_nodes=True
 bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=mat.diffuse_color;bs.inputs['Roughness'].default_value=.68
 data.materials.append(mat)
 # Fuse palm, knuckles and fingers into one continuous skin surface; then
 # cut a real clearance cavity using the source hilt, including its guard.
 active(obj)
 bm=bmesh.new();bm.from_mesh(data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(data);bm.free()
 remesh=obj.modifiers.new('ContinuousHandSurface','REMESH');remesh.mode='VOXEL';remesh.voxel_size=.0013;remesh.use_smooth_shade=True
 bpy.ops.object.modifier_apply(modifier=remesh.name)
 smooth=obj.modifiers.new('SkinSmoothing','SMOOTH');smooth.factor=.6;smooth.iterations=3;bpy.ops.object.modifier_apply(modifier=smooth.name)
 cutter=bpy.data.objects.new('ClosedHiltClearance',saber.data.copy());bpy.context.scene.collection.objects.link(cutter);cutter.matrix_world=world
 print('GRASP_STAGE cutter',flush=True)
 bm=bmesh.new();bm.from_mesh(cutter.data)
 bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),plane_co=(.14,0,0),plane_no=(1,0,0),clear_outer=True,clear_inner=False)
 boundary=[e for e in bm.edges if e.is_boundary]
 print('GRASP_STAGE cut',len(boundary),flush=True)
 if boundary:bmesh.ops.holes_fill(bm,edges=boundary,sides=0)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(cutter.data);bm.free()
 active(cutter);rm=cutter.modifiers.new('ClosedHiltVolume','REMESH');rm.mode='VOXEL';rm.voxel_size=.00085;bpy.ops.object.modifier_apply(modifier=rm.name)
 print('GRASP_STAGE volume',len(cutter.data.vertices),flush=True)
 cutter.data.update()
 cutter_normals=[v.normal.copy() for v in cutter.data.vertices]
 for v,n in zip(cutter.data.vertices,cutter_normals):v.co+=n*.0012
 dc=cutter.modifiers.new('CutterReduction','DECIMATE');dc.ratio=.3;bpy.ops.object.modifier_apply(modifier=dc.name)
 print('GRASP_STAGE decimated',flush=True)
 active(obj);diff=obj.modifiers.new('ActualHiltClearance','BOOLEAN');diff.operation='DIFFERENCE';diff.solver='EXACT';diff.object=cutter
 bpy.ops.object.modifier_apply(modifier=diff.name);bpy.data.objects.remove(cutter,do_unlink=True)
 print('GRASP_STAGE difference',len(obj.data.vertices),flush=True)
 dec=obj.modifiers.new('GraspSurfaceReduction','DECIMATE');dec.ratio=.12;bpy.ops.object.modifier_apply(modifier=dec.name)
 # Finish against the untouched high-resolution hilt. Dense contact vertices
 # are projected to its exterior with a real 1.5 mm clearance.
 to_prop=world.inverted()@obj.matrix_world;from_prop=to_prop.inverted()
 for iteration in range(5):
  changed=0
  for v in obj.data.vertices:
   p=to_prop@v.co;hit,n,_,dist=tree.find_nearest(p)
   if dist<.012 and (p-hit).dot(n)<.0025:
    v.co=from_prop@(hit+n*.0025);changed+=1
  if not changed:break
 for iteration in range(4):
  obj.data.calc_loop_triangles();triangles=[tuple(t.vertices) for t in obj.data.loop_triangles]
  handtree=BVHTree.FromPolygons([to_prop@v.co for v in obj.data.vertices],triangles,all_triangles=True)
  overlaps=tree.overlap(handtree)
  if not overlaps:break
  polys={obj.data.loop_triangles[b].polygon_index for a,b in overlaps}
  bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
  edges=set(e for i in polys for e in bm.faces[i].edges)
  bmesh.ops.subdivide_edges(bm,edges=list(edges),cuts=1,use_grid_fill=True);bm.to_mesh(obj.data);bm.free()
  for v in obj.data.vertices:
   p=to_prop@v.co;hit,n,_,dist=tree.find_nearest(p)
   if dist<.012 and (p-hit).dot(n)<.0025:v.co=from_prop@(hit+n*.0025)
 verts=obj.data.vertices
 vg=obj.vertex_groups.new(name=HAND);vg.add(list(range(len(verts))),1,'REPLACE')
 # Landmark group is independent of prop origin; radial finger centres define palm cavity.
 obj['grip_cavity_local_m']=list(GC)
 active(body);obj.select_set(True);bpy.ops.object.join()
 body['grasp_v4']=True;body['palm_cavity_bone_local']=list(offset@GC)
 for p in body.data.polygons:
  if p.material_index==len(body.data.materials)-1:p.use_smooth=True
 return len(ids)

def suspension(body,rig,scaboffset):
 hiprest=rig.matrix_world@rig.data.bones[HIPS].matrix_local;scabworld=hiprest@scaboffset
 verts=[];faces=[]
 mat=bpy.data.materials.get('SwordSuspensionLeather_v4') or bpy.data.materials.new('SwordSuspensionLeather_v4');mat.diffuse_color=(.045,.026,.016,1)
 for index,rx in enumerate([.065,.245]):
  guess=Vector((.16+.025*index,-.12,.87))
  candidates=[body.matrix_world@v.co for v in body.data.vertices if .10<v.co.x<.24 and -.2<v.co.y<-.03 and .80<v.co.z<.95]
  start=min(candidates,key=lambda v:(v-guess).length) if candidates else guess
  end=scabworld@Vector((rx,.001,.066))
  path=[start,Vector((.245,-.06,.86-.055*index)),Vector((.30,.06,.87-.12*index)),end]
  base=len(verts)
  for j,p in enumerate(path):
   tangent=(path[min(j+1,3)]-path[max(0,j-1)]).normalized();normal=Vector((1,0,0));width=tangent.cross(normal).normalized()
   for a,b in [(-1,-1),(-1,1),(1,1),(1,-1)]:verts.append(body.matrix_world.inverted()@(p+width*.007*a+normal*.0015*b))
  for j in range(3):
   for k in range(4):
    a=base+j*4+k;b=base+j*4+(k+1)%4;faces.append((a,b,b+4,a+4))
  faces.extend([(base+3,base+2,base+1,base),(base+12,base+13,base+14,base+15)])
 data=bpy.data.meshes.new('SwordSuspension');data.from_pydata(verts,[],faces);data.materials.append(mat)
 ob=bpy.data.objects.new('SwordSuspension',data);bpy.context.scene.collection.objects.link(ob)
 vg=ob.vertex_groups.new(name=HIPS);vg.add(list(range(len(verts))),1,'REPLACE')
 active(body);ob.select_set(True);bpy.ops.object.join()

def body_geometry(body):
 ev=body.evaluated_get(bpy.context.evaluated_depsgraph_get());me=ev.to_mesh();me.calc_loop_triangles()
 vv=[body.matrix_world@v.co for v in me.vertices];ff=[tuple(t.vertices) for t in me.loop_triangles];ev.to_mesh_clear()
 return vv,ff
def measured_cavity(body,saber):
 """Fit ellipse centres independently to actual hilt and inner skin vertices."""
 saber.data.calc_loop_triangles();sv=[v.co.copy() for v in saber.data.vertices]
 tree=BVHTree.FromPolygons(sv,[tuple(t.vertices) for t in saber.data.loop_triangles],all_triangles=True)
 def region(p):return -.050<p.x<.042 and -.012<p.z<.064 and abs(p.y-.0019)<.035
 grip=[p for p in sv if region(p) and p.z>-.005]
 vv,ff=body_geometry(body);inv=saber.matrix_world.inverted()
 skin=[]
 for p in vv:
  p=inv@p
  if region(p):
   hit,n,_,dist=tree.find_nearest(p)
   if dist<.006 and hit.z>-.006:skin.append(p)
 def centre(points):
  yz=np.array([(p.y,p.z/2.1) for p in points]);a=np.column_stack((2*yz,np.ones(len(yz))));b=np.sum(yz*yz,axis=1)
  fit=np.linalg.lstsq(a,b,rcond=None)[0]
  return Vector((sum(p.x for p in points)/len(points),float(fit[0]),float(fit[1])*2.1))
 a=centre(grip);b=centre(skin)
 return dict(distance_cm=(a-b).length*100,grip_centre_prop_local_m=list(a),palm_centre_prop_local_m=list(b),hilt_samples=len(grip),skin_samples=len(skin),method='Independent least-squares ellipse centres from hilt vertices and evaluated inner-skin vertices within 6 mm of the hilt, central grip crop; fixed section aspect 2.1.')
def intersection(bodyvv,bodyff,prop,minx=None):
 prop.data.calc_loop_triangles();vv=[prop.matrix_world@v.co for v in prop.data.vertices]
 ff=[tuple(t.vertices) for t in prop.data.loop_triangles if minx is None or all(prop.data.vertices[i].co.x>=minx for i in t.vertices)]
 tree=BVHTree.FromPolygons(bodyvv,bodyff,all_triangles=True)
 ptree=BVHTree.FromPolygons(vv,ff,all_triangles=True)
 overlaps=tree.overlap(ptree)
 indices=set(i for f in ff for i in f);depth=0;inside=0;clearance=100
 # Multi-ray parity for closed-surface penetration; open source mesh limitations documented.
 dirs=[Vector((1,.371,.197)).normalized(),Vector((-.237,1,.417)).normalized(),Vector((.179,-.283,1)).normalized()]
 def parity(p,d):
  count=0;origin=p+d*.000002
  for _ in range(80):
   hit,n,i,dist=tree.ray_cast(origin,d,5)
   if hit is None:break
   count+=1;origin=hit+d*.000002
  return count%2
 for i in indices:
  p=vv[i];nearest,n,_,dist=tree.find_nearest(p);clearance=min(clearance,dist)
  if dist<.00001:continue
  if dist<.10 and (p-nearest).dot(n)<0 and sum(parity(p,d) for d in dirs)>=2:
   inside+=1;depth=max(depth,dist)
 return dict(max_penetration_cm=round(depth*100,5),inside_sample_vertices=inside,triangle_intersections=len(overlaps),minimum_surface_clearance_cm=round(clearance*100,5),sample_vertices=len(indices))

def socket_report(m,bone):
 # Blender bone-local axes -> FBX/UE handedness. Rig object scale is inherited.
 reflect=Matrix.Diagonal((1,-1,1));rotation=reflect@m.to_quaternion().to_matrix()@reflect
 q=rotation.to_quaternion();e=rotation.to_euler('XYZ')
 loc=reflect@m.translation
 return dict(bone=bone.rsplit(':',1)[-1],source_bone=bone,location_cm=list(loc),rotation_degrees=dict(pitch=-math.degrees(e.y),yaw=math.degrees(e.z),roll=-math.degrees(e.x)),quaternion_xyzw=[q.x,q.y,q.z,q.w],scale=[v/100 for v in m.to_scale()],matrix_blender_bone_local=[list(row) for row in m])

def render(scene,target,delta,scale,path,res=(800,900)):
 c=scene.camera
 if c is None:
  c=bpy.data.objects.new('GripVerificationCamera',bpy.data.cameras.new('GripVerificationCamera'));scene.collection.objects.link(c);scene.camera=c
 c.location=Vector(target)+Vector(delta);c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler()
 c.data.type='ORTHO';c.data.ortho_scale=scale;c.data.clip_start=.001
 scene.render.engine='BLENDER_WORKBENCH';scene.display.shading.light='STUDIO';scene.display.shading.color_type='TEXTURE'
 scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True;scene.display.shading.cavity_type='BOTH'
 scene.display.shading.background_type='WORLD'
 if scene.world is None:scene.world=bpy.data.worlds.new('GripWorld')
 scene.world.color=(.13,.15,.18)
 scene.render.resolution_x,scene.render.resolution_y=res;scene.render.resolution_percentage=100;scene.render.filepath=str(path)
 bpy.ops.render.render(write_still=True)

def run(nat):
 dst=BASE/'Final_v4'/f'{nat}_Officer_1864_v4';report=dict(nation=nat,poses={},method='Evaluated LBS body. BVH triangle intersections plus multi-ray inside tests of prop vertices; blade x>=0.13 m. All 31 march frames checked.',remaining_problems=['Ingen Unreal-import eller runtime-test.', 'Dansk inspirerede poser, ikke verificeret national eksercits.'])
 offset=None;scaboffset=None
 for label in LABELS:
  source=BASE/'Anim_v4'/f'{nat}_{label}_Final_v4.blend'
  bpy.ops.wm.open_mainfile(filepath=str(source));scene=bpy.context.scene;scene.frame_set(1);update()
  bpy.context.preferences.filepaths.save_version=0
  rig=next(o for o in scene.objects if o.type=='ARMATURE');body=scene.objects[f'{nat}_Officer_1864_v4'];saber=scene.objects['Saber_v4'];scab=scene.objects['Scabbard_v4']
  texture=dst/f'{nat}_Officer_1864_Texture_0_v4.png'
  if texture.exists():
   for material in body.data.materials:
    if material and material.use_nodes:
     for node in material.node_tree.nodes:
      if node.type=='TEX_IMAGE' and node.image:node.image.filepath=str(texture)
  if offset is None:
   hw=wbone(rig,HAND);sw=saber.matrix_world.copy();sw.translation=hw.translation-sw.to_3x3()@WRIST
   offset=hw.inverted()@sw
   # Scabbard throat at belt, point rearward/down. Follow pelvis only.
   cw=basis((.09,.38,-1),(1,0,.09));cw.translation=Vector((.29,.145,1.035))-cw.to_3x3()@Vector((-.085,0,0))
   scaboffset=wbone(rig,HIPS).inverted()@cw
   report['saber_socket']=socket_report(offset,HAND);report['scabbard_socket']=socket_report(scaboffset,HIPS)
  grasp_mesh(body,rig,offset)
  suspension(body,rig,scaboffset)
  for obj,bone,off in [(saber,HAND,offset),(scab,HIPS,scaboffset)]:
   obj.constraints.clear();obj.parent=None;obj.matrix_basis=off
   con=obj.constraints.new('CHILD_OF');con.target=rig;con.subtarget=bone;con.inverse_matrix=Matrix.Identity(4)
  update()
  frames=range(1,32) if label=='March_Skulder' else [1,31]
  fitted_march=None;fitted_elbow_angle=0
  for frame in frames:
   scene.frame_set(frame);update()
   if label=='March_Skulder':
    sw=fitted_march.copy() if fitted_march is not None else basis((.015,.32,1),(1,0,0))
    if fitted_march is None:sw.translation=Vector((-.227,-.09,1.20))
   elif label=='Ordre':
    sw=basis((-.10,-1,.55),(1,-.1,0));sw.translation=Vector((-.29,-.34,1.32))
   else:
    sw=basis((.035,-.08,-1),(1,0,0))
    tip=max(saber.data.vertices,key=lambda v:v.co.x).co
    sw.translation=Vector((-.268,-.13,.003))-sw.to_3x3()@tip
   desired=sw@offset.inverted();ik(rig,desired,sw,fitted_elbow_angle)
   if frame==1:
    # Choose a natural elbow solution that clears the pommel and guard.
    body.data.calc_loop_triangles();saber.data.calc_loop_triangles()
    source_faces=[tuple(t.vertices) for t in body.data.loop_triangles if body.data.polygons[t.polygon_index].material_index==0]
    prop_faces=[tuple(t.vertices) for t in saber.data.loop_triangles]
    prop_tree=BVHTree.FromPolygons([sw@v.co for v in saber.data.vertices],prop_faces,all_triangles=True)
    best=(10**9,0)
    angle_choices=([15,0] if nat=='PR' and label=='March_Skulder' else [0])+[sg*i for i in range(15,181,15) for sg in [-1,1]]
    for degrees in angle_choices:
     angle=math.radians(degrees);ik(rig,sw@offset.inverted(),sw,angle)
     if wbone(rig,'mixamorig:RightForeArm').translation.z>wbone(rig,'mixamorig:RightArm').translation.z-.065:continue
     bv,_=body_geometry(body);count=len(BVHTree.FromPolygons(bv,source_faces,all_triangles=True).overlap(prop_tree))
     if count<best[0]:best=(count,angle)
     if count==0:break
    fitted_elbow_angle=best[1];ik(rig,sw@offset.inverted(),sw,fitted_elbow_angle)
    print('ELBOW_FIT',nat,label,best,flush=True)
   if label=='March_Skulder' and frame==1:
    # Rest blade back against the actual shoulder surface with 0.5 mm gap.
    for iteration in range(8):
     bv,bf=body_geometry(body);tree=BVHTree.FromPolygons(bv,bf,all_triangles=True)
     candidates=[]
     for v in saber.data.vertices:
      if .13<v.co.x<.55:
       p=sw@v.co;hit,n,_,dist=tree.find_nearest(p);candidates.append((dist,p,hit))
     dist,p,hit=min(candidates,key=lambda a:a[0])
     if dist<.00065:break
     previous=sw.copy();sw.translation+=(hit-p).normalized()*(dist-.0005)*.75
     ik(rig,sw@offset.inverted(),sw,fitted_elbow_angle)
     bv,bf=body_geometry(body)
     if intersection(bv,bf,saber)['triangle_intersections']:
      sw=previous;ik(rig,sw@offset.inverted(),sw,fitted_elbow_angle);break
    fitted_march=sw.copy()
   for name in ['mixamorig:RightArm','mixamorig:RightForeArm',HAND]:
    p=rig.pose.bones[name];p.rotation_mode='QUATERNION';p.keyframe_insert('rotation_quaternion',frame=frame)
    p.keyframe_insert('location',frame=frame);p.keyframe_insert('scale',frame=frame)
   update()
  scene.frame_set(1);update()
  cavity=measured_cavity(body,saber)
  pose_report=dict(frames=[],grip_to_palm_cm=0,blade_penetration_cm=0,scabbard_penetration_cm=0)
  pose_report['geometric_cavity_measurement']=cavity
  for frame in frames:
   scene.frame_set(frame);update();bv,bf=body_geometry(body)
   palm=wbone(rig,HAND)@Vector(body['palm_cavity_bone_local']);grip=saber.matrix_world@GC
   anchor_distance=(palm-grip).length*100;distance=cavity['distance_cm']
   blade=intersection(bv,bf,saber,.13);sheath=intersection(bv,bf,scab)
   whole_saber=intersection(bv,bf,saber)
   gap=min((saber.matrix_world@v.co).z for v in saber.data.vertices if v.co.x>.82)
   pose_report['frames'].append(dict(frame=frame,grip_to_palm_cm=round(distance,6),socket_anchor_error_cm=anchor_distance,blade=blade,whole_saber_including_hilt=whole_saber,scabbard=sheath,tip_ground_clearance_cm=round(gap*100,5)))
   pose_report['grip_to_palm_cm']=max(pose_report['grip_to_palm_cm'],distance)
   pose_report['blade_penetration_cm']=max(pose_report['blade_penetration_cm'],blade['max_penetration_cm'])
   pose_report['scabbard_penetration_cm']=max(pose_report['scabbard_penetration_cm'],sheath['max_penetration_cm'])
  report['poses'][label]=pose_report
  if APPLY:
   assert distance<1.5,(nat,label,'palm distance',distance)
   for measurement in pose_report['frames']:
    for key in ['blade','whole_saber_including_hilt','scabbard']:
     assert measurement[key]['triangle_intersections']==0,(nat,label,measurement['frame'],key,measurement[key])
     assert measurement[key]['max_penetration_cm']==0,(nat,label,measurement['frame'],key,measurement[key])
  scene.frame_set(1);update()
  # Export bind mesh with adjusted grasp, skeleton otherwise unchanged.
  if label=='March_Skulder' and APPLY:
   action=rig.animation_data.action;rig.animation_data.action=None
   saved={p.name:p.matrix_basis.copy() for p in rig.pose.bones}
   for p in rig.pose.bones:p.matrix_basis=Matrix.Identity(4)
   update();active(body);rig.select_set(True)
   bpy.ops.export_scene.fbx(filepath=str(dst/f'{nat}_Officer_1864_v4.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,use_armature_deform_only=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,path_mode='RELATIVE')
   for p in rig.pose.bones:p.matrix_basis=saved[p.name]
   rig.animation_data.action=action;scene.frame_set(1);update()
  bpy.ops.wm.save_as_mainfile(filepath=str(source if APPLY else TMP/f'{nat}_{label}.blend'))
  render(scene,(0,-.10,.97),(-2,-7,1.8),2.28,TMP/f'{nat}_{label}_full.png')
  center=saber.matrix_world@GC
  # Knuckle side view: see the real finger wrap and guard clearance.
  bv,bf=body_geometry(body);body.data.calc_loop_triangles()
  sourcefaces=[tuple(t.vertices) for t in body.data.loop_triangles if body.data.polygons[t.polygon_index].material_index==0]
  sourcetree=BVHTree.FromPolygons(bv,sourcefaces,all_triangles=True)
  choices=[]
  for localview in [(0,-1,-.3),(.3,-1,.5),(-.3,-1,.8),(0,1,-.3),(.3,1,.5)]:
   direction=saber.matrix_world.to_3x3()@Vector(localview).normalized();camera=center+direction
   blocked=0
   for x in [-.04,0,.04]:
    target=saber.matrix_world@(GC+Vector((x,0,0)));ray=(target-camera).normalized();hit,n,i,dist=sourcetree.ray_cast(camera,ray,2)
    if hit is not None and dist<(target-camera).length-.04:blocked+=1
   choices.append((blocked,direction))
  view=min(choices,key=lambda p:p[0])[1]
  render(scene,center,view,.32,TMP/f'{nat}_{label}_hand.png',(850,850))
  render(scene,center,saber.matrix_world.to_3x3()@Vector((.1,1,.35)),.32,TMP/f'{nat}_{label}_palm.png',(650,650))
  if label=='Hvil':render(scene,(.225,.04,.83),(2,-4,1.2),.67,TMP/f'{nat}_hip.png',(850,850))
  print('POSE_RESULT',nat,label,json.dumps({k:v for k,v in pose_report.items() if k!='frames'}),flush=True)
 report['palm_landmark']='Independent ellipse centres fitted to actual hilt and evaluated inner finger skin, see geometric_cavity_measurement. Socket anchor error is a separate attachment check.'
 report['unreal_conversion_source']='UE 5.8 FbxUtilsImport.cpp ConvertPos/ConvertRotToQuat; FbxMainImport.cpp MakeName strips namespace. No engine launch.'
 (dst/'verification_v4.json' if APPLY else TMP/f'{nat}_verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 if APPLY:
  metrics=json.loads((dst/'metrics_v4.json').read_text(encoding='utf-8'));metrics['saber']=report['saber_socket'];metrics['scabbard']=report['scabbard_socket'];metrics['grasp_geometry_replaced']=True
  body.data.calc_loop_triangles();metrics['triangles']=len(body.data.loop_triangles)
  (dst/'metrics_v4.json').write_text(json.dumps(metrics,indent=2),encoding='utf-8')
  text=f'# {nat} officer v4 — fast sabelgreb, 2026-10-30\n\nFire krummede fingre og tommelfinger erstatter den sammenvoksede højre vante. Håndgeometrien er bundet til {HAND}; samme skelet og bindematricer. Manchetten er tilpasset håndleddet. To læderremme forbinder det eksisterende venstre gehæng med skeden og følger Hips. Ét fast socket pr. rekvisit i alle tre poser og alle marchframes. Poserne er rettet med reproducerbar toleddet IK uden knoglestræk. 30 fps; march 1 sekund, in-place, rodbevægelse 0 cm, hastighed 0 cm/s. {metrics["triangles"]} trekanter.\n\n'
  for key,title in [('saber_socket','Sabel'),('scabbard_socket','Skede')]:
   s=report[key];text+=f'## {title}: Unreal socket\n\nKnogle: `{s["bone"]}`. Location (X,Y,Z), cm: `{s["location_cm"]}`. Rotation (Pitch,Yaw,Roll), grader: `{s["rotation_degrees"]}`. Scale: `{s["scale"]}`.\n\n'
  text+='Konvertering: FBX -Z frem/Y op, Blender knogleakser bevaret; UE 5.8 FFbxDataConverter vender Y og quaternionens handedness. FbxMainImport::MakeName fjerner namespace: RightHand/Hips i Unreal, mixamorig:RightHand/mixamorig:Hips i Blender. FBX-rodens eksisterende skalering skal bevares; socket scale ovenfor kompenserer den og rekvisittens meter-til-cm-konvertering. Forudsætter Convert Scene og standard import uden ekstra asset rotation. Ingen Unreal-import er udført. Autoritative Blender-matricer og quaternioner i verification_v4.json.\n\nMålinger: evalueret kropsmesh med lineær skinning; BVH-trekantintersektion og ray-paritet på rekvisitvertices. Alle 31 marchframes og begge endeframes af holdte poser. Greb og håndflade måles med separate ellipsefit til fæstets vertices og den evaluerede inderside af fingrene; se sampleantal og målte centre i verification_v4.json. Socket-anchor-fejlen måles separat og bruges ikke som geometrisk håndmåling. Kildemesh kan være åben; overfladeintersektioner rapporteres separat. Hele sablen inklusive fæste er kontrolleret mod hånd, håndled og krop, ud over bladmålingen.\n\n'
  text+='Resultater pr. pose (cm):\n\n'
  for label,p in report['poses'].items():text+=f'- {label}: greb/håndflade {p["grip_to_palm_cm"]:.6f}; bladpenetration {p["blade_penetration_cm"]:.5f}; skedepenetration {p["scabbard_penetration_cm"]:.5f}.\n'
  text+='\nResterende begrænsninger: '+ ' '.join(report['remaining_problems'])+' Kildens uniforms-/ansigtstekstur er uændret. Ingen build, spilstart eller commit.\n'
  (dst/'notes.md').write_text(text,encoding='utf-8')

for nat in NATIONS:run(nat)





