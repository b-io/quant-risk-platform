#!/bin/bash
set -e

# Install dependencies for Quant Risk Platform

# 1. Ensure Python dependencies
echo -e "\033[0;36m--- Installing Python dependencies ---\033[0m"
if [[ -f "requirements.txt" ]]; then
    pip install -r requirements.txt
fi
if [[ -f "pyproject.toml" ]]; then
    pip install -e .
fi

# 2. Ensure C++ dependencies via vcpkg
echo -e "\033[0;36m--- Checking vcpkg ---\033[0m"
VCPKG_EXEC=""
if [ ! -z "$VCPKG_ROOT" ] && [ -f "$VCPKG_ROOT/vcpkg" ]; then
    VCPKG_EXEC="$VCPKG_ROOT/vcpkg"
elif command -v vcpkg >/dev/null 2>&1; then
    VCPKG_EXEC=$(command -v vcpkg)
fi

if [ ! -z "$VCPKG_EXEC" ]; then
    echo "Using vcpkg at: $VCPKG_EXEC"
    "$VCPKG_EXEC" install
else
    echo -e "\033[0;33mWarning: vcpkg not found in PATH or VCPKG_ROOT.\033[0m"
fi

echo -e "\033[0;32m--- Dependency installation complete ---\033[0m"
