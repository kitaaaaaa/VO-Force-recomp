@echo off
rem First run: builds the game from your own disc image (tools\setup.ps1). After that: starts it.
rem On the first run you can drag your .iso onto this file. Later, arguments go to the game.
setlocal
set "ROOT=%~dp0"
set "EXE=%ROOT%out\build\win-amd64-release\force.exe"
if not exist "%ROOT%logs" mkdir "%ROOT%logs"
if exist "%EXE%" (
  start "" "%EXE%" --log_file="%ROOT%logs\force.log" %*
  exit /b 0
)
title Virtual-On Force setup
powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -File "%ROOT%tools\setup.ps1" %1
if errorlevel 1 (
  echo.
  echo Setup did not finish. The details are in logs\setup.log
  pause
  exit /b 1
)
start "" "%EXE%" --log_file="%ROOT%logs\force.log"
