# Time Series Analysis and Scenario Construction

## Why this matters

Risk systems rely on time series for:

- historical stress construction
- volatility estimation
- correlation estimation
- factor normalization
- model calibration
- backtesting

## Core time-series concepts

### Stationarity

A stationary process has stable statistical properties over time. Many raw financial series such as yields or prices are
not stationary, but returns, changes, or spread changes may be closer to stationary.

### Returns vs levels

For risk modeling, choosing whether to model:

- price / yield **levels**
- arithmetic changes
- log returns

is critical. Rates risk often uses **level changes in basis points**. Equity risk often uses **returns**. Credit risk
often uses **spread changes** or relative spread shocks.

### Volatility and covariance

Historical time series can be used to estimate:

- factor volatility
- covariance matrix
- correlation structure

These feed:

- parametric VaR
- factor simulation
- stress normalization

## Historical scenarios

A historical scenario is a snapshot of factor moves taken from a real date or window, then applied to today's portfolio.

Examples:

- parallel 25 bp rise in the front end during a central-bank shock,
- spread widening observed during a credit event,
- curve steepener from a historical macro date.

Historical scenarios are attractive because they are intuitive and tied to real market conditions.

For implementation, the scenario library should contain both:

- broad cross-asset crises such as 2008 and March 2020,
- famous rates-specific episodes such as 1994, the 2013 taper tantrum, the 2022 inflation repricing, and the 2022 UK
  gilt / LDI stress.

That distinction matters because a rates desk can suffer large losses even when the main stress is not an equity-style
crisis but a violent curve repricing or a long-end dislocation.

## Scenario construction choices

### Absolute shocks

Apply additive changes, e.g. `+15 bp` to a rate node.

Best for:

- yields
- spreads
- vol points in some contexts

### Relative shocks

Apply multiplicative changes, e.g. `+5%` to an index level.

Best for:

- prices
- equities
- FX spot in some systems

### Hybrid rules

Some risk systems use absolute shocks for rates / spreads and relative shocks for price-like factors.

## Historical stress replay

A historical stress engine usually works as:

1. choose current portfolio and current market state
2. take historical factor shocks from a prior date or interval
3. map them onto today’s factor set
4. apply shocked market state
5. reprice and aggregate losses

## Important implementation issues

- factor mapping must be explicit
- missing historical points require interpolation or exclusion rules
- scenario data should be versioned and auditable
- rate, spread, and volatility factors may need different shock conventions

## For this project

Use time-series analysis to support:

- historical stress scenario generation
- simple covariance estimation for Monte Carlo factor shocks
- backtesting of P&L explain / scenario frameworks

The engine should support both:

- user-defined deterministic scenarios
- historical scenarios derived from stored factor time series

The initial historical-stress package should explicitly ship a curated scenario library rather than only an empty
framework.
