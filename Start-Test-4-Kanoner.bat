@echo off
rem PROJECT 1864 - TEST 4: en dansk bataljon (4 kompagnier + batteri + morter + eskadron) mod et svensk kompagni med batteri, morter og eskadron.
rem Chefen holder et kompagni i reserve. Her ses kanonerne: batterierne skal vaelge maal, skyde og reagere paa modpartens ild.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=C:\Projekter\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=4 -Strategy1864HoldReserve -Strategy1864SkirmishSwedes -Strategy1864SkirmishArms -Strategy1864EnemyDefends -Strategy1864SkirmishPassive -Strategy1864DebugDecisions %*
