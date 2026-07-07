# FX Documentation Index

This section is the home for FX product conventions and implementation notes.
Detailed chapters belong here when they describe FX-specific coverage,
conventions, or valuation choices.

## Scope

FX documentation should cover:

- currency-pair conventions and quotation direction;
- spot settlement lag and holiday calendars;
- FX forwards, forward points, and broken-date interpolation;
- FX swaps and funding interpretation;
- non-deliverable forwards and fixing conventions;
- domestic and foreign discount-curve selection;
- FX volatility surfaces, delta conventions, smiles, and Greeks;
- settlement currency, cashflow currency, and reporting currency.

## Product Coverage

FX product coverage is organized around these product families:

1. FX spot exposure;
2. FX forwards;
3. FX swaps;
4. non-deliverable forwards;
5. vanilla European FX options;
6. FX option Greeks and volatility-surface risk;
7. barriers, digitals, and TARFs.

Current platform coverage:

- The canonical portfolio DTO supports FX spot, forwards, swaps, NDFs, and
  vanilla European FX options.
- Pricing uses spot rates, forward points, domestic and foreign discounting
  inputs, and volatility quotes where applicable.
- Risk factor bindings cover FX spot, forward points, and FX volatility inputs,
  and PnL explain includes explicit FX translation and market-move component
  slots.
- Barriers, digitals, TARFs, and richer smile/surface risk are outside current
  product coverage.

## Required Platform Objects

FX support depends on:

- canonical `RiskFactorId` values for FX spot, forward, and volatility points;
- market snapshots with spot rates, forward points, discount curves, and
  volatility surfaces;
- pricing results that state valuation currency and cashflow currencies;
- PnL explain components for FX translation and market move.

## Shared References

- `docs/reference/LEXIS.md` defines FX spot, forwards, FX swaps, NDFs, and FX
  volatility surfaces.
- `docs/reference/FORMULAS.md` defines the no-arbitrage FX forward relation.
- `docs/reference/SOURCES.md` lists public FX convention and source classes.

## Maintenance Rule

FX chapters should keep quotation convention, settlement convention, curve
selection, and reporting currency explicit. Avoid relying on hidden domestic or
foreign currency defaults.
