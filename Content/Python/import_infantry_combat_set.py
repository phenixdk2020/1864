import unreal, json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source=root/'SourceAssets/InfantryCombat'
manifest=json.loads((source/'manifest.json').read_text())
skeleton=unreal.load_asset('/Game/Units/Human/Skeletons/SK_Human_1864')
if not skeleton:
    raise RuntimeError('Shared human skeleton missing')
report={'skeleton':skeleton.get_path_name(),'meshes':{},'clips':[]}
for unit,path in {'Livgarden':'/Game/Units/Danish/Livgarden1864/Mesh/SK_DK_Livgarden_1864',
                  'Swedish':'/Game/Units/Swedish/Infantry1864/Mesh/SK_SE_Infantry_1864'}.items():
    mesh=unreal.load_asset(path)
    skel=mesh.get_editor_property('skeleton')
    if skel != skeleton:
        raise RuntimeError('Skeleton mismatch: '+unit)
    report['meshes'][unit]=mesh.get_path_name()
for clip in manifest['clips']:
    name='A_Combat_'+clip['name']
    dest='/Game/Units/Human/CombatAnimations'
    ui=unreal.FbxImportUI()
    ui.import_mesh=False
    ui.import_as_skeletal=True
    ui.import_animations=True
    ui.import_materials=False
    ui.import_textures=False
    ui.skeleton=skeleton
    ui.automated_import_should_detect_type=False
    ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
    ui.override_animation_name=name
    task=unreal.AssetImportTask()
    task.filename=str(source/clip['file'])
    task.destination_path=dest
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    task.options=ui
    task.factory=unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    seq=unreal.load_asset(dest+'/'+name)
    if not seq or seq.get_editor_property('skeleton') != skeleton:
        raise RuntimeError('Animation import failed: '+name)
    duration=seq.get_play_length()
    if duration < .1:
        raise RuntimeError('Empty clip: '+name)
    report['clips'].append({'name':name,'asset':seq.get_path_name(),'seconds':duration})
    unreal.log('COMBAT_ANIMATION_OK '+name+' '+str(duration))
unreal.EditorAssetLibrary.save_directory('/Game/Units/Human/CombatAnimations')
(source/'unreal_import_report.json').write_text(json.dumps(report,indent=2))
unreal.log('COMBAT_ANIMATION_SET_VERIFIED')
