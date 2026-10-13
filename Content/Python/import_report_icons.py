"""Importerer rapportens ikoner (Reference/UI/ReportIcons/Icon_*.png) som UI-teksturer i /Game/UI/ReportIcons.

UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Content/Python/import_report_icons.py -unattended
"""
from pathlib import Path
import unreal

SRC = Path(__file__).resolve().parents[2] / 'Reference' / 'UI' / 'ReportIcons'
DEST = '/Game/UI/ReportIcons'


def run():
    unreal.EditorAssetLibrary.make_directory(DEST)
    tasks = []
    for png in sorted(SRC.glob('Icon_*.png')):
        task = unreal.AssetImportTask()
        task.filename = str(png)
        task.destination_path = DEST
        task.destination_name = 'T_' + png.stem
        task.replace_existing = True
        task.automated = True
        task.save = True
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        tex = unreal.EditorAssetLibrary.load_asset(DEST + '/' + str(task.destination_name))
        if tex:
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            tex.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
            unreal.log('PROJECT1864-REPORT: ikon %s' % task.destination_name)


run()
