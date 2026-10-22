@echo off
rem PROJECT 1864 - KLIPVISER: alle bagte animationer i et gitter, hver med navn og laengde. Samme tegning som mandskabet i slaget.
rem Tilfoej et navnefilter, fx:  Start-Klipviser.bat -Strategy1864ClipViewer=Shoulder
rem Kamera: W A S D, hoejre musetast for at kigge, hjulet for at zoome. P pauser (klippene koerer videre).
set UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT=%~dp0Game1864.uproject
if not exist "%PROJECT%" set PROJECT=C:\Projekter\1864-Campaign\Game1864.uproject
start "" "%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -ResX=1920 -ResY=1080 -Strategy1864FlatQA -Strategy1864Skirmish=1 -Strategy1864SkirmishDanes=1 -Strategy1864SkirmishPassive -Strategy1864ClipViewer %*
