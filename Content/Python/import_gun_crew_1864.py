# Imports the 1864 gun crew clips (SourceAssets/Artillery/Crew/A_Gun_*.fbx, NOT *.prev.fbx; see notes.md there) as animations on
# /Game/Units/Human/Skeletons/SK_Human_1864 and the props (SourceAssets/Artillery/Props: SM_Sponge, SM_Rammer, SM_Cannonball_12pdr,
# see README.md there) as static meshes, all into /Game/Units/Artillery/Crew. Re-runnable: an existing asset is first copied to
# /Game/Units/Artillery/Crew/Backup_<date>/ (once per date), then deleted (a replace-import keeps old settings).
# Run (PowerShell, no editor/game running):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<this file> -unattended -nosplash -stdout
# Env vars:
#   GUN_CREW_CLIPS_ONLY=A_Gun_Aim,A_Gun_Ram  import only those clips (props untouched)
#   GUN_CREW_PROPS_ONLY=1                     import only the props
#   GUN_CREW_ROLL=90                          clip import roll (default 90 like the officer; the horse showed roll must be verified)
#   GUN_CREW_FIX_HIPS=0                       disable the hips-height repair (default on, see fix_hips_height)
# Orientation: clips use import_rotation roll=90 like the officer (Blender adds an 'Armature' root). Verified by the hips bone height at
# frame 0 (should be ~90-100 cm, Z up) compared with A_Combat_* references; a CHECK line (OK / SUSPECT) is logged per clip.
import os, datetime
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC_CLIPS = os.path.join(ROOT, 'SourceAssets', 'Artillery', 'Crew')
SRC_PROPS = os.path.join(ROOT, 'SourceAssets', 'Artillery', 'Props')
DEST = '/Game/Units/Artillery/Crew'
BACKUP = DEST + '/Backup_' + datetime.date.today().strftime('%Y-%m-%d')
SKELETON_PATH = '/Game/Units/Human/Skeletons/SK_Human_1864'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
CLIPS = ['A_Gun_StandBy', 'A_Gun_Sponge', 'A_Gun_Load', 'A_Gun_Ram', 'A_Gun_Prime', 'A_Gun_Aim', 'A_Gun_Push_Loop']
PROPS = ['SM_Sponge', 'SM_Rammer', 'SM_Cannonball_12pdr']
ROLL = float(os.environ.get('GUN_CREW_ROLL', '90'))
CLIP_ROTATION = unreal.Rotator(roll=ROLL, pitch=0, yaw=0)
# Hips rest height missing in the gun FBX clips: A_Officer_Idle hips Lcl Translation Y 93.45079803 minus A_Gun_StandBy -4.62825918 (same
# pose: identical x/z and spine; read from the FBX files). Correct this in Tools/Animation/build_gun_crew_clips_1864.py and re-export, then
# set GUN_CREW_FIX_HIPS=0.
HIPS_REST_Z = 98.07905721
LOG = 'GUNCREW-IMPORT '
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


def import_texture(file, name, srgb):
    remove_with_backup(DEST + '/' + name, name)
    run_task(os.path.join(SRC_PROPS, file), DEST, name, None, texture=True)
    tex = unreal.load_asset(DEST + '/' + name)
    if not tex:
        fail('texture ' + name)
        return None
    tex.set_editor_property('srgb', srgb)
    if not srgb:
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
    EAL.save_loaded_asset(tex)
    return tex


def prop_material(prop):
    short = prop[3:]  # Sponge / Rammer / Cannonball_12pdr
    mat_name = 'M_' + short
    base = import_texture('T_%s_BaseColor.png' % short, 'T_%s_BaseColor' % short, True)
    rough = import_texture('T_%s_Roughness.png' % short, 'T_%s_Roughness' % short, False)
    remove_with_backup(DEST + '/' + mat_name, mat_name)
    material = TOOLS.create_asset(mat_name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    if base:
        s = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, 0)
        s.set_editor_property('texture', base)
        mel.connect_material_property(s, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    if rough:
        s = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, 300)
        s.set_editor_property('texture', rough)
        s.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        mel.connect_material_property(s, 'R', unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material)
    EAL.save_loaded_asset(material)
    return material


def import_prop(name):
    remove_with_backup(DEST + '/' + name, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
        ui.set_editor_property(k, v)
    ui.static_mesh_import_data.set_editor_property('combine_meshes', True)
    run_task(os.path.join(SRC_PROPS, name + '.fbx'), DEST, name, ui)
    mesh = unreal.load_asset(DEST + '/' + name)
    if not isinstance(mesh, unreal.StaticMesh):
        fail(name + ' missing')
        return
    material = prop_material(name)
    if material:
        slots = list(mesh.get_editor_property('static_materials'))
        for s in slots:
            s.set_editor_property('material_interface', material)
        mesh.set_editor_property('static_materials', slots)
    EAL.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    size = box.max - box.min
    say('%s OK size(cm) X=%.1f Y=%.1f Z=%.1f min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f) tris=%d slots=%d' % (
        name, size.x, size.y, size.z, box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z,
        unreal.EditorStaticMeshLibrary.get_number_triangles(mesh, 0) if hasattr(unreal.EditorStaticMeshLibrary, 'get_number_triangles') else -1,
        len(mesh.get_editor_property('static_materials'))))


def hips_name(seq):
    names = [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(seq)]
    return next((n for n in names if n.lower().endswith('hips')), names[0])


def hips_loc(seq, frame):
    return unreal.AnimationLibrary.get_bone_pose_for_frame(seq, hips_name(seq), frame, False).translation


def clip_text(seq):
    n = max(int(unreal.AnimationLibrary.get_num_frames(seq)), 1)
    a, m, z = hips_loc(seq, 0), hips_loc(seq, n // 2), hips_loc(seq, n - 1)
    ok = 'OK' if 85.0 <= a.z <= 105.0 else 'SUSPECT(hips z f0 not ~90-100 cm; check roll / crouch pose?)'
    return 'length=%.3fs frames=%d hips f0=(%.1f %.1f %.1f) mid=(%.1f %.1f %.1f) last=(%.1f %.1f %.1f) CHECK=%s' % (
        seq.get_play_length(), n, a.x, a.y, a.z, m.x, m.y, m.z, z.x, z.y, z.z, ok)


def fix_hips_height(seq, skeleton):
    """The gun FBX clips store the Hips translation as an offset from the bind pose (hips z f0 ~ -5 cm instead of ~93 cm like the
    officer's: Lcl Translation 93.45 in A_Officer_Idle vs -4.63 in A_Gun_StandBy, same pose). Without repair the crew sinks ~98 cm
    into the ground. Only when f0 z is < 50 cm: add HIPS_REST_Z to every hips key (rotation/scale untouched). The skeleton's own
    reference pose cannot be used (SK_Human_1864 bind hips z = -8.2). Returns the added cm or 0."""
    if os.environ.get('GUN_CREW_FIX_HIPS', '1') == '0':
        return 0.0
    if hips_loc(seq, 0).z >= 50.0:
        return 0.0
    hips = hips_name(seq)
    add = float(os.environ.get('GUN_CREW_HIPS_REST_Z', HIPS_REST_Z))
    keys = int(unreal.AnimationLibrary.get_num_keys(seq))
    pos, rot, scl = [], [], []
    for i in range(keys):
        t = unreal.AnimationLibrary.get_bone_pose_for_frame(seq, hips, i, False)
        pos.append(unreal.Vector(t.translation.x, t.translation.y, t.translation.z + add))
        rot.append(t.rotation)
        scl.append(t.scale3d)
    ctl = seq.get_editor_property('controller')
    ctl.open_bracket(unreal.Text('gun crew hips height repair'), False)
    ctl.set_bone_track_keys(hips, pos, rot, scl)
    ctl.close_bracket(False)
    say('HIPSFIX %s +%.2f cm (HIPS_REST_Z) on %d keys' % (seq.get_name(), add, keys))
    return add


def import_clip(name, skeleton):
    remove_with_backup(DEST + '/' + name, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_mesh', False), ('import_as_skeletal', True), ('import_animations', True), ('import_materials', False),
                 ('import_textures', False), ('skeleton', skeleton), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_ANIMATION), ('override_animation_name', name)]:
        ui.set_editor_property(k, v)
    ui.anim_sequence_import_data.set_editor_property('import_rotation', CLIP_ROTATION)
    run_task(os.path.join(SRC_CLIPS, name + '.fbx'), DEST, name, ui)
    seq = unreal.load_asset(DEST + '/' + name)
    if not isinstance(seq, unreal.AnimSequence):
        fail(name + ' missing')
        return None
    if seq.get_editor_property('skeleton') != skeleton:
        fail(name + ' not on SK_Human_1864')
    say('%s RAW (as imported) %s' % (name, clip_text(seq)))
    fix_hips_height(seq, skeleton)
    EAL.save_loaded_asset(seq)
    return seq


def main():
    skeleton = unreal.load_asset(SKELETON_PATH)
    if not skeleton:
        raise RuntimeError('SK_Human_1864 missing')
    only = [c.strip() for c in os.environ.get('GUN_CREW_CLIPS_ONLY', '').split(',') if c.strip()]
    props_only = os.environ.get('GUN_CREW_PROPS_ONLY', '') not in ('', '0')
    clips = [] if props_only else ([c for c in CLIPS if c in only] if only else CLIPS)
    props = [] if only else PROPS
    say('roll=%.1f clips=%s props=%s' % (ROLL, ','.join(clips), ','.join(props)))
    if clips:
        for r in ['A_Combat_Idle', 'A_Combat_WalkShoulder', 'A_Combat_DeathFront']:
            ref = unreal.load_asset('/Game/Units/Human/CombatAnimations/' + r)
            if ref:
                say('REF %s %s' % (r, clip_text(ref)))
    for name in props:
        import_prop(name)
    for name in clips:
        seq = import_clip(name, skeleton)
        if seq:
            say('%s %s' % (name, clip_text(seq)))
    for name in clips + props:
        if not EAL.does_asset_exist(DEST + '/' + name):
            fail('asset missing ' + name)
    EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    say('DONE problems=%d %s' % (len(failures), '; '.join(failures)))


main()
