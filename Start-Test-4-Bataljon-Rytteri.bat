@echo off
rem PROJECT 1864 - TEST 4: en dansk bataljon (4 kompagnier) og en eskadron husarer angriber et svensk kompagni. Majoren vurderer styrkeforholdet:
rem ved overtal holdes et kompagni i reserve, ellers saettes alle ind. Ildbasen fastholder, flankerne gaar udenom, husarerne venter bag den aabne flanke
rem og chokerer i flanken paa det fastholdte kompagni - aldrig mod en fast karre - og samler sig bagefter.
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=C:\Projekter\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=4 -Strategy1864SkirmishDanishCavalry -Strategy1864SkirmishSwedes -Strategy1864EnemyDefends -Strategy1864SkirmishAttack %*
