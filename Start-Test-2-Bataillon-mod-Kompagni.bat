@echo off
rem PROJECT 1864 - TEST 2: en dansk bataljon (4 kompagnier) gaar frem mod et svensk kompagni. Chefen holder et kompagni i reserve, de tre andre
rem gaar frem: midten holder og skyder, de to andre gaar udenom ildfeltet og ind fra siden. Reserven saettes ind, hvis midten bliver haardt ramt.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=R:\Onedrive\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=4 -Strategy1864HoldReserve -Strategy1864SkirmishSwedes -Strategy1864EnemyDefends -Strategy1864SkirmishAttack -Strategy1864FieldLOD=2 %*
