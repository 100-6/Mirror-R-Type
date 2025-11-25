#!/bin/bash

set -e

BUILD_DIR="build"
BUILD_TESTS="OFF"

if [ "$1" == "clean" ]; then
    echo "Cleaning : ${BUILD_DIR}"
    rm -rf "$BUILD_DIR"
    exit 0
fi

if [ "$1" == "test" ] || [ "$2" == "test" ]; then
    BUILD_TESTS="ON"
    echo "Building with tests enabled"
fi

if [ -d "$BUILD_DIR" ] && [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Build directory detected. Cleaning and rebuilding..."
    rm -rf "$BUILD_DIR"
fi

echo "Configuring CMake..."
cmake -S . -B "$BUILD_DIR" -DBUILD_TESTS="$BUILD_TESTS"

echo "Building..."
cmake --build "$BUILD_DIR"
