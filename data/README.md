### Sample Data Directory

This directory contains JSON-formatted data for testing and demonstration purposes.

#### Folder Structure

- `market/`: Market snapshots (`demo_market.json`) containing quotes, yield curves, and other market state information.
- `portfolios/`: Sample portfolios. `demo_portfolio.json` is the stable numeric demo baseline; the generated `*_portfolio.json` files provide model and thematic portfolios for book-level structural golden coverage.
- `regression/`: Golden expected outputs used by the Python demo to detect valuation, risk, stress, P&L explain, and Monte Carlo drift. The top-level demo baseline checks end-to-end stability, `regression/product_families/` keeps product-family fixtures for product-level coverage, and `regression/portfolio_fixtures/` links portfolios to expected book/product/support coverage before numeric golden values are regenerated.
- `scenarios/`: Factor definitions, factor-to-quote bindings, and predefined factor scenarios (`demo_scenarios.json`) for stress testing and VaR analysis. The demo set binds every quote used by the model/thematic portfolios and covers common first-order stresses across USD/EUR rates, curve shape, basis, credit, FX, equity, commodity, volatility, and cross-asset risk-on/risk-off regimes.

The generated portfolio JSON includes explicit `portfolio -> books -> trades` metadata and keeps the canonical trade list as `portfolio -> trades`, with each trade carrying the required `book` field. Book-level P&L and risk are obtained by grouping trades by that field, which fits the current DTO while making book ownership visible in the data.

The basic model portfolio ladder is:

- `liquidity_reserve_model_portfolio.json` - 100 / 0
- `cash_plus_model_portfolio.json` - 95 / 5
- `defensive_income_model_portfolio.json` - 90 / 10
- `conservative_income_model_portfolio.json` - 80 / 20
- `cautious_model_portfolio.json` - 70 / 30
- `stable_balanced_model_portfolio.json` - 60 / 40
- `balanced_core_model_portfolio.json` - 50 / 50
- `balanced_growth_model_portfolio.json` - 40 / 60
- `growth_model_portfolio.json` - 30 / 70
- `high_growth_model_portfolio.json` - 20 / 80
- `aggressive_growth_model_portfolio.json` - 10 / 90
- `adventurous_model_portfolio.json` - 0-5 / 95-100, represented at 5 / 95 in the sample mix

The thematic variants are `stable_balanced_rates_hedged`, `cautious_fx_carry`,
`conservative_credit_income`, `balanced_multi_asset_income`,
`growth_global_macro`, `high_growth_equity_volatility`, and
`adventurous_commodity_volatility`. Refresh these files with
`python data/generate_portfolio_fixtures.py`.

#### Usage

These files can be imported into the platform's database using the `qrp_cli` utility or the `init.ps1/sh` script. For example:
```powershell
.\scripts\init.ps1 -BuildDir build\dev -Config RelWithDebInfo -Force
```
This script will automatically load the sample files into the default database (`var/quant_risk_platform.sqlite`).
