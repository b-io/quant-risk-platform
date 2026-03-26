# Market Data and Market Objects

This section groups documentation about normalized market inputs and the market objects built from them.

## Contents

- `docs/pricing/market-data/MULTI_ASSET_MARKET_DATA_AND_CURVES.md` — unified representation of rates, credit, FX,
  equity, commodity, and volatility inputs.

## Why this section exists

Market-data design is cross-asset and sits below individual pricing models. It deserves its own section because the
platform builds reusable curves, surfaces, and factor objects that many instruments depend on.
