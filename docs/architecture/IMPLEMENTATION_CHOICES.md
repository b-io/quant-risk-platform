# Implementation Choices - Quant Risk Platform

This document explains the **why** behind the core architectural decisions of the platform.

## 1. Rationale For Wrapping QuantLib Behind Platform-Specific Abstractions

**Choice:** We use DTOs (`domain::Trade`, `domain::MarketQuote`) and builders (`InstrumentFactory`, `CurveBuilder`)
rather than exposing QuantLib objects directly to the services.

**Reason:**

- **Decoupling:** QuantLib's interface is complex and highly object-oriented. Wrapping it allows us to expose a simpler,
  data-centric API (JSON/SQL) to users.
- **Persistence:** QuantLib objects are not easily serializable. Our DTOs and database schema provide a stable way to
  store and reload the state of the platform.
- **Validation:** We can perform domain-specific validation before attempting to build complex mathematical objects.

## 2. QuantLib Observer Pattern and Reactive Risk

The platform's analytics core relies heavily on the **QuantLib Observer Pattern** to achieve high-performance risk and
scenario revaluation.

### `Observable`, `Observer`, and `LazyObject`

- **`Observable`**: Any object that can change (e.g., a market quote, a yield curve).
- **`Observer`**: Any object that depends on an `Observable` (e.g., an instrument depends on a curve; a curve depends on
  quotes).
- **`LazyObject`**: A hybrid that is both an `Observer` and an `Observable`. It doesn't recalculate immediately when its
  inputs change; instead, it marks itself as "dirty" (invalidates its cache). Recalculation only happens when someone
  calls a method that requires the value (e.g., `NPV()`).

### `SimpleQuote` and `Handle<T>`

- **`SimpleQuote`**: A mutable container for a single market value (e.g., a 10Y Swap rate). It is an `Observable`.
- **`Handle<T>`**: A smart pointer wrapper that provides an additional layer of indirection.
- **`RelinkableHandle<T>`**: A `Handle` that can be "relinked" to a different underlying object without the observers
  needing to be aware of the switch.

### Invalidation vs. Recalculation

When we update a `SimpleQuote` value:

1. The quote notifies all its observers (e.g., the `PiecewiseYieldCurve` helpers).
2. The curve marks its cached bootstrap as invalid and notifies its observers (e.g., the `Swap` instrument).
3. The instrument marks its NPV as invalid.
4. **No math happens yet.**
5. When `QuantRiskPlatform` calls `instrument->NPV()`, the instrument asks the curve for its state. The curve sees it is
   invalid, re-bootstraps (or just re-interpolates if only values changed), and then the instrument calculates the new
   NPV.

### Pattern Rationale

- **Scenario Reuse**: We can create a `MarketState` once, build all curves and instruments, and then "bump" the quotes
  thousands of times for Taylor-series risk or Monte Carlo.
- **Minimal Rebuilds**: We avoid the overhead of reconstructing C++ objects. We only pay for the necessary mathematical
  recalculations.

### Concrete Platform Example

Consider a USD swap whose discount curve is built from `USD_OIS_1Y`, `USD_OIS_2Y`, `USD_OIS_5Y`, and `USD_OIS_10Y`.
At runtime the dependency chain is:

```text
MarketState["USD_OIS_5Y"] SimpleQuote
  -> OIS rate helper
  -> bootstrapped USD OIS curve
  -> swap pricing engine
  -> swap instrument NPV
```

For a `GLOBAL_RISK_OFF` scenario, the scenario engine resolves the factor shock into concrete quote values and updates
the existing quote handles:

```cpp
market::ScenarioEngine::apply_scenario_to_state(*state, base_market, scenario, factors, bindings);
const double shocked_npv = ValuationService::price_instrument(trade, *cached_instrument);
state->reset_to_snapshot(base_market);
```

The cached swap instrument is not rebuilt. Updating `USD_OIS_5Y` invalidates the observed curve and instrument. The next
`NPV()` call forces QuantLib to refresh the required values, and resetting the state restores the base quote handles for
the next scenario.

The runnable Python demo exposes this sequence through `qrp.create_revaluation_session(...)` and
`RevaluationSession.revalue_scenario(...)`. Run `python/examples/demo_platform.py` and look for the
`Observer Pattern Revaluation` section to see base, shocked, and restored portfolio values, quote handles moved by the
scenario, potentially affected trade ids, and candidate-only before/after diffs.

### Repository-Specific Implementation

- **Quote Creation**: Quotes are created in `MarketSnapshot` and stored as `shared_ptr<SimpleQuote>` in `MarketState`.
- **Handle Storage**: `MarketState` maintains a map of `quote_id` to `SimpleQuote` handles.
- **Scenario Application**: `ScenarioEngine` (and `QuantRiskPlatform::run_historical_var`) applies shocks directly to
  these `SimpleQuote` objects via `setValue()`.
- **Reusable Session API**: `RevaluationSession` owns a built `MarketState` and cached instruments in C++. Python gets
  domain-level methods such as `apply_quote_updates`, `apply_scenario`, `price`, `reset`, and `revalue_scenario`
  without direct access to raw QuantLib handles.
- **Opt-In Impact Diagnostics**: `preview_*_impact(...)` lazily builds a quote-to-trade dependency index only when a
  caller asks for it. `revalue_*_impact(...)` uses that index to price only structurally affected candidate trades and
  returns compact diff rows to Python. The ordinary quote-update and pricing path does not build or traverse this index.
- **Reactive Services**: `ValuationService` and `RiskService` rely on this behavior. When `RiskService` bumps a quote by
  1bp, it simply calls `setValue(val + 0.0001)`, then reads the instrument NPVs. The update propagates automatically.
- **Explicit vs. Automatic**:
    - **Automatic**: NPV and Curve updates are automatic via the Observer pattern.
    - **Explicit**: Market snapshot loading, portfolio building, and the initial curve construction are explicit steps
      orchestrated by the `app` layer. We do not "auto-detect" new files; they must be imported.

## 3. Rationale For SQLite As The Default Storage Backend

**Choice:** SQLite is used as the default implementation of `StorageBackend`.

**Reason:**

- **Simplicity:** Zero-configuration and no external server dependencies. Ideal for local development and standalone CLI
  usage.
- **File-based:** Entire databases can be shared as single files (`var/quant_risk_platform.sqlite`), facilitating
  reproducibility.
- **Standardization:** Using SQL ensures that we can eventually migrate to PostgreSQL or DuckDB by swapping the
  `StorageBackend` implementation without changing analytics logic.

## 4. Rationale For Historical VaR Before Monte Carlo Expansion

**Choice:** The platform prioritizes Historical VaR over a full path-based Monte Carlo framework.

**Reason:**

- **Interpretability:** Historical VaR is easier to explain to stakeholders (e.g., "what happens if 2008 repeats?").
- **Consistency:** It reuses the same scenario application logic as our stress testing and deterministic risk (PV01).
- **Data-driven:** It leverages our persistence layer to store and replay historical shock vectors.

## 5. Rationale For Multi-Asset Ingestion With Explicit Support Status

**Choice:** The database schema, ingestion logic, pricing registry, and portfolio fixtures are multi-asset, while each
product family still carries explicit support status and diagnostics.

**Reason:**

- **Vertical Slice:** Every supported product should flow through the same path: load -> store -> price -> risk ->
  persist.
- **Multi-Asset Shape:** Rates, FX, credit, commodities, and equities can share portfolio loading, market snapshots,
  factor bindings, result storage, and workflow orchestration.
- **Graceful Failure:** Unsupported products are stored and reported with structured diagnostics rather than crashing
  the engine.

## 6. How ad hoc analysis is exposed safely

**Choice:** We use typed ad hoc requests through a service layer rather than raw SQL execution for users.

**Reason:**

- **Consistency:** Ensures that business logic (e.g., how PV01 is aggregated) is not reinvented in ad hoc queries.
- **Security:** Prevents SQL injection and ensures the database remains in a valid state.
- **Abstraction:** Allows us to optimize the underlying queries or change the schema without breaking user scripts.
