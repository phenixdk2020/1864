@echo off
rem PROJECT 1864 - TEST 3: to danske kompagnier afventer spillerordrer; svensk fodfolk forsvarer, og en husareskadron rykker frem fra ca. 450 m.
rem Taster: O = kamporden, M = taktisk kort, INDSTILLINGER (oeverst) = flere valg.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=2 -Strategy1864SkirmishCavalry -Strategy1864SkirmishSwedes -Strategy1864EnemyDefends -Strategy1864SkirmishPassive %*
