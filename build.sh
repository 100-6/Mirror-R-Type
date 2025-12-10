#!/bin/bash

set -e

BUILD_DIR="build"
BUILD_TESTS="OFF"
VCPKG_DIR="vcpkg"
# Commit ID from CI/vcpkg.json to ensure consistency
VCPKG_COMMIT="bd52fac7114fdaa2208de8dd1227212a6683e562"

# Usage function
usage() {
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  (none)       - Full rebuild (clean + configure + build)"
    echo "  make         - Incremental build (only recompile changed files)"
    echo "  clean        - Remove build artifacts (keeps build directory)"
    echo "  fclean       - Remove entire build directory"
    echo "  re           - Alias for fclean + full rebuild"
    echo ""
    echo "Options:"
    echo "  test         - Enable tests compilation"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                 # Full rebuild"
    echo "  ./build.sh make            # Incremental build"
    echo "  ./build.sh make test       # Incremental build with tests"
    echo "  ./build.sh clean           # Clean artifacts"
    echo "  ./build.sh fclean          # Remove build directory"
    echo "  ./build.sh re              # Full clean + rebuild"
    exit 0
}

# Parse commands
CMD="${1:-rebuild}"

if [ "$CMD" == "help" ] || [ "$CMD" == "-h" ] || [ "$CMD" == "--help" ]; then
    usage
fi

# Check for test flag
if [ "$1" == "test" ] || [ "$2" == "test" ]; then
    BUILD_TESTS="ON"
    echo "Tests enabled"
fi

# Function to setup vcpkg
setup_vcpkg() {
    if [ ! -d "$VCPKG_DIR" ]; then
        echo "vcpkg not found. Cloning..."
        git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_DIR"
        
        echo "Checking out specific commit to match CI..."
        pushd "$VCPKG_DIR" > /dev/null
        git checkout "$VCPKG_COMMIT"
        
        echo "Bootstrapping vcpkg..."
        ./bootstrap-vcpkg.sh -disableMetrics
        popd > /dev/null
    else
        echo "vcpkg detected."
    fi
}

# Handle commands
case "$CMD" in
    clean)
        echo "Cleaning build artifacts in ${BUILD_DIR}..."
        if [ -d "$BUILD_DIR" ]; then
            cmake --build "$BUILD_DIR" --target clean 2>/dev/null || rm -rf "$BUILD_DIR"/*
            echo "✓ Build artifacts cleaned"
        else
            echo "Nothing to clean (build directory doesn't exist)"
        fi
        exit 0
        ;;

    fclean)
        echo "Removing entire build directory: ${BUILD_DIR}"
        rm -rf "$BUILD_DIR"
        echo "✓ Build directory removed"
        exit 0
        ;;

    re)
        echo "Full rebuild: fclean + configure + build"
        rm -rf "$BUILD_DIR"
        # Continue to rebuild below
        ;;

    make)
        echo "Incremental build..."
        setup_vcpkg
        if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
            echo "Build directory not found. Running full configuration first..."
            cmake -S . -B "$BUILD_DIR" -DBUILD_TESTS="$BUILD_TESTS" -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
        fi
        echo "Building (incremental)..."
        cmake --build "$BUILD_DIR" -j$(nproc)
        echo "✓ Build completed"
        exit 0
        ;;

    rebuild|*)
        # Default: full rebuild
        if [ -d "$BUILD_DIR" ] && [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
            echo "Build directory detected. Cleaning and rebuilding..."
            rm -rf "$BUILD_DIR"
        fi
        ;;
esac

# Full rebuild
setup_vcpkg

echo "Configuring CMake..."
# Explicitly use the local vcpkg toolchain
cmake -S . -B "$BUILD_DIR" -DBUILD_TESTS="$BUILD_TESTS" -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"

echo "Building..."
cmake --build "$BUILD_DIR" -j$(nproc)

echo "✓ Build completed successfully"
