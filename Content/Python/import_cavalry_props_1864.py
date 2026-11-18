# Imports the cavalry sabre and saddle (SourceAssets/Units1864/Cavalry/Props, see notes.md: Tools/Blender/build_cavalry_props_1864.py) as
# static meshes into /Game/Units/Cavalry/Props with a rough textured material each. The saddle is exported in the horse's axes with its origin
# on the bone saddle_root (hung on that socket with no offset); the sabre has its origin in the grip and the blade along +Z.
# Run (PowerShell, headless, no game running):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<this file> -unattended -nosplash -stdout
import os, datetime
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(ROOT, 'SourceAssets', 'Units1864', 'Cavalry', 'Props')
DEST = '/Game/Units/Cavalry/Props'
BACKUP = DEST + '/Backup_' + datetime.date.today().strftime('%Y-%m-%d')
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
LOG = 'CAVPROPS-IMPORT '
failures = []


def say(msg):
    unreal.log_warning(LOG + msg)


def remove_with_backup(name):
    path = DEST + '/' + name
    if EAL.does_asset_exist(path):
        if not EAL.does_asset_exist(BACKUP + '/' + name):
            EAL.duplicate_asset(path, BACKUP + '/' + name)
        EAL.delete_asset(path)


def run_task(filename, name, options=None):
    t = unreal.AssetImportTask()
    for k, v in [('filename', filename), ('destination_path', DEST), ('destination_name', name), ('automated', True),
                 ('replace_existing', True), ('save', True)]:
        t.set_editor_property(k, v)
    if options:
        t.set_editor_property('options', options)
        t.set_editor_property('factory', unreal.FbxFactory())
    TOOLS.import_asset_tasks([t])


def material_for(tag, png):
    tex_name, mat_name = 'T_' + tag, 'M_' + tag
    remove_with_backup(tex_name)
    remove_with_backup(mat_name)
    run_task(png, tex_name)
    texture = unreal.load_asset(DEST + '/' + tex_name)
    if not texture:
        failures.append('texture ' + tex_name)
        return None
    material = TOOLS.create_asset(mat_name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property('texture', texture)
    mel.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 220)
    rough.set_editor_property('r', 0.6 if 'Saber' in tag else 0.85)
    mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material)
    EAL.save_loaded_asset(material)
    return material


def import_static(name):
    remove_with_backup(name)
    ui = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
        ui.set_editor_property(k, v)
    ui.static_mesh_import_data.set_editor_property('combine_meshes', True)
    run_task(os.path.join(SRC, name + '.fbx'), name, ui)
    mesh = unreal.load_asset(DEST + '/' + name)
    if not isinstance(mesh, unreal.StaticMesh):
        failures.append(name + ' missing')
        return
    material = material_for(name.replace('SM_', ''), os.path.join(SRC, name + '_Texture_0.png'))
    if material:
        for i in range(mesh.get_num_sections(0)):
            mesh.set_material(i, material)
    EAL.save_loaded_asset(mesh)
    b = mesh.get_bounding_box()
    say('%s OK min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f) cm' % (name, b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z))


for prop in ['SM_Saber_1864', 'SM_Saddle_1864']:
    import_static(prop)
EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)
say('DONE problems=%d %s' % (len(failures), '; '.join(failures)))
