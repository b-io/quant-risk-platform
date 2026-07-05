# Equity Documentation Index

This section is the home for equity and equity-index product conventions and
implementation notes. Detailed chapters should be added here as equity support
matures.

## Scope

Equity documentation should cover:

- equity spot and index spot representation;
- forwards and futures;
- dividend curves and dividend-event modeling;
- borrow and funding curves;
- European equity and index options;
- American exercise using LSMC or finite differences;
- index methodology, corporate actions, and settlement conventions;
- equity volatility surfaces and Greeks.

## Product Coverage Sequence

Equity product support follows this implementation order:

1. equity spot exposure;
2. equity forwards and futures;
3. European equity and index options;
4. American equity options using LSMC or finite difference;
5. dividend curve and borrow curve support;
6. baskets, Asians, and barriers after the core infrastructure is stable.

Current platform coverage:

- The canonical portfolio DTO supports equity spot, equity forwards, equity
  futures, and equity/index options.
- Equity forwards and futures use spot or listed futures quotes with discount,
  dividend-yield, and borrow inputs.
- European options use a Black-Scholes cost-of-carry formula. American options
  currently use a recombining binomial exercise tree; replacing that tree with
  LSMC or finite differences remains a model upgrade rather than a DTO gap.
- The portfolio-backed golden fixtures under `data/portfolios/` link equity
  forwards, futures, and European/American options to book-level structural
  coverage.

## Required Platform Objects

Equity support depends on:

- canonical `RiskFactorId` values for equity spot, dividend, borrow, and
  volatility points;
- market snapshots with spot prices, dividend curves, borrow or funding curves,
  and volatility surfaces;
- product state that distinguishes single-name equity, index, future, and option
  contracts;
- exercise-policy support for American options;
- PnL explain components for dividends, carry, market move, exercise, and FX
  translation where applicable.

## Shared References

- `docs/reference/LEXIS.md` defines equity spot, dividend curve, borrow curve,
  equity forward, and American option.
- `docs/reference/FORMULAS.md` defines equity forward and option-parity
  relations.
- `docs/reference/SOURCES.md` lists public equity exchange, index-provider, and
  modeling references.

## Maintenance Rule

Equity chapters should keep dividend, borrow, funding, corporate-action,
exercise, and volatility conventions explicit. These assumptions should be
visible in product state, market state, model state, and pricing diagnostics.
