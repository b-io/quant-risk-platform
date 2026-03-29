#!/bin/bash
# init.sh - Initialize the database and load sample data

# Default build directory
BUILD_DIR="build/Debug"
CLI_EXE="./$BUILD_DIR/qrp_cli"
DB_FILE="var/quant_risk_platform.sqlite"

# Ensure var directory exists
mkdir -p var

if [ -f "$DB_FILE" ]; then
    echo "WARNING: Database $DB_FILE already exists."
    read -p "Do you want to delete and recreate it? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Deleting $DB_FILE..."
        rm "$DB_FILE"
    else
        echo "Initialization cancelled."
        exit 0
    fi
fi

echo "Initializing database schema..."
$CLI_EXE init-db

echo "Importing sample market data..."
$CLI_EXE import-market data/market/base_market.json

echo "Importing sample portfolios..."
$CLI_EXE import-portfolio data/portfolios/demo_macro_book.json

echo "Importing sample scenarios..."
$CLI_EXE import-scenarios data/scenarios/sample_scenarios.json

echo "Data initialization complete!"
$CLI_EXE list
