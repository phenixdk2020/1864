@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem PROJECT 1864 - sekventielle adfaerdstests. Starter en separat unattended spilproces.
set "UE=I:\Spil\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%~dp0Game1864.uproject"
if not exist "%UE%" (
    echo UnrealEditor.exe findes ikke: "%UE%"
    exit /b 2
)
if not exist "%PROJECT%" (
    echo Projektet findes ikke: "%PROJECT%"
    exit /b 2
)
pushd "%~dp0"
if errorlevel 1 exit /b 2
set "ACCEPT_MODE=-Strategy1864AcceptAll"
for %%A in (%*) do (
    set "ACCEPT_ARGUMENT=%%~A"
    if /i "!ACCEPT_ARGUMENT:~0,20!"=="-Strategy1864Accept=" set "ACCEPT_MODE="
)
"%UE%" "%PROJECT%" /Game/Maps/Strategy1864_Skirmish -game -windowed -nosplash -unattended -Strategy1864FlatQA -Strategy1864DebugDecisions %ACCEPT_MODE% %*
set "ACCEPT_EXIT=%ERRORLEVEL%"
popd
echo Adfaerdstests afsluttet med kode %ACCEPT_EXIT%. Se Saved\Reports\accept_*.md og Saved\Logs.
exit /b %ACCEPT_EXIT%
