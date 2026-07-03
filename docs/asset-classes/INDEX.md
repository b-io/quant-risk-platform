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

## Implementation Sequence

The asset-class expansion should follow the broader platform plan in
`docs/implementation/PHASED_BUILD_PLAN.md`:

1. harden the trade model, result model, risk-factor taxonomy, support flags,
   and golden datasets;
2. build the market-data foundation;
3. implement rates products;
4. add FX products;
5. add credit products;
6. add commodity products;
7. add equity products;
8. complete PnL explain;
9. complete VaR and Expected Shortfall contributions;
10. integrate LSMC as a reusable exercise-policy engine;
11. add production controls.

## Maintenance Rule

When adding an asset-class chapter, place it under the relevant asset-class
folder and link shared vocabulary, formulas, and sources from
`docs/reference/`. Avoid carrying duplicate formula sheets or glossaries inside
asset-class folders.
