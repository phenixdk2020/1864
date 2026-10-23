# Imports the Danish campaign-map scenery models (SourceAssets/Campaign1851/Scenery/DK, made by
# Tools/Map1851/build_scenery_models.py) as static meshes under /Game/Campaign1851/Scenery/DK, with their shared
# texture atlas and one unlit material that matches the painted map: the atlas lit by a fixed north-west light in the
# material (the map is unlit). Run with the editor closed:
#   UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file> -unattended -nop4
import os, unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source = os.path.join(root, 'SourceAssets', 'Campaign1851', 'Scenery', 'DK')
dest = '/Game/Campaign1851/Scenery/DK'
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TYPES = ['TownHouse_Ochre', 'TownHouse_White', 'TownHouse_Timber', 'MerchantHouse', 'Cottage', 'Farm', 'Church', 'Windmill',
         'Farm_Trelaenget', 'Farm_Tolaenget', 'Farm_Parallel', 'Farm_VestjyskLanghus', 'Farm_Haubarg', 'Farm_Niedersachsen']

# The atlas.
task = unreal.AssetImportTask()
for k, v in [('filename', os.path.join(source, 'T_SceneryDK_Atlas.png')), ('destination_path', dest), ('destination_name', 'T_SceneryDK_Atlas'),
             ('automated', True), ('replace_existing', True), ('save', True)]:
    task.set_editor_property(k, v)
tools.import_asset_tasks([task])
atlas = unreal.load_asset(dest + '/T_SceneryDK_Atlas')
assert isinstance(atlas, unreal.Texture2D), 'atlas missing'
atlas.set_editor_property('srgb', True)
lib.save_loaded_asset(atlas)

# The material: unlit like the map; emissive = atlas x (ambient + light from the north-west, as the painted map is lit).
material_path = dest + '/M_SceneryDK'
material = unreal.load_asset(material_path) if lib.does_asset_exist(material_path) else tools.create_asset('M_SceneryDK', dest, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(material)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
sample = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSample, -700, 0)
sample.set_editor_property('texture', atlas)
normal = MEL.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, -900, 300)
light = MEL.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -900, 450)
light.set_editor_property('constant', unreal.LinearColor(-0.45, -0.45, 0.77, 0.0))   # from the north-west and above (map: -Y is north)
dot = MEL.create_material_expression(material, unreal.MaterialExpressionDotProduct, -650, 350)
MEL.connect_material_expressions(normal, '', dot, 'A')
MEL.connect_material_expressions(light, '', dot, 'B')
sat = MEL.create_material_expression(material, unreal.MaterialExpressionSaturate, -500, 350)
MEL.connect_material_expressions(dot, '', sat, '')
shade = MEL.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -350, 300)
shade.set_editor_property('const_a', 0.62)
shade.set_editor_property('const_b', 1.05)
MEL.connect_material_expressions(sat, '', shade, 'Alpha')
lit = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 100)
MEL.connect_material_expressions(sample, 'RGB', lit, 'A')
MEL.connect_material_expressions(shade, '', lit, 'B')
MEL.connect_material_property(lit, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
MEL.recompile_material(material)
lib.save_loaded_asset(material)

for name in TYPES:
    asset = 'SM_' + name
    target = dest + '/' + asset
    options = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
        options.set_editor_property(k, v)
    options.static_mesh_import_data.set_editor_property('combine_meshes', True)
    options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs', False)
    task = unreal.AssetImportTask()
    for k, v in [('filename', os.path.join(source, asset + '.fbx')), ('destination_path', dest), ('destination_name', asset),
                 ('automated', True), ('replace_existing', True), ('save', True), ('options', options), ('factory', unreal.FbxFactory())]:
        task.set_editor_property(k, v)
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(target)
    assert isinstance(mesh, unreal.StaticMesh), asset + ': static mesh missing'
    slots = list(mesh.get_editor_property('static_materials'))
    for slot in slots:
        slot.set_editor_property('material_interface', material)
    mesh.set_editor_property('static_materials', slots)
    lib.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    unreal.log('SCENERY-IMPORT %s size %s' % (target, str(box.max - box.min)))
lib.save_directory(dest, only_if_is_dirty=False, recursive=True)
unreal.log('SCENERY-IMPORT DONE')
