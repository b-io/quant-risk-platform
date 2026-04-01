#!/bin/bash
set -e

# Install dependencies for Quant Risk Platform

# 1. Ensure Python dependencies
echo -e "\033[0;36m--- Selecting Python Interpreter ---\033[0m"

# Find the vcpkg-provided Python if it exists
VCPKG_PYTHON=""
candidates=(
    "build/Release-Python/vcpkg_installed/x64-windows/tools/python3/python.exe"
    "build/Release-Python/vcpkg_installed/x64-linux/tools/python3/python"
    "build/Release/vcpkg_installed/x64-windows/tools/python3/python.exe"
    "build/Release/vcpkg_installed/x64-linux/tools/python3/python"
)

for cand in "${candidates[@]}"; do
    if [[ -f "$cand" ]]; then
        VCPKG_PYTHON="$cand"
        break
    fi
done

PYTHON_EXEC="python3"
if [[ ! -z "$VCPKG_PYTHON" ]]; then
    PYTHON_EXEC="$VCPKG_PYTHON"
    echo "Using vcpkg-provided Python: $PYTHON_EXEC"
    
    # Ensure pip exists
    if ! "$PYTHON_EXEC" -m pip --version >/dev/null 2>&1; then
        echo -e "\033[0;33mpip not found for vcpkg Python. Attempting to install pip...\033[0m"
        "$PYTHON_EXEC" -m ensurepip --upgrade || echo "Failed to install pip via ensurepip"
    fi
else
    echo -e "\033[0;33mWarning: vcpkg Python not found. Using system 'python3'.\033[0m"
fi

echo -e "\033[0;36m--- Installing Python dependencies ---\033[0m"
if [[ -f "requirements.txt" ]]; then
    "$PYTHON_EXEC" -m pip install -r requirements.txt
fi
if [[ -f "pyproject.toml" ]]; then
    "$PYTHON_EXEC" -m pip install -e .
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
    "$VCPKG_EXEC" install --allow-unsupported
else
    echo -e "\033[0;33mWarning: vcpkg not found in PATH or VCPKG_ROOT.\033[0m"
fi

echo -e "\033[0;32m--- Dependency installation complete ---\033[0m"
