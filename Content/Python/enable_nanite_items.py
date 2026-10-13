"""Slaar Nanite til paa de tunge statiske modeller (heste, kanoner, morterer, sadler, sabler, vogne), saa GPU-prisen styres af pixels og ikke af
antallet af trekanter (de er ikke bagte og har ingen detaljeniveauer; en hest er 470.000 trekanter).

UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=Content/Python/enable_nanite_items.py -unattended
"""
import unreal

NAMES = ['SM_Horse_Static', 'SM_Cannon_1864', 'SM_Mortar_1864', 'SM_Saddle', 'SM_Saber', 'SM_Scabbard', 'SM_Wagon_Supply', 'SM_Wagon_Empty', 'SM_Wagon_Mortar']


def run():
    for name in NAMES:
        path = '/Game/Units/Items/' + name
        mesh = unreal.EditorAssetLibrary.load_asset(path)
        if mesh is None:
            unreal.log_error('PROJECT1864-NANITE: %s findes ikke' % path)
            continue
        try:
            mesh.modify()
            settings = unreal.MeshNaniteSettings()
            settings.enabled = True
            settings.position_precision = -2147483648   # automatic
            mesh.set_editor_property('nanite_settings', settings)
            saved = unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
            again = unreal.EditorAssetLibrary.load_asset(path).get_editor_property('nanite_settings').enabled
            unreal.log_error('PROJECT1864-NANITE: %s gemt=%s nanite nu=%s' % (name, saved, again))
        except Exception as error:
            unreal.log_error('PROJECT1864-NANITE: %s fejl: %s' % (name, error))


run()
