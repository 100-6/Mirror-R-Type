#!/bin/bash

set -e

BUILD_DIR="build"
BUILD_TESTS="OFF"
BUILD_RTYPE="OFF"
BUILD_BAGARIO="OFF"
VCPKG_DIR="vcpkg"
# Commit ID from CI/vcpkg.json to ensure consistency
VCPKG_COMMIT="bd52fac7114fdaa2208de8dd1227212a6683e562"

# Usage function
usage() {
    echo "Usage: $0 [TARGET] [COMMAND]"
    echo ""
    echo "Targets:"
    echo "  rtype        - Build R-Type game only (engine + r-type plugins)"
    echo "  bagario      - Build Bagario game only (engine + bagario plugins)"
    echo "  test         - Build tests only (engine + tests)"
    echo "  all          - Build everything (R-Type + Bagario + tests)"
    echo "  (none)       - Build R-Type + Bagario (no tests, default)"
    echo ""
    echo "Commands:"
    echo "  (none)       - Full rebuild (clean + configure + build)"
    echo "  make         - Incremental build (only recompile changed files)"
    echo "  clean        - Remove build artifacts (keeps build directory)"
    echo "  fclean       - Remove entire build directory"
    echo "  re           - Alias for fclean + full rebuild"
    echo ""
    echo "Examples:"
    echo "  ./build.sh rtype           # Build R-Type only (full rebuild)"
    echo "  ./build.sh rtype make      # Build R-Type only (incremental)"
    echo "  ./build.sh bagario         # Build Bagario only (full rebuild)"
    echo "  ./build.sh bagario make    # Build Bagario only (incremental)"
    echo "  ./build.sh test            # Build tests only"
    echo "  ./build.sh test make       # Build tests only (incremental)"
    echo "  ./build.sh all             # Build everything"
    echo "  ./build.sh                 # Build R-Type + Bagario (no tests)"
    echo "  ./build.sh make            # Incremental build (R-Type + Bagario)"
    echo "  ./build.sh clean           # Clean artifacts"
    echo "  ./build.sh fclean          # Remove build directory"
    exit 0
}

# Parse first argument for help
if [ "$1" == "help" ] || [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    usage
fi

# Parse target (rtype, bagario, test, or all)
CMD="${1:-rebuild}"

if [ "$1" == "rtype" ]; then
    BUILD_RTYPE="ON"
    BUILD_BAGARIO="OFF"
    BUILD_TESTS="OFF"
    CMD="${2:-rebuild}"
    echo "Building R-Type only"
elif [ "$1" == "bagario" ]; then
    BUILD_RTYPE="OFF"
    BUILD_BAGARIO="ON"
    BUILD_TESTS="OFF"
    CMD="${2:-rebuild}"
    echo "Building Bagario only"
elif [ "$1" == "test" ]; then
    BUILD_RTYPE="OFF"
    BUILD_BAGARIO="OFF"
    BUILD_TESTS="ON"
    CMD="${2:-rebuild}"
    echo "Building Tests only"
elif [ "$1" == "all" ]; then
    BUILD_RTYPE="ON"
    BUILD_BAGARIO="ON"
    BUILD_TESTS="ON"
    CMD="${2:-rebuild}"
    echo "Building everything (R-Type + Bagario + Tests)"
fi

# If no specific target, build R-Type + Bagario by default (backward compatibility)
if [ "$BUILD_RTYPE" == "OFF" ] && [ "$BUILD_BAGARIO" == "OFF" ] && [ "$BUILD_TESTS" == "OFF" ]; then
    BUILD_RTYPE="ON"
    BUILD_BAGARIO="ON"
    echo "Building R-Type + Bagario (no tests)"
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
            cmake -S . -B "$BUILD_DIR" \
                -DBUILD_TESTS="$BUILD_TESTS" \
                -DBUILD_RTYPE="$BUILD_RTYPE" \
                -DBUILD_BAGARIO="$BUILD_BAGARIO" \
                -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
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
cmake -S . -B "$BUILD_DIR" \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DBUILD_RTYPE="$BUILD_RTYPE" \
    -DBUILD_BAGARIO="$BUILD_BAGARIO" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"

echo "Building..."
cmake --build "$BUILD_DIR" -j$(nproc)

echo "✓ Build completed successfully"
