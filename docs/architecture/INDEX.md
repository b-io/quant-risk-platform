# Architecture Documentation Index

This section defines the platform architecture: module boundaries, market
objects, curve construction, analytics services, persistence, lineage, language
bindings, and implementation rationale.

## Scope

Architecture chapters should explain how the platform is shaped. They should
not duplicate asset-class formulas, glossary entries, or temporary roadmap
notes.

This section is the home for:

- platform layers and service boundaries;
- market-state and curve-object design;
- QuantLib integration and observer semantics;
- persistence, lineage, reconciliation, and replay;
- Python and C++ boundary decisions;
- implementation rationale that affects more than one product family.

## Recommended Reading Order

1. `docs/architecture/ARCHITECTURE.md`  
   Platform layers, data and analytics boundaries, and the main service facade.

2. `docs/architecture/MARKET_AND_CURVES.md`  
   Market-state schema and reusable curve objects.

3. `docs/architecture/CURVE_BOOTSTRAP_DESIGN.md`  
   Bootstrap inputs, reconstructibility, and quote-handle design.

4. `docs/architecture/ANALYTICS_SERVICES.md`  
   Valuation, sensitivities, explain, stress, VaR, and simulation as coherent
   service interfaces.

5. `docs/architecture/IMPLEMENTATION_CHOICES.md`  
   Repository-specific rationale for implementation shape.

6. `docs/architecture/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`  
   Historical replay, control totals, auditability, and reproducible storage.

7. `docs/architecture/PLATFORM_IMPLEMENTATION_GUIDE.md`  
   Practical build path from repository skeleton to working platform.

8. `docs/architecture/PYTHON_CPP_PERFORMANCE_AND_BINDINGS.md`  
   Language split, binding design, and performance rationale.

9. `docs/architecture/QUANTLIB_CURVE_API_REFERENCE.md`  
   QuantLib curve and term-structure API reference used by the implementation
   notes.

## Canonical Files

- `docs/architecture/ANALYTICS_SERVICES.md`
- `docs/architecture/ARCHITECTURE.md`
- `docs/architecture/CURVE_BOOTSTRAP_DESIGN.md`
- `docs/architecture/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`
- `docs/architecture/FACTOR_ONLY_SCENARIO_REFACTOR_REVIEW.md`
- `docs/architecture/IMPLEMENTATION_CHOICES.md`
- `docs/architecture/MARKET_AND_CURVES.md`
- `docs/architecture/PLATFORM_IMPLEMENTATION_GUIDE.md`
- `docs/architecture/PYTHON_CPP_PERFORMANCE_AND_BINDINGS.md`
- `docs/architecture/QUANTLIB_CURVE_API_REFERENCE.md`

## Architectural Dependency

The architecture dependency is:

$$
\text{architecture}
\rightarrow
\text{market state}
\rightarrow
\text{curve construction}
\rightarrow
\text{analytics services}
\rightarrow
\text{storage and controls}.
$$

If valuation is represented as

$$
PV
=
f(
\text{trade state},
\text{market state},
\text{model state},
\text{valuation context}
),
$$

then reproducibility requires stored inputs to reconstruct each argument of
$f$. This is the central link between architecture, market data, and controls.

## Closely Related Sections

- `docs/market-data/INDEX.md` defines normalized quotes, curves, surfaces, and
  snapshots.
- `docs/asset-classes/INDEX.md` defines the product families that consume those
  market objects.
- `docs/models/INDEX.md` defines reusable model families.
- `docs/risk/INDEX.md` defines factor mapping, scenario semantics, and output
  controls.
- `docs/implementation/INDEX.md` defines the phased platform build plan.

## Maintenance Rule

Whenever an implementation change affects architecture, object lifetime,
persistence, service boundaries, or observer semantics, update the relevant
architecture note in the same change.
