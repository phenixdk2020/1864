"""Kopiér officer- og regeringsdata 1825 til kampagnens runtime-data uden at ændre 1851."""
from pathlib import Path
import json

source = Path(__file__).resolve().parent
target = source.parents[1] / "Data" / "Campaign1851"
for name in ("Officers_1825.json", "Ministers_1825.json"):
    text = (source / name).read_text(encoding="utf-8")
    json.loads(text)
    (target / name).write_text(text, encoding="utf-8")
    print(name)
