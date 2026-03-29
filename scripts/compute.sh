#!/bin/bash

# Default values
PORTFOLIO_ID=${1:-"demo_macro_book"}
SNAPSHOT_ID=${2:-"SNAP:2024-03-24"}
SCENARIO_SET_ID=${3:-"SC_SET_HIST_01"}
BUILD_DIR=${4:-"build/Debug"}
LOG_LEVEL=${5:-"info"}

CLI_EXE="./$BUILD_DIR/qrp_cli"

# Check if executable exists
if [ ! -f "$CLI_EXE" ]; then
    echo "Error: Could not find qrp_cli at $CLI_EXE. Please build the project first."
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
