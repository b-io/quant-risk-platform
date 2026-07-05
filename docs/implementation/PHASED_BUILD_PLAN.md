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
  Adventurous Commodity Volatility; numeric golden outputs should be regenerated
  after the next build.

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
