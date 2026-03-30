# Quant Risk Platform Documentation

This `docs/` tree is the canonical documentation root for the repository.

## Structure

- `docs/design/` — architecture, module boundaries, market-and-curve design, implementation choices, and platform build
  guidance.
- `docs/pricing/` — pricing and market-construction documentation, organized by subdomain: market data, rates, credit,
  and volatility.
- `docs/risk/` — risk measures, explain, scenarios, historical stress, VaR, Monte Carlo, and macro workflow notes.
- `docs/theory/` — mathematical, statistical, portfolio, and asset-pricing foundations used by the implementation docs.
- `docs/roadmap/` — implementation roadmap, review history, and progress tracking.

## Documentation map

### Design

- `docs/design/ANALYTICS_SERVICES.md` — pricing, risk, explain, stress, VaR, and Monte Carlo service boundaries and
  orchestration.
- `docs/design/ARCHITECTURE.md` — end-to-end platform architecture, layering, domain boundaries, and canonical design
  principles.
- `docs/design/CURVE_BOOTSTRAP_DESIGN.md` — bootstrap-specific design note covering reconstruction, handles,
  observability, and curve update semantics.
- `docs/design/IMPLEMENTATION_CHOICES.md` — repository-specific implementation rationale, including QuantLib wrapping,
  abstraction boundaries, and observer semantics.
- `docs/design/INDEX.md` — alphabetical index for the design section and its closest related implementation notes.
- `docs/design/MARKET_AND_CURVES.md` — market schema, convention registry, reusable curves and surfaces, and market
  snapshot design.
- `docs/design/PLATFORM_IMPLEMENTATION_GUIDE.md` — practical implementation workflow, persistence, testing strategy,
  performance considerations, and extension path.
- `docs/design/QUANTLIB_CURVE_API_CHEAT_SHEET.md` — common QuantLib curve objects, helper classes, and query patterns
  mapped to platform usage.

### Pricing

- `docs/pricing/INDEX.md` — alphabetical index for pricing documentation and its sub-sections, plus the preferred
  reading order.
- `docs/pricing/credit/CDS_CURVES_AND_CREDIT_RISK_IN_PRACTICE.md` — CDS intuition, premium and protection legs,
  calibration logic, CS01, and practical credit-risk usage.
- `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md` — credit spread curves, hazard-rate calibration, survival
  probabilities, recovery assumptions, and builder design.
- `docs/pricing/credit/INDEX.md` — alphabetical index for credit pricing and credit-curve documentation.
- `docs/pricing/market-data/INDEX.md` — alphabetical index for market-data schema and reusable market-object
  documentation.
- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — normalized cross-asset market-data schema covering
  rates, credit, FX, equity, commodity, and volatility inputs.
- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — production-oriented rates curve implementation details,
  interpolation choices, extrapolation, and curve-stack design.
- `docs/pricing/rates/INDEX.md` — alphabetical index for rates curve construction, worked examples, implementation
  notes, and supporting assets.
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — rates factor modeling, short-rate and parametric curve
  models, modern practice, and practical model-selection guidance.
- `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md` — mean reversion, time-dependent
  volatility, CIR, Courtadon, Black-Karasinski, calibration trade-offs, and worked numerical examples.
- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — discounting and projection curves, OIS bootstrapping,
  conventions, helpers, and rates risk implications.
- `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — worked rates examples for discount factors, forwards, yield
  types, and recession-driven curve behavior.
- `docs/pricing/volatility/INDEX.md` — alphabetical index for option-pricing foundations and volatility-surface documentation.
- `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md` — European, American, and Bermudan option basics; put-call parity; arbitrage bounds; synthetic positions; and quote-validation logic.
- `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility quote schemas, surface construction, interpolation
  conventions, and risk integration.

### Risk

- `docs/risk/FRONT_OFFICE_AND_RISK_WORKING_PRACTICES.md` — practical workflow note on desk metrics, controls,
  escalation, and explainability expectations.
- `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md` — macro event workflow, trader decision process, and how the platform
  supports scenario design.
- `docs/risk/HISTORICAL_STRESS.md` — historical stress testing framework, event replay design, and scenario-library
  construction.
- `docs/risk/INDEX.md` — alphabetical index for risk, explain, scenario, stress, and simulation documentation.
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

### Theory

- `docs/theory/ASSET_PRICING_FOUNDATIONS.md` — pricing-kernel view of valuation plus CAPM, APT, Black-Scholes-Merton,
  term-structure, and credit foundations.
- `docs/theory/BROWNIAN_MOTION_AND_ITO.md` — Brownian motion, Wiener processes, Itô calculus, and
  stochastic-differential-equation basics.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md` — characteristic functions and a practical introduction to
  Fourier methods in quantitative finance.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md` — moment-generating functions, characteristic
  functions, and convergence notions used in probability and statistics.
- `docs/theory/INDEX.md` — alphabetical index for mathematical, statistical, portfolio, and asset-pricing foundations.
- `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md` — mean-variance portfolio construction, efficient frontiers, and practical
  portfolio-theory limits.
- `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md` — mean reversion, Ornstein-Uhlenbeck intuition, Vasicek,
  Hull-White, CIR, and worked short-rate examples.
- `docs/theory/PROBABILITY_LIMIT_THEOREMS.md` — laws of large numbers, central limit theorem, and the convergence ideas
  behind estimation and simulation.
- `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md` — random variables, expectation, variance, covariance, and core
  probability identities.
- `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md` — statistical distributions, maximum likelihood estimation, and
  asymptotic behavior of estimators.

### Roadmap

- `docs/roadmap/INDEX.md` — alphabetical index for roadmap and status material.
- `docs/roadmap/MASTER_ROADMAP_AND_NEXT_STEPS.md` — canonical implementation roadmap and next-pass coding instructions.
- `docs/roadmap/REVIEW_AND_ROADMAP.md` — deprecated broader review and earlier multi-milestone roadmap kept for
  historical context.
- `docs/roadmap/STATUS.md` — current implementation status against the roadmap and documentation plan.

### Support assets

- `docs/pricing/rates/assets/curve_objects_relationships.png` — diagram of the main curve object relationships used in
  the rates stack.
- `docs/pricing/rates/assets/interpolation_tradeoffs.png` — diagram summarizing interpolation and smoothness trade-offs
  across curve construction choices.

## Reading order for new contributors

1. `docs/design/ARCHITECTURE.md` — high-level architecture, domain boundaries, and canonical design principles.
2. `docs/design/MARKET_AND_CURVES.md` — market schema, reusable curves, and cross-asset market-object design.
3. `docs/design/PLATFORM_IMPLEMENTATION_GUIDE.md` — build sequence, testing approach, and extension path.
4. `docs/design/ANALYTICS_SERVICES.md` — how pricing, risk, explain, and simulation services fit together.
5. `docs/pricing/INDEX.md` — pricing section map and recommended pricing reading order.
6. `docs/risk/INDEX.md` — risk section map and recommended risk reading order.
7. `docs/theory/INDEX.md` — theory section map for mathematical and asset-pricing foundations.
8. `docs/roadmap/STATUS.md` — current implementation status and where to focus next.

## Documentation policy

- Keep `docs/design/ARCHITECTURE.md` as the canonical architecture overview.
- Keep implementation-facing pricing notes under `docs/pricing/` and organize them by subdomain.
- Keep implementation-facing risk notes under `docs/risk/`.
- Keep mathematical foundations under `docs/theory/`.
- Keep roadmap and handoff notes under `docs/roadmap/`.
- When adding a major module, update the relevant index file and `docs/roadmap/STATUS.md`.
- Keep index lists alphabetical and attach a short description to each referenced file.
- Keep deep worked examples and practical implementation notes close to the domain they support rather than in scratch
  folders.
