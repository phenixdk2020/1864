# The colour (fane) model with any flag on it: SourceAssets/Units1864/Flag_Standard_Split.fbx is the flag model split
# in Blender into the pole (slot 0, the model's own texture) and the cloth (slot 1, flat UVs: the pole side u = 0,
# the top v = 0). The cloth gets M_FlagCloth, whose texture parameter "Flag" takes any flag picture; every PNG in
# SourceAssets/Units1864/Flags becomes /Game/Units/Flags/T_<name> (Flag_DK, Flag_SE, ...).
# Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file>
import os, unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(root, 'SourceAssets', 'Units1864')
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def import_file(filename, dest, name, options=None):
    task = unreal.AssetImportTask()
    for k, v in [('filename', filename), ('destination_path', dest), ('destination_name', name),
                 ('automated', True), ('replace_existing', True), ('save', True)]:
        task.set_editor_property(k, v)
    if options:
        task.set_editor_property('options', options)
        task.set_editor_property('factory', unreal.FbxFactory())
    tools.import_asset_tasks([task])
    return unreal.load_asset(dest + '/' + name)


# The flags.
flags = os.path.join(source, 'Flags')
for f in sorted(os.listdir(flags)):
    if f.lower().endswith('.png'):
        tex = import_file(os.path.join(flags, f), '/Game/Units/Flags', 'T_' + os.path.splitext(f)[0])
        tex.set_editor_property('srgb', True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
        unreal.log('FLAG-IMPORT texture ' + tex.get_path_name())

# The cloth material: the flag texture, two-sided, cloth-rough.
material = unreal.load_asset('/Game/Units/Flags/M_FlagCloth')
if not material:
    material = tools.create_asset('M_FlagCloth', '/Game/Units/Flags', unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
material.set_editor_property('two_sided', True)
param = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
param.set_editor_property('parameter_name', 'Flag')
param.set_editor_property('texture', unreal.load_asset('/Game/Units/Flags/T_Flag_DK'))
mel.connect_material_property(param, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
rough = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 240)
rough.set_editor_property('r', 0.95)
mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)

# The model, replacing the unsplit import.
options = unreal.FbxImportUI()
for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
             ('import_textures', False), ('automated_import_should_detect_type', False),
             ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
    options.set_editor_property(k, v)
options.static_mesh_import_data.set_editor_property('combine_meshes', True)
mesh = import_file(os.path.join(source, 'Flag_Standard_Split.fbx'), '/Game/Units/Items', 'SM_Flag_Standard', options)
slots = list(mesh.get_editor_property('static_materials'))
pole = unreal.load_asset('/Game/Units/Items/M_Flag_Standard')
for slot in slots:
    cloth = 'cloth' in str(slot.get_editor_property('material_slot_name')).lower()
    slot.set_editor_property('material_interface', material if cloth else pole)
    unreal.log('FLAG-IMPORT slot %s -> %s' % (slot.get_editor_property('material_slot_name'), 'cloth' if cloth else 'pole'))
mesh.set_editor_property('static_materials', slots)
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
box = mesh.get_bounding_box()
unreal.log('FLAG-IMPORT DONE %d slots, bounds %s .. %s' % (len(slots), str(box.min), str(box.max)))
