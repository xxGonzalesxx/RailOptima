@echo off
setlocal
echo ====================================================================
echo  Running RailOptima Automation Tests (Unreal Engine 5)
echo ====================================================================

set UE_CMD="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJECT="c:\Projects\UE\RailOptima\RailOptima.uproject"

if not exist %UE_CMD% (
    echo [ERROR] UnrealEditor-Cmd.exe not found at: %UE_CMD%
    exit /b 1
)

echo Starting test suite: RailOptima...
%UE_CMD% %PROJECT% -ExecCmds="Automation RunTests RailOptima; Quit" -nullrhi -unattended -nopause -ReportExportPath="%~dp0Saved\AutomationReport" -log -abslog="%~dp0Saved\Logs\AutomationTestRun.log"

echo.
echo ====================================================================
echo  Testing completed! Log: Saved/Logs/AutomationTestRun.log
echo ====================================================================
endlocal
