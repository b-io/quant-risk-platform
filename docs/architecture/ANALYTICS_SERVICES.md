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
- `RevaluationSession`,
- `StressEngine`,
- `MonteCarloEngine`,
- `VarContributionService`,
- `CovarianceEstimator`,
- `PricingContext`.

Current limitations:

- instruments are still rebuilt inside multiple services, although
  `RevaluationSession` now provides a reusable C++ cache for quote updates and
  factor-scenario revaluation with opt-in dependency-graph and impact-diff
  diagnostics,
- risk is still mostly bump-and-revalue,
- explain is deterministic, componentized, persisted, and supports sequential factor revaluation,
- realized cashflow extraction currently recognizes deposit maturities and still needs broader product event sources for
  coupons, fixings, exercises, and settlements,
- Monte Carlo supports horizon-shock and aged-horizon revaluation modes, but it is still factor-shock simulation rather
  than a general multi-step exotic path framework,
- HVaR is implemented as historical scenario replay, while parametric and Monte Carlo VaR are not dedicated service
  methods,
- historical VaR and Expected Shortfall contribution analytics are first-class service outputs, while Monte Carlo and
  parametric contribution decomposition are not dedicated service outputs,
- LSMC exposes a C++-managed American option exercise-policy helper to Python with basis, path, and regression
  diagnostics, and American equity options, Bermudan swaptions, plus callable fixed-rate bonds use the shared
  exercise-policy layer in product paths,
- there is no platform-wide built portfolio cache shared by all analytics.

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
- **Aggregation**: Grouping by portfolio, book, strategy, currency, asset class, and factor family.

### P&L Explain

P&L explain reconciles the change in portfolio value between two dates into interpretable drivers:

- **Carry / Roll-down**: Natural aging of the portfolio.
- **Market Move**: Effect of shifts in curves, spreads, spots, forwards, and volatilities, with sequential factor
  revaluation when factor bindings are available.
- **New Trades / Unwinds**: Impact of portfolio changes.
- **Cash / Fixing**: Effect of realized cashflows.
- **Residual**: Unexplained discrepancy.

The current implementation persists explain runs and components in SQLite. Deposit maturity cash is extracted today;
broader coupon, fixing, exercise, and settlement event sources remain outside current explain coverage.

### VaR and Stress

- **Historical VaR**: Replaying historical factor returns over current positions.
- **Monte Carlo VaR**: Simulation-based loss distributions from factor shocks, with full path/exposure simulation as a
  future extension.
- **Historical Stress**: Replaying specific historical crisis periods.
- **Hypothetical Stress**: Manual shift-based scenarios.

### Monte Carlo Engine

The engine supports:

- **Horizon shock-only simulation**: Apply one correlated factor shock and reprice at the base valuation date.
- **Aged horizon revaluation**: Age to a horizon date, compute frozen aging P&L, apply factor shocks, and report market
  and total P&L diagnostics.

The reusable LSMC module handles exercise-policy examples, Python-facing American option valuation, American equity
option product valuation, Bermudan swaption product valuation, and callable fixed-rate bond issuer exercise through the
shared exercise-policy adapter. Commodity physical-flexibility products still use product-specific approximations. A
general multi-step exposure cube and path-dependent pricing service remains an extension area.

## 4. Architectural choice: Handle-based vs. Brute-force

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

## 5. Current service boundaries

1. Built-instrument and built-portfolio caching are not shared service capabilities.
2. Monte Carlo and parametric VaR contribution decomposition are not yet first-class outputs.
3. Monte Carlo has horizon-shock and aged-horizon modes, but not a general multi-step path/exposure service.
4. LSMC is exposed through a C++-managed helper and shared American equity, Bermudan swaption, and callable bond product
   paths, but the shared exercise-policy layer is not yet wired across every physical-flexibility product.
5. Product event-source integration for realized cashflow explain is limited.
6. Production controls, manifests, lineage reports, and benchmark governance remain hardening areas.
