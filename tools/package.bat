@echo off
rem Builds, cooks and stages the Win64 Shipping package.
rem
rem The RunUAT arguments are copied from a run of the editor's Project Launcher
rem (UE 5.8, 2026-11-20). Profile settings: Configuration=Shipping,
rem Content Scheme=Pak Files, Run off, Deploy off, Archive off.
rem The launcher's command is the "Parsing command line:" line of the UAT log
rem (Engine/Source/Programs/AutomationTool/Program.cs).
rem   - -ScriptsForProject, -noP4, -nocompile and -utf8output are added by the launcher itself
rem     (Engine/Source/Developer/LauncherServices/Private/Launcher/LauncherUATTask.h).
rem   - -CookCultures=ko is the editor's culture when the profile was created
rem     (LauncherProfileBuildCookRun.h, CookedCultures.Add(I18N.GetCurrentCulture()...)).
rem The only changes from the logged command: the project path is taken from this repo
rem and the engine path from %UE_ROOT% (no absolute paths with a user folder), and
rem paths are quoted. No argument was added or removed.
rem
rem Output: Saved\StagedBuilds\Windows\RankAndFile.exe

setlocal EnableExtensions

if not defined UE_ROOT goto :NoEngineRoot
set "RUNUAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%RUNUAT%" goto :NoRunUAT

for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "PROJECT=%ROOT%\RankAndFile.uproject"
set "STAGED=%ROOT%\Saved\StagedBuilds\Windows"
if not exist "%PROJECT%" goto :NoProject

call "%RUNUAT%" -ScriptsForProject="%PROJECT%" -noP4 -nocompile -utf8output BuildCookRun -project="%PROJECT%" -clientconfig=Shipping -serverconfig=Shipping -nocompileeditor -installed -unrealexe="%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -platform=Win64 -build -cook -CookCultures=ko -unversionedcookedcontent -zenstore -pak -stage -compressed
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" goto :UatFailed
if not exist "%STAGED%\RankAndFile.exe" goto :NoOutput

echo [package] Shipping package: "%STAGED%"
exit /b 0

:NoEngineRoot
echo [package] UE_ROOT is not set. Set it to the UE 5.8 install folder. 1>&2
exit /b 1

:NoRunUAT
echo [package] RunUAT.bat not found: "%RUNUAT%" 1>&2
exit /b 1

:NoProject
echo [package] Project file not found: "%PROJECT%" 1>&2
exit /b 1

:UatFailed
echo [package] RunUAT failed with exit code %RESULT%. 1>&2
exit /b %RESULT%

:NoOutput
echo [package] RunUAT succeeded but "%STAGED%\RankAndFile.exe" is missing. 1>&2
exit /b 1
