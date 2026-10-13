# Manuel import: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<denne fil>
# Runtime bruger fortsat SM_Horse_Static. Dette script starter Blender kun hvis mesh-FBX mangler.
import os
import shutil
import subprocess
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DEST = '/Game/Units/Horse'
BLEND = os.environ.get('PROJECT1864_HORSE_BLEND',
    'R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView/Items/Horse_1_rigged_v3.blend')
MESH_FBX = os.environ.get('PROJECT1864_HORSE_FBX', os.path.join(ROOT, 'Saved', 'HorseImport', 'Horse_1_rigged_v3.fbx'))
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def export_mesh_if_needed():
    if os.path.isfile(MESH_FBX):
        return
    blender = os.environ.get('PROJECT1864_BLENDER') or shutil.which('blender')
    if not blender:
        raise RuntimeError('Angiv PROJECT1864_BLENDER eller en eksporteret PROJECT1864_HORSE_FBX')
    if not os.path.isfile(BLEND):
        raise FileNotFoundError(BLEND)
    os.makedirs(os.path.dirname(os.path.abspath(MESH_FBX)), exist_ok=True)
    export_script = os.path.join(os.path.dirname(os.path.abspath(MESH_FBX)), 'export_horse_mesh.py')
    with open(export_script, 'w', encoding='utf-8') as stream:
        stream.write("""import bpy
bpy.ops.object.select_all(action='DESELECT')
rigs = [obj for obj in bpy.context.scene.objects if obj.type == 'ARMATURE']
if len(rigs) != 1:
    raise RuntimeError('Forventede præcis én hesterig')
rig = rigs[0]
rig.select_set(True)
bpy.context.view_layer.objects.active = rig
for obj in bpy.context.scene.objects:
    if obj.type == 'MESH' and any(mod.type == 'ARMATURE' and mod.object == rig for mod in obj.modifiers):
        obj.select_set(True)
if len(bpy.context.selected_objects) < 2:
    raise RuntimeError('Ingen mesh bundet til hesteriggen')
bpy.ops.export_scene.fbx(filepath=%r, use_selection=True, object_types={'ARMATURE', 'MESH'},
    add_leaf_bones=False, bake_anim=False, axis_forward='-Z', axis_up='Y',
    apply_unit_scale=True, use_armature_deform_only=False)
""" % MESH_FBX)
    subprocess.run([blender, '--background', BLEND, '--python-exit-code', '1', '--python', export_script], check=True)
    if not os.path.isfile(MESH_FBX):
        raise RuntimeError('Blender skrev ingen mesh-FBX')


def import_fbx(filename, name, skeletal_mesh, skeleton=None):
    options = unreal.FbxImportUI()
    properties = {
        'import_as_skeletal': True,
        'import_mesh': skeletal_mesh,
        'import_animations': not skeletal_mesh,
        'import_materials': False,
        'import_textures': False,
        'create_physics_asset': False,
        'automated_import_should_detect_type': False,
        'mesh_type_to_import': unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal_mesh else unreal.FBXImportType.FBXIT_ANIMATION,
    }
    if skeleton:
        properties['skeleton'] = skeleton
    for key, value in properties.items():
        options.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    for key, value in {'filename': filename, 'destination_path': DEST, 'destination_name': name,
                       'automated': True, 'replace_existing': False, 'save': True,
                       'options': options, 'factory': unreal.FbxFactory()}.items():
        task.set_editor_property(key, value)
    TOOLS.import_asset_tasks([task])
    asset = unreal.load_asset(DEST + '/' + name)
    expected = unreal.SkeletalMesh if skeletal_mesh else unreal.AnimSequence
    if not isinstance(asset, expected):
        raise RuntimeError('Import fejlede: ' + name)
    return asset


def main():
    animation_files = [os.path.join(ROOT, 'Reference', 'Animations', 'Horse', 'A_Horse_' + gait + '.fbx')
                       for gait in ('Stand', 'Walk', 'Trot', 'Canter', 'Gallop', 'Rear', 'Fall')]
    for filename in animation_files:
        if not os.path.isfile(filename):
            raise FileNotFoundError(filename)
    mesh = unreal.load_asset(DEST + '/SK_Horse_1864')
    skeleton = unreal.load_asset(DEST + '/SK_Horse_1864_Skeleton')
    if skeleton and not isinstance(skeleton, unreal.Skeleton):
        raise RuntimeError('SK_Horse_1864_Skeleton skal være et Skeleton-asset')
    if not mesh:
        export_mesh_if_needed()
        mesh = import_fbx(MESH_FBX, 'SK_Horse_1864', True, skeleton)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError('SK_Horse_1864 skal være et SkeletalMesh-asset')
    imported_skeleton = mesh.get_editor_property('skeleton')
    if not skeleton:
        if not imported_skeleton or not unreal.EditorAssetLibrary.rename_asset(imported_skeleton.get_path_name(), DEST + '/SK_Horse_1864_Skeleton'):
            raise RuntimeError('Kunne ikke navngive hesteskelettet')
        skeleton = unreal.load_asset(DEST + '/SK_Horse_1864_Skeleton')
    if imported_skeleton != skeleton:
        raise RuntimeError('Mesh og klip skal dele SK_Horse_1864_Skeleton')
    material = unreal.load_asset('/Game/Units/Items/M_Horse')
    if material:
        unreal.MaterialEditingLibrary.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
        slots = list(mesh.get_editor_property('materials'))
        for slot in slots:
            slot.set_editor_property('material_interface', material)
        mesh.set_editor_property('materials', slots)
    for filename in animation_files:
        name = os.path.splitext(os.path.basename(filename))[0]
        clip = unreal.load_asset(DEST + '/' + name) or import_fbx(filename, name, False, skeleton)
        if not isinstance(clip, unreal.AnimSequence) or clip.get_editor_property('skeleton') != skeleton:
            raise RuntimeError('Forkert hesteskelet: ' + name)
        unreal.log('PROJECT1864-TRAIN HORSE-IMPORT PASS ' + name)
    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    unreal.log('PROJECT1864-TRAIN HORSE-IMPORT DONE; statisk fallback er uændret')


if __name__ == '__main__':
    main()
