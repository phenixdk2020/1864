# The battlefield's look (PROJECT 1864): imports the generated textures (SourceAssets/Battle1864/Textures, from
# Tools/Battle/make_battle_textures.py) and meshes (SourceAssets/Battle1864/Meshes, from make_battle_meshes.py) into
# /Game/Battle, and builds the lit materials:
#   M_BattleGround  the ground: the picture's vertex colours with a tiled grass/dirt detail, macro variation, normals
#   M_Foliage       masked two-sided foliage cards with wind (vertex colour R), AO (G), per-instance tint, and a
#                   distance thinning for the grass; instances MI_Spruce, MI_Pine, MI_Leaves, MI_Grass
#   M_Bark, M_FenceWood
# Run: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<this file>
import os, unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
src = os.path.join(root, 'SourceAssets', 'Battle1864')
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
TEX, MESH, MAT = '/Game/Battle/Textures', '/Game/Battle/Foliage', '/Game/Battle/Materials'


def run(task_list):
    tools.import_asset_tasks(task_list)


# ---------------------------------------------------------------- textures
for f in sorted(os.listdir(os.path.join(src, 'Textures'))):
    name = os.path.splitext(f)[0]
    task = unreal.AssetImportTask()
    for k, v in [('filename', os.path.join(src, 'Textures', f)), ('destination_path', TEX), ('destination_name', name),
                 ('automated', True), ('replace_existing', True), ('save', False)]:
        task.set_editor_property(k, v)
    run([task])
    t = unreal.load_asset(TEX + '/' + name)
    if name.endswith('_N'):
        t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        t.set_editor_property('srgb', False)
        t.set_editor_property('flip_green_channel', True)
    elif name.endswith('_Macro'):
        t.set_editor_property('srgb', False)
    eal.save_loaded_asset(t)
    unreal.log('BATTLE-GFX texture ' + name)


def tex(name):
    return unreal.load_asset(TEX + '/' + name)


# ---------------------------------------------------------------- material helpers
class Graph:
    def __init__(self, name, path=MAT):
        m = unreal.load_asset(path + '/' + name)
        if not m:
            m = tools.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
        mel.delete_all_material_expressions(m)
        self.m = m
        self.x = -1600
        self.y = 0

    def node(self, cls, **props):
        e = mel.create_material_expression(self.m, cls, self.x, self.y)
        self.y += 140
        if self.y > 1800:
            self.y = 0
            self.x += 260
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(self, a, a_out, b, b_in):
        mel.connect_material_expressions(a, a_out, b, b_in)

    def const(self, v):
        return self.node(unreal.MaterialExpressionConstant, r=v)

    def const3(self, r, g, b):
        return self.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(r, g, b, 1))

    def scalar(self, name, v):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=v)

    def op(self, cls, a, b, a_out='', b_out=''):
        e = self.node(cls)
        self.link(a, a_out, e, 'A')
        self.link(b, b_out, e, 'B')
        return e

    def mul(self, a, b, a_out='', b_out=''):
        return self.op(unreal.MaterialExpressionMultiply, a, b, a_out, b_out)

    def add(self, a, b, a_out='', b_out=''):
        return self.op(unreal.MaterialExpressionAdd, a, b, a_out, b_out)

    def sub(self, a, b, a_out='', b_out=''):
        return self.op(unreal.MaterialExpressionSubtract, a, b, a_out, b_out)

    def div(self, a, b, a_out='', b_out=''):
        return self.op(unreal.MaterialExpressionDivide, a, b, a_out, b_out)

    def lerp(self, a, b, alpha, a_out='', b_out='', alpha_out=''):
        e = self.node(unreal.MaterialExpressionLinearInterpolate)
        self.link(a, a_out, e, 'A')
        self.link(b, b_out, e, 'B')
        self.link(alpha, alpha_out, e, 'Alpha')
        return e

    def sat(self, a, a_out=''):
        e = self.node(unreal.MaterialExpressionSaturate)
        self.link(a, a_out, e, '')
        return e

    def mask(self, a, r=False, g=False, b=False, al=False, a_out=''):
        e = self.node(unreal.MaterialExpressionComponentMask, r=r, g=g, b=b, a=al)
        self.link(a, a_out, e, '')
        return e

    def sample(self, texture, uv, uv_out='', sampler=None):
        e = self.node(unreal.MaterialExpressionTextureSample, texture=texture)
        if sampler:
            e.set_editor_property('sampler_type', sampler)
        if uv is not None:
            self.link(uv, uv_out, e, 'UVs')
        return e

    def srgb(self, vc_node):
        """Vertex colours are stored as sRGB (the campaign's mesh writer): back to linear."""
        e = self.node(unreal.MaterialExpressionPower, const_exponent=2.2)
        self.link(vc_node, '', e, 'Base')
        return e

    def tex_param(self, name, texture):
        return self.node(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name, texture=texture)

    def out(self, e, prop, e_out=''):
        mel.connect_material_property(e, e_out, prop)

    def done(self):
        # The instanced trees, bushes, fences, grass and map pieces need the flag, or the game draws the default material.
        for usage in (unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, unreal.MaterialUsage.MATUSAGE_STATIC_LIGHTING):
            mel.set_material_usage(self.m, usage)
        mel.layout_material_expressions(self.m)
        mel.recompile_material(self.m)
        eal.save_loaded_asset(self.m)
        unreal.log('BATTLE-GFX material ' + self.m.get_name())


# ---------------------------------------------------------------- the ground
g = Graph('M_BattleGround')
vc = g.srgb(g.node(unreal.MaterialExpressionVertexColor))
wp = g.node(unreal.MaterialExpressionWorldPosition)
wxy = g.mask(wp, r=True, g=True)
uv_fine = g.div(wxy, g.const(300.0))           # 3 m tiles
uv_mid = g.div(wxy, g.const(1130.0))           # 11.3 m tiles (breaks the repetition)
uv_macro = g.div(wxy, g.const(40000.0))        # 400 m
uv_dirt = g.div(wxy, g.const(400.0))
grass_a = g.sample(tex('T_Ground_Grass_D'), uv_fine)
grass_b = g.sample(tex('T_Ground_Grass_D'), uv_mid)
grass = g.lerp(grass_a, grass_b, g.const(0.4), 'RGB', 'RGB')
dirt = g.sample(tex('T_Ground_Dirt_D'), uv_dirt)
macro = g.sample(tex('T_Ground_Macro'), uv_macro, sampler=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
# The detail as a brightness modulator round its mean (keeps the picture's colours) plus some of its own colour.
lum = g.node(unreal.MaterialExpressionDesaturation)
g.link(grass, '', lum, '')
lum_mod = g.div(g.mask(lum, r=True), g.const(0.196))                # mean luminance of the grass texture (linear)
grass_tinted = g.mul(vc, lum_mod)
grass_col = g.lerp(grass_tinted, g.mul(grass, g.const(0.75)), g.scalar('GrassDetailColour', 0.18))
dirt_lum = g.node(unreal.MaterialExpressionDesaturation)
g.link(dirt, 'RGB', dirt_lum, '')
dirt_col = g.mul(vc, g.div(g.mask(dirt_lum, r=True), g.const(0.22)))
# Dirt where the picture is brown (red above green), grass where it is green.
# Dirt where the picture is a greyish brown (ways, trampled ground, the town): red over green, and blue enough
# against red (a ripe grain field is yellow, little blue, and keeps the grass detail).
vr, vg, vb = g.mask(vc, r=True), g.mask(vc, g=True), g.mask(vc, b=True)
browner = g.sat(g.mul(g.sub(vr, vg), g.const(25.0)))
greyer = g.sat(g.mul(g.sub(g.div(vb, g.add(vr, g.const(0.01))), g.const(0.28)), g.const(8.0)))
dirt_mask = g.mul(browner, greyer)
# Grain fields where the picture is yellow (red over green, little blue): the stubble and straw texture.
field = g.sample(tex('T_Ground_Field_D'), uv_dirt)
field_lum = g.node(unreal.MaterialExpressionDesaturation)
g.link(field, 'RGB', field_lum, '')
field_col = g.mul(vc, g.div(g.mask(field_lum, r=True), g.const(0.308)))
field_mask = g.mul(browner, g.sub(g.const(1.0), greyer))
base = g.lerp(g.lerp(grass_col, field_col, field_mask), dirt_col, dirt_mask)
# Macro: lush (darker, greener) and dry (yellower) patches, and a brightness swing.
dry_tint = g.const3(0.30, 0.26, 0.10)
base = g.lerp(base, g.mul(base, g.const3(1.25, 1.1, 0.75)), g.mul(g.mask(macro, g=True), g.scalar('DryPatches', 0.35)))
base = g.mul(base, g.lerp(g.const(0.88), g.const(1.08), g.mask(macro, r=True)))
base = g.mul(base, g.scalar('GroundBrightness', 1.0))
g.out(base, unreal.MaterialProperty.MP_BASE_COLOR)
n_grass = g.sample(tex('T_Ground_Grass_N'), uv_fine, sampler=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
n_dirt = g.sample(tex('T_Ground_Dirt_N'), uv_dirt, sampler=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
normal = g.lerp(n_grass, n_dirt, dirt_mask, 'RGB', 'RGB')
g.out(normal, unreal.MaterialProperty.MP_NORMAL)
g.out(g.lerp(g.const(0.92), g.const(0.85), dirt_mask), unreal.MaterialProperty.MP_ROUGHNESS)
g.out(g.const(0.0), unreal.MaterialProperty.MP_SPECULAR)
g.done()

# ---------------------------------------------------------------- foliage cards
f = Graph('M_Foliage')
f.m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
f.m.set_editor_property('two_sided', True)
f.m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
card = f.tex_param('Card', tex('T_Card_Leaves'))
vcf = f.node(unreal.MaterialExpressionVertexColor)
rnd = f.node(unreal.MaterialExpressionPerInstanceRandom)
tint_a = f.node(unreal.MaterialExpressionVectorParameter, parameter_name='TintA', default_value=unreal.LinearColor(0.92, 1.0, 0.85, 1))
tint_b = f.node(unreal.MaterialExpressionVectorParameter, parameter_name='TintB', default_value=unreal.LinearColor(1.12, 1.05, 0.8, 1))
tint = f.lerp(tint_a, tint_b, rnd, 'RGB', 'RGB')
ao = f.lerp(f.scalar('AOMin', 0.45), f.const(1.0), vcf, '', '', 'G')
col = f.mul(f.mul(card, tint, 'RGB'), ao)
col = f.mul(col, f.scalar('Brightness', 1.0))
f.out(col, unreal.MaterialProperty.MP_BASE_COLOR)
f.out(f.mul(f.mul(col, f.const3(0.8, 1.0, 0.5)), f.scalar('Translucency', 0.45)), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
f.out(f.scalar('Roughness', 0.75), unreal.MaterialProperty.MP_ROUGHNESS)
f.out(f.const(0.25), unreal.MaterialProperty.MP_SPECULAR)
# Opacity: the card's alpha, thinned with the camera distance (the grass fades out between FadeStart and FadeEnd).
cam = f.node(unreal.MaterialExpressionCameraPositionWS)
wpf = f.node(unreal.MaterialExpressionWorldPosition)
dist = f.node(unreal.MaterialExpressionDistance)
f.link(cam, '', dist, 'A')
f.link(wpf, '', dist, 'B')
fade = f.sat(f.div(f.sub(dist, f.scalar('FadeStart', 1.0e9)), f.scalar('FadeLength', 4000.0)))
f.out(f.sub(f.mask(card, al=True, a_out='A'), fade), unreal.MaterialProperty.MP_OPACITY_MASK)
# Wind.
wind = f.node(unreal.MaterialExpressionMaterialFunctionCall)
wind.set_editor_property('material_function', unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions01/WorldPositionOffset/SimpleGrassWind'))
f.link(f.scalar('WindIntensity', 0.25), '', wind, 'WindIntensity')
f.link(f.mask(vcf, r=True), '', wind, 'WindWeight')
f.link(f.scalar('WindSpeed', 0.6), '', wind, 'WindSpeed')
f.link(f.const(0.0), '', wind, 'AdditionalWPO')
f.out(wind, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
f.done()


def instance(name, parent, scalars, textures, vectors=None):
    mi = unreal.load_asset(MAT + '/' + name)
    if not mi:
        mi = tools.create_asset(name, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in scalars.items():
        mel.set_material_instance_scalar_parameter_value(mi, k, v)
    for k, v in textures.items():
        mel.set_material_instance_texture_parameter_value(mi, k, v)
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, v)
    eal.save_loaded_asset(mi)
    unreal.log('BATTLE-GFX instance ' + name)
    return mi


mi_spruce = instance('MI_Spruce', f.m, {'WindIntensity': 0.12, 'AOMin': 0.35, 'Brightness': 0.85}, {'Card': tex('T_Card_Spruce')},
                     {'TintA': unreal.LinearColor(0.85, 0.95, 0.9, 1), 'TintB': unreal.LinearColor(1.0, 1.05, 0.85, 1)})
mi_pine = instance('MI_Pine', f.m, {'WindIntensity': 0.12, 'AOMin': 0.4}, {'Card': tex('T_Card_Pine')})
mi_leaves = instance('MI_Leaves', f.m, {'WindIntensity': 0.2, 'AOMin': 0.4}, {'Card': tex('T_Card_Leaves')})
mi_grass = instance('MI_Grass', f.m, {'WindIntensity': 0.45, 'WindSpeed': 0.9, 'AOMin': 0.9, 'Brightness': 0.92, 'Translucency': 0.25, 'FadeStart': 6000.0, 'FadeLength': 4500.0, 'Roughness': 0.85},
                    {'Card': tex('T_Card_Grass')}, {'TintA': unreal.LinearColor(0.8, 0.95, 0.7, 1), 'TintB': unreal.LinearColor(1.0, 1.05, 0.7, 1)})

# ---------------------------------------------------------------- bark and wood
for name, d, n, uvscale, wind_on in [('M_Bark', 'T_Bark_D', 'T_Bark_N', 1.0, True), ('M_FenceWood', 'T_FenceWood_D', 'T_FenceWood_N', 1.0, False)]:
    b = Graph(name)
    tc = b.node(unreal.MaterialExpressionTextureCoordinate, u_tiling=uvscale, v_tiling=uvscale)
    vcb = b.node(unreal.MaterialExpressionVertexColor)
    dif = b.sample(tex(d), tc)
    b.out(b.mul(dif, b.lerp(b.const(0.4), b.const(1.0), vcb, '', '', 'G'), 'RGB'), unreal.MaterialProperty.MP_BASE_COLOR)
    b.out(b.sample(tex(n), tc, sampler=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), unreal.MaterialProperty.MP_NORMAL)
    b.out(b.const(0.9), unreal.MaterialProperty.MP_ROUGHNESS)
    if wind_on:
        w = b.node(unreal.MaterialExpressionMaterialFunctionCall)
        w.set_editor_property('material_function', unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions01/WorldPositionOffset/SimpleGrassWind'))
        b.link(b.scalar('WindIntensity', 0.12), '', w, 'WindIntensity')
        b.link(b.mask(vcb, r=True), '', w, 'WindWeight')
        b.link(b.scalar('WindSpeed', 0.6), '', w, 'WindSpeed')
        b.link(b.const(0.0), '', w, 'AdditionalWPO')
        b.out(w, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    b.done()

# ---------------------------------------------------------------- lit scenery (buildings, banks) and water
sc = Graph('M_BattleScenery')
vcs = sc.srgb(sc.node(unreal.MaterialExpressionVertexColor))
wps = sc.node(unreal.MaterialExpressionWorldPosition)
noise_uv = sc.div(sc.mask(wps, r=True, g=True), sc.const(500.0))
grain = sc.sample(tex('T_Ground_Dirt_D'), noise_uv)
grain_l = sc.node(unreal.MaterialExpressionDesaturation)
sc.link(grain, 'RGB', grain_l, '')
sc.out(sc.mul(sc.mul(vcs, sc.div(sc.mask(grain_l, r=True), sc.const(0.2))), sc.const(0.85)), unreal.MaterialProperty.MP_BASE_COLOR)
sc.out(sc.const(0.85), unreal.MaterialProperty.MP_ROUGHNESS)
sc.out(sc.const(0.3), unreal.MaterialProperty.MP_SPECULAR)
sc.done()

wa = Graph('M_BattleWater')
vcw = wa.srgb(wa.node(unreal.MaterialExpressionVertexColor))
wpw = wa.node(unreal.MaterialExpressionWorldPosition)
uvw = wa.div(wa.mask(wpw, r=True, g=True), wa.const(900.0))
pan = wa.node(unreal.MaterialExpressionPanner, speed_x=0.02, speed_y=0.01)
wa.link(uvw, '', pan, 'Coordinate')
ripple = wa.sample(tex('T_Ground_Dirt_N'), pan, sampler=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
flat = wa.const3(0.0, 0.0, 1.0)
wa.out(wa.lerp(flat, ripple, wa.const(0.25), '', 'RGB'), unreal.MaterialProperty.MP_NORMAL)
wa.out(wa.mul(vcw, wa.const(0.55)), unreal.MaterialProperty.MP_BASE_COLOR)
wa.out(wa.const(0.06), unreal.MaterialProperty.MP_ROUGHNESS)
wa.out(wa.const(0.7), unreal.MaterialProperty.MP_SPECULAR)
wa.done()

slot_materials = {'Bark': unreal.load_asset(MAT + '/M_Bark'), 'Spruce': mi_spruce, 'Pine': mi_pine, 'Leaves': mi_leaves,
                  'Grass': mi_grass, 'Wood': unreal.load_asset(MAT + '/M_FenceWood')}

# ---------------------------------------------------------------- meshes
for f_ in sorted(os.listdir(os.path.join(src, 'Meshes'))):
    if not f_.endswith('.fbx'):
        continue
    name = os.path.splitext(f_)[0]
    options = unreal.FbxImportUI()
    for k, v in [('import_as_skeletal', False), ('import_mesh', True), ('import_animations', False), ('import_materials', False),
                 ('import_textures', False), ('automated_import_should_detect_type', False),
                 ('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)]:
        options.set_editor_property(k, v)
    data = options.static_mesh_import_data
    data.set_editor_property('combine_meshes', True)
    data.set_editor_property('vertex_color_import_option', unreal.VertexColorImportOption.REPLACE)
    data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    data.set_editor_property('generate_lightmap_u_vs', False)
    data.set_editor_property('auto_generate_collision', False)
    task = unreal.AssetImportTask()
    for k, v in [('filename', os.path.join(src, 'Meshes', f_)), ('destination_path', MESH), ('destination_name', name),
                 ('automated', True), ('replace_existing', True), ('save', False), ('options', options), ('factory', unreal.FbxFactory())]:
        task.set_editor_property(k, v)
    run([task])
    mesh = unreal.load_asset(MESH + '/' + name)
    slots = list(mesh.get_editor_property('static_materials'))
    for s in slots:
        sn = str(s.get_editor_property('material_slot_name'))
        key = next((k for k in slot_materials if sn.lower().startswith(k.lower())), None)
        if key:
            s.set_editor_property('material_interface', slot_materials[key])
    mesh.set_editor_property('static_materials', slots)
    eal.save_loaded_asset(mesh)
    box = mesh.get_bounding_box()
    unreal.log('BATTLE-GFX mesh %s slots %s height %.0f' % (name, [str(s.get_editor_property('material_slot_name')) for s in slots], box.max.z - box.min.z))
unreal.log('BATTLE-GFX DONE')
