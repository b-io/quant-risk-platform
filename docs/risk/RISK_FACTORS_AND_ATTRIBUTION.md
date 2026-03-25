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

## Why this matters

Without explicit factor definitions, the engine can still produce numbers, but it cannot easily answer business
questions such as:

- which curve contributed most to DV01,
- which issuer family drove CS01,
- how risk aggregates by desk or strategy,
- why risk changed between two dates.

## Target objects

A good next-step model would include:

- `RiskFactorDefinition`,
- `RiskFactorShockRule`,
- `RiskAttributionReport`,
- aggregation keys such as book, desk, currency, and strategy.

## Initial factor families

- rates parallel shifts,
- rates key-rate nodes,
- credit spread factors,
- FX spot factors,
- volatility surface nodes later.
