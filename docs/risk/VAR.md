# Value-at-Risk Documentation

This document separates VaR methodologies clearly.

## Required split

The repository should document and implement three distinct approaches:

1. **Historical VaR**
2. **Parametric VaR**
3. **Monte Carlo VaR**

They may share factor definitions, market data, and reporting formats, but they should not be treated as one generic
engine because the assumptions, diagnostics, and failure modes are different.

## Common definition

For a portfolio P&L random variable $\Pi$, a one-sided confidence level $\alpha$ VaR is commonly interpreted as a loss
threshold such that only a tail proportion of outcomes is worse than that level. In sign conventions where losses are
reported as positive values, the VaR number is often written as the negative of a lower-tail P&L quantile.

## Historical VaR

Historical VaR replays observed historical factor moves against today's portfolio. It is intuitive and easy to explain,
but it depends heavily on the chosen history and may underrepresent structural regime changes.

Typical workflow:

1. collect historical factor moves,
2. map those moves onto today's market state,
3. revalue the portfolio under each replayed scenario,
4. read the empirical tail quantile of the resulting P&L distribution.

Historical VaR fits naturally with historical stress and scenario libraries, but it needs clean factor mapping and clear
rules for missing history.

## Parametric VaR

The first target should be delta-normal VaR using a covariance matrix and a linearized risk mapping. It is fast and
scalable, but its assumptions become weak for strong non-linearity and heavy tails.

A simple delta-normal approximation uses:

- a vector of factor sensitivities,
- a factor covariance matrix,
- a horizon scaling assumption,
- a confidence-level multiplier.

This approach is often the most practical starting point for large portfolios because it is cheap to compute and easy to
aggregate, even though it can materially understate option or basis risks.

## Monte Carlo VaR

Monte Carlo VaR should sit on top of the broader simulation framework, not replace it. It is useful when the portfolio
is non-linear or when one-step Gaussian approximations are insufficient.

The engine should make it explicit whether it is doing:

- one-step factor simulation,
- full path simulation,
- simulation under a Gaussian, t, or other factor model,
- simulation with static or stochastic volatility assumptions.

## Expected Shortfall

Even when the primary report is VaR, the same scenario distribution should ideally support expected shortfall as a tail
average. That is especially useful for comparing tail shape across methodologies.

## Output requirements

Every VaR report should state at least:

- method,
- horizon,
- confidence level,
- factor set,
- valuation mode,
- main assumptions.

Useful additions are:

- lookback window,
- number of scenarios or paths,
- covariance source,
- treatment of non-linear products,
- diversification versus standalone views,
- top factor contributors.

## Method comparison

A clear documentation set should keep the following trade-offs visible:

- **Historical VaR**: intuitive and scenario-based, but history-dependent.
- **Parametric VaR**: fast and scalable, but assumption-heavy.
- **Monte Carlo VaR**: flexible and better for non-linearity, but computationally heavier.

That separation is important both for implementation and for interpretation of reported numbers.
