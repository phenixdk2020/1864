import os, sys, json
from pathlib import Path
RUN = Path(__file__).resolve().parent
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
os.environ['GRADIO_ANALYTICS_ENABLED'] = 'False'
os.environ['TEMP'] = str(RUN / 'temp'); os.environ['TMP'] = str(RUN / 'temp')
from gradio_client import Client, handle_file
nat = sys.argv[1]
resolution = int(sys.argv[2]) if len(sys.argv) > 2 else 256
M = Path(r'R:/Onedrive/Dokumenter/Unreal Projects/Game1864/Reference/Units/MultiView')
srcs = {'DK': (M/'Danish Gunbattery', 'DK_Infanteri_Morkeblaa_1864_Apose_'),
        'NO': (M/'Norwegian Gunbattery', 'NO_Kanonmand_1864_Apose_')}
d, pre = srcs[nat]
out = RUN / (nat + '_attempt_' + str(resolution) + '_v1')
out.mkdir(exist_ok=False)
client = Client('http://127.0.0.1:42003', download_files=str(out), verbose=False)
result = client.predict(caption=None, image=None,
    mv_image_front=handle_file(str(d/(pre+'Front.png'))),
    mv_image_left=handle_file(str(d/(pre+'Side.png'))),
    mv_image_back=handle_file(str(d/(pre+'Back.png'))),
    mv_image_right=None, steps=30, guidance_scale=5.0, seed=1234,
    octree_resolution=resolution, check_box_rembg=True,
    num_chunks=8000, randomize_seed=False, api_name='/generation_all')
(out/'result_v1.json').write_text(json.dumps(result, ensure_ascii=False, indent=2, default=str), encoding='utf-8')
print(json.dumps(result, ensure_ascii=False, default=str), flush=True)
