# Imports Mixamo animations (downloaded "with skin") as animation sequences on the rigged
# infantry skeleton; the embedded meshes are ignored. Run with the editor closed:
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Tools/Rig/import_animations.py
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import os

import unreal

FOLDER = _PROJECT + r"\Reference\Units\MultiView\3D figurer"
SKELETON = "/Game/Units/Infantry/Infantry_Rigged/SkeletalMeshes/Infantry_Rigged_Skeleton"
DEST = "/Game/Units/Infantry/Animations"
CLIPS = ["Idle", "Walking", "Rifle Aiming Idle", "Firing Rifle", "Reloading", "Walking To Dying"]

skeleton = unreal.load_asset(SKELETON)
tasks = []
for clip in CLIPS:
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("skeleton", skeleton)
    common.set_editor_property("import_only_animations", True)

    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(pipeline)

    task = unreal.AssetImportTask()
    task.filename = os.path.join(FOLDER, clip + ".fbx")
    task.destination_path = DEST
    task.destination_name = "A_Infantry_" + clip.replace(" ", "")
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.set_editor_property("options", stack)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    info = asset.get_class().get_name()
    if isinstance(asset, unreal.AnimSequence):
        info += "|length=%.2fs|skeleton=%s" % (asset.get_editor_property("sequence_length") if hasattr(asset, "sequence_length") else unreal.AnimationLibrary.get_sequence_length(asset),
                                            asset.get_editor_property("skeleton").get_name())
    unreal.log("ANIM-IMPORT|" + path + "|" + info)
