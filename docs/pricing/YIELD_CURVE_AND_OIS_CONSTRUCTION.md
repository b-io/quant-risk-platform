# Yield Curves, OIS Curves, and Interest-Rate Curve Construction

## Why curves matter

In rates analytics, the curve is the central market object. Pricing, discounting, sensitivity analysis, stress testing, and P&L explain all depend on the quality and consistency of curve construction.

## Main curve concepts

### Discount curve
Used to discount future cash flows to present value.

### Projection / forward curve
Used to project floating cash flows associated with a specific index.

### OIS curve
In modern collateralized pricing, overnight indexed swap (OIS) curves are commonly used for discounting because they are closer to collateral remuneration conventions.

## Market-consistent curve construction

There are two broad approaches:

### 1. Direct curve from zero-rate nodes
This is simple and sometimes acceptable for demos or if the market input is already zero rates. It is **not** the full market-consistent construction process.

### 2. Bootstrapping from tradable market instruments
This is the preferred production approach.

Examples of instruments used:
- deposits / overnight fixes for the short end
- OIS swaps for OIS discount curves
- IRS swaps for IBOR-style projection curves
- FRAs, futures, or basis swaps depending on the setup

QuantLib typically supports this via `RateHelper` objects.

## Bootstrapping intuition

A bootstrapped curve is built sequentially so that each market instrument is repriced consistently using the curve being built.

For example:
- short maturities are pinned down first
- longer maturities are added one by one
- interpolation fills the gaps between pillars

The result is a term structure that matches the chosen market inputs.

## Key implementation choices

### Day count
Examples:
- Actual/360
- Actual/365
- 30/360

### Calendar and business-day convention
Examples:
- TARGET
- UnitedStates
- Following / Modified Following

### Pillar selection
The pillar dates are the maturities / effective dates used to anchor the curve.

### Interpolation
Common choices:
- linear on zero rates
- log-linear on discount factors
- cubic spline variants

Different interpolation choices affect sensitivities and smoothness.

## OIS curves by currency

A production-shaped project should support at least mocked but convention-aware OIS curves for multiple currencies:
- USD
- EUR
- GBP
- CHF

The exact instruments and conventions differ by currency and market practice, so the implementation should encode conventions explicitly rather than hardcoding one generic setup.

## Risk derived from curves

From the built curve, the engine should expose:
- discount factors
- zero rates
- forward rates
- PV01 / DV01
- key-rate risk
- scenario valuation under curve shocks

## Credit link

Credit risk often introduces:
- spread curves
- hazard / default curves
- spread shocks or CS01

A good multi-asset architecture should treat spread curves similarly to rates curves at the platform level, while pricing logic remains asset-class specific.

## For this project

The first production-quality step is:
- implement typed market quotes
- build OIS curves from mocked market instruments via QuantLib helpers
- store the built curves inside an immutable market state
- use curve identifiers instead of hardcoded names like `USD_OIS`
