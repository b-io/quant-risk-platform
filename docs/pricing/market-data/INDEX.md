# Market Data and Market Objects

This section groups documentation about normalized market inputs and the market objects built from them.

## Contents

- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — normalized cross-asset market-data schema covering
  rates, credit, FX, equity, commodity, and volatility inputs.

## Why this section exists

Market-data design is cross-asset and sits below individual pricing models. It deserves its own section because the
platform builds reusable curves, surfaces, and factor objects that many instruments depend on.

## What to understand from this section

After reading this section, the reader should have a clear view of:

- how raw quotes are normalized into typed platform inputs,
- why market state is separated from positions,
- how reusable curve and surface objects fit into valuation and risk,
- which metadata should live in the market schema rather than in product-specific code.
