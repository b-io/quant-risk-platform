# Credit Curve Construction

This document is the canonical home for credit-curve design notes as the platform grows.

## Scope

Credit documentation in this repository should cover:

- spread quote schema,
- CDS conventions,
- hazard-rate and default-probability curve construction,
- survival probabilities,
- recovery-rate handling,
- credit discounting versus survival modeling,
- CS01 and spread-factor mapping.

Credit is intentionally documented separately from rates because the underlying market objects answer different
questions. A rates curve explains discounting or forward projection. A credit curve explains default timing,
survival, and loss-given-default assumptions.

## Core objects

A production-shaped credit stack should expose at least:

- a **credit quote schema** describing instrument type, reference entity, currency, seniority, restructuring clause,
  recovery assumption, maturity, and quote convention,
- a **credit curve specification** describing the curve family, calibration universe, interpolation choice, and
  bootstrap hints,
- a **credit curve object** exposing survival probability, default probability, hazard rate, and risky discounting
  inputs,
- a **factor mapping layer** that ties curve nodes back to CS01, stress, and P&L explain outputs.

## Typical market inputs

The most common inputs are:

- **CDS par spreads** across standard maturities,
- **bond spreads** or spread-to-benchmark observations,
- **recovery-rate assumptions**, either fixed or scenario-dependent,
- **discount curves** used to value premium and protection legs,
- **reference-entity metadata** such as sector, region, issuer family, and seniority.

The schema should make it explicit which quotes are direct calibration inputs and which are auxiliary metadata.

## Construction flow

A practical construction flow is:

1. Normalize credit quotes into a typed market schema.
2. Resolve credit conventions from a registry rather than from hardcoded product logic.
3. Select a calibration family for a reference entity and seniority bucket.
4. Combine the credit quotes with a discount curve in the same currency.
5. Bootstrap a survival or hazard-rate term structure.
6. Publish reusable handles so the same object can be consumed by pricing, stress, and CS01 logic.

That sequence matters because credit construction is not independent of the discounting framework. A CDS curve is
built from spreads, but the valuation equations still require discount factors and convention-aware premium-leg timing.

## Outputs that should be available

A reusable credit curve object should provide, directly or through a thin service layer:

- survival probability $Q(t)$,
- cumulative default probability $1-Q(t)$,
- hazard rate $\lambda(t)$,
- risky PV inputs for CDS or credit-sensitive bonds,
- node labels for curve diagnostics,
- calibration diagnostics such as residual errors or failed nodes.

## Risk implications

The design should support at least the following downstream uses:

- **CS01** by parallel and bucketed spread shocks,
- **historical stress** by replaying spread moves by issuer, sector, or rating bucket,
- **P&L explain** with spread-move and recovery buckets,
- **VaR** where spread factors sit beside rates, FX, and volatility factors.

Credit-factor definitions should therefore carry more metadata than a simple tenor label. They usually need a
reference entity, seniority, currency, and spread family identifier.

## Intended implementation path

### Phase 1

- extend the market schema to support credit spread instruments,
- define issuer or reference-entity identifiers,
- define recovery-rate storage,
- define a `CreditCurveBuilder` interface,
- define diagnostics and serialization fields for calibrated curves.

### Phase 2

- bootstrap survival curves from CDS spreads or simplified spread inputs,
- expose survival probabilities and default densities,
- map credit factors for CS01 and stress,
- separate quoted spread data from scenario overlays.

### Phase 3

- integrate credit curves into pricing and portfolio risk aggregation,
- support issuer-level and sector-level aggregation,
- connect the same credit objects to explain, historical stress, VaR, and Monte Carlo workflows.

## Why this deserves its own folder

Credit curves are not just another rates curve. They add:

- reference-entity granularity,
- recovery assumptions,
- survival probabilities,
- alignment with default timing and payoff conventions,
- different risk sensitivities than pure rates.

That is why credit belongs under pricing documentation rather than being hidden inside general theory notes.
