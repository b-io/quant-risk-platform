# Risk Factors and Attribution

This document is the canonical home for explicit factor-risk design.

## Purpose

The platform should move from implicit bumping to explicit factor definitions. A risk factor model should identify:

- factor type,
- factor name,
- asset class,
- currency,
- tenor or pillar,
- shock rule,
- aggregation tags,
- mapping to affected trades and curves.

Explicit factor definitions make the engine easier to reason about and much easier to explain. The goal is not only to
produce a number, but also to say which market objects moved, how the move was represented, and how the result should
be aggregated.

## Why this matters

Without explicit factor definitions, the engine can still produce numbers, but it cannot easily answer business
questions such as:

- which curve contributed most to DV01,
- which issuer family drove CS01,
- how risk aggregates by desk or strategy,
- why risk changed between two dates.

A production-shaped risk engine therefore needs a factor vocabulary that is stable across valuation, stress, explain,
and VaR.

## Target objects

A good next-step model would include:

- `RiskFactorDefinition`,
- `RiskFactorShockRule`,
- `RiskAttributionReport`,
- aggregation keys such as book, desk, currency, and strategy.

## Minimum metadata per factor

Each factor definition should include at least:

- a stable factor ID,
- factor family such as rates, credit, FX, equity, commodity, or volatility,
- market object family and curve or surface identifier,
- currency,
- pillar, expiry, tenor, or node label where relevant,
- shock size and shock type,
- optional business tags such as desk, book, strategy, or issuer group.

## Initial factor families

- rates parallel shifts,
- rates key-rate nodes,
- credit spread factors,
- FX spot factors,
- volatility surface nodes later.

As the platform evolves, factor families should remain separate even when they are aggregated together in one report.
That separation is essential for explain, VaR, and stress semantics.

## Attribution views that should be supported

Risk results become much more useful when the same raw shocks can be re-aggregated by several business axes.
Typical views include:

- by trade,
- by portfolio or book,
- by desk or strategy,
- by currency,
- by issuer or sector,
- by factor family,
- by tenor bucket.

## Relationship to scenarios and explain

A factor model should be reused by:

- bump-and-revalue sensitivities,
- historical or hypothetical stress,
- market-move explain,
- VaR methodologies,
- Monte Carlo simulation mappings.

That reuse is one of the main reasons to make factor definitions first-class objects instead of leaving them implicit in
individual analytics services.
