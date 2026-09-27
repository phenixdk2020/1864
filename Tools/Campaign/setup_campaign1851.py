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
DETAIL_TILE_KM = _meta.get("detailTileKm", 2.5)

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("CAMPAIGN-SETUP|" + msg)


# ------------------------------------------------------------------ textures
tasks = []
for name in ("Denmark1851_Color", "Fields1851_Detail", "Denmark1851_DetailMask", "Bornholm1851_Color"):
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
detail_tex = texture("Fields1851_Detail", False, wrap=True)  # neutral 0.5 must stay 0.5 for the x2 multiply
mask_tex = texture("Denmark1851_DetailMask", False)
texture("Bornholm1851_Color", True)


# ------------------------------------------------------------------ materials
def new_material(name):
    path = "%s/%s" % (MAT_DEST, name)
    if lib.does_asset_exist(path):
        lib.delete_asset(path)
    m = tools.create_asset(name, MAT_DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    return m


# Map: painted colour x lerp(1, detail*2, mask) -> emissive (unlit: the painting already carries its shading).
m = new_material("M_Campaign1851Map")
col = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -900, -200)
col.set_editor_property("texture", color_tex)
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1200, 150)
uv.set_editor_property("u_tiling", SIZE_KM[0] / DETAIL_TILE_KM)
uv.set_editor_property("v_tiling", SIZE_KM[1] / DETAIL_TILE_KM)
det = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -900, 150)
det.set_editor_property("texture", detail_tex)
det.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
MEL.connect_material_expressions(uv, "", det, "UVs")
mask = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -900, 450)
mask.set_editor_property("texture", mask_tex)
mask.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
x2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -600, 150)
x2.set_editor_property("const_b", 2.0)
MEL.connect_material_expressions(det, "RGB", x2, "A")
blend = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 200)
blend.set_editor_property("const_a", 1.0)
MEL.connect_material_expressions(x2, "", blend, "B")
MEL.connect_material_expressions(mask, "A", blend, "Alpha")
final = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 0)
MEL.connect_material_expressions(col, "RGB", final, "A")
MEL.connect_material_expressions(blend, "", final, "B")
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
