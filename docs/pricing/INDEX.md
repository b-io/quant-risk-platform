# Pricing Documentation Index

This folder contains market-construction and pricing-facing documentation.

## Section structure

- `docs/pricing/credit/` — credit spread, hazard-rate, survival-curve, and CDS implementation notes.
- `docs/pricing/market-data/` — cross-asset market-data schema, factor normalization, and reusable market objects.
- `docs/pricing/rates/` — rates curve construction, discounting, projection, worked examples, and model choices.
- `docs/pricing/volatility/` — option-pricing foundations plus volatility term structures and surfaces across asset
  classes.

## Key documents

- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` — CDS intuition, premium and protection legs,
  calibration logic, CS01, and practical credit-risk usage.
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — credit spread curves, hazard-rate calibration, survival
  probabilities, recovery assumptions, and builder design.
- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — normalized cross-asset market-data schema covering
  rates, credit, FX, equity, commodity, and volatility inputs.
- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — production-oriented rates curve implementation details,
  interpolation choices, extrapolation, and curve-stack design.
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — rates factor modeling, short-rate and parametric curve
  models, modern practice, and practical model-selection guidance.
- `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md` — mean reversion, time-dependent
  volatility, positivity trade-offs, market calibration limits, and worked short-rate examples.
- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — discounting and projection curves, OIS bootstrapping,
  conventions, helpers, and rates risk implications.
- `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — worked rates examples for discount factors, forwards, yield
  types, and recession-driven curve behavior.
- `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md` — European, American, and Bermudan
  option basics; put-call parity; arbitrage bounds; synthetic positions; and quote-validation logic.
- `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility quote schemas, surface construction, interpolation
  conventions, and risk integration.

## Recommended reading order

1. `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — normalized market-state schema and reusable market
   objects.
2. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — core discounting and projection-curve construction
   concepts.
3. `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — worked examples that make the rates material concrete.
4. `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — implementation detail for production curve stacks and
   interpolation choices.
5. `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — broader rates model landscape, including dynamic factor
   choices and modern practice.
6. `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md` — detailed term-structure model
   trade-offs, calibration limits, and numerical examples.
7. `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — extension from rates to credit survival modeling and recovery
   assumptions.
8. `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` — CDS-specific mechanics and risk interpretation.
9. `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md` — option-payoff foundations, parity,
   bounds, carry adjustments, and implementation controls.
10. `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility-surface design and cross-asset option-risk
    integration.

## Maintenance rule

When adding a new pricing topic, place it inside the most specific subfolder available rather than flattening the
section again.
