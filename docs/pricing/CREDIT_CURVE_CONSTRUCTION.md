# Credit Curve Construction

This document is the canonical home for credit-curve design notes as the platform grows.

## Scope

Credit documentation in this repository should eventually cover:

- spread quote schema,
- CDS conventions,
- hazard-rate and default-probability curve construction,
- survival probabilities,
- recovery-rate handling,
- credit discounting vs survival modeling,
- CS01 and spread-factor mapping.

## Intended implementation path

### Phase 1
- extend the market schema to support credit spread instruments,
- define issuer / reference-entity identifiers,
- define recovery-rate storage,
- define a `CreditCurveBuilder` interface.

### Phase 2
- bootstrap survival curves from CDS spreads or simplified spread inputs,
- expose survival probabilities and default densities,
- map credit factors for CS01 and stress.

### Phase 3
- integrate credit curves into pricing and portfolio risk aggregation.

## Why this deserves its own folder

Credit curves are not just another rates curve. They add:

- reference-entity granularity,
- recovery assumptions,
- survival probabilities,
- alignment with default timing and payoff conventions,
- different risk sensitivities than pure rates.

That is why credit belongs under pricing documentation rather than being hidden inside general theory notes.
