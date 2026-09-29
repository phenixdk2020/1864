@echo off
rem PROJECT 1864 - starter kampagnekortet som spil (uden editor).
rem Fortsaetter fra autogemmet. Brug Start-Kampagne-NytSpil.bat for at starte forfra.
rem Kan kopieres til skrivebordet: ligger der intet projekt ved siden af filen, bruges den faste sti.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Campaign1851 -game -windowed -ResX=1920 -ResY=1080 %*
