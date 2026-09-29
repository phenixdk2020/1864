@echo off
rem PROJECT 1864 - starter kampagnekortet som et nyt spil (1. juli 1851, fuld kasse).
rem Det gamle autogem bliver overskrevet, naar spillet gemmer.
rem Kan kopieres til skrivebordet: ligger der intet projekt ved siden af filen, bruges den faste sti.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Campaign1851 -game -windowed -ResX=1920 -ResY=1080 -CampaignNew %*
