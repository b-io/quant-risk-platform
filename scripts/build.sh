#!/bin/bash
# Generic build script for Quant Risk Platform
# This script uses CMake presets for consistency across environments.

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

PRESET="dev"
SKIP_ENV=0
EXTRA_ARGS=()

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -ExtraArgs) shift; EXTRA_ARGS+=("$@"); break ;;
        -Preset) PRESET="$2"; shift ;;
        -SkipEnv) SKIP_ENV=1 ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

if [ "$SKIP_ENV" != "1" ] && [ -f "$SCRIPT_DIR/env.sh" ]; then
    QRP_ENV_QUIET=1 QRP_PROJECT_ROOT="$PROJECT_ROOT" . "$SCRIPT_DIR/env.sh"
fi

echo "--- Starting build for Quant Risk Platform (Preset: ${PRESET}) ---"

# 1. Configure CMake
echo "Configuring CMake with preset..."
cmake --preset "${PRESET}" "${EXTRA_ARGS[@]}"

# 2. Build the project
BUILD_DIR="${PROJECT_ROOT}/build/${PRESET}"
echo "Building ${BUILD_DIR}..."
cmake --build "${BUILD_DIR}"

echo "--- Build Successful! ---"
