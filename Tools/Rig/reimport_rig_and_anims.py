# Rebuilds the rigged infantryman and its animations from scratch (after a skeleton change).
# Run with the editor closed:
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Tools/Rig/reimport_rig_and_anims.py
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import os

import unreal

FOLDER = _PROJECT + r"\Reference\Units\MultiView\3D figurer"
RIG_DEST = "/Game/Units/Infantry/Infantry_Rigged"
ANIM_DEST = "/Game/Units/Infantry/Animations"
CLIPS = ["Idle", "Walking", "Rifle Aiming Idle", "Firing Rifle", "Reloading", "Walking To Dying"]
tools = unreal.AssetToolsHelpers.get_asset_tools()

for folder in (ANIM_DEST, RIG_DEST):
    if unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.log("REIMPORT|delete %s -> %s" % (folder, unreal.EditorAssetLibrary.delete_directory(folder)))

task = unreal.AssetImportTask()
task.filename = os.path.join(FOLDER, "Infantry_Rigged.glb")
task.destination_path = "/Game/Units/Infantry"
task.automated = True
task.save = True
tools.import_asset_tasks([task])

skeleton = unreal.load_asset(RIG_DEST + "/SkeletalMeshes/Infantry_Rigged_Skeleton")
mesh = unreal.load_asset(RIG_DEST + "/SkeletalMeshes/Infantry_Rigged")
unreal.log("REIMPORT|skeleton=%s mesh=%s" % (skeleton, mesh))

tasks = []
for clip in CLIPS:
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("skeleton", skeleton)
    common.set_editor_property("import_only_animations", True)
    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(pipeline)
    t = unreal.AssetImportTask()
    t.filename = os.path.join(FOLDER, clip + ".fbx")
    t.destination_path = ANIM_DEST
    t.destination_name = "A_Infantry_" + clip.replace(" ", "")
    t.automated = True
    t.save = True
    t.replace_existing = True
    t.set_editor_property("options", stack)
    tasks.append(t)
try:
    tools.import_asset_tasks(tasks)
except Exception as e:  # missing-track warnings are raised as errors; the sequences are still created
    unreal.log_warning("REIMPORT|anim import reported: " + str(e).splitlines()[0])

for path in unreal.EditorAssetLibrary.list_assets(ANIM_DEST, recursive=True):
    anim = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(anim, unreal.AnimSequence):
        tracks = unreal.AnimationLibrary.get_animation_track_names(anim)
        bound = [t for t in tracks if skeleton.get_editor_property("bone_tree") is None or True]
        unreal.log("REIMPORT|anim|%s|%.2fs|tracks=%d|first=%s" % (anim.get_name(), unreal.AnimationLibrary.get_sequence_length(anim),
                                                               len(tracks), ",".join(str(t) for t in list(tracks)[:4])))
