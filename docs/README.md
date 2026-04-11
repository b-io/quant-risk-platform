# Quant Risk Platform Documentation

This `docs/` tree is the canonical documentation root for the repository.

## Structure

- `docs/design/` — architecture, module boundaries, market-and-curve design, persistence, lineage, performance, and platform build guidance.
- `docs/pricing/` — pricing and market-construction documentation, organized by subdomain: market data, rates, credit, and volatility.
- `docs/risk/` — risk measures, P&L explain, scenarios, stress testing, VaR, Expected Shortfall, Monte Carlo, and macro workflow notes.
- `docs/theory/` — mathematical, stochastic-process, statistical, portfolio, and asset-pricing foundations used by the implementation docs.
- `docs/roadmap/` — implementation roadmap, review history, and progress tracking.

## Documentation map

### Design

- `docs/design/ANALYTICS_SERVICES.md` — pricing, risk, explain, stress, VaR, and Monte Carlo service boundaries and orchestration.
- `docs/design/ARCHITECTURE.md` — end-to-end platform architecture, layering, domain boundaries, and canonical design principles.
- `docs/design/CURVE_BOOTSTRAP_DESIGN.md` — bootstrap-specific design note covering reconstruction, handles, observability, and curve update semantics.
- `docs/design/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md` — snapshots, schema evolution, reconciliation, and the control requirements around stored analytics.
- `docs/design/IMPLEMENTATION_CHOICES.md` — repository-specific implementation rationale, including QuantLib wrapping, abstraction boundaries, and observer semantics.
- `docs/design/MARKET_AND_CURVES.md` — market schema, convention registry, reusable curves and surfaces, and market snapshot design.
- `docs/design/PLATFORM_IMPLEMENTATION_GUIDE.md` — practical implementation workflow, persistence, testing strategy, performance considerations, and extension path.
- `docs/design/PYTHON_CPP_PERFORMANCE_AND_BINDINGS.md` — language split, profiling, memory-layout concerns, vectorization, numerical stability, and binding design.
- `docs/design/QUANTLIB_CURVE_API_CHEAT_SHEET.md` — common QuantLib curve objects, helper classes, and query patterns mapped to platform usage.

### Pricing

- `docs/pricing/INDEX.md` — index for pricing documentation and its sub-sections, plus the preferred reading order.
- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — normalized cross-asset market-data schema covering rates, credit, FX, equity, commodity, and volatility inputs.
- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — discounting and projection curves, OIS bootstrapping, conventions, helpers, and rates risk implications.
- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — production-oriented rates curve implementation details, interpolation choices, extrapolation, and curve-stack design.
- `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — worked rates examples for discount factors, forwards, yield types, and curve behavior.
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — rates factor modeling, short-rate and parametric curve models, and practical model-selection guidance.
- `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md` — mean reversion, time-dependent volatility, CIR, Black-Karasinski, calibration trade-offs, and numerical examples.
- `docs/pricing/credit/BOND_SPREADS_AND_DEFAULT_RISK.md` — bond price and yield, credit spread, Z-spread, asset-swap spread, CDS spread, hazard rates, survival probabilities, recovery, CS01, and jump-to-default.
- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` — CDS premium and protection legs, hazard and survival curves, calibration logic, CS01, basis interpretation, and production credit analytics.
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — credit spread curves, hazard-rate calibration, survival probabilities, recovery assumptions, and builder design.
- `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md` — option-pricing identities, put-call parity, arbitrage bounds, synthetic positions, and quote-validation logic.
- `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md` — Black-Scholes-Merton, Black 76, exercise styles, and numerical-method trade-offs.
- `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md` — delta, gamma, vega, theta, rho, portfolio aggregation, non-linear exposures, and scenario revaluation versus Greeks-based approximation.
- `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md` — local volatility, stochastic volatility, SABR, and calibration trade-offs.
- `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility quote schemas, surface construction, interpolation conventions, and risk integration.

### Risk

- `docs/risk/RISK_MEASURES_AND_EXPLAIN.md` — compact overview of sensitivities, explain concepts, and reporting language.
- `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md` — canonical factor vocabulary, factor mapping, trade-to-portfolio aggregation, and attribution design.
- `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md` — daily explain decomposition, carry, roll-down, market-move P&L, trade-flow effects, and residual control.
- `docs/risk/HISTORICAL_STRESS.md` — historical-stress construction, scenario mapping, and stressed P&L interpretation.
- `docs/risk/VAR.md` — VaR and ES methodologies, assumptions, and output requirements.
- `docs/risk/VAR_STRESS_BACKTESTING_AND_AGGREGATION.md` — integration of VaR, stress, backtesting, and hierarchical aggregation.
- `docs/risk/MONTE_CARLO_FOUNDATIONS.md` — LLN, CLT, error bars, and Monte Carlo consistency.
- `docs/risk/MONTE_CARLO_IMPLEMENTATION.md` — path simulation, performance engineering, and scenario aggregation.
- `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md` — top-down macro workflow, trade expression, and platform support.
- `docs/risk/MACRO_INDICATORS_AND_INDEX_CONSTRUCTION.md` — macro indicator formulas and index-construction conventions.
- `docs/risk/MACRO_REGIMES_AND_EVENT_FLOWS.md` — regime logic, event flows, and macro-to-market transmission.

### Theory

- `docs/theory/ASSET_PRICING_FOUNDATIONS.md` — discounting, no-arbitrage, factor models, and reduced-form credit foundations.
- `docs/theory/BROWNIAN_MOTION_AND_ITO.md` — Brownian motion, Itô processes, Itô's lemma, and standard stochastic dynamics.
- `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md` — Ornstein-Uhlenbeck logic, Vasicek, Hull-White, CIR, Black-Karasinski, and short-rate interpretation.
- `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md` — portfolio mean-variance foundations and implementation limits.
- `docs/theory/PROBABILITY_LIMIT_THEOREMS.md` — LLN, CLT, convergence, and statistical consistency.
- `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md` — expectation, variance, covariance, and conditioning.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md` and `docs/theory/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md` — transform methods, MGFs, convergence, and distributional diagnostics.
- `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md` — reference distributions, likelihoods, MLE, and asymptotic inference.

## Reading path

A compact reading path for the repository is:

1. `docs/design/ARCHITECTURE.md`
2. `docs/design/MARKET_AND_CURVES.md`
3. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
4. `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md`
5. `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md`
6. `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md`
7. `docs/risk/HISTORICAL_STRESS.md`
8. `docs/risk/VAR.md`
9. `docs/theory/BROWNIAN_MOTION_AND_ITO.md`
10. `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md`

## Documentation principles

- Use book-style explanations rather than task-specific wording.
- Prefer stable notation and define symbols before using them.
- Keep formulas close to their interpretation and implementation meaning.
- When a formula appears, explain what each variable means and what object is being shocked, calibrated, or repriced.
- Keep worked examples in the canonical chapter where the concept is introduced.
