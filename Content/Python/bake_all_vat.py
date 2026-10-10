"""Bag alle fodsoldatvarianter og deres kompatible animationsklip, med gevær.

Manuel kørsel (kræver editor-modulet med StrategyCrowdModel.BakeAsset):
UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Content/Python/bake_all_vat.py -unattended -AllowCommandletRendering

Gemmer /Game/Battle/VAT/VAT_<soldat>_<gevær> samt M_CrowdVAT_All.
Scriptet starter ikke en ny editorproces. Ryttere/stab/ordonnanser bruger den ubevæbnede variant.
"""
from pathlib import Path
import runpy
import unreal


def bake_all():
    if not hasattr(unreal, 'StrategyCrowdModel'):
        raise RuntimeError('PROJECT1864-CROWD: Strategy1864-editor-modulet mangler')
    unreal.EditorAssetLibrary.make_directory('/Game/Battle/VAT')
    material_script = runpy.run_path(str(Path(__file__).with_name('make_crowd_material.py')))
    uniform_script = runpy.run_path(str(Path(__file__).with_name('prepare_campaign_uniforms.py')))
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    assets = [entry.get_asset() for entry in registry.get_assets_by_path('/Game/Units', recursive=True)]
    animations = [asset for asset in assets if isinstance(asset, unreal.AnimSequence)]
    # Discover infantry/guard/jager/officer/drummer/flag variants rather than hardcoding Denmark.
    meshes = [asset for asset in assets if isinstance(asset, unreal.SkeletalMesh)
              and (any(token in asset.get_path_name().lower()
                       for token in ('infantry', 'livgarden', 'guard', 'jager', 'officer', 'drummer', 'flag', 'rider', 'cavalry'))
                   or 'human' in str(asset.get_editor_property('skeleton')).lower())
              and 'horse' not in asset.get_name().lower()]
    rifles = [asset for asset in assets if isinstance(asset, unreal.StaticMesh)
              and any(token in asset.get_name().lower() for token in ('rifle', 'musket'))]
    if not meshes or not animations or not rifles:
        raise RuntimeError('PROJECT1864-CROWD: soldater, animationer eller geværer mangler under /Game/Units')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    failures = []
    prepared_materials = set()
    for rifle in rifles:
        for slot in rifle.get_editor_property('static_materials'):
            material = slot.get_editor_property('material_interface')
            while isinstance(material, unreal.MaterialInstance):
                material = material.get_editor_property('parent')
            if isinstance(material, unreal.Material) and material.get_path_name() not in prepared_materials:
                material_script['configure_crowd_fade'](material)
                prepared_materials.add(material.get_path_name())
    for mesh in meshes:
        uniform_script['prepare'](mesh.get_path_name())
        # Near figures need the complementary mask too, otherwise even phase-correct swaps can pop.
        for slot in mesh.get_editor_property('materials'):
            material = slot.get_editor_property('material_interface')
            while isinstance(material, unreal.MaterialInstance):
                material = material.get_editor_property('parent')
            if isinstance(material, unreal.Material) and material.get_path_name() not in prepared_materials:
                material_script['configure_crowd_fade'](material)
                prepared_materials.add(material.get_path_name())
        skeleton = mesh.get_editor_property('skeleton')
        clips = [clip for clip in animations if clip.get_editor_property('skeleton') == skeleton]
        if not clips:
            failures.append(mesh.get_path_name() + ': ingen kompatible klip')
            continue
        # Include ALL clips for the skeleton: stance transitions, deaths, reloads, crawl and idles.
        for rifle in rifles + [None]:
            label = mesh.get_name() + '/' + (rifle.get_name() if rifle else 'Unarmed')
            try:
                model = unreal.StrategyCrowdModel.bake_asset(world, mesh, rifle, clips)
                if not model or not unreal.EditorAssetLibrary.save_loaded_asset(model, only_if_is_dirty=False):
                    raise RuntimeError('bagning/gemning fejlede')
                unreal.log('PROJECT1864-CROWD: gemt %s (%d klip)' % (label, len(clips)))
            except Exception as error:
                failures.append(label + ': ' + str(error))
    if failures:
        raise RuntimeError('PROJECT1864-CROWD: ufuldstændig bagning:\n' + '\n'.join(failures))
    unreal.log('PROJECT1864-CROWD: alle fundne fodsoldatvarianter bagt')


if __name__ == '__main__':
    bake_all()
