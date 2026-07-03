# VaR, Stress Testing, Backtesting, and Aggregation Across Books

This chapter explains how routine risk measures and tail-risk measures fit together in one platform.

## 1. Historical VaR and Monte Carlo VaR

Historical VaR applies observed factor moves to today's portfolio and reads the empirical loss quantile. If the resulting losses are the sample $\{L_i\}_{i=1}^N$, then

$$
VaR_\alpha=\mathrm{Quantile}_\alpha(\{L_i\}_{i=1}^N)
$$

Monte Carlo VaR simulates factor dynamics and revalues the portfolio under those simulated states. Let $X_t$ be the factor state and let the model evolve it by

$$
X_{t+\Delta t}=f(X_t\ ;\theta\ ;\varepsilon)
$$

Where:

- $\theta$ is the vector of model parameters.
- $\varepsilon$ is the random driver.

## 2. Stress testing versus VaR

VaR summarizes a distribution at a selected confidence level. Stress testing asks what happens under explicitly chosen severe scenarios.

Let $M^{base}$ be the base market state and let $M_s^{shock}$ be scenario $s$. Then scenario P&L is

$$
P\&L_s^{stress}=V(M_s^{shock})-V(M^{base})
$$

## 3. Backtesting

Backtesting compares predicted risk with realized outcomes. Let daily loss be $L_t$ and reported VaR be $VaR_t$. The exception indicator is

$$
I_t=\mathbf{1}_{\{L_t>VaR_t\}}
$$

Where:

- $I_t=1$ means the realized loss exceeded VaR on day $t$.

A good backtesting pack looks beyond exception count and also studies clustering, regime dependence, and dominant books or factors.

## 4. Aggregation across books

Let $r_{i,k}$ be trade-level exposure of trade $i$ to factor $k$. Portfolio sensitivity to factor $k$ is

$$
R_k^{portfolio}=\sum_i r_{i,k}
$$

Let $P\&L_i^{scenario}$ be scenario P&L of trade $i$. Then portfolio scenario P&L is

$$
P\&L_{portfolio}^{scenario}=\sum_i P\&L_i^{scenario}
$$

Aggregation is meaningful only if all trade-level results were produced under the same factor definitions, market snapshot, and methodology.

## 5. Reporting stack

A coherent reporting stack usually contains:

1. sensitivities for rapid monitoring,
2. P&L explain for day-over-day reconciliation,
3. historical stress for known adverse episodes,
4. VaR and ES for distribution-based risk,
5. backtesting for ongoing model validation,
6. contribution reports with drill-down by reporting group, book, factor family, tenor, or issuer.
