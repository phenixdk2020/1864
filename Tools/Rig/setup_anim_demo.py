# Imports the musket and places an InfantryAnimDemo in Battle_LookDev, in the gap between
# Kompagni_1 and Kompagni_2, facing the same way as the companies. Run with the editor closed.
import os as _os, unreal as _ue
_PROJECT = _os.path.normpath(_ue.Paths.convert_relative_path_to_full(_ue.Paths.project_dir()))
import unreal

MUSKET_SRC = _PROJECT + r"\Reference\Units\MultiView\3D figurer\SM_Musket.glb"
MUSKET_DEST = "/Game/Units/Infantry/Musket"
LEVEL = "/Game/Maps/Battle_LookDev"

if not unreal.EditorAssetLibrary.does_directory_have_assets(MUSKET_DEST):
    task = unreal.AssetImportTask()
    task.filename = MUSKET_SRC
    task.destination_path = MUSKET_DEST
    task.automated = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
for path in unreal.EditorAssetLibrary.list_assets(MUSKET_DEST, recursive=True):
    unreal.log("ANIMDEMO|asset=" + path)

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL)
descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
unreal.WorldPartitionBlueprintLibrary.load_actors([d.get_editor_property("guid") for d in descs])

actors_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors_sub.get_all_level_actors():
    cls = actor.get_class().get_name()
    if cls == "InfantryAnimDemo":
        actors_sub.destroy_actor(actor)
    elif cls == "InfantryCompany" and actor.get_editor_property("SoldierMaterial") is not None:
        # The textured crowd soldier must keep its own materials.
        actor.set_editor_property("SoldierMaterial", None)
        actor.rebuild_formation()
        unreal.log("ANIMDEMO|cleared SoldierMaterial on " + actor.get_actor_label())

demo_class = unreal.load_class(None, "/Script/Game1864.InfantryAnimDemo")
demo = actors_sub.spawn_actor_from_class(demo_class, unreal.Vector(-600.0, -3600.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
demo.set_actor_label("Infantry_AnimDemo")
unreal.log("ANIMDEMO|placed=" + demo.get_path_name())

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("ANIMDEMO|saved")
