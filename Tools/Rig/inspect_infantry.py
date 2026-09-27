# Lists infantry assets and the companies' soldier mesh in Battle_LookDev (diagnostics).
import unreal

for path in unreal.EditorAssetLibrary.list_assets("/Game/Units/Infantry/Animations", recursive=True):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, unreal.AnimSequence):
        unreal.log("INSPECT|anim|%s|%.2fs|frames=%d" % (path, unreal.AnimationLibrary.get_sequence_length(asset),
                                                       unreal.AnimationLibrary.get_num_frames(asset)))

subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.log("INSPECT|load_level=" + str(subsystem.load_level("/Game/Maps/Battle_LookDev")))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.log("INSPECT|world=" + (world.get_path_name() if world else "None"))
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
unreal.log("INSPECT|actors=%d" % len(actors))
for actor in actors:
    name = actor.get_class().get_name()
    if name in ("InfantryCompany",) or "Infantry" in actor.get_actor_label():
        mesh = actor.get_editor_property("SoldierMesh") if name == "InfantryCompany" else None
        unreal.log("INSPECT|actor|%s|%s|mesh=%s" % (name, actor.get_actor_label(), mesh.get_name() if mesh else None))
