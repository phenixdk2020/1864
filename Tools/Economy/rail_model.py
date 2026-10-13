"""Regnestykke for jernbanens indtjening (samme formel som ACampaign1851Map::RailwayIncomePerYear).

    python Tools/Economy/rail_model.py [By1 By2 ...]     # standard: fire eksempellinjer
Passagerer: tyngdemodel, rejser pr. aar = G * sqrt(popA * popB), indtaegt = rejser * afstand * billetpris pr. km.
Fragt: 0,15 t pr. indbygger i de to byer, dobbelt hvis en af dem er havneby, indtaegt = tons * afstand * takst.
"""
import json
import math
import os
import sys

G_TRIPS = 0.9            # rejser pr. aar pr. sqrt(popA*popB)
FARE_PER_KM = 0.03       # rd pr. person-km (blandet 1.-3. klasse)
TONS_PER_HEAD = 0.15     # fragtvarer pr. indbygger pr. aar (t)
PORT_FACTOR = 2.0
FREIGHT_RATE = 0.012     # rd pr. ton-km
LINE_FACTOR = 1.15       # banen er laengere end luftlinjen
RD_PER_KM = 12000.0
UPKEEP_PER_KM = 150.0

MAP = os.path.join(os.path.dirname(__file__), "..", "..", "Data", "Campaign1851", "Denmark1851_Map.json")


def km(a, b):
    la1, lo1, la2, lo2 = map(math.radians, (a["lat"], a["lon"], b["lat"], b["lon"]))
    return 6371.0 * math.hypot((lo2 - lo1) * math.cos((la1 + la2) / 2), la2 - la1)


def line_income(towns, ports):
    pop_pairs = []
    total_len = sum(km(towns[i], towns[i + 1]) for i in range(len(towns) - 1)) * LINE_FACTOR
    passengers = 0.0
    freight = 0.0
    for i in range(len(towns)):
        for j in range(i + 1, len(towns)):
            d = km(towns[i], towns[j]) * LINE_FACTOR
            trips = G_TRIPS * math.sqrt(towns[i]["pop"] * towns[j]["pop"])
            passengers += trips * d * FARE_PER_KM
            tons = TONS_PER_HEAD * (towns[i]["pop"] + towns[j]["pop"]) / max(1, len(towns) - 1)
            if towns[i]["name"] in ports or towns[j]["name"] in ports:
                tons *= PORT_FACTOR
            freight += tons * d * FREIGHT_RATE
    return total_len, passengers, freight


def main():
    data = json.load(open(MAP, encoding="utf-8"))
    cities = {c["name"]: c for c in data["cities"]}

    def find(part):
        for n in cities:
            if part.lower() in n.lower().replace("�", "")[:20] or part.lower() in n.lower():
                return n
        return None

    ports = set()   # coastal towns: Ordinary map data has no flag here, so the example uses a short list
    for p in ("Nyborg", "Aarhus", "Aalborg", "Korsør", "Odense", "Fredericia", "Kolding", "Randers", "Helsing", "nhavn"):
        n = find(p)
        if n:
            ports.add(n)
    lines = [("København", "Roskilde"), ("Odense", "Nyborg"), ("Aarhus", "Randers"), ("Fredericia", "Kolding")]
    if len(sys.argv) > 2:
        lines = [tuple(sys.argv[1:3])]
    print("%-24s %5s %10s %10s %10s %10s %10s %12s" % ("linje", "km", "passagerer", "fragt", "i alt/aar", "anlaeg", "drift", "tilbage (aar)"))
    for a, b in lines:
        na, nb = find(a), find(b)
        if not na or not nb:
            print("mangler", a, b)
            continue
        length, pas, fr = line_income([cities[na], cities[nb]], ports)
        gross = pas + fr
        cost = length * RD_PER_KM
        upkeep = length * UPKEEP_PER_KM
        net = gross - upkeep
        payback = cost / net if net > 0 else float("inf")
        print("%-24s %5.0f %10.0f %10.0f %10.0f %10.0f %10.0f %12s" % ((a + "-" + b), length, pas, fr, gross, cost, upkeep, ("%.0f" % payback) if payback != float("inf") else "aldrig"))


if __name__ == "__main__":
    main()
