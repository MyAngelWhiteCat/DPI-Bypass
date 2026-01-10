@echo off
taskkill /f /im DPIbypass.exe > nul 2>&1
if errorlevel 1 echo Cant find DPIbypass.exe process
pause