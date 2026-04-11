#!/bin/bash

# Default values
PORTFOLIO_ID="demo_macro_book"
SNAPSHOT_ID="SNAP:2024-03-24"
SCENARIO_SET_ID="SC_SET_HIST_01"
BUILD_DIR="build/Release-Python"
CONFIG="Release"
LOG_LEVEL="info"

# Simple argument parsing
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -Portfolio) PORTFOLIO_ID="$2"; shift ;;
        -Snapshot) SNAPSHOT_ID="$2"; shift ;;
        -Scenarios) SCENARIO_SET_ID="$2"; shift ;;
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -LogLevel) LOG_LEVEL="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

# Helper to find executable
find_exe() {
    local exe_name=$1
    local paths=(
        "$BUILD_DIR/$exe_name"
        "$BUILD_DIR/$CONFIG/$exe_name"
        "$BUILD_DIR/bin/$exe_name"
        "$BUILD_DIR/bin/$CONFIG/$exe_name"
    )
    for path in "${paths[@]}"; do
        if [ -f "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    return 1
}

CLI_EXE=$(find_exe "qrp_cli")

if [ -z "$CLI_EXE" ]; then
    echo "Error: Could not find qrp_cli in $BUILD_DIR or nested directories. Please build the project first."
    exit 1
fi

# Set log level
export QRP_LOG_LEVEL=$LOG_LEVEL

echo "--- Quant Risk Platform: Compute Flow ---"
echo "Portfolio: $PORTFOLIO_ID"
echo "Snapshot:  $SNAPSHOT_ID"
echo "Scenarios: $SCENARIO_SET_ID"
echo "----------------------------------------"

# 1. Run Valuation
echo -e "\n[1/3] Running Valuation..."
VAL_OUTPUT=$($CLI_EXE run-valuation --portfolio "$PORTFOLIO_ID" --snapshot "$SNAPSHOT_ID")
if [ $? -ne 0 ]; then echo "Valuation failed"; exit 1; fi

VAL_RUN_ID=$(echo "$VAL_OUTPUT" | grep -oP 'Run ID: \K.*')
echo "Valuation Run ID: $VAL_RUN_ID"

# 2. Run Risk (Sensitivities)
echo -e "\n[2/3] Running Risk (Sensitivities)..."
RISK_OUTPUT=$($CLI_EXE run-risk --portfolio "$PORTFOLIO_ID" --snapshot "$SNAPSHOT_ID")
if [ $? -ne 0 ]; then echo "Risk calculation failed"; exit 1; fi

RISK_RUN_ID=$(echo "$RISK_OUTPUT" | grep -oP 'Run ID: \K.*')
echo "Risk Run ID: $RISK_RUN_ID"

# 3. Run Historical VaR
echo -e "\n[3/3] Running Historical VaR..."
VAR_OUTPUT=$($CLI_EXE run-hvar --portfolio "$PORTFOLIO_ID" --snapshot "$SNAPSHOT_ID" --scenarios "$SCENARIO_SET_ID")
if [ $? -ne 0 ]; then echo "VaR calculation failed"; exit 1; fi

VAR_RUN_ID=$(echo "$VAR_OUTPUT" | grep -oP 'Run ID: \K.*')
echo "VaR Run ID: $VAR_RUN_ID"

# 4. Generate Final Report
echo -e "\n--- Final Valuation Report ---"
$CLI_EXE report "$VAL_RUN_ID"

echo -e "\nComputation flow completed successfully!"
