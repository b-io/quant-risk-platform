# Risk Measures, P&L Explain, and Risk Attribution

## Deterministic risk measures

A front-office risk engine should support at least the following deterministic measures.

### PV01 / DV01

The change in portfolio value for a 1 bp shift in relevant rates.

Possible variants:

- parallel DV01 on a curve
- instrument-level DV01
- portfolio DV01

### Key-rate risk

Sensitivity to shocks at selected maturities / curve buckets, such as:

- 1M
- 3M
- 6M
- 1Y
- 2Y
- 5Y
- 10Y
- 30Y

This helps identify where along the curve the portfolio is exposed.

### CS01

The change in value for a 1 bp spread widening, usually used for credit spread exposure.

### Scenario P&L

Portfolio value under explicit curve, spread, volatility, or cross-asset shocks.

## P&L explain

P&L explain reconciles the change in valuation from one date to another.

Typical decomposition:

- previous NPV
- carry / accrual
- market move effect
- fixing / coupon / cash effect
- new trades / amendments / unwinds
- residual

A useful equation is:

`current NPV - previous NPV = explained components + residual`

Residual should be small if the explain is well specified and aligned with front-office valuation conventions.

## Risk attribution

Risk attribution decomposes the level of risk into interpretable contributions.

Examples:

- DV01 by currency
- DV01 by curve
- DV01 by tenor bucket
- DV01 by book / strategy / desk
- CS01 by issuer family or credit bucket

## Risk change explain

Risk can also be explained through time.

Example:

- why did the portfolio DV01 increase from yesterday to today?

Possible drivers:

- new trades
- matured trades / roll-down
- market move changing instrument sensitivities
- fixing effects
- model changes

## For this project

Implement these separately:

1. **Risk attribution report**
    - explain current risk by factor family / bucket / business tag

2. **P&L explain report**
    - explain valuation changes across dates

3. optional later **risk change explain**
    - explain changes in risk between dates

The current placeholder where market move P&L equals total P&L with zero residual is only a scaffold and should be
replaced.
