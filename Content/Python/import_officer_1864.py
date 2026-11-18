# Imports the 1864 officer (captain) from SourceAssets/Units1864/Officer/Final_v5 (see notes.md and report.json there) into
# /Game/Units/Human/Officer: SK_Officer_1864_{DK,SE,PR,NO} (existing skeleton SK_Human_1864, never changed), SM_Officer_Saber/Scabbard
# and the clips A_Officer_{Walk,WalkSheathed,Idle,Order,Death,SabreDraw,SabreSheath}. Re-runnable: an existing asset is first copied to
# /Game/Units/Human/Officer/Backup_<date>/ (once per date), then deleted (a replace-import keeps old settings).
# Run (PowerShell, no editor/game running):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<this file> -unattended -nosplash -stdout
#
# Orientation choices (measured, see OFFICER-IMPORT log lines):
# - Meshes: import_rotation/scale 0 / 1 (as import_units_1864.py does for the soldiers). The Blender armature is exported with scale 0.01
#   and no offset exactly like the soldiers' (notes.md), so the bind pose is the soldier's: ~199 cm tall, matches SK_DK_Livgarden_1864.
# - Clips: import_rotation roll=90 like the colour bearer (import_colour_bearer.py; Blender adds an 'Armature' root that lays the clip down
#   in UE, roll +90 stands it up), import_uniform_scale 1. Verified by the hips bone height at frame 0 (should be ~90-100 cm), compared
#   with A_Combat_WalkShoulder / A_Combat_Idle / A_Combat_DeathFront.
import os, datetime
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, 'SourceAssets', 'Units1864', 'Officer', 'Final_v5')
DEST = '/Game/Units/Human/Officer'
BACKUP = DEST + '/Backup_' + datetime.date.today().strftime('%Y-%m-%d')
SKELETON_PATH = '/Game/Units/Human/Skeletons/SK_Human_1864'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
MESHES = ['DK', 'SE', 'PR', 'NO']
PROPS = [('SM_Officer_Saber', '/Game/Units/Items/M_Saber'), ('SM_Officer_Scabbard', '/Game/Units/Items/M_Scabbard')]
CLIPS = ['A_Officer_Walk', 'A_Officer_WalkSheathed', 'A_Officer_Idle', 'A_Officer_Order', 'A_Officer_Death', 'A_Officer_SabreDraw', 'A_Officer_SabreSheath']
CLIP_ROTATION = unreal.Rotator(roll=90, pitch=0, yaw=0)
LOG = 'OFFICER-IMPORT '
failures = []


def say(msg):
    unreal.log_warning(LOG + msg)


def fail(msg):
    failures.append(msg)
    unreal.log_error(LOG + 'PROBLEM ' + msg)


def remove_with_backup(path, name):
    if EAL.does_asset_exist(path):
        if not EAL.does_asset_exist(BACKUP + '/' + name):
            EAL.duplicate_asset(path, BACKUP + '/' + name)
        EAL.delete_asset(path)


def run_task(filename, dest, name, options, texture=False):
    t = unreal.AssetImportTask()
    for k, v in [('filename', filename), ('destination_path', dest), ('destination_name', name), ('automated', True),
                 ('replace_existing', True), ('save', True)]:
        t.set_editor_property(k, v)
    if not texture:
        t.set_editor_property('options', options)
        t.set_editor_property('factory', unreal.FbxFactory())
    TOOLS.import_asset_tasks([t])


def texture_material(tag):
    tex_name, mat_name = 'T_Officer_' + tag, 'M_Officer_' + tag
    remove_with_backup(DEST + '/' + tex_name, tex_name)
    remove_with_backup(DEST + '/' + mat_name, mat_name)
    run_task(os.path.join(SRC, 'SK_Officer_1864_%s_Texture_0.png' % tag), DEST, tex_name, None, texture=True)
    texture = unreal.load_asset(DEST + '/' + tex_name)
    if not texture:
        fail('texture ' + tex_name)
        return None
    material = TOOLS.create_asset(mat_name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property('texture', texture)
    mel.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 220)
    rough.set_editor_property('r', 0.85)
    mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    mel.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    mel.recompile_material(material)
    return material


def mesh_bounds_text(mesh):
    b = mesh.get_bounds()
    o, e = b.origin, b.box_extent
    return 'zmin=%.1f zmax=%.1f height=%.1f xrange=%.1f yrange=%.1f cm' % (
        o.z - e.z, o.z + e.z, 2 * e.z, 2 * e.x, 2 * e.y)


def import_mesh(tag, skeleton):
    name = 'SK_Officer_1864_' + tag
    remove_with_backup(DEST + '/' + name, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', True), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('create_physics_asset', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_SKELETAL_MESH), ('skeleton', skeleton)]:
        ui.set_editor_property(k, v)
    run_task(os.path.join(SRC, name + '.fbx'), DEST, name, ui)
    mesh = unreal.load_asset(DEST + '/' + name)
    if not isinstance(mesh, unreal.SkeletalMesh):
        fail(name + ' missing'); return
    if mesh.get_editor_property('skeleton') != skeleton:
        fail(name + ' not on SK_Human_1864')
    material = texture_material(tag)
    if material:
        slots = list(mesh.get_editor_property('materials'))
        for s in slots:
            s.set_editor_property('material_interface', material)
        mesh.set_editor_property('materials', slots)
    EAL.save_loaded_asset(mesh)
    say('%s OK %s slots=%d' % (name, mesh_bounds_text(mesh), len(mesh.get_editor_property('materials'))))


def import_prop(name, material_path):
    remove_with_backup(DEST + '/' + name, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
        ui.set_editor_property(k, v)
    ui.static_mesh_import_data.set_editor_property('combine_meshes', True)
    run_task(os.path.join(SRC, name + '.fbx'), DEST, name, ui)
    mesh = unreal.load_asset(DEST + '/' + name)
    if not isinstance(mesh, unreal.StaticMesh):
        fail(name + ' missing'); return
    material = unreal.load_asset(material_path)  # same material as the old SM_Saber/SM_Scabbard (no prop texture in Final_v5)
    if material:
        slots = list(mesh.get_editor_property('static_materials'))
        for s in slots:
            s.set_editor_property('material_interface', material)
        mesh.set_editor_property('static_materials', slots)
    EAL.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    say('%s OK size(cm) %s min %s max %s' % (name, str(box.max - box.min), str(box.min), str(box.max)))


def hips_name(seq):
    names = [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(seq)]
    return next((n for n in names if n.lower().endswith('hips')), names[0])


def hips_loc(seq, frame):
    t = unreal.AnimationLibrary.get_bone_pose_for_frame(seq, hips_name(seq), frame, False)
    return t.translation


def clip_text(seq):
    n = max(int(unreal.AnimationLibrary.get_num_frames(seq)), 1)
    a, m, z = hips_loc(seq, 0), hips_loc(seq, n // 2), hips_loc(seq, n - 1)
    return 'length=%.3fs frames=%d hips f0=(%.1f %.1f %.1f) mid=(%.1f %.1f %.1f) last=(%.1f %.1f %.1f)' % (
        seq.get_play_length(), n, a.x, a.y, a.z, m.x, m.y, m.z, z.x, z.y, z.z)


def import_clip(name, skeleton):
    remove_with_backup(DEST + '/' + name, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_mesh', False), ('import_as_skeletal', True), ('import_animations', True), ('import_materials', False),
                 ('import_textures', False), ('skeleton', skeleton), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_ANIMATION), ('override_animation_name', name)]:
        ui.set_editor_property(k, v)
    ui.anim_sequence_import_data.set_editor_property('import_rotation', CLIP_ROTATION)
    run_task(os.path.join(SRC, name + '.fbx'), DEST, name, ui)
    seq = unreal.load_asset(DEST + '/' + name)
    if not isinstance(seq, unreal.AnimSequence):
        fail(name + ' missing'); return
    if seq.get_editor_property('skeleton') != skeleton:
        fail(name + ' not on SK_Human_1864')
    EAL.save_loaded_asset(seq)
    return seq


def main():
    skeleton = unreal.load_asset(SKELETON_PATH)
    if not skeleton:
        raise RuntimeError('SK_Human_1864 missing')
    # Soldier references
    soldier = unreal.load_asset('/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864')
    say('REF SK_DK_Livgarden_1864 ' + mesh_bounds_text(soldier))
    for r in ['A_Combat_WalkShoulder', 'A_Combat_Idle', 'A_Combat_DeathFront', 'A_Combat_ColourWalk']:
        say('REF %s %s' % (r, clip_text(unreal.load_asset('/Game/Units/Human/CombatAnimations/' + r))))
    # OFFICER_CLIPS_ONLY=A_Officer_Idle,A_Officer_Order (env var): import only those clips, leave meshes/props/other clips untouched.
    only = [c.strip() for c in os.environ.get('OFFICER_CLIPS_ONLY', '').split(',') if c.strip()]
    clips = [c for c in CLIPS if c in only] if only else CLIPS
    if only:
        say('CLIPS ONLY: ' + ', '.join(clips))
    else:
        for tag in MESHES:
            import_mesh(tag, skeleton)
        for name, mat in PROPS:
            import_prop(name, mat)
    for name in clips:
        seq = import_clip(name, skeleton)
        if seq:
            say('%s OK %s' % (name, clip_text(seq)))
    for name in (clips if only else ['SK_Officer_1864_' + t for t in MESHES] + [p[0] for p in PROPS] + CLIPS):
        if not EAL.does_asset_exist(DEST + '/' + name):
            fail('asset missing ' + name)
    EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    say('DONE problems=%d %s' % (len(failures), '; '.join(failures)))


main()
