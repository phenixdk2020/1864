"""Import af runtime-fliser; k?res kun eksplicit med editoren lukket.
Ingen k?rsel ved normal Python-validering. Se Docs/Kortfliser.md.
"""
import ast
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
DEST = "/Game/Campaign1851/MapTiles"
MAT_DEST = "/Game/Campaign1851"
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
manifest = json.loads((ROOT / "Data/Campaign1851/MapTiles.json").read_text(encoding="utf-8"))
meta = json.loads((ROOT / "Data/Campaign1851/Denmark1851_Map.json").read_text(encoding="utf-8"))
SIZE_KM = (meta["extentKm"]["xMax"] - meta["extentKm"]["xMin"], meta["extentKm"]["yMax"] - meta["extentKm"]["yMin"])
PARCEL_TILE_KM = meta.get("parcelTileKm", 8.0)

# Ordinary streamed textures with mips (user decision 2026-10-21): no Virtual Texture Support needed.
# The C++ side loads the nearest tiles asynchronously; the engine streams their mips by distance.


def import_texture(filename, name, srgb):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    factory = unreal.TextureFactory()
    factory.set_editor_property("create_material", False)
    task.set_editor_property("factory", factory)
    tools.import_asset_tasks([task])
    tex = lib.load_asset(DEST + "/" + name)
    if not isinstance(tex, unreal.Texture2D):
        raise RuntimeError("Import fejlede: " + str(filename))
    tex.set_editor_property("srgb", srgb)
    tex.set_editor_property("virtual_texture_streaming", False)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    tex.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.PAD_TO_POWER_OF_TWO)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT if srgb else unreal.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    lib.save_loaded_asset(tex)
    return tex


mpc = lib.load_asset(MAT_DEST + "/MPC_Campaign1851Season")
parcel_tex = lib.load_asset(MAT_DEST + "/Map/Fields1851_Parcels")
amt_tex = lib.load_asset(MAT_DEST + "/Map/Denmark1851_Amter")
if not mpc or not parcel_tex or not amt_tex:
    raise RuntimeError("Den oprindelige kampagnes materialer/teksturer skal findes f?r fliseimport.")


# Only the GPU layers are imported. Height/Features/Amter remain CPU runtime data,
# avoiding a second, unused copy of five hundred megabytes of source artwork.
first = manifest["tiles"][0]
first_textures = {}
for tile in manifest["tiles"]:
    for layer in ("Color", "DetailMask"):
        texture = import_texture(ROOT / tile["layers"][layer]["file"], f"T_Tile_{tile['x']}_{tile['y']}_{layer}", layer == "Color")
        if tile is first:
            first_textures[layer] = texture
# NPOT overview: padding changes the sampled content area; compensate below using
# the source/padded dimensions, rather than stretching or cropping the projection.
overview_path = ROOT / manifest["overview"]["color"]
overview_tex = import_texture(overview_path, "T_MapTiles_Overview_Color", True)

# Reuse the existing Danish season/cloud/sea shader code verbatim. Execute only
# named pure definitions, NEVER the setup script's asset imports/level creation.
shader_path = ROOT / "Tools/Campaign/setup_campaign1851.py"
shader_tree = ast.parse(shader_path.read_text(encoding="utf-8-sig"), filename=str(shader_path))
shader_symbols = {"SEASON_PARAMS", "MAP_SEASON", "CLOUD_HLSL", "MAP_ATMOS", "season", "atmosphere", "expr"}
shader_nodes = []
for node in shader_tree.body:
    if isinstance(node, ast.FunctionDef) and node.name in shader_symbols:
        shader_nodes.append(node)
    elif isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id in shader_symbols for t in node.targets):
        shader_nodes.append(node)
exec(compile(ast.Module(body=shader_nodes, type_ignores=[]), str(shader_path), "exec"), globals())


def new_material(name):
    path = MAT_DEST + "/" + name
    material = lib.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(name, MAT_DEST, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(material)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    return material


def overview_sample(uv_index):
    node = expr(unreal.MaterialExpressionTextureSample, -1800, -500, texture=overview_tex,
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    # Source dimensions are known from the overview manifest and can be read via
    # Texture2D size functions only before padding? Use manifest's explicit size.
    overview_size = manifest["overview"]["size"]
    sx, sy = overview_size
    px, py = 1 << (sx-1).bit_length(), 1 << (sy-1).bit_length()
    uv_node = expr(unreal.MaterialExpressionTextureCoordinate, -2100, -500,
                   coordinate_index=uv_index, u_tiling=sx/px, v_tiling=sy/py)
    MEL.connect_material_expressions(uv_node, "", node, "UVs")
    return node


m = new_material("M_Campaign1851Tile")
col_tile = expr(unreal.MaterialExpressionTextureSampleParameter2D, -1800, -300,
                parameter_name="TileColor", texture=first_textures["Color"],
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
ready = expr(unreal.MaterialExpressionScalarParameter, -1600, -100, parameter_name="TileReady", default_value=0.0)
col = expr(unreal.MaterialExpressionLinearInterpolate, -1300, -300)
MEL.connect_material_expressions(overview_sample(2), "RGB", col, "A")
MEL.connect_material_expressions(col_tile, "RGB", col, "B")
MEL.connect_material_expressions(ready, "", col, "Alpha")
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1600, 100, coordinate_index=1, u_tiling=SIZE_KM[0] / PARCEL_TILE_KM, v_tiling=SIZE_KM[1] / PARCEL_TILE_KM)
par = expr(unreal.MaterialExpressionTextureSample, -1300, 100, texture=parcel_tex)
MEL.connect_material_expressions(uv, "", par, "UVs")
mask = expr(unreal.MaterialExpressionTextureSampleParameter2D, -1300, 450, parameter_name="TileDetailMask", texture=first_textures["DetailMask"], sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)

lum_w = expr(unreal.MaterialExpressionConstant3Vector, -1300, -50, constant=unreal.LinearColor(0.2126, 0.7152, 0.0722, 1.0))
luma = expr(unreal.MaterialExpressionDotProduct, -1000, -100)
MEL.connect_material_expressions(col, "", luma, "A")
MEL.connect_material_expressions(lum_w, "", luma, "B")
ratio = expr(unreal.MaterialExpressionDivide, -850, -100, const_b=0.14)
MEL.connect_material_expressions(luma, "", ratio, "A")
clamp = expr(unreal.MaterialExpressionClamp, -700, -100, min_default=0.55, max_default=1.45)
MEL.connect_material_expressions(ratio, "", clamp, "")
lit = expr(unreal.MaterialExpressionMultiply, -550, 50)
MEL.connect_material_expressions(par, "RGB", lit, "A")
MEL.connect_material_expressions(clamp, "", lit, "B")

depth = expr(unreal.MaterialExpressionPixelDepth, -1300, 700)
per = expr(unreal.MaterialExpressionDivide, -1100, 700, const_b=7000.0)
MEL.connect_material_expressions(depth, "", per, "A")
fade = expr(unreal.MaterialExpressionSubtract, -950, 700, const_a=2.0)
MEL.connect_material_expressions(per, "", fade, "B")
sat = expr(unreal.MaterialExpressionSaturate, -800, 700)
MEL.connect_material_expressions(fade, "", sat, "")
alpha = expr(unreal.MaterialExpressionMultiply, -650, 500)
MEL.connect_material_expressions(mask, "A", alpha, "A")
mask_ready = expr(unreal.MaterialExpressionMultiply, -800, 550)
MEL.connect_material_expressions(sat, "", mask_ready, "A")
MEL.connect_material_expressions(ready, "", mask_ready, "B")
MEL.connect_material_expressions(mask_ready, "", alpha, "B")
strength = expr(unreal.MaterialExpressionMultiply, -500, 500, const_b=0.8)
MEL.connect_material_expressions(alpha, "", strength, "A")

final = expr(unreal.MaterialExpressionLinearInterpolate, -300, 0)
MEL.connect_material_expressions(col, "", final, "A")
MEL.connect_material_expressions(lit, "", final, "B")
MEL.connect_material_expressions(strength, "", final, "Alpha")
seasoned = season(m, final, "", MAP_SEASON, -100, 300)
# The selected amt lights up: its land a little brighter and warmer.
amt_ids = expr(unreal.MaterialExpressionTextureSample, -700, 900, texture=amt_tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
legacy_uv = expr(unreal.MaterialExpressionTextureCoordinate, -1000, 950, coordinate_index=1)
MEL.connect_material_expressions(legacy_uv, "", amt_ids, "UVs")
selected = expr(unreal.MaterialExpressionCollectionParameter, -700, 1150, collection=mpc, parameter_name="SelectedAmt")
glow = expr(unreal.MaterialExpressionCustom, 100, 600, code="""
float id = round(Id * 255.0);
float valid = step(0.0, UV.x) * step(UV.x, 1.0) * step(0.0, UV.y) * step(UV.y, 1.0);
float hit = step(abs(id - Sel), 0.4) * step(0.5, Sel) * valid;
return lerp(C, C * 1.3 + float3(0.04, 0.032, 0.01), hit);
""", output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, description="SelectedAmt")
glow_inputs = []
for name in ("C", "Id", "Sel", "UV"):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    glow_inputs.append(ci)
glow.set_editor_property("inputs", glow_inputs)
MEL.connect_material_expressions(seasoned, "", glow, "C")
MEL.connect_material_expressions(amt_ids, "R", glow, "Id")
MEL.connect_material_expressions(selected, "", glow, "Sel")
MEL.connect_material_expressions(legacy_uv, "", glow, "UV")
atmos = atmosphere(m, glow, "", MAP_ATMOS, 400, 600)
MEL.connect_material_property(atmos, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
lib.save_loaded_asset(m)
unreal.log("CAMPAIGN-TILES|master oprettet")


master = m
m = new_material("M_Campaign1851TileOverview")
world_col = overview_sample(0)
world_season = season(m, world_col, "RGB", MAP_SEASON, -100, 300)
world_atmos = atmosphere(m, world_season, "", MAP_ATMOS, 400, 600)
MEL.connect_material_property(world_atmos, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
lib.save_loaded_asset(m)

# Instances collect tile references for the directory cook label below. C++ uses a
# dynamic instance of this same master and loads its two textures asynchronously.
for tile in manifest["tiles"]:
    name = f"MI_Tile_{tile['x']}_{tile['y']}"
    path = DEST + "/" + name
    instance = lib.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(instance, master)
    for layer, parameter in (("Color", "TileColor"), ("DetailMask", "TileDetailMask")):
        texture = lib.load_asset(DEST + f"/T_Tile_{tile['x']}_{tile['y']}_{layer}")
        MEL.set_material_instance_texture_parameter_value(instance, parameter, texture)
    MEL.set_material_instance_scalar_parameter_value(instance, "TileReady", 1.0)
    MEL.update_material_instance(instance)
    lib.save_loaded_asset(instance)
# A default-scanned PrimaryAssetLabel makes dynamically constructed soft paths
# cookable without changing Config. The map master is included explicitly.
label_path = DEST + "/PAL_MapTiles"
if lib.does_asset_exist(label_path):
    label = lib.load_asset(label_path)
else:
    label_factory = unreal.DataAssetFactory()
    label_factory.set_editor_property("data_asset_class", unreal.PrimaryAssetLabel)
    label = tools.create_asset("PAL_MapTiles", DEST, unreal.PrimaryAssetLabel, label_factory)
label.set_editor_property("label_assets_in_my_directory", True)
label.set_editor_property("explicit_assets", [master, m])
label_rules = unreal.PrimaryAssetRules()
label_rules.set_editor_property("priority", 1)
label_rules.set_editor_property("apply_recursively", True)
label_rules.set_editor_property("cook_rule", unreal.PrimaryAssetCookRule.ALWAYS_COOK)
label.set_editor_property("rules", label_rules)
lib.save_loaded_asset(label)
unreal.log(f"CAMPAIGN-TILES|import afsluttet|fliser={len(manifest['tiles'])}|lag=Color,DetailMask|oversigt=1")
