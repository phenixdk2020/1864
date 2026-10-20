#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Genererer en syntetisk kampagnelog til Saved/CampaignLogs/test_synthetic.jsonl."""
import json
import os
import random
from datetime import date, timedelta

random.seed(1825)
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "Saved", "CampaignLogs", "test_synthetic.jsonl")
NATIONS = ["DK", "PR", "SE", "NO"]
WAR_DATE = date(1831, 5, 2)
MOB_DATE = date(1831, 3, 10)


def main():
    recs = []
    recs.append({"t": "session", "event": "new", "date": "1825-01-01", "scenario": 1825, "save_version": 39, "slot": ""})
    state = {n: {"tr": random.uniform(150e3, 400e3), "army": random.randint(20000, 60000), "mob": 0}
             for n in NATIONS}
    rel = {a: {b: random.randint(-10, 20) for b in NATIONS if b != a} for a in NATIONS}
    d = date(1825, 1, 1)
    end = date(1832, 12, 31)
    wk = 0
    at_war = False
    while d <= end:
        for n in NATIONS:
            s = state[n]
            inc = 9000 + random.uniform(-800, 800) + (d.year - 1825) * 200
            exp = inc * random.uniform(0.85, 1.12)
            if n == "PR" and MOB_DATE <= d <= WAR_DATE + timedelta(days=200):
                exp *= 1.5
                s["mob"] = 2 if d < WAR_DATE else 3
            elif n == "PR":
                s["mob"] = 0
            if n == "DK" and at_war:
                exp *= 1.3
            s["tr"] += inc - exp
            if n == "PR" and d.year == 1831 and d.month >= 11:
                s["tr"] -= 6000
            # relationer driver
            for o in rel[n]:
                rel[n][o] += random.choice([-1, 0, 0, 1])
            if n == "PR" and MOB_DATE - timedelta(days=120) <= d <= WAR_DATE:
                rel["PR"]["DK"] -= 2
            snap = {"t": "snapshot", "date": d.isoformat(), "nation": n,
                    "treasury": round(s["tr"], 1), "income": round(inc, 1), "expenses": round(exp, 1),
                    "army_men": s["army"] + random.randint(-200, 200) + (15000 if s["mob"] >= 2 else 0),
                    "mobilisation": s["mob"], "relations": dict(rel[n]),
                    "stability": round(random.uniform(1, 3), 2), "districts": 20 + n.__len__()}
            if random.random() < 0.03:  # manglende felter
                snap.pop("stability")
                snap.pop("districts")
            recs.append(snap)
        d += timedelta(days=7)
        wk += 1
        if wk == 150:
            recs.append({"t": "session", "event": "save", "date": d.isoformat(), "scenario": 1825, "save_version": 39, "slot": "auto"})
        if d >= WAR_DATE and not at_war and d < WAR_DATE + timedelta(days=7):
            at_war = True
    recs.append({"t": "session", "event": "load", "date": "1828-06-01", "scenario": 1825, "save_version": 39, "slot": "auto"})

    kinds = ["relation", "recruit", "budget", "tax", "build", "research", "march", "other", "alliance"]
    whats = {"relation": "Forbedrer forhold", "recruit": "Hverver regimenter", "budget": "Justerer budget",
             "tax": "Aendrer skat", "build": "Bygger i provins", "research": "Starter forskning",
             "march": "Flytter haer", "other": "Diverse", "alliance": "Alliance foreslaaet"}
    decs = []
    for _ in range(185):
        n = random.choice(NATIONS)
        actor = "player" if n == "DK" and random.random() < 0.5 else random.choice(["ai", "ai", "minister", "event"])
        k = random.choice(kinds)
        dd = date(1825, 1, 1) + timedelta(days=random.randint(0, 2900))
        o = random.choice([x for x in NATIONS if x != n])
        decs.append({"t": "decision", "date": dd.isoformat(), "nation": n, "actor": actor, "kind": k,
                     "what": "%s (%s)" % (whats[k], o), "why": "Regel: score %.1f over taerskel" % random.uniform(1, 9),
                     "inputs": {"target": o, "score": round(random.uniform(0, 10), 2), "treasury": round(random.uniform(1e5, 4e5))}})
    decs += [
        {"t": "decision", "date": MOB_DATE.isoformat(), "nation": "PR", "actor": "ai", "kind": "mobilise",
         "what": "Mobiliserer til niveau 2", "why": "Spaending mod Danmark om Holsten", "inputs": {"target": "DK", "relation": -48}},
        {"t": "decision", "date": "1831-04-15", "nation": "PR", "actor": "ai", "kind": "mobilise",
         "what": "Mobiliserer til niveau 3", "why": "Krigsforberedelse", "inputs": {"target": "DK"}},
        {"t": "decision", "date": "1829-09-01", "nation": "PR", "actor": "minister", "kind": "mobilise",
         "what": "Delvis mobilisering", "why": "Manoevre", "inputs": {}},
        {"t": "decision", "date": WAR_DATE.isoformat(), "nation": "PR", "actor": "ai", "kind": "war",
         "what": "Krigserklaering mod Danmark", "why": "Casus belli: Holsten", "inputs": {"target": "DK", "relation": -61}},
        {"t": "decision", "date": "1831-05-03", "nation": "DK", "actor": "player", "kind": "mobilise",
         "what": "Mobiliserer", "why": "Svar paa krigserklaering", "inputs": {}},
        {"t": "decision", "date": "1831-11-20", "nation": "DK", "actor": "player", "kind": "peace",
         "what": "Fredsslutning med Preussen", "why": "Kassen er toem", "inputs": {"target": "PR"}},
        {"t": "decision", "date": "1827-06-12", "nation": "DK", "actor": "player", "kind": "alliance",
         "what": "Alliance med Sverige", "why": "Sikring af Oeresund", "inputs": {"target": "SE"}},
        {"t": "decision", "date": "1830-01-05", "nation": "SE", "actor": "ai", "kind": "weird_new_kind",
         "what": "Ukendt slags", "why": "", "inputs": {}},
    ]
    recs += decs
    ev_defs = [("pr_zollverein", "PR", 0.3, True), ("dk_staenderforsamling", "DK", 0.5, True),
               ("se_nordisk_union", "SE", 0.05, False), ("pr_holsten_krise", "PR", 0.25, True)]
    for eid, n, p, can in ev_defs:
        for i in range(random.randint(3, 8)):
            dd = date(1826 + i % 6, 1 + (i * 3) % 12, 1)
            fired = eid in ("pr_holsten_krise",) and i == 4 or eid == "dk_staenderforsamling" and i == 2 or eid == "pr_zollverein" and i == 5
            recs.append({"t": "event", "date": dd.isoformat(), "id": eid, "nation": n, "possible": can,
                         "probability": p, "fired": fired,
                         "why": "Forudsaetninger opfyldt" if fired else ("Terning slog fejl" if can else "Forudsaetning ikke opfyldt")})
    recs.append({"t": "session", "event": "quit", "date": "1832-12-30", "scenario": 1825, "save_version": 39, "slot": ""})

    # beslutninger/events i datoorden efter snapshot-lignende raekkefoelge er ikke noedvendig; sorter alt stabilt
    recs.sort(key=lambda r: (r.get("date", "0"), 0 if r.get("t") == "session" else 1))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        for r in recs:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    print("Skrev %d poster til %s" % (len(recs), OUT))


if __name__ == "__main__":
    main()
