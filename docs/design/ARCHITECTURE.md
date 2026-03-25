# Quant Risk Platform Architecture

This document is the **single design source of truth** for the platform architecture. It combines the previous high-level and low-level notes and explains not only **what** the components are, but also **why** the design choices were made.

## 1. Design goals

The target system is a production-shaped quantitative risk engine with the following properties:

- market-consistent valuation,
- reusable market objects and reactive quote handles,
- clear separation between market construction, instruments, and analytics,
- deterministic risk, historical stress, and Monte Carlo,
- scalable revaluation and future parallelization,
- application-level interfaces for CLI and Python.

## 2. High-level architecture

```mermaid
flowchart TD
    A[Market and portfolio JSON DTOs] --> B[Loaders and validation]
    B --> C[Market builders]
    C --> D[MarketState]
    D --> E[PricingContext]
    E --> F[InstrumentFactory]
    F --> G[ValuationService]
    F --> H[RiskService]
    F --> I[PnlExplainService]
    F --> J[StressEngine]
    F --> K[MonteCarloEngine]
    G --> L[CLI and Python bindings]
    H --> L
    I --> L
    J --> L
    K --> L
```

### Why this layering is used

- **DTOs** isolate external file formats from internal objects.
- **Loaders and validation** keep malformed payloads out of the engine.
- **Market builders** convert raw quotes and conventions into QuantLib term structures.
- **MarketState** owns reusable handles and curves.
- **PricingContext** resolves which curves and conventions an instrument should use.
- **InstrumentFactory** translates trades into QuantLib instruments.
- **Analytics services** operate on built market state and built instruments rather than re-parsing JSON.
- **CLI / Python** expose stable application services rather than raw QuantLib internals.

## 3. Core QuantLib design choices

### 3.1 Why use `SimpleQuote` handles

`SimpleQuote` is the right primitive for a revaluation engine because it supports in-place updates of market inputs.

Why this matters:

- a bump to a quote should not require rebuilding the entire engine,
- QuantLib's observer pattern automatically invalidates dependent objects,
- risk, stress, and Monte Carlo can reuse instruments and curves.

This is the correct foundation for PV01, key-rate risk, historical stress, and scenario engines.

### 3.2 Why use `RateHelper` objects for yield curves

A curve should be calibrated to market instruments, not merely interpolated through arbitrary rates.

Using QuantLib `RateHelper` objects gives:

- instrument-consistent bootstrapping,
- transparent calibration logic,
- extension paths to OIS, IBOR, basis, FRA, and swap helpers.

### 3.3 Why use `PiecewiseYieldCurve<Discount, LogLinear>` initially

This is a good first production choice because:

- discount factors stay positive,
- interpolation is stable and widely used,
- deterministic risk analytics are easier to explain and test.

Later, the platform can add spline or parametric fits when there is a real need.

## 4. Current runtime flow

```mermaid
sequenceDiagram
    participant U as User or Python client
    participant L as JSON loader
    participant M as Market builder
    participant S as MarketState
    participant P as PricingContext
    participant F as InstrumentFactory
    participant A as Analytics service

    U->>L: load market and portfolio files
    L->>M: validated DTOs
    M->>S: build curves and quote handles
    S->>P: provide market access
    A->>F: request instrument build
    F->>P: resolve curves and conventions
    A->>S: apply shocks via handles when needed
    A->>U: return structured results
```

## 5. Current code mapping

### Domain layer

Files:

- `cpp/include/qrp/domain/types.hpp`
- `cpp/include/qrp/domain/market_data.hpp`
- `cpp/include/qrp/domain/portfolio.hpp`

Current state:

- basic DTOs exist,
- typed curve identifiers exist,
- schema is still too thin for full convention-aware curve building.

### Market layer

Files:

- `cpp/include/qrp/market/market_state.hpp`
- `cpp/include/qrp/market/market_snapshot.hpp`
- `cpp/src/market/market_snapshot.cpp`
- `cpp/include/qrp/market/scenario_engine.hpp`
- `cpp/src/market/scenario_engine.cpp`

Current state:

- quote handles and bootstrapped yield curves exist,
- scenario application can mutate quote handles in place,
- credit curves, vol surfaces, and richer curve families are missing,
- conventions are still embedded in builder logic rather than registered centrally.

### Pricing context

File:

- `cpp/include/qrp/analytics/pricing_context.hpp`

Current state:

- basic curve lookup exists,
- curve-family mismatch bug appears fixed (`OIS` is used consistently now),
- still needs a proper `MarketConventionRegistry` to avoid hardcoded instrument assumptions.

### Instrument layer

Files:

- `cpp/include/qrp/instruments/instrument_factory.hpp`
- `cpp/src/instruments/instrument_factory.cpp`

Current state:

- vanilla swaps and fixed-rate bonds are implemented,
- calendars, schedules, and day-count conventions are mostly hardcoded,
- there is not yet a reusable built-position cache.

### Analytics layer

Files:

- `valuation_service.cpp`
- `risk_service.cpp`
- `pnl_explain_service.cpp`
- `stress_engine.cpp`
- `monte_carlo_engine.cpp`

Current state:

- valuation works for the current sample instruments,
- risk is still brute-force bump-and-revalue,
- P&L explain is still placeholder-level,
- Monte Carlo is currently a one-step Gaussian scenario engine rather than a true path engine,
- historical stress is still scenario replay over generic shocked quotes.

## 6. Main design gaps to close next

```mermaid
flowchart LR
    A[Thin market schema] --> B[No convention registry]
    B --> C[Hardcoded schedules and day counts]
    C --> D[Fragile multi-curve support]
    D --> E[Prototype risk and explain engines]
```

The main gaps are:

1. **Market conventions are not centralized.**
2. **Curve families are not yet modeled beyond simple yield curves.**
3. **Instrument construction is not driven by conventions.**
4. **The analytics services do not yet share a built-position cache.**
5. **P&L explain, stress, and Monte Carlo need more realistic models.**

## 7. Target architecture evolution

### Phase 1: solid deterministic rates engine

- central `MarketConventionRegistry`,
- richer market schema,
- OIS discount + projection curves,
- reusable built-position cache,
- deterministic PV01 / key-rate / scenario engines,
- production-shaped P&L explain.

### Phase 2: credit and volatility market objects

- hazard / survival curves,
- credit spread factors,
- vol surfaces and smile inputs,
- richer factor mapping and risk attribution.

### Phase 3: scalable simulation architecture

- Monte Carlo path engine,
- historical scenario store,
- cached factor mappings,
- parallel scenario execution.

## 8. Why the current design is still the right base

Even though the implementation is incomplete, the chosen direction is good because:

- QuantLib handles are the right primitive for fast revaluation,
- rate helpers are the right primitive for market-consistent curve construction,
- layering is already service-oriented rather than notebook-oriented,
- the current gaps are mainly missing abstractions and incomplete analytics, not a fundamentally wrong architecture.

## 9. Documentation policy going forward

To keep the design coherent:

- maintain **one canonical architecture document**: `docs/design/ARCHITECTURE.md`,
- use specialized supporting notes only when they add real value,
- every design note must explain both:
  - the chosen design,
  - why that design is preferred over simpler alternatives,
- every important QuantLib choice should be justified in terms of correctness, performance, and extensibility.
