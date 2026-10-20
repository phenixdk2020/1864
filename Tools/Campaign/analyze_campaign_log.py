#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Analyse af kampagnelog (JSON lines) -> selvstaendig HTML-rapport paa dansk.

Brug: python Tools/Campaign/analyze_campaign_log.py <log.jsonl> [--out DIR]

Kun Python 3 standardbibliotek + matplotlib. Tolerant over for manglende/ekstra felter,
tom fil, kun en nation, afkortet sidste linje og ukendte 'kind'-vaerdier.
"""
import argparse
import base64
import html
import io
import json
import os
import sys
from collections import Counter, defaultdict
from datetime import date

NATION_NAMES = {
    "DK": "Danmark", "PR": "Preussen", "SE": "Sverige", "NO": "Norge", "RU": "Rusland",
    "GB": "Storbritannien", "UK": "Storbritannien", "FR": "Frankrig", "AT": "Østrig",
    "HA": "Hannover", "OL": "Oldenborg", "MK": "Mecklenburg",
}


def nname(c):
    return NATION_NAMES.get(c, str(c))


def parse_date(s):
    try:
        y, m, d = str(s)[:10].split("-")
        return date(int(y), int(m), int(d))
    except Exception:
        return None


def num(v):
    if isinstance(v, bool):
        return None
    if isinstance(v, (int, float)):
        return float(v)
    try:
        return float(v)
    except Exception:
        return None


def load(path):
    recs, bad = [], 0
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    o = json.loads(line)
                    if isinstance(o, dict):
                        recs.append(o)
                    else:
                        bad += 1
                except Exception:
                    bad += 1  # fx afkortet sidste linje
    except OSError as e:
        print("Kan ikke laese %s: %s" % (path, e), file=sys.stderr)
    return recs, bad


def fig_b64(fig):
    import matplotlib.pyplot as plt
    buf = io.BytesIO()
    fig.savefig(buf, format="png", dpi=90, bbox_inches="tight")
    plt.close(fig)
    return base64.b64encode(buf.getvalue()).decode("ascii")


def line_chart(title, series, ylabel="", hlines=()):
    """series: {label: [(date, value)]}. Returnerer <img>-tag eller ''."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    series = {k: v for k, v in series.items() if v}
    if not series:
        return ""
    fig, ax = plt.subplots(figsize=(7.5, 2.9))
    for lab, pts in series.items():
        pts = sorted(pts)
        ax.plot([p[0] for p in pts], [p[1] for p in pts], label=lab, linewidth=1.4)
    for y in hlines:
        ax.axhline(y, color="grey", linestyle="--", linewidth=0.8)
    ax.set_title(title, fontsize=10)
    if ylabel:
        ax.set_ylabel(ylabel, fontsize=8)
    ax.grid(alpha=0.3)
    ax.tick_params(labelsize=8)
    if len(series) > 1 or list(series)[0] not in ("",):
        ax.legend(fontsize=7, ncol=3)
    fig.autofmt_xdate()
    return '<img alt="%s" src="data:image/png;base64,%s">' % (html.escape(title), fig_b64(fig))


def compact(v, maxlen=140):
    if isinstance(v, dict):
        s = ", ".join("%s=%s" % (k, compact(x, 40)) for k, x in v.items())
    elif isinstance(v, float):
        s = ("%.2f" % v).rstrip("0").rstrip(".")
    else:
        s = str(v)
    return s if len(s) <= maxlen else s[:maxlen - 1] + "…"


def esc(s):
    return html.escape(str(s if s is not None else ""))


def fmt_rd(v):
    return ("{:,.0f}".format(v)).replace(",", ".") + " rd."


def analyse(recs):
    A = {
        "sessions": [], "decisions": [], "events": [], "snaps": defaultdict(list),
        "scenarios": set(), "nations": set(), "dates": [],
    }
    for r in recs:
        t = r.get("t")
        d = parse_date(r.get("date"))
        if d:
            A["dates"].append(d)
        if t == "session":
            A["sessions"].append(r)
            if r.get("scenario") is not None:
                A["scenarios"].add(r.get("scenario"))
        elif t == "decision":
            r["_d"] = d
            A["decisions"].append(r)
            if r.get("nation"):
                A["nations"].add(r["nation"])
        elif t == "event":
            r["_d"] = d
            A["events"].append(r)
            if r.get("nation"):
                A["nations"].add(r["nation"])
        elif t == "snapshot":
            r["_d"] = d
            n = r.get("nation")
            if n:
                A["nations"].add(n)
                A["snaps"][n].append(r)
    return A


def turning_points(A):
    tp = []
    for dc in A["decisions"]:
        k = dc.get("kind")
        if k in ("war", "peace", "alliance"):
            tp.append((dc["_d"], dc.get("nation", "?"), k, dc.get("what") or "", dc.get("why") or ""))
    # relationskrydsninger
    for n, snaps in A["snaps"].items():
        prev = {}
        for s in sorted((s for s in snaps if s["_d"]), key=lambda s: s["_d"]):
            rel = s.get("relations")
            if not isinstance(rel, dict):
                continue
            for o, v in rel.items():
                v = num(v)
                if v is None:
                    continue
                p = prev.get(o)
                if p is not None:
                    for th in (-50, 50):
                        if (p < th <= v) or (p >= th > v):
                            retning = "under" if v < p else "over"
                            if th == 50 and v < p:
                                retning = "under"
                            tp.append((s["_d"], n, "relation",
                                       "Relation %s→%s krydsede %+d (%s) : %.0f → %.0f"
                                       % (n, o, th, retning, p, v), ""))
                prev[o] = v
    tp.sort(key=lambda x: (x[0] or date.min))
    return tp


def nation_summary(n, A):
    out = []
    decs = [d for d in A["decisions"] if d.get("nation") == n]
    nm = nname(n)
    mob = [d for d in decs if d.get("kind") == "mobilise"]
    if mob:
        out.append("%s mobiliserede %d gang%s" % (nm, len(mob), "e" if len(mob) != 1 else ""))
    snaps = sorted((s for s in A["snaps"].get(n, []) if s["_d"]), key=lambda s: s["_d"])

    def rel_at(d, other):
        best = None
        for s in snaps:
            if s["_d"] > d:
                break
            rel = s.get("relations")
            if isinstance(rel, dict) and other in rel:
                best = num(rel[other])
        return best

    for w in [d for d in decs if d.get("kind") == "war"]:
        extra = ""
        inp = w.get("inputs") if isinstance(w.get("inputs"), dict) else {}
        rv = None
        for key in ("relation", "rel", "relation_value"):
            if key in inp and num(inp[key]) is not None:
                rv = num(inp[key])
        if rv is None and w["_d"]:
            tgt = inp.get("target") or inp.get("enemy") or inp.get("against")
            if tgt:
                rv = rel_at(w["_d"], tgt)
        if rv is not None:
            extra = " ved relation %s%d" % ("−" if rv < 0 else "", abs(round(rv)))
        out.append("krig erklæret %s%s" % (w["_d"].isoformat() if w["_d"] else "?", extra))
    peaces = [d for d in decs if d.get("kind") == "peace"]
    if peaces:
        out.append("fred sluttet %s" % ", ".join(p["_d"].isoformat() if p["_d"] else "?" for p in peaces))
    tre = [(num(s.get("treasury")), s["_d"]) for s in snaps if num(s.get("treasury")) is not None]
    if tre:
        lo = min(tre, key=lambda x: x[0])
        hi = max(tre, key=lambda x: x[0])
        out.append("laveste kasse %s (%s)" % (lo[1].strftime("%Y-%m"), fmt_rd(lo[0])))
        out.append("højeste kasse %s (%s)" % (hi[1].strftime("%Y-%m"), fmt_rd(hi[0])))
    arm = [(num(s.get("army_men")), s["_d"]) for s in snaps if num(s.get("army_men")) is not None]
    if arm:
        out.append("hær %s → %s mand" % ("{:,.0f}".format(arm[0][0]).replace(",", "."),
                                          "{:,.0f}".format(arm[-1][0]).replace(",", ".")))
    pl = [d for d in decs if d.get("actor") == "player"]
    if pl:
        kinds = Counter(d.get("kind", "other") for d in pl)
        out.append("spilleren traf %d beslutning%s for nationen (%s)" % (
            len(pl), "er" if len(pl) != 1 else "",
            ", ".join("%s×%d" % (k, c) for k, c in kinds.most_common())))
    if not out:
        return "Ingen data nok til et resumé."
    s = "; ".join(out)
    return s[0].upper() + s[1:] + "."


def nation_charts(n, A):
    snaps = [s for s in A["snaps"].get(n, []) if s["_d"]]
    def ser(key):
        return [(s["_d"], num(s.get(key))) for s in snaps if num(s.get(key)) is not None]
    imgs = []
    imgs.append(line_chart("Kasse (rd.)", {"Kasse": ser("treasury")}))
    imgs.append(line_chart("Indtægter og udgifter (rd.)",
                           {"Indtægt": ser("income"), "Udgift": ser("expenses")}))
    imgs.append(line_chart("Hærstyrke (mand)", {"Hær": ser("army_men")}))
    imgs.append(line_chart("Mobilisering (0-3)", {"Mobilisering": ser("mobilisation")}))
    rel = defaultdict(list)
    for s in snaps:
        r = s.get("relations")
        if isinstance(r, dict):
            for o, v in r.items():
                if num(v) is not None:
                    rel[nname(o)].append((s["_d"], num(v)))
    imgs.append(line_chart("Relationer til andre nationer", rel, hlines=(-50, 0, 50)))
    return "".join('<div class="ch">%s</div>' % i for i in imgs if i)


CSS = """
body{font-family:Segoe UI,Arial,sans-serif;margin:20px auto;max-width:1150px;color:#222;background:#fafafa}
h1{margin-bottom:4px}h2{border-bottom:2px solid #8a6d3b;padding-bottom:3px;margin-top:34px}
table{border-collapse:collapse;width:100%;font-size:13px;background:#fff}
th,td{border:1px solid #ccc;padding:3px 6px;vertical-align:top;text-align:left}
th{background:#eee}.ch{display:inline-block;margin:4px;vertical-align:top}
.meta td:first-child{font-weight:bold;width:160px}.sum{background:#fff7e0;padding:8px;border-left:4px solid #8a6d3b}
.filters label{margin-right:14px}.small{color:#666;font-size:12px}
.tag-player{background:#d6ecff}.never{color:#a00}
"""

JS = """
function applyFilter(){
 var n=document.getElementById('fN').value,a=document.getElementById('fA').value,k=document.getElementById('fK').value;
 var rows=document.querySelectorAll('#dec tbody tr'),c=0;
 for(var i=0;i<rows.length;i++){var r=rows[i];
  var ok=(!n||r.dataset.n===n)&&(!a||r.dataset.a===a)&&(!k||r.dataset.k===k);
  r.style.display=ok?'':'none';if(ok)c++;}
 document.getElementById('cnt').textContent=c+' af '+rows.length+' beslutninger';
}
window.addEventListener('load',applyFilter);
"""


def options(vals):
    return '<option value="">(alle)</option>' + "".join(
        '<option value="%s">%s</option>' % (esc(v), esc(v)) for v in sorted(vals))


def build_html(path, recs, bad):
    A = analyse(recs)
    h = []
    name = os.path.basename(path)
    h.append('<!doctype html><html lang="da"><head><meta charset="utf-8">'
             '<title>Kampagnerapport – %s</title><style>%s</style></head><body>' % (esc(name), CSS))
    h.append("<h1>Kampagnerapport</h1><div class='small'>Kilde: %s &middot; %d poster læst%s</div>" % (
        esc(name), len(recs), (", %d ulæselige linjer sprunget over (evt. afkortet slutning)" % bad) if bad else ""))
    if not recs:
        h.append("<p><b>Loggen er tom eller kunne ikke læses.</b></p></body></html>")
        return "".join(h)

    # 1. Header
    ds = sorted(A["dates"])
    rng = "%s – %s" % (ds[0].isoformat(), ds[-1].isoformat()) if ds else "ukendt"
    sc = ", ".join(str(s) for s in sorted(A["scenarios"], key=str)) or "ukendt"
    h.append("<h2>Overblik</h2><table class='meta'>")
    h.append("<tr><td>Scenarie</td><td>%s</td></tr>" % esc(sc))
    h.append("<tr><td>Datointerval</td><td>%s</td></tr>" % esc(rng))
    h.append("<tr><td>Nationer</td><td>%s</td></tr>" % esc(", ".join(
        "%s (%s)" % (nname(n), n) for n in sorted(A["nations"])) or "ingen"))
    h.append("<tr><td>Poster</td><td>%d beslutninger, %d begivenhedsposter, %d snapshots</td></tr>" % (
        len(A["decisions"]), len(A["events"]), sum(len(v) for v in A["snaps"].values())))
    h.append("</table>")
    if A["sessions"]:
        lab = {"new": "Nyt spil", "load": "Indlæst", "save": "Gemt", "quit": "Afsluttet"}
        h.append("<h3>Sessioner</h3><table><thead><tr><th>Dato</th><th>Hændelse</th><th>Scenarie</th>"
                 "<th>Save-version</th><th>Slot</th></tr></thead><tbody>")
        for s in A["sessions"]:
            h.append("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>" % (
                esc(s.get("date", "")), esc(lab.get(s.get("event"), s.get("event", ""))),
                esc(s.get("scenario", "")), esc(s.get("save_version", "")), esc(s.get("slot", ""))))
        h.append("</tbody></table>")

    # 2. Nationer
    h.append("<h2>Nationer</h2>")
    for n in sorted(A["nations"]):
        h.append("<h3>%s (%s)</h3>" % (esc(nname(n)), esc(n)))
        h.append("<p class='sum'>%s</p>" % esc(nation_summary(n, A)))
        try:
            ch = nation_charts(n, A)
        except Exception as e:  # diagrammer maa aldrig vaelte rapporten
            ch = "<p class='small'>Diagrammer kunne ikke laves: %s</p>" % esc(e)
        h.append(ch or "<p class='small'>Ingen snapshots for denne nation.</p>")

    # 3. Tidslinje
    decs = sorted(A["decisions"], key=lambda d: d["_d"] or date.min)
    h.append("<h2>Beslutningstidslinje</h2>")
    h.append("<div class='filters'><label>Nation <select id='fN' onchange='applyFilter()'>%s</select></label>"
             "<label>Aktør <select id='fA' onchange='applyFilter()'>%s</select></label>"
             "<label>Slags <select id='fK' onchange='applyFilter()'>%s</select></label>"
             "<span id='cnt' class='small'></span></div>" % (
                 options({d.get("nation", "") for d in decs} - {""}),
                 options({d.get("actor", "") for d in decs} - {""}),
                 options({d.get("kind", "") for d in decs} - {""})))
    h.append("<table id='dec'><thead><tr><th>Dato</th><th>Nation</th><th>Aktør</th><th>Slags</th>"
             "<th>Hvad</th><th>Hvorfor</th><th>Input</th></tr></thead><tbody>")
    for d in decs:
        h.append("<tr data-n='%s' data-a='%s' data-k='%s'%s><td>%s</td><td>%s</td><td>%s</td><td>%s</td>"
                 "<td>%s</td><td>%s</td><td class='small'>%s</td></tr>" % (
                     esc(d.get("nation", "")), esc(d.get("actor", "")), esc(d.get("kind", "")),
                     " class='tag-player'" if d.get("actor") == "player" else "",
                     esc(d.get("date", "")), esc(d.get("nation", "")), esc(d.get("actor", "")),
                     esc(d.get("kind", "")), esc(d.get("what", "")), esc(d.get("why", "")),
                     esc(compact(d.get("inputs", "")) if d.get("inputs") else "")))
    h.append("</tbody></table>")

    # 4. Begivenheder
    h.append("<h2>Begivenheder</h2>")
    ev = defaultdict(lambda: {"poss": 0, "fired": 0, "probs": [], "why": Counter(), "nations": set(), "n": 0})
    for e in A["events"]:
        x = ev[e.get("id", "?")]
        x["n"] += 1
        if e.get("possible"):
            x["poss"] += 1
        if e.get("fired"):
            x["fired"] += 1
        p = num(e.get("probability"))
        if p is not None:
            x["probs"].append(p)
        if e.get("why"):
            x["why"][str(e["why"])] += 1
        if e.get("nation"):
            x["nations"].add(e["nation"])
    if ev:
        h.append("<table><thead><tr><th>Id</th><th>Nation</th><th>Mulig</th><th>Udløst</th>"
                 "<th>Sandsynlighed</th><th>Begrundelser</th></tr></thead><tbody>")
        for i, x in sorted(ev.items()):
            pr = ("%.0f%%" % (min(x["probs"]) * 100) if min(x["probs"]) == max(x["probs"])
                  else "%.0f–%.0f%%" % (min(x["probs"]) * 100, max(x["probs"]) * 100)) if x["probs"] else "–"
            why = "; ".join("%s (×%d)" % (w, c) for w, c in x["why"].most_common(4))
            h.append("<tr><td>%s</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td><td>%s</td></tr>" % (
                esc(i), esc(", ".join(sorted(x["nations"]))), x["poss"], x["fired"], pr, esc(why)))
        h.append("</tbody></table>")
        never = sorted(i for i, x in ev.items() if x["fired"] == 0)
        h.append("<h3>Begivenheder der aldrig blev udløst</h3>")
        h.append("<ul>%s</ul>" % "".join("<li class='never'>%s</li>" % esc(i) for i in never)
                 if never else "<p>Alle loggede begivenheder blev udløst mindst én gang.</p>")
    else:
        h.append("<p>Ingen begivenhedsposter i loggen.</p>")

    # 5. Vendepunkter
    h.append("<h2>Vendepunkter</h2>")
    tps = turning_points(A)
    if tps:
        lab = {"war": "Krig", "peace": "Fred", "alliance": "Alliance", "relation": "Relation"}
        h.append("<table><thead><tr><th>Dato</th><th>Nation</th><th>Type</th><th>Beskrivelse</th></tr></thead><tbody>")
        for d, n, k, w, y in tps:
            h.append("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s%s</td></tr>" % (
                esc(d.isoformat() if d else "?"), esc(n), esc(lab.get(k, k)), esc(w),
                (" <span class='small'>– %s</span>" % esc(y)) if y else ""))
        h.append("</tbody></table>")
    else:
        h.append("<p>Ingen krige, fred, alliancer eller relationskrydsninger fundet.</p>")

    h.append("<script>%s</script></body></html>" % JS)
    return "".join(h)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Analyser kampagnelog -> HTML-rapport")
    ap.add_argument("log")
    ap.add_argument("--out", help="Outputmappe (standard: ved siden af loggen)")
    a = ap.parse_args(argv)
    recs, bad = load(a.log)
    out_dir = a.out or os.path.dirname(os.path.abspath(a.log))
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.splitext(os.path.basename(a.log))[0]
    out = os.path.join(out_dir, base + "_report.html")
    with open(out, "w", encoding="utf-8") as f:
        f.write(build_html(a.log, recs, bad))
    print("Rapport skrevet: %s" % out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
