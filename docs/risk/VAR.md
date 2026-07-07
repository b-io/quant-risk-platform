# Value-at-Risk and Expected Shortfall

This chapter separates the main VaR methodologies and states their assumptions explicitly.

## Notation used in this chapter

Unless stated otherwise:

- $\Pi$ is portfolio P&L over the chosen horizon.
- $L=-\Pi$ is loss under the positive-loss convention.
- $\alpha$ is the confidence level.
- $N$ is the number of scenarios or paths.
- $L_i$ is the loss in scenario $i$.

## 1. Common definition

Let $L$ be the loss random variable. VaR at confidence level $\alpha$ is the loss threshold exceeded only in the tail of the distribution.

A compact definition is

$$
VaR_\alpha=\mathrm{Quantile}_\alpha(L)
$$

Where:

- $\mathrm{Quantile}_\alpha(L)$ is the $\alpha$-quantile of the loss distribution.

Expected Shortfall at confidence level $\alpha$ is the average loss beyond VaR. In a loss convention,

$$
ES_\alpha=\mathbb{E}[L\mid L\ge VaR_\alpha]
$$

## 2. Historical VaR

Historical VaR replays observed historical factor moves against today's portfolio. If the replayed losses are the sample $\{L_i\}_{i=1}^N$, then

$$
\widehat{VaR}^{hist}_\alpha=\mathrm{Quantile}_\alpha(\{L_i\}_{i=1}^N)
$$

Historical VaR is intuitive and scenario-based, but it depends on the lookback window and factor mapping.

## 3. Historical VaR contribution analytics

The implementation stores historical scenario P&L at both portfolio and trade
level, then calculates contribution rows under the positive-loss convention.
If $s^\*$ is the scenario selected by portfolio historical VaR, component VaR
for group $g$ is

$$
C_g^{VaR}=-P\&L_g(s^\*)
$$

If $T_\alpha$ is the portfolio tail set used for Expected Shortfall, component
ES is

$$
C_g^{ES}=-\frac{1}{|T_\alpha|}\sum_{s\in T_\alpha}P\&L_g(s)
$$

Supported historical contribution aggregations are:

- trade,
- book,
- strategy,
- currency,
- asset class,
- risk factor.

Trade-based aggregations sum trade scenario P&L into the requested group.
Risk-factor aggregation allocates each scenario's portfolio P&L across shocked
factors in proportion to absolute shock magnitude. That allocation is an
approximation and should be reported with the residual by aggregation type.

Each contribution row includes:

- component VaR and component ES,
- portfolio VaR and ES share,
- standalone VaR and ES for the group path,
- incremental VaR and ES from removing the group,
- marginal VaR and ES using the same remove-group approximation,
- confidence level, scenario count, tail count, VaR scenario, sign convention,
  aggregation rule, and calculation method.

## 4. Parametric VaR

In a delta-normal approximation with factor-sensitivity vector $w$ and factor covariance matrix $\Sigma$, the portfolio standard deviation is

$$
\sigma_P=\sqrt{w^\top \Sigma w}
$$

Where:

- $w$ is the vector of factor sensitivities mapped into the parametric model.
- $\Sigma$ is the factor covariance matrix.

If $z_\alpha$ is the standard-normal quantile at confidence level $\alpha$, a one-step parametric VaR is

$$
VaR_\alpha^{para}=z_\alpha\sigma_P
$$

This method is fast and scalable, but it can understate non-linear or heavy-tailed risk.

## 5. Monte Carlo VaR

Monte Carlo VaR simulates future factor states and reprices the portfolio under each state. If the simulated losses are the sample $\{L_i\}_{i=1}^N$, then

$$
\widehat{VaR}^{MC}_\alpha=\mathrm{Quantile}_\alpha(\{L_i\}_{i=1}^N)
$$

The method is flexible and handles non-linearity well, but it is computationally heavier and inherits model risk from the simulation model.

## 6. Output requirements

Every VaR or ES report should state at least:

- method,
- horizon,
- confidence level,
- factor set,
- valuation mode,
- lookback window or number of paths,
- contribution aggregation rule and residual when contributions are reported,
- main assumptions.
