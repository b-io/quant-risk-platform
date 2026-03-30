# Platform Design and Implementation Guide 

## 1. Target system overview

A practical architecture summary is:

- a **C++ analytics core** for performance-critical pricing and risk,
- a **Python interface layer** for orchestration, scripting, and user APIs,
- a **typed market-state layer** for curves and quotes,
- **reusable services** for valuation, risk, explain, stress, and simulation,
- **persistent storage** for snapshots, portfolios, scenarios, and run outputs,
- common logic exposed via **GUI, Excel API, and Python API**.

This reflects the needs of a production quantitative analytics platform better than describing isolated quant models.

---

## 2. Architecture to present

### 2.1 High-level pipeline

$$
\text{Data ingestion}
\rightarrow
\text{validation and normalization}
\rightarrow
\text{market builders}
\rightarrow
\text{MarketState}
\rightarrow
\text{InstrumentFactory}
\rightarrow
\text{PricingContext}
\rightarrow
\text{Valuation / Risk / Explain / Stress / MC services}
\rightarrow
\text{Persistence + APIs}
$$

### 2.2 Why this split is good

It separates:
- raw data from built market objects,
- product schema from pricing engines,
- analytics logic from user interfaces,
- business workflows from quant internals.

This improves:
- testability,
- performance,
- traceability,
- future extension to credit and macro strategies.

---

## 3. Market data design

## 3.1 Raw market data is not enough

Raw inputs often come in inconsistent forms:
- vendor field names,
- curve points without conventions,
- desk overrides,
- stale timestamps,
- missing tenors,
- multiple sources.

A production system needs a normalized schema.

### 3.2 Recommended quote schema

Each quote should carry enough information to build the correct market object:

- asset class,
- family,
- currency,
- curve or surface family ID,
- tenor / expiry / strike / delta where relevant,
- quote type,
- source,
- valuation timestamp,
- convention metadata,
- quality / status flags.

For credit specifically add:
- reference entity,
- seniority,
- restructuring clause if relevant,
- recovery assumption,
- spread family identifier.

### 3.3 Why this matters

Without typed metadata:
- bootstrapping becomes fragile,
- factor mapping becomes ambiguous,
- P&L explain becomes weak,
- risk aggregation becomes inconsistent.

---

## 4. Convention registry

A convention registry should own:
- calendars,
- day counts,
- business-day conventions,
- settlement lags,
- floating index definitions,
- fixed-leg conventions,
- CDS coupon / schedule conventions,
- volatility quote conventions.

Why?

Because conventions are shared infrastructure, not local product trivia.

> Calendars, day counts, and fixing rules should not be spread throughout pricing code. A stable convention layer ensures that pricing, risk, explain, and data validation all use the same assumptions.

---

## 5. Market builders

## 5.1 Separate builders by object type

Recommended builders:
- `RatesCurveBuilder`
- `ProjectionCurveBuilder`
- `CreditCurveBuilder`
- `VolSurfaceBuilder`
- `FxMarketBuilder`

This is better than one giant monolithic builder.

## 5.2 Builder outputs

Each builder should output:
- built QuantLib or internal market object,
- IDs and labels,
- diagnostics,
- dependencies,
- reusable handles.

#### Example for 5.2 — diagnostics surfaced to front office and support
- calibration residuals,
- missing required nodes,
- extrapolation warnings,
- failed quote normalization,
- unsupported quote conventions.

---

## 6. Market state

## 6.1 What it should contain

A `MarketState` should contain:
- valuation date,
- normalized quotes,
- built curves / surfaces,
- quote handles,
- factor IDs,
- provenance and diagnostics.

## 6.2 Why it is central

Many trades depend on the same market objects.

Instead of rebuilding market inputs trade by trade, build the market state once and reuse it.

This supports:
- valuation,
- bucketed risk,
- scenario stress,
- historical replay,
- P&L explain,
- Monte Carlo.

---

## 7. Reactive risk and scenario design

This is one of the most important design points because the current implementation direction already fits it.

## 7.1 Observer-based revaluation intuition

Dependency chain:

$$
\text{SimpleQuote}
\rightarrow
\text{Curve}
\rightarrow
\text{Instrument}
\rightarrow
\text{NPV}
$$

Where:
- $PV$ is the present value of the cash flow or instrument.

When a quote changes:
- dependent objects are marked dirty,
- recalculation happens lazily when NPV or risk is requested.

That gives:
- fast bump-and-revalue,
- good scenario performance,
- clean reuse of market and instrument objects.

## 7.2 Why this is better than brute-force rebuilds

Brute-force rebuild means:
- reload DTOs,
- rebuild curves,
- rebuild instruments,
- rewire pricing engines,
- then revalue.

This is simpler conceptually but too expensive at scale.

A stronger production design is:
- build once,
- shock quote handles,
- reprice.

---

## 8. Factor taxonomy

A large platform needs explicit factors, not only ad hoc bumps.

Example factor IDs:

- `RF:RATES:USD:SOFR:2Y`
- `RF:RATES:CHF:OIS:10Y`
- `RF:CREDIT:ACME_SNR:CDS:5Y`
- `RF:FX:EURUSD:SPOT:ALL`
- `RF:IRVOL:USD:SWAPTION:5Yx10Y:ATM`

A good factor model enables:
- deterministic risk,
- historical stress,
- P&L explain,
- historical VaR,
- Monte Carlo,
- consistent aggregation.

Without factor IDs, the platform can produce numbers but cannot explain them well.

---

## 9. Instrument factory and pricing context

## 9.1 Instrument factory

Trade DTOs should not be priced directly by services.

Instead:
- validate,
- map to canonical product types,
- build reusable instrument representation,
- attach tags and diagnostics.

## 9.2 Pricing context

A pricing context resolves the market dependencies for each trade:
- discount curve,
- forecast curve,
- credit curve,
- vol surface,
- fixings,
- FX conversion context.

That lets product code stay concise.

---

## 10. Analytics services

Recommended services:

- `ValuationService`
- `RiskService`
- `PnlExplainService`
- `StressEngine`
- `HistoricalVarEngine`
- `MonteCarloEngine`

Each service should:
- consume built market state and built instruments,
- return structured results,
- avoid reparsing inputs,
- expose diagnostics.

### Good result schema

Every output should carry:
- run ID,
- valuation date,
- trade ID / portfolio ID,
- factor ID if relevant,
- measure name,
- currency,
- book / strategy / desk tags,
- provenance.

That makes downstream reporting easy.

---

## 11. Persistence and auditability

A platform like this should persist:
- market snapshots,
- portfolios,
- scenarios,
- pricing runs,
- risk runs,
- explain runs,
- reports,
- diagnostics.

Why this matters:
- reproducibility,
- auditability,
- run comparison,
- historical replay,
- troubleshooting.

Why SQLite or SQL at all:

> A persistent run history and stable serialization layer make the platform a reproducible analytics system rather than only a pricing library. The backend can evolve, but the analytics services should stay independent of the storage implementation.

---

## 12. GUI, Excel API, and Python API

The key principle is:

$$
\text{one analytics core}
\rightarrow
\text{multiple access channels}
$$

Do not implement different pricing logic for each interface.

### GUI
Use for:
- portfolio drill-down,
- run comparison,
- monitoring,
- scenario dashboards.

### Excel API
Use for:
- desk consumption,
- rapid user checks,
- ad hoc analysis,
- controlled UDFs or service calls.

### Python API
Use for:
- research workflows,
- scripting,
- batch analysis,
- notebooks,
- integration with broader quant processes.

A sensible prioritization is:
- common service contracts first,
- then user interfaces on top.

---

## 13. Performance engineering

For this role, be ready to discuss performance beyond generic slogans.

### 13.1 Bottlenecks
Typical bottlenecks:
- repeated object construction,
- repeated curve rebuilds,
- excessive copying,
- Python in hot loops,
- unbatched scenario runs,
- poor memory locality,
- over-serialization,
- unnecessary cross-language calls.

### 13.2 Performance levers
Good levers:
- build once, reuse many times,
- keep hot loops in C++,
- batch scenario application,
- cache built instruments,
- avoid repeated lookup and conversion costs,
- profile before optimizing,
- separate latency-sensitive from throughput-sensitive workloads.

### 13.3 Python vs. C++ in this platform
A strong answer:

> Python is best used where flexibility and iteration speed matter, but not in the hot path. Pricing, scenario revaluation, bucketed risk, and Monte Carlo should stay in C++. Python is ideal for orchestration, notebooks, service wrappers, and external user access, but repeated Python-to-C++ crossings inside inner loops should be minimized.

---

## 14. Testing strategy

A strong quant platform needs more than unit tests.

### 14.1 Test layers
- unit tests for helpers and builders,
- integration tests for portfolio workflows,
- regression tests for stable valuation and risk outputs,
- scenario tests,
- explain consistency tests,
- performance tests for critical workloads.

### 14.2 What to regression-test
- curve calibration outputs,
- trade valuations,
- PV01 / CS01 buckets,
- scenario P&L,
- explain decomposition,
- persistence round-trips.

### 14.3 Why tests matter in quant systems
Because tiny convention changes can materially change:
- NPV,
- sensitivities,
- explain,
- trader trust.

---

## 15. How to extend the current system

You already have the right skeleton. The most natural extension path is:

### 15.1 Strengthen rates production quality
- explicit discount vs. projection curves,
- richer factor IDs,
- robust explain waterfall,
- missing fixings controls,
- curve diagnostics.

### 15.2 Add credit market objects
- typed CDS quotes,
- issuer and seniority mapping,
- recovery storage,
- credit curve builder,
- survival/hazard outputs,
- CS01 results.

### 15.3 Add explain and historical replay
- versioned market snapshots,
- versioned portfolio states,
- historical factor returns,
- scenario libraries,
- run comparison.

### 15.4 Improve end-user access
- richer Python bindings,
- simple Excel service wrapper,
- dashboard/GUI backed by persisted runs.

This is a strong extension path for the platform.

---

## 16. A strong closing statement for architecture questions

> The platform should be treated as a reusable market-state and analytics system, not just a pricing library. The difficult part is not only pricing a trade once; it is building market-consistent curves and surfaces, reusing them efficiently across portfolios, exposing factor-based risk and explain, persisting runs for reproducibility, and making the same analytics available consistently through Python, Excel, and UI channels.
