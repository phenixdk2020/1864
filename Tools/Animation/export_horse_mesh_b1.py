"""Exports the rigged horse (one armature + its mesh) to FBX for Unreal and measures the 7 horse clips.
blender -b <Horse_1_rigged_v3.blend> -P export_horse_mesh_b1.py
Writes SourceAssets/Horse/Horse_1_rigged_v3.fbx (previous version kept as .prev.fbx) and SourceAssets/Horse/horse_clip_report.json.
The blend holds a duplicate armature+mesh pair (.001); only Horse_1_Armature_v2.001 + Horse_1_Mesh.001 (active scene, same as the clips) are exported.
Axes as the infantry/rider clips: axis_forward -Z, axis_up Y, so the Blender -X (head) end becomes Unreal +Y; HorseYaw -90 turns it onto +X."""
import bpy, glob, json, os, shutil
from pathlib import Path
project = Path(__file__).resolve().parents[2]
out_dir = project / 'SourceAssets/Horse'
out_dir.mkdir(parents=True, exist_ok=True)
fbx = out_dir / 'Horse_1_rigged_v3.fbx'
if fbx.exists():
    shutil.copy2(fbx, out_dir / 'Horse_1_rigged_v3.prev.fbx')
# The active scene holds the .001 pair (the one the clips were made from); the other scene's pair is identical.
rig = bpy.data.objects['Horse_1_Armature_v2.001']
mesh = bpy.data.objects['Horse_1_Mesh.001']
twin = bpy.data.objects['Horse_1_Armature_v2']
drift = max((b.head_local - twin.data.bones[b.name].head_local).length for b in rig.data.bones)
print('TWIN_DRIFT', drift)
for o in bpy.data.objects: o.select_set(False)
rig.select_set(True); mesh.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
    bake_anim=False, axis_forward='-Z', axis_up='Y', apply_unit_scale=True, use_armature_deform_only=False)
print('HORSE_FBX', fbx, fbx.stat().st_size)

report = {}
for f in sorted(glob.glob(str(project / 'Reference/Animations/Horse/A_Horse_*.fbx'))):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=f)
    r = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = r.animation_data.action
    try: r.animation_data.action_slot = act.slots[0]
    except Exception: pass
    s, e = map(int, act.frame_range)
    root0 = None; rootN = None; hoof_speeds = []; prev = None
    hooves = [n for n in ('front_hoof.L', 'front_hoof.R', 'hind_hoof.L', 'hind_hoof.R')]
    pel_z = []
    series = {h: [] for h in hooves}
    for fr in range(s, e + 1):
        bpy.context.scene.frame_set(fr)
        rw = r.matrix_world @ r.pose.bones['root'].head
        if root0 is None: root0 = rw.copy()
        rootN = rw.copy()
        for h in hooves:
            series[h].append(r.matrix_world @ r.pose.bones[h].head)
        pel_z.append((r.matrix_world @ r.pose.bones['pelvis'].head).z)
    # stance speed: horizontal hoof speed (relative to root) while the hoof is on the ground (z close to its minimum)
    sp = []
    for h in hooves:
        zs = [p.z for p in series[h]]; zmin = min(zs)
        run = []
        for i, z in enumerate(zs + [99.0]):
            if z < zmin + 0.012:
                run.append(i)
            else:
                if len(run) >= 3:
                    sp.append(abs(series[h][run[-1]].x - series[h][run[0]].x) / ((run[-1] - run[0]) / 30.0) * 100.0)  # cm/s on the long axis
                run = []
    stance = sum(sp) / len(sp) if sp else 0.0
    n = e - s + 1
    report[os.path.basename(f)] = {'frames': n, 'fps': 30, 'length_s': round((n - 1) / 30.0, 3), 'bones': len(r.data.bones),
        'root_motion_cm': [round((rootN - root0).x * 100, 2), round((rootN - root0).y * 100, 2), round((rootN - root0).z * 100, 2)],
        'stance_hoof_speed_cm_s': round(stance, 1), 'pelvis_z_range_cm': [round(min(pel_z) * 100, 1), round(max(pel_z) * 100, 1)]}
(out_dir / 'horse_clip_report.json').write_text(json.dumps(report, indent=2))
print('HORSE_REPORT=' + json.dumps(report))
