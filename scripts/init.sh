#!/bin/bash
# init.sh - Initialize the database and load sample data

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Default values
BUILD_DIR="build/dev"
CONFIG="RelWithDebInfo"
MARKET_FILE="data/market/demo_market.json"
PORTFOLIO_FILE="data/portfolios/demo_portfolio.json"
SCENARIO_FILE="data/scenarios/demo_scenarios.json"
FORCE=false
SKIP_ENV=0

# Simple argument parsing
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -MarketFile) MARKET_FILE="$2"; shift ;;
        -PortfolioFile) PORTFOLIO_FILE="$2"; shift ;;
        -ScenarioFile) SCENARIO_FILE="$2"; shift ;;
        -Force) FORCE=true ;;
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
DB_FILE="var/quant_risk_platform.sqlite"

if [ -z "$CLI_EXE" ]; then
    echo "Error: qrp_cli executable not found in $BUILD_DIR or nested directories."
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
"$CLI_EXE" import-market "$MARKET_FILE"

echo "Importing sample portfolios..."
"$CLI_EXE" import-portfolio "$PORTFOLIO_FILE"

echo "Importing sample scenarios..."
"$CLI_EXE" import-scenarios "$SCENARIO_FILE"

echo "Data initialization complete!"
"$CLI_EXE" list
