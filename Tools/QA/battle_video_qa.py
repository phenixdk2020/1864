#!/usr/bin/env python3
"""Video QA of the 3D battles: record a scripted test battle, cut it into stills and motion strips, read the formation
metrics, and let three reviewers (local Ollama vision model, Codex, Copilot) judge the pictures against
Tools/QA/rubric_battle.md. The findings are merged into a Danish report. Shared parts: Tools/QA/qa_core.py.

Stages (each can run alone on an existing run folder):
  capture  start the game with the scenario's drill plan and recorder, move the frames and the log into the run folder
  extract  ffmpeg: mp4, one key frame a second, 3x2 motion strips (0.2 s apart), centre close-ups, contact sheet
  metrics  Strategy1864FormationMetric/Closest lines -> metrics.json, with flagged seconds
  review   Ollama on every key/strip/close-up, then Codex and Copilot on the (at most 8) most suspicious pictures
  report   merge the findings -> rapport.md

Usage:
  python Tools/QA/battle_video_qa.py all tab-i-kolonne
  python Tools/QA/battle_video_qa.py review --run Saved/QA/2026-10-28-tab-i-kolonne --skip codex
  python Tools/QA/battle_video_qa.py import --frames Saved/Recording --log <log> --fps 10 --start 6 --name dry-run
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

import qa_core as qa

METRIC_RE = re.compile(r"Strategy1864FormationMetric (\S+) t=([\d.]+) status=(.*?) formation=(\S+) ready=(\d+)/(\d+) "
                       r"max-gap-cm=(\S+) p95-gap-cm=(\S+) min-neighbour-cm=(\S+) bearer-gap-cm=(\S+) drill=(\d) plan=(\d) "
                       r"arrival=(\d) speed=(\S+)")
CLOSEST_RE = re.compile(r"Strategy1864FormationClosest (\S+) t=([\d.]+) (\d+) cm: (.*)")

KINDS = {"overlap": "Mænd oven i hinanden", "rifle_hand": "Gevær uden for hånden", "flag_hand": "Fane uden for hånden",
         "bearer": "Fanebærerens plads", "ground": "Svæver / i jorden", "ranks": "Rækker og huller",
         "column": "Kolonnens form", "step_up": "Opslutning efter tab", "closing": "Sammenrykning",
         "death_pose": "Døde mænds stilling", "volley": "Salver", "reload": "Ladning", "smoke": "Krudtrøg",
         "foot_slide": "Glidende fødder", "turn": "Drejning", "other": "Andet"}


# ---------------------------------------------------------------- capture

def capture(name: str, force: bool) -> Path:
    cfg = qa.load_scenarios()
    sc = cfg["scenarios"][name]
    qa.guard_unreal_free()
    if not qa.guard_gpu_free(force, "Optagelsen"):
        sys.exit(1)
    run = qa.new_run_dir(name)
    start, end = sc["record"]
    base = [a for a in cfg["base_args"] if a not in sc.get("drop_args", [])]
    args = [cfg["map"], *base, "-benchmark", f"-fps={sc['fps']}", f"-Strategy1864DrillTest={sc['drill']}",
            f"-Strategy1864FollowCam={sc['follow']}", f"-Strategy1864Record={start}:{end}", *sc.get("extra_args", [])]
    qa.write_json(run / "meta.json", {"scenario": name, "title": sc["title"], "fps": sc["fps"], "start": start, "end": end,
                                      "focus": sc.get("focus", []), "command": args})
    qa.log(f"starter spillet: {name} (optager {start}-{end} s)")
    began = qa.run_game(cfg, args, run, sc.get("timeout_s", 900))
    frames = qa.move_new(qa.RECORDING, "frame_*.png", began, run / "frames", lambda i, f: f"frame_{i:05d}.png")
    qa.log(f"{len(frames)} billeder flyttet til {run / 'frames'}")
    if not frames:
        sys.exit("[qa] ingen billeder optaget; se game.log")
    return run


def import_run(frames_dir: Path, log_file: Path, fps: float, start: float, name: str, first: int, last: int | None) -> Path:
    """A run folder from an earlier recording (copied, the source is left as it is)."""
    run = qa.new_run_dir(name)
    frames = sorted(frames_dir.glob("frame_*.png"))[first:last]
    for i, frame in enumerate(frames):
        shutil.copy2(frame, run / "frames" / f"frame_{i:05d}.png")
    shutil.copy2(log_file, run / "game.log")
    qa.write_json(run / "meta.json", {"scenario": name, "title": f"Import af {frames_dir.name}", "fps": fps, "start": start,
                                      "end": start + len(frames) / fps, "focus": [], "command": "import"})
    qa.log(f"{len(frames)} billeder kopieret til {run}")
    return run


# ---------------------------------------------------------------- metrics

def as_float(text: str) -> float:
    try:
        value = float(text)
    except ValueError:
        return float("inf")
    return value if value < 1e30 else float("inf")


def metrics(run: Path) -> dict:
    meta = qa.run_meta(run)
    text = (run / "game.log").read_text(encoding="utf-8", errors="replace")
    rows, closest, flags = [], [], []
    for m in METRIC_RE.finditer(text):
        row = {"unit": m[1], "t": float(m[2]), "status": m[3], "formation": m[4], "ready": int(m[5]), "men": int(m[6]),
               "max_gap": as_float(m[7]), "p95_gap": as_float(m[8]), "min_neighbour": as_float(m[9]),
               "bearer_gap": as_float(m[10]), "drill": int(m[11]), "plan": int(m[12]), "arrival": int(m[13]),
               "speed": as_float(m[14])}
        rows.append(row)
        if not meta["start"] - 0.5 <= row["t"] <= meta["end"] + 0.5:
            continue
        busy = row["drill"] or row["plan"] or row["arrival"]
        if row["min_neighbour"] < 40:
            flags.append({"t": row["t"], "flag": "overlap", "detail": f"mindste naboafstand {row['min_neighbour']:.0f} cm"})
        if row["men"] and row["ready"] / row["men"] < 0.8 and not busy:
            flags.append({"t": row["t"], "flag": "ranks", "detail": f"kun {row['ready']}/{row['men']} på plads ({row['status']})"})
        if row["bearer_gap"] < 40:
            flags.append({"t": row["t"], "flag": "bearer_close", "detail": f"fanebærer {row['bearer_gap']:.0f} cm fra nærmeste mand"})
        elif 450 < row["bearer_gap"] < float("inf"):
            flags.append({"t": row["t"], "flag": "bearer_far", "detail": f"fanebærer {row['bearer_gap']:.0f} cm fra nærmeste mand"})
    for m in CLOSEST_RE.finditer(text):
        closest.append({"unit": m[1], "t": float(m[2]), "cm": int(m[3]), "pair": m[4][:160]})
    data = {"rows": rows, "closest": closest, "flags": flags,
            "flagged_seconds": sorted({round(f["t"], 1) for f in flags})}
    qa.write_json(run / "metrics.json", data)
    qa.log(f"målinger: {len(rows)} linjer, {len(flags)} markeringer i {len(data['flagged_seconds'])} sekunder")
    return data


def metric_excerpt(data: dict, images: list[Path], window: float = 2.0) -> str:
    times = [qa.image_time(i.name) for i in images]
    lines = []
    for row in data["rows"]:
        if any(abs(row["t"] - t) <= window for t in times):
            lines.append(f"t={row['t']:.1f} {row['unit']} status={row['status']} formation={row['formation']} "
                         f"ready={row['ready']}/{row['men']} min-neighbour={row['min_neighbour']:.0f}cm "
                         f"bearer-gap={row['bearer_gap']:.0f}cm p95-gap={row['p95_gap']:.0f}cm speed={row['speed']:.0f}")
    for c in data["closest"]:
        if any(abs(c["t"] - t) <= window for t in times):
            lines.append(f"t={c['t']:.1f} nærmeste par {c['cm']} cm: {c['pair']}")
    if not lines:
        return "(ingen målinger i dette tidsrum)"
    return ("ready = mænd på plads inden for 60 cm; min-neighbour = mindste afstand mellem to mænd\n"
            + "\n".join(lines[:80]))


def data_formation(data: dict, t: float) -> str:
    near = [r for r in data["rows"] if abs(r["t"] - t) <= 1.0]
    return near[0]["formation"] if near else "unknown"


def summary(data: dict, meta: dict) -> list[str]:
    rows_in_window = [r for r in data["rows"] if meta["start"] <= r["t"] <= meta["end"]]
    if not rows_in_window:
        return []
    worst = min(r["min_neighbour"] for r in rows_in_window)
    return [f"Målinger i optagevinduet: {len(rows_in_window)} sekunder, mindste naboafstand {worst:.0f} cm, "
            f"{len(data['flags'])} markeringer."]


# ---------------------------------------------------------------- track

def ollama_prompt(img: Path, data: dict, meta: dict) -> str:
    """A short English checklist: the 8B model answers it far better than the full Danish rubric."""
    formation = data_formation(data, qa.image_time(img.name))
    kind = ("a motion strip: 6 frames 0.2 s apart, read left to right, top to bottom" if img.name.startswith("strip")
            else "a close-up of the centre of the screen" if img.name.startswith("closeup") else "a screenshot")
    return (f"QA of a 3D game with 19th-century infantry. The picture '{img.name}' is {kind}. The company is in {formation} "
            "formation (in a column the colour bearer walks in front and the men face the marching direction; in line he "
            "stands beside the right flank).\n"
            "Look carefully at the soldiers and report every visible defect of these kinds:\n"
            "overlap = two soldiers inside each other or merged; rifle_hand = rifle floating away from the hands or through the body; "
            "flag_hand = flag pole not in the bearer's hands; ground = figures floating above or sunk into the ground; "
            "ranks = ragged ranks or a gap; death_pose = a fallen man lying wrongly (in the air, inside another man); "
            "foot_slide = feet sliding without stepping (strips only); other = anything else that looks broken.\n"
            "Only report what you can see. For each finding give image, kind, severity 1-3, where (in Danish), description "
            "(in Danish), confidence 0-1 and code_hint (empty string). If nothing is wrong, return an empty list.")


def ollama_order(images: list[Path], data: dict) -> list[Path]:
    """Flagged seconds first, strips before close-ups and keys."""
    flagged = {round(t) for t in data["flagged_seconds"]}
    return sorted(images, key=lambda p: (round(qa.image_time(p.name)) not in flagged, not p.name.startswith("strip"),
                                         not p.name.startswith("closeup"), qa.image_time(p.name)))


TRACK = qa.Track(
    name="battle", heading="Video-QA", rubric="rubric_battle.md", kinds=KINDS,
    intro="Du er QA-tester på et 3D-strategispil fra 1850'erne-60'erne (dansk infanteri, eksercerreglement 1863).",
    rules=("Brug formationen i målingerne (Line/Column) til at forstå billedet: i kolonne går fanebæreren foran forreste række, "
           "og mændene vender i marchretningen."),
    codex_role=("Rolle: teknisk QA. Find fejl i animation og formation, og peg i code_hint på den sandsynlige årsag i "
                "Source/Strategy1864 (fx Visual/StrategyInfantryVisualComponent.cpp). Du må læse koden, men ikke ændre noget."),
    copilot_role=("Rolle: historisk realisme. Vurdér billederne mod eksercerreglementet 1863: afstande, takt, formationens "
                  "form, fanens plads og hvordan mændene bevæger sig."),
    ollama_prompt=ollama_prompt,
    kinds_for_metric={"overlap": {"overlap"}, "ranks": {"ranks", "closing", "step_up", "column"},
                      "bearer_close": {"bearer", "overlap", "flag_hand"}, "bearer_far": {"bearer", "flag_hand"}},
    metric_kind={"overlap": "overlap", "ranks": "ranks", "bearer_close": "bearer", "bearer_far": "bearer"},
    picture_note=lambda data, img: f", formation {data_formation(data, qa.image_time(img.name))}",
    excerpt=metric_excerpt, summary=summary,
    copilot_refs=sorted((qa.ROOT / "Reference" / "Drill").glob("*.png"))[:1])


def header(meta: dict) -> list[str]:
    return [f"Scenarie `{meta['scenario']}`, optaget {meta['start']}–{meta['end']} s ved {meta['fps']} billeder/s. "
            f"Video: [video.mp4](video.mp4), kontaktark: [kontaktark.jpg](kontaktark.jpg)."]


# ---------------------------------------------------------------- main

def main() -> None:
    qa.utf8_console()
    ap = argparse.ArgumentParser(description="Video-QA af 3D-slagene (se Tools/QA/README.md)")
    ap.add_argument("stage", choices=["all", "capture", "extract", "metrics", "review", "report", "import", "list"])
    ap.add_argument("scenario", nargs="?", help="navn fra Tools/QA/scenarios.json (capture/all)")
    qa.common_args(ap)
    ap.add_argument("--frames", type=Path, help="import: mappe med frame_*.png")
    ap.add_argument("--log", type=Path, help="import: spillets log")
    ap.add_argument("--fps", type=float, default=10, help="import: optagelsens billeder/s")
    ap.add_argument("--start", type=float, default=0, help="import: første billedes spiltid (s)")
    ap.add_argument("--first", type=int, default=0, help="import: første billednummer")
    ap.add_argument("--last", type=int, default=None, help="import: sidste billednummer (eksklusiv)")
    ap.add_argument("--name", default="import", help="import: navn på kørslen")
    a = ap.parse_args()
    skip = {s for s in a.skip.split(",") if s}
    if a.stage == "list":
        for name, sc in qa.load_scenarios()["scenarios"].items():
            print(f"{name:22} {sc['title']}")
        return
    if a.stage in ("all", "capture"):
        if not a.scenario:
            ap.error("angiv et scenarie")
        run = capture(a.scenario, a.force)
    elif a.stage == "import":
        if not (a.frames and a.log):
            ap.error("import kræver --frames og --log")
        run = import_run(a.frames, a.log, a.fps, a.start, a.name, a.first, a.last)
    else:
        if not a.run:
            ap.error("angiv --run")
        run = qa.resolve_run(a.run)
    if a.stage in ("all", "import", "metrics"):
        metrics(run)
    if a.stage in ("all", "import", "extract"):
        flagged = qa.read_json(run / "metrics.json")["flagged_seconds"] if (run / "metrics.json").exists() else []
        qa.extract_video(run, flagged)
    if a.stage in ("all", "import", "review"):
        qa.review(run, TRACK, skip, a.ollama_limit, a.force, ollama_order)
    if a.stage in ("all", "import", "review", "report"):
        qa.report(run, TRACK, header(qa.run_meta(run)))


if __name__ == "__main__":
    main()
