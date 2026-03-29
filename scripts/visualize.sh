#!/bin/bash
# visualize.sh - Visualize data from the database

DB_FILE="var/quant_risk_platform.sqlite"

if [ ! -f "$DB_FILE" ]; then
    echo "Database file $DB_FILE not found."
    exit 1
fi

# Check if sqlite3 is installed
if ! command -v sqlite3 &> /dev/null; then
    echo "sqlite3 command not found. Please install it to use this script."
    exit 1
fi

TABLE=${1:-summary}

case $TABLE in
    portfolios)
        echo "=== Portfolios ==="
        sqlite3 -header -column "$DB_FILE" "SELECT portfolio_id, portfolio_name, base_currency FROM portfolios;"
        ;;
    trades)
        PORTFOLIO_ID=$2
        if [ -z "$PORTFOLIO_ID" ]; then
            echo "Usage: $0 trades <portfolio_id>"
            exit 1
        fi
        echo "=== Trades for $PORTFOLIO_ID ==="
        sqlite3 -header -column "$DB_FILE" "SELECT trade_id, book_id, asset_class, product_type, currency, notional FROM trades WHERE portfolio_id = '$PORTFOLIO_ID';"
        ;;
    runs)
        echo "=== Analysis Runs ==="
        sqlite3 -header -column "$DB_FILE" "SELECT run_id, run_type, portfolio_id, snapshot_id, created_at FROM analysis_runs ORDER BY created_at DESC;"
        ;;
    results)
        RUN_ID=$2
        if [ -z "$RUN_ID" ]; then
            echo "Usage: $0 results <run_id>"
            exit 1
        fi
        echo "=== Valuation Results for $RUN_ID ==="
        sqlite3 -header -column "$DB_FILE" "SELECT trade_id, npv_base, valuation_ccy, status FROM valuation_results WHERE run_id = '$RUN_ID';"
        ;;
    risk)
        RUN_ID=$2
        if [ -z "$RUN_ID" ]; then
            echo "Usage: $0 risk <run_id>"
            exit 1
        fi
        echo "=== Risk Results for $RUN_ID ==="
        sqlite3 -header -column "$DB_FILE" "SELECT trade_id, risk_measure, risk_factor_id, value FROM risk_results WHERE run_id = '$RUN_ID';"
        ;;
    summary|*)
        echo "=== Platform Data Summary ==="
        echo "Portfolios:"
        sqlite3 "$DB_FILE" "SELECT count(*) FROM portfolios;"
        echo "Trades:"
        sqlite3 "$DB_FILE" "SELECT count(*) FROM trades;"
        echo "Market Snapshots:"
        sqlite3 "$DB_FILE" "SELECT count(*) FROM market_snapshots;"
        echo "Analysis Runs:"
        sqlite3 "$DB_FILE" "SELECT count(*) FROM analysis_runs;"
        echo ""
        echo "Usage: $0 [portfolios | trades <id> | runs | results <id> | risk <id>]"
        ;;
esac
