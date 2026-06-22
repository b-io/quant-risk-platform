#!/bin/bash
# Generic build script for Quant Risk Platform
# This script uses CMake presets for consistency across environments.

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

if [ "${QRP_SKIP_ENV:-0}" != "1" ] && [ -f "$SCRIPT_DIR/env.sh" ]; then
    QRP_ENV_QUIET=1 QRP_PROJECT_ROOT="$PROJECT_ROOT" . "$SCRIPT_DIR/env.sh"
fi

PRESET=${1:-Release-Python}
if [ "$#" -gt 0 ]; then
    shift 1
fi
EXTRA_ARGS="$@"

echo "--- Starting build for Quant Risk Platform (Preset: ${PRESET}) ---"

# 1. Configure CMake
echo "Configuring CMake with preset..."
cmake --preset "${PRESET}" ${EXTRA_ARGS}

# 2. Build the project
echo "Building with preset..."
cmake --build --preset "${PRESET}"

echo "--- Build Successful! ---"
