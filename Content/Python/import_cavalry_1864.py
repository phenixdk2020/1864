# Imports the five rigged 1864 cavalry riders (husars, dragoons; 2026-11-12) from SourceAssets/Units1864/Cavalry/Final_v1 (see notes.md and report.json there) into
# /Game/Units/Cavalry/Riders: SK_Cavalry_1864_{DK,NO,SE,PR} on the existing skeleton SK_Human_1864 (never changed), plus
# M_Cavalry_{NAME} and T_Cavalry_{NAME} (from SK_Cavalry_1864_{NAT}_Texture_0.png). Re-runnable: an existing asset is first copied to
# /Game/Units/Cavalry/Riders/Backup_<date>/ (once per date), then deleted (a replace-import keeps old settings).
# Run (PowerShell, headless, no game running):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<this file> -unattended -nosplash -stdout
#
# Orientation: import_rotation/scale 0 / 1 like import_officer_1864.py / import_units_1864.py. The armature is the soldier's (scale 0.01,
# no offset, notes.md), so the bind pose is the soldier's: ~199 cm tall, z -100..99 like SK_Officer_1864_DK. Measured below (CAVALRY-IMPORT).
# The texture has baked-in specular highlights, so the material is rough (0.85).
import os, datetime
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, 'SourceAssets', 'Units1864', 'Cavalry', 'Final_v1')
DEST = '/Game/Units/Cavalry/Riders'
BACKUP = DEST + '/Backup_' + datetime.date.today().strftime('%Y-%m-%d')
SKELETON_PATH = '/Game/Units/Human/Skeletons/SK_Human_1864'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
MESHES = ['SE_Husar', 'PR_Husar', 'PR_Dragon', 'SE_Dragon', 'DK_Dragon']
ROUGHNESS = 0.85
LOG = 'CAVALRY-IMPORT '
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
    tex_name, mat_name = 'T_Cavalry_' + tag, 'M_Cavalry_' + tag
    remove_with_backup(DEST + '/' + tex_name, tex_name)
    remove_with_backup(DEST + '/' + mat_name, mat_name)
    png = os.path.join(SRC, 'SK_Cavalry_1864_%s_Texture_0.png' % tag)
    if not os.path.isfile(png):
        fail('texture file missing ' + png)
        return None
    run_task(png, DEST, tex_name, None, texture=True)
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
    rough.set_editor_property('r', ROUGHNESS)
    mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    mel.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    mel.recompile_material(material)
    EAL.save_loaded_asset(material)
    return material


def mesh_bounds_text(mesh):
    b = mesh.get_bounds()
    o, e = b.origin, b.box_extent
    return 'zmin=%.1f zmax=%.1f height=%.1f xrange=%.1f yrange=%.1f cm' % (
        o.z - e.z, o.z + e.z, 2 * e.z, 2 * e.x, 2 * e.y)


def material_check_text(mesh):
    """Reads back: slot materials, and for each the texture in the TextureSample expression and the roughness constant."""
    out = []
    for s in mesh.get_editor_property('materials'):
        mi = s.get_editor_property('material_interface')
        if not mi:
            out.append('slot=None')
            continue
        tex = '-'
        try:
            expr = unreal.MaterialEditingLibrary.get_material_property_input_node(mi, unreal.MaterialProperty.MP_BASE_COLOR)
            t = expr.get_editor_property('texture') if expr else None
            tex = t.get_name() if t else 'None'
        except Exception as ex:
            tex = 'unreadable(%s)' % ex
        rough = '?'
        try:
            rexpr = unreal.MaterialEditingLibrary.get_material_property_input_node(mi, unreal.MaterialProperty.MP_ROUGHNESS)
            rough = '%.2f' % rexpr.get_editor_property('r') if rexpr else 'none'
        except Exception as ex:
            rough = 'unreadable(%s)' % ex
        out.append('slot=%s texture=%s roughness=%s' % (mi.get_name(), tex, rough))
    return '; '.join(out)


def import_mesh(tag, skeleton):
    name = 'SK_Cavalry_1864_' + tag
    remove_with_backup(DEST + '/' + name, name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', True), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('create_physics_asset', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_SKELETAL_MESH), ('skeleton', skeleton)]:
        ui.set_editor_property(k, v)
    run_task(os.path.join(SRC, name + '.fbx'), DEST, name, ui)
    mesh = unreal.load_asset(DEST + '/' + name)
    if not isinstance(mesh, unreal.SkeletalMesh):
        fail(name + ' missing')
        return
    if mesh.get_editor_property('skeleton') != skeleton:
        fail(name + ' not on SK_Human_1864')
    material = texture_material(tag)
    if material:
        slots = list(mesh.get_editor_property('materials'))
        for s in slots:
            s.set_editor_property('material_interface', material)
        mesh.set_editor_property('materials', slots)
    EAL.save_loaded_asset(mesh)
    say('%s OK %s slots=%d | %s' % (name, mesh_bounds_text(mesh), len(mesh.get_editor_property('materials')),
                                    material_check_text(mesh)))


def main():
    skeleton = unreal.load_asset(SKELETON_PATH)
    if not skeleton:
        raise RuntimeError('SK_Human_1864 missing')
    # References: soldier and officer meshes (expect ~199 cm, z -100..99)
    for ref in ['/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864', '/Game/Units/Human/Officer/SK_Officer_1864_DK']:
        m = unreal.load_asset(ref)
        if m:
            say('REF %s %s' % (ref.split('/')[-1], mesh_bounds_text(m)))
    for tag in MESHES:
        import_mesh(tag, skeleton)
    for tag in MESHES:
        for name in ['SK_Cavalry_1864_' + tag, 'M_Cavalry_' + tag, 'T_Cavalry_' + tag]:
            if not EAL.does_asset_exist(DEST + '/' + name):
                fail('asset missing ' + name)
    EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    say('DONE problems=%d %s' % (len(failures), '; '.join(failures)))


main()


