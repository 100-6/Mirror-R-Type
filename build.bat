@echo off
echo Cleaning build directory...
if exist build rmdir /s /q build

echo Configuring CMake...
cmake -S . -B build
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b %errorlevel%
)

echo Building project...
cmake --build build
if %errorlevel% neq 1 (
    echo Build failed!
    exit /b %errorlevel%
)

echo.
echo Build completed successfully!
