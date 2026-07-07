# Quant Risk Platform

![Coverage](coverage/coverage-badge.svg)

Production-shaped C++ and Python platform for pricing, risk analytics, scenario analysis, and P&L across asset
classes, with QuantLib at the core and Python used as an interface layer.

> Current implementation covers rates, FX, credit, commodity, and equity products, with valuation, risk,
> stress/HVaR, persistence, and PnL explain workflows wired through the C++ core, CLI, Python bindings, and SQLite
> storage.

## Quickstart

For build, test, and demo instructions, including Python binding workflows and smoke tests,
see [QUICKSTART.md](QUICKSTART.md).

## CI and Coverage

The GitHub Actions workflow in `.github/workflows/tests.yml` runs C++ tests, Python tests, C++ gcovr coverage, Python
coverage, and uploads the generated reports as workflow artifacts. On pushes to `main` or `master`, it also refreshes
the committed lightweight coverage badge and summary files under `coverage/`.

For the auto-commit step, enable repository write tokens in GitHub:

- `Settings > Actions > General > Workflow permissions`
- Select `Read and write permissions`
- Keep pull-request workflows restricted unless you intentionally need write access from forks

The local equivalent for the full coverage gate is:

```powershell
.\scripts\test.ps1 -Coverage
```

The script uses 95% minimum line coverage thresholds for C++ and Python by default. It prints C++ coverage at the end of
the C++ section, Python coverage at the end of the Python section, and a final combined summary.

### Supported builds on Windows (summary)

- Daily development: `dev` for C++ plus Python with a static `qrp_core`.
- Shared-core validation: `dev-shared` for C++ plus Python with a shared `qrp_core`.
- Production-style checks: `release` and `release-shared`.
- Native C++ debugging: `debug` and `debug-shared`.

Suggested commands:

- Daily C++ plus Python:
  `cmake --preset dev && cmake --build build/dev --target quant_risk_platform && ctest --test-dir build/dev -R python_import`
- Production-style C++ plus Python:
  `cmake --preset release && cmake --build build/release --target quant_risk_platform && ctest --test-dir build/release -R python_import`

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

Current implementation maturity is best described as a **multi-asset analytics platform**: rates, FX, credit,
commodities, and equities are represented in the canonical trade model and pricing registry, scenario/risk workflows
are persisted, and PnL explain is exposed through the service layer, CLI, Python bindings, and SQLite result tables.

Known boundaries are VaR and Expected Shortfall contribution analytics, reusable LSMC exercise-policy integration,
broader realized-event sources for explain, built-portfolio caching, and production-control hardening.

## Implemented today

The current codebase already includes:

- market loading and JSON DTOs,
- normalized market, portfolio, scenario, valuation, risk, VaR, and PnL explain result models,
- convention-aware rates market construction and QuantLib helper-based curve bootstrapping,
- quote/factor metadata for rates, FX, credit, commodity, equity, and volatility inputs,
- pricing for supported rates, FX, credit, commodity, and equity products,
- high-performance reactive risk using QuantLib handles and the Observer pattern,
- historical stress and HVaR scenario workflows,
- SQLite-backed persistence for portfolios, market snapshots, scenarios, and analytics results,
- end-to-end persisted valuation, risk, HVaR, and PnL explain workflows,
- PnL explain with carry, realized cash, sequential factor market move, residual reconciliation, and persisted
  components,
- Python bindings and a C++ CLI.

## Known boundaries

- VaR and Expected Shortfall contribution analytics are not yet first-class outputs.
- LSMC exists conceptually in the modeling docs, but is not yet exposed as a reusable exercise-policy engine for every
  early-exercise or physical-flexibility product.
- Realized cash/event-source integration currently covers deposit maturity support and does not yet cover every coupon,
  fixing, exercise, and settlement event source.
- Built-position and built-portfolio caching are not yet shared across repeated analytics.
- Production controls, run manifests, benchmark governance, performance gates, and validation reports remain hardening
  areas.

## Documentation layout

```text
docs/
|-- architecture/    # platform layering, market objects, services, persistence, and bindings
|-- market-data/     # normalized quotes, snapshots, curves, surfaces, provenance, and validation
|-- asset-classes/   # rates, credit, FX, commodities, and equity
|-- models/          # reusable model families such as volatility and exercise models
|-- risk/            # risk factors, PnL explain, stress, VaR, ES, Monte Carlo, and attribution
|-- foundations/     # probability, statistics, stochastic processes, and asset pricing
|-- reference/       # shared lexis, formulas, and public sources
`-- implementation/  # implementation notes, standards, and checkpoints
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
|   |-- benchmarks/
|   |-- bindings/
|   |-- cli/
|   `-- src/
|-- data/
|   |-- market/
|   |-- portfolios/
|   `-- scenarios/
|-- docs/
|   |-- architecture/
|   |-- market-data/
|   |-- asset-classes/
|   |-- models/
|   |-- risk/
|   |-- foundations/
|   |-- reference/
|   `-- implementation/
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
- price portfolios spanning rates, FX, credit, commodities, and equities,
- compute deterministic sensitivities such as PV01, FX delta, CS01, and option Greeks where supported,
- apply market shocks, replay historical scenarios, and generate stressed P&L,
- run persisted valuation, risk, HVaR, and PnL explain workflows,
- compare stored runs and generate run reports,
- drive the same workflows through the C++ CLI or Python bindings for demos and orchestration.

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
- **Scalable** toward larger portfolios, repeated scenarios, and parallel execution,
- **Pragmatic** about valuation, risk, and reporting workflows.

## Disclaimer

This repository is a technical demonstration project intended for educational and professional portfolio purposes. It is
not investment advice and should not be used for production trading or risk management without additional validation,
controls, governance, and operational hardening.
