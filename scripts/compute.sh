#!/bin/bash

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default values
BUILD_DIR="build/dev"
CONFIG="RelWithDebInfo"
LOG_LEVEL="info"
PORTFOLIO_ID="demo_portfolio"
SCENARIO_SET_ID="demo_factor_scenarios"
SNAPSHOT_ID="DEMO_MKT_2026_03_24"
SKIP_ENV=0

# Simple argument parsing
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -LogLevel) LOG_LEVEL="$2"; shift ;;
        -PortfolioId) PORTFOLIO_ID="$2"; shift ;;
        -ScenarioSetId) SCENARIO_SET_ID="$2"; shift ;;
        -SnapshotId) SNAPSHOT_ID="$2"; shift ;;
        -SkipEnv) SKIP_ENV=1 ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

if [ "$SKIP_ENV" != "1" ] && [ -f "$SCRIPT_DIR/env.sh" ]; then
    QRP_ENV_QUIET=1 QRP_PROJECT_ROOT="$PROJECT_ROOT" . "$SCRIPT_DIR/env.sh"
fi

# Helper to find executable
find_exe() {
    local exe_name=$1
    local names=("$exe_name")
    case "$exe_name" in
        *.exe) ;;
        *) names+=("$exe_name.exe") ;;
    esac

    local name
    for name in "${names[@]}"; do
        local paths=(
            "$BUILD_DIR/$name"
            "$BUILD_DIR/$CONFIG/$name"
            "$BUILD_DIR/cpp/cli/$name"
            "$BUILD_DIR/cpp/cli/$CONFIG/$name"
            "$BUILD_DIR/bin/$name"
            "$BUILD_DIR/bin/$CONFIG/$name"
        )
        for path in "${paths[@]}"; do
            if [ -f "$path" ]; then
                echo "$path"
                return 0
            fi
        done
    done
    return 1
}

CLI_EXE=$(find_exe "qrp_cli")

if [ -z "$CLI_EXE" ]; then
    echo "Error: Could not find qrp_cli executable in $BUILD_DIR or nested directories. Please build the project first."
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
VAL_OUTPUT=$("$CLI_EXE" run-valuation --portfolio "$PORTFOLIO_ID" --snapshot "$SNAPSHOT_ID")
if [ $? -ne 0 ]; then echo "Valuation failed"; exit 1; fi

VAL_RUN_ID=$(echo "$VAL_OUTPUT" | grep -oP 'Run ID: \K.*')
echo "Valuation Run ID: $VAL_RUN_ID"

# 2. Run Risk (Sensitivities)
echo -e "\n[2/3] Running Risk (Sensitivities)..."
RISK_OUTPUT=$("$CLI_EXE" run-risk --portfolio "$PORTFOLIO_ID" --snapshot "$SNAPSHOT_ID")
if [ $? -ne 0 ]; then echo "Risk calculation failed"; exit 1; fi

RISK_RUN_ID=$(echo "$RISK_OUTPUT" | grep -oP 'Run ID: \K.*')
echo "Risk Run ID: $RISK_RUN_ID"

# 3. Run Historical VaR
echo -e "\n[3/3] Running Historical VaR..."
VAR_OUTPUT=$("$CLI_EXE" run-hvar --portfolio "$PORTFOLIO_ID" --snapshot "$SNAPSHOT_ID" --scenarios "$SCENARIO_SET_ID")
if [ $? -ne 0 ]; then echo "VaR calculation failed"; exit 1; fi

VAR_RUN_ID=$(echo "$VAR_OUTPUT" | grep -oP 'Run ID: \K.*')
echo "VaR Run ID: $VAR_RUN_ID"

# 4. Generate Final Report
echo -e "\n--- Final Valuation Report ---"
"$CLI_EXE" report "$VAL_RUN_ID"

echo -e "\nComputation flow completed successfully!"
