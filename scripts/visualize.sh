#!/bin/bash
# visualize.sh - Visualize data from the database

# Default values
DB_FILE="var/quant_risk_platform.sqlite"
BUILD_DIR="build/Release-Python"
CONFIG="Release"
TABLE="summary"
ID=""

# Simple argument parsing
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -Table) TABLE="$2"; shift ;;
        -Id) ID="$2"; shift ;;
        portfolios|trades|runs|results|risk|summary) TABLE="$1" ;;
        *) if [ -z "$ID" ]; then ID="$1"; else echo "Unknown parameter: $1"; exit 1; fi ;;
    esac
    shift
done

if [ ! -f "$DB_FILE" ]; then
    echo "Database file $DB_FILE not found."
    exit 1
fi

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

run_query() {
    local sql=$1
    if command -v sqlite3 &> /dev/null; then
        sqlite3 -header -column "$DB_FILE" "$sql"
    else
        echo "sqlite3 not found. For better visualization, install sqlite3."
        if [ ! -z "$CLI_EXE" ]; then
            echo "Falling back to qrp_cli list..."
            "$CLI_EXE" list
        fi
    fi
}

case $TABLE in
    portfolios)
        echo "=== Portfolios ==="
        run_query "SELECT portfolio_id, portfolio_name, base_currency FROM portfolios;"
        ;;
    trades)
        if [ -z "$ID" ]; then
            echo "Usage: $0 trades <portfolio_id>"
            exit 1
        fi
        echo "=== Trades for $ID ==="
        run_query "SELECT trade_id, book_id, asset_class, product_type, currency, notional FROM trades WHERE portfolio_id = '$ID';"
        ;;
    runs)
        echo "=== Analysis Runs ==="
        run_query "SELECT run_id, run_type, portfolio_id, snapshot_id, created_at FROM analysis_runs ORDER BY created_at DESC;"
        ;;
    results)
        if [ -z "$ID" ]; then
            echo "Usage: $0 results <run_id>"
            exit 1
        fi
        if [ ! -z "$CLI_EXE" ]; then
            echo "Using qrp_cli report for $ID ..."
            "$CLI_EXE" report "$ID"
        else
            echo "=== Valuation Results for $ID ==="
            run_query "SELECT trade_id, npv_base, valuation_ccy, status FROM valuation_results WHERE run_id = '$ID';"
        fi
        ;;
    risk)
        if [ -z "$ID" ]; then
            echo "Usage: $0 risk <run_id>"
            exit 1
        fi
        echo "=== Risk Results for $ID ==="
        run_query "SELECT trade_id, risk_measure, risk_factor_id, value FROM risk_results WHERE run_id = '$ID';"
        ;;
    summary|*)
        echo "=== Platform Data Summary ==="
        if command -v sqlite3 &> /dev/null; then
            echo -n "Portfolios: "; sqlite3 "$DB_FILE" "SELECT count(*) FROM portfolios;"
            echo -n "Trades: "; sqlite3 "$DB_FILE" "SELECT count(*) FROM trades;"
            echo -n "Market Snapshots: "; sqlite3 "$DB_FILE" "SELECT count(*) FROM market_snapshots;"
            echo -n "Analysis Runs: "; sqlite3 "$DB_FILE" "SELECT count(*) FROM analysis_runs;"
        else
            if [ ! -z "$CLI_EXE" ]; then "$CLI_EXE" list; fi
        fi
        echo ""
        echo "Usage: $0 [portfolios | trades <id> | runs | results <id> | risk <id>]"
        ;;
esac
