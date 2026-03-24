# Quant Risk Platform

Production-style C++ and Python platform for pricing, risk analytics, scenario analysis, and P&L across asset classes.

> Initial implementation focuses on rates, with an architecture designed to extend to credit, FX, equities, and commodities.

## Quickstart

For instructions on how to build the C++ core and run the Python demo, please refer to the [QUICKSTART.md](QUICKSTART.md) guide.

## Overview

Quant Risk Platform is a compact, production-style analytics platform designed to demonstrate how front-office pricing, risk, and P&L workflows can be implemented with a scalable architecture.

The project combines a high-performance C++ core with a Python-facing API, with a focus on:
- pricing and risk analytics for trading portfolios
- market data and curve construction
- scenario analysis and stress testing
- P&L explain and risk decomposition
- extensibility across asset classes
- clean engineering, testability, and operational robustness

The initial scope targets **rates**, which is the most direct and credible starting point for a front-office quant/risk platform. The architecture is deliberately designed so that additional asset classes can be integrated without major restructuring.

## Goals

- Build a **production-style** quant platform rather than a research notebook or toy library
- Demonstrate strong **C++ / Python** integration
- Provide a clear separation between:
  - market data
  - instrument models
  - pricing and risk engines
  - scenario and P&L logic
  - application/API layers
- Support **scalable portfolio analytics** for front-office workflows
- Keep the implementation compact, testable, and interview-ready

## Initial Scope

### Asset class
- Rates

### Supported analytics in the first version
- Curve construction and calibration
- Pricing for selected rates instruments
- Sensitivities such as DV01 / PV01 and bucketed risk
- Scenario analysis and stress testing
- P&L explain from market moves
- Portfolio-level aggregation

### Planned extensions
- Credit products and credit-spread risk
- FX and cross-currency analytics
- Equity and commodity derivatives
- Multi-threaded scenario engines
- Caching and incremental recomputation
- Service or API layer for interactive consumption

## Architecture

The platform is designed around modular components.

### Core layers

**1. Market data layer**
- quotes, fixings, reference data
- yield curves and term structures
- scenario shocks and market snapshots

**2. Instrument layer**
- common trade representation
- instrument-specific pricing setup
- portfolio composition and aggregation

**3. Analytics layer**
- pricing
- sensitivities
- scenario analysis
- stress testing
- P&L explain

**4. Platform layer**
- configuration
- logging and error handling
- performance measurement
- serialization / input-output helpers

**5. Interface layer**
- Python bindings for interactive workflows
- CLI examples for reproducible runs
- notebooks or scripts for demos

## Technology Stack

- **C++17/20** for the core analytics engine
- **CMake** for reproducible builds
- **fmt** for formatting (included via QuantLib or standalone)
- **GoogleTest** or Catch2 for unit tests
- **nlohmann-json** for JSON parsing
- **pybind11** for Python bindings
- **Python** for orchestration, demos, and exploratory analysis
- **QuantLib** for financial instruments, curves, and pricing building blocks

## Repository Structure

```text
quant-risk-platform/
├── CMakeLists.txt
├── CMakePresets.json
├── cpp/
│   ├── include/qrp/
│   │   ├── domain/       # Data transfer objects
│   │   ├── market/       # Curve and scenario logic
│   │   ├── instruments/  # Factory and QL wrappers
│   │   ├── analytics/    # Valuation, Risk, Pnl Services
│   │   ├── io/           # JSON loaders
│   ├── bindings/         # pybind11 module
│   └── cli/              # C++ executable
├── python/
│   ├── examples/         # Demo scripts
├── data/                 # Sample market and portfolio JSONs
├── tests/                # Unit and Integration tests
└── benchmarks/           # Performance tests
```

## Example Use Cases

- Build a rates curve from market quotes
- Price a portfolio of vanilla rates instruments
- Compute PV01 / DV01 and bucketed curve risk
- Apply curve shocks and generate stressed P&L
- Compare base and shocked market states
- Expose the same analytics through Python for desk-facing workflows

## Design Principles

- **Production-first:** code should look like a maintainable platform, not a notebook export
- **Extensible:** new asset classes should plug into a stable architecture
- **Transparent:** analytics should be explainable and easy to inspect
- **Testable:** all core components should be covered by deterministic tests
- **Scalable:** portfolio analytics should support batching, parallelization, and future distribution
- **Pragmatic:** focus on workflows traders, PMs, and risk teams actually use

## Scalability Considerations

Scalability is a first-class design concern in this project.

The platform should be structured to support:
- large portfolios with many instruments
- repeated scenario runs over the same portfolio
- efficient recomputation when only a subset of market inputs changes
- parallel execution of pricing and scenario tasks
- separation between immutable market states and analytics requests
- future migration toward distributed risk runs if needed

Initial implementations may remain single-node, but interfaces and data flow should avoid assumptions that would block later scaling.

## Current Status

This repository is under active development.

The first milestone is:
- project skeleton
- build system
- minimal C++ core
- Python bindings
- sample rates workflow
- tests and benchmarks

## Why this project

This project is intended to demonstrate the combination of skills typically required in a front-office quant engineering role:
- quantitative modeling awareness
- strong software engineering fundamentals
- pricing and risk analytics
- performance-oriented implementation
- practical delivery to front-office users

## Short Repository Description

Use this as the GitHub repository description:

**Production-style C++ and Python platform for pricing, risk analytics, scenario analysis, and P&L across asset classes.**

## Optional Topics

Suggested GitHub topics:
- cplusplus
- front-office
- portfolio-analytics
- pricing
- pnl
- pybind11
- python
- quant
- quantlib
- rates
- risk-analytics
- trading

## Roadmap

### Milestone 1
- Repository skeleton
- CMake + presets
- Minimal pybind11 module
- Curve construction prototype
- One pricing example
- Unit tests

### Milestone 2
- Portfolio representation
- Risk measures (PV01 / DV01 / bucketed risk)
- Scenario engine
- P&L explain
- Benchmarks

### Milestone 3
- Improved performance and caching
- Parallel execution
- Additional instruments
- Better reporting and examples
- Extension path toward credit / FX / equities / commodities

## Disclaimer

This repository is a technical demonstration project intended for educational and professional portfolio purposes. It is not investment advice and is not intended for production trading use without additional validation, controls, and governance.
