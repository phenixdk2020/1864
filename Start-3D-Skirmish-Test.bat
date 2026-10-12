@echo off
rem PROJECT 1864 - TEST af 3D-slaget i smaa: 2 danske kompagnier mod 2 fjendtlige paa en eng, med batteri, morter og eskadron
rem paa hver side. Fjenden angriber; majoren beordrer angreb. Fjendens skudvidde vises (til test).
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864SkirmishArms -Strategy1864SkirmishAttack -Strategy1864ShowEnemyRange %*
