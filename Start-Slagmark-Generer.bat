@echo off
rem PROJECT 1864 - laver et nyt slagmarkskort ved Rendsborg (Saved\Battle\Battlefield_Test.json) og lukker spillet igen.
rem Brug bagefter Start-3D-Slag-Test.bat. Andet sted: aendr lat+lon herunder (fx 55.678+12.57).
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Campaign1851 -game -windowed -ResX=1280 -ResY=720 -CampaignNew -CampaignSeed=42 -CampaignUiShotsQuit "-CampaignUiShots=40:genfield=54.304+9.663,50:clear" %*
