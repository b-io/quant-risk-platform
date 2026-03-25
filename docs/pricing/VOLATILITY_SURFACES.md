# Volatility Surfaces

This document is the canonical home for volatility documentation as the platform grows.

## Scope

Volatility documentation should eventually cover:

- cap/floor vol term structures,
- swaption vol matrices and surfaces,
- FX implied vols,
- equity implied vol surfaces,
- strike / delta / moneyness conventions,
- interpolation and arbitrage considerations,
- vega and smile risk mapping.

## Planned design direction

### Data model
Quotes should eventually include:

- asset class,
- underlying or index,
- expiry,
- tenor,
- strike or delta,
- volatility type,
- quote convention,
- surface family ID.

### Builder layer
The platform should expose a `VolSurfaceBuilder` interface that hides QuantLib-specific construction details behind platform abstractions.

### Risk integration
Volatility surfaces should integrate with:

- pricing engines for optionality,
- vega risk,
- scenario stress,
- VaR and Monte Carlo factor definitions.
