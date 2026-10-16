"""Uniform plates for the unit card (PROJECT 1864): the front references (standing straight, not A-pose) of each Danish arm (the uniform
concept art in Reference/Units/MultiView), centre-cropped to 2:3, 400 x 600, written to
Reference/Campaign1851/Uniforms/T_Uniform_<Arm>.png (the arm names of ECampaign1851Arm).
Run: python Tools/Campaign/make_uniform_cards.py, then the import (Tools/Campaign/import_building_cards.py imports
this folder too)."""
import os
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
SRC = r"R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView"
OUT = os.path.join(ROOT, 'Reference', 'Campaign1851', 'Uniforms')
ARMS = {
    'Infantry': r'Danish Infantry\DK_Infanteri_1864_pose_Front.png',
    'Guard': r'Danish Infantry\DK_Livgarden_1864_pose_Front.png',
    'Jager': r'Danish Infantry\DK_Infanteri_Regiment6_1864_pose_Front.png',
    'Cavalry': r'Danish Cavalry\DK_Dragon_1864_pose_Front.png',
    'Hussar': r'Danish Cavalry\DK_Garderhusar_1864_pose_Front.png',
    'Artillery': r'Danish Gunbattery\DK_Infanteri_Morkeblaa_1864_pose_Front.png',
    'HorseArtillery': r'Danish Gunbattery\DK_Infanteri_Morkeblaa_1864_pose_Front.png',
}
# Campaign nation ids (Nations_1825.json / Nations1851.json).
NATION_ARMS = {
    'SE': {
        'Infantry': r'Swedish Infantry\SE_Infanteri_Type1_1864_pose_Front.png',
        'Guard': r'Swedish Infantry\SE_Eliteinfanteri_1864_pose_Front.png',
        'Jager': r'Swedish Infantry\SE_Jaeger_1864_pose_Front.png',
        'Cavalry': r'Swedish Cavalry\SE_Dragon_1864_pose_Front.png',
        'Hussar': r'Swedish Cavalry\SE_Husar_1864_pose_Front.png',
        'Artillery': r'Swedish Gunbattery\SE_Artilleri_1864_pose_Front.png',
        'HorseArtillery': r'Swedish Gunbattery\SE_Artilleri_1864_pose_Front.png',
    },
    'PR': {
        'Infantry': r'Prussisk Infantry\PR_Infanteri_1864_pose_Front.png',
        'Guard': r'Prussisk Infantry\PR_Garde_1864_pose_Front.png',
        'Jager': r'Prussisk Infantry\PR_Jaeger_1864_pose_Front.png',
        'Cavalry': r'Prussisk Cavalry\PR_Dragon_1864_pose_Front.png',
        'Hussar': r'Prussisk Cavalry\PR_Husar_1864_pose_Front.png',
        'Artillery': r'Prussisk Gunbattery\PR_Artilleri_1864_pose_Front.png',
        'HorseArtillery': r'Prussisk Gunbattery\PR_Artilleri_1864_pose_Front.png',
    },
    'NO': {
        'Infantry': r'Norwegian Infantry\NO_Infanteri_1864_pose_Front.png',
        'Guard': r'Norwegian Infantry\NO_Garde_1864_pose_Front.png',
        'Jager': r'Norwegian Infantry\NO_Jaeger_1864_pose_Front.png',
        'Cavalry': r'Norwegian Cavalry\NO_Dragon_1864_pose_Front.png',
        'Artillery': r'Norwegian Gunbattery\NO_Artilleri_1864_pose_Front.png',
        'HorseArtillery': r'Norwegian Gunbattery\NO_Artilleri_1864_pose_Front.png',
    },  # Norway has no hussar.
}


def main():
    os.makedirs(OUT, exist_ok=True)
    written, skipped = [], []
    for nation, arms in [('', ARMS), *NATION_ARMS.items()]:
        for arm, rel in arms.items():
            name = 'T_Uniform_%s%s.png' % (nation + '_' if nation else '', arm)
            source = os.path.join(SRC, rel)
            if not os.path.isfile(source):
                skipped.append(name)
                print('Skipped %s: missing %s' % (name, source))
                continue
            with Image.open(source) as original:
                im = original.convert('RGB')
            w, h = im.size
            # Preserve the legacy Danish resize exactly. New plates never stretch.
            if nation and w * 3 != h * 2:
                if w * 3 > h * 2:
                    crop_w = h * 2 / 3
                    left = (w - crop_w) / 2
                    im = im.crop((left, 0, left + crop_w, h))
                else:
                    crop_h = w * 3 / 2
                    top = (h - crop_h) / 2
                    im = im.crop((0, top, w, top + crop_h))
            plate = im.resize((400, 600), Image.LANCZOS)
            plate.save(os.path.join(OUT, name))
            written.append(name)
            print('Written', name)
    print('Summary: %d written, %d skipped' % (len(written), len(skipped)))
    print('Written: ' + ', '.join(written))
    print('Skipped: ' + ', '.join(skipped))


if __name__ == '__main__':
    main()
