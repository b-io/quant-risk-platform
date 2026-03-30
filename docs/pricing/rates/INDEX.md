# Rates Pricing and Curve Construction

This section covers rates-specific curve construction, curve usage, dynamic term-structure modeling, and model
selection.

## Contents

- `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — production-oriented rates curve implementation details,
  interpolation choices, extrapolation, and curve-stack design.
- `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — rates factor modeling, short-rate and parametric curve
  models, modern practice, and practical model-selection guidance.
- `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md` — mean reversion, time-dependent
  volatility, CIR, Courtadon, Black-Karasinski, BDT, LMM comparisons, and worked numerical examples.
- `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — discounting and projection curves, OIS bootstrapping,
  conventions, helpers, and rates risk implications.
- `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — worked rates examples for discount factors, forwards, yield
  types, and recession-driven curve behavior.

## Support assets

- `docs/pricing/rates/assets/curve_objects_relationships.png` — diagram of the main curve object relationships used in
  the rates stack.
- `docs/pricing/rates/assets/interpolation_tradeoffs.png` — diagram summarizing interpolation and smoothness trade-offs
  across curve construction choices.

## Scope

The rates section is the canonical home for:

- OIS discounting by currency,
- IBOR or term projection curves,
- conventions used by deposits, FRAs, futures, and swaps,
- rates-specific bootstrap choices,
- rates risk diagnostics and scenario foundations,
- implementation trade-offs for interpolation, extrapolation, and dynamic rate models.

## What to understand from this section

After reading this section, the reader should be able to explain:

- why discount and projection curves should be separated,
- how rates conventions affect helper construction and pricing,
- why curve objects must be reusable across valuation, risk, and stress,
- how rates curve design drives PV01, bucketed risk, and scenario semantics,
- where bootstrapping ends and dynamic curve or factor modeling begins,
- why one-factor models, affine models, and forward-rate market models make different calibration and implementation
  trade-offs.

## Recommended reading order

1. `docs/pricing/rates/YIELD_CURVE_AND_OIS_CONSTRUCTION.md` — first understand how today's market discount and
   projection curves are built.
2. `docs/pricing/rates/YIELD_CURVE_WORKED_EXAMPLE.md` — then make the static curve material concrete with numerical
   examples.
3. `docs/pricing/rates/DETAILED_YIELD_CURVE_IMPLEMENTATION.md` — next study interpolation, extrapolation, and production
   implementation choices.
4. `docs/pricing/rates/RATES_FACTORS_AND_CURVE_MODELS.md` — then place short-rate, forward-rate, and parametric models
   in the wider modeling landscape.
5. `docs/pricing/rates/TIME_DEPENDENT_VOLATILITY_AND_MEAN_REVERSION_MODELS.md` — finally study the detailed trade-offs
   among mean-reverting and time-dependent-volatility model families.
