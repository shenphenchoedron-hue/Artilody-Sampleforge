@echo off
rem Double-click to build Artilody Sampleforge on Windows.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Windows.ps1"
if errorlevel 1 (
  echo.
  echo Build failed / Build fejlede. Copy the error message above.
)
pause
