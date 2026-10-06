@echo off
rem Imports the building cards, uniforms and portraits (Reference\Campaign1851\...) into the game.
rem Close Unreal first. See Docs\BILLEDER-TIL-SPILLET.md.
setlocal
set UE="I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJ="%~dp0..\..\Game1864.uproject"
%UE% %PROJ% -run=pythonscript -script="%~dp0import_building_cards.py" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput | findstr /C:"CARD " /C:"CARDS DONE" /C:"Error"
echo.
echo Faerdig. Start spillet.
pause
