# Volatility Term Structures and Surfaces

This section covers volatility-specific market objects used across asset classes.

## Contents

- `docs/pricing/volatility/VOLATILITY_SURFACES.md` — volatility quote schemas, surface builders, interpolation,
  conventions, and risk integration.

## Scope

Volatility is a separate documentation section because surface construction and option risk are cross-asset concerns.
This is the right place for later cap/floor, swaption, FX, equity, and commodity volatility notes.

## What to understand from this section

After reading this section, the reader should understand:

- why volatility quotes need richer coordinates than simple tenor labels,
- how surface construction differs from scalar curve construction,
- how interpolation and quoting conventions affect pricing,
- how vega and smile risks should map back to explicit surface factors.
