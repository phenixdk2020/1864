# PROJECT 1864 — Unreal Engine 5.8

Unreal-porten af PROJECT 1864 (C++-modulet `Game1864`). Designmanual og Unity-prototype ligger i `phenixdk2020/Strategy`.

## Kanaler

| Kanal (branch)     | Lokal mappe              | Indhold                                                                  |
|--------------------|--------------------------|--------------------------------------------------------------------------|
| `channel-campaign` | `R:\Onedrive\1864-Campaign` | Det malede kampagnekort "Danmark 1851" (LAEA-projektion), byer, UI   |
| `channel-battle`   | `R:\Onedrive\1864-Battle`   | 3D-slag: kompagnier, soldatfigur, animationer, taktisk kamera        |
| `main`             | –                        | Fælles grundlag: projektfil, config, byggefiler, fælles materialer og MCP-værktøjer |

Hver kanal er et selvstændigt Unreal-projekt, der kan åbnes for sig. Rettelser, der gælder begge kanaler
(config, byggefiler, fælles materialer), laves på `main` og flettes ind i begge kanaler.

## Kom i gang

1. Hent kanalen med PROJECT1864-OneClick-Updater (UECAMPAIGN / UEBATTLE).
2. Dobbeltklik `Game1864.uproject`, og svar **Ja** til at bygge de manglende moduler (kræver Visual Studio 2026 med "Game development with C++").

`Binaries/`, `Intermediate/`, `Saved/` og `DerivedDataCache/` er ikke i Git; de genskabes ved første build.

## Værktøjer

- `Tools/MCP` — HTTP-hjælpere til Unreals ModelContextProtocol-plugin (port 8000).
- `Tools/Campaign` (campaign) og `Tools/Rig` (battle) — Python-scripts, der køres med
  `UnrealEditor-Cmd.exe Game1864.uproject -run=pythonscript -script=<sti>` (brug `/` i stien).
