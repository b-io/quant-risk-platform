#!/bin/bash
# test.sh - Run all platform tests and print coverage summaries.

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="build/Release-Python"
CONFIG="Release"
PYTHON_EXECUTABLE=""
PYTHON_COVERAGE_MIN_LINE="0.0"
RUN_CPP_COVERAGE=0
SKIP_ENV=0
SKIP_PYTHON_COVERAGE=0

while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -Coverage) RUN_CPP_COVERAGE=1 ;;
        -PythonCoverageMinLine) PYTHON_COVERAGE_MIN_LINE="$2"; shift ;;
        -PythonExecutable) PYTHON_EXECUTABLE="$2"; shift ;;
        -SkipEnv) SKIP_ENV=1 ;;
        -SkipPythonCoverage) SKIP_PYTHON_COVERAGE=1 ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

resolve_project_path() {
    case "$1" in
        /*|[A-Za-z]:*) printf '%s\n' "$1" ;;
        *) printf '%s\n' "$PROJECT_ROOT/$1" ;;
    esac
}

RESOLVED_BUILD_DIR="$(resolve_project_path "$BUILD_DIR")"

if [ "$SKIP_ENV" != "1" ] && [ -f "$SCRIPT_DIR/env.sh" ]; then
    QRP_ENV_QUIET=1 QRP_PROJECT_ROOT="$PROJECT_ROOT" . "$SCRIPT_DIR/env.sh"
fi

resolve_python_for_coverage() {
    if [ -n "$PYTHON_EXECUTABLE" ]; then
        printf '%s\n' "$PYTHON_EXECUTABLE"
        return 0
    fi

    if [ -n "${QRP_PYTHON_EXECUTABLE:-}" ]; then
        printf '%s\n' "$QRP_PYTHON_EXECUTABLE"
        return 0
    fi

    if [ -n "${VCPKG_ROOT:-}" ] && [ -n "${VCPKG_TARGET_TRIPLET:-}" ]; then
        local vcpkg_python="$VCPKG_ROOT/installed/$VCPKG_TARGET_TRIPLET/tools/python3/python"
        if [ -x "$vcpkg_python" ]; then
            printf '%s\n' "$vcpkg_python"
            return 0
        fi
        if [ -x "$vcpkg_python.exe" ]; then
            printf '%s\n' "$vcpkg_python.exe"
            return 0
        fi
    fi

    if command -v python3 >/dev/null 2>&1; then
        command -v python3
        return 0
    fi
    if command -v python >/dev/null 2>&1; then
        command -v python
        return 0
    fi
    return 1
}

show_cpp_coverage_score() {
    local metric_path="$RESOLVED_BUILD_DIR/coverage/cpp/coverage_metric.json"
    if [ ! -f "$metric_path" ]; then
        echo "C++ Coverage Score: not generated (run with -Coverage using a GCC/Clang coverage build)."
        return 0
    fi

    if [ -n "${RESOLVED_PYTHON:-}" ]; then
        "$RESOLVED_PYTHON" - "$metric_path" <<'PY'
import json
import sys
metric = json.load(open(sys.argv[1], encoding="utf-8"))
print(
    "C++ Coverage Score: "
    f"{metric['line_coverage_percent']:.2f}% line "
    f"({metric['status']}, threshold {metric['threshold_percent']:.2f}%, "
    f"{metric['lines_covered']}/{metric['lines_valid']})"
)
PY
    else
        echo "C++ Coverage Score: metric available at $metric_path"
    fi
}

show_python_coverage_score() {
    if [ "$SKIP_PYTHON_COVERAGE" = "1" ]; then
        echo "Python Coverage Score: skipped."
        return 0
    fi

    local metric_path="$RESOLVED_BUILD_DIR/coverage/python/coverage_metric.json"
    if [ ! -f "$metric_path" ]; then
        echo "Python Coverage Score: not generated."
        return 0
    fi

    if [ -n "${RESOLVED_PYTHON:-}" ]; then
        "$RESOLVED_PYTHON" - "$metric_path" <<'PY'
import json
import sys
metric = json.load(open(sys.argv[1], encoding="utf-8"))
print(
    "Python Coverage Score: "
    f"{metric['line_coverage_percent']:.2f}% line "
    f"({metric['status']}, tests {metric['tests_run']}, "
    f"threshold {metric['threshold_percent']:.2f}%, "
    f"{metric['lines_covered']}/{metric['lines_valid']})"
)
PY
    else
        echo "Python Coverage Score: metric available at $metric_path"
    fi
}

echo "Building tests (Config: $CONFIG) in $RESOLVED_BUILD_DIR..."
cmake --build "$RESOLVED_BUILD_DIR" --target unit_tests integration_tests --config "$CONFIG"

# Helper to find executable in either $BUILD_DIR/tests/ or $BUILD_DIR/tests/$CONFIG/
find_test_exe() {
    local exe_name=$1
    local paths=(
        "$RESOLVED_BUILD_DIR/tests/$exe_name"
        "$RESOLVED_BUILD_DIR/tests/$CONFIG/$exe_name"
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
    echo "Error: unit_tests not found in $RESOLVED_BUILD_DIR/tests/ or $RESOLVED_BUILD_DIR/tests/$CONFIG/"
    exit 1
fi

echo "Running Unit Tests: $UNIT_TEST_EXE"
"$UNIT_TEST_EXE"

if [ -z "$INTEGRATION_TEST_EXE" ]; then
    echo "Error: integration_tests not found in $RESOLVED_BUILD_DIR/tests/ or $RESOLVED_BUILD_DIR/tests/$CONFIG/"
    exit 1
fi

echo "Running Integration Tests: $INTEGRATION_TEST_EXE"
"$INTEGRATION_TEST_EXE"

if [ "$RUN_CPP_COVERAGE" = "1" ]; then
    echo "Building C++ coverage target in $RESOLVED_BUILD_DIR..."
    cmake --build "$RESOLVED_BUILD_DIR" --target coverage --config "$CONFIG"
fi

PYTHON_COVERAGE_EXIT_CODE=0
RESOLVED_PYTHON=""
if [ "$SKIP_PYTHON_COVERAGE" != "1" ]; then
    if RESOLVED_PYTHON="$(resolve_python_for_coverage)"; then
        echo "Running Python coverage with $RESOLVED_PYTHON..."
        "$RESOLVED_PYTHON" "$PROJECT_ROOT/scripts/coverage/python/coverage_metric.py" \
            --project-root "$PROJECT_ROOT" \
            --source "$PROJECT_ROOT/python/qrp" \
            --tests "$PROJECT_ROOT/tests/python" \
            --json "$RESOLVED_BUILD_DIR/coverage/python/coverage_metric.json" \
            --markdown "$RESOLVED_BUILD_DIR/coverage/python/coverage_metric.md" \
            --threshold "$PYTHON_COVERAGE_MIN_LINE" || PYTHON_COVERAGE_EXIT_CODE=$?
    else
        echo "Warning: Python executable was not found; Python coverage was not generated." >&2
    fi
fi

echo "All tests passed successfully!"
echo "Coverage Summary"
show_cpp_coverage_score
show_python_coverage_score

if [ "$PYTHON_COVERAGE_EXIT_CODE" != "0" ]; then
    exit "$PYTHON_COVERAGE_EXIT_CODE"
fi
