@echo off
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_QA -game -windowed -ResX=1280 -ResY=720 -Strategy1864Duel -Strategy1864DuelCamera=2500 -Strategy1864DuelFocus=0 -Strategy1864DuelPitch=-35 %*
