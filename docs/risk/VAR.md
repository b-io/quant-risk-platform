# Value-at-Risk Documentation

This document separates VaR methodologies clearly.

## Required split

The repository should document and implement three distinct approaches:

1. **Historical VaR**
2. **Parametric VaR**
3. **Monte Carlo VaR**

## Historical VaR

Historical VaR replays observed historical factor moves against today's portfolio. It is intuitive and easy to explain,
but it depends heavily on the chosen history and may underrepresent structural regime changes.

## Parametric VaR

The first target should be delta-normal VaR using a covariance matrix and a linearized risk mapping. It is fast and
scalable, but its assumptions become weak for strong non-linearity and heavy tails.

## Monte Carlo VaR

Monte Carlo VaR should sit on top of the broader simulation framework, not replace it. It is useful when the portfolio
is non-linear or when one-step Gaussian approximations are insufficient.

## Output requirements

Every VaR report should state at least:

- method,
- horizon,
- confidence level,
- factor set,
- valuation mode,
- main assumptions.
