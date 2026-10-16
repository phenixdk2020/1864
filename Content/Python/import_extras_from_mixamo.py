# Imports the extra combat clips straight from the user's original Mixamo FBX (with mesh and bind pose).
# The Blender-processed copies (build_mixamo_extra_set.py) came in lying down (tested 2026-10-16); the raw Mixamo files stand.
import unreal
from pathlib import Path
MV=Path(r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView')
INF=MV/'Animationer M\u00e6nd'; CAV=MV/'Danish Cavalry'
CLIPS=[('ReloadKneelingB',INF,'Reload.fbx'),('ReloadKneeling',INF,'Reload kneeling.fbx'),('RiflePunch',INF,'Rifle Punch.fbx'),
       ('SideStep',INF,'Rifle Side Step.fbx'),('Turn',INF,'Rifle Turn.fbx'),('Signal',INF,'Military Signaling.fbx'),
       ('DeathBackHeadshot',INF,'Death From Back Headshot.fbx'),('SabreDraw',CAV,'Draw Sword 1.fbx'),('SabreWithdraw',CAV,'Withdrawing Sword.fbx'),
       ('SabreSheath',CAV,'Sheathing Sword.fbx'),('SabreSheath2',CAV,'Sheath Sword 2.fbx'),('SabreCombo',CAV,'One Hand Sword Combo.fbx'),('SabreFight',CAV,'Sword Fight One.fbx')]
skeleton=unreal.load_asset('/Game/Units/Human/Skeletons/SK_Human_1864')
dest='/Game/Units/Human/CombatAnimations'
for clip,folder,fn in CLIPS:
    name='A_Combat_'+clip; path=dest+'/'+name
    if not (folder/fn).exists():
        unreal.log_warning('MIXIMPORT missing '+str(folder/fn)); continue
    if unreal.EditorAssetLibrary.does_asset_exist(path): unreal.EditorAssetLibrary.delete_asset(path)   # a replace-import keeps old settings
    ui=unreal.FbxImportUI(); ui.import_mesh=False; ui.import_as_skeletal=True; ui.import_animations=True
    ui.import_materials=False; ui.import_textures=False; ui.skeleton=skeleton; ui.automated_import_should_detect_type=False
    ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION; ui.override_animation_name=name
    t=unreal.AssetImportTask(); t.filename=str(folder/fn); t.destination_path=dest; t.destination_name=name
    t.automated=True; t.replace_existing=True; t.save=True; t.options=ui; t.factory=unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    s=unreal.load_asset(path)
    unreal.log_warning('MIXIMPORT '+name+' '+(str(s.get_play_length()) if s else 'FAILED'))
