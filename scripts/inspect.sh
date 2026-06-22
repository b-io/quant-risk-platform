#!/bin/bash
# inspect.sh - Inspect persisted data and run outputs from the database

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

# Default values
BUILD_DIR="build/Release-Python"
CONFIG="Release"
DB_FILE="var/quant_risk_platform.sqlite"
ID=""
SKIP_ENV=0
TABLE="summary"

# Simple argument parsing
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -BuildDir) BUILD_DIR="$2"; shift ;;
        -Config) CONFIG="$2"; shift ;;
        -DbFile) DB_FILE="$2"; shift ;;
        -Id) ID="$2"; shift ;;
        -SkipEnv) SKIP_ENV=1 ;;
        -Table) TABLE="$2"; shift ;;
        portfolios|trades|runs|results|risk|summary) TABLE="$1" ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

if [ "$SKIP_ENV" != "1" ] && [ -f "$SCRIPT_DIR/env.sh" ]; then
    QRP_ENV_QUIET=1 QRP_PROJECT_ROOT="$PROJECT_ROOT" . "$SCRIPT_DIR/env.sh"
fi

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
        echo "sqlite3 not found. For richer inspection, install sqlite3."
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
            echo "Usage: $0 trades -Id <portfolio_id>"
            exit 1
        fi
        echo "=== Trades for $ID ==="
        run_query "SELECT trade_id, book_id, asset_class, trade_type, currency, notional FROM trades WHERE portfolio_id = '$ID';"
        ;;
    runs)
        echo "=== Analysis Runs ==="
        run_query "SELECT run_id, run_type, portfolio_id, snapshot_id, created_at FROM analysis_runs ORDER BY created_at DESC;"
        ;;
    results)
        if [ -z "$ID" ]; then
            echo "Usage: $0 results -Id <run_id>"
            exit 1
        fi
        if command -v sqlite3 &> /dev/null; then
            echo "=== Valuation Results for $ID ==="
            run_query "SELECT trade_id, npv_base, valuation_ccy, status, asset_class, product_type, support_status, model_name, status_message, error_message FROM valuation_results WHERE run_id = '$ID';"
            echo "=== Risk Results for $ID ==="
            run_query "SELECT trade_id, risk_measure, risk_factor_id, value FROM risk_results WHERE run_id = '$ID';"
            echo "=== Scenario Results for $ID ==="
            run_query "SELECT scenario_name, portfolio_pnl FROM scenario_results WHERE run_id = '$ID';"
            echo "=== VaR Results for $ID ==="
            run_query "SELECT method, confidence_level, var_value, expected_shortfall, scenario_count FROM var_results WHERE run_id = '$ID';"
        elif [ ! -z "$CLI_EXE" ]; then
            echo "Using qrp_cli report for $ID ..."
            "$CLI_EXE" report "$ID"
        else
            echo "sqlite3 and qrp_cli are not available."
            exit 1
        fi
        ;;
    risk)
        if [ -z "$ID" ]; then
            echo "Usage: $0 risk -Id <run_id>"
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
        echo "Usage: $0 [portfolios | trades | runs | results | risk] [-Id <id>] [-DbFile <path>]"
        ;;
esac
