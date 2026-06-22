#!/bin/bash
# test.sh - Run all platform tests

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

if [ "${QRP_SKIP_ENV:-0}" != "1" ] && [ -f "$SCRIPT_DIR/env.sh" ]; then
    QRP_ENV_QUIET=1 QRP_PROJECT_ROOT="$PROJECT_ROOT" . "$SCRIPT_DIR/env.sh"
fi

# Default build directory and config
BUILD_DIR="${1:-build/Release-Python}"
CONFIG="${2:-Release}"

echo "Building tests (Config: $CONFIG) in $BUILD_DIR..."
cmake --build "$BUILD_DIR" --target unit_tests integration_tests --config "$CONFIG"

# Helper to find executable in either $BUILD_DIR/tests/ or $BUILD_DIR/tests/$CONFIG/
find_test_exe() {
    local exe_name=$1
    local paths=(
        "$BUILD_DIR/tests/$exe_name"
        "$BUILD_DIR/tests/$CONFIG/$exe_name"
    )
    for path in "${paths[@]}"; do
        if [ -f "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    return 1
}

UNIT_TEST_EXE=$(find_test_exe "unit_tests")
INTEGRATION_TEST_EXE=$(find_test_exe "integration_tests")

if [ -z "$UNIT_TEST_EXE" ]; then
    echo "Error: unit_tests not found in $BUILD_DIR/tests/ or $BUILD_DIR/tests/$CONFIG/"
    exit 1
fi

echo "Running Unit Tests: $UNIT_TEST_EXE"
"$UNIT_TEST_EXE"

if [ -z "$INTEGRATION_TEST_EXE" ]; then
    echo "Error: integration_tests not found in $BUILD_DIR/tests/ or $BUILD_DIR/tests/$CONFIG/"
    exit 1
fi

echo "Running Integration Tests: $INTEGRATION_TEST_EXE"
"$INTEGRATION_TEST_EXE"

echo "All tests passed successfully!"
