### Sample Data Directory

This directory contains JSON-formatted data for testing and demonstration purposes.

#### Folder Structure

- `market/`: Market snapshots (`demo_market.json`) containing quotes, yield curves, and other market state information.
- `portfolios/`: Sample portfolios (`demo_portfolio.json`) with swaps, bonds, FX forwards, and equity spot exposure.
- `regression/`: Golden expected outputs used by the Python demo to detect valuation, risk, stress, P&L explain, and Monte Carlo drift. The top-level demo baseline checks end-to-end stability, while `regression/product_families/` keeps product-family fixtures for rates, FX, and equity.
- `scenarios/`: Factor definitions, factor-to-quote bindings, and predefined factor scenarios (`demo_scenarios.json`) for stress testing and VaR analysis. The demo set covers common first-order stresses across USD/EUR rates, curve shape, basis, FX, equity, volatility, and cross-asset risk-on/risk-off regimes.

#### Usage

These files can be imported into the platform's database using the `qrp_cli` utility or the `init.ps1/sh` script. For example:
```powershell
.\scripts\init.ps1 -BuildDir build\Release-Python -Config Release -Force
```
This script will automatically load the sample files into the default database (`var/quant_risk_platform.sqlite`).
