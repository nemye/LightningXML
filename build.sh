#!/usr/bin/env bash
set -euo pipefail

# Directory where build artifacts will go
BUILD_DIR="build"

# Installation prefix (change if needed)
INSTALL_PREFIX="$PWD/install"

# Create and enter the build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Set default build type to RELEASE
CMAKE_BUILD_TYPE="RELEASE"

# Check if CMAKE_BUILD_TYPE argument is provided
if [ "$#" -gt 0 ]; then
  CMAKE_BUILD_TYPE=$1
fi

# Configure the project
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" ..

# Build with all available cores
cmake --build . --config $CMAKE_BUILD_TYPE -- -j"$(nproc)"

# Run install
cmake --install . --config $CMAKE_BUILD_TYPE
