"""The ripe wheat's alpha card (PROJECT 1864): golden stalks with ears and awns, bleeding colour into the transparent
texels. Written to SourceAssets/Battle1864/Textures/T_Card_Wheat.png (the ground textures are not touched).
Run: python Tools/Battle/make_wheat_card.py"""
import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'SourceAssets', 'Battle1864', 'Textures')
S = 512
r = np.random.default_rng(1851)
img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
# Stalks from the bottom edge: straw-coloured, a slight bend, an ear at the top with its awns.
for k in range(130):
    x = 6 + r.random() * (S - 12)
    h = S * (0.80 + 0.18 * r.random())
    bend = (r.random() - 0.5) * 36
    g = 0.85 + 0.45 * r.random()
    straw = (int(196 * g), int(160 * g), int(84 * g), 255)
    pts = [(x + bend * (s / 10) ** 2, S - h * (s / 10)) for s in range(11)]
    for s in range(10):
        d.line([pts[s], pts[s + 1]], fill=straw, width=2)
    tx, ty = pts[-1]
    # The ear: grains in two rows, a little heavier than the stalk, bowed over.
    gold = (int(226 * g), int(184 * g), int(92 * g), 255)
    for j in range(11):
        gy = ty + j * 5.5
        gx = tx + math.sin(j * 0.5) * 2 + 0.7 * j * (1 if bend > 0 else -1)
        d.ellipse([gx - 5, gy - 3.5, gx + 5, gy + 3.5], fill=gold)
        d.ellipse([gx - 2 + (3 if j % 2 else -3), gy - 4, gx + 2 + (3 if j % 2 else -3), gy + 2], fill=(int(236 * g), int(200 * g), int(110 * g), 255))
    for a in (-0.35, -0.12, 0.12, 0.35):
        d.line([(tx, ty + 6), (tx + math.sin(a) * 22, ty - 20 * math.cos(a))], fill=(int(214 * g), int(176 * g), int(96 * g), 255), width=1)
# A few dry leaves low on the stalks.
for k in range(40):
    x = 10 + r.random() * (S - 20)
    h = S * (0.2 + 0.3 * r.random())
    bend = (r.random() - 0.5) * 120
    g = 0.7 + 0.4 * r.random()
    pts = [(x + bend * (s / 6) ** 2, S - h * (s / 6)) for s in range(7)]
    for s in range(6):
        d.line([pts[s], pts[s + 1]], fill=(int(150 * g), int(140 * g), int(70 * g), 255), width=max(1, 5 - s))
arr = np.asarray(img).astype(float)
rgb, a = arr[:, :, :3], arr[:, :, 3:] / 255.0
blur = np.asarray(Image.fromarray(np.clip(rgb * a, 0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(8))).astype(float)
wa = np.asarray(Image.fromarray((a[:, :, 0] * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(8))).astype(float)[:, :, None] / 255.0
rgb = np.where(a > 0.5, rgb, np.clip(blur / np.maximum(wa, 1e-3), 0, 255))
Image.fromarray(np.dstack([rgb, arr[:, :, 3]]).astype(np.uint8), 'RGBA').save(os.path.join(OUT, 'T_Card_Wheat.png'))
print('T_Card_Wheat')
