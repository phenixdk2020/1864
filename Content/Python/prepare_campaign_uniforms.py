"""Tilføj kampagnens farveparametre til infanteriets teksturerede materialer.

Køres manuelt i Unreal Editors Python-konsol eller via bagescriptet.
Scriptet starter ikke editoren og køres ikke af kampagnen eller byggeprocessen.
Uændrede uniformer beholder deres oprindelige tekstur. Snit/hovedbeklædning er
en del af de importerede meshes; de har ingen udskiftelige hatmodeller.
"""
import unreal


def prepare(mesh_path):
    mesh = unreal.load_asset(mesh_path)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("Soldatermesh mangler: " + mesh_path)
    bounds = mesh.get_editor_property('imported_bounds')
    low = bounds.origin.z - bounds.box_extent.z
    height = max(1.0, 2.0 * bounds.box_extent.z)
    mel = unreal.MaterialEditingLibrary
    materials = []
    for slot in mesh.get_editor_property('materials'):
        material = slot.get_editor_property('material_interface')
        while isinstance(material, unreal.MaterialInstance):
            material = material.get_editor_property('parent')
        if isinstance(material, unreal.Material) and material not in materials:
            materials.append(material)
    for material in materials:
        if 'OverrideCoat' in [str(name) for name in mel.get_scalar_parameter_names(material)]:
            continue
        # UE 5.8 implementation reads GetExpressionInputForProperty directly (also in commandlets).
        diffuse = mel.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
        diffuse_output = mel.get_material_property_input_node_output_name(material, unreal.MaterialProperty.MP_BASE_COLOR)
        if diffuse is None:
            raise RuntimeError("Ingen basefarve i uniformmaterialet: " + material.get_path_name())
        local = mel.create_material_expression(material, unreal.MaterialExpressionPreSkinnedPosition, -600, 300)
        recolour = mel.create_material_expression(material, unreal.MaterialExpressionCustom, 100, 0)
        recolour.set_editor_property('description', 'Campaign unit uniform colours')
        recolour.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        # Height bands are approximate garment masks for the current single-atlas imports.
        # Preserve skin, boots and light belts instead of tinting the entire soldier.
        recolour.set_editor_property('code', f'''
float h = (Local.z - {low:.8f}) / {height:.8f};
float skin = step(Base.b * 1.3, Base.r) * step(Base.g * 1.12, Base.r)
           * step(Base.b * 1.05, Base.g) * step(0.12, Base.r);
float belt = step(0.68, min(Base.r, min(Base.g, Base.b)));
float cloth = (1.0 - skin) * (1.0 - belt);
float coatMask = step(0.48, h) * (1.0 - step(0.82, h)) * cloth;
float trouserMask = step(0.12, h) * (1.0 - step(0.48, h)) * (1.0 - skin);
float hatMask = step(0.88, h) * (1.0 - skin);
float detail = 0.65 + 0.35 * saturate(dot(Base, float3(0.2126,0.7152,0.0722)));
float3 result = lerp(Base, Coat * detail, coatMask * CoatOn);
result = lerp(result, Trousers * detail, trouserMask * TrousersOn);
return lerp(result, Hat * detail, hatMask * HatOn);
''')
        sources = [('Base', diffuse, diffuse_output), ('Local', local, '')]
        for key, parameter, colour in [
                ('Coat', 'CoatColor', unreal.LinearColor(0.1, 0.15, 0.25, 1)),
                ('Trousers', 'TrouserColor', unreal.LinearColor(0.2, 0.2, 0.2, 1)),
                ('Hat', 'HeadgearColor', unreal.LinearColor(0.02, 0.02, 0.02, 1))]:
            node = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -350, 200)
            node.set_editor_property('parameter_name', parameter)
            node.set_editor_property('default_value', colour)
            sources.append((key, node, 'RGB'))
        for key, parameter in [('CoatOn', 'OverrideCoat'), ('TrousersOn', 'OverrideTrousers'), ('HatOn', 'OverrideHeadgear')]:
            node = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -350, 500)
            node.set_editor_property('parameter_name', parameter)
            node.set_editor_property('default_value', 0.0)
            sources.append((key, node, ''))
        inputs = []
        for key, _, _ in sources:
            custom_input = unreal.CustomInput()
            custom_input.set_editor_property('input_name', key)
            inputs.append(custom_input)
        recolour.set_editor_property('inputs', inputs)
        for key, node, output in sources:
            mel.connect_material_expressions(node, output, recolour, key)
        mel.connect_material_property(recolour, '', unreal.MaterialProperty.MP_BASE_COLOR)
        mel.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
        errors = mel.recompile_material(material)
        if errors:
            raise RuntimeError("Uniformshader: " + '\n'.join(errors))
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        unreal.log('Kampagneuniform forberedt: ' + material.get_path_name())


if __name__ == '__main__':
    for path in [
        '/Game/Units/Danish/Infantry1864/Mesh/SK_DK_Infantry_1864',
        '/Game/Units/Danish/Jager1864/Mesh/SK_DK_Jager_1864',
        '/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864']:
        prepare(path)
