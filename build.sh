#!/bin/bash

set -e

BUILD_DIR="build"

if [ "$1" == "clean" ]; then
    echo "Cleaning : ${BUILD_DIR}"
    rm -rf "$BUILD_DIR"
    exit 0
fi

if [ -d "$BUILD_DIR" ] && [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Build directory detected. Cleaning and rebuilding..."
    rm -rf "$BUILD_DIR"
fi

echo "Configuring CMake..."
cmake -S . -B "$BUILD_DIR"

echo "Building..."
cmake --build "$BUILD_DIR"
