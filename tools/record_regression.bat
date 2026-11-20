@echo off
rem Runs the RankRegression commandlet (Source/RankEditor/AGENTS.md "RankRegression", tools/AGENTS.md "record_regression.bat").
rem
rem   tools\record_regression.bat -tally       Branch tally and comparison with the current record. Never writes the record file.
rem   tools\record_regression.bat              Writes Source\RankTests\Fixtures\regression_battles_v1.json when it does not exist yet.
rem   tools\record_regression.bat -overwrite   Replaces the record file. Only after a person approved the comparison table.
rem
rem When to write (tools/AGENTS.md): right after RankCore is confirmed as the baseline, or after a person-approved
rem rule, value or bug fix. Never to make a failing test pass.
rem Writing needs a clean working tree under Source and Data\defs. The commit hash comes from git.
rem The commandlet refuses to write when a T1 branch has 0 battles, a T1 branch has no detector,
rem or the record exists without -overwrite.
setlocal EnableExtensions
if not defined UE_ROOT goto :NoEngineRoot
set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if not exist "%EDITOR%" goto :NoEditor
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "PROJECT=%ROOT%\RankAndFile.uproject"
if not exist "%PROJECT%" goto :NoProject

set "MODE_ARGS="
if "%~1"=="-tally" goto :Run
if "%~1"=="" set "MODE_ARGS=-write"
if "%~1"=="-overwrite" set "MODE_ARGS=-write -overwrite"
if not defined MODE_ARGS goto :Usage

where git >nul 2>&1
if errorlevel 1 goto :NoGit
set "DIRTY="
for /f "delims=" %%L in ('git -C "%ROOT%" status --porcelain -- Source Data/defs') do set "DIRTY=1"
if defined DIRTY goto :Dirty
set "COMMIT="
for /f "delims=" %%C in ('git -C "%ROOT%" rev-parse HEAD') do set "COMMIT=%%C"
if not defined COMMIT goto :NoGit
set "MODE_ARGS=%MODE_ARGS% -commit=%COMMIT%"

:Run
"%EDITOR%" "%PROJECT%" -run=RankRegression %MODE_ARGS% -unattended -nullrhi -nosplash -stdout
set "RESULT=%ERRORLEVEL%"
echo [record_regression] commandlet exit code %RESULT%
if not "%RESULT%"=="0" exit /b 1
exit /b 0

:Usage
echo [record_regression] usage: record_regression.bat [-tally ^| -overwrite] 1>&2
exit /b 1
:NoEngineRoot
echo [record_regression] UE_ROOT is not set. Set it to the engine folder that contains Engine\Binaries. 1>&2
exit /b 1
:NoEditor
echo [record_regression] UnrealEditor-Cmd.exe not found: "%EDITOR%" 1>&2
exit /b 1
:NoProject
echo [record_regression] project file not found: "%PROJECT%" 1>&2
exit /b 1
:NoGit
echo [record_regression] git is needed to read the commit hash. 1>&2
exit /b 1
:Dirty
echo [record_regression] Source or Data\defs has uncommitted changes. Commit first so the record names the exact commit. 1>&2
exit /b 1
