# Market and Curves Design

This document describes how the platform should represent market data, conventions, and built market objects.

## 1. Purpose

The market layer is responsible for converting raw external market inputs into reusable pricing objects:

- yield and discount curves,
- projection curves,
- later credit curves,
- later volatility term structures and surfaces,
- mutable quote handles for fast revaluation.

The design goal is to separate:

1. **raw market inputs**,
2. **conventions and metadata**,
3. **built QuantLib objects**,
4. **scenario application over the built state**.

## 2. Current implementation status

Implemented today:

- `domain::MarketQuote`, `domain::CurveSpec`, and `domain::MarketSnapshot`,
- a first `MarketConventionRegistry` for rates conventions,
- `MarketState` holding built curves and `SimpleQuote` handles,
- bootstrapped rates curves using QuantLib `RateHelper`s,
- handle-based scenario application without rebuilding the whole portfolio.

Still missing or thin:

- explicit curve purpose (`discount`, `projection`, `credit`, `volatility`),
- richer quote metadata for helpers and fixing conventions,
- typed credit and volatility builders,
- relinkable-handle design for larger scenario and simulation workflows,
- validation and schema versioning.

## 3. Recommended market schema direction

A robust schema should distinguish:

### 3.1. Quote and Risk Factor Taxonomy

Each quote should carry enough metadata to build the correct helper or surface node. The platform uses a unified risk
factor taxonomy to ensure that deterministic risk, stress, and simulation share the same factor map.

**Format:** `RF:<family>:<currency_or_market>:<object>:<bucket>`, with shorter spot forms where no tenor is needed.

**Supported Families:**

- `RF:RATES:<CCY>:<CURVE>:<TENOR>` (e.g., `RF:RATES:USD:OIS:2Y`)
- `RF:RATESVOL:<CCY>:<SURFACE>:<EXPIRY>:<TENOR_OR_STRIKE>`
- `RF:CREDIT:<ENTITY>:<SPREAD/HAZARD/RECOVERY>:<TENOR_OR_SPOT>`
- `RF:CREDITVOL:<ENTITY>:<EXPIRY>:<TENOR_OR_STRIKE>`
- `RF:FX:<CCYPAIR>:SPOT` and `RF:FX:<CCYPAIR>:FWDPTS_<TENOR>`
- `RF:FXVOL:<CCYPAIR>:<EXPIRY>:<DELTA_OR_STRIKE>`
- `RF:EQ:<TICKER>:SPOT`, `RF:EQ:<TICKER>:DIVYLD:<TENOR>`, `RF:EQ:<TICKER>:BORROW:<TENOR>`, and `RF:EQ:<INDEX>:FUT:<TENOR>`
- `RF:EQVOL:<TICKER_OR_INDEX>:<EXPIRY>:<STRIKE>`
- `RF:COM:<UNDERLIER>:SPOT`, `RF:COM:<UNDERLIER>:FWD:<TENOR>`, and `RF:COMVOL:<UNDERLIER>:<EXPIRY>:<STRIKE>`

**Tradeoffs:**

- **Pros:** Precise bootstrapping, fewer hardcoded assumptions, seamless attribution.
- **Cons:** More complex JSON schema, larger data payloads.

### 3.2. Curve specifications

A curve specification should identify:

- the target curve ID,
- the quote IDs used,
- the construction family,
- interpolation policy,
- fallback conventions,
- diagnostic output preferences.

## 4. Separation Of OIS And Projection Curves

A production rates platform should generally separate:

- **discount curves** for collateral-consistent discounting,
- **projection curves** for floating-rate forecast generation.

This matters because post-crisis rates pricing is generally multi-curve rather than single-curve. Even if the current
prototype still sometimes falls back to one curve, the design should preserve a clean separation.

## 5. Conventions

The role of `MarketConventionRegistry` is to keep conventions out of pricing logic.

It should eventually own:

- currency calendars,
- day counts,
- business-day conventions,
- settlement lags,
- fixed-leg conventions,
- floating-index conventions,
- compounding and frequency rules,
- quote conventions.

## 6. QuantLib rationale

The main QuantLib choices in the market layer are:

- `SimpleQuote` for mutable market nodes,
- `RateHelper` classes for market-consistent bootstrapping,
- piecewise term structures for production calibration,
- later `RelinkableHandle` for efficient scenario and simulation reuse.

## 7. Next concrete implementation steps

1. enrich `MarketQuote` and `CurveSpec`,
2. expand the convention registry beyond rates,
3. split the current monolithic builder into typed builders,
4. add explicit `MarketSnapshot` diagnostics,
5. introduce credit and volatility builders behind stable interfaces.
