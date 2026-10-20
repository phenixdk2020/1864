#!/usr/bin/env python3
"""Valider kampagneloggens JSONL-skema uden Unreal. Ukendte kinds giver en advarsel."""
import argparse
from collections import Counter
from datetime import date
import json
import math
from pathlib import Path

KINDS = set("war peace relation mobilise march recruit budget tax build research event alliance other".split())
ACTORS = {"ai", "player", "minister", "event"}

def number(value):
    return type(value) in (int, float) and math.isfinite(value)

def validate(record, strict_kinds=False):
    errors = []
    if not isinstance(record, dict):
        return ["posten er ikke et objekt"]
    def require(key, test, description):
        if key not in record or not test(record[key]):
            errors.append(key + ": " + description)
    require("t", lambda v: isinstance(v, str) and v in {"session", "decision", "event", "snapshot"}, "ukendt posttype")
    try:
        value = record.get("date")
        assert isinstance(value, str) and len(value) == 10
        assert date.fromisoformat(value).isoformat() == value
    except (ValueError, TypeError, AssertionError):
        errors.append("date: forventer YYYY-MM-DD")
    def string(key):
        require(key, lambda v: isinstance(v, str), "forventer tekst")
    typ = record.get("t")
    if typ == "session":
        require("event", lambda v: isinstance(v, str) and v in {"new", "load", "save", "quit"}, "ukendt sessionshaendelse")
        for key in ("scenario", "save_version"):
            require(key, lambda v: type(v) is int, "forventer heltal")
        string("slot")
    else:
        string("nation")
    if typ == "decision":
        require("actor", lambda v: isinstance(v, str) and v in ACTORS, "ukendt aktoer")
        for key in ("kind", "what", "why"):
            string(key)
        if strict_kinds:
            require("kind", lambda v: isinstance(v, str) and v in KINDS, "ukendt kind")
        require("inputs", lambda v: isinstance(v, dict) and all(isinstance(x, str) or number(x) for x in v.values()), "forventer objekt med tal/tekst")
    if typ == "event":
        for key in ("id", "why"):
            string(key)
        for key in ("possible", "fired"):
            require(key, lambda v: type(v) is bool, "forventer bool")
        require("probability", lambda v: number(v) and 0 <= v <= 1, "forventer sandsynlighed 0..1")
    if typ == "snapshot":
        for key in ("treasury", "income", "expenses", "army_men", "stability", "districts"):
            if key in record:
                require(key, number, "forventer endeligt tal")
        if "mobilisation" in record:
            require("mobilisation", lambda v: number(v) and 0 <= v <= 3, "forventer 0..3")
        if "relations" in record:
            require("relations", lambda v: isinstance(v, dict) and all(number(x) for x in v.values()), "forventer objekt med tal")
    return errors

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--strict-kinds", action="store_true")
    args = parser.parse_args()
    counts, errors, unknown = Counter(), [], set()
    with args.log.open(encoding="utf-8") as stream:
        for line_no, line in enumerate(stream, 1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
                problems = validate(record, args.strict_kinds)
                if isinstance(record, dict):
                    counts[str(record.get("t"))] += 1
                    if record.get("t") == "decision" and isinstance(record.get("kind"), str) and record["kind"] not in KINDS:
                        unknown.add(record["kind"])
            except (ValueError, TypeError) as exc:
                problems = [str(exc)]
            errors.extend("Linje %d: %s" % (line_no, problem) for problem in problems)
    for error in errors[:30]:
        print(error)
    if unknown:
        print("Advarsel: ukendte kinds: " + ", ".join(sorted(unknown)))
    print("Poster: %s; fejl: %d" % (dict(counts), len(errors)))
    return 1 if errors else 0

if __name__ == "__main__":
    raise SystemExit(main())
