# Sets up the 1851 campaign map in Unreal: imports the map textures, builds the map materials and
# creates the level /Game/Maps/Campaign1851. Run with the editor closed:
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=R:/.../Tools/Campaign/setup_campaign1851.py
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import unreal

REF = _PROJECT + "/Reference/Campaign1851/"
TEX_DEST = "/Game/Campaign1851/Map"
MAT_DEST = "/Game/Campaign1851"
LEVEL = "/Game/Maps/Campaign1851"
import json as _json
with open(_PROJECT + "/Data/Campaign1851/Denmark1851_Map.json", encoding="utf-8") as _f:
    _meta = _json.load(_f)
_ext = _meta["extentKm"]
SIZE_KM = (_ext["xMax"] - _ext["xMin"], _ext["yMax"] - _ext["yMin"])
PARCEL_TILE_KM = _meta.get("parcelTileKm", 8.0)

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("CAMPAIGN-SETUP|" + msg)


# ------------------------------------------------------------------ textures
tasks = []
for name in ("Denmark1851_Color", "Fields1851_Parcels", "Denmark1851_DetailMask", "Bornholm1851_Color"):
    t = unreal.AssetImportTask()
    t.filename = REF + name + ".png"
    t.destination_path = TEX_DEST
    t.automated = True
    t.save = True
    t.replace_existing = True
    tasks.append(t)
tools.import_asset_tasks(tasks)


def texture(name, srgb, wrap=False):
    tex = lib.load_asset("%s/%s" % (TEX_DEST, name))
    tex.set_editor_property("srgb", srgb)
    # Non-power-of-two sources get no mips; stretch so they mip and stream (UVs are unaffected).
    tex.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    addr = unreal.TextureAddress.TA_WRAP if wrap else unreal.TextureAddress.TA_CLAMP
    tex.set_editor_property("address_x", addr)
    tex.set_editor_property("address_y", addr)
    lib.save_loaded_asset(tex)
    log("texture %s %dx%d srgb=%s" % (name, tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), srgb))
    return tex


color_tex = texture("Denmark1851_Color", True)
parcel_tex = texture("Fields1851_Parcels", True, wrap=True)
mask_tex = texture("Denmark1851_DetailMask", False)  # alpha = farmland (no heath, dunes or woods)
texture("Bornholm1851_Color", True)
if lib.does_asset_exist(TEX_DEST + "/Fields1851_Detail"):  # replaced by the parcels in v00.00.17
    lib.delete_asset(TEX_DEST + "/Fields1851_Detail")


# ------------------------------------------------------------------ materials
def new_material(name):
    path = "%s/%s" % (MAT_DEST, name)
    if lib.does_asset_exist(path):
        lib.delete_asset(path)
    m = tools.create_asset(name, MAT_DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    return m


# Map (unlit: the painting already carries its shading). Close in, field parcels with hedgerows are laid
# over the farmland; they take the painted colour's brightness (hillshade) and fade out between
# 70 and 140 km camera depth so the overview stays the painting:
#   emissive = lerp(C, P * clamp(luma(C) / 0.14, 0.55, 1.45), mask.a * 0.8 * saturate(2 - depth / 7000))
def expr(cls, x, y, **props):
    e = MEL.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


m = new_material("M_Campaign1851Map")
col = expr(unreal.MaterialExpressionTextureSample, -1300, -300, texture=color_tex)
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1600, 100, u_tiling=SIZE_KM[0] / PARCEL_TILE_KM, v_tiling=SIZE_KM[1] / PARCEL_TILE_KM)
par = expr(unreal.MaterialExpressionTextureSample, -1300, 100, texture=parcel_tex)
MEL.connect_material_expressions(uv, "", par, "UVs")
mask = expr(unreal.MaterialExpressionTextureSample, -1300, 450, texture=mask_tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)

lum_w = expr(unreal.MaterialExpressionConstant3Vector, -1300, -50, constant=unreal.LinearColor(0.2126, 0.7152, 0.0722, 1.0))
luma = expr(unreal.MaterialExpressionDotProduct, -1000, -100)
MEL.connect_material_expressions(col, "RGB", luma, "A")
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
MEL.connect_material_expressions(sat, "", alpha, "B")
strength = expr(unreal.MaterialExpressionMultiply, -500, 500, const_b=0.8)
MEL.connect_material_expressions(alpha, "", strength, "A")

final = expr(unreal.MaterialExpressionLinearInterpolate, -300, 0)
MEL.connect_material_expressions(col, "RGB", final, "A")
MEL.connect_material_expressions(lit, "", final, "B")
MEL.connect_material_expressions(strength, "", final, "Alpha")
MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
lib.save_loaded_asset(m)
log("material M_Campaign1851Map")

# Backdrop: the colour the painted sheet fades to at its edges (sRGB 15,28,43 -> linear).
b = new_material("M_Campaign1851Backdrop")
c = MEL.create_material_expression(b, unreal.MaterialExpressionConstant3Vector, -400, 0)
c.set_editor_property("constant", unreal.LinearColor(0.0048, 0.0116, 0.0242, 1.0))
MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(b)
lib.save_loaded_asset(b)
log("material M_Campaign1851Backdrop")

# Scenery (3D towns, farms, trees): unlit vertex colour with the lighting baked in by C++.
# Vertex colours are stored as sRGB, so pow 2.2 back to linear; PerInstanceRandom varies each piece a little.
s = new_material("M_Campaign1851Scenery")
s.set_editor_property("used_with_instanced_static_meshes", True)
vc = MEL.create_material_expression(s, unreal.MaterialExpressionVertexColor, -900, 0)
gamma = MEL.create_material_expression(s, unreal.MaterialExpressionPower, -650, 0)
gamma.set_editor_property("const_exponent", 2.2)
MEL.connect_material_expressions(vc, "", gamma, "Base")
rnd = MEL.create_material_expression(s, unreal.MaterialExpressionPerInstanceRandom, -900, 250)
vary = MEL.create_material_expression(s, unreal.MaterialExpressionLinearInterpolate, -650, 250)
vary.set_editor_property("const_a", 0.84)
vary.set_editor_property("const_b", 1.1)
MEL.connect_material_expressions(rnd, "", vary, "Alpha")
lit = MEL.create_material_expression(s, unreal.MaterialExpressionMultiply, -400, 100)
MEL.connect_material_expressions(gamma, "", lit, "A")
MEL.connect_material_expressions(vary, "", lit, "B")
MEL.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(s)
lib.save_loaded_asset(s)
log("material M_Campaign1851Scenery")

# Construction: the scenery look (unlit vertex colour), masked above the BuildTop world height so
# a building on a site can rise storey by storey. Two-sided: the cut walls show their inside.
k = new_material("M_Campaign1851Construction")
k.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
k.set_editor_property("two_sided", True)
kvc = MEL.create_material_expression(k, unreal.MaterialExpressionVertexColor, -700, 0)
kgamma = MEL.create_material_expression(k, unreal.MaterialExpressionPower, -450, 0)
kgamma.set_editor_property("const_exponent", 2.2)
MEL.connect_material_expressions(kvc, "", kgamma, "Base")
MEL.connect_material_property(kgamma, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
top = MEL.create_material_expression(k, unreal.MaterialExpressionScalarParameter, -900, 300)
top.set_editor_property("parameter_name", "BuildTop")
top.set_editor_property("default_value", 1.0e7)
wpos = MEL.create_material_expression(k, unreal.MaterialExpressionWorldPosition, -900, 450)
wz = MEL.create_material_expression(k, unreal.MaterialExpressionComponentMask, -700, 450)
wz.set_editor_property("r", False)
wz.set_editor_property("g", False)
wz.set_editor_property("b", True)
MEL.connect_material_expressions(wpos, "", wz, "")
left = MEL.create_material_expression(k, unreal.MaterialExpressionSubtract, -500, 350)
MEL.connect_material_expressions(top, "", left, "A")
MEL.connect_material_expressions(wz, "", left, "B")
sharp = MEL.create_material_expression(k, unreal.MaterialExpressionMultiply, -350, 350)
sharp.set_editor_property("const_b", 4.0)
MEL.connect_material_expressions(left, "", sharp, "A")
ksat = MEL.create_material_expression(k, unreal.MaterialExpressionSaturate, -200, 350)
MEL.connect_material_expressions(sharp, "", ksat, "")
MEL.connect_material_property(ksat, "", unreal.MaterialProperty.MP_OPACITY_MASK)
MEL.recompile_material(k)
lib.save_loaded_asset(k)
log("material M_Campaign1851Construction")

# Building cards for the town panel (from the 1851 building illustrations, 512 px).
BUILDINGS_DEST = "/Game/Campaign1851/Buildings"
cards = []
for name in ("T_Barracks_Infantry", "T_Module_Stables", "T_Module_Depot", "T_Module_Infirmary"):
    task = unreal.AssetImportTask()
    task.filename = REF + "Buildings/" + name + ".png"
    task.destination_path = BUILDINGS_DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    cards.append(task)
tools.import_asset_tasks(cards)
for name in ("T_Barracks_Infantry", "T_Module_Stables", "T_Module_Depot", "T_Module_Infirmary"):
    card = lib.load_asset(BUILDINGS_DEST + "/" + name)
    card.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    card.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    lib.save_loaded_asset(card)
    log("texture " + name)

# ------------------------------------------------------------------ level
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if lib.does_asset_exist(LEVEL):
    level_sub.load_level(LEVEL)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in actors.get_all_level_actors():
        if a.get_class().get_name() in ("Campaign1851Map", "DirectionalLight", "PostProcessVolume"):
            actors.destroy_actor(a)
else:
    level_sub.new_level(LEVEL)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_class = unreal.load_class(None, "/Script/Game1864.Campaign1851Map")
actors.spawn_actor_from_class(map_class, unreal.Vector(0, 0, 0)).set_actor_label("Campaign1851Map")

sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 50000), unreal.Rotator(0.0, -48.0, -35.0))
sun.light_component.set_editor_property("intensity", 6.0)

pp = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
pp.set_editor_property("unbound", True)
s = pp.get_editor_property("settings")
for prop, value in (
    ("override_auto_exposure_method", True), ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
    ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.0),
    ("override_auto_exposure_apply_physical_camera_exposure", True), ("auto_exposure_apply_physical_camera_exposure", False),
    ("override_bloom_intensity", True), ("bloom_intensity", 0.0),
    ("override_vignette_intensity", True), ("vignette_intensity", 0.0),
    ("override_motion_blur_amount", True), ("motion_blur_amount", 0.0),
):
    s.set_editor_property(prop, value)
pp.set_editor_property("settings", s)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/Game1864.Campaign1851GameMode"))
level_sub.save_current_level()
log("level %s saved" % LEVEL)
