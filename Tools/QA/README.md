# Video-QA af 3D-slagene

`battle_video_qa.py` optager en scriptet testkamp og klipper den op i billeder. Tre bedømmere gennemgår billederne:

- **Ollama** (lokal, `qwen3-vl:8b-instruct`): alle nøglebilleder, striber og nærbilleder.
- **Codex** (`codex exec -s read-only`): højst 8 af de mest mistænkelige billeder plus spillets målinger. Peger på den sandsynlige årsag i koden.
- **Copilot** (`copilot -p`, shell og skrivning nægtet): de samme 8 billeder plus et referencebillede fra `Reference/Drill`. Vurderer realismen mod reglementet fra 1863.

Fundene flettes i `Saved/QA/<dato>-<scenarie>/rapport.md`:

- **Bekræftet:** set af mindst to bedømmere eller støttet af målingerne.
- **Mulig:** set af kun én bedømmer.
- **Kun i målingerne:** markeret af målingerne, men ingen bedømmer så det på billederne.

Rapporten retter intet. Hvert bekræftet fund kontrolleres på billederne og i koden, før der rettes.

## Kørsel

```
python Tools/QA/battle_video_qa.py list
python Tools/QA/battle_video_qa.py all tab-i-kolonne
python Tools/QA/battle_video_qa.py review --run Saved/QA/2026-10-28-tab-i-kolonne --skip codex,copilot
python Tools/QA/battle_video_qa.py report --run Saved/QA/2026-10-28-tab-i-kolonne
python Tools/QA/battle_video_qa.py import --frames Saved/Recording --log Saved/Logs/<log> --fps 10 --start 6 --last 381 --name gammel
```

Trinnene hedder `capture`, `metrics`, `extract`, `review` og `report`. `all` kører dem alle, og `import` laver en kørsel ud fra en ældre optagelse.

- **capture** starter spillet med `-benchmark -fps=N`, scenariets `-Strategy1864DrillTest`, `-Strategy1864FollowCam` og `-Strategy1864Record`. Det slår også `-Strategy1864FormationMetric`, `-Strategy1864NoHud`, `-Strategy1864ShotsQuit` og `-ABSLOG` til kørselsmappen til. Bagefter flyttes billederne fra denne kørsel ud af `Saved/Recording`.
- **metrics** læser loggens `Strategy1864FormationMetric`- og `Closest`-linjer og markerer:
  - mindste naboafstand under 40 cm;
  - færre end 80 % på plads uden omformning;
  - fanebæreren under 40 cm eller over 4,5 m fra nærmeste mand.
- **extract** laver `video.mp4`, `keys/` (ét billede i sekundet), `strips/` (6 billeder med 0,2 s mellemrum, 3×2), `closeups/` (midten forstørret) og `kontaktark.jpg`.
- **review** og **report** er beskrevet ovenfor. Bedømmernes svar ligger i `reviews/`.

## Regler

- Scriptet nægter at starte, hvis UnrealEditor eller spillet allerede kører, eller hvis GPU'en bruger over 6 GB, fx en Pinokio-app. Det lukker aldrig dit spil. `--force` tilsidesætter kun GPU-tjekket.
- Ét GPU-tungt job ad gangen: Ollama kører først, når optagelsen er færdig, og frigiver VRAM bagefter (`keep_alive=0`).
- Codex og Copilot bruger kvote, så de får højst 8 billeder pr. kørsel. Hvis en bedømmer fejler, står det i rapporten, og kæden fortsætter.
- Nye scenarier tilføjes i `scenarios.json`. Tjeklisten er `battle_rubric.md`, og svarformatet er `review_schema.json`.
