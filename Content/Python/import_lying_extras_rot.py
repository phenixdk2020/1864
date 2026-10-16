# Re-import the extra clips that lie down, with the legacy FBX importer and a -90 degree roll (found by test on 2026-10-16).
import unreal
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
src=root/'SourceAssets/InfantryCombat'
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
skeleton=unreal.load_asset('/Game/Units/Human/Skeletons/SK_Human_1864')
dest='/Game/Units/Human/CombatAnimations'
for clip in ['SabreDraw','SabreFight','SabreCombo','RiflePunch','ReloadKneeling','SabreSheath','SabreSheath2','SabreWithdraw']:
    name='A_Combat_'+clip
    ui=unreal.FbxImportUI(); ui.import_mesh=False; ui.import_as_skeletal=True; ui.import_animations=True
    ui.import_materials=False; ui.import_textures=False; ui.skeleton=skeleton; ui.automated_import_should_detect_type=False
    ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
    ui.get_editor_property('anim_sequence_import_data').set_editor_property('import_rotation',unreal.Rotator(roll=-90,pitch=0,yaw=0))
    ui.override_animation_name=name
    t=unreal.AssetImportTask(); t.filename=str(src/(name+'.fbx')); t.destination_path=dest; t.destination_name=name
    t.automated=True; t.replace_existing=True; t.save=True; t.options=ui; t.factory=unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    s=unreal.load_asset(dest+'/'+name)
    unreal.log_warning('FIX8 '+name+' '+(str(s.get_play_length()) if s else 'FAILED'))
