@echo off
rem Runs every RankAndFile.* automation test headless and checks the exported report.
rem Command line follows tools/AGENTS.md ("run_tests.bat") and docs/design.md 9.4.
rem - "Automation RunTests <filter>; Quit": the automation command splits on ';' and runs Quit after the tests
rem   (Engine/Source/Developer/AutomationController/Private/AutomationCommandline.cpp).
rem - -ReportExportPath writes index.json (AutomationControllerManager.cpp, SaveJsonTestPassSummary).
rem Pass: at least one test ran and every test in the report is Success. Zero tests is a failure.
setlocal EnableExtensions
if not defined UE_ROOT goto :NoEngineRoot
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if not exist "%EDITOR%" goto :NoEditor
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "PROJECT=%ROOT%\RankAndFile.uproject"
set "REPORT=%ROOT%\Saved\TestReport"
set "REPORT_JSON=%REPORT%\index.json"
if not exist "%PROJECT%" goto :NoProject
if exist "%REPORT_JSON%" del /q "%REPORT_JSON%"

"%EDITOR%" "%PROJECT%" -ExecCmds="Automation RunTests RankAndFile; Quit" -unattended -nullrhi -nosplash -stdout -ReportExportPath="%REPORT%"
set "EDITOR_EXIT=%ERRORLEVEL%"
echo [run_tests] editor exit code %EDITOR_EXIT%
if not exist "%REPORT_JSON%" goto :NoReport

powershell -NoProfile -ExecutionPolicy Bypass -Command "$r = Get-Content -Raw -Encoding UTF8 -LiteralPath $env:REPORT_JSON | ConvertFrom-Json; $t = @($r.tests); foreach ($x in $t) { Write-Output ('[run_tests] ' + $x.state + ' ' + $x.fullTestPath) }; $ok = @($t | Where-Object { $_.state -eq 'Success' }).Count; Write-Output ('[run_tests] tests ' + $t.Count + ', success ' + $ok + ', failed ' + $r.failed + ', notRun ' + $r.notRun + ', inProcess ' + $r.inProcess); if ($t.Count -eq 0) { Write-Output '[run_tests] FAIL: no test was run'; exit 1 }; if ($ok -ne $t.Count) { Write-Output '[run_tests] FAIL'; exit 1 }; Write-Output '[run_tests] PASS'; exit 0"
set "CHECK=%ERRORLEVEL%"
if not "%CHECK%"=="0" exit /b 1
if not "%EDITOR_EXIT%"=="0" goto :EditorFailed
exit /b 0

:NoEngineRoot
echo [run_tests] UE_ROOT is not set. Set it to the engine folder that contains Engine\Binaries. 1>&2
exit /b 1
:NoEditor
echo [run_tests] UnrealEditor-Cmd.exe not found: "%EDITOR%" 1>&2
exit /b 1
:NoProject
echo [run_tests] project file not found: "%PROJECT%" 1>&2
exit /b 1
:NoReport
echo [run_tests] FAIL: no report at "%REPORT_JSON%". Tests did not run or the editor stopped early. 1>&2
exit /b 1
:EditorFailed
echo [run_tests] FAIL: every test passed but the editor exited with code %EDITOR_EXIT%. 1>&2
exit /b 1
