import os
import sys
import json
import shutil
from pathlib import Path

RUN = Path(__file__).resolve().parent
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
os.environ['GRADIO_ANALYTICS_ENABLED'] = 'False'
os.environ['TEMP'] = str(RUN / 'temp')
os.environ['TMP'] = str(RUN / 'temp')
from gradio_client import Client, handle_file

nat = sys.argv[1]
resolution = int(sys.argv[2]) if len(sys.argv) > 2 else 256
root = RUN.parents[3]
sources = root / 'Reference' / 'Units' / 'MultiView' / (nat + ' Officer')
out = RUN / (nat + '_attempt_' + str(resolution) + '_v1')
out.mkdir(exist_ok=False)
client = Client('http://127.0.0.1:42003', download_files=str(out), verbose=False)
try:
    result = client.predict(
        caption=None, image=None,
        mv_image_front=handle_file(str(sources / (nat + '_Officer_1864_Front.png'))),
        mv_image_left=handle_file(str(sources / (nat + '_Officer_1864_Side.png'))),
        mv_image_back=handle_file(str(sources / (nat + '_Officer_1864_Back.png'))),
        mv_image_right=None, steps=30, guidance_scale=5.0, seed=1234,
        octree_resolution=resolution, check_box_rembg=True,
        num_chunks=8000, randomize_seed=False, api_name='/generation_all')
    with (out / 'result_v1.json').open('x', encoding='utf-8') as f:
        json.dump(result, f, ensure_ascii=False, indent=2, default=str)
    print(json.dumps(result, ensure_ascii=False, default=str), flush=True)
except Exception as exc:
    with (out / 'error_v1.txt').open('x', encoding='utf-8') as f:
        f.write(repr(exc))
    raise
