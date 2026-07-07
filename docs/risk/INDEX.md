# Risk Documentation Index

This section contains risk, explain, scenario, stress, simulation, and
attribution documentation.

Risk should be read after architecture, market data, and asset-class valuation
chapters. Factor design, scenario semantics, and explain logic depend on
explicit pricing objects and reproducible market state.

## Canonical Files

- `docs/risk/RISK_MEASURES_AND_EXPLAIN.md`
- `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md`
- `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md`
- `docs/risk/TIME_SERIES_AND_SCENARIOS.md`
- `docs/risk/HISTORICAL_STRESS.md`
- `docs/risk/VAR.md`
- `docs/risk/VAR_STRESS_BACKTESTING_AND_AGGREGATION.md`
- `docs/risk/MONTE_CARLO.md`
- `docs/risk/MONTE_CARLO_FOUNDATIONS.md`
- `docs/risk/MONTE_CARLO_IMPLEMENTATION.md`
- `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md`
- `docs/risk/MACRO_INDICATORS_AND_INDEX_CONSTRUCTION.md`
- `docs/risk/MACRO_REGIMES_AND_EVENT_FLOWS.md`
- `docs/risk/VALUATION_RISK_CONTROL_WORKFLOWS.md`
- `docs/asset-classes/commodities/RISK_BACKTESTING_GOVERNANCE.md`

## Recommended Reading Order

### Part 1: Risk Vocabulary And Explain

1. `docs/risk/RISK_MEASURES_AND_EXPLAIN.md`
2. `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md`
3. `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md`

### Part 2: Scenarios, Stress, And Historical Replay

4. `docs/risk/TIME_SERIES_AND_SCENARIOS.md`
5. `docs/risk/HISTORICAL_STRESS.md`

### Part 3: VaR, Expected Shortfall, And Aggregation

6. `docs/risk/VAR.md`
7. `docs/risk/VAR_STRESS_BACKTESTING_AND_AGGREGATION.md`

### Part 4: Simulation-Based Risk

8. `docs/risk/MONTE_CARLO.md`
9. `docs/risk/MONTE_CARLO_FOUNDATIONS.md`
10. `docs/risk/MONTE_CARLO_IMPLEMENTATION.md`

### Part 5: Macro And Workflow Context

11. `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md`
12. `docs/risk/MACRO_INDICATORS_AND_INDEX_CONSTRUCTION.md`
13. `docs/risk/MACRO_REGIMES_AND_EVENT_FLOWS.md`
14. `docs/risk/VALUATION_RISK_CONTROL_WORKFLOWS.md`

### Part 6: Commodity Risk Application

15. `docs/asset-classes/commodities/RISK_BACKTESTING_GOVERNANCE.md`  
   Application of the generic risk framework to power, gas, carbon, flexible
   assets, and commodity-specific validation.

## Current Implementation Coverage

Current risk implementation includes:

- PnL explain with persisted result/component tables, sequential factor
  revaluation, realized deposit maturity cash, and reconciliation;
- HVaR and historical stress replay factor-bound scenarios over the current
  market state;
- historical VaR and Expected Shortfall contributions by trade, book, strategy,
  currency, asset class, and risk factor;
- deterministic sensitivities for the supported product families.

Known boundaries are:

- Monte Carlo and parametric VaR contribution decomposition;
- reusable LSMC integration where early exercise and physical flexibility affect
  risk;
- broader realized cashflow/event sources for explain;
- production controls for run manifests, lineage, benchmark portfolios,
  performance gates, coverage gates, and validation reports.

## Shared References

- `docs/reference/LEXIS.md` defines risk factor, `RiskFactorId`, sensitivity,
  PnL explain, VaR, and Expected Shortfall.
- `docs/reference/FORMULAS.md` contains shared risk expansion, PnL explain, VaR,
  Expected Shortfall, and Monte Carlo notation.
- `docs/reference/SOURCES.md` lists public risk and modeling references.

## Maintenance Rule

Risk chapters should state factor identity, shock unit, bump size, revaluation
policy, aggregation key, sign convention, horizon, confidence level, and
reconciliation rule explicitly.
