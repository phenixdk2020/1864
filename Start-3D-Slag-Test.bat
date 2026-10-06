@echo off
rem PROJECT 1864 - TEST af 3D-slaget paa et genereret slagmarkskort (Rendsborg). Laver et nyt kort foerst, hvis
rem Saved\Battle\Battlefield_Test.json mangler (kraever at spillet har vaeret startet via Start-Slagmark-Generer.bat).
rem Taster: O = kamporden, M = taktisk kort, INDSTILLINGER (oeverst) = fjendens skudvidde og fjenden angriber/forsvarer.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
if not exist "%~dp0Saved\Battle\Battlefield_Test.json" (
  echo Der er intet slagmarkskort endnu: koer Start-Slagmark-Generer.bat foerst.
  pause
  exit /b 1
)
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Field -game -windowed -ResX=1920 -ResY=1080 -Strategy1864Field=Battlefield_Test.json %*
