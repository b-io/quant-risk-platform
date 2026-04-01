# Data Storage, Historical Snapshots, Lineage, and Reconciliation

## Why this chapter matters

Quantitative analytics are only credible if results can be reproduced and reconciled. A platform that stores only final
P&L or risk numbers is not operationally complete. It must preserve the data model, historical market state, model
version, and run metadata required to rerun and explain past results. This chapter explains the data-storage and control
principles that support that goal.

## 1. Canonical data domains

A practical platform separates several distinct data domains.

### 1.1 Trades and positions

Trade data describes contractual objects:

- product type,
- identifiers,
- notionals,
- directions,
- coupon or strike fields,
- schedule information,
- counterparty or legal tags where relevant,
- lifecycle status.

Positions are derived holdings views that may aggregate multiple trades for risk or reporting. Both layers matter.

### 1.2 Market data

Market data includes:

- spot prices,
- rates and curve instruments,
- credit spreads and CDS quotes,
- volatility surface quotes,
- fixings,
- benchmark indices,
- corporate actions or reference events where relevant.

Market data should be stored with source, timestamp, quality status, and transformation lineage.

### 1.3 Reference data

Reference data covers the metadata required to interpret trades and quotes correctly:

- calendars,
- day-count conventions,
- business-day rules,
- benchmark definitions,
- issuer hierarchies,
- product templates,
- factor taxonomies.

Reference-data errors are particularly dangerous because they often create silent valuation defects.

## 2. Curve and calibration layer as first-class stored objects

A historical run should not rely on reconstructing curve state from memory or inference alone. The platform should store
or be able to reconstruct from immutable inputs:

- the raw quotes used for the build,
- the conventions applied,
- the calibration method and parameters,
- the resulting curve or surface nodes,
- diagnostics such as repricing residuals.

This applies to rates curves, credit curves, and volatility surfaces alike.

## 3. Pricing, risk, and scenario engines

The analytics layer uses a common sequence:
$$
\text{Trades and positions} \rightarrow \text{Market and reference data} \rightarrow \text{Calibrated market state} \rightarrow \text{Pricing and risk engines} \rightarrow \text{Stored outputs}
$$
To make this auditable, each run should record:

- valuation timestamp,
- input snapshot identifiers,
- model version or pricing-engine version,
- scenario definition identifiers,
- result status and diagnostics.

## 4. Storage-layer design

### 4.1 Relational trade and reference data

Canonical trade and reference data belong naturally in relational structures because consistency and referential integrity
matter more than read convenience. Relational storage is usually best for:

- normalized trade stores,
- product-reference tables,
- issuer and hierarchy mappings,
- factor-taxonomy registries,
- audit logs of controlled changes.

### 4.2 Time-series and snapshot stores

Time-indexed data stores are better suited for:

- historical market quotes,
- fixing histories,
- stored curve nodes over time,
- monitoring metrics,
- scenario result histories.

The exact technology can vary, but the logical separation is valuable even when one physical database stores multiple
workloads.

### 4.3 Joins versus denormalized analytical views

Canonical source-of-truth tables should remain normalized where practical. However, risk and explain reporting often need
fast denormalized views. The standard pattern is therefore:

- normalized canonical data for control and maintenance,
- denormalized materializations for read-heavy analytics.

This avoids duplicating business rules in every report while still supporting performance.

## 5. Historical snapshots and lineage

### 5.1 Snapshot principle

To reproduce a historical result, the platform needs the exact versions of:

- trade population,
- market data,
- reference data,
- calibrated curves and surfaces or the immutable inputs needed to rebuild them,
- scenario definitions,
- model version and configuration.

### 5.2 Lineage principle

Lineage answers four questions:

1. which inputs produced this result,
2. where did those inputs come from,
3. which transformation and model versions were used,
4. who or what changed them.

A result without lineage is difficult to defend in control, support, or audit settings.

## 6. Schema evolution

Production schemas evolve. New products, conventions, and reporting requirements introduce new fields and relationships.
Good practice includes:

- explicit schema versioning,
- backward-compatible readers where feasible,
- migration scripts,
- documented deprecation paths,
- validation of old snapshots under new code.

Schema evolution should be treated as part of platform design, not as a later operational inconvenience.

## 7. Reconciliation as a control discipline

Reconciliation ensures that supposedly equivalent objects across systems really do agree. Important reconciliation types
include:

- position reconciliation between trade store and downstream portfolio views,
- market-data reconciliation between vendor feed and normalized snapshot,
- valuation reconciliation between front-office and risk views,
- P&L reconciliation between explain reports and official control reports,
- risk reconciliation across intraday and end-of-day engines.

Reconciliation requires stable identifiers. If the same trade, curve, or factor has different identities across systems,
control breaks down quickly.

## 8. APIs, monitoring, and controls

### 8.1 API layer

The API layer should expose stable access to:

- trade and portfolio retrieval,
- market snapshot retrieval,
- valuation and risk requests,
- scenario execution,
- historical result queries.

Stable API contracts matter because front-end tools, notebooks, batch jobs, and support tooling all depend on them.

### 8.2 Monitoring and controls

A production platform should monitor at least:

- missing or stale market data,
- failed curve calibrations,
- anomalous risk jumps,
- valuation failures by product or book,
- broken lineage links,
- persistence failures or partial writes.

Controls should fail loudly when reproducibility is compromised.

## 9. Relationship to the codebase design

The repository already expresses this architecture in `MarketState`, pricing services, persistence interfaces, and local
storage. This chapter provides the conceptual rationale for why those layers must exist separately.

- `docs/design/ARCHITECTURE.md` explains the end-to-end engine layering.
- `docs/design/MARKET_AND_CURVES.md` explains normalized market-state construction.
- `docs/design/PYTHON_CPP_PERFORMANCE_AND_BINDINGS.md` explains how API design and performance engineering interact.
- `docs/risk/PNL_EXPLAIN_IN_PRACTICE.md` explains why snapshots and lineage are required for reproducible explain.
