"""Generates the battlefield's textures (PROJECT 1864): tiling ground (grass, dirt), macro variation, bark, fence
wood, and the alpha cards for the trees (spruce sprig, pine tuft, leaf cluster) and the grass blades.
Run with a Python that has numpy and Pillow: python Tools/Battle/make_battle_textures.py"""
import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'SourceAssets', 'Battle1864', 'Textures')
os.makedirs(OUT, exist_ok=True)


def tile_noise(size, cells, octaves=5, seed=0):
    """Tileable value noise in [0,1]."""
    r = np.random.default_rng(seed)
    total = np.zeros((size, size))
    amp, norm = 1.0, 0.0
    for o in range(octaves):
        c = cells * (2 ** o)
        lat = r.random((c, c))
        x = np.arange(size) * c / size
        i0 = np.floor(x).astype(int) % c
        i1 = (i0 + 1) % c
        t = x - np.floor(x)
        t = t * t * (3 - 2 * t)
        a = lat[i0][:, i0] * (1 - t)[None, :] + lat[i0][:, i1] * t[None, :]
        b = lat[i1][:, i0] * (1 - t)[None, :] + lat[i1][:, i1] * t[None, :]
        total += amp * (a * (1 - t)[:, None] + b * t[:, None])
        norm += amp
        amp *= 0.5
    return total / norm


def save(arr, name, mode='RGB'):
    arr = np.clip(arr, 0, 255).astype(np.uint8)
    Image.fromarray(arr, mode).save(os.path.join(OUT, name + '.png'))
    print('wrote', name)


def normal_from_height(h, strength):
    dx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * strength
    dy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * strength
    n = np.dstack([-dx, -dy, np.ones_like(h)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return (n * 0.5 + 0.5) * 255


def strokes_tiled(size, count, length, width, palette, seed, base):
    """Blade strokes drawn with wrap-around (tileable), with a height map (lighter blades stand higher)."""
    r = np.random.default_rng(seed)
    img = Image.new('RGB', (size, size), base)
    hmap = Image.new('L', (size, size), 60)
    d = ImageDraw.Draw(img)
    dh = ImageDraw.Draw(hmap)
    for _ in range(count):
        x, y = r.random() * size, r.random() * size
        a = r.random() * math.pi * 2
        l = length * (0.5 + r.random())
        col = palette[r.integers(len(palette))]
        k = 0.75 + 0.5 * r.random()
        col = tuple(int(min(255, c * k)) for c in col)
        hv = int(80 + 175 * r.random())
        x2, y2 = x + math.cos(a) * l, y + math.sin(a) * l
        for ox in (-size, 0, size):
            for oy in (-size, 0, size):
                if -l - 2 < x + ox < size + l + 2 and -l - 2 < y + oy < size + l + 2:
                    d.line([(x + ox, y + oy), (x2 + ox, y2 + oy)], fill=col, width=width)
                    dh.line([(x + ox, y + oy), (x2 + ox, y2 + oy)], fill=hv, width=width)
    return np.asarray(img).astype(float), np.asarray(hmap).astype(float) / 255.0


# ---- ground grass (1 tile = 3 m): dense short blades over dark soil
S = 1024
grass, gh = strokes_tiled(S, 60000, 14, 2, [(86, 112, 52), (104, 128, 60), (70, 96, 44), (122, 132, 70), (96, 110, 58), (132, 128, 76)], 11, (54, 66, 36))
var = tile_noise(S, 6, 4, 3)[:, :, None]
grass = grass * (0.85 + 0.3 * var)
save(grass, 'T_Ground_Grass_D')
save(normal_from_height(gh * 0.6 + tile_noise(S, 16, 3, 5) * 0.4, 3.0), 'T_Ground_Grass_N')

# ---- dirt (tracks, trampled ground)
dn = tile_noise(S, 24, 5, 21)
pebbles = tile_noise(S, 160, 1, 22)
dirt = np.dstack([118 + 50 * dn, 96 + 40 * dn, 66 + 30 * dn]) * (0.9 + 0.2 * (pebbles > 0.8)[:, :, None])
save(dirt, 'T_Ground_Dirt_D')
save(normal_from_height(dn * 0.7 + pebbles * 0.3, 4.0), 'T_Ground_Dirt_N')

# ---- macro variation (1 tile = 400 m): R = lushness, G = dryness, B = darkness, smooth blobs
macro = np.dstack([tile_noise(512, 4, 4, 31), tile_noise(512, 5, 4, 32), tile_noise(512, 7, 3, 33)]) * 255
save(macro, 'T_Ground_Macro')

# ---- bark (vertical furrows) and fence wood (grain along U)
bx = tile_noise(512, 8, 4, 41)
furrows = np.abs(np.sin((np.arange(512)[None, :] / 512.0 * 2 * math.pi * 9) + bx * 6))
bark_h = furrows * 0.7 + tile_noise(512, 32, 3, 42) * 0.3
bark = np.dstack([62 + 50 * bark_h, 48 + 38 * bark_h, 36 + 28 * bark_h])
save(bark, 'T_Bark_D')
save(normal_from_height(bark_h, 5.0), 'T_Bark_N')
grain = tile_noise(512, 4, 5, 51)
wood_h = np.abs(np.sin(np.arange(512)[:, None] / 512.0 * 2 * math.pi * 14 + grain * 8)) * 0.6 + tile_noise(512, 40, 2, 52) * 0.4
wood = np.dstack([120 + 50 * wood_h, 104 + 40 * wood_h, 84 + 30 * wood_h]) * 0.85
save(np.broadcast_to(wood, (512, 512, 3)), 'T_FenceWood_D')
save(normal_from_height(np.broadcast_to(wood_h, (512, 512)).copy(), 3.0), 'T_FenceWood_N')


# ---- foliage cards (RGBA, alpha-tested)
def card(size, draw_fn, name, seed):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw_fn(ImageDraw.Draw(img), np.random.default_rng(seed), size)
    # Bleed the colour into the transparent texels (no dark fringes when mipmapped).
    arr = np.asarray(img).astype(float)
    rgb, a = arr[:, :, :3], arr[:, :, 3:] / 255.0
    blur = np.asarray(Image.fromarray(np.clip(rgb * a, 0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(8))).astype(float)
    wa = np.asarray(Image.fromarray((a[:, :, 0] * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(8))).astype(float)[:, :, None] / 255.0
    fill = np.clip(blur / np.maximum(wa, 1e-3), 0, 255)
    rgb = np.where(a > 0.5, rgb, fill)
    save(np.dstack([rgb, arr[:, :, 3]]), name, 'RGBA')


def spruce(d, r, S2):
    # Drooping spruce sprigs: twigs from the left (the trunk) to the right, needles all along, side twigs.
    def twig(x0, y0, ang, length, width, depth):
        x, y = x0, y0
        steps = int(length / 6)
        for s in range(steps):
            a = ang + 0.3 * (s / steps)
            nx, ny = x + math.cos(a) * 6, y + math.sin(a) * 6
            d.line([(x, y), (nx, ny)], fill=(70, 52, 36, 255), width=max(1, int(width * (1 - s / steps)) + 1))
            for side in (-1, 1):
                for k in range(2):
                    na = a + side * (0.9 + 0.4 * r.random())
                    nl = (14 + 10 * r.random()) * (1 - 0.5 * s / steps)
                    g = 0.7 + 0.5 * r.random()
                    d.line([(x, y), (x + math.cos(na) * nl, y + math.sin(na) * nl)], fill=(int(34 * g), int(62 * g), int(38 * g), 255), width=3)
            if depth > 0 and s % 7 == 3:
                for side in (-1, 1):
                    twig(x, y, a + side * 0.7, length * 0.35, width * 0.5, depth - 1)
            x, y = nx, ny
    twig(8, S2 * 0.30, 0.10, S2 * 0.95, 5, 1)
    twig(8, S2 * 0.62, 0.02, S2 * 0.85, 4, 1)


def pine(d, r, S2):
    # Scots pine: a dense mass of needle tufts on a few twigs (the card is mostly filled).
    for t in range(8):
        cx, cy = 60 + r.random() * (S2 - 120), 50 + r.random() * (S2 * 0.6)
        d.line([(S2 * 0.5, S2 * 0.98), (cx, cy)], fill=(110, 70, 44, 255), width=5)
    for t in range(46):
        cx, cy = r.normal(S2 * 0.5, S2 * 0.17), r.normal(S2 * 0.42, S2 * 0.16)
        if not (40 < cx < S2 - 40 and 40 < cy < S2 - 40):
            continue
        for k in range(80):
            a = r.random() * 2 * math.pi
            l = 14 + 24 * r.random()
            g = 0.7 + 0.55 * r.random()
            d.line([(cx, cy), (cx + math.cos(a) * l, cy + math.sin(a) * l)], fill=(int(46 * g), int(72 * g), int(46 * g), 255), width=3)


def leaves(d, r, S2):
    # A broadleaf cluster: oval leaves on twigs.
    d.line([(S2 * 0.5, S2), (S2 * 0.5, S2 * 0.35)], fill=(84, 64, 44, 255), width=6)
    for k in range(260):
        cx, cy = r.normal(S2 * 0.5, S2 * 0.19), r.normal(S2 * 0.45, S2 * 0.19)
        if not (24 < cx < S2 - 24 and 24 < cy < S2 - 24):
            continue
        a = r.random() * math.pi
        l, w = 24 + 10 * r.random(), 11 + 5 * r.random()
        g = 0.65 + 0.6 * r.random()
        pts = [(cx + math.cos(a) * l * math.cos(t) - math.sin(a) * w * math.sin(t), cy + math.sin(a) * l * math.cos(t) + math.cos(a) * w * math.sin(t)) for t in np.linspace(0, 2 * math.pi, 14)]
        d.polygon(pts, fill=(int(62 * g), int(94 * g), int(42 * g), 255))


def blades(d, r, S2):
    # Grass card: tall blades from the bottom edge, a few seed heads.
    for k in range(150):
        x = 10 + r.random() * (S2 - 20)
        h = S2 * (0.4 + 0.58 * r.random())
        bend = (r.random() - 0.5) * 110
        g = 0.65 + 0.6 * r.random()
        dry = r.random() < 0.22
        col = (int((122 if dry else 80) * g), int((124 if dry else 112) * g), int((70 if dry else 48) * g), 255)
        pts = [(x + bend * (s / 8) ** 2, S2 - h * (s / 8)) for s in range(9)]
        for s in range(8):
            d.line([pts[s], pts[s + 1]], fill=col, width=max(1, int(5 * (1 - s / 8)) + 1))
        if r.random() < 0.08:
            tip = pts[-1]
            d.ellipse([tip[0] - 4, tip[1] - 14, tip[0] + 4, tip[1] + 2], fill=(150, 140, 96, 255))


card(512, spruce, 'T_Card_Spruce', 1)
card(512, pine, 'T_Card_Pine', 2)
card(512, leaves, 'T_Card_Leaves', 3)
card(512, blades, 'T_Card_Grass', 4)
