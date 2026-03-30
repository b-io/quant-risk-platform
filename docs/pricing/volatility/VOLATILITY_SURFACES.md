# Volatility Surfaces

This document is the canonical market-data and construction note for volatility in the platform. It is intentionally
written as a practitioner-oriented reference rather than a short checklist.

Volatility is not a single number. In modern pricing systems it is a structured market object with:

- quote coordinates,
- convention metadata,
- interpolation rules,
- arbitrage constraints,
- calibration lineage,
- and explicit downstream risk mapping.

The same 25% volatility number can mean very different things depending on whether it is:

- an equity implied volatility at strike 100 and maturity 6M,
- an FX 25-delta risk reversal at 1Y,
- a normal swaption volatility for expiry 5Y into tenor 10Y,
- or a caplet lognormal volatility at strike 3.00%.

A robust platform must preserve those distinctions from the first market-data object onward.

---

## 1. What a volatility surface is

A volatility surface is a function that maps quote coordinates to an implied volatility. The coordinate system depends
on asset class and market convention.

Typical examples:

- **equity**: volatility as a function of expiry and strike,
- **FX**: volatility as a function of expiry and delta or strike,
- **rates cap/floor**: volatility as a function of option maturity and strike,
- **rates swaptions**: volatility as a function of option expiry, swap tenor, and strike or moneyness,
- **commodity options**: volatility as a function of expiry and strike, often with delivery-specific structure.

In implementation terms the surface is the market-side object from which pricing models read implied volatilities.

---

## 2. Why implied volatility exists at all

A vanilla option price can be inverted into the volatility that makes a benchmark model reproduce that price.

For equities, that benchmark is usually Black-Scholes-Merton. For forwards and futures, it is often Black 76. For many
rates desks, the quoting convention may be:

- normal volatility,
- lognormal volatility,
- shifted lognormal volatility.

The point of implied volatility is not that the benchmark model is “true.” The point is that it gives the market a
stable quoting language.

Practitioners therefore use volatility surfaces for three different tasks at once:

1. **market representation** — storing liquid market quotes in a comparable form,
2. **pricing input** — feeding pricing engines for vanilla and exotic products,
3. **risk language** — expressing vega and smile risk in intuitive surface buckets.

---

## 3. Quote coordinates and required metadata

A normalized volatility quote should carry enough metadata to reconstruct exactly where it sits on a surface and how it
must be interpreted.

### 3.1 Minimal normalized fields

Typical fields include:

- asset class,
- underlying or index name,
- currency if relevant,
- expiry,
- tenor where relevant,
- strike, moneyness, or delta,
- option type where required by convention,
- volatility type,
- quote convention,
- surface-family identifier,
- source,
- timestamp,
- quality flags.

### 3.2 Why this is non-negotiable

Without metadata, the same number can be interpreted in incompatible ways.

Examples:

- 45% could be a lognormal equity vol,
- 45 basis points could be a normal swaption vol,
- 25-delta could mean spot delta or forward delta,
- strike could be absolute, relative, or shifted.

If the schema loses those distinctions, the platform can still compile but pricing and risk will silently be wrong.

---

## 4. Surface families by asset class

### 4.1 Equity

Equity options are usually stored as expiry-strike surfaces or expiry-moneyness surfaces.

Common conventions:

- strike,
- log-moneyness,
- delta,
- sticky-strike or sticky-delta scenario behavior.

Important practical features:

- skew is often materially negative for indices,
- dividends matter,
- spot-vol correlation matters for exotics,
- calibration must distinguish liquid listed quotes from stale strikes.

### 4.2 FX

FX surfaces are usually quoted in a delta language rather than directly by strike. A common market set for one expiry
is:

- ATM,
- 25-delta risk reversal,
- 25-delta butterfly,
- sometimes 10-delta smile points.

That means the platform must convert between:

- market quote representation,
- internal strike representation,
- and pricing-engine coordinates.

This conversion depends on:

- domestic and foreign discount curves,
- spot vs. forward delta convention,
- premium-adjusted vs. non-premium-adjusted delta.

### 4.3 Rates: cap/floor volatility

Cap/floor volatilities are often represented by:

- option maturity,
- strike,
- volatility type: normal, lognormal, shifted lognormal.

The correct choice depends on the rate regime. When rates are very low or negative, normal or shifted-lognormal
quoting is often more stable than pure lognormal quoting.

### 4.4 Rates: swaption volatility

Swaptions are commonly quoted by:

- expiry,
- underlying swap tenor,
- strike or moneyness,
- volatility type.

This creates a cube rather than a simple surface when strike dependence is included.

### 4.5 Commodities

Commodity surfaces often reflect:

- delivery month structure,
- seasonality,
- convenience yield effects,
- Samuelson-style term-structure behavior where short-dated volatility exceeds long-dated volatility.

---

## 5. Volatility types: normal, lognormal, shifted lognormal

A platform should make volatility type explicit because the same quoted number cannot be used interchangeably across
pricing engines.

### 5.1 Lognormal volatility

Used in Black-Scholes or Black 76 style models.

Best when:

- the underlying is positive,
- relative moves are natural,
- the market quotes in Black implied volatility.

Trade-off:

- pure lognormal dynamics become awkward when forwards or rates can approach zero or go negative.

### 5.2 Normal volatility

Used in Bachelier-style pricing.

Best when:

- the underlying can be near zero,
- absolute moves matter more than proportional moves,
- rates desks quote normal vols for swaptions in low-rate regimes.

Trade-off:

- negative rates are allowed naturally, but large positive rates may be less realistic in some contexts.

### 5.3 Shifted lognormal volatility

A compromise frequently used in rates.

Instead of modeling $F$ directly as lognormal, model $F + s$ as lognormal for a chosen shift $s > 0$.

Best when:

- the market wants to keep Black-style quoting,
- forwards can be near or below zero,
- the shift is an accepted desk convention.

Trade-off:

- the shift becomes a model parameter with practical but not always deep economic meaning.

---

## 6. Construction workflow

A production surface builder usually performs the following steps.

### 6.1 Ingest raw quotes

Collect raw market quotes with source timestamps and raw vendor conventions.

### 6.2 Normalize coordinates and conventions

Convert raw quote representation into canonical internal representation.

Examples:

- delta quotes to strikes,
- premium-adjusted deltas to a single internal standard,
- normal-vol basis points to decimal units,
- expiries and tenors to canonical business-calendar coordinates.

### 6.3 Group quotes into coherent surface families

The builder must decide which quotes belong to the same surface object. A typical surface family key may include:

- asset class,
- currency,
- underlying,
- volatility type,
- quote convention,
- market snapshot ID.

### 6.4 Validate surface completeness

Checks include:

- required coordinates present,
- no duplicate nodes with inconsistent values,
- enough points for interpolation,
- no unit mismatch,
- no impossible expiry/tenor combinations.

### 6.5 Build interpolation object

Choose interpolation in the relevant dimensions.

Typical choices:

- linear in volatility,
- linear in total variance,
- spline smoothing,
- SABR parameter interpolation,
- smile-by-smile construction with tenor interpolation.

### 6.6 Run diagnostics

Diagnostics should expose:

- missing buckets,
- extrapolated queries,
- suspicious local arbitrage,
- discontinuities across expiries,
- stale quotes,
- calibration failures if model fitting is used.

---

## 7. Interpolation choices and trade-offs

Interpolation is one of the largest practical design choices because the market is sparse but pricing queries are dense.

### 7.1 Linear in volatility

Simple and transparent.

Advantages:

- easy to explain,
- robust,
- cheap to compute.

Limitations:

- does not preserve total variance linearity,
- can distort time scaling,
- may create unrealistic smile shapes.

### 7.2 Linear in total variance

Interpolate $w(T,K)=\tau \sigma^2(T,K)$ rather than $\sigma$ itself.

Advantages:

- often more stable across maturities,
- more aligned with variance accumulation intuition,
- common market practice for some surfaces.

Limitations:

- still not a full arbitrage guarantee,
- smile interpolation across strike may remain problematic.

### 7.3 Spline or smooth interpolation

Advantages:

- visually smooth,
- differentiability can help Greeks.

Limitations:

- can overshoot,
- may create local arbitrage,
- harder to explain to validation and risk.

### 7.4 Model-based interpolation

Examples:

- fit SABR slice parameters by expiry,
- fit SVI or SSVI in equity/FX,
- interpolate model parameters rather than raw quotes.

Advantages:

- can enforce shape discipline,
- often extrapolates better,
- can encode desk intuition.

Limitations:

- calibration can fail,
- parameters may be unstable,
- model risk becomes part of the market-data layer.

---

## 8. No-arbitrage considerations

Volatility surfaces are not just pretty charts. Bad interpolation can imply static arbitrage.

Important no-arbitrage checks include:

- **calendar consistency**: longer expiry should not imply impossible option-price ordering,
- **butterfly consistency**: option prices as a function of strike should preserve convexity,
- **monotonicity** where required by option-price structure,
- **non-negative total variance**.

In practice, many platforms adopt a layered approach:

1. allow transparent interpolation for day-to-day risk,
2. detect suspicious regions,
3. optionally use arbitrage-aware fitting for production or validation-critical surfaces.

Modern practice is pragmatic rather than doctrinaire. Desks often prefer a surface that is stable, explainable, and
close to the market over an elegant calibration that is numerically fragile.

---

## 9. Numerical examples

### 9.1 Example: total-variance interpolation across maturities

Suppose one strike is quoted at:

- 6M volatility: $20\%$,
- 1Y volatility: $24\%$.

Then total variances are:

$$
0.5 \times 0.20^2 = 0.0200,
$$

$$
1.0 \times 0.24^2 = 0.0576.
$$

At $T=0.75$ years, linear interpolation in total variance gives:

$$
w(0.75)=0.0200 + 0.5(0.0576-0.0200)=0.0388.
$$

So the interpolated volatility is:

$$
\sigma(0.75)=
\sqrt{\frac{0.0388}{0.75}}
\approx 22.75\%.
$$

This is often better behaved than directly interpolating $20\%$ and $24\%$ linearly in volatility space.

### 9.2 Example: normal vs. lognormal interpretation

Suppose a 1Y swaption market quote is either:

- 100 bp normal volatility,
- or 20% lognormal volatility.

These are not interchangeable. The correct option premium depends on:

- the forward swap rate,
- the strike,
- the annuity,
- and the pricing formula associated with the quote type.

This is why `vol_type` must be part of the surface identity, not just an attribute for display.

### 9.3 Example: sparse FX smile input

Suppose 1Y EURUSD quotes are:

- ATM vol = 9.0%,
- 25-delta risk reversal = -1.2%,
- 25-delta butterfly = 0.4%.

A builder may reconstruct:

- 25-delta call vol,
- 25-delta put vol,
- ATM anchor,
- then convert the delta coordinates to strike coordinates using the current spot, forward, and discount curves.

The main lesson is that the surface object may be strike-based internally even when the market is delta-based
externally.

---

## 10. Sticky rules under scenarios

A volatility surface under stress or VaR is not only a static object. It also needs a rule for how it moves when the
underlying or market state changes.

Common rules:

- **sticky strike**: volatility at a strike stays attached to that strike,
- **sticky delta**: volatility stays attached to a moneyness or delta coordinate,
- **model-consistent re-marking**: the surface is rebuilt from a recalibrated model.

Trade-offs:

- sticky strike is simple and common for equity-style risk reports,
- sticky delta may better reflect FX market intuition,
- model-consistent re-marking is theoretically cleaner but operationally heavier and less transparent.

Modern platforms often support more than one rule because front office, market risk, and model validation may each want
different scenario semantics.

---

## 11. How the surface should appear in the platform design

A reusable surface object should provide at least:

- implied volatility lookup at market nodes,
- interpolated volatility lookup between nodes,
- explicit coordinate metadata,
- quote-convention metadata,
- node labels for vega bucketing,
- diagnostics on missing regions,
- an extrapolation policy,
- provenance: source, build time, and calibration diagnostics.

A builder abstraction such as `VolSurfaceBuilder` should hide library-specific details while exposing the platform’s
chosen semantics.

---

## 12. Relationship to pricing models

The surface is not the same thing as the dynamics model.

Examples:

- Black-Scholes-Merton uses one volatility number,
- Black 76 prices options on forwards or futures,
- local volatility fits a full surface exactly but imposes deterministic diffusion,
- stochastic-volatility models fit smile dynamics more realistically but not always exactly,
- SABR is widely used for rates smile parameterization.

This repository therefore separates:

- **surface construction** — this document,
- **exercise-style and option-pricing choices** — see `docs/pricing/volatility/OPTION_PRICING_AND_EXERCISE_STYLES.md`,
- **volatility models and calibration trade-offs** — see
  `docs/pricing/volatility/VOLATILITY_MODELS_AND_CALIBRATION_TRADEOFFS.md`.

---

## 13. Modern practice

Modern practice is usually:

- store the market in implied-volatility language,
- preserve conventions explicitly,
- interpolate conservatively,
- use separate pricing models for different product families,
- map vega to stable desk buckets,
- and keep scenario semantics explicit rather than hidden.

In other words, the surface is both a market object and a risk object. It should therefore be engineered with the same
care as discount curves and credit curves.
