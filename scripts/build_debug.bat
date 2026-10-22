@echo off
setlocal
set "ROOT=%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%I"
if not defined MSBUILD exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0request_app_exit.ps1"
if errorlevel 1 exit /b 1
"%MSBUILD%" "%ROOT%\SimpleCaptureTool.sln" /m /t:Build /p:Configuration=Debug /p:Platform=x64
exit /b %ERRORLEVEL%
