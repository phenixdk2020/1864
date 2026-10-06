# The ripe wheat of the battlefield (PROJECT 1864): imports T_Card_Wheat (Tools/Battle/make_wheat_card.py) and the
# SM_Wheat_* meshes (Tools/Battle/make_battle_meshes.py), and makes MI_Wheat from M_Foliage. Touches nothing else
# (import_battle_graphics.py rebuilds the whole look; this does only the wheat).
# Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=Content/Python/import_wheat.py
import os, unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
src = os.path.join(root, 'SourceAssets', 'Battle1864')
TEX, MESH, MAT = '/Game/Battle/Textures', '/Game/Battle/Foliage', '/Game/Battle/Materials'
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def import_one(path, dest, name, options=None):
    task = unreal.AssetImportTask()
    for k, v in [('filename', path), ('destination_path', dest), ('destination_name', name), ('automated', True), ('replace_existing', True), ('save', False)]:
        task.set_editor_property(k, v)
    if options:
        task.set_editor_property('options', options)
    tools.import_asset_tasks([task])


import_one(os.path.join(src, 'Textures', 'T_Card_Wheat.png'), TEX, 'T_Card_Wheat')
card = unreal.load_asset(TEX + '/T_Card_Wheat')
eal.save_loaded_asset(card)

mi = unreal.load_asset(MAT + '/MI_Wheat') or tools.create_asset('MI_Wheat', MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mi, unreal.load_asset(MAT + '/M_Foliage'))
for k, v in {'WindIntensity': 0.35, 'WindSpeed': 0.8, 'AOMin': 0.7, 'Brightness': 1.0, 'Translucency': 0.45, 'FadeStart': 9000.0, 'FadeLength': 4000.0, 'Roughness': 0.8}.items():
    mel.set_material_instance_scalar_parameter_value(mi, k, v)
mel.set_material_instance_texture_parameter_value(mi, 'Card', card)
mel.set_material_instance_vector_parameter_value(mi, 'TintA', unreal.LinearColor(1.0, 0.95, 0.8, 1))
mel.set_material_instance_vector_parameter_value(mi, 'TintB', unreal.LinearColor(1.12, 1.0, 0.72, 1))
eal.save_loaded_asset(mi)

for name in ('SM_Wheat_A', 'SM_Wheat_B'):
    options = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False), ('import_textures', False),
                 ('automated_import_should_detect_type', False), ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
        options.set_editor_property(k, v)
    data = options.static_mesh_import_data
    data.set_editor_property('combine_meshes', True)
    data.set_editor_property('vertex_color_import_option', unreal.VertexColorImportOption.REPLACE)
    data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    data.set_editor_property('generate_lightmap_u_vs', False)
    data.set_editor_property('auto_generate_collision', False)
    import_one(os.path.join(src, 'Meshes', name + '.fbx'), MESH, name, options)
    mesh = unreal.load_asset(MESH + '/' + name)
    for i in range(mesh.get_num_sections(0)):
        mesh.set_material(i, mi)
    eal.save_loaded_asset(mesh)
    unreal.log('WHEAT ' + name)
unreal.log('WHEAT DONE')
