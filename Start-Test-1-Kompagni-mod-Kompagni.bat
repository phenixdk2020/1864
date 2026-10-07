@echo off
rem PROJECT 1864 - TEST 1: et dansk kompagni gaar frem og skyder mod et svensk kompagni, som holder sin stilling og skyder tilbage, til det ene brydes.
rem Taster: O = kamporden, M = taktisk kort, INDSTILLINGER (oeverst) = flere valg.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=1 -Strategy1864SkirmishSwedes -Strategy1864EnemyDefends -Strategy1864SkirmishAttack -Strategy1864FieldLOD=2 %*
