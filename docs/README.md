# Quant Risk Platform Documentation

This `docs/` tree is the canonical public documentation root for the repository.
It is organized as a compact quant-finance and implementation handbook: theory
first where needed, then market state, asset-class conventions, reusable models,
risk analytics, controls, and implementation sequencing.

Private preparation notes, company-specific research, local notes, and simplified
scratch examples belong under the ignored `temp/docs/` workspace.

## Structure

The top-level folders have distinct roles.

- `docs/foundations/` - probability, statistics, stochastic processes, asset
  pricing, portfolio theory, and transform methods.
- `docs/architecture/` - platform layering, market objects, curve construction,
  analytics services, persistence, lineage, bindings, and implementation
  rationale.
- `docs/market-data/` - normalized quote identity, market snapshots, curves,
  surfaces, conventions, provenance, and validation.
- `docs/asset-classes/` - rates, credit, FX, equity, and commodity product
  families and market conventions.
- `docs/models/` - reusable model families, currently focused on volatility and
  nonlinear option pricing.
- `docs/risk/` - risk factors, sensitivities, PnL explain, stress testing, VaR,
  Expected Shortfall, Monte Carlo, backtesting, and attribution.
- `docs/reference/` - shared lexis, formula notation, and public source lists.
- `docs/implementation/` - implementation roadmap and delivery standards.

## Reading Paths

### Platform Path

1. `docs/architecture/ARCHITECTURE.md`
2. `docs/architecture/MARKET_AND_CURVES.md`
3. `docs/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`
4. `docs/asset-classes/INDEX.md`
5. `docs/models/INDEX.md`
6. `docs/risk/INDEX.md`
7. `docs/architecture/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`
8. `docs/implementation/PHASED_BUILD_PLAN.md`

### Mathematical Path

1. `docs/foundations/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md`
2. `docs/foundations/STATISTICAL_DISTRIBUTIONS_AND_MLE.md`
3. `docs/foundations/PROBABILITY_LIMIT_THEOREMS.md`
4. `docs/foundations/BROWNIAN_MOTION_AND_ITO.md`
5. `docs/foundations/ASSET_PRICING_FOUNDATIONS.md`
6. `docs/market-data/INDEX.md`
7. `docs/asset-classes/rates/INDEX.md`
8. `docs/models/volatility/INDEX.md`
9. `docs/risk/INDEX.md`

### Asset-Class Path

1. `docs/market-data/INDEX.md`
2. `docs/asset-classes/rates/INDEX.md`
3. `docs/asset-classes/credit/INDEX.md`
4. `docs/asset-classes/fx/INDEX.md`
5. `docs/asset-classes/commodities/INDEX.md`
6. `docs/asset-classes/equity/INDEX.md`
7. `docs/models/volatility/INDEX.md`

## Reference Policy

Shared definitions belong in `docs/reference/`.

- `docs/reference/LEXIS.md` defines project-wide terms and canonical domain
  language.
- `docs/reference/FORMULAS.md` gives common notation and formulas used across
  chapters.
- `docs/reference/SOURCES.md` lists public references and source classes.

Asset-class chapters should link to these files instead of carrying duplicate
glossaries, formula sheets, or bibliographies.

## Formula Style

Use dollar-delimited LaTeX because it renders in most Markdown environments.

Inline formulas use single dollar signs, for example `$PV$`, `$D(t,T)$`, or
`$\sigma$`.

Displayed formulas use double dollar signs on their own lines:

$$
PV = \sum_{i=1}^{n} CF_i D(t,T_i)
$$

Each displayed formula should define its symbols, units, convention, and
implementation meaning in the surrounding prose.

## Documentation Principles

The documentation should consistently:

- define the object before describing the algorithm;
- separate market state, product state, model state, and analytics outputs;
- explain what is observed, calibrated, simulated, shocked, stored, or repriced;
- place formulas near their implementation meaning;
- use stable notation across chapters;
- keep publishable documentation independent from private preparation material.
