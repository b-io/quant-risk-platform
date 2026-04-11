# Quant Risk Platform Documentation

This `docs/` tree is the canonical documentation root for the repository.

The documentation is split into two layers:

- the **canonical platform documentation** in `docs/design/`, `docs/pricing/`, `docs/risk/`, `docs/theory/`, and `docs/roadmap/`,
- the **temporary study workspace** in `docs/temp/`, which is intentionally separated from the canonical platform notes.

The canonical layer should read like a coherent technical handbook for the platform and for the quantitative concepts that the platform implements.

## How to use this tree

There are two sensible ways to read the documentation.

### Path A — understand the platform end to end

Read in this order:

1. `docs/design/ARCHITECTURE.md`
2. `docs/design/MARKET_AND_CURVES.md`
3. `docs/design/CURVE_BOOTSTRAP_DESIGN.md`
4. `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`
5. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md`
6. `docs/pricing/credit/CREDIT_CURVE_CONSTRUCTION.md`
7. `docs/pricing/volatility/VOLATILITY_SURFACES.md`
8. `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md`
9. `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md`
10. `docs/risk/VAR_STRESS_BACKTESTING_AND_AGGREGATION.md`
11. `docs/design/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`
12. `docs/roadmap/MASTER_ROADMAP_AND_NEXT_STEPS.md`

This path starts from architecture, moves through market state and pricing objects, then finishes with risk, storage, and implementation sequencing.

### Path B — build the mathematical foundations first

Read in this order:

1. `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md`
2. `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md`
3. `docs/theory/PROBABILITY_LIMIT_THEOREMS.md`
4. `docs/theory/BROWNIAN_MOTION_AND_ITO.md`
5. `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md`
6. `docs/theory/ASSET_PRICING_FOUNDATIONS.md`
7. `docs/pricing/INDEX.md`
8. `docs/risk/INDEX.md`
9. `docs/design/INDEX.md`

This path is better when the reader wants to move from probability and stochastic processes toward pricing, risk, and implementation.

## Structure

- `docs/design/` — architecture, service boundaries, observer semantics, persistence, lineage, implementation choices, and platform build guidance.
- `docs/pricing/` — market-state design and pricing notes, organized into market data, rates, credit, and volatility.
- `docs/risk/` — explain, factor attribution, scenario design, stress testing, VaR, Expected Shortfall, Monte Carlo, and macro workflow notes.
- `docs/theory/` — probability, statistics, stochastic processes, term-structure models, transform methods, portfolio theory, and asset-pricing foundations.
- `docs/roadmap/` — current implementation plan, historical review notes, and status tracking.
- `docs/temp/` — temporary study workspace kept separate from the canonical platform notes.

## Documentation principles

The documentation should consistently do the following:

- define the object before describing the algorithm,
- separate market state, product state, and analytics outputs,
- place formulas next to implementation meaning,
- explain what is shocked, calibrated, stored, or repriced,
- prefer stable notation over chapter-specific notation drift,
- keep temporary study-oriented material outside the canonical documentation layer.

## Notation and formula style

Use inline math for short expressions such as $PV$, $D(t,T)$, $F(t;T_1,T_2)$, or $\sigma$.

Use unindented display math for equations that deserve their own line:

$$
PV = \sum_{i=1}^{n} CF_i \, D(t,T_i)
$$

$$
\mathrm{PnL}_{\text{explained}} \approx \sum_k \Delta x_k \, \frac{\partial PV}{\partial x_k}
$$

When a formula is shown, the surrounding text should explain:

- what each symbol means,
- whether the quantity is observed, calibrated, simulated, or derived,
- how the formula maps to implementation objects.

## Section map

### Design

Start with `docs/design/INDEX.md`.

Key files:

- `docs/design/ARCHITECTURE.md`
- `docs/design/MARKET_AND_CURVES.md`
- `docs/design/CURVE_BOOTSTRAP_DESIGN.md`
- `docs/design/ANALYTICS_SERVICES.md`
- `docs/design/IMPLEMENTATION_CHOICES.md`
- `docs/design/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`

### Pricing

Start with `docs/pricing/INDEX.md`.

Pricing is organized from market state to asset-class specifics:

- `docs/pricing/market-data/`
- `docs/pricing/rates/`
- `docs/pricing/credit/`
- `docs/pricing/volatility/`

### Risk

Start with `docs/risk/INDEX.md`.

Risk notes should be read as the continuation of pricing notes, because factor design, explain logic, and scenario semantics only make sense once the pricing objects are clear.

### Theory

Start with `docs/theory/INDEX.md`.

Theory notes are not meant to be detached mathematics. They exist to support the pricing and risk documents.

### Roadmap

Start with `docs/roadmap/INDEX.md`.

Roadmap notes should be read after the architectural and pricing/risk notes, because they are implementation planning documents rather than conceptual primers.

## Repository-wide study order

A practical full pass through the documentation is:

1. `docs/README.md`
2. `docs/theory/INDEX.md`
3. `docs/design/INDEX.md`
4. `docs/pricing/INDEX.md`
5. `docs/risk/INDEX.md`
6. `docs/roadmap/INDEX.md`
7. `docs/temp/README.md` only after the canonical material is clear
