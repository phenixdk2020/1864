# The rigged horse SK_Horse_1864 uses /Game/Units/Items/M_Horse, made for the old static horse: without the skeletal-mesh usage flag the
# engine draws the default grey material on it (log: "Material M_Horse missing usage flag SkeletalMesh!", QA 2026-11-12: flat grey, light
# triangles at the hocks). Sets the flag on every material of SK_Horse_1864 and saves. Run headless (PowerShell, no game running):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<this file> -unattended -nosplash -stdout
import unreal

LOG = 'HORSE-MATERIAL '
mel = unreal.MaterialEditingLibrary
mesh = unreal.load_asset('/Game/Units/Horse/SK_Horse_1864')
done = set()
for slot in (mesh.get_editor_property('materials') if mesh else []):
    mi = slot.get_editor_property('material_interface')
    base = mi.get_base_material() if mi else None
    if not base or base.get_path_name() in done:
        continue
    done.add(base.get_path_name())
    changed = mel.set_material_usage(base, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    mel.recompile_material(base)
    unreal.EditorAssetLibrary.save_loaded_asset(base)
    unreal.log_warning(LOG + '%s skeletal usage set (needs_recompile=%s)' % (base.get_path_name(), changed))
unreal.log_warning(LOG + 'DONE materials=%d mesh=%s' % (len(done), 'ok' if mesh else 'MISSING'))
