# Asset-Class Documentation Index

This section contains product-family and asset-class convention chapters. It is
not the home for shared market-state design, general volatility theory, or common
formulas; those belong in `docs/market-data/`, `docs/models/`, and
`docs/reference/`.

The dependency order is:

$$
\text{market data}
\rightarrow
\text{asset-class conventions}
\rightarrow
\text{product representation}
\rightarrow
\text{valuation}
\rightarrow
\text{risk mapping}.
$$

## Section Structure

- `docs/asset-classes/rates/` - discounting, projection, OIS curves, IBOR tenor
  curves, rates products, and term-structure modeling.
- `docs/asset-classes/credit/` - spread measures, survival curves, CDS logic,
  recovery assumptions, and credit-risk interpretation.
- `docs/asset-classes/fx/` - spot, forwards, swaps, NDFs, currency conventions,
  and FX volatility coverage.
- `docs/asset-classes/commodities/` - commodity forwards, futures, delivery
  periods, power, gas, carbon, options, storage, swing, and flexibility.
- `docs/asset-classes/equity/` - equity spot, forwards, futures, dividends,
  borrow, index products, and equity options.

## Recommended Reading Order

1. `docs/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`  
   Establish the normalized market state before reading product-specific
   chapters.

2. `docs/asset-classes/rates/INDEX.md`  
   Rates curves provide the discounting and projection machinery reused across
   the platform.

3. `docs/asset-classes/credit/INDEX.md`  
   Credit adds survival, recovery, default events, and spread-risk semantics.

4. `docs/asset-classes/fx/INDEX.md`  
   FX introduces currency-pair conventions, settlement, forward points, and
   cross-currency discounting.

5. `docs/asset-classes/commodities/INDEX.md`  
   Commodities add delivery-period contracts, physical constraints, carry,
   location, and flexibility.

6. `docs/asset-classes/equity/INDEX.md`  
   Equity products add dividends, borrow, index conventions, and early exercise.

7. `docs/models/INDEX.md`  
   Reusable model families such as volatility surfaces and LSMC should be read
   across asset classes, not as part of one asset class.

## Current Coverage

The repository currently represents and prices product families across rates,
FX, credit, commodities, and equities. The canonical trade model, market-data
foundation, pricing registry, valuation workflows, risk workflows, stress/HVaR,
historical VaR/Expected Shortfall contribution analytics, and PnL explain all
share the same multi-asset coverage.
The Python bindings also expose `RevaluationSession`, which reuses one built
market state and instrument cache for quote updates and factor-scenario
revaluation across those same product families.

Known boundaries are:

- Monte Carlo and parametric VaR contribution decomposition are not yet
  first-class outputs;
- LSMC exercise-policy helpers are exposed and used by American equity options,
  Bermudan swaptions, and callable fixed-rate bonds, but the shared layer is not
  yet wired across all physical-flexibility products;
- shared built-position caching across every analytics service is not yet
  generalized beyond the revaluation-session API;
- production controls and validation reports remain hardening areas.

## Maintenance Rule

When adding an asset-class chapter, place it under the relevant asset-class
folder and link shared vocabulary, formulas, and sources from
`docs/reference/`. Avoid carrying duplicate formula sheets or glossaries inside
asset-class folders.
