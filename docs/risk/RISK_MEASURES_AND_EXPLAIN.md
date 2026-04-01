# Risk Measures, P&L Explain, and Risk Attribution

This chapter is the compact overview of the main deterministic measures and explain concepts used across the repository.
Detailed treatments of day-over-day explain, historical stress, and VaR live in the dedicated chapters referenced at the
end.

## Deterministic risk measures

A front-office risk engine should support at least the following deterministic measures.

### PV01 / DV01

The change in portfolio value for a 1 bp shift in relevant rates. A symmetric bump-and-reprice approximation is
$$
DV01 \approx \frac{PV(r-1bp)-PV(r+1bp)}{2}
$$
Possible variants include:

- parallel DV01 on a curve,
- instrument-level DV01,
- portfolio DV01,
- clean-price versus dirty-price variants.

### Key-rate risk

Key-rate risk measures sensitivity to shocks at selected maturities or curve buckets, such as:

- 1M,
- 3M,
- 6M,
- 1Y,
- 2Y,
- 5Y,
- 10Y,
- 30Y.

This helps identify where along the curve the portfolio is exposed.

### CS01

CS01 is the change in value for a 1 bp spread widening, usually used for credit spread exposure. A typical symmetric
approximation is
$$
CS01 \approx \frac{PV(s-1bp)-PV(s+1bp)}{2}
$$
Reports should state clearly which spread object is shocked: cash-bond spread, Z-spread, CDS spread curve, or bucketed
credit curve.

### Scenario P&L

Scenario P&L is the portfolio value under explicit curve, spread, volatility, or cross-asset shocks:
$$
P\&L^{scenario}=V(M^{shock})-V(M^{base})
$$
This measure is indispensable when Greek approximations become unreliable.

## P&L explain

P&L explain reconciles the change in valuation from one date to another.

A useful decomposition is
$$
P\&L_{actual}=P\&L_{market}+P\&L_{carry}+P\&L_{cashflows}+P\&L_{trade\ flow}+P\&L_{model\ change}+P\&L_{valuation\ adjustments}+P\&L_{unexplained}
$$
Typical components are:

- previous NPV,
- carry or accrual,
- market-move effect,
- fixing, coupon, or cash effect,
- new trades, amendments, or unwinds,
- residual unexplained P&L.

Residual should be small if the explain is well specified and aligned with front-office valuation conventions.

## Risk attribution

Risk attribution decomposes the level of risk into interpretable contributions.

Examples include:

- DV01 by currency,
- DV01 by curve,
- DV01 by tenor bucket,
- DV01 by book, strategy, or desk,
- CS01 by issuer family or credit bucket,
- vega by surface bucket.

## Risk change explain

Risk can also be explained through time. A simple conceptual split is:

- new trades,
- matured trades or roll-down,
- market moves changing sensitivities,
- fixing effects,
- model changes.

This is often the most useful control companion to ordinary P&L explain because it explains why exposure changed even if
P&L did not move dramatically.

## Relationship to the rest of the documentation

- `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md` gives the full treatment of actual, theoretical, explained, and unexplained
  P&L.
- `docs/risk/VAR_STRESS_BACKTESTING_AND_AGGREGATION.md` explains where deterministic measures sit relative to VaR,
  stress, backtesting, and aggregation across books.
- `docs/pricing/credit/BOND_SPREADS_AND_DEFAULT_RISK.md` explains the underlying credit spread measures used by CS01.
- `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md` explains option Greeks and the limits of local
  sensitivity-based approximations.
