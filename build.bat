@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build
set BUILD_TESTS=OFF
set BUILD_RTYPE=OFF
set BUILD_BAGARIO=OFF
set VCPKG_DIR=vcpkg
set VCPKG_COMMIT=bd52fac7114fdaa2208de8dd1227212a6683e562

REM Load environment variables from .env if it exists
if exist ".env" (
    echo Loading environment variables from .env...
    for /f "usebackq tokens=*" %%a in (".env") do (
        echo %%a | findstr /r "^[^#]" >nul && set %%a
    )
)

set CMD=%1
if "%CMD%"=="" set CMD=rebuild
if "%CMD%"=="help" goto :usage
if "%CMD%"=="-h" goto :usage
if "%CMD%"=="--help" goto :usage

REM Parse target (rtype, bagario, test, or all)
if "%1"=="rtype" (
    set BUILD_RTYPE=ON
    set BUILD_BAGARIO=OFF
    set BUILD_TESTS=OFF
    set CMD=%2
    if "%CMD%"=="" set CMD=rebuild
    echo Building R-Type only
)
if "%1"=="bagario" (
    set BUILD_RTYPE=OFF
    set BUILD_BAGARIO=ON
    set BUILD_TESTS=OFF
    set CMD=%2
    if "%CMD%"=="" set CMD=rebuild
    echo Building Bagario only
)
if "%1"=="test" (
    set BUILD_RTYPE=OFF
    set BUILD_BAGARIO=OFF
    set BUILD_TESTS=ON
    set CMD=%2
    if "%CMD%"=="" set CMD=rebuild
    echo Building Tests only
)
if "%1"=="all" (
    set BUILD_RTYPE=ON
    set BUILD_BAGARIO=ON
    set BUILD_TESTS=ON
    set CMD=%2
    if "%CMD%"=="" set CMD=rebuild
    echo Building everything (R-Type + Bagario + Tests)
)

REM If no specific target, build all by default (backward compatibility)
if "%BUILD_RTYPE%%BUILD_BAGARIO%%BUILD_TESTS%"=="OFFOFFOFF" (
    set BUILD_RTYPE=ON
    set BUILD_BAGARIO=ON
    echo Building R-Type + Bagario (no tests^)
)

if "%CMD%"=="clean" goto :clean
if "%CMD%"=="fclean" goto :fclean
if "%CMD%"=="re" goto :re
if "%CMD%"=="make" goto :make
if "%CMD%"=="rebuild" goto :rebuild
goto :rebuild

:usage
echo Usage: %0 [TARGET] [COMMAND]
echo.
echo Targets:
echo   rtype        - Build R-Type game only (engine + r-type plugins)
echo   bagario      - Build Bagario game only (engine + bagario plugins)
echo   test         - Build tests only (engine + tests)
echo   all          - Build everything (R-Type + Bagario + tests)
echo   (none)       - Build R-Type + Bagario (no tests, default)
echo.
echo Commands:
echo   (none)       - Full rebuild (clean + configure + build)
echo   make         - Incremental build (only recompile changed files)
echo   clean        - Remove build artifacts (keeps build directory)
echo   fclean       - Remove entire build directory
echo   re           - Alias for fclean + full rebuild
echo.
echo Examples:
echo   build.bat rtype           # Build R-Type only (full rebuild)
echo   build.bat rtype make      # Build R-Type only (incremental)
echo   build.bat bagario         # Build Bagario only (full rebuild)
echo   build.bat bagario make    # Build Bagario only (incremental)
echo   build.bat test            # Build tests only
echo   build.bat test make       # Build tests only (incremental)
echo   build.bat all             # Build everything
echo   build.bat                 # Build R-Type + Bagario (no tests)
echo   build.bat make            # Incremental build (R-Type + Bagario)
echo   build.bat clean           # Clean artifacts
echo   build.bat fclean          # Remove build directory
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
    cmake -S . -B "%BUILD_DIR%" -DBUILD_TESTS=%BUILD_TESTS% -DBUILD_RTYPE=%BUILD_RTYPE% -DBUILD_BAGARIO=%BUILD_BAGARIO% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake"
)
echo Building (incremental)...
cmake --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo ✗ Build failed
    exit /b 1
)

REM Copy .env file to build directory if it exists
if exist ".env" (
    echo Copying .env to build directory...
    copy .env "%BUILD_DIR%\.env" >nul
    echo ✓ .env copied to %BUILD_DIR%\
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
cmake -S . -B "%BUILD_DIR%" -DBUILD_TESTS=%BUILD_TESTS% -DBUILD_RTYPE=%BUILD_RTYPE% -DBUILD_BAGARIO=%BUILD_BAGARIO% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake"
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

REM Copy .env file to build directory if it exists
if exist ".env" (
    echo Copying .env to build directory...
    copy .env "%BUILD_DIR%\.env" >nul
    echo ✓ .env copied to %BUILD_DIR%\
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
