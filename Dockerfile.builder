# ==========================================
# Base Builder Image with vcpkg pre-installed
# Target: ARM64 (Raspberry Pi)
# ==========================================
# This image contains all build dependencies and vcpkg
# pre-bootstrapped to speed up CI builds significantly.
# ==========================================

FROM debian:bookworm-slim

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libudev-dev \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /vcpkg

# Clone and bootstrap vcpkg at specific commit
ARG VCPKG_COMMIT=bd52fac7114fdaa2208de8dd1227212a6683e562
RUN git clone --depth 1 --branch master https://github.com/Microsoft/vcpkg.git . && \
    git fetch --depth 1 origin ${VCPKG_COMMIT} && \
    git checkout ${VCPKG_COMMIT} && \
    ./bootstrap-vcpkg.sh -disableMetrics

# Pre-install common dependencies to cache them
# Copy vcpkg.json to install dependencies
WORKDIR /prebuild
COPY vcpkg.json .

# Install vcpkg dependencies (this is the slow part we want to cache)
# Clean up after install to reduce image size (keep .git until after install)
RUN /vcpkg/vcpkg install --triplet arm64-linux-dynamic && \
    rm -rf /vcpkg/.git /vcpkg/buildtrees /vcpkg/downloads /vcpkg/packages

# Set environment variables for builds
ENV VCPKG_ROOT=/vcpkg
ENV VCPKG_DEFAULT_TRIPLET=arm64-linux-dynamic

WORKDIR /build
