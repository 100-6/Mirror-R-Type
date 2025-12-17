@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build
set BUILD_TESTS=OFF
set VCPKG_DIR=vcpkg
set VCPKG_COMMIT=bd52fac7114fdaa2208de8dd1227212a6683e562

set CMD=%1
if "%CMD%"=="" set CMD=rebuild
if "%CMD%"=="help" goto :usage
if "%CMD%"=="-h" goto :usage
if "%CMD%"=="--help" goto :usage

if "%1"=="test" (
    set BUILD_TESTS=ON
    echo Tests enabled
)
if "%2"=="test" (
    set BUILD_TESTS=ON
    echo Tests enabled
)

if "%CMD%"=="clean" goto :clean
if "%CMD%"=="fclean" goto :fclean
if "%CMD%"=="re" goto :re
if "%CMD%"=="make" goto :make
if "%CMD%"=="rebuild" goto :rebuild
goto :rebuild

:usage
echo Usage: %0 [COMMAND] [OPTIONS]
echo.
echo Commands:
echo   (none)       - Full rebuild (clean + configure + build)
echo   make         - Incremental build (only recompile changed files)
echo   clean        - Remove build artifacts (keeps build directory)
echo   fclean       - Remove entire build directory
echo   re           - Alias for fclean + full rebuild
echo.
echo Options:
echo   test         - Enable tests compilation
echo.
echo Examples:
echo   build.bat                 # Full rebuild
echo   build.bat make            # Incremental build
echo   build.bat make test       # Incremental build with tests
echo   build.bat clean           # Clean artifacts
echo   build.bat fclean          # Remove build directory
echo   build.bat re              # Full clean + rebuild
exit /b 0

:clean
echo Cleaning build artifacts in %BUILD_DIR%...
if exist "%BUILD_DIR%" (
    cmake --build "%BUILD_DIR%" --target clean 2>nul || rmdir /s /q "%BUILD_DIR%\*" 2>nul
    echo ✓ Build artifacts cleaned
) else (
    echo Nothing to clean (build directory doesn't exist)
)
exit /b 0

:fclean
echo Removing entire build directory: %BUILD_DIR%
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo ✓ Build directory removed
) else (
    echo Build directory doesn't exist
)
exit /b 0

:re
echo Full rebuild: fclean + configure + build
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
goto :rebuild

:make
echo Incremental build...
call :setup_vcpkg
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Build directory not found. Running full configuration first...
    cmake -S . -B "%BUILD_DIR%" -DBUILD_TESTS=%BUILD_TESTS% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake"
)
echo Building (incremental)...
cmake --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo ✗ Build failed
    exit /b 1
)
echo ✓ Build completed
exit /b 0

:rebuild
if exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Build directory detected. Cleaning and rebuilding...
    rmdir /s /q "%BUILD_DIR%"
)

call :setup_vcpkg

echo Configuring CMake...
cmake -S . -B "%BUILD_DIR%" -DBUILD_TESTS=%BUILD_TESTS% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake"
if %errorlevel% neq 0 (
    echo ✗ CMake configuration failed
    exit /b 1
)

echo Building...
cmake --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo ✗ Build failed
    exit /b 1
)

echo ✓ Build completed successfully
exit /b 0

:setup_vcpkg
if not exist "%VCPKG_DIR%" (
    echo vcpkg not found. Cloning...
    git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_DIR%"

    echo Checking out specific commit to match CI...
    pushd "%VCPKG_DIR%"
    git checkout %VCPKG_COMMIT%

    echo Bootstrapping vcpkg...
    call bootstrap-vcpkg.bat -disableMetrics
    popd
) else (
    echo vcpkg detected.
)
exit /b 0
