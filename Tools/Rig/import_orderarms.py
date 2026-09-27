# Imports the order-arms crowd soldier as a Nanite static mesh and puts it on every InfantryCompany
# in Battle_LookDev. Run with the editor closed:
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Tools/Rig/import_orderarms.py
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import unreal

SOURCE = _PROJECT + r"\Reference\Units\MultiView\3D figurer\SM_Infantry_OrderArms.glb"
DEST = "/Game/Units/Infantry/OrderArms"
LEVEL = "/Game/Maps/Battle_LookDev"

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = DEST
task.automated = True
task.save = True
task.replace_existing = True
if not unreal.EditorAssetLibrary.does_directory_have_assets(DEST):
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

mesh = None
for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log("ORDERARMS|asset=" + path + "|class=" + asset.get_class().get_name())
    if isinstance(asset, unreal.StaticMesh):
        mesh = asset

if mesh is None:
    raise RuntimeError("ORDERARMS|no static mesh imported")

# Nanite: hundreds of 40k-triangle soldiers per company only stay cheap with Nanite.
settings = mesh.get_editor_property("nanite_settings")
settings.set_editor_property("enabled", True)
mesh.set_editor_property("nanite_settings", settings)
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
unreal.log("ORDERARMS|nanite=" + str(mesh.get_editor_property("nanite_settings").get_editor_property("enabled")))

# Put it on the companies and drop the single preview figure.
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL)
# World Partition: actors live in external packages and are not loaded in a commandlet; load them all.
descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.get_editor_property("guid") for d in descs])
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
unreal.log("ORDERARMS|actors=%d of %d descs" % (len(actors), len(descs)))
for actor in actors:
    cls = actor.get_class().get_name()
    if cls == "InfantryCompany":
        actor.set_editor_property("SoldierMesh", mesh)
        actor.rebuild_formation()
        unreal.log("ORDERARMS|company=" + actor.get_actor_label())
    elif actor.get_actor_label().startswith("Infantry_Rigged_Preview"):
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)
        unreal.log("ORDERARMS|removed preview")

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("ORDERARMS|saved")
