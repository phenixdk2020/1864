# map1851 — generator for det malede Danmarkskort 1851

Bygger korttexturerne til `Assets/Campaign1851/Resources/Map1851/` (scene `CampaignMap1851`).

## Krav

- Python 3 med `numpy`, `scipy` og `Pillow`.
- Natural Earth 10m (public domain), udpakket i én mappe:
  - `ne_10m_land`
  - `ne_10m_admin_0_countries`
  - `ne_10m_admin_1_states_provinces`

  Kilde: `https://naciscdn.org/naturalearth/10m/{physical,cultural}/<navn>.zip`

## Kørsel

```bash
python build_map.py <natural-earth-mappe> [--height=4096] [--preview]
```

- `--height=4096` er fuld kvalitet (124 m/px, ca. 3 GB RAM). Brug `--height=2048` når der er lidt hukommelse.
- `--preview` gemmer desuden `preview.png` i en fjerdedel af størrelsen.

## Output

| Fil | Indhold |
|---|---|
| `Denmark1851_Color.png` | Malet kort (sRGB) |
| `Denmark1851_Height.png` | 16-bit højde til terræn-mesh |
| `Denmark1851_Regions.png` | Region-id × 60 (0 hav, 1 Kongeriget, 2 Slesvig, 3 Holsten/Lauenborg, 4 udland) |
| `Denmark1851_DetailMask.png` | Alfa = monarkiets land (maske til markdetaljen) |
| `Fields1851_Detail.png` | Fliselagt markmosaik til tæt zoom |
| `Bornholm1851_Color.png` | Indsat kort |
| `Denmark1851_Map.json` | Udsnit, størrelse i km, byer, stednavne |

Byerne og befolkningstallene ligger i `cities1850.json`. Grænser (Kongeå, Ejder), anakronismer og store skove er konstanter øverst i `build_map.py`.

## QA-billeder fra Unity

```bash
Unity.exe -batchmode -projectPath <projekt> -executeMethod CampaignMap1851Editor.CaptureCli -map1851Shots "ud.png|55.55|10.45|960|0"
```

Format pr. billede: `sti|bredde|længde|afstand-km|drejning`, flere adskilt af `;`.

## Additiv nordudvidelse til niveau B (2026-10-17)

Det aktive Unreal-kort ligger i `Data/Campaign1851`. Udvid det eksisterende kort uden at genberegne danske provinser/ruter:

```powershell
python Tools/Map1851/build_map.py SourceAssets/NaturalEarth --out=Data/Campaign1851 --extend-north
python Tools/Countries/import_sweden_norway.py
python Tools/Countries/import_prussia.py
python Tools/Countries/activate_prussia_events.py
python Tools/Countries/integrate_level_b.py
python Tools/Map1851/validate_neighbours.py
python Tools/Countries/validate_sweden_norway.py
python Tools/Countries/validate_prussia.py
```

Kør kommandoerne enkeltvis. Nordudvidelsen streamer Natural Earth-landformer og undgår admin-1-datasættet. A* benytter kompakte rasterarrays. Udvidelsen er en no-op, hvis metadata allerede har `northExtension`. Landepakkerne skal være tilgængelige på importernes konfigurerede stier.

Det oprindelige raster bliver nederste halvdel, med identiske danske pixels og uændrede by-/amtsindeks. Nye nordlige distrikter og scenarieopdelte hære aktiveres af `integrate_level_b.py`. En almindelig fuld regenerering uden `--extend-north` er en anden operation og kan overskrive udvidelsen; den kræver særskilt kontrol af danske data.

Se [niveau B-rapporten](../../Docs/Niveau-B-Sverige-Norge-Preussen-2026-10-17.md) for skøn, saveversion 38, valideringsoutput og udestående niveau C. Kørsel af generator/validator giver ikke tilladelse til at starte Unreal.
