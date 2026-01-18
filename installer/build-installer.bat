@echo off
REM R-Type Installer Build Script
REM This script automates the process of creating the Windows installer

echo ========================================
echo R-Type Installer Build Script
echo ========================================
echo.

REM Check if NSIS is installed
where makensis >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] NSIS not found!
    echo Please install NSIS from: https://nsis.sourceforge.io/
    echo And add it to your PATH
    pause
    exit /b 1
)

REM Check if build directory exists
if not exist "..\build" (
    echo [ERROR] Build directory not found!
    echo Please compile the project first using build.bat
    pause
    exit /b 1
)

echo [1/4] Preparing installer files...
powershell -ExecutionPolicy Bypass -File prepare-installer.ps1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to prepare files
    pause
    exit /b 1
)

echo.
echo [2/4] Copying files to build directory...
xcopy /E /Y /I ..\build\installer-package\* ..\build\ >nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to copy files
    pause
    exit /b 1
)

echo [3/4] Building NSIS installer...
makensis rtype-installer.nsi
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to build installer
    pause
    exit /b 1
)

echo.
echo [4/4] Creating portable ZIP archive...
powershell -Command "Compress-Archive -Path '..\build\installer-package\*' -DestinationPath 'RType-Windows-x64.zip' -Force"

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Installer created: installer\RType-Setup.exe
echo Portable ZIP: installer\RType-Windows-x64.zip
echo.
echo You can now distribute these files!
echo.
pause