# The far soldiers' material and LODs (PROJECT 1864, the hybrid of baked and full animation; see
# Source/Strategy1864/Visual/StrategyCrowdModel.h):
#  - the soldier meshes get four LODs (LOD3, about 2,500 vertices, is what the crowd model bakes);
#  - M_CrowdVAT skins in the vertex shader from the bone texture (three texels a bone, a row a frame): bone indices
#    in UV1-2, weights in UV3-4, the clip in the instance's custom data (first row, frames, start time, frame rate;
#    a negative rate plays once and holds), two frames blended.
# Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file>
import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MAT = '/Game/Battle/Materials'

sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
for p in ['/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864', '/Game/Units/Danish/Infantry1864/Mesh/SK_DK_Infantry_1864',
          '/Game/Units/Danish/Jager1864/Mesh/SK_DK_Jager_1864', '/Game/Units/Swedish/Infantry1864/Mesh/SK_SE_Infantry_1864']:
    m = unreal.load_asset(p)
    if m and sub.get_lod_count(m) < 4:
        sub.regenerate_lod(m, 4, False, False)
        unreal.EditorAssetLibrary.save_loaded_asset(m)
    if m:
        unreal.log('CROWD-MAT lods %s %s' % (p.split('/')[-1], [sub.get_num_verts(m, i) for i in range(sub.get_lod_count(m))]))

m = unreal.load_asset(MAT + '/M_CrowdVAT')
if not m:
    m = tools.create_asset('M_CrowdVAT', MAT, unreal.Material, unreal.MaterialFactoryNew())
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

diffuse = node(unreal.MaterialExpressionTextureSampleParameter2D, x=-400, parameter_name='Diffuse',
               texture=unreal.load_asset('/Game/Battle/Textures/T_Ground_Dirt_D'))
mel.connect_material_property(diffuse, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionConstant, x=-400, r=0.8)
mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
spec = node(unreal.MaterialExpressionConstant, x=-400, r=0.3)
mel.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)
mel.set_material_usage(m, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
mel.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('CROWD-MAT DONE')
