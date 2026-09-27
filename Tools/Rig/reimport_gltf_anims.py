# Imports Infantry_Rigged.glb (mesh + skeleton + embedded animations) and moves the animation
# sequences to /Game/Units/Infantry/Animations/A_Infantry_<Clip>. Run with the editor closed.
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import os

import unreal

SOURCE = _PROJECT + r"\Reference\Units\MultiView\3D figurer\Infantry_Rigged.glb"
RIG_DEST = "/Game/Units/Infantry/Infantry_Rigged"
ANIM_DEST = "/Game/Units/Infantry/Animations"
CLIPS = ["Idle", "Walking", "RifleAimingIdle", "FiringRifle", "Reloading", "WalkingToDying"]
lib = unreal.EditorAssetLibrary

# Old FBX-imported sequences (wrong axis conversion) go first.
for path in lib.list_assets(ANIM_DEST, recursive=True):
    unreal.log("GLTFANIM|delete %s -> %s" % (path, lib.delete_asset(path)))

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = "/Game/Units/Infantry"
task.automated = True
task.save = True
task.replace_existing = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

for path in lib.list_assets(RIG_DEST, recursive=True):
    asset = lib.load_asset(path)
    if not isinstance(asset, unreal.AnimSequence):
        continue
    name = asset.get_name()
    clip = next((c for c in sorted(CLIPS, key=len, reverse=True) if name.endswith(c) or ("_" + c) in name or name == c), None)
    if clip is None:
        unreal.log_warning("GLTFANIM|unmatched sequence " + path)
        continue
    target = "%s/A_Infantry_%s" % (ANIM_DEST, clip)
    ok = lib.rename_asset(path, target)
    unreal.log("GLTFANIM|%s -> %s (%s) %.2fs" % (name, target, ok, unreal.AnimationLibrary.get_sequence_length(lib.load_asset(target))))

lib.save_directory(ANIM_DEST)
lib.save_directory(RIG_DEST)
unreal.log("GLTFANIM|done")
