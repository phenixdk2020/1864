# B-271 — Ranged combat calibration notes

**Status:** AKTIV

Denne fil samler de konkrete måle-/tuningbeslutninger for B-271 uden at ændre den overordnede roadmap-rækkefølge.

## Aktuel 09f14 testhypotese

For et velordnet ca. 190-mands kompagni med ca. 58 % af mændene i den aktuelle salve ønskes foreløbigt omtrent:

- CLOSE: ca. 20–30 hits pr. salve som centerområde
- MEDIUM: ca. 10–20 hits
- LONG: ca. 4–10 hits

Dette er QA-mål for gameplay-kalibrering, ikke endelige historiske facitværdier.

## Metode

1. Hold testformation, styrke og forhold så ens som muligt.
2. Log faktisk afstand og resolved hits for hver salve.
3. Kør mindst 10–20 salver pr. band.
4. Beregn gennemsnit samt min/max før værdier ændres igen.
5. Først derefter besluttes næste justering.

## Senere våbenmodel

Den endelige model skal ikke bruge én universel hit-kurve. Våbenprofilen skal senere kunne skelne mellem bl.a. glatløbede musketter og riflede/tapriffel/Minié-lignende våben med egne distance-/accuracy-profiler.
