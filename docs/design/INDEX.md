# Design Documentation Index

## Canonical platform design

- `docs/design/ANALYTICS_SERVICES.md` — pricing, risk, explain, stress, VaR, and Monte Carlo service boundaries and
  orchestration.
- `docs/design/ARCHITECTURE.md` — end-to-end platform architecture, layering, domain boundaries, and canonical design
  principles.
- `docs/design/CURVE_BOOTSTRAP_DESIGN.md` — bootstrap-specific design note covering reconstruction, handles,
  observability, and curve update semantics.
- `docs/design/IMPLEMENTATION_CHOICES.md` — repository-specific implementation rationale, including QuantLib wrapping,
  abstraction boundaries, and observer semantics.
- `docs/design/MARKET_AND_CURVES.md` — market schema, convention registry, reusable curves and surfaces, and market
  snapshot design.
- `docs/design/PLATFORM_IMPLEMENTATION_GUIDE.md` — practical implementation workflow, persistence, testing strategy,
  performance considerations, and extension path.
- `docs/design/QUANTLIB_CURVE_API_CHEAT_SHEET.md` — common QuantLib curve objects, helper classes, and query patterns
  mapped to platform usage.

## Closely related implementation notes

### Pricing documentation

- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` — CDS intuition, premium and protection legs,
  calibration logic, CS01, and practical credit-risk usage.
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — credit spread curves, hazard-rate calibration, survival
  probabilities, recovery assumptions, and builder design.
- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — normalized cross-asset market-data schema covering
  rates, credit, FX, equity, commodity, and volatility inputs.
- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — production-oriented rates curve implementation details,
  interpolation choices, extrapolation, and curve-stack design.
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — rates factor modeling, short-rate and parametric curve
  models, and practical model-selection guidance.
- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — discounting and projection curves, OIS bootstrapping,
  conventions, helpers, and rates risk implications.
- `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — worked rates examples for discount factors, forwards, yield
  types, and recession-driven curve behavior.
- `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md` — Black-Scholes-Merton, Black 76, European,
  American, and Bermudan exercise, plus numerical-method trade-offs.
- `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md` — local vol, stochastic vol, SABR,
  callable-product model choices, calibration trade-offs, and modern desk practice.
- `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility quote schemas, surface construction, interpolation,
  no-arbitrage checks, sticky rules, and risk integration.

### Risk documentation

- `docs/risk/FRONT_OFFICE_AND_RISK_WORKING_PRACTICES.md` — practical workflow note on desk metrics, controls,
  escalation, and explainability expectations.
- `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md` — macro event workflow, trader decision process, and how the platform
  supports scenario design.
- `docs/risk/HISTORICAL_STRESS.md` — historical stress testing framework, event replay design, and scenario-library
  construction.
- `docs/risk/MACRO_INDICATORS_AND_INDEX_CONSTRUCTION.md` — macro indicator taxonomy, aggregation logic, normalization
  choices, and index-construction patterns.
- `docs/risk/MACRO_REGIMES_AND_EVENT_FLOWS.md` — macro regime maps, GDP and PMI relationships, and event-flow logic for
  scenario interpretation.
- `docs/risk/MONTE_CARLO.md` — risk-engine view of Monte Carlo simulation, simulation architecture, and why LLN and CLT
  matter in practice.
- `docs/risk/MONTE_CARLO_FOUNDATIONS.md` — LLN, CLT, error bars, confidence intervals, and variance-reduction
  foundations for simulation.
- `docs/risk/MONTE_CARLO_IMPLEMENTATION.md` — path generation, book aggregation, parallel execution, and production
  implementation trade-offs for Monte Carlo.
- `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md` — factor definitions, bucketing, aggregation, and attribution design
  across pricing and risk.
- `docs/risk/RISK_MEASURES_AND_EXPLAIN.md` — PV01, DV01, CS01, P&L explain, attribution concepts, and reporting
  structure.
- `docs/risk/TIME_SERIES_AND_SCENARIOS.md` — historical data handling, factor moves, and scenario-construction workflow.
- `docs/risk/VAR.md` — historical, parametric, and Monte Carlo Value-at-Risk methods and implementation considerations.

### Theory foundations

- `docs/theory/ASSET_PRICING_FOUNDATIONS.md` — pricing-kernel view of valuation plus CAPM, APT, Black-Scholes-Merton,
  term-structure, and credit foundations.
- `docs/theory/BROWNIAN_MOTION_AND_ITO.md` — Brownian motion, Wiener processes, Itô calculus, and
  stochastic-differential-equation basics.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md` — characteristic functions and a practical introduction to
  Fourier methods in quantitative finance.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md` — moment-generating functions, characteristic
  functions, and convergence notions used in probability and statistics.
- `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md` — mean-variance portfolio construction, efficient frontiers, and practical
  portfolio-theory limits.
- `docs/theory/PROBABILITY_LIMIT_THEOREMS.md` — laws of large numbers, central limit theorem, and the convergence ideas
  behind estimation and simulation.
- `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md` — random variables, expectation, variance, covariance, and core
  probability identities.
- `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md` — statistical distributions, maximum likelihood estimation, and
  asymptotic behavior of estimators.
