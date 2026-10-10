# Imports ONLY the three wagons (supply, empty, mortar wagon) of SourceAssets/Units1864 as static meshes under /Game/Units/Items (the other items are in import_units_1864.py). Imports the PROJECT 1864 unit models and items from SourceAssets/Units1864 (the FBX files with the texture taken
# from the matching GLB): the Danish line infantry and the 6th Regiment's jæger on the shared human skeleton, and
# the flag, the field gun, the mortar, the saddle, the scabbard, the sabre and the (unrigged) horse as static meshes.
# Each gets a textured material. Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file>
import os, unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(root, 'SourceAssets', 'Units1864')
tools = unreal.AssetToolsHelpers.get_asset_tools()
def texture_material(name, dest, skeletal):
    task = unreal.AssetImportTask()
    for k, v in [('filename', os.path.join(source, name + '_Texture_0.png')), ('destination_path', dest), ('destination_name', 'T_' + name),
                 ('automated', True), ('replace_existing', True), ('save', True)]:
        task.set_editor_property(k, v)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(dest + '/T_' + name)
    assert texture, 'texture missing for ' + name
    material = unreal.load_asset(dest + '/M_' + name)
    if not material:
        material = tools.create_asset('M_' + name, dest, unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property('texture', texture)
    unreal.MaterialEditingLibrary.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 220)
    rough.set_editor_property('r', 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    if skeletal:
        unreal.MaterialEditingLibrary.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    unreal.MaterialEditingLibrary.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(material)
    return material


def assign(mesh, material):
    slots = list(mesh.get_editor_property('static_materials' if isinstance(mesh, unreal.StaticMesh) else 'materials'))
    for slot in slots:
        slot.set_editor_property('material_interface', material)
    mesh.set_editor_property('static_materials' if isinstance(mesh, unreal.StaticMesh) else 'materials', slots)



def import_static(name, dest, asset):
    target = dest + '/' + asset
    if not unreal.EditorAssetLibrary.does_asset_exist(target):
        options = unreal.FbxImportUI()
        for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                     ('import_textures', False), ('automated_import_should_detect_type', False),
                     ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
            options.set_editor_property(k, v)
        options.static_mesh_import_data.set_editor_property('combine_meshes', True)
        task = unreal.AssetImportTask()
        for k, v in [('filename', os.path.join(source, name + '.fbx')), ('destination_path', dest), ('destination_name', asset),
                     ('automated', True), ('save', True), ('options', options), ('factory', unreal.FbxFactory())]:
            task.set_editor_property(k, v)
        tools.import_asset_tasks([task])
    mesh = unreal.load_asset(target)
    assert isinstance(mesh, unreal.StaticMesh), name + ': static mesh missing'
    assign(mesh, texture_material(name, dest, False))
    box = mesh.get_bounding_box()
    unreal.log('UNITS-IMPORT PASS static %s size %s' % (target, str(box.max - box.min)))
    unreal.EditorAssetLibrary.save_directory(dest, only_if_is_dirty=False, recursive=True)

for name, asset in [('Wagon_Supply', 'SM_Wagon_Supply'), ('Wagon_Empty', 'SM_Wagon_Empty'), ('Wagon_Mortar', 'SM_Wagon_Mortar')]:
    try:
        import_static(name, '/Game/Units/Items', asset)
    except Exception as e:
        unreal.log_error('WAGON-IMPORT FAIL %s: %s' % (name, e))
unreal.log('WAGON-IMPORT DONE')
