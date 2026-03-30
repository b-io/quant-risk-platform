#!/bin/bash
# Generic build script for Quant Risk Platform

BUILD_TYPE=${1:-Release}
shift 1
EXTRA_ARGS="$@"

# Set the project root to the directory where the script is located
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/${BUILD_TYPE}"

echo "--- Starting build for Quant Risk Platform (${BUILD_TYPE}) ---"

# 1. Configure CMake
echo "Configuring CMake..."

CMAKE_OPTS="-S ${PROJECT_ROOT} -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${EXTRA_ARGS}"

# Practical discovery order:
# 1. Explicit CMake inputs (Python3_EXECUTABLE, etc. - passed via EXTRA_ARGS)
# 2. VCPKG_ROOT env var for toolchain location
# 3. Normal discovery from PATH

# vcpkg discovery:
# CMAKE_TOOLCHAIN_FILE is the canonical way to point to vcpkg.
# If VCPKG_ROOT is set, it is used to derive the toolchain location.
if [ ! -z "$VCPKG_ROOT" ]; then
  VCPKG_TOOLCHAIN="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  if [ -f "$VCPKG_TOOLCHAIN" ]; then
    CMAKE_OPTS="${CMAKE_OPTS} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN}"
    echo "Using vcpkg toolchain derived from VCPKG_ROOT: $VCPKG_TOOLCHAIN"
  else
    echo "Error: VCPKG_ROOT is set but toolchain file not found: $VCPKG_TOOLCHAIN"
    exit 1
  fi
fi

# Python discovery:
# This project relies on standard FindPython3 behavior.
# Users should pass -DPython3_EXECUTABLE=... or -DPython3_ROOT_DIR=... as EXTRA_ARGS
# if they wish to override discovery. No environment variables are used.

cmake ${CMAKE_OPTS}

# 2. Build the project
echo "Building all targets..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"

if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "--- Build Successful! ---"
echo "Artifacts are in: ${BUILD_DIR}"
