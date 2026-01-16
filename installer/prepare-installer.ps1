# R-Type Installer Preparation Script
# This script prepares all files needed for the NSIS installer

param(
    [string]$BuildDir = "../build",
    [string]$VcpkgDir = "../vcpkg/installed/x64-windows"
)

Write-Host "R-Type Installer Preparation Script" -ForegroundColor Cyan
Write-Host "====================================`n" -ForegroundColor Cyan

# Check if build directory exists
if (-not (Test-Path $BuildDir)) {
    Write-Host "Error: Build directory not found at $BuildDir" -ForegroundColor Red
    Write-Host "Please build the project first using build.bat or CMake" -ForegroundColor Yellow
    exit 1
}

# Create temporary package directory
$packageDir = Join-Path $BuildDir "installer-package"
Write-Host "Creating package directory: $packageDir" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

# Function to copy with error handling
function Copy-WithCheck {
    param($Source, $Destination)
    if (Test-Path $Source) {
        Copy-Item $Source $Destination -Force -Recurse
        Write-Host "  [OK] Copied: $Source" -ForegroundColor Green
        return $true
    } else {
        Write-Host "  [SKIP] Not found: $Source" -ForegroundColor Yellow
        return $false
    }
}

# Copy executables
Write-Host "`nCopying executables..." -ForegroundColor Cyan
$executablesCopied = 0
if (Copy-WithCheck "$BuildDir/Release/r-type_client.exe" "$packageDir/") { $executablesCopied++ }
elseif (Copy-WithCheck "$BuildDir/r-type_client.exe" "$packageDir/") { $executablesCopied++ }

if (Copy-WithCheck "$BuildDir/Release/r-type_server.exe" "$packageDir/") { $executablesCopied++ }
elseif (Copy-WithCheck "$BuildDir/r-type_server.exe" "$packageDir/") { $executablesCopied++ }

if ($executablesCopied -eq 0) {
    Write-Host "Error: No executables found!" -ForegroundColor Red
    exit 1
}

# Copy plugins
Write-Host "`nCopying plugins..." -ForegroundColor Cyan
$pluginsDir = Join-Path $packageDir "plugins"
New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null

$pluginsSources = @(
    "$BuildDir/plugins",
    "$BuildDir/Release/plugins"
)

foreach ($source in $pluginsSources) {
    if (Test-Path $source) {
        Get-ChildItem "$source/*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName $pluginsDir -Force
            Write-Host "  [OK] Copied plugin: $($_.Name)" -ForegroundColor Green
        }
    }
}

# Copy DLL dependencies from vcpkg
Write-Host "`nCopying DLL dependencies..." -ForegroundColor Cyan
if (Test-Path "$VcpkgDir/bin") {
    Get-ChildItem "$VcpkgDir/bin/*.dll" | ForEach-Object {
        Copy-Item $_.FullName $packageDir -Force
        Write-Host "  [OK] Copied: $($_.Name)" -ForegroundColor Green
    }
} else {
    Write-Host "  [WARN] vcpkg bin directory not found at: $VcpkgDir/bin" -ForegroundColor Yellow
    Write-Host "  You may need to copy DLL dependencies manually" -ForegroundColor Yellow
}

# Copy assets
Write-Host "`nCopying assets..." -ForegroundColor Cyan
if (Test-Path "../assets") {
    Copy-Item "../assets" "$packageDir/assets" -Recurse -Force
    Write-Host "  [OK] Copied assets directory" -ForegroundColor Green
} else {
    Write-Host "  [WARN] Assets directory not found" -ForegroundColor Yellow
}

# Check for LICENSE file
Write-Host "`nChecking for LICENSE..." -ForegroundColor Cyan
if (-not (Test-Path "../LICENSE")) {
    Write-Host "  [WARN] LICENSE file not found, creating a basic one..." -ForegroundColor Yellow
    @"
EPITECH Project License

This project is part of the EPITECH curriculum.
Copyright (c) 2025
"@ | Out-File -FilePath "../LICENSE" -Encoding utf8
    Write-Host "  [OK] Created LICENSE file" -ForegroundColor Green
}

# List package contents
Write-Host "`nPackage contents:" -ForegroundColor Cyan
Write-Host "=================" -ForegroundColor Cyan
Get-ChildItem $packageDir -Recurse -File | ForEach-Object {
    $relativePath = $_.FullName.Replace($packageDir, "").TrimStart('\')
    Write-Host "  $relativePath" -ForegroundColor White
}

# Calculate package size
$totalSize = (Get-ChildItem $packageDir -Recurse -File | Measure-Object -Property Length -Sum).Sum
$sizeInMB = [math]::Round($totalSize / 1MB, 2)
Write-Host "`nTotal package size: $sizeInMB MB" -ForegroundColor Cyan

# Summary
Write-Host "`n====================================`n" -ForegroundColor Cyan
Write-Host "Package preparation complete!" -ForegroundColor Green
Write-Host "Package location: $packageDir" -ForegroundColor White
Write-Host "`nNext steps:" -ForegroundColor Cyan
Write-Host "1. Install NSIS from: https://nsis.sourceforge.io/" -ForegroundColor White
Write-Host "2. Copy contents of $packageDir to $BuildDir" -ForegroundColor White
Write-Host "3. Run: makensis installer\rtype-installer.nsi" -ForegroundColor White
Write-Host "4. Installer will be created at: installer\RType-Setup.exe" -ForegroundColor White
Write-Host "`nOr use the GitHub Action to build automatically!" -ForegroundColor Yellow
Write-Host "====================================`n" -ForegroundColor Cyan
