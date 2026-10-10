"""Generates the missing infantry animations with HY-Motion through a local ComfyUI (Pinokio install).

Usage (ComfyUI must be running on 127.0.0.1:8188):
    python Tools/Animation/hymotion_generate.py [clip_name ...]      # default: all clips below

Output: <ComfyUI>/output/hymotion_fbx/<clip>_*.fbx (retargeted to the Mixamo skeleton of DK_Infantry_1864.fbx).
Then run Tools/Animation/clean_clips_blender.py to strip root motion / make loops, and import into Unreal.
"""
import json
import sys
import time
import urllib.request

SERVER = "http://127.0.0.1:8188"
NETWORK = "HY-Motion-1.0-Lite"
LLM = "Qwen3-8B-bnb-4bit"   # pre-quantised 4-bit Qwen3-8B (the GGUF de-quantises to 16 GB and does not fit an 8 GB card)
CHARACTER = "3d/DK_Infantry_1864.fbx"   # in ComfyUI/input

# name, prompt, duration seconds, seed
CLIPS = [
    ("A_Dive_To_Prone", "A soldier carrying a rifle throws himself to the ground and lies flat on his stomach.", 2.5, 11),
    ("A_Run_Crouched", "A soldier carrying a rifle runs forward bent over low, staying small.", 2.5, 12),
    ("A_Fix_Bayonet", "A soldier standing holding a musket fixes a bayonet onto the barrel with both hands.", 3.0, 13),
    ("A_Bayonet_Parry", "A soldier with a rifle and bayonet parries an incoming thrust and steps back.", 2.0, 14),
    ("A_Bayonet_Thrust_Hit", "A soldier is stabbed in the chest, doubles over and falls to his knees then onto his side.", 3.0, 15),
    ("A_Kneel_Fire_Loop", "A soldier kneeling on one knee aims a rifle, fires, and holds the aim.", 2.5, 16),
    ("A_Square_Kneel_Brace", "A soldier kneels and braces a rifle with bayonet pointing forward at an angle, steady, waiting.", 3.0, 17),
    ("A_Dig_Shovel", "A man digs with a shovel into the ground, lifts a shovelful of earth and throws it aside, repeating.", 4.0, 18),
    ("A_Carry_Timber", "A man carries a heavy log on his shoulder and walks forward slowly.", 3.0, 19),
    ("A_Wounded_Limp", "A wounded soldier limps forward holding his injured arm with the other hand.", 3.0, 20),
    ("A_Wounded_Writhe", "A wounded man lies on the ground on his back and writhes in pain, rolling slightly.", 4.0, 21),
    ("A_Surrender_Hands_Up", "A soldier drops his rifle and raises both hands above his head to surrender.", 2.5, 22),
    ("A_Prisoner_Walk", "A soldier walks forward with both hands raised to shoulder height, subdued and tired.", 3.0, 23),
    ("A_Rest_Sit", "A soldier sits on the ground resting with a rifle across his knees, looks around, breathes.", 4.0, 24),
    ("A_March_Rifle_Shoulder", "A soldier marches in step with a rifle on his right shoulder, upright, steady rhythm.", 2.0, 25),
    ("A_Stand_Up_From_Prone", "A soldier lying on his stomach gets up quickly to his feet holding a rifle.", 2.5, 26),
    ("A_Standard_Bearer_Walk", "A man marches carrying a tall flag pole upright with both hands, steady.", 3.0, 27),
    ("A_Drummer_Walk", "A drummer marches forward while beating a snare drum hanging at his side with two sticks.", 3.0, 28),
    ("A_Bugler_Stand", "A man stands and plays a bugle, raising it to his mouth, then lowers it.", 3.0, 29),
    ("A_Officer_Point", "An officer stands upright and points forward with his arm extended, giving a command.", 2.5, 30),
    ("A_Officer_Telescope", "An officer raises a telescope to his eye and looks into the distance, then lowers it.", 3.0, 31),
    ("A_Gun_Load_Swab", "A man pushes a long rammer into a cannon muzzle, pulls it out and steps back.", 3.0, 32),
    ("A_Gun_Push", "Three men push a heavy cannon forward leaning into it, straining.", 4.0, 33),
]


def api(path, data=None):
    req = urllib.request.Request(SERVER + path, data=json.dumps(data).encode() if data is not None else None,
                                 headers={"Content-Type": "application/json"})
    return json.loads(urllib.request.urlopen(req, timeout=1800).read())


def graph(name, prompt, duration, seed):
    return {
        "1": {"class_type": "HYMotionLoadNetwork", "inputs": {"model_name": NETWORK}},
        "2": {"class_type": "HYMotionLoadLLM", "inputs": {"model_name": LLM, "quantization": "bnb-4bit", "offload_to_cpu": True}},
        "3": {"class_type": "HYMotionEncodeText", "inputs": {"llm": ["2", 0], "text": prompt}},
        "4": {"class_type": "HYMotionGenerate", "inputs": {"network": ["1", 0], "conditioning": ["3", 0], "duration": duration,
                                                         "seed": seed, "cfg_scale": 5.0, "num_samples": 1}},
        "5": {"class_type": "HYMotionExportFBX", "inputs": {"motion_data": ["4", 0], "output_dir": "hymotion_fbx",
                                                          "filename_prefix": name, "custom_fbx_path": CHARACTER,
                                                          "yaw_offset": 0.0, "scale": 0.0}},
    }


def main():
    wanted = set(sys.argv[1:])
    for name, prompt, duration, seed in CLIPS:
        if wanted and name not in wanted:
            continue
        res = api("/prompt", {"prompt": graph(name, prompt, duration, seed)})
        pid = res["prompt_id"]
        print(name, "queued", pid, flush=True)
        for _ in range(600):
            time.sleep(3)
            hist = api("/history/" + pid)
            if pid in hist:
                status = hist[pid].get("status", {})
                print(name, status.get("status_str"), flush=True)
                if status.get("status_str") != "success":
                    print(json.dumps(status.get("messages", [])[-3:])[:600])
                break


if __name__ == "__main__":
    main()
