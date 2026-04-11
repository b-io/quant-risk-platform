### Runtime Data Directory

This directory is used for storing transient runtime data, such as the platform's local database.

#### Files

- `quant_risk_platform.sqlite`: The default SQLite database containing portfolios, trades, market snapshots, and valuation results.

#### Note

The database is typically initialized and managed using the `qrp_cli` utility or the `scripts/init.ps1/sh` script. Ensure this folder has write permissions for the application.
