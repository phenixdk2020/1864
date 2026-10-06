# B-281 — HQ UI, selection og bataljonsfront

**Status: AKTIV / v00.00.09f16**

## Problemfund fra v00.00.09f15

- Major mangler mouse-over information.
- Major-selection føles ikke som almindelig unit-selection.
- HQ AI kan ikke toggles.
- Major-panelet er for højt og ligger anderledes end unit-panelet.
- FORSVAR HER kan ende med to kompagnier, der ikke står på én fælles lige front.

## Beslutning

- Major bliver first-class selectable HQ med samme click/hover-konvention som enheder.
- Major AI starter OFF, men kan toggles ON/OFF fra HQ-panelet.
- Kompagni- og HQ-paneler deler samme compact bottom UI-theme og samme vertikale footprint.
- FORSVAR HER bruger én fælles battalion facing og én fælles line direction for alle direkte underenheder.
- Underordnede kompagnier skal ende i Line med samme facing og tilstrækkelig spacing til ikke at overlappe.

## Acceptance

1. Hover på Major viser tooltip.
2. Klik på Major viser HQ selection-ring og command-links.
3. HQ AI kan toggles ON/OFF; default er OFF.
4. Unit- og Major-panel er begge compact bottom panels.
5. FORSVAR HER på åbent terræn giver en lige bataljonsfront.
6. Existing bridge/building routing må ikke regressere.
