# Volatility Surfaces

This document is the canonical home for volatility documentation as the platform grows.

## Scope

Volatility documentation should cover:

- cap/floor vol term structures,
- swaption vol matrices and surfaces,
- FX implied vols,
- equity implied vol surfaces,
- strike, delta, and moneyness conventions,
- interpolation and arbitrage considerations,
- vega and smile risk mapping.

Volatility is documented separately because surface construction is a cross-asset problem. The exact product differs,
but the platform concerns are similar: quote normalization, convention handling, interpolation, diagnostics, and risk
mapping.

## Required quote coordinates

A normalized volatility quote should carry enough metadata to reconstruct where it sits on a surface. Typical fields
include:

- asset class,
- underlying or index name,
- currency if relevant,
- expiry,
- tenor where relevant,
- strike, moneyness, or delta,
- volatility type,
- quote convention,
- surface family identifier,
- source and timestamp metadata.

Without that metadata, the same number can be interpreted in incompatible ways. A swaption normal volatility, an FX
risk-reversal quote, and an equity implied volatility cannot be treated as interchangeable surface nodes.

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

The market schema should also indicate whether a quote belongs to a one-dimensional term structure or a genuine
surface. That distinction matters for construction, diagnostics, and shocked revaluation.

### Builder layer

The platform should expose a `VolSurfaceBuilder` interface that hides QuantLib-specific construction details behind
platform abstractions.

The builder should be responsible for:

- grouping quotes into consistent surface families,
- validating coordinate completeness,
- choosing interpolation and extrapolation rules,
- exposing reusable handles for scenario and risk workflows,
- surfacing diagnostics when a surface is underdetermined or internally inconsistent.

### Construction choices

Important implementation choices include:

- lognormal versus normal volatility representation,
- strike-space versus delta-space parameterization,
- interpolation along expiry, tenor, and strike dimensions,
- arbitrage-aware smoothing where appropriate,
- surface freezing versus sticky-rule behavior under scenarios.

These choices should be explicit in the market schema rather than hidden in pricing-engine code.

### Risk integration

Volatility surfaces should integrate with:

- pricing engines for optionality,
- vega risk,
- scenario stress,
- VaR and Monte Carlo factor definitions.

A later factor model should support parallel vol shocks, term-structure shocks, smile twists, and product-specific
surface buckets.

## Outputs that matter downstream

A reusable surface object should provide at least:

- implied volatility lookup at calibrated nodes,
- interpolated volatility lookup between nodes,
- surface-family metadata,
- node labels for vega bucketing,
- diagnostics on missing areas or extrapolation use.

## Why this deserves its own folder

Volatility documentation sits beside rates and credit, but is not a subtopic of either one. Surface construction,
quote conventions, and volatility risk are large enough to justify a dedicated section from the start.
