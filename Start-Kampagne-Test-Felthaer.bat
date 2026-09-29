@echo off
rem PROJECT 1864 - TEST: nyt spil med en faerdig felthaer (2 divisioner og en reserve med brigader),
rem og kampordenen (traeet og diagrammet) aaben. Det gamle autogem bliver overskrevet, naar spillet gemmer.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Campaign1851 -game -windowed -ResX=1920 -ResY=1080 -CampaignNew -CampaignTestFieldArmy -CampaignOpenOOB -CampaignOpenWindow=chart %*
