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

## 3. Parametric VaR

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

## 4. Monte Carlo VaR

Monte Carlo VaR simulates future factor states and reprices the portfolio under each state. If the simulated losses are the sample $\{L_i\}_{i=1}^N$, then

$$
\widehat{VaR}^{MC}_\alpha=\mathrm{Quantile}_\alpha(\{L_i\}_{i=1}^N)
$$

The method is flexible and handles non-linearity well, but it is computationally heavier and inherits model risk from the simulation model.

## 5. Output requirements

Every VaR or ES report should state at least:

- method,
- horizon,
- confidence level,
- factor set,
- valuation mode,
- lookback window or number of paths,
- main assumptions.
