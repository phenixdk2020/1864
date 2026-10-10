"""Importér Reference/Battle/Audio til /Game/Battle/Audio i Unreal Editor."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = root / 'Reference/Battle/Audio'
files = sorted(source.glob('*.wav'))
if len(files) != 17:
    raise RuntimeError('Generér først alle 17 WAV-filer med Tools/Battle/make_battle_sounds.py')
tools = unreal.AssetToolsHelpers.get_asset_tools()
for path in files:
    task = unreal.AssetImportTask()
    for key, value in [('filename', str(path)), ('destination_path', '/Game/Battle/Audio'),
                       ('destination_name', path.stem), ('automated', True),
                       ('replace_existing', True), ('save', False)]:
        task.set_editor_property(key, value)
    tools.import_asset_tasks([task])
    sound = unreal.load_asset('/Game/Battle/Audio/' + path.stem)
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError('SoundWave-import fejlede: ' + path.stem)
    sound.set_editor_property('looping', path.stem in ('distant_rumble', 'cavalry_hooves'))
    unreal.EditorAssetLibrary.save_loaded_asset(sound)
    unreal.log('PROJECT1864-AUDIO import ' + path.stem)
