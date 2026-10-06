"""Uniform plates for the unit card (PROJECT 1864): the A-pose front references of each Danish arm (the uniform
concept art in Reference/Units/MultiView), cropped round the soldier, 320 x 480, written to
Reference/Campaign1851/Uniforms/T_Uniform_<Arm>.png (the arm names of ECampaign1851Arm).
Run: python Tools/Campaign/make_uniform_cards.py, then the import (Tools/Campaign/import_building_cards.py imports
this folder too)."""
import os
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
SRC = r"R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView"
OUT = os.path.join(ROOT, 'Reference', 'Campaign1851', 'Uniforms')
ARMS = {
    'Infantry': r'Danish Infantry\DK_Infanteri_1864_Apose_Front.png',
    'Guard': r'Danish Infantry\DK_Livgarden_1864_Apose_Front.png',
    'Jager': r'Danish Infantry\DK_Infanteri_Regiment6_1864_Apose_Front.png',
    'Cavalry': r'Danish Cavalry\DK_Dragon_1864_Apose_Front.png',
    'Hussar': r'Danish Cavalry\DK_Garderhusar_1864_Apose_Front.png',
    'Artillery': r'Danish Gunbattery\DK_Infanteri_Morkeblaa_1864_Apose_Front.png',
    'HorseArtillery': r'Danish Gunbattery\DK_Infanteri_Morkeblaa_1864_Apose_Front.png',
}
os.makedirs(OUT, exist_ok=True)
for arm, rel in ARMS.items():
    im = Image.open(os.path.join(SRC, rel)).convert('RGB')
    w, h = im.size
    plate = im.crop((int(w * 0.19), int(h * 0.04), int(w * 0.81), int(h * 0.97))).resize((320, 480), Image.LANCZOS)
    plate.save(os.path.join(OUT, 'T_Uniform_%s.png' % arm))
    print('uniform', arm)
