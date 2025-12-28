@echo off
title WinDivert Installer for DPIbypass
echo ============================================
echo    WinDivert Driver Installation
echo ============================================
echo.

:: Get script directory
set "SCRIPT_DIR=%~dp0"
echo Script directory: %SCRIPT_DIR%

:: Check for administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] Please run as Administrator!
    echo.
    echo How to run as admin:
    echo 1. Right-click windivert_install.bat
    echo 2. Select "Run as administrator"
    echo.
    echo OR in PowerShell admin:
    echo cd "%SCRIPT_DIR%"
    echo .\windivert_install.bat
    pause
    exit /b 1
)

echo [1/5] Checking for WinDivert64.sys...
if not exist "%SCRIPT_DIR%WinDivert64.sys" (
    echo [ERROR] WinDivert64.sys not found!
    echo Looking in: %SCRIPT_DIR%
    echo.
    echo Files in directory:
    dir "%SCRIPT_DIR%*.sys"
    echo.
    pause
    exit /b 1
)
echo    Found: %SCRIPT_DIR%WinDivert64.sys

echo [2/5] Copying driver to system folder...
copy "%SCRIPT_DIR%WinDivert64.sys" "%SystemRoot%\System32\drivers\WinDivert.sys" /Y
if %errorLevel% neq 0 (
    echo [ERROR] Failed to copy driver!
    pause
    exit /b 1
)
echo    File copied to System32\drivers\WinDivert.sys

echo [3/5] Registering WinDivert service...
sc query WinDivert1 >nul 2>&1
if %errorLevel% equ 0 (
    echo    Service already exists, stopping...
    sc stop WinDivert1 >nul 2>&1
    sc delete WinDivert1 >nul 2>&1
    timeout /t 2 /nobreak >nul
)

sc create WinDivert1 type= kernel start= demand binPath= "System32\drivers\WinDivert.sys" DisplayName= "WinDivert Driver"
if %errorLevel% neq 0 (
    echo [ERROR] Failed to create service!
    pause
    exit /b 1
)
echo    Service successfully registered

echo [4/5] Starting service...
sc start WinDivert1
if %errorLevel% neq 0 (
    echo [WARNING] Service not started, but will start automatically when needed
) else (
    echo    Service successfully started
)

echo [5/5] Verifying installation...
sc query WinDivert1 >nul
if %errorLevel% equ 0 (
    echo ============================================
    echo    INSTALLATION COMPLETED SUCCESSFULLY!
    echo ============================================
    echo.
    echo You can now run DPIbypass.exe
    echo Remember to run it as Administrator!
    
    :: Also copy DLL if building from source
    if exist "%SCRIPT_DIR%WinDivert.dll" (
        echo.
        echo DLL files are present for compilation
    )
) else (
    echo ============================================
    echo    [WARNING] Installation issues detected
    echo ============================================
    echo Try rebooting your computer
)

echo.
echo Press any key to exit...
pause >nul