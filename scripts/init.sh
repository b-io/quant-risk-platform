#!/bin/bash
# init.sh - Initialize the database and load sample data

# Default values
BUILD_DIR="build/Release-Python"
CONFIG="Release"
FORCE=false

# Simple argument parsing
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -f|-Force|--force) FORCE=true ;;
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
DB_FILE="var/quant_risk_platform.sqlite"

if [ -z "$CLI_EXE" ]; then
    echo "Error: qrp_cli not found in $BUILD_DIR or nested directories."
    exit 1
fi

# Ensure var directory exists
mkdir -p var

if [ -f "$DB_FILE" ]; then
    if [ "$FORCE" = false ]; then
        echo "WARNING: Database $DB_FILE already exists."
        read -p "Do you want to delete and recreate it? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Initialization cancelled."
            exit 0
        fi
    fi
    echo "Deleting $DB_FILE..."
    rm "$DB_FILE"
fi

echo "Initializing database schema..."
"$CLI_EXE" init-db

echo "Importing sample market data..."
"$CLI_EXE" import-market data/market/demo_market.json

echo "Importing sample portfolios..."
"$CLI_EXE" import-portfolio data/portfolios/demo_portfolio.json

echo "Importing sample scenarios..."
"$CLI_EXE" import-scenarios data/scenarios/demo_scenarios.json

echo "Data initialization complete!"
"$CLI_EXE" list
