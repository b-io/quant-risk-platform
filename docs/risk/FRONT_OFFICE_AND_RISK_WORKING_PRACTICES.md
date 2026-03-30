# Front Office and Risk: Good Practice and What They Really Look At

## 1. Why this chapter exists

It is not enough to state formulas without explaining when they are reliable, how they are computed, and what controls
make them trustworthy. Modern front-office teams and independent risk teams look at related but distinct questions, and
both viewpoints matter when designing analytics platforms.

This chapter provides that practical lens.

---

## 2. Front office versus risk: same objects, different priorities

Front office usually focuses on:

- speed,
- tradability,
- sensitivity to market moves,
- hedge ratios,
- live and historical P\&L,
- scenario response under plausible market moves,
- confidence that the numbers update quickly and correctly.

Independent risk usually focuses on:

- reconciliation and control,
- concentration,
- tail scenarios,
- model limitations,
- consistency across books,
- governance and escalation,
- robustness under bad liquidity or stale data.

A useful summary:

> Front office needs fast, actionable numbers; risk needs robust, challengeable numbers. A good platform has to serve
> both without forking the market state or valuation logic.

---

## 3. Realistic rates metrics they look at today

For a modern rates or macro book, realistic daily views include:

- PV / NPV
- DV01 / PV01
- key-rate DV01
- carry and roll-down
- curve steepener / flattener sensitivity
- spread duration
- scenario P\&L
- VaR / expected shortfall
- explain versus actual P\&L
- market-data quality flags

#### Example for 3 — quick rates move viewed by front office and risk

Suppose a book is:

- +USD 5Y bond DV01 = +42k USD/bp
- -USD 2Y swap DV01 = -18k USD/bp

If the market shocks by:

- +10bp in 5Y
- +5bp in 2Y

Approximate P\&L:

$$
\Delta P \approx 42{,}000 \cdot (-10) + (-18{,}000)\cdot(-5)
= -420{,}000 + 90{,}000 = -330{,}000
$$

Meaning:

- the book loses about 330k under that linearized move.

What front office asks:

- was the move parallel or bucketed?
- what hedge can reduce the 5Y concentration?
- what part of the move was expected carry/roll?

What risk asks:

- how much nonlinear residual is hidden behind the linear estimate?
- what if the move is 25bp instead of 10bp?
- how does this combine with spread and volatility shocks?

---

## 4. Approximation hierarchy: what is acceptable and when

### 4.1 Modified duration / DV01

Good for:

- fast first-order explanation,
- directional hedging,
- small moves.

Not enough for:

- large shocks,
- callable or highly convex positions,
- options,
- significant curve reshaping.

### 4.2 Key-rate or bucketed risk

Good for:

- non-parallel curve understanding,
- realistic hedge design,
- concentrated tenor exposures.

### 4.3 Full revaluation

Needed for:

- nonlinear products,
- large stress scenarios,
- path-dependent positions,
- proper P\&L explain.

Best practice:

- show fast approximations and full revaluation side by side,
- document why the approximation is acceptable or not.

---

## 5. What good practice looks like in modern times

### 5.1 One canonical market state

Do not let:

- front office curve,
- risk curve,
- stress curve,
- explain curve

all drift separately without traceability.

Good practice:

- one canonical market snapshot,
- explicit scenario overlays,
- versioned market data and conventions.

### 5.2 Reproducibility

For every result, be able to answer:

- what market snapshot?
- what curve version?
- what pricing model version?
- what random seed / simulation config?
- what trade population?

### 5.3 Explainability

A realistic P\&L explain should split:

- carry / roll,
- market move,
- new trades,
- fixings / cash,
- residual.

### 5.4 Controls

Modern teams care about:

- stale quote detection,
- curve-arbitrage sanity checks,
- negative or explosive forwards,
- broken fixings,
- scenario completeness,
- missing sensitivities.

### 5.5 Performance with transparency

Fast numbers that cannot be explained are not good enough.
Slow numbers that cannot support the desk are also not good enough.

The right balance is:

- shared pricing core,
- incremental updates,
- cached curve objects,
- explicit approximations,
- and easy drill-down from aggregate risk to trade-level contributors.

---

## 6. What realistic macro / rates teams watch after a data release

Suppose CPI surprises on the upside.

Front office immediately checks:

- front-end rates repricing,
- curve steepening or flattening,
- FX reaction,
- spread widening or tightening,
- live book P\&L,
- key bucket moves.

Risk checks shortly after:

- whether large limit consumption appeared,
- whether concentrated books now dominate desk risk,
- whether stress results changed materially,
- whether the move exposed stale hedge assumptions.

That is why a quant developer should think beyond isolated pricing functions.

---

> For small, liquid rates moves, modified duration and DV01 are useful first-order tools, but they should still be
> backed by full bucketed risk and scenario revaluation.

> Front office wants speed and hedging relevance; risk wants traceability, controls, and challengeability. The platform
> has to satisfy both from the same underlying market state.

> In modern practice it is better to expose both the approximation and the exact revaluation than to hide the
> approximation behind a single reported number.

---

## 8. Checklist for interpreting a formula in practice

- what it means,
- one concrete number,
- where it is good enough,
- where it breaks,
- and what front office or risk would do with it.

This distinction separates abstract formulas from platform-relevant risk interpretation.
