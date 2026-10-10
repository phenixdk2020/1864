# Alle fodsoldaters VAT-materiale (PROJECT 1864, the hybrid of baked and full animation; see
# Source/Strategy1864/Visual/StrategyCrowdModel.h):
#  - modellen bager LOD0 med RGB vertexfarvemasker; eksisterende soldaterassets bevarer deres LODs;
#  - M_CrowdVAT_All skins in the vertex shader from the bone texture (three texels a bone, a row a frame): bone indices
#    in UV1-2, weights in UV3-4, the clip in the instance's custom data (first row, frames, start time, frame rate;
#    a negative rate plays once and holds), two frames blended.
# Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file>
import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MAT = '/Game/Battle/Materials'


def configure_crowd_fade(material, vat=False):
    """Complementary pixel coverage avoids a hard handoff and double drawing.

    Original infantry materials are opaque; leave other blend modes for explicit authoring.
    """
    if not vat and 'CrowdOpacity' in [str(n) for n in mel.get_scalar_parameter_names(material)]:
        return
    if material.get_editor_property('blend_mode') not in [unreal.BlendMode.BLEND_OPAQUE, unreal.BlendMode.BLEND_MASKED]:
        raise RuntimeError('PROJECT1864-CROWD: unsupported near material blend: ' + material.get_path_name())
    previous_mask = mel.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY_MASK)
    previous_output = mel.get_material_property_input_node_output_name(material, unreal.MaterialProperty.MP_OPACITY_MASK)
    def fade_node(cls, **properties):
        result = mel.create_material_expression(material, cls, -500, 1800)
        for key, value in properties.items():
            result.set_editor_property(key, value)
        return result
    if vat:
        alpha = fade_node(unreal.MaterialExpressionPerInstanceCustomData, data_index=16)
        vertex_alpha = fade_node(unreal.MaterialExpressionVertexInterpolator)
        mel.connect_material_expressions(alpha, '', vertex_alpha, '')
        alpha = vertex_alpha
    else:
        alpha = fade_node(unreal.MaterialExpressionScalarParameter, parameter_name='CrowdOpacity', default_value=1.0)
    coverage = fade_node(unreal.MaterialExpressionCustom)
    coverage.set_editor_property('description', 'Complementary VAT/skeletal coverage')
    coverage.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    threshold = '1.0 - step(noise, 1.0 - Alpha)' if vat else 'step(noise, Alpha)'
    coverage.set_editor_property('code', '''
if (Alpha <= 0.0) return 0.0;
if (Alpha >= 1.0) return 1.0;
float noise = frac(sin(dot(floor(Parameters.SvPosition.xy), float2(12.9898, 78.233))) * 43758.5453);
return ''' + threshold + ';')
    coverage_input = unreal.CustomInput()
    coverage_input.set_editor_property('input_name', 'Alpha')
    coverage.set_editor_property('inputs', [coverage_input])
    mel.connect_material_expressions(alpha, '', coverage, 'Alpha')
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    if previous_mask is not None:
        combined_mask = fade_node(unreal.MaterialExpressionMultiply)
        mel.connect_material_expressions(previous_mask, previous_output, combined_mask, 'A')
        mel.connect_material_expressions(coverage, '', combined_mask, 'B')
        coverage = combined_mask
    mel.connect_material_property(coverage, '', unreal.MaterialProperty.MP_OPACITY_MASK)
    errors = mel.recompile_material(material)
    if errors:
        raise RuntimeError("PROJECT1864-CROWD: shaderfejl: " + "\n".join(errors))
    unreal.EditorAssetLibrary.save_loaded_asset(material)

m = unreal.load_asset(MAT + '/M_CrowdVAT_All')
if not m:
    m = tools.create_asset('M_CrowdVAT_All', MAT, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
y = [0]


def node(cls, x=-1200, **props):
    e = mel.create_material_expression(m, cls, x, y[0])
    y[0] += 130
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


pos = node(unreal.MaterialExpressionPreSkinnedPosition)
uv = [node(unreal.MaterialExpressionTextureCoordinate, coordinate_index=i) for i in range(1, 5)]
data = [node(unreal.MaterialExpressionPerInstanceCustomData, data_index=i) for i in range(4)]
time = node(unreal.MaterialExpressionTime)
bones = node(unreal.MaterialExpressionTextureObjectParameter, parameter_name='BoneTex', texture=unreal.load_asset('/Game/Battle/Textures/T_Ground_Macro'))

code = '''
float frames = max(D1, 1.0);
float t = max((T - D2) * abs(D3), 0.0);
float f = D3 >= 0.0 ? fmod(t, frames) : min(t, frames - 1.0);
float f0 = floor(f);
float a = f - f0;
float f1 = D3 >= 0.0 ? fmod(f0 + 1.0, frames) : min(f0 + 1.0, frames - 1.0);
int r0 = (int)(D0 + f0);
int r1 = (int)(D0 + f1);
float4 P = float4(Pos, 1.0);
float4 idx = float4(I01, I23);
float4 w = float4(W01, W23);
float3 s0 = 0;
float3 s1 = 0;
[unroll] for (int k = 0; k < 4; k++)
{
    int b = (int)(idx[k] + 0.5) * 3;
    s0 += w[k] * float3(dot(P, BoneTex.Load(int3(b, r0, 0))), dot(P, BoneTex.Load(int3(b + 1, r0, 0))), dot(P, BoneTex.Load(int3(b + 2, r0, 0))));
    s1 += w[k] * float3(dot(P, BoneTex.Load(int3(b, r1, 0))), dot(P, BoneTex.Load(int3(b + 1, r1, 0))), dot(P, BoneTex.Load(int3(b + 2, r1, 0))));
}
return lerp(s0, s1, a) - Pos;
'''
custom = node(unreal.MaterialExpressionCustom, x=-700)
custom.set_editor_property('code', code)
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property('description', 'CrowdSkin')
names = ['Pos', 'I01', 'I23', 'W01', 'W23', 'D0', 'D1', 'D2', 'D3', 'T', 'BoneTex']
inputs = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property('input_name', n)
    inputs.append(ci)
custom.set_editor_property('inputs', inputs)
for n, src in zip(names, [pos] + uv + data + [time, bones]):
    mel.connect_material_expressions(src, '', custom, n)
to_world = node(unreal.MaterialExpressionTransform, x=-400,
                transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
mel.connect_material_expressions(custom, '', to_world, '')
mel.connect_material_property(to_world, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
normal = node(unreal.MaterialExpressionPreSkinnedNormal)
skin_normal = node(unreal.MaterialExpressionCustom)
skin_normal.set_editor_property('code', code.replace('float4(Pos, 1.0)', 'float4(Pos, 0.0)').replace('return lerp(s0, s1, a) - Pos;', 'return normalize(lerp(s0, s1, a));'))
skin_normal.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
skin_normal.set_editor_property('description', 'CrowdSkinNormal')
skin_normal.set_editor_property('inputs', inputs)
for n, src in zip(names, [normal] + uv + data + [time, bones]):
    mel.connect_material_expressions(src, '', skin_normal, n)
normal_world = node(unreal.MaterialExpressionTransform,
                    transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                    transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
mel.connect_material_expressions(skin_normal, '', normal_world, '')
normal_interpolator = node(unreal.MaterialExpressionVertexInterpolator)
mel.connect_material_expressions(normal_world, '', normal_interpolator, '')
m.set_editor_property('tangent_space_normal', False)
mel.connect_material_property(normal_interpolator, '', unreal.MaterialProperty.MP_NORMAL)


diffuse = node(unreal.MaterialExpressionTextureSampleParameter2D, x=-400, parameter_name='Diffuse',
               texture=unreal.load_asset('/Game/Battle/Textures/T_Ground_Dirt_D'))
# Per-instance colours are evaluated in the vertex stage, then interpolated for the pixel shader.
mask = node(unreal.MaterialExpressionVertexColor)
colour_inputs = [('Base', diffuse, 'RGB'), ('Mask', mask, '')]
for part, offset, switch in [('CoatColor', 4, 13), ('TrouserColor', 7, 14), ('HeadgearColor', 10, 15)]:
    channels = [node(unreal.MaterialExpressionPerInstanceCustomData, data_index=i)
                for i in [offset, offset + 1, offset + 2, switch]]
    packed = channels[0]
    for channel in channels[1:]:
        append = node(unreal.MaterialExpressionAppendVector)
        mel.connect_material_expressions(packed, '', append, 'A')
        mel.connect_material_expressions(channel, '', append, 'B')
        packed = append
    interpolator = node(unreal.MaterialExpressionVertexInterpolator)
    mel.connect_material_expressions(packed, '', interpolator, '')
    colour_inputs.append((part, interpolator, ''))
recolour = node(unreal.MaterialExpressionCustom)
recolour.set_editor_property('description', 'Per-instance uniform colours; RGB garment vertex masks')
recolour.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
recolour.set_editor_property('code', """
float skin = step(Base.b * 1.3, Base.r) * step(Base.g * 1.12, Base.r)
           * step(Base.b * 1.05, Base.g) * step(0.12, Base.r);
float belt = step(0.68, min(Base.r, min(Base.g, Base.b)));
float detail = 0.65 + 0.35 * saturate(dot(Base, float3(.2126,.7152,.0722)));
float3 result = lerp(Base, CoatColor.rgb * detail, Mask.r * (1-skin) * (1-belt) * CoatColor.a);
result = lerp(result, TrouserColor.rgb * detail, Mask.g * (1-skin) * TrouserColor.a);
return lerp(result, HeadgearColor.rgb * detail, Mask.b * (1-skin) * HeadgearColor.a);
""")
recolour_inputs = []
for key, _, _ in colour_inputs:
    ci = unreal.CustomInput()
    ci.set_editor_property('input_name', key)
    recolour_inputs.append(ci)
recolour.set_editor_property('inputs', recolour_inputs)
for key, source, output in colour_inputs:
    mel.connect_material_expressions(source, output, recolour, key)
mel.connect_material_property(recolour, '', unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionConstant, x=-400, r=0.8)
mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
spec = node(unreal.MaterialExpressionConstant, x=-400, r=0.3)
mel.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)
mel.set_material_usage(m, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
configure_crowd_fade(m, vat=True)
errors = mel.recompile_material(m)
if errors:
    raise RuntimeError("PROJECT1864-CROWD: shaderfejl: " + "\n".join(errors))
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('PROJECT1864-CROWD: uniformmateriale gemt')
