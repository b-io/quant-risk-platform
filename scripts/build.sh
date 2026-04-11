#!/bin/bash
# Generic build script for Quant Risk Platform
# This script uses CMake presets for consistency across environments.

PRESET=${1:-Release-Python}
shift 1
EXTRA_ARGS="$@"

echo "--- Starting build for Quant Risk Platform (Preset: ${PRESET}) ---"

# 1. Configure CMake
echo "Configuring CMake with preset..."
cmake --preset "${PRESET}" ${EXTRA_ARGS}

if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi

# 2. Build the project
echo "Building with preset..."
cmake --build --preset "${PRESET}"

if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "--- Build Successful! ---"
