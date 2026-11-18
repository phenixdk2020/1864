"""Headless Blender 5.2: officer reduction, existing infantry skeleton, FBX and QA.
Run: blender -b --python Tools/Animation/build_officers_v3.py
All outputs are new; refuses existing nation directories. No Unreal operations.
"""
import bpy, bmesh, math, json, shutil, traceback
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'SourceAssets/Units1864/Officer'

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
    dst = OUT / f'{nat}_Officer_1864_v3'
    if dst.exists() and any(dst.iterdir()): raise RuntimeError(f'Output exists: {dst}')
    dst.mkdir(exist_ok=True)
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
    mesh = bpy.context.object; mesh.name = f'{nat}_Officer_1864_v3'
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
    # Bake only the arm fit into infantry rest. Leave torso/head/legs intact:
    # full cage baking pinches collars, while no baking stretches wrists.
    arm_deltas={}
    for b in fitted.data.bones:
        if any(token in b.name for token in ('Arm','Hand')):
            target=rig.matrix_world@rig.data.bones[b.name].matrix_local
            target.translation+=Vector((0,0,.998))
            target.translation*=rig_scale
            target=Matrix.LocRotScale(target.translation,target.to_quaternion(),Vector((1,1,1)))
            arm_deltas[b.name]=target@b.matrix_local.inverted()
    for v in mesh.data.vertices:
        original_co=v.co.copy(); correction=Vector((0,0,0))
        fade=max(0,min(1,(abs(original_co.x)-.16)/.10))
        for g in v.groups:
            name=mesh.vertex_groups[g.group].name
            if name in arm_deltas:
                correction+=(arm_deltas[name]@original_co-original_co)*g.weight*fade
        v.co+=correction
    # Retain officer torso in its original A-pose. Weighting cage supplies
    # weights only; using its edit-bone offsets to deform the rest mesh causes
    # epaulette and ankle pinching. Bind directly to the unchanged shared rig.
    active(mesh)
    for mod in list(mesh.modifiers):
        if mod.type=='ARMATURE': mesh.modifiers.remove(mod)
    mesh.parent=None; mesh.matrix_world=Matrix.Identity(4)
    bpy.data.objects.remove(fitted,do_unlink=True)
    # Move the entire skinned assembly to ground; preserve rig/mesh relative bind.
    low,high=world_bounds(mesh)
    final_scale=1.80/(high[2]-low[2])
    assembly=Matrix.Diagonal((rig_scale,rig_scale,rig_scale,1))
    assembly.translation=Vector((0,0,.998*rig_scale))
    rig.matrix_world=assembly@rig.matrix_world
    mod=mesh.modifiers.new('InfantrySkin_v3','ARMATURE'); mod.object=rig
    weld_normals(mesh)
    assert len(rig.data.bones)==41
    for b in rig.data.bones:
        assert max(abs(b.matrix_local[i][j]-original[b.name][0][i][j]) for i in range(4) for j in range(4)) < 1e-5
        assert (b.parent.name if b.parent else None)==original[b.name][1]
    for mat in mesh.data.materials:
        if mat and mat.use_nodes:
            for node in mat.node_tree.nodes:
                if node.type=='TEX_IMAGE' and node.image:
                    assert tuple(node.image.size)==(2048,2048)
    texture=dst/f'{nat}_Officer_1864_Texture_0_v3.png'
    shutil.copy2(OUT/f'{nat}_Officer_1864_Texture_0_v2.png',texture)
    for mat in mesh.data.materials:
        if mat and mat.use_nodes:
            for node in mat.node_tree.nodes:
                if node.type=='TEX_IMAGE' and node.image:
                    node.image.filepath=str(texture)
    active(mesh); rig.select_set(True)
    fbx=dst/f'{nat}_Officer_1864_v3.fbx'
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'ARMATURE','MESH'},
        add_leaf_bones=False,use_armature_deform_only=False,bake_anim=False,
        axis_forward='-Z',axis_up='Y',apply_unit_scale=True,path_mode='RELATIVE')
    count=triangles(mesh)
    # Copy the A-pose mesh evaluated without the skin modifier for contact sheet.
    a_mesh=mesh.copy(); a_mesh.data=mesh.data.copy(); a_mesh.modifiers.clear()
    bpy.context.collection.objects.link(a_mesh); a_mesh.location.x=-.65
    # Import actual existing march and transfer FULL local joint transforms,
    # accounting for clips whose bind pose differs from the soldier FBX.
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'SourceAssets/InfantryCombat/A_Combat_Walk.fbx'))
    added=set(bpy.data.objects)-before
    cliprig=next(o for o in added if o.type=='ARMATURE')
    scene=bpy.context.scene; scene.render.fps=30
    start,end=scene.frame_start,scene.frame_end
    if cliprig.animation_data and cliprig.animation_data.action:
        start,end=map(int,cliprig.animation_data.action.frame_range)
    scene.frame_set(start+max(1,(end-start)//4))
    for pb in sorted(rig.pose.bones,key=lambda b:len(b.parent_recursive)):
        src=cliprig.pose.bones.get(pb.name)
        if src:
            src_local=src.parent.matrix.inverted()@src.matrix if src.parent else src.matrix
            dst_rest=pb.bone.parent.matrix_local.inverted()@pb.bone.matrix_local if pb.parent else pb.bone.matrix_local
            if pb.parent:
                pb.matrix_basis=dst_rest.inverted()@src_local
            else:
                # Different clip origin: preserve bind-root translation and use
                # rotational delta only for the stationary deformation test.
                pb.rotation_mode='QUATERNION'
                pb.rotation_quaternion=src.bone.matrix_local.to_quaternion().inverted()@src.matrix.to_quaternion()
    for o in added: bpy.data.objects.remove(o,do_unlink=True)
    mesh.location.x=.65; rig.location.x+=.65
    bpy.context.view_layer.update()
    deps=bpy.context.evaluated_depsgraph_get()
    evaluated=mesh.evaluated_get(deps)
    evmesh=evaluated.to_mesh()
    posed_coords=[mesh.matrix_world@v.co for v in evmesh.vertices]
    posed_bounds=[[min(v[i] for v in posed_coords),max(v[i] for v in posed_coords)] for i in range(3)]
    evaluated.to_mesh_clear()
    # Workbench textured preview; no GPU compute jobs, rendered sequentially.
    scene.render.engine='BLENDER_WORKBENCH'
    scene.display.shading.light='STUDIO'; scene.display.shading.color_type='TEXTURE'
    scene.display.shading.show_shadows=True; scene.display.shading.show_cavity=True
    scene.display.shading.background_type='WORLD'
    scene.world=bpy.data.worlds.new('PreviewWorld_v3'); scene.world.color=(.19,.21,.24)
    cam=bpy.data.objects.new('Camera_v3',bpy.data.cameras.new('Camera_v3'))
    scene.collection.objects.link(cam); scene.camera=cam
    cam.location=(2,-6,2.25); target=Vector((0,0,.92))
    cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO'; cam.data.ortho_scale=3.25; cam.data.clip_end=100
    scene.render.resolution_x=1280; scene.render.resolution_y=900; scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(dst/'preview_v3.png')
    bpy.ops.render.render(write_still=True)
    report=dict(nation=nat,triangles=count,bones=len(rig.data.bones),same_rest_names_hierarchy=True,
                source_skeleton='SourceAssets/Units1864/DK_Infantry_1864.fbx',
                auto_unweighted=auto_unweighted,corrected_vertices=fixed,uv_layers=len(mesh.data.uv_layers),
                texture=[2048,2048],height_m=1.8,posed_bounds=posed_bounds,
                clip='A_Combat_Walk.fbx',frame=scene.frame_current)
    (dst/'metrics_v3.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    (dst/'notes.md').write_text(f'''# {nat}-officer v3

- Trekanter: {count}. UV bevaret; original 2048 × 2048 base color kopieret uændret.
- {len(rig.data.bones)} knogler: identiske navne, hierarki og lokale bindematricer som DK_Infantry_1864.fbx (kontrolleret i script).
- Automatisk bone-heat skinning på midlertidigt tilpasset skelet; {fixed} vertices korrigeret (hoved/hat, bælte, vantehænder, ben/fødder og eventuelle uvægtede punkter). Geometrien bevarer sin oprindelige A-pose og bindes med disse vægte til infanteriets uændrede skelet; proportioner afviger lidt fra infanteriet.
- Højde 1,80 m inkl. hovedbeklædning, fødder ved Z=0. Z op i Blender, fremad -Y ligesom infanteriet. FBX -Z/Y, samme eksportakse som eksisterende infanteriklips; anvend eksisterende infanteri-importkonvention i senere Unreal-trin.
- preview_v3.png: A-pose til venstre, eksisterende A_Combat_Walk.fbx, frame {scene.frame_current}, til højre. Arme, ben og hoved skal vurderes visuelt; ingen Unreal-validering.
- Kendte kildedefekter: sammenvoksede fingre, bløde ansigtsdetaljer, fladt bælte/gehæng og afrundede støvler. Ingen fingeranimation; skelet er bevaret uden ekstra knogler. Frakkeskøder kan strække eller klippe i store skridt; ingen cloth-simulering.
- Ingen klip ændret eller leveret; 30 fps-testframe alene, ingen ny ganghastighed eller root motion.
''',encoding='utf-8')
    print('OFFICER_RESULT',json.dumps(report),flush=True)

for nation in ['DK','SE','NO','PR']:
    try: build(nation)
    except Exception:
        traceback.print_exc()
        raise
