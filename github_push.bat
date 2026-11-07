@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\github_push.ps1" %*
set "publish_exit=%errorlevel%"
if not "%publish_exit%"=="0" echo Upload stopped. Review the error and retained local changes before retrying.
if "%~1"=="" pause
exit /b %publish_exit%
