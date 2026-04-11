# Design Documentation Index

This section is the canonical design layer for the platform.

The design notes explain how market data, curves, instruments, analytics services, persistence, and language boundaries fit together into a single production-shaped system.

## What belongs in this section

This section is the home for:

- platform boundaries and module responsibilities,
- market-state and curve-object design,
- QuantLib integration choices and observer semantics,
- persistence, lineage, and reconciliation requirements,
- implementation workflow and binding strategy.

## Recommended study order

### Phase 1 — global structure

1. `docs/design/ARCHITECTURE.md`  
   Read this first to understand the platform layers, the boundary between data, analytics, and storage, and the main service façade.

2. `docs/design/MARKET_AND_CURVES.md`  
   Read this next because the market-state schema and reusable curve objects drive almost every later design choice.

3. `docs/design/CURVE_BOOTSTRAP_DESIGN.md`  
   Read this once the market object model is clear. This note explains why bootstrap inputs must remain reconstructible and why quote handles matter.

### Phase 2 — analytics orchestration

4. `docs/design/ANALYTICS_SERVICES.md`  
   This note explains how valuation, sensitivities, explain, stress, VaR, and simulation should be exposed as coherent services.

5. `docs/design/IMPLEMENTATION_CHOICES.md`  
   This is the repository-specific rationale note. It should explain not only *what* the code does, but *why* the code is shaped that way.

### Phase 3 — persistence and production constraints

6. `docs/design/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`  
   Read this before touching historical replay, control totals, or auditability. The stored data model determines which analyses remain reproducible.

7. `docs/design/PLATFORM_IMPLEMENTATION_GUIDE.md`  
   This is the practical build path from repository skeleton to working platform.

8. `docs/design/PYTHON_CPP_PERFORMANCE_AND_BINDINGS.md`  
   This note explains the language split and performance rationale.

### Phase 4 — QuantLib-specific support material

9. `docs/design/QUANTLIB_CURVE_API_CHEAT_SHEET.md`  
   Use this as a reference while reading the curve and implementation notes.

## Canonical files

- `docs/design/ANALYTICS_SERVICES.md`
- `docs/design/ARCHITECTURE.md`
- `docs/design/CURVE_BOOTSTRAP_DESIGN.md`
- `docs/design/DATA_STORAGE_LINEAGE_AND_RECONCILIATION.md`
- `docs/design/IMPLEMENTATION_CHOICES.md`
- `docs/design/MARKET_AND_CURVES.md`
- `docs/design/PLATFORM_IMPLEMENTATION_GUIDE.md`
- `docs/design/PYTHON_CPP_PERFORMANCE_AND_BINDINGS.md`
- `docs/design/QUANTLIB_CURVE_API_CHEAT_SHEET.md`

## Why this order works

The correct design dependency is:

$$
\text{architecture} \rightarrow \text{market state} \rightarrow \text{curve construction} \rightarrow \text{analytics services} \rightarrow \text{storage and controls}
$$

This order matters because a risk or persistence design that ignores market-object structure usually becomes inconsistent later.

For example, if the market layer does not preserve quote identity, conventions, and curve lineage, then a later request such as historical replay or aged-horizon revaluation becomes fragile. In symbolic form, if valuation is a function

$$
PV = f(\text{trade state}, \text{market state}, \text{model state}, \text{valuation context}),
$$

then reproducibility requires the stored representation to preserve enough information to reconstruct each argument of $f$.

## Closely related sections

The design notes are tightly coupled to:

- `docs/pricing/INDEX.md`, because the design must support the pricing objects described there,
- `docs/risk/INDEX.md`, because factor mapping and scenario semantics depend on the curve and service design,
- `docs/roadmap/INDEX.md`, because implementation planning should only extend the canonical design rather than contradict it.

## Maintenance rule

Whenever an implementation change affects architecture, object lifetime, persistence, or observer semantics, update this section first and then update the roadmap and status notes.
