# Imports the colour bearer's walk and stand (Tools/Animation/build_colour_bearer.py, after Reference/Units/MultiView/Flag/Fane_March_*.png).
# Based on the v9 import (Blender, exported WITH skinned mesh and bind pose, built from the raw Mixamo walk)
# over the game's three shoulder clips. The old assets are first duplicated to CombatAnimations/Backup_v4, so they can be
# restored if v9 imports lying down. Modelled on import_extras_from_mixamo.py (delete first: a replace-import keeps old settings).
import unreal
from pathlib import Path
SRC = Path(r'R:\Onedrive\1864-Campaign\Saved\Codex\colour_bearer')
CLIPS = [('ColourWalk', 'A_Combat_ColourWalk.fbx'), ('ColourIdle', 'A_Combat_ColourIdle.fbx')]
skeleton = unreal.load_asset('/Game/Units/Human/Skeletons/SK_Human_1864')
dest = '/Game/Units/Human/CombatAnimations'
backup = dest + '/Backup_v4'
for clip, fn in CLIPS:
    name = 'A_Combat_' + clip
    path = dest + '/' + name
    if not (SRC / fn).exists():
        unreal.log_warning('COLOURBEARER missing ' + str(SRC / fn)); continue
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.does_asset_exist(backup + '/' + name):
            unreal.EditorAssetLibrary.duplicate_asset(path, backup + '/' + name)
        unreal.EditorAssetLibrary.delete_asset(path)
    ui = unreal.FbxImportUI(); ui.import_mesh = False; ui.import_as_skeletal = True; ui.import_animations = True
    ui.import_materials = False; ui.import_textures = False; ui.skeleton = skeleton; ui.automated_import_should_detect_type = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION; ui.override_animation_name = name
    # Blender's FBX export adds an 'Armature' root that lays the clip down in UE; roll +90 stands it up
    # (probe 2026-10-18: Saved/Codex/shoulder_v9/probe_orientation.py, hips 93 cm above ground with roll 90, along the ground without).
    ui.anim_sequence_import_data.set_editor_property('import_rotation', unreal.Rotator(roll=90, pitch=0, yaw=0))
    t = unreal.AssetImportTask(); t.filename = str(SRC / fn); t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True; t.options = ui; t.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    s = unreal.load_asset(path)
    unreal.log_warning('COLOURBEARER ' + name + ' ' + (str(s.get_play_length()) if s else 'FAILED'))
