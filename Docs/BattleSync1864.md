# Slaget i kampagneprojektet

**Siden 1. oktober 2026 udvikles slaget kun her i Game1864-projektet:**
- koden ligger i `Source/Strategy1864`;
- indholdet ligger i `Content/Units`;
- slagkortene ligger i `Content/Maps/Strategy1864_*`.

Strategy1864-projektet bruges ikke længere. Spejlingen nedenfor er historik. Kør ikke `Sync-Battle.ps1` igen, for det ville overskrive ændringer, der er lavet her.

Før skiftet blev slaget udviklet i **Strategy1864** (`R:\Onedrive\Dokumenter\Unreal Projects\Strategy1864`, GitHub `phenixdk2020/Strategy`, grenen `unreal-port`). Kampagneprojektet får en kopi af det, så begge dele kører i samme spil.

## Hvad kopieres
| Fra Strategy1864 | Til 1864-Campaign | Hvordan |
|---|---|---|
| `Unreal/Source/Strategy1864` (modulet) | `Source/Strategy1864` | Spejles fra den seneste commit, eller med `-WorkingTree` fra arbejdsmappen |
| `Unreal/Content/Units` (soldater, våben, animationer, skeletter) | `Content/Units` | Spejles |
| `Unreal/Content/Maps/Strategy1864_*.umap` (slagkortene) | `Content/Maps` | Kopieres. Kampagnens kort røres ikke |
| `+ActionMappings` og `+AxisMappings` i `DefaultInput.ini` | `Config/DefaultInput.ini` | Mellem to markeringer |

`Tools/Battle/LastSync.txt` viser, hvornår der sidst blev spejlet, og fra hvilken commit.

## Sådan spejler man
Kør:

```
powershell -File Tools\Battle\Sync-Battle.ps1
```

Med `-WorkingTree` kopieres også det, der endnu ikke er committet i Strategy1864.

Bagefter skal projektet bygges og ændringen committes i 1864-Campaign.

## Engangsændringer i kampagneprojektet
- **Modul:** `Game1864.uproject` og begge `Target.cs`-filer har modulet `Strategy1864` med. Modulet beholder sit navn, så alle henvisninger i slagets filer (`/Script/Strategy1864…`) fortsat virker.
- **Spiltilstand:** i `DefaultEngine.ini` får kort, hvis navn starter med `Strategy1864_`, slagets egen spiltilstand (`StrategyGameMode`). Kampagnen beholder `Campaign1851GameMode`.

## Ændring i Strategy1864 (commit 66c231c)
`StrategyQARuntimeSubsystem` starter nu kun på kort, der hedder `Strategy1864…`. Ellers ville kampagnekortet få testslaget og slagets kamera.

## Test
Slagkortet startes i kampagneprojektet sådan:

```
UnrealEditor.exe Game1864.uproject /Game/Maps/Strategy1864_QA -game
```

## Næste skridt
- **Fra kampagnen til slaget:** åbne slagkortet med det aktuelle slag, ud fra `BattleRequest_N.json`, `Units.json` og `Battlefield_N.json`. Strategy1864 har allerede typer til en slagmark fra kampagnen (`StrategyBattlefieldGenerationTypes.h`).
- **Tilbage igen:** skrive `BattleResult_N.json` og vende tilbage til kampagnekortet.
- **På sigt:** flytte udviklingen helt over (model A), når slaget er færdigt nok.
