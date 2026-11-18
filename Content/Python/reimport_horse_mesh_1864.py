# Re-imports the rigged horse's mesh (SourceAssets/Horse/Horse_1_rigged_v3.fbx, the tail rebuilt 2026-11-13 by
# Tools/Animation/fix_horse_tail.py) over /Game/Units/Horse/SK_Horse_1864 on its existing skeleton. The clips are left alone.
# Copy SK_Horse_1864.uasset to a backup folder on disk before running.
# Run (PowerShell, headless, no game running):
#   UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<this file> -unattended -nosplash -stdout
import os
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MESH_FBX = os.environ.get('PROJECT1864_HORSE_FBX', os.path.join(ROOT, 'SourceAssets', 'Horse', 'Horse_1_rigged_v3.fbx'))
DEST = '/Game/Units/Horse'
LOG = 'HORSE-REIMPORT '


def say(msg):
    unreal.log_warning(LOG + msg)


skeleton = unreal.load_asset(DEST + '/SK_Horse_1864_Skeleton')
before = unreal.load_asset(DEST + '/SK_Horse_1864')
if not isinstance(skeleton, unreal.Skeleton) or not isinstance(before, unreal.SkeletalMesh):
    raise RuntimeError('SK_Horse_1864 or its skeleton is missing')


def bone_names():
    # The skeleton's bones by name; an empty list if this engine's Python has no such call (then the check is skipped).
    try:
        return [str(b) for b in skeleton.get_reference_pose().get_bone_names()]
    except Exception as e:
        say('bone names not readable: %s' % e)
        return []


bones_before = bone_names()

options = unreal.FbxImportUI()
for key, value in {'import_as_skeletal': True, 'import_mesh': True, 'import_animations': False, 'import_materials': False,
                   'import_textures': False, 'create_physics_asset': False, 'automated_import_should_detect_type': False,
                   'mesh_type_to_import': unreal.FBXImportType.FBXIT_SKELETAL_MESH, 'skeleton': skeleton}.items():
    options.set_editor_property(key, value)
task = unreal.AssetImportTask()
for key, value in {'filename': MESH_FBX, 'destination_path': DEST, 'destination_name': 'SK_Horse_1864', 'automated': True,
                   'replace_existing': True, 'save': True, 'options': options, 'factory': unreal.FbxFactory()}.items():
    task.set_editor_property(key, value)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

mesh = unreal.load_asset(DEST + '/SK_Horse_1864')
problems = []
if not isinstance(mesh, unreal.SkeletalMesh):
    problems.append('mesh missing after import')
else:
    if mesh.get_editor_property('skeleton') != skeleton:
        problems.append('mesh is not on SK_Horse_1864_Skeleton')
    material = unreal.load_asset('/Game/Units/Items/M_Horse')
    if material:
        unreal.MaterialEditingLibrary.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
        slots = list(mesh.get_editor_property('materials'))
        for slot in slots:
            slot.set_editor_property('material_interface', material)
        mesh.set_editor_property('materials', slots)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    else:
        problems.append('M_Horse missing')
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    bones_after = bone_names()
    if bones_after != bones_before:
        problems.append('skeleton bones changed %d -> %d' % (len(bones_before), len(bones_after)))
    b = mesh.get_bounds()
    say('SK_Horse_1864 bones=%d extent=(%.1f %.1f %.1f) cm sockets-bone saddle_root=%s' % (
        len(bones_after), b.box_extent.x, b.box_extent.y, b.box_extent.z, 'saddle_root' in bones_after))
say('DONE problems=%d %s' % (len(problems), '; '.join(problems)))
