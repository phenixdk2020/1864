# Imports the three march-death clips (soldier hit while marching in column, rifle on the right shoulder).
# Modelled on import_shoulder_v10.py: same skeleton, same options, exported WITH skinned mesh/bind pose (Tools/Animation/build_march_death_set.py).
# Source: SourceAssets/InfantryCombat. Existing assets of the same name are first duplicated to CombatAnimations/Backup_marchdeath.
# NOT run by the Blender agent; run from the editor via unreal-integration.
import unreal
from pathlib import Path
SRC = Path(r'C:\Projekter\1864-Campaign\SourceAssets\InfantryCombat')
CLIPS = [('DeathMarchFront', 'A_Combat_DeathMarchFront.fbx'), ('DeathMarchBack', 'A_Combat_DeathMarchBack.fbx'), ('DeathMarchCrumple', 'A_Combat_DeathMarchCrumple.fbx')]
skeleton = unreal.load_asset('/Game/Units/Human/Skeletons/SK_Human_1864')
dest = '/Game/Units/Human/CombatAnimations'
backup = dest + '/Backup_marchdeath'
for clip, fn in CLIPS:
    name = 'A_Combat_' + clip
    path = dest + '/' + name
    if not (SRC / fn).exists():
        unreal.log_warning('MARCHDEATH missing ' + str(SRC / fn)); continue
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.does_asset_exist(backup + '/' + name):
            unreal.EditorAssetLibrary.duplicate_asset(path, backup + '/' + name)
        unreal.EditorAssetLibrary.delete_asset(path)
    ui = unreal.FbxImportUI(); ui.import_mesh = False; ui.import_as_skeletal = True; ui.import_animations = True
    ui.import_materials = False; ui.import_textures = False; ui.skeleton = skeleton; ui.automated_import_should_detect_type = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION; ui.override_animation_name = name
    # Blender's FBX export adds an 'Armature' root that lays the clip down in UE; roll +90 stands it up (as for the shoulder clips)
    ui.anim_sequence_import_data.set_editor_property('import_rotation', unreal.Rotator(roll=90, pitch=0, yaw=0))
    t = unreal.AssetImportTask(); t.filename = str(SRC / fn); t.destination_path = dest; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True; t.options = ui; t.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    s = unreal.load_asset(path)
    unreal.log_warning('MARCHDEATH ' + name + ' ' + (str(s.get_play_length()) if s else 'FAILED'))
