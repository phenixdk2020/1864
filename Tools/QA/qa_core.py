"""Shared parts of the QA tracks in Tools/QA (battle video, campaign map and menus, animations, behaviour).

Each track script describes itself with a Track (rubric, finding kinds, prompts, which metric flags support which kind)
and uses the same guards, ffmpeg helpers, three reviewers (local Ollama vision model, Codex, Copilot), merging and Danish
report. Pictures are named with their time (`strip_t0029.2.jpg`) in the timeline tracks, or by what they show
(`map_odense_04km.jpg`) in the still tracks; findings are grouped by time there and by picture here.

Rules (AGENTS.md): only the capture stages start the game, and they refuse when a game or editor already runs (the user's
own game is never closed) or when another job holds the GPU. Reviewers never edit files: Codex runs with a read-only
sandbox, Copilot with shell and write denied.
"""
from __future__ import annotations

import base64
import datetime as dt
import json
import re
import shutil
import subprocess
import sys
import time
import urllib.request
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

ROOT = Path(__file__).resolve().parents[2]
QA_TOOLS = ROOT / "Tools" / "QA"
QA_OUT = ROOT / "Saved" / "QA"
RECORDING = ROOT / "Saved" / "Recording"
SCREENSHOTS = ROOT / "Saved" / "Screenshots"
OLLAMA = "http://localhost:11434"
OLLAMA_MODEL = "qwen3-vl:8b-instruct"
STRIP_EVERY_S = 3.0        # a motion strip every 3 s, plus one at every flagged second
CLOSEUP_EVERY_S = 5.0      # a centre close-up every 5 s, plus at every flagged second
MAX_REMOTE_IMAGES = 8      # pictures for Codex and Copilot each (quota)
GPU_LIMIT_MIB = 6000       # more than this in use (not counting Ollama's own model) means another GPU job runs
REVIEWERS = ("ollama", "codex", "copilot")


def log(msg: str) -> None:
    print(f"[qa] {msg}", flush=True)


def utf8_console() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data) -> None:
    path.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")


def load_scenarios() -> dict:
    return read_json(QA_TOOLS / "scenarios.json")


def run_meta(run: Path) -> dict:
    return read_json(run / "meta.json")


def tool_exe(name: str) -> str:
    """The real executable (an npm .cmd wrapper would send the prompt through cmd, which breaks it at newlines and quotes)."""
    if name == "codex":
        vendored = Path.home() / "AppData/Roaming/npm/node_modules/@openai/codex/node_modules/@openai/codex-win32-x64/vendor/x86_64-pc-windows-msvc/bin/codex.exe"
        if vendored.exists():
            return str(vendored)
    return shutil.which(name) or name


# ---------------------------------------------------------------- guards

def running_processes() -> str:
    try:
        return subprocess.run(["tasklist", "/FO", "CSV", "/NH"], capture_output=True, text=True, errors="replace").stdout
    except OSError:
        return ""


def gpu_used_mib() -> int:
    try:
        out = subprocess.run(["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"],
                             capture_output=True, text=True).stdout
        return int(out.split()[0])
    except (OSError, ValueError, IndexError):
        return 0


def ollama_vram_mib() -> int:
    """VRAM held by models Ollama keeps loaded (ours, freed by keep_alive) - not another job."""
    try:
        with urllib.request.urlopen(f"{OLLAMA}/api/ps", timeout=5) as resp:
            return sum(int(m.get("size_vram", 0)) for m in json.loads(resp.read()).get("models", [])) // (1024 * 1024)
    except Exception:  # noqa: BLE001 - Ollama not running
        return 0


def guard_unreal_free() -> None:
    procs = running_processes()
    for name in ("UnrealEditor.exe", "UnrealEditor-Cmd.exe", "Game1864.exe"):
        if f'"{name}"' in procs:
            sys.exit(f"[qa] {name} kører allerede. Luk det selv (scriptet lukker aldrig dit spil) og prøv igen.")


def guard_gpu_free(force: bool, what: str) -> bool:
    used = gpu_used_mib() - ollama_vram_mib()
    if used > GPU_LIMIT_MIB and not force:
        log(f"GPU'en bruger allerede {used} MiB (fx en Pinokio-app). {what} springes over; brug --force for at køre alligevel.")
        return False
    return True


# ---------------------------------------------------------------- runs

def new_run_dir(name: str, sub: str = "frames") -> Path:
    run = QA_OUT / f"{dt.date.today().isoformat()}-{name}"
    if run.exists():
        run = QA_OUT / f"{dt.date.today().isoformat()}-{name}-{dt.datetime.now().strftime('%H%M%S')}"
    (run / sub).mkdir(parents=True)
    return run


def run_game(cfg: dict, args: list[str], run: Path, timeout_s: float) -> float:
    """Start the editor as a game with the given arguments (log to the run folder) and wait. Returns the start time."""
    full = [cfg["editor"], str(ROOT / "Game1864.uproject"), *args, f"-ABSLOG={run / 'game.log'}"]
    began = time.time()
    proc = subprocess.Popen(full, cwd=ROOT)
    try:
        proc.wait(timeout=timeout_s)
    except subprocess.TimeoutExpired:
        log("testkørslen svarede ikke inden for tiden; lukker kun denne testproces")
        proc.kill()
    return began


def move_new(src: Path, pattern: str, since: float, dest: Path, rename: Callable[[int, Path], str] | None = None) -> list[Path]:
    """Move the files this run wrote (the game numbers from 0 and overwrites; older files stay where they are)."""
    files = sorted(p for p in src.glob(pattern) if p.stat().st_mtime >= since - 1)
    moved = []
    for i, f in enumerate(files):
        target = dest / (rename(i, f) if rename else f.name)
        shutil.move(str(f), target)
        moved.append(target)
    return moved


# ---------------------------------------------------------------- ffmpeg

def ffmpeg(*args: str) -> None:
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *args], check=True)


def stamp(t: float) -> str:
    return f"t{t:06.1f}"


def frame_time(meta: dict, index: int) -> float:
    return meta["start"] + index / meta["fps"]


def frame_at(meta: dict, t: float, count: int) -> int:
    return max(0, min(count - 1, round((t - meta["start"]) * meta["fps"])))


def contact_sheet(images: list[Path], out: Path, columns: int = 5, rows: int = 4) -> None:
    if not images:
        return
    stride = max(1, len(images) // (columns * rows))
    sheet_list = out.parent / "sheet_list.txt"
    sheet_list.write_text("".join(f"file '{k.as_posix()}'\n" for k in images[::stride][:columns * rows]), encoding="utf-8")
    ffmpeg("-f", "concat", "-safe", "0", "-i", str(sheet_list), "-vf", f"scale=384:216:force_original_aspect_ratio=decrease,"
           f"pad=384:216:(ow-iw)/2:(oh-ih)/2,tile={columns}x{rows}", "-frames:v", "1", "-q:v", "3", str(out))
    sheet_list.unlink()


def extract_video(run: Path, flagged: list[float] | None = None, extra_times: list[float] | None = None) -> None:
    """video.mp4, keys/ (one a second), strips/ (6 frames 0.2 s apart, 3x2), closeups/ (centre), kontaktark.jpg."""
    meta = run_meta(run)
    frames = sorted((run / "frames").glob("frame_*.png"))
    count, fps = len(frames), meta["fps"]
    for sub in ("keys", "strips", "closeups"):
        (run / sub).mkdir(exist_ok=True)
    pattern = str(run / "frames" / "frame_%05d.png")
    ffmpeg("-framerate", str(fps), "-i", pattern, "-c:v", "libx264", "-crf", "23", "-pix_fmt", "yuv420p",
           "-vf", "scale=1280:-2", str(run / "video.mp4"))
    t_end = frame_time(meta, count - 1)
    marked = {round(f, 1) for f in (flagged or [])} | {round(f, 1) for f in (extra_times or [])}
    t = float(meta["start"])
    while t <= t_end:
        i = frame_at(meta, t, count)
        ffmpeg("-i", str(frames[i]), "-vf", "scale=960:-2", "-q:v", "3", str(run / "keys" / f"key_{stamp(t)}.jpg"))
        t += 1.0
    step = max(1, round(0.2 * fps))
    span = t_end - meta["start"]
    for st in sorted({round(meta["start"] + k * STRIP_EVERY_S, 1) for k in range(int(span / STRIP_EVERY_S) + 1)} | marked):
        i = frame_at(meta, st, count)
        if i + 5 * step >= count:
            continue
        ffmpeg("-start_number", str(i), "-i", pattern, "-vf", f"select='not(mod(n\\,{step}))',scale=640:-2,tile=3x2",
               "-frames:v", "1", "-q:v", "3", str(run / "strips" / f"strip_{stamp(st)}.jpg"))
    for ct in sorted({round(meta["start"] + k * CLOSEUP_EVERY_S, 1) for k in range(int(span / CLOSEUP_EVERY_S) + 1)} | marked):
        i = frame_at(meta, ct, count)
        ffmpeg("-i", str(frames[i]), "-vf", "crop=iw/2:ih/2,scale=960:-2", "-q:v", "3", str(run / "closeups" / f"closeup_{stamp(ct)}.jpg"))
    keys = sorted((run / "keys").glob("*.jpg"))
    contact_sheet(keys, run / "kontaktark.jpg")
    log(f"udtrukket: {len(keys)} nøglebilleder, {len(list((run / 'strips').glob('*.jpg')))} striber, "
        f"{len(list((run / 'closeups').glob('*.jpg')))} nærbilleder, video.mp4, kontaktark.jpg")


# ---------------------------------------------------------------- tracks

def image_time(name: str) -> float:
    m = re.search(r"_t(\d+\.\d)", name)
    return float(m[1]) if m else -1.0


def image_kind(name: str) -> str:
    return "stribe (6 billeder, 0,2 s mellemrum, venstre->højre, øverst->nederst)" if name.startswith("strip") else \
        "nærbillede (midten af billedet forstørret)" if name.startswith("closeup") else \
        "udsnit i fuld opløsning" if name.startswith("crop") else "skærmbillede"


@dataclass
class Track:
    """What makes one QA track different from the others."""
    name: str                                   # battle, campaign, animation, behaviour
    heading: str                                # report title prefix
    rubric: str                                 # file in Tools/QA
    kinds: dict[str, str]                       # finding kind -> Danish name
    intro: str                                  # who the reviewers are and what the game is
    rules: str                                  # extra rules for the remote reviewers
    codex_role: str
    copilot_role: str
    ollama_prompt: Callable[[Path, dict, dict], str]           # (picture, metrics, meta)
    picture_dirs: tuple[str, ...] = ("strips", "closeups", "keys")
    kinds_for_metric: dict[str, set[str]] = field(default_factory=dict)   # metric flag -> kinds it supports
    metric_kind: dict[str, str] = field(default_factory=dict)             # metric flag -> kind when no reviewer saw it
    picture_note: Callable[[dict, Path], str] = lambda data, img: ""      # extra per picture for the remote listing
    excerpt: Callable[[dict, list[Path]], str] = lambda data, imgs: ""    # metrics to give the remote reviewers
    summary: Callable[[dict, dict], list[str]] = lambda data, meta: []    # metric lines at the top of the report
    copilot_refs: list[Path] = field(default_factory=list)
    timeline: bool = True                       # group findings by time (video) or by picture (stills)

    def schema_path(self, run: Path) -> Path:
        """The shared review schema with this track's finding kinds."""
        schema = read_json(QA_TOOLS / "review_schema.json")
        schema["properties"]["findings"]["items"]["properties"]["kind"]["enum"] = list(self.kinds)
        path = run / "reviews" / "schema.json"
        write_json(path, schema)
        return path


def prompt_text(track: Track, meta: dict, extra: str) -> str:
    rubric = (QA_TOOLS / track.rubric).read_text(encoding="utf-8")
    focus = ", ".join(meta.get("focus", [])) or "alle punkter"
    return (f"{track.intro} Scenarie: {meta['title']}. Vægt især: {focus}.\n\n{rubric}\n\n{extra}\n"
            f"Regler: Et fund skal kunne SES på billedet; skriv ikke fund, der kun bygger på målingerne (de er baggrund). {track.rules}\n"
            "Svar KUN med JSON efter skemaet {\"findings\": [{\"image\", \"kind\", \"severity\", \"where\", \"description\", "
            "\"confidence\", \"code_hint\"}]}. code_hint er en tom streng, hvis du ikke ved det.")


def parse_findings(text: str, reviewer: str) -> list[dict]:
    start, end = text.find("{"), text.rfind("}")
    if start < 0 or end < start:
        return []
    try:
        data = json.loads(text[start:end + 1])
    except json.JSONDecodeError:
        return []
    out = []
    for f in data.get("findings", []):
        if not isinstance(f, dict) or not f.get("image"):
            continue
        f["reviewer"] = reviewer
        f["t"] = image_time(str(f["image"]))
        out.append(f)
    return out


# ---------------------------------------------------------------- reviewers

def ollama_review(run: Path, track: Track, meta: dict, data: dict, images: list[Path], limit: int) -> dict:
    schema = read_json(track.schema_path(run))
    findings, errors, began, raw = [], 0, time.time(), []
    for n, img in enumerate(images[:limit]):
        body = {"model": OLLAMA_MODEL, "stream": False, "format": schema, "keep_alive": "10m",
                "options": {"temperature": 0, "num_ctx": 8192},
                "messages": [{"role": "user", "content": track.ollama_prompt(img, data, meta),
                              "images": [base64.b64encode(img.read_bytes()).decode()]}]}
        try:
            req = urllib.request.Request(f"{OLLAMA}/api/chat", data=json.dumps(body).encode(), headers={"Content-Type": "application/json"})
            with urllib.request.urlopen(req, timeout=300) as resp:
                answer = json.loads(resp.read())["message"]["content"]
            raw.append({"image": img.name, "answer": answer})
            for f in parse_findings(answer, "ollama"):
                f["image"] = img.name   # the small model sometimes misnames the picture
                f["t"] = image_time(img.name)
                findings.append(f)
        except Exception as e:  # noqa: BLE001 - a reviewer failing must not stop the chain
            errors += 1
            log(f"ollama fejl ved {img.name}: {e}")
        if n % 10 == 9:
            log(f"ollama: {n + 1}/{min(limit, len(images))} billeder")
    try:   # free the VRAM again
        urllib.request.urlopen(urllib.request.Request(f"{OLLAMA}/api/generate", data=json.dumps(
            {"model": OLLAMA_MODEL, "keep_alive": 0}).encode(), headers={"Content-Type": "application/json"}), timeout=60).read()
    except Exception:  # noqa: BLE001
        pass
    write_json(run / "reviews" / "ollama_answers.json", raw)
    return {"reviewer": "ollama", "images": min(limit, len(images)), "errors": errors, "seconds": round(time.time() - began),
            "findings": findings}


def remote_prompt(track: Track, meta: dict, data: dict, images: list[Path], role: str) -> str:
    listing = "\n".join(f"- {img.name}: {image_kind(img.name)}"
                        + (f", t={image_time(img.name):.1f} s" if image_time(img.name) >= 0 else "")
                        + track.picture_note(data, img) for img in images)
    excerpt = track.excerpt(data, images)
    return prompt_text(track, meta, f"{role}\n\nVedhæftede billeder:\n{listing}"
                                    + (f"\n\nMålinger fra spillet omkring de billeder:\n{excerpt}" if excerpt else ""))


def codex_review(run: Path, track: Track, meta: dict, data: dict, images: list[Path]) -> dict:
    began = time.time()
    out = run / "reviews" / "codex_answer.txt"
    prompt = remote_prompt(track, meta, data, images, track.codex_role)
    args = [tool_exe("codex"), "exec", prompt, "--ephemeral", "-s", "read-only", "--skip-git-repo-check", "-C", str(ROOT),
            "--output-schema", str(track.schema_path(run)), "-o", str(out)]
    for img in images:
        args += ["-i", str(img)]
    try:
        proc = subprocess.run(args, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=1200)
        answer = out.read_text(encoding="utf-8") if out.exists() else proc.stdout
        status = "ok" if proc.returncode == 0 else f"fejl {proc.returncode}: {proc.stderr.strip()[-300:]}"
    except (OSError, subprocess.TimeoutExpired) as e:
        answer, status = "", f"fejl: {e}"
    return {"reviewer": "codex", "images": len(images), "status": status, "seconds": round(time.time() - began),
            "findings": parse_findings(answer, "codex")}


def copilot_review(run: Path, track: Track, meta: dict, data: dict, images: list[Path]) -> dict:
    began = time.time()
    refs = [r for r in track.copilot_refs if r.exists()]
    role = track.copilot_role + (f" De sidste {len(refs)} vedhæftede billeder ({', '.join(r.name for r in refs)}) er referencer og "
                                 "ikke fra spillet." if refs else "")
    prompt = remote_prompt(track, meta, data, images, role)
    args = [tool_exe("copilot"), "-p", prompt, "-s", "--no-ask-user", "--deny-tool", "shell", "--deny-tool", "write", "--model", "auto"]
    for img in [*images, *refs]:
        args += ["--attachment", str(img)]
    try:
        proc = subprocess.run(args, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=1200, cwd=run)
        answer = proc.stdout
        status = "ok" if proc.returncode == 0 else f"fejl {proc.returncode}: {proc.stderr.strip()[-300:]}"
    except (OSError, subprocess.TimeoutExpired) as e:
        answer, status = "", f"fejl: {e}"
    (run / "reviews" / "copilot_answer.txt").write_text(answer, encoding="utf-8")
    return {"reviewer": "copilot", "images": len(images), "status": status, "seconds": round(time.time() - began),
            "findings": parse_findings(answer, "copilot")}


def flag_matches(flag: dict, name: str, window: float) -> bool:
    if "image" in flag:
        return flag["image"] == name
    return image_time(name) >= 0 and abs(image_time(name) - flag["t"]) <= window


def pictures(run: Path, track: Track) -> dict[str, Path]:
    return {p.name: p for sub in track.picture_dirs for p in sorted((run / sub).glob("*.jpg"))}


def pick_remote_images(run: Path, track: Track, data: dict, local: list[dict]) -> list[Path]:
    """The most suspicious pictures: metric flags first (strips weigh most), then Ollama's surest findings."""
    score: dict[str, float] = {}
    pics = pictures(run, track)
    for f in data.get("flags", []):
        for name in pics:
            if flag_matches(f, name, 0.6):
                score[name] = score.get(name, 0) + (3 if name.startswith("strip") else 2)
    for f in local:
        if f["image"] in pics:
            score[f["image"]] = score.get(f["image"], 0) + float(f.get("severity", 1)) * float(f.get("confidence", 0.5))
    chosen, moments = [], set()
    for name, _ in sorted(score.items(), key=lambda kv: -kv[1]):
        moment = round(image_time(name)) if track.timeline else name
        if moment in moments:
            continue   # one picture per second, so eight pictures cover eight moments
        moments.add(moment)
        chosen.append(pics[name])
        if len(chosen) == MAX_REMOTE_IMAGES:
            break
    if len(chosen) < MAX_REMOTE_IMAGES:   # a calm run: spread the rest over the first picture folder
        spread = sorted((run / track.picture_dirs[0]).glob("*.jpg"))
        for p in spread[::max(1, len(spread) // MAX_REMOTE_IMAGES)]:
            if p not in chosen and len(chosen) < MAX_REMOTE_IMAGES:
                chosen.append(p)
    return sorted(chosen, key=lambda p: (image_time(p.name), p.name))


def review(run: Path, track: Track, skip: set[str], ollama_limit: int, force: bool,
           order: Callable[[list[Path], dict], list[Path]] | None = None) -> None:
    meta = run_meta(run)
    data = read_json(run / "metrics.json")
    (run / "reviews").mkdir(exist_ok=True)
    local: list[dict] = []
    if "ollama" not in skip:
        if '"UnrealEditor.exe"' in running_processes() and not force:
            log("Unreal kører; ollama springes over (ét GPU-tungt job ad gangen)")
            write_json(run / "reviews" / "ollama.json", {"reviewer": "ollama", "status": "sprunget over: Unreal kører", "findings": []})
        elif guard_gpu_free(force, "Ollama"):
            images = list(pictures(run, track).values())
            images = order(images, data) if order else images
            result = ollama_review(run, track, meta, data, images, ollama_limit)
            result["status"] = "ok" if not result["errors"] else f"{result['errors']} fejl"
            write_json(run / "reviews" / "ollama.json", result)
            local = result["findings"]
            log(f"ollama: {len(local)} fund på {result['images']} billeder ({result['seconds']} s)")
        else:
            write_json(run / "reviews" / "ollama.json", {"reviewer": "ollama", "status": "sprunget over: GPU optaget", "findings": []})
    remote = pick_remote_images(run, track, data, local)
    write_json(run / "reviews" / "remote_images.json", [p.name for p in remote])
    for name, fn in (("codex", codex_review), ("copilot", copilot_review)):
        if name in skip:
            continue
        log(f"{name}: {len(remote)} billeder")
        result = fn(run, track, meta, data, remote)
        write_json(run / "reviews" / f"{name}.json", result)
        log(f"{name}: {result['status']}, {len(result['findings'])} fund ({result['seconds']} s)")


# ---------------------------------------------------------------- report

def same_moment(track: Track, a: dict, b: dict) -> bool:
    if track.timeline and a["t"] >= 0 and b["t"] >= 0:
        return abs(a["t"] - b["t"]) <= 1.5
    return a.get("image") == b.get("image")


def merge(run: Path, track: Track) -> tuple[list[dict], dict]:
    data = read_json(run / "metrics.json")
    results = {}
    for name in REVIEWERS:
        path = run / "reviews" / f"{name}.json"
        results[name] = read_json(path) if path.exists() else {"status": "ikke kørt", "findings": []}
    groups: list[dict] = []
    for name in REVIEWERS:
        for f in results[name]["findings"]:
            if f.get("kind") not in track.kinds:
                f["kind"] = "other"
            for g in groups:
                if g["kind"] == f["kind"] and same_moment(track, g, f):
                    g["items"].append(f)
                    break
            else:
                groups.append({"kind": f["kind"], "t": f["t"], "image": f["image"], "items": [f]})
    flags = data.get("flags", [])
    for g in groups:
        g["reviewers"] = sorted({f["reviewer"] for f in g["items"]})
        g["metric"] = [fl for fl in flags if g["kind"] in track.kinds_for_metric.get(fl["flag"], set())
                       and any(flag_matches(fl, f["image"], 1.5) for f in g["items"])]
        # Ollama alone is a triage pass (many repeated, generic findings), so it never confirms on its own.
        g["confirmed"] = bool(g["metric"]) or (len(g["reviewers"]) >= 2 and bool(set(g["reviewers"]) - {"ollama"}))
        g["severity"] = max(int(f.get("severity", 1)) for f in g["items"])
        g["confidence"] = max(float(f.get("confidence", 0)) for f in g["items"])
    # Metric flags no reviewer reported still belong in the report.
    for fl in flags:
        if not any(fl in g["metric"] for g in groups):
            groups.append({"kind": track.metric_kind.get(fl["flag"], "other"), "t": fl.get("t", -1.0), "image": fl.get("image", ""),
                           "items": [], "reviewers": [], "metric": [fl], "confirmed": False, "severity": 2, "confidence": 1.0,
                           "metric_only": True})
    groups.sort(key=lambda g: (not g["confirmed"], -g["severity"], -g["confidence"], g["t"], g["image"]))
    return groups, results


def report(run: Path, track: Track, header: list[str]) -> Path:
    meta = run_meta(run)
    groups, results = merge(run, track)
    data = read_json(run / "metrics.json")
    confirmed = [g for g in groups if g["confirmed"]]
    possible_all = [g for g in groups if not g["confirmed"] and not g.get("metric_only")]
    # Ollama-only findings: only the surest few are listed, the rest are counted per kind.
    possible = [g for g in possible_all if g["reviewers"] != ["ollama"]]
    ollama_only = [g for g in possible_all if g["reviewers"] == ["ollama"]]
    ollama_shown = [g for g in ollama_only if g["severity"] >= 2 and g["confidence"] >= 0.8][:10]
    ollama_counts: dict[str, int] = {}
    for g in ollama_only:
        ollama_counts[g["kind"]] = ollama_counts.get(g["kind"], 0) + 1
    metric_only = [g for g in groups if g.get("metric_only")]

    def picture(g: dict) -> str:
        names = []
        for name in [f["image"] for f in g["items"]] + ([g["image"]] if g.get("metric_only") and g["image"] else []):
            for sub in track.picture_dirs:
                if (run / sub / name).exists() and f"{sub}/{name}" not in names:
                    names.append(f"{sub}/{name}")
        return ", ".join(f"[{Path(n).stem}]({n})" for n in names[:3]) or "–"

    def rows(items: list[dict]) -> list[str]:
        first = "t (s)" if track.timeline else "Billede"
        out = [f"| {first} | Fejl | Alvor | Set af | Måling | Beskrivelse | Billeder |", "|---|---|---|---|---|---|---|"]
        for g in items:
            desc = " / ".join(dict.fromkeys(f"{f.get('where', '')}: {f.get('description', '')}".strip(": ") for f in g["items"]))
            hints = "; ".join(dict.fromkeys(f["code_hint"] for f in g["items"] if f.get("code_hint")))
            if hints:
                desc += f" — kode: {hints}"
            metric = "; ".join(dict.fromkeys(fl["detail"] for fl in g["metric"])) or "–"
            where = f"{g['t']:.1f}" if track.timeline and g["t"] >= 0 else Path(g["image"]).stem
            out.append(f"| {where} | {track.kinds.get(g['kind'], g['kind'])} | {g['severity']} | {', '.join(g['reviewers']) or '–'} "
                       f"| {metric} | {desc.replace('|', '/')[:400] or '–'} | {picture(g)} |")
        return out

    lines = [f"# {track.heading}: {meta['title']}", "", *header, "",
             "## Bedømmere", "", "| Bedømmer | Status | Billeder | Fund | Tid |", "|---|---|---|---|---|"]
    for name in REVIEWERS:
        r = results[name]
        lines.append(f"| {name} | {r.get('status', '–')} | {r.get('images', 0)} | {len(r['findings'])} | {r.get('seconds', 0)} s |")
    summary = track.summary(data, meta)
    if summary:
        lines += ["", *summary]
    lines += ["", f"## Bekræftede fund ({len(confirmed)})", "",
              "Set af mindst to bedømmere eller støttet af spillets målinger.", ""]
    lines += rows(confirmed) if confirmed else ["Ingen."]
    lines += ["", f"## Mulige fund ({len(possible)})", "", "Set af Codex eller Copilot alene, uden støtte i målingerne. Skal ses efter.", ""]
    lines += rows(possible) if possible else ["Ingen."]
    lines += ["", f"## Kun Ollama ({len(ollama_only)})", "",
              "Den lokale model er en grovsortering og gentager sig ofte. Her vises højst 10 med alvor 2+ og sikkerhed 0,8+. "
              "Alle typer: " + (", ".join(f"{track.kinds.get(k, k)} {n}" for k, n in sorted(ollama_counts.items(), key=lambda kv: -kv[1])) or "ingen") + ".", ""]
    lines += rows(ollama_shown) if ollama_shown else ["Ingen over grænsen."]
    lines += ["", f"## Kun i målingerne ({len(metric_only)})", "", "Markeret af målingerne, men ingen bedømmer så det på billederne.", ""]
    lines += rows(metric_only) if metric_only else ["Ingen."]
    lines += ["", "Fundene er ikke rettet. Hvert bekræftet fund kontrolleres på billederne og i koden, før der rettes."]
    path = run / "rapport.md"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    log(f"rapport: {path} ({len(confirmed)} bekræftede, {len(possible)} mulige, {len(metric_only)} kun målinger)")
    return path


def common_args(ap) -> None:
    ap.add_argument("--run", type=Path, help="eksisterende kørselsmappe")
    ap.add_argument("--skip", default="", help="kommasepareret: ollama,codex,copilot")
    ap.add_argument("--ollama-limit", type=int, default=60, help="højst så mange billeder til Ollama")
    ap.add_argument("--force", action="store_true", help="kør selv om GPU'en er optaget")


def resolve_run(path: Path) -> Path:
    return path if path.is_absolute() else ROOT / path
