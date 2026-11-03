"""Joins the 6 preview frames of each rider clip into one sheet: python -I compose_cavalry_rider_sheets.py"""
from pathlib import Path
from PIL import Image
base = Path(__file__).resolve().parents[2] / 'SourceAssets/InfantryCombat/Preview'
for d in sorted(base.glob('Cav*')):
    if not d.is_dir():
        continue
    files = sorted(d.glob('[0-9][0-9].png'))
    if not files:
        continue
    imgs = [Image.open(f).convert('RGB') for f in files]
    w, h = imgs[0].size
    sheet = Image.new('RGB', (w * 3, h * 2), (255, 255, 255))
    for i, im in enumerate(imgs):
        sheet.paste(im, ((i % 3) * w, (i // 3) * h))
    sheet.save(base / (d.name + '_sheet.png'))
    print(d.name, len(files))
