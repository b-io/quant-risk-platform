# Volatility Documentation Index

This section covers option-pricing foundations, exercise styles, Greeks,
volatility surfaces, and volatility-model calibration choices used across asset
classes.

Volatility belongs under `docs/models/` because the same concepts support rates,
FX, equity, credit, and commodity options.

## Canonical Files

- `docs/models/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md`
- `docs/models/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md`
- `docs/models/volatility/GREEKS_AND_NONLINEAR_RISK.md`
- `docs/models/volatility/VOLATILITY_SURFACES.md`
- `docs/models/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md`

## Recommended Reading Order

1. `docs/models/volatility/OPTION_PRICING_FOUNDATIONS_AND_PUT_CALL_PARITY.md`
2. `docs/models/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md`
3. `docs/models/volatility/GREEKS_AND_NONLINEAR_RISK.md`
4. `docs/models/volatility/VOLATILITY_SURFACES.md`
5. `docs/models/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md`

## Scope

This section should make clear:

- how parity and arbitrage bounds validate option quotes;
- why exercise style determines model and numerical method selection;
- why a volatility surface is a calibrated market object rather than a raw
  table;
- how delta, gamma, vega, theta, rho, smile risk, and cross-factor effects
  enter hedging and explain;
- when full revaluation is preferable to local approximation.

## Shared References

- `docs/reference/LEXIS.md` defines implied volatility, Greeks, exercise policy,
  and LSMC.
- `docs/reference/FORMULAS.md` contains Black-76, Bachelier, put-call parity,
  local risk expansion, and LSMC formulas.
- `docs/reference/SOURCES.md` lists public option-pricing and implementation
  references.

## Maintenance Rule

Volatility chapters should state underlying, forward convention, discounting,
expiry, strike or moneyness convention, volatility type, smile interpolation,
and calibration objective explicitly.
