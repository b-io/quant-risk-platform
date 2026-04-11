# Volatility Term Structures and Surfaces

This section covers volatility-specific market objects and option-pricing foundations used across asset classes.

The volatility notes come after rates and credit because surface construction relies on correct forwards, discounting, carry conventions, and factor mapping.

## Canonical files

- `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md`
- `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md`
- `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md`
- `docs/pricing/volatility/VOLATILITY_SURFACES.md`
- `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md`

## Recommended study order

1. `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md`
2. `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md`
3. `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md`
4. `docs/pricing/volatility/VOLATILITY_SURFACES.md`
5. `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md`

## Core volatility identities

For a non-dividend-paying equity under simple textbook assumptions, put-call parity is

$$
C - P = S_0 - K e^{-rT}
$$

More generally, parity is a forward-consistency statement, not a model-specific trick.

For a pricing function $PV(x)$ depending on market factors $x$, a local second-order expansion is

$$
\Delta PV \approx \sum_i \frac{\partial PV}{\partial x_i}\Delta x_i + \frac{1}{2}\sum_i \frac{\partial^2 PV}{\partial x_i^2}(\Delta x_i)^2 + \sum_{i<j}\frac{\partial^2 PV}{\partial x_i \partial x_j}\Delta x_i \Delta x_j
$$

This is the reason option risk documentation cannot stop at delta. Gamma, vega, smile risk, and cross-factor effects appear naturally once the payoff is nonlinear.

## What this section should make operationally clear

By the end of this section, the reader should understand:

- how parity and arbitrage bounds help validate option quotes,
- why exercise style matters for model choice and numerical method,
- why a volatility surface is not just a table of implied vol numbers,
- how delta, gamma, vega, theta, and rho feed hedging and explain,
- why full revaluation is often preferable to linear approximation for large moves or strong nonlinearities.
