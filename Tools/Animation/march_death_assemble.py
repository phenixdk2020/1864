"""Assembles the contact sheet and preview MP4s of build_march_death_set.py (system Python with PIL + ffmpeg on PATH, NOT Blender).

    python march_death_assemble.py <out dir with frames/ and march_death_build.json> <preview dir> <blend frames>

Writes <out dir>/sheet.png (rows: the three deaths; columns: WalkShoulder frame 0 for comparison, frame 0, end of the blend, mid-fall,
final pose) and <preview dir>/DeathMarchFront.mp4 etc. (side view, 512x352, 30 fps, last pose held 1 s).
"""
import json, shutil, subprocess, sys
from pathlib import Path
from PIL import Image, ImageDraw

out, prev, blend = Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3])
build = json.loads((out / 'march_death_build.json').read_text())['clips']
names = list(build)
cell = None
rows = []
for n in names:
    fs = sorted((out / 'frames' / n).glob('*.png'))
    n_fall = build[n]['keys'][-1][0]
    picks = [('walk f0', sorted((out / 'frames' / 'WalkShoulder').glob('*.png'))[0]), ('f0', fs[0]), ('blend end f%d' % blend, fs[blend]),
             ('mid-fall f%d' % (n_fall // 2), fs[n_fall // 2]), ('final f%d' % (len(fs) - 1), fs[-1])]
    rows.append((n, picks))
w, h = Image.open(rows[0][1][0][1]).size
sheet = Image.new('RGB', (w * 5, h * len(rows)), 'white')
for r, (n, picks) in enumerate(rows):
    for c, (lab, f) in enumerate(picks):
        im = Image.open(f).convert('RGB')
        ImageDraw.Draw(im).text((6, 6), n + ' - ' + lab, fill=(0, 0, 0))
        sheet.paste(im, (c * w, r * h))
sheet.save(out / 'sheet.png')
prev.mkdir(parents=True, exist_ok=True)
ff = shutil.which('ffmpeg')
if ff:
    for n in names:
        subprocess.run([ff, '-y', '-loglevel', 'error', '-framerate', '30', '-i', str(out / 'frames' / n / '%03d.png'), '-vf', 'tpad=stop_mode=clone:stop_duration=1',
                        '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '20', str(prev / (n.replace('DeathMarch', 'DeathMarch') + '.mp4'))], check=False)
print('ASSEMBLED', out / 'sheet.png')
