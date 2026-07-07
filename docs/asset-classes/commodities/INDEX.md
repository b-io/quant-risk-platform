# Commodities Documentation Index

This section covers commodity market conventions, products, and implementation
implications. It includes power, gas, carbon, delivery-period products, listed
futures and options, and physical flexibility.

Commodities are separated from the shared reference layer because the asset
class introduces conventions and constraints, while formulas and source lists
that apply elsewhere belong in `docs/reference/`.

## Canonical Files

1. `docs/asset-classes/commodities/POWER_SYSTEM_STRUCTURE.md`  
   Physical system drivers, balancing, storage limits, weather, transmission,
   and their implications for price formation.

2. `docs/asset-classes/commodities/MARKET_DATA_AND_TICKERS.md`  
   Canonical quote mapping for power, gas, carbon, delivery periods, futures,
   forwards, and related spreads.

3. `docs/asset-classes/commodities/CURVES_AND_TERM_STRUCTURE.md`  
   Forward versus futures curves, delivery-period averaging, bootstrapping,
   smoothing, bottom-up shape, and curve-construction constraints.

4. `docs/asset-classes/commodities/OPTIONS_VOL_MONTE_CARLO.md`  
   Commodity options, Black-76, Bachelier, implied volatility, Monte Carlo, and
   path-generation considerations.

5. `docs/asset-classes/commodities/STRUCTURED_PRODUCTS_AND_FLEXIBILITY.md`  
   Storage, swing contracts, thermal dispatch, hydro reservoirs, batteries,
   PPAs, virtual power plants, and LSMC-style valuation.

6. `docs/asset-classes/commodities/RISK_BACKTESTING_GOVERNANCE.md`  
   Commodity risk factors, PnL, sensitivities, VaR, Expected Shortfall, stress
   testing, backtesting, and validation.

## Shared References

- `docs/reference/LEXIS.md` contains commodity terms such as baseload,
  peakload, delivery period, gas storage, swing contract, spark spread, and EUA.
- `docs/reference/FORMULAS.md` contains delivery-period averaging, curve
  fitting, spark-spread, option, risk, and LSMC formulas.
- `docs/reference/SOURCES.md` contains public exchange, transparency-platform,
  vendor, and modeling references.

## Platform Mapping

Commodity implementation should preserve the same platform layers used by other
asset classes:

$$
\text{quotes}
\rightarrow
\text{validated market state}
\rightarrow
\text{curves and surfaces}
\rightarrow
\text{pricing}
\rightarrow
\text{risk and PnL explain}.
$$

The commodity-specific additions are delivery-period aggregation, physical
location, product unit, delivery calendar, storage or exercise constraints, and
linked risk factors such as power, gas, carbon, weather, and outages.

## Product Coverage

Commodity product coverage is organized around these product families:

1. commodity spot and forward exposure;
2. futures;
3. futures strips;
4. options on futures;
5. calendar spread options;
6. swing and storage contracts using LSMC;
7. multi-commodity spread options.

Current platform coverage:

- The canonical portfolio DTO supports commodity spot, forwards, futures,
  futures strips, options on futures, calendar spread options, and swing
  contracts.
- Spot, forward, future, strip, future-option, and calendar-spread-option
  valuation is wired into the pricing registry.
- Swing valuation is available as an intrinsic exercise-envelope approximation
  and remains marked partially supported until the full storage/LSMC engine is
  promoted into product pricing.
- Multi-commodity spread options are outside current product coverage.
- The portfolio-backed golden fixtures under `data/portfolios/` link
  commodity products to book-level structural coverage.

## Maintenance Rule

Keep commodity chapters focused on conventions, market objects, products,
valuation methods, and risk mapping. Shared definitions, formulas, and source
lists should be maintained in `docs/reference/`.
