# Volatility Term Structures and Surfaces

This section covers volatility-specific market objects and option-pricing foundations used across asset classes.

## Contents

- `docs/pricing/volatility/GREEKS_AND_NONLINEAR_RISK.md` — delta, gamma, vega, theta, rho, portfolio aggregation,
  non-linear exposures, and scenario revaluation versus Greek approximation.
- `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md` — European, American, and Bermudan option basics,
  Black-Scholes-Merton, Black 76, and numerical-method trade-offs.
- `docs/pricing/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md` — static no-arbitrage, put-call parity,
  arbitrage bounds, synthetic positions, and quote validation checks.
- `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md` — local volatility, stochastic volatility,
  SABR, and calibration trade-offs.
- `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility quote schemas, smile and tenor conventions,
  interpolation choices, no-arbitrage checks, and risk integration.

## Scope

Volatility is a separate documentation section because surface construction and option risk are cross-asset concerns.
This is the right place for cap/floor, swaption, FX, equity, and commodity volatility notes, and for the static
no-arbitrage identities that must be respected before calibration.

## What to understand from this section

After reading this section, the reader should understand:

- how vanilla option payoffs differ across exercise styles,
- why put-call parity is a forward-consistency identity rather than a model-specific trick,
- how parity and arbitrage bounds constrain cleaned option quotes,
- why volatility-surface construction depends on correct forward, discounting, and carry inputs,
- how delta, gamma, vega, theta, and rho feed desk hedging and explain,
- how vega and smile risks should map back to explicit surface factors.
