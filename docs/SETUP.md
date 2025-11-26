# Setup Guide

This guide will help you set up the Mirror R-Type project on your local machine.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Platform-Specific Setup](#platform-specific-setup)
  - [Linux](#linux-debianubuntu)
  - [macOS](#macos)
  - [Windows](#windows)
- [Building the Project](#building-the-project)
- [Running the Game](#running-the-game)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Tools

- **CMake** 3.20 or higher
- **C++20 compatible compiler**:
  - GCC 12+ (Linux)
  - Clang 15+ (macOS/Linux)
  - MSVC 2022+ (Windows)
- **Git**

### vcpkg (Package Manager)

The project uses vcpkg for dependency management. It will be automatically set up during the build process.

---

## Platform-Specific Setup

### Linux (Debian/Ubuntu)

#### 1. Install System Dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    curl \
    zip \
    unzip \
    tar
```

#### 2. Install Compiler

```bash
# GCC 12+
sudo apt-get install g++-12

# Or Clang 15+
sudo apt-get install clang-15
```

#### 3. Setup vcpkg

```bash
cd Mirror-R-Type
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
cd ..
```

#### 4. Build the Project

```bash
cmake -B build
cmake --build build -j$(nproc)
```

---

### macOS

#### 1. Install Homebrew

If not already installed:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. Install Dependencies

```bash
brew install cmake git llvm
```

#### 3. Setup vcpkg

```bash
cd Mirror-R-Type
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
cd ..
```

#### 4. Build the Project

```bash
# Use Homebrew's clang
export CC=/opt/homebrew/opt/llvm/bin/clang
export CXX=/opt/homebrew/opt/llvm/bin/clang++

cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)
```

---

### Windows

#### 1. Install Visual Studio 2022

Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/) with:
- Desktop development with C++
- C++ CMake tools
- Windows 10 SDK

#### 2. Install Git

Download from [git-scm.com](https://git-scm.com/download/win)

#### 3. Setup vcpkg

Open **PowerShell** or **Developer Command Prompt**:

```powershell
cd Mirror-R-Type
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
cd ..
```

#### 4. Build the Project

```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

---

## Building the Project

### Standard Build

```bash
# Configure
cmake -B build

# Build
cmake --build build

# Build specific target
cmake --build build --target r_type_client
cmake --build build --target r_type_server
cmake --build build --target asio_network
```

### Build Options

```bash
# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build without tests
cmake -B build -DBUILD_TESTS=OFF

# Build only server
cmake -B build -DBUILD_CLIENT=OFF -DBUILD_SERVER=ON
```

### Clean Build

```bash
rm -rf build
cmake -B build
cmake --build build
```

---

## Running the Game

### Start the Server

```bash
./build/r_type_server
```

The server will start on port `8080` by default.

### Start the Client

```bash
./build/r_type_client
```

The client will connect to `localhost:8080` by default.

### Command Line Options

```bash
# Server with custom port
./build/r_type_server --port 9000

# Client with custom server
./build/r_type_client --host 192.168.1.100 --port 9000
```

---

## Troubleshooting

### vcpkg Issues

#### "Could NOT find Boost"

Solution: Ensure vcpkg is properly bootstrapped and retry:

```bash
cd vcpkg
git pull
./bootstrap-vcpkg.sh  # or .bat on Windows
cd ..
rm -rf build
cmake -B build
```

#### "Boost takes too long to compile"

vcpkg compiles Boost from source (10-20 minutes first time). This is normal. Subsequent builds are instant (cached).

Alternatively, use system packages:

```bash
# Linux
sudo apt-get install libboost-all-dev

# macOS
brew install boost

# Then build without vcpkg
rm -rf vcpkg vcpkg_installed
cmake -B build
```

---

### Build Errors

#### "CMake version too old"

Update CMake:

```bash
# Linux
sudo apt-get install cmake

# macOS
brew upgrade cmake

# Or download from cmake.org
```

#### "Compiler does not support C++20"

Update your compiler:

```bash
# Linux - GCC
sudo apt-get install g++-12

# Linux - Clang
sudo apt-get install clang-15

# macOS
brew install llvm
```

---

### Runtime Errors

#### "Plugin not found"

Ensure plugins are built and in the correct location:

```bash
ls build/plugins/
# Should show: asio_network.so, raylib_graphics.so, miniaudio_audio.so
```

#### "Port already in use"

Change the port:

```bash
./build/r_type_server --port 9000
```

Or find and kill the process using the port:

```bash
# Linux/macOS
sudo lsof -i :8080
kill <PID>

# Windows
netstat -ano | findstr :8080
taskkill /PID <PID> /F
```

---

### Graphics Issues

#### "Failed to initialize window"

On Linux, ensure you have X11 or Wayland:

```bash
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
```

On WSL, you need an X server like [VcXsrv](https://sourceforge.net/projects/vcxsrv/).

---

## Next Steps

- Read the [Developer Guide](DEVELOPER.md) to contribute
- Check out the [Architecture Documentation](Rtype_architecture.md)
- Learn about [Creating Plugins](../engine/plugin_manager/HOW_TO_CREATE_PLUGIN.md)

---

## Need Help?

- Check existing [GitHub Issues](https://github.com/100-6/Mirror-R-Type/issues)
- Contact the development team

