"""Syntetiske slaglyde. Ingen netværk; numpy er valgfri."""
import argparse
import math
import random
import struct
import wave
from pathlib import Path
try:
    import numpy as np
except ImportError:
    np = None
RATE = 44100
rng = random.Random(1864)


def blast(seconds, frequency, decay, crack=1.0):
    result = []
    low = 0.0
    for i in range(int(seconds * RATE)):
        t = i / RATE
        noise = rng.uniform(-1, 1)
        low += 0.045 * (noise - low)
        attack = min(1.0, t / 0.0007)
        tail = min(1.0, (seconds - t) / 0.025)
        result.append(attack * tail * (crack * noise * math.exp(-t / 0.022)
                      + 0.65 * math.sin(2 * math.pi * frequency * t) * math.exp(-t / decay)
                      + 1.8 * low * math.exp(-t / (decay * 1.4))))
    return result


def overlap(clips, seconds, count, spread):
    result = [0.0] * int(seconds * RATE)
    for _ in range(count):
        clip = rng.choice(clips)
        offset = int(rng.uniform(0, spread) * RATE)
        gain = rng.uniform(0.65, 1.0)
        for i, value in enumerate(clip):
            if offset + i < len(result):
                result[offset + i] += gain * value
    return result


def save(folder, name, samples):
    # Fjern DC og normalisér peak til -3 dBFS, også uden numpy.
    mean = sum(samples) / len(samples)
    samples = [s - mean for s in samples]
    peak = max(abs(s) for s in samples) or 1
    scale = 32767 * 10 ** (-3 / 20) / peak
    if np is not None:
        data = np.rint(np.asarray(samples) * scale).astype('<i2').tobytes()
    else:
        data = b''.join(struct.pack('<h', round(s * scale)) for s in samples)
    with wave.open(str(folder / (name + '.wav')), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(data)
    print(name, len(samples) / RATE)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pure-python', action='store_true')
    args = parser.parse_args()
    global np
    if args.pure_python:
        np = None
    folder = Path(__file__).resolve().parents[2] / 'Reference/Battle/Audio'
    folder.mkdir(parents=True, exist_ok=True)
    muskets = [blast(0.48 + i * 0.025, 105 + i * 13, 0.075 + i * 0.008,
                     0.9 + i * 0.07) for i in range(6)]
    for i, clip in enumerate(muskets, 1):
        save(folder, f'musket_shot_{i:02}', clip)
    save(folder, 'volley_small', overlap(muskets, 1.05, 20, 0.35))
    save(folder, 'volley_large', overlap(muskets, 1.95, 60, 1.2))
    for i in range(4):
        save(folder, f'cannon_{i+1:02}', blast(1.95, 42 + i * 14, 0.38 + i * 0.08, 0.8))
    save(folder, 'mortar_thump', blast(1.8, 48, 0.42, 0.45))
    save(folder, 'rifle_crack', blast(0.38, 180, 0.045, 1.8))
    clash = blast(0.32, 2300, 0.06, 0.7)
    for i in range(len(clash)):
        t = i / RATE
        clash[i] += 0.3 * math.sin(2 * math.pi * 3713 * t) * math.exp(-t / 0.055)
    save(folder, 'bayonet_clash', clash)
    rumble = overlap([blast(2.5, 45 + i * 8, 0.8, 0.03) for i in range(4)], 8, 35, 8)
    # Kort ind-/udtoning ved loopgrænsen begrænser klik.
    hooves = overlap([blast(0.12, 130, 0.025, 0.12)], 4, 32, 3.85)
    for name, clip in [('distant_rumble', rumble), ('cavalry_hooves', hooves)]:
        n = int(0.15 * RATE)
        for i in range(n):
            clip[i] *= i / n
            clip[-1-i] *= i / n
        save(folder, name, clip)


if __name__ == '__main__':
    main()
