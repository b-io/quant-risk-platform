# Risk Documentation Index

This folder contains risk, explain, scenario, and simulation documentation.

## Core risk topics

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

## Recommended order

1. `docs/risk/RISK_MEASURES_AND_EXPLAIN.md` — core sensitivities, explain concepts, and reporting semantics.
2. `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md` — factor definitions, bucketing, and aggregation logic.
3. `docs/risk/TIME_SERIES_AND_SCENARIOS.md` — historical time series, factor moves, and scenario construction.
4. `docs/risk/HISTORICAL_STRESS.md` — event replay and historical stress-library design.
5. `docs/risk/VAR.md` — VaR framework choices and how they relate to scenarios and simulation.
6. `docs/risk/MONTE_CARLO.md` — conceptual Monte Carlo architecture in the engine.
7. `docs/risk/MONTE_CARLO_FOUNDATIONS.md` — simulation statistics, error bars, and confidence interpretation.
8. `docs/risk/MONTE_CARLO_IMPLEMENTATION.md` — production implementation details for paths, aggregation, and
   parallelism.
9. `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md` — how macro workflows consume the scenario platform.
10. `docs/risk/MACRO_INDICATORS_AND_INDEX_CONSTRUCTION.md` — macro indicator and index-construction details.
11. `docs/risk/MACRO_REGIMES_AND_EVENT_FLOWS.md` — macro regimes and event-flow interpretation.
12. `docs/risk/FRONT_OFFICE_AND_RISK_WORKING_PRACTICES.md` — practical operational expectations and control usage.
