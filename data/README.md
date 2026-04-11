### Sample Data Directory

This directory contains JSON-formatted data for testing and demonstration purposes.

#### Folder Structure

- `market/`: Market snapshots (`demo_market.json`) containing quotes, yield curves, and other market state information.
- `portfolios/`: Sample portfolios (`demo_portfolio.json`) with various financial trades like Swaps, FX Forwards, Options, etc.
- `scenarios/`: Predefined market scenarios and historical returns (`demo_scenarios.json`) for stress testing and VaR analysis.

#### Usage

These files can be imported into the platform's database using the `qrp_cli` utility or the `init.ps1/sh` script. For example:
```powershell
.\scripts\init.ps1 -BuildDir build\Release-Python -Config Release -Force
```
This script will automatically load the sample files into the default database (`var/quant_risk_platform.sqlite`).
