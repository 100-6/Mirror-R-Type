#!/bin/bash

set -e

BUILD_DIR="build"

if [ "$1" == "clean" ]; then
    echo "Cleaning : ${BUILD_DIR}"
    rm -rf "$BUILD_DIR"
    exit 0
fi

cmake -S . -B "$BUILD_DIR"

echo "Building"
cmake --build "$BUILD_DIR"
