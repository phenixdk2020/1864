"""Headless Blender 5.2: officer reduction, existing infantry skeleton, FBX and QA.
Run: blender -b --python Tools/Animation/build_officers_v3.py
All outputs are new; refuses existing nation directories. No Unreal operations.
"""
import bpy, bmesh, math, json, shutil, traceback
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'SourceAssets/Units1864/Officer'
import sys
DRAFT='--draft' in sys.argv
DEST=OUT/('ReviewPRFit_v4' if DRAFT else 'Delivery_v4')

def active(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

def weld_normals(obj):
    bm = bmesh.new(); bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=0.00002)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data); bm.free(); obj.data.update()

def triangles(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)

def world_bounds(obj):
    coords = [obj.matrix_world @ v.co for v in obj.data.vertices]
    return [min(v[i] for v in coords) for i in range(3)], [max(v[i] for v in coords) for i in range(3)]

def build(nat):
    dst = DEST / f'{nat}_Officer_1864_v4'
    if dst.exists() and any(dst.iterdir()): raise RuntimeError(f'Output exists: {dst}')
    dst.mkdir(exist_ok=True,parents=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'SourceAssets/Units1864/DK_Infantry_1864.fbx'))
    rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    rig.animation_data_clear()
    for pb in rig.pose.bones: pb.matrix_basis = Matrix.Identity(4)
    for obj in list(bpy.context.scene.objects):
        if obj != rig: bpy.data.objects.remove(obj, do_unlink=True)
    # Keep ALL original local rest transforms, names, parents and object scale.
    original = {b.name: (b.matrix_local.copy(), b.parent.name if b.parent else None) for b in rig.data.bones}
    bpy.ops.import_scene.gltf(filepath=str(OUT/f'{nat}_Officer_1864_v2.glb'))
    meshes = [o for o in bpy.context.selected_objects if o.type == 'MESH']
    active(meshes[0])
    for o in meshes: o.select_set(True)
    if len(meshes)>1: bpy.ops.object.join()
    mesh = bpy.context.object; mesh.name = f'{nat}_Officer_1864_v4'
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    low, high = world_bounds(mesh)
    # Use infantry's -Y forward / Z up convention in Blender and same FBX axes.
    scale = 1.80/(high[2]-low[2])
    for v in mesh.data.vertices:
        v.co = Vector(((v.co.x-(low[0]+high[0])/2)*scale, v.co.y*scale, (v.co.z-low[2])*scale))
    weld_normals(mesh)
    arm_width_scale=max(abs(v.co.x) for v in mesh.data.vertices)/.65
    protect = mesh.vertex_groups.new(name='DetailProtection_v3')
    for v in mesh.data.vertices:
        z=v.co.z; x=abs(v.co.x)
        weight = 1.0 if z>1.49 or (x>.48 and .75<z<1.2) or 1.00<z<1.10 else 0.0
        protect.add([v.index], weight, 'REPLACE')
    active(mesh)
    dec = mesh.modifiers.new('DetailAwareReduction_v3','DECIMATE')
    dec.ratio = 18000/triangles(mesh)
    dec.vertex_group = protect.name; dec.vertex_group_factor = 0.8
    dec.invert_vertex_group = True
    bpy.ops.object.modifier_apply(modifier=dec.name)
    mesh.vertex_groups.remove(mesh.vertex_groups['DetailProtection_v3'])
    weld_normals(mesh)
    # Fit a TEMPORARY weighting cage to the officer A-pose. The export rig is
    # untouched; bake from fitted cage back to infantry rest before final bind.
    fitted = rig.copy(); fitted.data=rig.data.copy(); bpy.context.collection.objects.link(fitted)
    fitted.name='WeightingCage_v3'
    fitted.matrix_world = Matrix.Identity(4)
    active(fitted); bpy.ops.object.mode_set(mode='EDIT')
    world = rig.matrix_world.copy()
    # Original infantry spans ~1.99 m, centered about z=0. Officer is 1.80 m.
    # This affine fit is only the cage; original rig remains exact.
    rig_scale=1.80/1.991153121
    def fit_point(p):
        p=world@p
        return Vector((p.x*rig_scale,p.y*rig_scale,(p.z+0.998)*rig_scale))
    for eb in fitted.data.edit_bones:
        eb.head=fit_point(rig.data.bones[eb.name].head_local)
        eb.tail=fit_point(rig.data.bones[eb.name].tail_local)
    # Align arms with generated A-pose, retaining the original bone axes.
    for side, sign in [('Left',1),('Right',-1)]:
        prefix='mixamorig:'+side
        targets = {'Arm':Vector((sign*.185,.025,1.385)),
                   'ForeArm':Vector((sign*.365,.015,1.205)),
                   'Hand':Vector((sign*.515,-.015,1.025))}
        if nat=='PR':
            targets['ForeArm']=Vector((sign*.40,-.005,1.13))
            targets['Hand']=Vector((sign*.60,-.08,.88))
        for p in targets.values(): p.x*=arm_width_scale
        original_hand=fitted.data.edit_bones[prefix+'Hand'].head.copy()
        hand_delta=targets['Hand']-original_hand
        for eb in fitted.data.edit_bones:
            if eb.name.startswith(prefix+'Hand'):
                eb.head+=hand_delta; eb.tail+=hand_delta
        for part,nextpart in [('Arm','ForeArm'),('ForeArm','Hand')]:
            eb=fitted.data.edit_bones[prefix+part]
            eb.head=targets[part]; eb.tail=targets[nextpart]
        sh=fitted.data.edit_bones.get(prefix+'Shoulder')
        if sh: sh.tail=targets['Arm']
    bpy.ops.object.mode_set(mode='OBJECT')
    active(mesh); fitted.select_set(True); bpy.context.view_layer.objects.active=fitted
    bpy.ops.object.parent_set(type='ARMATURE_AUTO')
    auto_unweighted=sum(not v.groups for v in mesh.data.vertices)
    # Fix disconnected head/hat, belt, mitten hands and unweighted vertices.
    deform=[b for b in fitted.data.bones if b.use_deform and not b.name.endswith('_End')]
    def segment_dist(p,b):
        d=b.tail_local-b.head_local
        t=max(0,min(1,(p-b.head_local).dot(d)/max(d.length_squared,1e-12)))
        return (p-b.head_local-t*d).length
    fixed=0
    for v in mesh.data.vertices:
        p=v.co; force=None
        if p.z>1.56 or (p.z>1.49 and abs(p.x)<.095): force='mixamorig:Head'
        elif abs(p.x)>.50*arm_width_scale and .75<p.z<1.10: force='mixamorig:'+('Left' if p.x>0 else 'Right')+'Hand'
        elif 1.01<p.z<1.075 and abs(p.x)<.18: force='mixamorig:Spine'
        if force or not v.groups:
            if not force: force=min(deform,key=lambda b:segment_dist(p,b)).name
            for vg in mesh.vertex_groups: vg.remove([v.index])
            vg=mesh.vertex_groups.get(force) or mesh.vertex_groups.new(name=force)
            vg.add([v.index],1.0,'REPLACE'); fixed+=1
        if abs(p.x)>.23*arm_width_scale and max(.78,1.50-1.15*abs(p.x)/arm_width_scale)<p.z<1.49:
            prefix='mixamorig:'+('Left' if p.x>0 else 'Right')
            names=[prefix+s for s in ('Arm','ForeArm','Hand')]
            if abs(p.x)>.50*arm_width_scale and p.z<1.10: names=[prefix+'Hand']
            near=sorted([(segment_dist(p,fitted.data.bones[n]),n) for n in names])[:2]
            vals=[1/max(.015,d)**4 for d,n in near]; total=sum(vals)
            for vg in mesh.vertex_groups: vg.remove([v.index])
            for (_,n),w in zip(near,vals):
                vg=mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n)
                vg.add([v.index],w/total,'REPLACE')
            fixed+=1
        # Remove toe/end-bone heat leakage and cross-leg influences. The mesh
        # has mitten hands; fingers and terminal helpers remain in skeleton
        # but need no skin influence. Smooth two nearest limb segments.
        if p.z < .77:
            prefix='mixamorig:'+('Left' if p.x>0 else 'Right')
            names=[prefix+s for s in ('UpLeg','Leg','Foot')]
            if p.z < .18: names=[prefix+'Foot']
            near=sorted([(segment_dist(p,fitted.data.bones[n]),n) for n in names])[:2]
            vals=[1/max(.015,d)**4 for d,n in near]; total=sum(vals)
            for vg in mesh.vertex_groups: vg.remove([v.index])
            for (_,n),w in zip(near,vals):
                vg=mesh.vertex_groups.get(n) or mesh.vertex_groups.new(name=n)
                vg.add([v.index],w/total,'REPLACE')
            fixed+=1
    # Recompute continuous sleeve weights on the unbaked A-pose. No finger or
    # opposite-arm leakage, and no inverse-distance spikes at joint centres.
    adjacency=[set() for v in mesh.data.vertices]
    for e in mesh.data.edges:
        a,b=e.vertices; adjacency[a].add(b); adjacency[b].add(a)
    arm_indices=[]
    def smoothstep(a,b,x):
        t=max(0,min(1,(x-a)/(b-a))); return t*t*(3-2*t)
    for v in mesh.data.vertices:
        p=v.co; x=abs(p.x)/arm_width_scale
        if x>.20 and p.z>max(.50,1.35-1.50*x) and p.z<1.49:
            side='Left' if p.x>0 else 'Right'; prefix='mixamorig:'+side
            elbow=fitted.data.bones[prefix+'ForeArm'].head_local
            wrist=fitted.data.bones[prefix+'Hand'].head_local
            shoulder=fitted.data.bones[prefix+'Arm'].head_local
            axis=(wrist-shoulder).normalized(); along=(p-shoulder).dot(axis)
            el=(elbow-shoulder).dot(axis); wr=(wrist-shoulder).dot(axis)
            fore=smoothstep(el-.055,el+.055,along)
            hand=smoothstep(wr-.025,wr+.025,along)
            vals={prefix+'Arm':1-fore,prefix+'ForeArm':fore*(1-hand),prefix+'Hand':hand}
            for vg in mesh.vertex_groups: vg.remove([v.index])
            for name,w in vals.items():
                if w>0: mesh.vertex_groups[name].add([v.index],w,'REPLACE')
            arm_indices.append(v.index)
    # Mesh adjacency smoothing; limit and normalise for linear FBX skinning.
    active(mesh)
    for vg in mesh.vertex_groups: vg.lock_weight=False
    weights=[{g.group:g.weight for g in v.groups if g.weight>1e-7} for v in mesh.data.vertices]
    for iteration in range(4):
        updated=[dict(w) for w in weights]
        for idx in arm_indices:
            near=adjacency[idx]
            if not near:continue
            names=set(weights[idx])
            for n in near:names.update(weights[n])
            updated[idx]={g:.65*weights[idx].get(g,0)+.35*sum(weights[n].get(g,0) for n in near)/len(near) for g in names}
        weights=updated
    for v,w in zip(mesh.data.vertices,weights):
        pairs=sorted(w.items(),key=lambda x:x[1],reverse=True)[:4];total=sum(x[1] for x in pairs)
        for vg in mesh.vertex_groups:vg.remove([v.index])
        for g,weight in pairs:
            if weight>1e-7:mesh.vertex_groups[g].add([v.index],weight/total,'REPLACE')
    # Compact the generated right mitten into a grasping hand; retain UV and
    # topology. There are no separate finger loops to articulate anatomically.
    wrist=fitted.data.bones['mixamorig:RightHand'].head_local
    axis=(wrist-fitted.data.bones['mixamorig:RightForeArm'].head_local).normalized()
    for v in mesh.data.vertices:
        handweight=sum(g.weight for g in v.groups if mesh.vertex_groups[g.group].name=='mixamorig:RightHand')
        if handweight>.5:
            rel=v.co-wrist;along=rel.dot(axis)
            if along>.025:v.co-=axis*(along-.025)*.50*smoothstep(.5,.95,handweight)
    arm_deltas={}; assembly=Matrix.Diagonal((rig_scale,rig_scale,rig_scale,1))
    assembly.translation=Vector((0,0,.998*rig_scale))
    for side in ['Left','Right']:
        prefix='mixamorig:'+side
        for part in ['Arm','ForeArm','Hand']:
            name=prefix+part; src=fitted.data.bones[name]
            target=rig.data.bones[name]
            a=src.head_local; b=src.tail_local
            c=assembly@rig.matrix_world@target.head_local
            d=assembly@rig.matrix_world@target.tail_local
            if part=='Hand':
                # Carry the mitten with the forearm, without injecting the
                # infantry hand's unrelated axial roll into the sleeve.
                q=arm_deltas[prefix+'ForeArm'].to_quaternion()
            else: q=(b-a).rotation_difference(d-c)
            arm_deltas[name]=Matrix.Translation(c)@q.to_matrix().to_4x4()@Matrix.Translation(-a)
    for v in mesh.data.vertices:
        co=v.co.copy(); correction=Vector()
        for g in v.groups:
            name=mesh.vertex_groups[g.group].name
            if name in arm_deltas: correction+=(arm_deltas[name]@co-co)*g.weight
        v.co+=correction
    active(mesh)
    for m in list(mesh.modifiers):
        if m.type=='ARMATURE':mesh.modifiers.remove(m)
    mesh.parent=None;mesh.matrix_world=Matrix.Identity(4)
    bpy.data.objects.remove(fitted,do_unlink=True)
    rig.matrix_world=assembly@rig.matrix_world
    mod=mesh.modifiers.new('InfantrySkin_v4','ARMATURE');mod.object=rig
    # LBS is deliberately used: Blender and FBX must show the same deformation.
    mod.use_deform_preserve_volume=False
    weld_normals(mesh)
    assert len(rig.data.bones)==41
    for b in rig.data.bones:
        assert max(abs(b.matrix_local[i][j]-original[b.name][0][i][j]) for i in range(4) for j in range(4))<1e-5
    texture=dst/f'{nat}_Officer_1864_Texture_0_v4.png'
    shutil.copy2(OUT/f'{nat}_Officer_1864_Texture_0_v2.png',texture)
    for mat in mesh.data.materials:
        if mat and mat.use_nodes:
            for node in mat.node_tree.nodes:
                if node.type=='TEX_IMAGE' and node.image:node.image.filepath=str(texture)
    active(mesh);rig.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(dst/f'{nat}_Officer_1864_v4.fbx'),use_selection=True,
      object_types={'ARMATURE','MESH'},add_leaf_bones=False,use_armature_deform_only=False,
      bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,path_mode='RELATIVE')
    # Author rotations in world space by shortest swing from the current axis.
    # This avoids guessing Euler channels on the imported Mixamo bone rolls.
    def aim(name,direction):
        pb=rig.pose.bones['mixamorig:'+name]; bpy.context.view_layer.update()
        world=rig.matrix_world@pb.matrix
        axis=world.to_3x3()@Vector((0,1,0))
        q=axis.rotation_difference(Vector(direction).normalized())
        new=Matrix.LocRotScale(world.translation,q@world.to_quaternion(),world.to_scale())
        pb.matrix=rig.matrix_world.inverted()@new;bpy.context.view_layer.update()
    def reset():
        rig.animation_data_clear()
        for pb in rig.pose.bones:pb.matrix_basis=Matrix.Identity(4)
        bpy.context.view_layer.update()
    def pose(kind,phase=0):
        reset()
        if kind=='a':
            aim('RightArm',(-.08,.02,-1));aim('RightForeArm',(.03,-.45,.30))
            aim('LeftArm',(.12,-.35*math.cos(phase),-1));aim('LeftForeArm',(.05,-.15*math.cos(phase)-.12,-1))
            for side,sg in [('Left',1),('Right',-1)]:
                aim(side+'UpLeg',(0,sg*.40*math.cos(phase),-1))
                aim(side+'Leg',(0,sg*.18*math.cos(phase),-1))
        elif kind=='b':
            aim('RightArm',(-.10,-1,.1));aim('RightForeArm',(-.04,-.8,1))
            aim('LeftArm',(.12,.03,-1));aim('LeftForeArm',(.04,-.10,-1))
        else:
            aim('RightArm',(-.20,-.02,-1));aim('RightForeArm',(-.08,-.15,-1))
            aim('LeftArm',(.12,.03,-1));aim('LeftForeArm',(.04,-.10,-1))
    # Load each supplied prop once. Geometry remains a separate object, scaled
    # to a 94 cm sabre, with the handle at x=-.82 in the source FBX.
    props=[]
    for filename in ['Saber','Scabbard']:
        before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(ROOT/f'SourceAssets/Units1864/{filename}.fbx'))
        objects=[o for o in set(bpy.data.objects)-before if o.type=='MESH']
        active(objects[0])
        for o in objects:o.select_set(True)
        if len(objects)>1:bpy.ops.object.join()
        prop=bpy.context.object;prop.name=filename+'_v4'
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        for v in prop.data.vertices:v.co=(v.co-Vector((-.82,0,0)))*.472
        # Source props have missing image dependencies. Explicit neutral metal
        # and dark scabbard material avoids misleading magenta in previews.
        prop.data.materials.clear();mat=bpy.data.materials.new(filename+'_PreviewMaterial_v4')
        mat.diffuse_color=(.58,.61,.65,1) if filename=='Saber' else (.055,.045,.035,1)
        mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=mat.diffuse_color
        bs.inputs['Metallic'].default_value=.8;bs.inputs['Roughness'].default_value=.3
        prop.data.materials.append(mat);props.append(prop)
    saber,scabbard=props
    # Palm anchor is measured from the hand-weighted geometry in bind space.
    reset();hand=rig.data.bones['mixamorig:RightHand']; handworld=rig.matrix_world@hand.matrix_local
    vv=[v.co for v in mesh.data.vertices if any(mesh.vertex_groups[g.group].name==hand.name and g.weight>.85 for g in v.groups)]
    palm=sum(vv,Vector())/len(vv)
    # Move the grip slightly toward the wrist from the mitten centroid.
    palm=palm.lerp(handworld.translation,.20)
    palm_local=handworld.inverted()@palm
    pose('a')
    ph=rig.matrix_world@rig.pose.bones[hand.name].matrix
    # Upright blade with a small rearward inclination at the right shoulder.
    orient=Vector((1,0,0)).rotation_difference(Vector((0,.12,1)).normalized())
    worldprop=Matrix.LocRotScale(ph@palm_local,orient,Vector((1,1,1)))
    saber_offset=ph.inverted()@worldprop
    # Correct wrist roll only once in the authored pose, then keep one fixed
    # palm attachment for every frame and every pose.
    def attach():
        ph=rig.matrix_world@rig.pose.bones[hand.name].matrix
        saber.matrix_world=ph@saber_offset
        hip=rig.matrix_world@rig.pose.bones['mixamorig:Hips'].matrix
        scabbard.matrix_world=hip@scabbard_offset
    reset();hip=rig.matrix_world@rig.pose.bones['mixamorig:Hips'].matrix
    scabworld=Matrix.LocRotScale(Vector((.205,.06,1.025)),Vector((1,0,0)).rotation_difference(Vector((.08,.22,-1))),Vector((1,1,1)))
    scabbard_offset=hip.inverted()@scabworld
    # Rest and order hand orientations drive the fixed socket, never a
    # per-pose prop offset. Match desired blade direction by rotating the hand.
    def pose_with_saber(kind,phase=0):
        pose(kind,phase)
        pb=rig.pose.bones[hand.name]
        current=rig.matrix_world@pb.matrix@saber_offset
        if kind=='a':direction=Vector((0,.12,1))
        elif kind=='b':direction=Vector((-.1,-1,.52))
        else:
            # Ground tip beside the right boot. Source tip x=.993 after grip.
            grip=current.translation;direction=Vector((grip.x,-.10,.025))-grip
        rot=current.to_3x3()@Vector((1,0,0));q=rot.rotation_difference(direction.normalized())
        hw=rig.matrix_world@pb.matrix
        pb.matrix=rig.matrix_world.inverted()@Matrix.LocRotScale(hw.translation,q@hw.to_quaternion(),hw.to_scale())
        bpy.context.view_layer.update();attach()
    bpy.context.preferences.filepaths.file_preview_type='NONE'
    scene=bpy.context.scene;scene.render.fps=30;scene.frame_start=1;scene.frame_end=31
    anim=(DEST if DRAFT else OUT)/'Anim_v4';anim.mkdir(exist_ok=True,parents=True)
    # Save three reviewable pose/animation scenes, preserving the same skeleton.
    # March is an in-place one-second test cycle, with opposite arm/leg swing.
    for kind,label in [('a','March_Skulder'),('b','Ordre'),('c','Hvil')]:
        frames=range(1,32) if kind=='a' else [1,31]
        action=bpy.data.actions.new(f'{nat}_{label}_v4')
        for f in frames:
            pose_with_saber(kind,2*math.pi*(f-1)/30)
            rig.animation_data_create();rig.animation_data.action=action
            for pb in rig.pose.bones:
                pb.rotation_mode='QUATERNION'
                pb.keyframe_insert('rotation_quaternion',frame=f)
                pb.keyframe_insert('location',frame=f)
        rig.animation_data.action=action
        # Child Of constraints use full bone matrices without Blender's
        # bone-parent tail offset; the props are not part of the body export.
        for prop,bone,offset in [(saber,hand.name,saber_offset),(scabbard,'mixamorig:Hips',scabbard_offset)]:
            prop.constraints.clear();con=prop.constraints.new('CHILD_OF');con.target=rig;con.subtarget=bone
            con.inverse_matrix=Matrix.Identity(4);prop.matrix_basis=offset
        scene.frame_set(1);bpy.context.view_layer.update()
        bpy.ops.wm.save_as_mainfile(filepath=str(anim/f'{nat}_{label}_Reviewed_v4.blend'))
        for prop in props:prop.constraints.clear()
    # Contact sheet: three static evaluated meshes, keeping separate props.
    rig.animation_data_clear()
    for i,kind in enumerate(['a','b','c']):
        pose_with_saber(kind,0)
        deps=bpy.context.evaluated_depsgraph_get()
        for obj in [mesh,saber,scabbard]:
            ev=obj.evaluated_get(deps);data=bpy.data.meshes.new_from_object(ev)
            copy=bpy.data.objects.new(obj.name+'_'+kind+'_v4',data);scene.collection.objects.link(copy)
            copy.matrix_world=obj.matrix_world.copy();copy.location.x+=(i-1)*1.45
    for obj in [mesh,rig,saber,scabbard]:obj.hide_render=True
    scene.render.engine='BLENDER_WORKBENCH';scene.display.shading.light='STUDIO';scene.display.shading.color_type='TEXTURE'
    scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
    scene.display.shading.background_type='WORLD';scene.world=bpy.data.worlds.new('World_v4');scene.world.color=(.16,.18,.21)
    cam=bpy.data.objects.new('Camera_v4',bpy.data.cameras.new('Camera_v4'));scene.collection.objects.link(cam);scene.camera=cam
    def render(filename,target,scale,res):
        cam.location=Vector(target)+Vector((2.0,-7,2.0));cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
        cam.data.type='ORTHO';cam.data.ortho_scale=scale
        scene.render.resolution_x,scene.render.resolution_y=res;scene.render.resolution_percentage=100
        scene.render.filepath=str(dst/filename);bpy.ops.render.render(write_still=True)
    render('preview_v4.png',(0,-.1,1.02),5.2,(1950,1000))
    for i,kind in enumerate(['a','b','c']):
        render('arms_'+kind+'_v4.png',((i-1)*1.45,-.09,1.23),1.05,(800,850))
    def offset_report(m):
        return dict(matrix_blender_bone_local=[list(row) for row in m],translation_bone_units=list(m.translation),rotation_quaternion_wxyz=list(m.to_quaternion()),scale=list(m.to_scale()))
    report=dict(nation=nat,bones=41,triangles=triangles(mesh),max_influences=max(len(v.groups) for v in mesh.data.vertices),
      unweighted=sum(not v.groups for v in mesh.data.vertices),saber=offset_report(saber_offset),scabbard=offset_report(scabbard_offset),
      palm_bind_world_m=list(palm),march_duration_s=1,march_root_motion_cm=0,march_speed_cm_s=0)
    (dst/'metrics_v4.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    (dst/'notes.md').write_text(f'''# {nat} officer v4

Armrettelse: v3 bagte ændrede bone-roll-matricer fra en tilpasset A-pose ind i infanteriets bindepose. Sammen med skarpe inverse-afstands-vægte gav det drejede og indsnævrede ærmer. V4 genbruger den ubagte v2-geometri, samme reduktion til {triangles(mesh)} trekanter og originale UV/tekstur. Armene tilpasses med korteste svingrotation uden aksialt roll; hånden følger underarmens orientering. Kontinuerte albue-/håndledsvægte, fire udglatningspassager, højst fire vægte og normalisering. Infanteriets 41 navne, forældre og bindematricer er bevaret. Ingen ekstra socket-knogler. LBS, ikke Blender-only preserve volume.

Poser i ../Anim_v4/: {nat}_March_Skulder_Reviewed_v4.blend, {nat}_Ordre_Reviewed_v4.blend, {nat}_Hvil_Reviewed_v4.blend. 30 fps. March: 31 frames, 1 sekund, in-place, root motion 0 cm, målt root-hastighed 0 cm/s. Testcyklus, ikke produktionsgang med fodkontakt. Ordre og hvil er holdte testposer. Ingen rifleklip anvendt.

Sabel og tom skede er separate objekter fra Saber.fbx og Scabbard.fbx. Kildens lange X-akse skaleres med 0,472 og grip-origin flyttes fra (-0,82;0;0) til nul. Samlet sabellængde ca. 94 cm. Sabel: mixamorig:RightHand. Skede: mixamorig:Hips, venstre hofte. Den samme faste matrix anvendes i alle poser; Child Of i blend-filer. Præcise offsets findes i metrics_v4.json:

Sabel bone-local matrix: {json.dumps(report['saber'],ensure_ascii=False)}

Skede bone-local matrix: {json.dumps(report['scabbard'],ensure_ascii=False)}

Blender-koordinater: meter i verden, knoglelokale translationer i riggens importerede enheder; brug matrixen sammen med FBX-importens akse-/enhedskonvertering, ikke som direkte Unreal Rotator. Rig object matrix: {json.dumps([list(row) for row in rig.matrix_world])}. FBX eksport -Z frem/Y op, som v3. Unreal-socket er ikke oprettet eller valideret.

Historisk grundlag: Exerceer-Reglement for det kongelige danske Infanteri, 1863, §26, Greb med Sablen, https://media.1864.dk/data/photos/11351.pdf . Skulder og sabel ved foden er fortolkninger af reglementet; fremadrettet ordrepose er en illustrativ kommando, ikke dokumenteret eksercergreb. Samme danske testposer bruges til SE, NO og PR, uden påstand om nationalt korrekt eksercits.

Resterende fejl: kildens fingre er sammenvoksede vantehænder og kan ikke lukke anatomisk om fæstet; grebet er placeret i håndfladen, men individuel fingerkontakt er ikke løst. Bløde ansigts-/uniformsdetaljer, fladt gehæng og frakkeskøder uden cloth. Sabre-/skedeteksturer mangler i kilde-FBX; previews bruger neutrale materialer. Ingen Unreal-test. Nationsspecifik visuel vurdering tilføjes efter renderkontrol.
''',encoding='utf-8')
    print('V4_RESULT',json.dumps(report),flush=True)

for nation in (['PR'] if DRAFT else ['DK','SE','NO','PR']):build(nation)
