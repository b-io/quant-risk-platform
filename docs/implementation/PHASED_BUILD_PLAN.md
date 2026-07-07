# Phased Build Plan

The platform should be extended by market conventions and risk factors before
product coverage. This order keeps valuation, risk, explain, and persistence
coherent as new instruments are added.

The governing dependency is:

$$
\text{trade model}
\rightarrow
\text{market state}
\rightarrow
\text{risk-factor taxonomy}
\rightarrow
\text{asset-class products}
\rightarrow
\text{risk and controls}.
$$

Each phase should leave behind regression data, diagnostics, and documentation
that make the next phase an extension of the same system rather than a special
case.

## Phase 1: Core Platform Hardening

This phase defines the canonical trade and result contracts.

Deliverables:

- Normalize products around one trade model: `Trade`, `ProductType`,
  `AssetClass`, `Payoff`, `Schedule`, `Currency`, `Book`, `Counterparty`, and
  `Maturity`.
- Add a structured `PricingResult` model containing NPV, currency, model,
  curve identifiers, valuation date, risk tags, and diagnostics.
- Add a unified `RiskFactorId` taxonomy for rates curve nodes, FX spot, equity
  spot, credit spread, hazard rate, volatility point, and commodity forward
  node.
- Add product support status flags: `Supported`, `PartiallySupported`, and
  `Unsupported`, with explicit failure reasons.
- Create golden regression datasets for each product family.

Validation standard:

- A supported trade must identify its market inputs, pricing model, output
  currency, risk-factor tags, and regression fixture.
- An unsupported trade must fail with a stable diagnostic rather than an
  implicit exception or silent zero value.

Current implementation checkpoint:

- The canonical domain model includes `Trade`, `Portfolio`, `TradeType`,
  `ProductType`, `AssetClass`, and `SupportStatus` values shared by JSON
  loading, pricing, persistence, CLI, Python bindings, and tests.
- Normalized valuation, risk, stress, Monte Carlo, PnL explain, and optimization
  result structures exist, with persisted application workflows for valuation,
  risk, HVaR, and PnL explain.
- Product support profiles expose `Supported`, `PartiallySupported`,
  `Unsupported`, and `Failed` states with model names and diagnostic messages.
- Portfolio-backed golden fixtures and integration tests cover the supported
  multi-asset product set; the remaining hardening work is richer schema
  migration, validation diagnostics, and built-position caching.

## Phase 2: Market Data Foundation

This phase makes market data reusable across products and risk workflows.

Deliverables:

- Rates: discount curves, forward curves, OIS curves, IBOR tenor curves, and
  later inflation curves.
- FX: spot rates, forward points, domestic and foreign discount curves, and FX
  volatility surfaces.
- Credit: issuer curves, CDS spread curves, hazard curves, recovery rates, and
  bond curves.
- Equity: spot prices, dividend curves, borrow or funding curves, and equity
  volatility surfaces.
- Commodities: futures curves, convenience-yield or proxy-carry inputs, and
  commodity volatility surfaces.
- Market snapshot versioning, quote source and provenance, stale quote checks,
  and missing quote diagnostics.

Validation standard:

- A market snapshot must be replayable from stored quote identities,
  conventions, timestamps, source policy, and calibration configuration.
- Missing or stale inputs should block valuation when no documented fallback
  exists.

Current implementation checkpoint:

- `MarketQuote`, `CurveSpec`, `MarketSnapshot`, factor definitions, and factor
  bindings carry quote identity, source/provenance fields, risk-factor IDs,
  curve purpose, and market metadata.
- Rates curves are bootstrapped from QuantLib helpers, with a rates convention
  registry and reusable `SimpleQuote` handles held in `MarketState`.
- Scenario, risk, stress/HVaR, and PnL explain workflows reuse the same
  factor-bound quote handles and restore the base market state after shocks.
- Market snapshots, scenario sets, factors, and factor bindings are persisted in
  SQLite. Typed credit, volatility, commodity, and equity market-object builders
  remain the main Phase 2 hardening gap.

## Phase 3: Common Rates Products

Rates products should be implemented before exotic expansion because rates
curves are shared dependencies for most other asset classes.

Deliverables:

- Cash and deposit instruments.
- Forward rate agreements.
- Interest-rate futures.
- Vanilla fixed-float swaps.
- OIS swaps.
- Fixed-rate bonds.
- Floating-rate notes.
- Caps and floors.
- European swaptions.
- Bermudan swaptions using LSMC.

Validation standard:

- Curve construction, product valuation, cashflow generation, sensitivities,
  and regression tests should use the same market-state and schedule objects.
- OIS discounting and projection curves should remain distinct.

Current implementation checkpoint:

- Deposits, FRAs, interest-rate futures, vanilla swaps, OIS swaps, fixed-rate
  bonds, floating-rate notes, caps/floors, European swaptions, and Bermudan
  swaptions are represented in the canonical portfolio DTO.
- These products are wired into the product-pricing registry and covered by the
  multi-asset demo portfolio and portfolio-backed golden fixtures.
- Rates valuation uses bootstrapped discount/projection curves, quote handles,
  and volatility quotes where applicable. PnL explain currently extracts
  realized cash for deposit maturities.
- Remaining rates work is mainly convention hardening: schedule generation,
  calendars, day-count defaults, coupon/fixing event extraction, and reusable
  built-position caching.

## Phase 4: FX Products

FX support should begin with linear exposure and then move to volatility and
surface risk.

Deliverables:

- FX spot exposure.
- FX forwards.
- FX swaps.
- Non-deliverable forwards.
- Vanilla European FX options.
- FX option Greeks and volatility-surface risk.
- Later products such as barriers, digitals, and TARFs only after vanilla
  options and surface risk are stable.

Validation standard:

- Currency-pair conventions, spot lag, settlement currency, forward points, and
  discount-curve selection must be explicit in trade and market data.

Current implementation checkpoint:

- FX spot, forwards, swaps, NDFs, and vanilla European FX options are represented
  in the canonical portfolio DTO and pricing registry.
- FX pricing consumes spot rates, forward points, domestic/foreign discounting
  inputs, and volatility quotes where applicable.
- FX spot, forward-point, and volatility risk-factor IDs are available for
  deterministic risk, scenario replay, HVaR, and PnL explain market-move
  attribution.
- Barriers, digitals, TARFs, richer smile/surface conventions, and broader
  settlement/fixing event extraction remain future FX extensions.

## Phase 5: Credit Products

Credit support should separate cash-credit spread measures from reduced-form
default modeling.

Deliverables:

- Credit bonds with spread discounting.
- Single-name CDS.
- CDS indices.
- Credit curve bootstrapping from CDS spreads.
- Bond CS01 and spread duration.
- CDS options and credit index options. Code names should prefer `CDSOption`
  or `CreditIndexOption` over ambiguous labels such as credit swaption.
- Later CDO and tranche products only after CDS and index infrastructure are
  stable.

Validation standard:

- Recovery assumptions, reference entity, issuer identity, default event
  conventions, accrual-on-default logic, and curve calibration inputs must be
  explicit.

Current implementation checkpoint:

- Credit bonds, single-name CDS, CDS indices, CDS options, and credit index
  options are represented in the canonical portfolio DTO and pricing registry.
- Credit products consume spread, recovery, discount, and spread-volatility
  inputs from market snapshots and expose credit factor bindings for CS01-style
  risk.
- The first implementation uses deterministic spread/recovery inputs and simple
  spread or hazard interpolation from live quote handles when multiple tenors
  are available.
- CDO/tranche products, detailed accrual-on-default conventions, counterparty
  credit risk, and richer typed credit-curve builders remain outside current
  scope.

## Phase 6: Commodities

Commodity support should distinguish financial commodity exposure from
delivery-period and physical-flexibility products.

Deliverables:

- Commodity spot and forward exposure.
- Futures.
- Futures strips.
- Options on futures.
- Calendar spread options.
- Swing and storage contracts using LSMC.
- Later multi-commodity spread options.

Validation standard:

- Delivery period, location, grade, unit, calendar, settlement rule, and
  aggregation method must be part of the canonical product or market-data
  representation.

Current implementation checkpoint:

- `CommoditySpotTrade`, `CommodityForwardTrade`, `CommodityFutureTrade`,
  `CommodityFutureStripTrade`, `CommodityFutureOptionTrade`,
  `CommodityCalendarSpreadOptionTrade`, and `CommoditySwingTrade` are part of
  the canonical portfolio DTO and product-pricing registry.
- Spot, forwards, futures, strips, options on futures, and calendar spread
  options have deterministic pricing support from quote handles and configured
  discount curves.
- Swing contracts are available through an intrinsic exercise-envelope
  approximation and are reported as partially supported until the full storage
  and LSMC exercise engine is promoted into the product path.
- The portfolio-backed structural golden set includes commodity coverage through
  the model ladder and thematic portfolios, notably Growth Global Macro and
  Adventurous Commodity Volatility, with current regression coverage maintained
  through the portfolio-backed golden fixtures.

## Phase 7: Equity Products

Equity support should combine clean linear products with dividend, borrow, and
exercise modeling.

Deliverables:

- Equity spot exposure.
- Equity forwards and futures.
- European equity and index options.
- American equity options using LSMC or finite differences.
- Dividend curve and borrow curve support.
- Later baskets, Asians, and barriers.

Validation standard:

- Dividend treatment, borrow or funding assumption, corporate-action handling,
  exercise policy, and volatility-surface convention must be explicit.

Current implementation checkpoint:

- `EquitySpotTrade`, `EquityForwardTrade`, `EquityFutureTrade`, and
  `EquityOptionTrade` are part of the canonical portfolio DTO and
  product-pricing registry.
- Equity spot, forwards, futures, European options, and American options are
  priced from spot, discount, dividend-yield, borrow, futures, and volatility
  quotes where applicable.
- European options use a Black-Scholes cost-of-carry formula. American options
  use a recombining binomial exercise tree, which keeps early exercise explicit
  while leaving LSMC or finite-difference replacement as a later model upgrade.
- The portfolio-backed structural golden set includes equity coverage through
  the model ladder and thematic portfolios, notably Growth Global Macro and High
  Growth Equity Volatility.

## Phase 8: PnL Explain

PnL explain should become a first-class analytics workflow rather than a
byproduct of valuation.

Deliverables:

- Dedicated PnL explain unit tests.
- Realized cash PnL from coupons, fixings, maturities, option exercises, and
  settlement cashflows.
- Factor-by-factor explain using sequential revaluation.
- Components for carry, roll-down, market move, cash, trade activity, FX
  translation, model or configuration change, and residual.
- Dedicated persistence tables for PnL explain results.
- Reconciliation checks requiring total PnL to equal the sum of components plus
  residual.

Validation standard:

- Explain results must reconcile numerically and identify residuals by trade,
  book, asset class, currency, and risk-factor group.

Current implementation checkpoint:

- `PnlExplainService` now produces trade-level explain rows with carry,
  roll-down, market move, realized cash, trade activity, FX translation,
  model/configuration change, and residual components.
- Market move can be decomposed by risk factor through sequential full
  revaluation of factor-bound quotes. When factor definitions or bindings are
  absent, the service falls back to aggregate market-move revaluation.
- Deposit maturity principal and simple ACT/360 interest are recognized as
  realized cash when maturity falls between the previous and current snapshots.
- PnL explain results and components are persisted in dedicated SQLite tables
  and exposed through the application workflow and `run-pnl-explain` CLI
  command.
- Reconciliation diagnostics are stored per trade, with residual components
  tagged by asset class, book, currency, product type, and reconciliation
  status.
- Roll-down is currently reported as a separate zero component while
  frozen-market aging is carried in the carry line; trade activity, FX
  translation, and model-change lines are explicit zero components until
  separate trade-population, reporting-currency, and model-version inputs are
  introduced.

## Phase 9: VaR And ES Contributions

VaR and Expected Shortfall should support contribution analysis as well as
portfolio totals.

Deliverables:

- Component VaR and component ES by trade.
- Aggregation by book, strategy, currency, asset class, and risk factor.
- Marginal VaR by bumping or removing factor groups.
- Incremental VaR for new trades.
- Persistence for VaR paths, path-level PnL, and contribution results.
- Reports for top contributors and concentration risk.

Validation standard:

- Contribution calculations must state sign convention, horizon, confidence
  level, scenario set, aggregation rule, and residual or approximation error.

Current implementation checkpoint:

- Historical VaR is available as an application workflow over stored scenario
  sets, persisted aggregate VaR/ES metrics, path-level trade PnL rows, and run
  reports.
- Monte Carlo simulation results include aggregate VaR and Expected Shortfall
  metrics for one-step factor simulations.
- Historical VaR/ES contribution analytics are first-class C++ and Python
  service outputs for trade, book, strategy, currency, asset-class, and
  risk-factor aggregation.
- Component VaR/ES uses the positive-loss convention. Component VaR is read from
  the portfolio VaR scenario; component ES is averaged over the portfolio tail.
- Standalone, incremental, and marginal rows are calculated per aggregation
  group. Marginal rows currently use the remove-group approximation.
- SQLite persists `var_scenario_pnls` and `var_contributions`, including
  sign-convention, confidence-level, tail-count, aggregation-rule,
  calculation-method, concentration-share, and residual metadata.
- Top-contributor helpers and `python/examples/demo_platform.py` demonstrate
  contribution reporting.
- Monte Carlo contribution decomposition remains aggregate-only; dedicated
  Monte Carlo path-level contribution persistence is still an extension area.

## Phase 10: LSMC Integration

LSMC should be a reusable exercise-policy engine rather than an isolated product
implementation.

Deliverables:

- Expose LSMC to Python.
- Add a generic `ExercisePolicy` abstraction.
- Integrate LSMC into American equity options.
- Integrate LSMC into Bermudan swaptions.
- Integrate LSMC into callable bonds.
- Integrate LSMC into commodity swing and gas storage products.
- Add convergence tests, basis-function tests, seed reproducibility tests, and
  regression benchmarks.

Validation standard:

- The LSMC engine must serialize model configuration, basis functions,
  regression diagnostics, seed, path count, exercise dates, and convergence
  metrics.

Current implementation checkpoint:

- A generic LSMC module exists with `LsmcEngine`, `LsmcConfig`, `LsmcResult`,
  dynamic-programming decision-problem interfaces, stochastic-process
  primitives, and ordinary-least-squares regression support.
- The LSMC result captures value, standard error, path values, VaR, and Expected
  Shortfall, and unit coverage includes an American put exercise-policy example.
- Bermudan swaption pricing already uses the generic LSMC engine through a
  product-specific one-factor approximation.
- American equity options and commodity swing/storage products still use
  product-specific approximations or partial support rather than the shared LSMC
  exercise-policy layer.
- Phase 10 remains a model-integration milestone: expose LSMC through bindings,
  serialize full diagnostics, and connect the reusable engine to all
  early-exercise and physical-flexibility products.

## Phase 11: Production Controls

Production controls should make every analytics run reproducible and
challengeable.

Deliverables:

- Run manifests containing inputs, versions, model configuration, market
  snapshot, and code version.
- Result lineage for valuation, risk, VaR, stress, and PnL explain.
- Benchmark portfolios by asset class.
- Performance gates.
- Coverage gates.
- Validation reports for unsupported trades, missing market data, stale quotes,
  failed pricing, and large residuals.

Validation standard:

- A completed run should answer which trades were valued, which market snapshot
  and model configuration were used, which products were unsupported, which
  diagnostics were raised, and whether valuation, risk, and explain outputs
  passed reconciliation gates.

Current implementation checkpoint:

- SQLite persistence stores portfolios, trades, market snapshots, factors,
  scenario sets, analysis runs, valuation results, risk results, HVaR results,
  and PnL explain results/components.
- CLI and application workflows can initialize storage, import inputs, run
  valuation/risk/HVaR/PnL explain, list stored data, print run reports, and
  compare runs.
- Coverage gates are enforced by the test scripts and CI artifacts, with the
  current threshold at 95% line coverage for C++ and Python.
- Full production run manifests, model/version manifests, benchmark portfolio
  governance, performance gates, validation reports, and operational lineage
  controls remain Phase 11 hardening work.

## Phase 12: Portfolio Optimization And Recommendations

Portfolio optimization should become an explainable decision workflow rather
than a single target allocation.

Deliverables:

- Efficient-frontier generation under client, regulatory, liquidity, turnover,
  product, and concentration constraints.
- Current portfolio, benchmark, nearest feasible, and recommended portfolio
  markers on the frontier.
- Benchmark-relative performance, tracking-error, drawdown, and allocation-drift
  reports.
- Recommendation actions for rebalancing, hedging, concentration reduction,
  unsupported-product replacement, factor-risk reduction, and return/risk
  improvement.
- A recommendation tree where each action produces a new feasible portfolio
  state with cost, turnover, expected-return, risk, constraint, and diagnostic
  impacts.
- Persistence for optimization inputs, solver configuration, frontier points,
  recommendation nodes, recommendation edges, and accepted actions.
- Dashboard views that show recommendation paths on the efficient frontier and
  explain why each action improves or worsens the constrained objective.

Validation standard:

- Optimization and recommendation outputs must state the objective, constraints,
  benchmark, expected-return source, covariance source, solver configuration,
  feasibility status, binding constraints, turnover, and residual constraint
  violations.

Current implementation checkpoint:

- A solver-neutral optimization layer exists with `OptimizationProblem`,
  `OptimizationResult`, solver capabilities, portfolio optimization objectives,
  linear constraints, turnover constraints, risk-model inputs, and a high-level
  `PortfolioOptimizationEngine`.
- A CVXPY adapter and Python worker are present for solving supported
  optimization problems, with unit tests covering serialization, validation, and
  solver behavior.
- Mean-variance, minimum-variance, return-maximization, tracking-error, and
  turnover-constrained workflows are represented at the engine/adapter level.
- Efficient-frontier persistence, recommendation-tree generation, accepted-action
  workflows, and dashboard recommendation paths are not yet implemented.

## Cross-Phase Dashboard Requirements

Analytics dashboards should remain usable as portfolios, scenarios, and factor
sets become dense.

Requirements:

- Chart titles should be visually bold and consistent across Plotly panels.
- Dense categorical x-axis labels should wrap and, when diagonal labels are
  needed, use a bottom-left to top-right angle consistently.
- Charts with many rows should use bounded row-aware heights or split into
  dedicated detail panels before labels overlap.
- Trade-level views should preserve hierarchy where useful, especially
  `asset_class`, `product_type`, and `trade_id`.
- Dashboard tables should use responsive fixed layouts and soft wrapping rather
  than horizontal scrollbars.
- Default trade ordering should be `asset_class`, then `product_type`, then
  `trade_id`; metric-ranked exceptions must make the ranking explicit.

## Cross-Phase Documentation Requirements

Every phase should update:

- the relevant asset-class or model chapter;
- `docs/reference/LEXIS.md` when canonical terms are introduced;
- `docs/reference/FORMULAS.md` when reusable notation is introduced;
- `docs/reference/SOURCES.md` when public source classes are added;
- golden datasets and regression-test documentation;
- product support matrices and known limitations.

This discipline keeps the documentation aligned with the implementation rather
than allowing product additions to create local conventions.
