# Pricing Documentation Index

This folder contains market-construction and pricing-facing documentation.

## Section structure

- `docs/pricing/market-data/` — cross-asset market-data schema, factor normalization, and reusable market objects.
- `docs/pricing/rates/` — rates curve construction, discounting, projection, and rates-specific pricing inputs.
- `docs/pricing/credit/` — credit-spread, hazard-rate, and survival-curve design notes.
- `docs/pricing/volatility/` — volatility term structures and surfaces across asset classes.

## Key documents

- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`
- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`
- `docs/pricing/volatility/VOLATILITY_SURFACES.md`

## Recommended reading order

1. `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`
2. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
3. `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`
4. `docs/pricing/volatility/VOLATILITY_SURFACES.md`

## Maintenance rule

When adding a new pricing topic, place it inside the most specific subfolder available rather than flattening the
section again.
