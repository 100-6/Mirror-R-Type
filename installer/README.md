# R-Type Windows Installer

This directory contains all the files needed to create a Windows installer for the R-Type game.

## Files Overview

### Core Files

- **`rtype-installer.nsi`** - NSIS script that defines the installer
  - Configures installation directory, shortcuts, and uninstaller
  - Handles file copying and registry entries
  - Provides multilingual support (English/French)
  - Creates Start Menu shortcuts and Desktop icon

- **`prepare-installer.ps1`** - PowerShell script for file preparation
  - Automatically collects all required files
  - Copies executables (client and server)
  - Gathers plugin DLLs
  - Collects vcpkg dependencies
  - Copies game assets
  - Creates an `installer-package` directory ready for packaging

- **`build-installer.bat`** - All-in-one Windows batch script
  - Verifies NSIS installation
  - Runs the preparation script
  - Copies files to the correct location
  - Compiles the NSIS installer
  - Creates a portable ZIP archive

- **`.gitignore`** - Prevents committing generated files
  - Excludes installer executables
  - Excludes ZIP archives
  - Excludes temporary build directories

## Quick Usage

### Automatic Method (via GitHub Actions) - Recommended

Simply create and push a git tag:

```bash
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions will automatically:
1. Build the project on Windows
2. Create the installer
3. Publish a GitHub release with downloadable files

### Manual Method (on Windows)

#### Prerequisites
1. **NSIS**: Download from [nsis.sourceforge.io](https://nsis.sourceforge.io/)
2. **Compiled project**: Run `build.bat` first

#### Steps

**Option 1: Automated (Recommended)**
```powershell
cd installer
.\build-installer.bat
```

**Option 2: Step by Step**
```powershell
# 1. Prepare files
cd installer
.\prepare-installer.ps1

# 2. Copy files to build directory
Copy-Item ..\build\installer-package\* ..\build\ -Recurse -Force

# 3. Build the installer
makensis rtype-installer.nsi
```

## Output Files

After building, you'll get:

- **`RType-Setup.exe`** - Windows installer (~50-100 MB)
  - Installs to Program Files
  - Creates shortcuts
  - Adds to Windows Apps & Features
  - Includes uninstaller

- **`RType-Windows-x64.zip`** - Portable version
  - Extract and run anywhere
  - No installation required
  - All dependencies included

## Installer Contents

The installer packages:

```
R-Type/
├── r_type_client.exe          # Game client
├── r_type_server.exe          # Game server
├── *.dll                      # vcpkg dependencies (Boost, SFML, etc.)
├── plugins/
│   ├── raylib_graphics.dll
│   ├── asio_network.dll
│   ├── miniaudio_audio.dll
│   └── ...                    # Other plugins
└── assets/
    └── fonts/                 # Game resources
```

## Customization

To customize the installer, edit `rtype-installer.nsi`:

- **Line 9**: Program name
- **Line 10**: Output filename
- **Line 13**: Default installation directory
- **Lines 22-27**: Version information
- **Lines 34-35**: Icon files (optional)

## Requirements

### For Building Locally
- Windows 10/11 (64-bit)
- Visual Studio 2022 or MSVC Build Tools
- NSIS 3.x
- PowerShell 5.0+

### For End Users
- Windows 10/11 (64-bit)
- Visual C++ Redistributable 2015-2022 (usually pre-installed)

## Complete Documentation

For detailed release instructions, see [docs/RELEASE.md](../docs/RELEASE.md)

## Troubleshooting

### "NSIS not found"
Install NSIS from [nsis.sourceforge.io](https://nsis.sourceforge.io/) and add it to your PATH.

### "Executables not found"
Build the project first:
```bash
.\build.bat
```

### "Missing DLLs"
Ensure vcpkg built all dependencies:
```bash
ls vcpkg\installed\x64-windows\bin\*.dll
```

## Release Workflow

1. **Develop** - Make changes to your code
2. **Build** - Compile with `build.bat`
3. **Test** - Verify the game works locally
4. **Tag** - Create a version tag (`git tag v1.0.0`)
5. **Push** - Push the tag to trigger GitHub Actions
6. **Distribute** - Share the generated installer from GitHub Releases

## GitHub Actions Integration

The installer creation is fully automated in the CI/CD pipeline:

- **Trigger**: Push a tag matching `v*` (e.g., `v1.0.0`)
- **Build time**: ~15-30 minutes
- **Outputs**: Installer + ZIP archive
- **Distribution**: Automatic GitHub Release creation

See [`.github/workflows/windows-release.yml`](../.github/workflows/windows-release.yml)

## Support

For questions or issues, see the main [documentation](../docs/) or open a GitHub issue.