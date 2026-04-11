# Factor-Only Scenario Architecture Review and Refactor Instructions

## 1. Review of the current implementation

The recent change is a useful intermediate step, but it is still **hybrid**, not truly factor-driven.

### What has improved

- `ScenarioDefinition` now contains `factor_shocks`.
- `FactorDefinition` and `FactorBinding` exist.
- `FactorShockResolver::resolve_quote_values(...)` provides the correct conceptual direction: shocks are applied to factors first, then resolved to quote-level values.
- `MonteCarloEngine` can now generate shocks for arbitrary factors instead of assuming currencies only.

### What is still wrong for a real system

The current implementation still does this:

```cpp
// 3. Apply legacy shocks (currency parallel, node, twist, credit)
// Note: In a fully factor-driven system, these would also be factors.
apply_scenario_to_state(state, base_dto, scenario);
```

This means the system still has **two parallel scenario representations**:

1. the new factor representation: `scenario.factor_shocks`
2. the legacy representation: `parallel_shocks`, `node_shocks`, `twist_shocks`, `credit_shocks`

That is not acceptable as a target architecture because:

- it duplicates business logic;
- it creates ambiguity about the source of truth;
- it makes risk numbers dependent on whether a shock was expressed as a factor or a legacy field;
- it invites double-application or inconsistent application of the same economic shock;
- it prevents historical scenario, Monte Carlo, stress, and deterministic risk from sharing one canonical shock model.

## 2. Target principle

The platform should be **factor-only**.

All scenario definitions should be expressed as:

- a set of **factor definitions**,
- a set of **factor bindings**,
- a set of **factor shocks**.

There should be **no production analytics path** that depends on legacy shock fields.

The canonical pipeline should be:

```text
scenario.factor_shocks
    -> factor resolver
    -> quote-level shocked values
    -> state update
    -> curve/instrument revaluation
```

## 3. What must change

### 3.1 Remove legacy shock semantics from the production path

The following fields in `ScenarioDefinition` should be treated as migration-only and then removed:

- `parallel_shocks`
- `node_shocks`
- `twist_shocks`
- `credit_shocks`

The production `ScenarioDefinition` should converge to something like:

```cpp
struct ScenarioDefinition {
    std::string name;
    std::map<std::string, double> factor_shocks; // factor_id -> shock
};
```

Optional metadata can be added separately:

```cpp
struct ScenarioMetadata {
    std::string scenario_id;
    std::string scenario_type;   // MonteCarlo, HistoricalStress, Hypothetical, RiskBump
    std::string horizon;
    std::string provenance;
};
```

### 3.2 Delete the hybrid application path

The binding-aware overload must **not** call the legacy overload afterward.

Current behavior:

```cpp
auto shocked_quotes = FactorShockResolver::resolve_quote_values(...);
for (const auto& [qid, value] : shocked_quotes) {
    state.add_quote(qid, value);
}
apply_scenario_to_state(state, base_dto, scenario); // wrong
```

Correct behavior:

```cpp
auto shocked_quotes = FactorShockResolver::resolve_quote_values(...);
for (const auto& [qid, value] : shocked_quotes) {
    state.add_quote(qid, value);
}
```

Nothing else.

### 3.3 Convert every existing scenario type into factors

Do not keep "legacy scenarios" as a permanent concept. Convert them into factor families.

#### Currency parallel shock

Today:
- `parallel_shocks[ccy] = bump`

Target:
- define factors for the relevant rate curve nodes or FX spot factors;
- apply the same factor shock to all factors in the selected family if a parallel scenario is desired.

Example:
- `RF:RATE:USD:ZERO:1Y`
- `RF:RATE:USD:ZERO:2Y`
- `RF:RATE:USD:ZERO:5Y`
- `RF:RATE:USD:ZERO:10Y`

Parallel USD rate shock becomes:

```text
factor_shocks[
  RF:RATE:USD:ZERO:1Y,
  RF:RATE:USD:ZERO:2Y,
  RF:RATE:USD:ZERO:5Y,
  RF:RATE:USD:ZERO:10Y
] = +1bp
```

#### Node shock

Today:
- `node_shocks[currency][quote_id] = bump`

Target:
- a node is already a factor.

Example:

```text
factor_id = RF:RATE:USD:ZERO:5Y
```

Then:

```text
factor_shocks[RF:RATE:USD:ZERO:5Y] = +1bp
```

#### Twist shock

Today:
- represented procedurally as `slope * (tenor - pivot)`

Target:
- expand the twist into factor shocks before application.

For each rate-node factor with maturity `T_i`, define:

```text
shock_i = slope * (T_i - T_pivot)
```

and populate `scenario.factor_shocks[factor_i] = shock_i`.

The scenario engine should apply factor shocks, not re-implement twist mathematics internally.

#### Credit shock

Today:
- `credit_shocks[quote_id] = bump`

Target:
- use credit spread factors directly.

Examples:
- `RF:CREDIT:ACME:CDS:1Y`
- `RF:CREDIT:ACME:CDS:3Y`
- `RF:CREDIT:ACME:CDS:5Y`

## 4. Refactor instructions by component

### 4.1 `ScenarioDefinition`

#### Required change

Collapse it to factor-only shock storage.

#### Remove from the public API

- `parallel_shocks`
- `node_shocks`
- `twist_shocks`
- `credit_shocks`

#### Keep

- `name`
- `factor_shocks`

If backward compatibility is temporarily required, put legacy fields behind a migration adapter, not in the core model.

### 4.2 `ScenarioEngine`

#### Required change

Keep only the factor-driven implementation as the production entry point:

```cpp
static void apply_scenario_to_state(
    MarketState& state,
    const domain::MarketSnapshot& base_dto,
    const ScenarioDefinition& scenario,
    const std::vector<domain::FactorDefinition>& factors,
    const std::vector<domain::FactorBinding>& bindings);
```

#### Remove or deprecate

The legacy overload:

```cpp
static void apply_scenario_to_state(
    MarketState& state,
    const domain::MarketSnapshot& base_dto,
    const ScenarioDefinition& scenario);
```

If retained temporarily, it should be a migration shim in a separate compatibility module, not part of the main engine.

### 4.3 `RiskService`

`RiskService` still builds shocks using old semantics.

#### Current behavior

- PV01 uses `parallel_shocks`
- key-rate DV01 uses `node_shocks`
- CS01 uses `credit_shocks`

#### Target behavior

All risk should be expressed through factor shocks.

##### PV01 / parallel rate bump

- select all rate factors relevant to the trade or currency;
- assign the same +1bp shock to each selected factor;
- run the factor-only scenario engine.

##### Key-rate DV01

- select one rate factor at a time;
- assign +1bp to that factor only;
- store result by `factor_id`;
- derive display labels from factor metadata.

##### CS01

- select all credit spread factors mapped to the issuer/reference entity;
- apply the spread bump at factor level.

#### Important rule

Risk bucketing should be keyed by **factor identifiers**, not raw quote IDs or tenor strings alone.

### 4.4 `StressEngine`

Historical and hypothetical stress scenarios must be stored and replayed as **factor vectors**, not legacy shock structures.

#### Historical stress

- retrieve historical factor moves from `factor_observations`;
- build a scenario as `factor_id -> move`;
- resolve via bindings;
- reprice.

#### Hypothetical stress

- specify factor families and shock magnitudes;
- expand them into a factor vector before valuation.

### 4.5 `MonteCarloEngine`

The engine is already closer to the right design.

#### Keep

- factor-based shock generation
- covariance-driven simulation
- factor IDs in `scenario.factor_shocks`

#### Change

- remove the fallback to legacy application once factor bindings are expected in production;
- require the factor universe and bindings for production runs;
- do not allow analytics to silently drop back to the legacy path.

Recommended behavior:

```cpp
if (bindings.empty()) {
    throw std::runtime_error("MonteCarloEngine requires factor bindings in production mode");
}
```

If demo mode is needed, make it explicit:

```cpp
run_simulation_demo(...)
run_simulation_production(...)
```

Do not mix them.

### 4.6 Python bindings

The Python bindings still expose legacy shock fields.

Remove from the exposed API:

- `parallel_shocks`
- `node_shocks`
- `credit_shocks`
- `twist_shocks`

Expose only:

- `name`
- `factor_shocks`

If migration support is required, put legacy constructors in a separate compatibility namespace.

## 5. Data model requirements

To make factor-only scenarios realistic, the data model must treat factors as first-class objects.

### 5.1 Factor definitions

Each factor should carry enough metadata to support pricing, risk, and historical reconstruction.

Minimum fields:

- `factor_id`
- `factor_type`
- `shock_measure`
- `currency`
- `curve_id`
- `tenor`
- `description`

### 5.2 Factor bindings

Each factor must map to one or more quotes with explicit shock semantics.

Minimum fields per binding:

- `factor_id`
- `quote_id`
- `binding_weight`
- `shock_measure`
- `transformation`
- `is_active`
- `effective_from`
- `effective_to`

Examples of transformations:

- Absolute shift
- Relative shift
- Log-return shift
- Basis-point shift
- Vol-point shift

### 5.3 Historical factor observations

Historical covariance, stress, and historical simulation must be based on **factor observations**, not ad hoc quote snapshots.

The database should support:

- factor level at date/time
- factor move over horizon
- provenance of the move construction
- data quality flags

## 6. How to express previous legacy shocks as factors

### Parallel curve bump

Instead of:

```cpp
scenario.parallel_shocks["USD"] = 0.0001;
```

Use:

```cpp
for (const auto& factor : usd_zero_curve_factors) {
    scenario.factor_shocks[factor.factor_id] = 0.0001;
}
```

### Single node bump

Instead of:

```cpp
scenario.node_shocks["USD"]["USD_OIS_5Y"] = 0.0001;
```

Use:

```cpp
scenario.factor_shocks["RF:RATE:USD:ZERO:5Y"] = 0.0001;
```

### Twist

Instead of storing a twist object in the scenario, compile it into factor shocks:

```cpp
for (const auto& factor : usd_zero_curve_factors) {
    double T = tenor_to_years(factor.tenor);
    scenario.factor_shocks[factor.factor_id] = slope * (T - pivot);
}
```

### Credit spread bump

Instead of:

```cpp
scenario.credit_shocks["ACME_CDS_5Y"] = 0.0001;
```

Use:

```cpp
scenario.factor_shocks["RF:CREDIT:ACME:CDS:5Y"] = 0.0001;
```

## 7. Why this matters

A factor-only architecture is the only way to make the following analytics consistent:

- PV01 / DV01
- key-rate DV01
- CS01
- historical VaR
- historical stress
- hypothetical stress
- Monte Carlo VaR
- factor attribution
- P&L explain by factor family

If some analytics use legacy curve shocks and others use factor shocks, the system will eventually produce irreconcilable results.

## 8. Immediate next implementation tasks

### Priority 1

1. Remove the legacy call from `ScenarioEngine::apply_scenario_to_state(..., factors, bindings)`.
2. Mark the legacy overload as deprecated.
3. Stop using legacy shock fields in new code.

### Priority 2

4. Refactor `RiskService` to generate factor shocks only.
5. Refactor `StressEngine` to replay factor vectors only.
6. Make `MonteCarloEngine` require bindings in production mode.

### Priority 3

7. Remove legacy fields from Python bindings.
8. Add migration helpers that translate old scenario JSON into factor-only scenarios at load time.
9. Add tests ensuring identical results between old scenarios and their factorized equivalents during migration.

## 9. Acceptance criteria

The refactor is complete only when all of the following are true:

- there is one canonical scenario representation: `factor_id -> shock`;
- all analytics use the same factor-resolution path;
- no production analytics code path calls legacy shock application;
- historical stress, Monte Carlo, and bump-and-revalue all operate on the same factor universe;
- factor bindings are stored and versioned;
- scenario replay is reproducible from factor shocks plus market snapshot.

## 10. Required code review message

Use this as the direct instruction to the coding assistant:

> The recent change is still hybrid, not factor-only. Please remove the legacy post-processing in `ScenarioEngine::apply_scenario_to_state(..., factors, bindings)` and convert every scenario representation to `factor_id -> shock`. Parallel, node, twist, and credit shocks must become compiled factor vectors, not separate runtime semantics. `RiskService`, `StressEngine`, Monte Carlo, Python bindings, and scenario JSON loading should all use the same canonical factor-only path. Do not preserve legacy fields in the production model except behind an explicit migration adapter.
