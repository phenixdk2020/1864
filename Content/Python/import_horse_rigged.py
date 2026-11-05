# Manuel import: UnrealEditor-Cmd Game1864.uproject -run=pythonscript -script=<denne fil>
# StrategyCavalryVisualComponent bruger SK_Horse_1864 + A_Horse_* naar de findes (ellers den statiske hest SM_Horse_Static).
# Mesh-FBX ligger i SourceAssets/Horse (Tools/Animation/export_horse_mesh_b1.py); Blender startes kun hvis den mangler.
# Kør kun naar hverken UnrealEditor, Build.bat eller spillet koerer.
#
# 2026-11-05 (hest laa ned i alle klip): aarsag var import_rotation roll=90 paa hesteklippene. Hestens mesh blev importeret uden rotation
# (ref-pose Z op: pelvis z=105,8, head z=148,8, hove z=4,1), men klippene blev roteret 90 grader om X, saa hele skelettet blev evalueret med hoejden
# i Y (pelvis (59,8, 105,8, 0)) og hesten laa paa siden/med maven paa jorden i alle gangarter. Hesteklippene er eksporteret med samme akser som meshet
# (Blender axis_forward -Z, axis_up Y; Tools/Animation/horse_gaits_blender.py), saa de skal importeres med roll=0 (HORSE_CLIP_ROLL). Menneskeklippene
# beholder roll=90 (deres mesh/armatur er ikke ens med hestens); roll=90 er altsaa ikke en generel regel for alle Blender-klip, men skal maales mod meshets ref-pose.
# Gamle klip kopieres foer sletning til /Game/Units/Horse/Backup_20261105 (kun hvis mappen ikke findes), og klippene slettes og importeres paany
# (replace_existing=False). Skelet og mesh er korrekte og beholdes. Efter importen kontrolleres at pelvis/saddle_root i klip frame 0 ligger i Z-hoejde
# med ref-posen (maal med AnimPoseExtensions, ikke med oejet).
import os
import shutil
import subprocess
import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DEST = '/Game/Units/Horse'
BLEND = os.environ.get('PROJECT1864_HORSE_BLEND',
    'R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView/Items/Horse_1_rigged_v3.blend')
MESH_FBX = os.environ.get('PROJECT1864_HORSE_FBX', os.path.join(ROOT, 'SourceAssets', 'Horse', 'Horse_1_rigged_v3.fbx'))
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
HORSE_CLIP_ROLL = float(os.environ.get('PROJECT1864_HORSE_CLIP_ROLL', '0'))
BACKUP_DIR = DEST + '/Backup_20261105'
CLIP_NAMES = ['A_Horse_' + gait for gait in ('Stand', 'Walk', 'Trot', 'Canter', 'Gallop', 'Rear', 'Fall')]


def export_mesh_if_needed():
    if os.path.isfile(MESH_FBX):
        return
    blender = os.environ.get('PROJECT1864_BLENDER') or shutil.which('blender')
    if not blender:
        raise RuntimeError('Angiv PROJECT1864_BLENDER eller en eksporteret PROJECT1864_HORSE_FBX (SourceAssets/Horse/Horse_1_rigged_v3.fbx)')
    if not os.path.isfile(BLEND):
        raise FileNotFoundError(BLEND)
    # Tools/Animation/export_horse_mesh_b1.py writes SourceAssets/Horse/Horse_1_rigged_v3.fbx (one armature + its mesh; the blend holds a duplicate pair).
    subprocess.run([blender, '--background', BLEND, '--python-exit-code', '1', '--python',
                    os.path.join(ROOT, 'Tools', 'Animation', 'export_horse_mesh_b1.py')], check=True)
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
    if not skeletal_mesh:
        options.anim_sequence_import_data.set_editor_property('import_rotation', unreal.Rotator(roll=HORSE_CLIP_ROLL, pitch=0, yaw=0))
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


def backup_and_delete_clips():
    # Sikkerhedskopi af de nuvaerende klip (kun foerste gang), derefter slettes de, saa de importeres frisk med den rette rotation.
    if not unreal.EditorAssetLibrary.does_directory_exist(BACKUP_DIR):
        unreal.EditorAssetLibrary.make_directory(BACKUP_DIR)
        for name in CLIP_NAMES:
            if unreal.EditorAssetLibrary.does_asset_exist(DEST + '/' + name):
                if not unreal.EditorAssetLibrary.duplicate_asset(DEST + '/' + name, BACKUP_DIR + '/' + name):
                    raise RuntimeError('Backup fejlede: ' + name)
    for name in CLIP_NAMES:
        if unreal.EditorAssetLibrary.does_asset_exist(DEST + '/' + name):
            if not unreal.EditorAssetLibrary.does_asset_exist(BACKUP_DIR + '/' + name):
                raise RuntimeError('Ingen backup, sletter ikke: ' + name)
            unreal.EditorAssetLibrary.delete_asset(DEST + '/' + name)


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
        if not imported_skeleton or (imported_skeleton.get_path_name().split('.')[0] != DEST + '/SK_Horse_1864_Skeleton' and not unreal.EditorAssetLibrary.rename_asset(imported_skeleton.get_path_name(), DEST + '/SK_Horse_1864_Skeleton')):
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
    if os.environ.get('PROJECT1864_HORSE_KEEP_CLIPS') != '1':
        backup_and_delete_clips()
    for filename in animation_files:
        name = os.path.splitext(os.path.basename(filename))[0]
        clip = unreal.load_asset(DEST + '/' + name) or import_fbx(filename, name, False, skeleton)
        if not isinstance(clip, unreal.AnimSequence) or clip.get_editor_property('skeleton') != skeleton:
            raise RuntimeError('Forkert hesteskelet: ' + name)
        unreal.log('PROJECT1864-TRAIN HORSE-IMPORT PASS ' + name)
    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    unreal.log('PROJECT1864-TRAIN HORSE-IMPORT DONE; statisk fallback SM_Horse_Static er uændret')


if __name__ == '__main__':
    main()
