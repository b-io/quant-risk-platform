# Quant Risk Platform

Production-shaped C++ and Python platform for pricing, risk analytics, scenario analysis, and P&L across asset
classes, with QuantLib at the core and Python used as an interface layer.

> The current implementation focus is rates. The architecture is intentionally designed to extend toward credit, FX,
> equities, commodities, and volatility-driven analytics without changing the core layering.

## Quickstart

For build, test, and demo instructions, including Python binding workflows and smoke tests,
see [QUICKSTART.md](QUICKSTART.md).

### Supported builds on Windows (summary)

- C++ only: Debug, Release, RelWithDebInfo
- Python bindings: Release, RelWithDebInfo

Suggested commands:

- C++ only Release: `cmake --preset Release && cmake --build --preset Release --target qrp_core`
- Python Release:
  `cmake --preset Release-Python && cmake --build --preset Release-Python --target quant_risk_platform && ctest --preset Release-Python -R python_import`

## Documentation

The canonical project documentation lives under [`docs/`](docs/README.md).

Recommended starting points:

- [`docs/architecture/ARCHITECTURE.md`](docs/architecture/ARCHITECTURE.md)
- [`docs/architecture/MARKET_AND_CURVES.md`](docs/architecture/MARKET_AND_CURVES.md)
- [`docs/architecture/ANALYTICS_SERVICES.md`](docs/architecture/ANALYTICS_SERVICES.md)
- [`docs/market-data/INDEX.md`](docs/market-data/INDEX.md)
- [`docs/asset-classes/INDEX.md`](docs/asset-classes/INDEX.md)
- [`docs/models/INDEX.md`](docs/models/INDEX.md)
- [`docs/asset-classes/commodities/INDEX.md`](docs/asset-classes/commodities/INDEX.md)
- [`docs/risk/INDEX.md`](docs/risk/INDEX.md)
- [`docs/foundations/INDEX.md`](docs/foundations/INDEX.md)
- [`docs/reference/INDEX.md`](docs/reference/INDEX.md)
- [`docs/implementation/PHASED_BUILD_PLAN.md`](docs/implementation/PHASED_BUILD_PLAN.md)

## Overview

Quant Risk Platform is a compact but production-shaped analytics platform designed to demonstrate how pricing, risk,
and P&L workflows can be implemented with reusable market state, reusable instruments, and clear service boundaries.

The project combines a high-performance C++ core with thin Python-facing bindings, with a focus on:

- pricing and risk analytics for trading portfolios,
- market data and curve construction,
- scenario analysis and stress testing,
- P&L explain and risk decomposition,
- extensibility across asset classes,
- clean engineering, testability, and operational robustness.

## Core principles

- **C++ owns analytics**: pricing, curve building, scenarios, risk, stress, P&L explain, and Monte Carlo belong in the
  C++ core.
- **Python is an interface layer**: bindings, demos, notebooks, and simple orchestration only.
- **Separate market state from positions**: curves and quotes should be built once and reused across many trades.
- **Build once, reuse many times**: the same built market objects and instruments should support valuation, risk,
  stress, explain, and simulation.
- **Wrap QuantLib behind platform abstractions**: use QuantLib as the implementation engine, but keep repository-level
  interfaces stable and platform-shaped.
- **Keep `docs/` canonical**: documentation should track the code and public design rationale as the project evolves.

## Current repository status

The repository is beyond scaffold stage and already contains:

- a C++ core under `cpp/` for market construction, instruments, and analytics services,
- pybind11 bindings under `cpp/bindings/`,
- a CLI under `cpp/cli/`,
- sample market and portfolio data under `data/`,
- tests under `tests/`,
- canonical documentation under `docs/`.

Current implementation maturity is best described as a
**rates-focused platform with persistent storage and reactive risk foundation**. The next architecture milestones are
core trade/result hardening, market-data normalization, refined risk-factor taxonomy, and staged asset-class expansion.

## Implemented today

The current codebase already includes:

- market loading and JSON DTOs,
- convention-aware rates market construction,
- QuantLib helper-based rates curve bootstrapping,
- pricing for selected vanilla rates instruments,
- high-performance reactive risk (Observer pattern),
- SQLite-backed persistence for portfolios, market snapshots, and results,
- end-to-end persisted valuation and risk workflows,
- Python bindings and a C++ CLI.

## Planned next steps

- expand pricing coverage to FX, Equities, and Credit,
- refine the risk factor taxonomy for consistent aggregation,
- separate historical, parametric, and Monte Carlo VaR,
- evolve Monte Carlo from one-step factor simulation toward a fuller path-based framework,
- add BI-style ad hoc analysis over stored results.

## Documentation layout

```text
docs/
|-- design/      # architecture, layering, market and service design
|-- pricing/     # pricing and market-construction notes by subdomain
|   |-- market-data/
|   |-- rates/
|   |-- credit/
|   `-- volatility/
|-- energy/      # power, gas, carbon, flexibility, and energy-market risk notes
|-- risk/        # risk measures, scenarios, stress, VaR, and Monte Carlo
`-- theory/      # mathematical and statistical foundations
```

## Repository structure

```text
quant-risk-platform/
|-- CMakeLists.txt
|-- CMakePresets.json
|-- QUICKSTART.md
|-- README.md
|-- LICENSE
|-- vcpkg.json
|-- cmake/
|-- cpp/
|   |-- include/qrp/
|   |   |-- analytics/
|   |   |-- conventions/
|   |   |-- domain/
|   |   |-- instruments/
|   |   |-- io/
|   |   |-- market/
|   |   `-- util/
|   |-- bindings/
|   |-- cli/
|   `-- src/
|-- data/
|   |-- market/
|   |-- portfolios/
|   `-- scenarios/
|-- docs/
|   |-- design/
|   |-- pricing/
|   |   |-- market-data/
|   |   |-- rates/
|   |   |-- credit/
|   |   `-- volatility/
|   |-- energy/
|   |-- risk/
|   `-- theory/
|-- python/
|   |-- examples/
|   |-- notebooks/
|   |-- pyproject.toml
|   |-- requirements.txt
|   |-- uv.lock
|   `-- qrp/
|-- scripts/
|   `-- coverage/
|       |-- cpp/
|       `-- python/
`-- tests/
    |-- integration/
    |-- regression/
    `-- unit/
```

## Example workflows

- build rates curves from market quotes,
- price a portfolio of vanilla rates instruments,
- compute PV01 or bucketed curve risk,
- apply market shocks and generate stressed P&L,
- compare base and shocked market states,
- drive the same workflows through Python for demos and orchestration.

## Technology stack

- **C++20** for the core analytics engine,
- **QuantLib** for curve and instrument building blocks,
- **CMake** for reproducible builds,
- **fmt** for formatting,
- **nlohmann-json** for JSON parsing,
- **pybind11** for Python bindings,
- **GoogleTest** for tests,
- **Python** for orchestration, demos, and notebooks.

## Design priorities

- **Production-shaped** rather than notebook-shaped,
- **Extensible** across asset classes,
- **Transparent** in valuation and risk decomposition,
- **Testable** with deterministic samples and regression coverage,
- **Scalable** toward larger portfolios, repeated scenarios, and future parallelization,
- **Pragmatic** about valuation, risk, and reporting workflows.

## Disclaimer

This repository is a technical demonstration project intended for educational and professional portfolio purposes. It is
not investment advice and should not be used for production trading or risk management without additional validation,
controls, governance, and operational hardening.
