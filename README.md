# Quant Risk Platform

Production-shaped C++ and Python platform for pricing, risk analytics, scenario analysis, and P&L across asset
classes, with QuantLib at the core and Python used as an interface layer.

> The current implementation focus is rates. The architecture is intentionally designed to extend toward credit, FX,
> equities, commodities, and volatility-driven analytics without changing the core layering.

## Quickstart

For build, test, and demo instructions, see [QUICKSTART.md](QUICKSTART.md).

## Documentation

The canonical project documentation lives under [`docs/`](docs/README.md).

Recommended starting points:

- [`docs/design/ARCHITECTURE.md`](docs/design/ARCHITECTURE.md)
- [`docs/design/MARKET_AND_CURVES.md`](docs/design/MARKET_AND_CURVES.md)
- [`docs/design/ANALYTICS_SERVICES.md`](docs/design/ANALYTICS_SERVICES.md)
- [`docs/pricing/INDEX.md`](docs/pricing/INDEX.md)
- [`docs/risk/INDEX.md`](docs/risk/INDEX.md)
- [`docs/theory/INDEX.md`](docs/theory/INDEX.md)
- [`docs/roadmap/STATUS.md`](docs/roadmap/STATUS.md)

## Overview

Quant Risk Platform is a compact but production-shaped analytics platform designed to demonstrate how front-office
pricing, risk, and P&L workflows can be implemented with reusable market state, reusable instruments, and clear service
boundaries.

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
- **Keep `docs/` canonical**: documentation should track the code and roadmap as the project evolves.

## Current repository status

The repository is beyond scaffold stage and already contains:

- a C++ core under `cpp/` for market construction, instruments, and analytics services,
- pybind11 bindings under `cpp/bindings/`,
- a CLI under `cpp/cli/`,
- sample market and portfolio data under `data/`,
- tests under `tests/`,
- canonical documentation under `docs/`.

Current implementation maturity is best described as a **rates-focused prototype with real QuantLib foundations**. The
next architecture milestones are richer market conventions and schema, typed market builders, and reusable built
positions.

## Implemented today

The current codebase already includes:

- market loading and JSON DTOs,
- convention-aware rates market construction,
- QuantLib helper-based rates curve bootstrapping,
- pricing for selected vanilla rates instruments,
- early bump-and-revalue risk,
- early stress, P&L explain, and Monte Carlo services,
- Python bindings and a C++ CLI.

## Planned next steps

- harden market conventions and market schema,
- split market construction into typed builders,
- add reusable built instruments and built portfolios,
- strengthen pricing outputs and diagnostics,
- refactor risk around explicit factor definitions,
- separate historical, parametric, and Monte Carlo VaR,
- evolve Monte Carlo from one-step factor simulation toward a fuller framework.

## Repository structure

```text
quant-risk-platform/
├── CMakeLists.txt
├── CMakePresets.json
├── QUICKSTART.md
├── README.md
├── cpp/
│   ├── include/qrp/
│   │   ├── analytics/
│   │   ├── conventions/
│   │   ├── domain/
│   │   ├── instruments/
│   │   ├── io/
│   │   ├── market/
│   │   └── util/
│   ├── bindings/
│   ├── cli/
│   └── src/
├── data/
│   ├── market/
│   ├── portfolios/
│   └── scenarios/
├── docs/
│   ├── design/
│   ├── pricing/
│   │   ├── market-data/
│   │   ├── rates/
│   │   ├── credit/
│   │   └── volatility/
│   ├── risk/
│   ├── roadmap/
│   └── theory/
├── python/
│   ├── examples/
│   └── notebooks/
├── tests/
│   ├── integration/
│   ├── regression/
│   └── unit/
└── vcpkg.json
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
- **Pragmatic** about the workflows desks and risk teams actually use.

## Disclaimer

This repository is a technical demonstration project intended for educational and professional portfolio purposes. It is
not investment advice and should not be used for production trading or risk management without additional validation,
controls, governance, and operational hardening.
