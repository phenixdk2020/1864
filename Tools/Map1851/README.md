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
