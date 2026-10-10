"""Concept pictures of the supply and ammunition wagons through the local ComfyUI (SDXL, Juggernaut XL).

    python Tools/Animation/generate_wagon_images.py
Output: <ComfyUI>/output/wagon_*.png, copied (white made transparent) to Reference/Campaign1851/Wagons/.
"""
import json
import os
import time
import urllib.request

from PIL import Image

SERVER = "http://127.0.0.1:8188"
CKPT = "juggernautXL_v8Rundiffusion.safetensors"
STYLE = ("painted game illustration, three-quarter isometric view from the front left and slightly above, "
         "red brick and slate roof colour palette, muted warm colours, detailed, soft shading, small patch of cobblestone ground, "
         "single object isolated on a plain white background, no text, no people")
NEG = "photo, text, watermark, people, soldiers, multiple wagons, cropped, blurry, modern, low quality, deformed wheels"
JOBS = [
    ("wagon_ammunition_a", "a Danish 1850s military ammunition wagon (caisson), dark green painted wooden box with iron fittings, two large spoked wheels and a limber, drawn by two brown horses in harness, " + STYLE, 101),
    ("wagon_ammunition_b", "a Danish 1850s four-wheeled ammunition wagon with a closed wooden powder box painted dark blue-green, spare wheel on the side, two horses in harness, " + STYLE, 102),
    ("wagon_supply_a", "a Danish 1850s army supply wagon, a four-wheeled farm-style wagon with a canvas cover stretched over wooden hoops, sacks and barrels visible at the rear, two draught horses in harness, " + STYLE, 103),
    ("wagon_supply_b", "a Danish 1850s open army provision wagon loaded with grain sacks and barrels, wooden sides painted red-brown, four wheels, two horses in harness, " + STYLE, 104),
    ("wagon_field_forge", "a Danish 1850s army field forge wagon with a small forge and bellows, tools hanging at the side, two horses in harness, " + STYLE, 105),
    ("wagon_ambulance", "a Danish 1850s army ambulance wagon, covered four-wheeled wagon with a red cross-free white canvas cover, stretcher poles visible at the rear, two horses in harness, " + STYLE, 106),
]


def api(path, data=None):
    req = urllib.request.Request(SERVER + path, data=json.dumps(data).encode() if data is not None else None,
                                 headers={"Content-Type": "application/json"})
    return json.loads(urllib.request.urlopen(req, timeout=1800).read())


def graph(prefix, prompt, seed):
    return {
        "1": {"class_type": "CheckpointLoaderSimple", "inputs": {"ckpt_name": CKPT}},
        "2": {"class_type": "CLIPTextEncode", "inputs": {"text": prompt, "clip": ["1", 1]}},
        "3": {"class_type": "CLIPTextEncode", "inputs": {"text": NEG, "clip": ["1", 1]}},
        "4": {"class_type": "EmptyLatentImage", "inputs": {"width": 1024, "height": 1024, "batch_size": 1}},
        "5": {"class_type": "KSampler", "inputs": {"model": ["1", 0], "positive": ["2", 0], "negative": ["3", 0], "latent_image": ["4", 0],
                                                  "seed": seed, "steps": 30, "cfg": 6.0, "sampler_name": "dpmpp_2m", "scheduler": "karras", "denoise": 1.0}},
        "6": {"class_type": "VAEDecode", "inputs": {"samples": ["5", 0], "vae": ["1", 2]}},
        "7": {"class_type": "SaveImage", "inputs": {"images": ["6", 0], "filename_prefix": prefix}},
    }


def white_to_alpha(src, dst):
    im = Image.open(src).convert("RGBA")
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            m = min(r, g, b)
            if m > 238:
                px[x, y] = (r, g, b, 0)
            elif m > 215:
                px[x, y] = (r, g, b, int((238 - m) / 23 * 255))
    im.save(dst)


def main():
    out_dir = os.path.join(os.path.dirname(__file__), "..", "..", "Reference", "Campaign1851", "Wagons")
    os.makedirs(out_dir, exist_ok=True)
    comfy_out = r"D:\pinokio\api\comfy.git\app\output"
    for prefix, prompt, seed in JOBS:
        pid = api("/prompt", {"prompt": graph(prefix, prompt, seed)})["prompt_id"]
        print(prefix, "queued", flush=True)
        for _ in range(400):
            time.sleep(3)
            hist = api("/history/" + pid)
            if pid in hist:
                imgs = hist[pid].get("outputs", {}).get("7", {}).get("images", [])
                for im in imgs:
                    src = os.path.join(comfy_out, im.get("subfolder", ""), im["filename"])
                    white_to_alpha(src, os.path.join(out_dir, prefix + ".png"))
                    print(prefix, "ok", flush=True)
                break


if __name__ == "__main__":
    main()
