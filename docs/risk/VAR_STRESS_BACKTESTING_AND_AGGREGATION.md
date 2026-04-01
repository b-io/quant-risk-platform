# VaR, Stress Testing, Backtesting, and Aggregation Across Books

## Why this chapter matters

A risk platform is expected to answer both routine and extreme questions. Routine questions are often handled by
sensitivities, explain, and Value-at-Risk. Extreme questions are handled by stress testing and scenario design. This
chapter unifies those ideas and explains how they aggregate across books and portfolios.

## 1. Historical VaR and Monte Carlo VaR

### 1.1 Historical VaR

Historical VaR applies observed historical factor moves to the current portfolio and reads the loss quantile from the
resulting scenario distribution. If the shocked P&Ls are $L_1,\ldots,L_N$, then the confidence-level-$c$ historical VaR
is
$$
VaR_c = \text{Quantile}_c(L_1,\ldots,L_N)
$$
up to a sign convention. Many desks prefer reporting VaR as a positive loss number, in which case the sign convention
must be documented consistently.

Historical VaR is attractive because it is intuitive and directly tied to realized historical market moves. Its main
weaknesses are also clear:

- it is limited by the historical window,
- it can underweight new regimes,
- it may miss future scenarios that have never yet occurred,
- it inherits whatever distortions exist in the chosen data history.

### 1.2 Monte Carlo VaR

Monte Carlo VaR simulates future market-factor scenarios from a stochastic model and revalues the portfolio under those
simulated states. In generic notation, factor dynamics are simulated as
$$
X_{t+\Delta t}=f(X_t,\theta,\varepsilon)
$$
for model parameters $\theta$ and random driver $\varepsilon$. The resulting loss distribution is then used to compute
VaR or Expected Shortfall.

Monte Carlo VaR is flexible and can incorporate hypothetical regimes, non-linear dependence structures, or path-dependent
payoffs. Its weaknesses are model risk, calibration sensitivity, computational cost, and implementation complexity.

## 2. Stress testing versus VaR

VaR summarizes a distribution at a selected confidence level. Stress testing asks what happens under explicitly chosen
severe but plausible scenarios. The distinction matters operationally:

- VaR is a routine statistical risk measure.
- Stress testing is a management and tail-risk tool.

A stylized stress P&L under scenario $s$ is
$$
P\&L^{stress}_s = V(M^{shock}_s)-V(M^{base})
$$
Unlike VaR, stress results are interpretable one scenario at a time and can be tied directly to economic narratives such
as rate shocks, spread gaps, volatility dislocations, or cross-asset liquidity crises.

## 3. Scenario design

Scenario design is a modeling discipline in its own right. Useful scenarios are:

- economically coherent,
- relevant to the portfolio,
- severe but plausible,
- clearly documented,
- reproducible from stored definitions.

Scenario families often include:

- parallel or bucketed rate shocks,
- spread-widening or basis scenarios,
- smile and volatility-surface shocks,
- historical crisis replays,
- macro event scenarios combining rates, FX, equities, commodities, and credit.

Good scenarios are not merely large numbers. They should reflect the co-movement structure that a desk actually worries
about.

## 4. Backtesting

Backtesting compares predicted risk with realized outcomes. In a VaR context, the standard control question is whether
realized losses exceed the reported VaR too often. If daily losses are $L_t$ and daily VaR is $VaR_t$, an exception
indicator is
$$
I_t = \mathbf{1}_{\{L_t > VaR_t\}}
$$
The observed frequency and clustering of exceptions are then reviewed over time.

Backtesting should not be reduced to a single count. Useful analysis also includes:

- whether exceptions cluster during specific regimes,
- whether particular books or factors dominate exceptions,
- whether model updates reduce or increase exception frequency,
- whether data quality problems masquerade as model failure.

## 5. Sensitivity-based risk and when it is enough

Sensitivity-based risk uses first-order or low-order approximations such as:

- DV01 or PV01,
- key-rate risk,
- CS01,
- delta,
- vega.

These measures are fast, transparent, and easy to aggregate. They are usually appropriate for:

- routine hedging,
- intraday dashboards,
- limit monitoring,
- initial scenario screening.

They are not enough when non-linearity, path dependence, jump risk, or large cross-factor shocks dominate the portfolio.
In such cases the system should escalate to scenario revaluation or simulation.

## 6. Aggregation across books and portfolios

A risk platform rarely stops at single-trade outputs. It must aggregate across:

- trades,
- strategies,
- books,
- desks,
- currencies,
- asset classes,
- legal entities when relevant.

This aggregation requires a common factor taxonomy. For a portfolio with trade-level exposures $r_{i,k}$ to factor $k$,
a simple additive sensitivity aggregation is
$$
R_k^{portfolio}=\sum_i r_{i,k}
$$
This is straightforward mathematically but operationally hard because all trade-level measures must use the same factor
definitions, market snapshots, and model versions.

For scenario P&L the aggregation is similarly simple after consistent revaluation:
$$
P\&L^{scenario}_{portfolio}=\sum_i P\&L^{scenario}_i
$$
The challenge is again consistency of shocks and valuation methodology.

## 7. Why hierarchical drill-down matters

Aggregated risk numbers are only useful if users can drill down. A desk head may start from total VaR or total stress
loss, then ask:

- which book contributed most,
- which factor family dominated,
- which tenor or issuer bucket drove the result,
- which trades changed most from yesterday.

A good platform therefore stores both the aggregate result and the contributing components, together with lineage.

## 8. Practical reporting stack

A coherent reporting stack often looks like this:

1. sensitivity dashboard for fast monitoring,
2. P&L explain report for day-over-day reconciliation,
3. historical stress report for known adverse episodes,
4. VaR and Expected Shortfall for distribution-based risk,
5. scenario library with top-loss and contribution analysis,
6. backtesting pack for ongoing model validation and control.

The reports should be connected, not isolated. A large VaR move should link naturally to factor sensitivities, stress
scenarios, and any changes in the trade population.

## 9. Relationship to the rest of the documentation

- `docs/risk/RISK_MEASURES_AND_EXPLAIN.md` covers the core deterministic risk measures.
- `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md` explains day-over-day valuation reconciliation.
- `docs/risk/HISTORICAL_STRESS.md` covers the event-replay view in more detail.
- `docs/risk/MONTE_CARLO.md`, `docs/risk/MONTE_CARLO_FOUNDATIONS.md`, and
  `docs/risk/MONTE_CARLO_IMPLEMENTATION.md` cover simulation theory and implementation.
