# Imports the rigged infantryman into /Game/Units/Infantry (run via -run=pythonscript).
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import unreal

SOURCE = _PROJECT + r"\Reference\Units\MultiView\3D figurer\Infantry_Rigged.glb"

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = "/Game/Units/Infantry"
task.automated = True
task.save = True
task.replace_existing = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

unreal.log("INFANTRY-IMPORT|objects=" + ", ".join(str(p) for p in task.imported_object_paths))
for path in unreal.EditorAssetLibrary.list_assets("/Game/Units/Infantry", recursive=True):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log("INFANTRY-IMPORT|asset=" + path + "|class=" + asset.get_class().get_name())
