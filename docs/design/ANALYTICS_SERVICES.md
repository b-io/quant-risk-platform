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

### Risk

- PV01 / DV01,
- key-rate risk,
- CS01,
- later vega and optional gamma,
- aggregation by business tags.

### Explain

- market-move explain,
- carry / roll-down,
- cash and fixing effects,
- new-trade and removed-trade effects,
- residual.

### Stress

- generic scenarios,
- historical stress replay,
- scenario-set governance.

### VaR

- historical simulation,
- parametric delta-normal,
- Monte Carlo VaR via the simulation framework.

### Monte Carlo

- one-step factor simulation,
- path simulation,
- stochastic-process abstraction,
- result aggregation.

## 4. Architectural direction

The platform should move from free-function style analytics toward service objects with stable DTOs. A good target is:

- `PricingService`
- `RiskService`
- `PnlExplainService`
- `HistoricalStressEngine`
- `HistoricalVarEngine`
- `ParametricVarEngine`
- `MonteCarloEngine`

all operating on shared built positions and shared market state.

## 5. Immediate priorities

1. add a built-instrument / built-portfolio cache,
2. separate factor definitions from scenario definitions,
3. split Monte Carlo one-step simulation from path simulation,
4. add explicit VaR engines by methodology,
5. upgrade explain from placeholder logic to reconciliation logic.
