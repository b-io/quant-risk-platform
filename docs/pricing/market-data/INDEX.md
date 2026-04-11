# Market Data and Market Objects

This section is the entry point to the pricing layer.

Before discussing instruments or models, the documentation must define the market state. In the platform, the market state is the normalized collection of quotes, conventions, curves, surfaces, fixings, and metadata required to reconstruct pricing inputs consistently.

## Canonical file

- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md`

## Why this section comes first

A valuation function can be written abstractly as

$$
PV = f(\text{trade state}, \text{market state}, \text{model state}, \text{valuation context})
$$

If the market state is under-specified, then the same trade can price differently depending on hidden defaults in code. That is exactly what the documentation should avoid.

This section therefore explains:

- how raw market observations are normalized,
- how quote identity and metadata are preserved,
- how curves and surfaces are reconstructed from stored inputs,
- how the same market state should support valuation, explain, stress, and replay.

## What to understand from this section

After reading the canonical file, the reader should be able to explain:

- the difference between raw observations and normalized platform quotes,
- why quote IDs, source metadata, timestamps, and conventions matter,
- why the market layer must be reusable across products and asset classes,
- why persistence must preserve enough structure for later reconstruction.

## Suggested next step

After this file, move immediately to `docs/pricing/rates/INDEX.md`, because rates curves are the first concrete example of reusable market objects.
