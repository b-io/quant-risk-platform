# Curve Bootstrapping and Reconstruction: Design Choices

This document justifies the design and implementation choices used in the curve-building layer. It complements inline
comments in:

- `cpp/src/market/market_snapshot.cpp`
- `cpp/include/qrp/market/market_state.hpp`

## Goals

- Market-consistent discounting and forwarding.
- Efficient revaluation for risk/scenario runs.
- Extensible to more curve families (credit, vol surfaces) without changing the hot path.

## Technique 1: Piecewise Bootstrapping with Helpers

We use `QuantLib::PiecewiseYieldCurve<Discount, LogLinear>` fed by `RateHelper`s (Deposit, OIS, FRA, IRS).

Why:

- Industry standard. Rate helpers encode instrument-specific pricing equations.
- Calibration stability: LogLinear on discount factors enforces positive discount factors and a smooth discount
  function.
- Deterministic and reproducible.

Implementation notes:

- Helpers are held as `QuantLib::ext::shared_ptr` (ABI-compatible with QuantLib’s build).
- We pass the 4-argument constructor with an explicit `IterativeBootstrap<CurveT>` to avoid overload ambiguity across
  versions.

Tradeoffs:

- Bootstrapping is costlier than parametric fits, but correctness and auditability are superior for a desk-style
  platform.

## Technique 2: OIS Discounting and IBOR Forwarding (Multi-Curve)

Rationale:

- Post-2008 market practice splits discounting (OIS) from forecasting (IBOR/term indices).
- Aligns DV01 attribution with factor families (discount vs. forward curves).

Implementation:

- `PricingContext` maps trades to `{discount_curve_id, forecast_curve_id}`.
- `InstrumentFactory` builds legs using discount and forecast handles separately.

Tradeoffs:

- Requires multiple curves and conventions. We encapsulate this via typed IDs and (future) `MarketConventionRegistry`.

## Technique 3: Handle-Based Quotes and Reactive Curves

We store quotes as `SimpleQuote` handles in `MarketState` and link helpers to these handles.

Why:

- Efficient revaluation: bumping a `SimpleQuote` propagates to the curve via QuantLib’s observer mechanism; no rebuilds.
- Enables high-throughput PV01/key-rate/scenario and Monte Carlo.

Implementation:

- `MarketState` owns `QuantLib::ext::shared_ptr<SimpleQuote>`.
- `CurveBuilder` creates `Handle<Quote>` from these shared quotes when constructing helpers.

## Alternatives Considered

- Nelson–Siegel or spline fits: faster initial fit but weaker instrument consistency and explainability.
- Rebuilding curves per scenario: simpler code but orders of magnitude slower for risk.

## References

- QuantLib Reference: Yield Term Structures, Helpers, and Handles.
- Post-crisis multi-curve literature on OIS discounting and IBOR forecasting.
