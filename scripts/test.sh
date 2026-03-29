#!/bin/bash
# test.sh - Run all platform tests

# Exit on error
set -e

# Default build directory
BUILD_DIR="build/Debug"

echo "Building tests..."
cmake --build "$BUILD_DIR" --target unit_tests integration_tests

echo "Running Unit Tests..."
./"$BUILD_DIR"/tests/unit_tests

echo "Running Integration Tests..."
./"$BUILD_DIR"/tests/integration_tests

echo "All tests passed successfully!"
