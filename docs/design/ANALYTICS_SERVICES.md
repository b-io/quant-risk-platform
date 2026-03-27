# Analytics Services Design

This document describes the service layer that should sit on top of market objects and built instruments.

## 1. Service philosophy

Analytics should consume:

- a built market snapshot,
- a built instrument or built portfolio representation,
- a request object,
- and return structured output.

This avoids reparsing trades and rebuilding curves inside every analytic.

## 2. Current implementation status

Implemented today:

- `ValuationService`,
- `RiskService`,
- `PnlExplainService`,
- `StressEngine`,
- `MonteCarloEngine`,
- `PricingContext`.

Current limitations:

- instruments are rebuilt inside multiple services,
- risk is still mostly bump-and-revalue,
- explain is still approximate,
- Monte Carlo is a one-step factor simulation rather than a general path framework,
- there is no explicit historical VaR vs parametric VaR split,
- there is no built portfolio cache.

## 3. Target service split

### Pricing

- deterministic valuation,
- structured valuation diagnostics,
- curve and engine provenance.

### Risk and Sensitivities

- **Normalized Result Model**: All risks follow a common sensitivity result schema
  (risk_measure, risk_factor_id, trade_id, etc.).
- **PV01 / DV01**: Delta risk by shifting interest rate nodes by 1bp.
- **Bucketed Risk**: Key-rate sensitivities by shifting nodes individually (using `RF:` identifiers).
- **CS01**: Credit spread risk by shifting CDS quotes.
- **Greeks**: Delta, gamma, vega, theta for supported options.
- **Aggregation**: Grouping by portfolio, book, desk, currency, asset class, and factor family.

### P&L Explain

P&L explain reconciles the change in portfolio value between two dates into interpretable drivers:

- **Carry / Roll-down**: Natural aging of the portfolio.
- **Market Move**: Effect of shifts in curves, spreads, and spots.
- **New Trades / Unwinds**: Impact of portfolio changes.
- **Cash / Fixing**: Effect of realized cashflows.
- **Residual**: Unexplained discrepancy.

### VaR and Stress

- **Historical VaR**: Replaying historical factor returns over current positions.
- **Monte Carlo VaR**: Full path-based simulation.
- **Historical Stress**: Replaying specific historical crisis periods.
- **Hypothetical Stress**: Manual shift-based scenarios.

### Monte Carlo Engine

The engine supports both:

- **One-step factor simulation**: For Greeks and simple VaR.
- **Full Path simulation**: For American options, path-dependent derivatives, and exposure extensions.

## 4. Architectural choice: Handle-based vs Brute-force

**Our approach:** Handle-based reactive risk.

QuantLib objects (Curves, Instruments) are built using `Handle<Quote>` where the quote is a `SimpleQuote`.
This design choice is fundamental to the project.

- **Choice:** Applying shocks by updating `SimpleQuote::setValue()`.
- **Reason:** It triggers QuantLib's internal observer chain. Curves are NOT re-bootstrapped; rather,
  the discount/forward factors are re-evaluated from existing (invalidated) cached values in the term structure.
- **Performance:** Handle updates are near-instantaneous. Valuation becomes the bottleneck (NPV calls),
  not market data updates.
- **Comparison:** Brute-force (rebuilding curves per scenario) is 10x-100x slower depending on the number of instruments
  and curve complexity.

## 5. Immediate priorities

1. add a built-instrument / built-portfolio cache,
2. separate factor definitions from scenario definitions,
3. split Monte Carlo one-step simulation from path simulation,
4. add explicit VaR engines by methodology,
5. upgrade explain from placeholder logic to reconciliation logic.
