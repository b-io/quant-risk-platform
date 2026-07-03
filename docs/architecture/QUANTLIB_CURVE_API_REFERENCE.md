# QuantLib Curve API Reference

This note summarizes the QuantLib curve classes and query methods that appear most often in a production rates or credit stack.

## 1. Core idea

A term structure object provides values such as discount factors, zero rates, forward rates, or survival probabilities as a function of time or date. In practice, the platform should wrap QuantLib objects behind repository-specific interfaces, but it is still useful to know the underlying QuantLib vocabulary.

## 2. Common yield-curve objects

- `ql.FlatForward` - flat zero or flat forward curve used for tests and simple scenarios.
- `ql.DiscountCurve` - curve defined directly by discount factors.
- `ql.ZeroCurve` - curve defined by zero rates.
- `ql.ForwardCurve` - curve defined by instantaneous or simple forwards depending on construction.
- `ql.PiecewiseLogLinearDiscount` and related piecewise curves - bootstrapped market-consistent curves.
- `ql.RelinkableYieldTermStructureHandle` - mutable handle used to relink a pricing engine or index to a new curve without rebuilding every dependent object.

## 3. Core queries

Let `curve` be a yield term structure and let `T` be a maturity date.

- `curve.discount(T)` - discount factor to date `T`.
- `curve.zeroRate(T, dayCounter, compounding, frequency)` - zero rate implied by the curve.
- `curve.forwardRate(T1, T2, dayCounter, compounding, frequency)` - forward rate between `T1` and `T2`.
- `curve.referenceDate()` - curve anchor date.
- `curve.dayCounter()` - day-count convention used by the curve.
- `curve.maxDate()` - last supported date.

## 4. Indices and forwarding curves

A floating index such as `ql.Sofr`, `ql.Euribor3M`, or `ql.USDLibor(ql.Period('3M'))` is usually built with a forwarding-curve handle. The index uses that handle to project floating coupons and can also rely on fixings history.

Typical pattern:

- build a discount curve,
- build one or more forwarding curves,
- bind each index to the correct forwarding handle,
- bind discounting engines to the discount curve,
- price trades from a coherent curve set.

## 5. Credit objects

For credit work the key objects are the default-probability term structures, for example hazard-rate or survival-probability curves. The same practical ideas apply:

- build from normalized quotes,
- expose survival probabilities and default probabilities,
- keep handles reusable across pricing, stress, and CS01 runs,
- archive enough metadata to reproduce the calibration.

## 6. Practical use rules

- Use flat curves for unit tests and controlled scenario analysis.
- Use piecewise bootstrapped curves for market-consistent pricing.
- Archive conventions together with quotes and curve specs.
- Prefer relinkable handles over repeated object reconstruction in risk runs.
- Keep QuantLib-specific details inside adapters so the rest of the platform does not depend on low-level implementation choices.
