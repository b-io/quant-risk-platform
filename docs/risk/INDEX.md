# Risk Documentation Index

This section contains risk, explain, scenario, and simulation documentation.

The risk notes should be read after the design and pricing layers. Risk depends on explicit pricing objects, explicit market factors, and clear scenario semantics.

## Core risk topics

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
- `docs/risk/FRONT_OFFICE_AND_RISK_WORKING_PRACTICES.md`

## Recommended study order

### Phase 1 — risk vocabulary and explain

1. `docs/risk/RISK_MEASURES_AND_EXPLAIN.md`
2. `docs/risk/RISK_FACTORS_AND_ATTRIBUTION.md`
3. `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md`

### Phase 2 — scenarios, stress, and historical replay

4. `docs/risk/TIME_SERIES_AND_SCENARIOS.md`
5. `docs/risk/HISTORICAL_STRESS.md`

### Phase 3 — VaR and aggregation

6. `docs/risk/VAR.md`
7. `docs/risk/VAR_STRESS_BACKTESTING_AND_AGGREGATION.md`

### Phase 4 — simulation-based risk

8. `docs/risk/MONTE_CARLO.md`
9. `docs/risk/MONTE_CARLO_FOUNDATIONS.md`
10. `docs/risk/MONTE_CARLO_IMPLEMENTATION.md`

### Phase 5 — workflow and macro context

11. `docs/risk/GLOBAL_MACRO_AND_TRADER_WORKFLOW.md`
12. `docs/risk/MACRO_INDICATORS_AND_INDEX_CONSTRUCTION.md`
13. `docs/risk/MACRO_REGIMES_AND_EVENT_FLOWS.md`
14. `docs/risk/FRONT_OFFICE_AND_RISK_WORKING_PRACTICES.md`

## Core risk formulas

A first-order explain approximation is

$$
\Delta PV \approx \sum_k \frac{\partial PV}{\partial x_k}\Delta x_k
$$

A historical VaR estimator at confidence level $\alpha$ can be expressed informally as the negative lower empirical quantile of the scenario PnL distribution:

$$
\mathrm{VaR}_{\alpha} = - q_{1-\alpha}(\mathrm{PnL})
$$

A Monte Carlo estimator of an expectation is

$$
\hat{\mu}_N = \frac{1}{N}\sum_{i=1}^{N} X_i
$$

with standard error that shrinks at order

$$
O\left(\frac{1}{\sqrt{N}}\right)
$$

This is why simulation accuracy improves slowly and why variance reduction matters in production.

## What this section should clarify

By the end of this section, the reader should be able to explain:

- how sensitivities differ from full revaluation,
- why explained and unexplained PnL are control concepts rather than just reporting labels,
- how scenarios map from factor moves to repricing inputs,
- how historical stress differs from statistical VaR,
- why Monte Carlo requires both statistical discipline and systems engineering discipline.
