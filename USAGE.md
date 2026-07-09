# Usage

This page is for people who want to run Quant Risk Platform, use the Python bindings, run the demo workflows, or inspect
persisted results.

## Scope

Quant Risk Platform is a local analytics library and demo platform. It is useful for:

- loading market, portfolio, and scenario JSON files;
- pricing supported multi-asset portfolios;
- computing deterministic risk and stressed P&L;
- running historical VaR and Expected Shortfall with contribution reports;
- explaining P&L between market snapshots;
- persisting analysis runs in SQLite for inspection and comparison;
- generating a Python demo dashboard.

It is not a hosted service, a trading system, or a fully governed production risk stack.

## Requirements

You need:

- CMake 3.20 or newer;
- a C++20 compiler;
- vcpkg for C++ dependencies;
- Python 3.9 or newer;
- `uv` if you want the easiest Python demo environment.

On Windows, MSVC users should create `.env.cmd` from `msvc.env.cmd.example`. On Linux or macOS, create `.env.sh` from
`gcc.env.sh.example` or `clang.env.sh.example`.

## Build And Test

Use the repository scripts for the normal path.

### Windows PowerShell

```powershell
copy msvc.env.cmd.example .env.cmd
.\scripts\install.ps1
.\scripts\build.ps1
.\scripts\test.ps1
```

### Linux Or macOS

```bash
cp gcc.env.sh.example .env.sh
./scripts/install.sh
./scripts/build.sh
./scripts/test.sh
```

For detailed build variants, see [QUICKSTART.md](QUICKSTART.md).

## Run The Python Demo

The demo loads the checked-in market, portfolio, factor, scenario, and regression files. It runs valuation, factor shock
resolution, `RevaluationSession` observer-pattern revaluation, stress, historical VaR contributions, deterministic
risk, P&L explain, LSMC exercise-policy valuation, Monte Carlo, golden checks, and an optional optimization worker
check.

```powershell
uv sync --project python --extra dashboard --extra optimization
uv run --project python python python\examples\demo_platform.py
```

Generate the optional HTML dashboard:

```powershell
uv run --project python python python\examples\demo_platform.py --dashboard
```

The dashboard is written under `reports/`.

### Observer Pattern Example

The `Observer Pattern Revaluation` section in `python/examples/demo_platform.py` demonstrates the platform's reactive
market-state design:

- `RevaluationSession` builds one C++-owned market state and one cache of QuantLib instruments;
- the selected scenario updates mutable `SimpleQuote` handles in that market state;
- dependent curves and instruments observe those handles and recalculate lazily on the next NPV call;
- the demo restores the original market and verifies that the restored portfolio value matches the base value.

This is the same mechanism used by stress, risk, and Monte Carlo paths to avoid rebuilding curves and instruments for
every shock. The implementation detail is documented in
[docs/architecture/IMPLEMENTATION_CHOICES.md](docs/architecture/IMPLEMENTATION_CHOICES.md).

The same API can be used directly from Python:

```python
session = qrp.create_revaluation_session(portfolio, market, factors, bindings)

base_total = session.total_npv()
quote_updates = {"EURUSD": 1.09, "USD_OIS_5Y": 0.052}

graph = session.dependency_graph()
eurusd_edges = session.dependencies_for_quote("EURUSD")
preview = session.preview_quote_update_impact(quote_updates)
report = session.revalue_quote_update_impact(quote_updates, pnl_tolerance=1e-8)

print(base_total, graph.dependency_count, preview.potentially_affected_trade_ids, report.candidate_pnl)
print([edge.trade_id for edge in eurusd_edges[:5]])
for row in report.trade_diffs:
    if row.moved_above_tolerance:
        print(row.trade_id, row.pnl, row.dependency_quote_ids)

scenario_report = session.revalue_scenario_impact(global_risk_off_scenario)
print(scenario_report.scenario_name, scenario_report.candidate_pnl)
```

The graph and impact APIs are the lightweight a priori layer: `dependency_graph()` and
`dependencies_for_quote(...)` expose a read-only diagnostic snapshot, while `preview_*_impact(...)` maps touched quote
ids to trades through explicit quote ids, market-quote metadata matches, and curve quote membership. They do not reprice.
The `revalue_*_impact(...)` calls use that candidate set, price only those cached instruments, and return the
before/after diffs that Python needs. When these APIs are not called, the normal session path does not build or traverse
the dependency index.

## Use The Python Bindings

Build the extension first:

```powershell
cmake --preset dev
cmake --build build\dev --target quant_risk_platform
```

Then import it from the build output:

```python
from pathlib import Path
import sys

project = Path.cwd()
sys.path.insert(0, str(project / "build" / "dev" / "python" / "RelWithDebInfo"))

import quant_risk_platform as qrp

market = qrp.load_market(str(project / "data" / "market" / "demo_market.json"))
portfolio = qrp.load_portfolio(str(project / "data" / "portfolios" / "demo_portfolio.json"))

valuation_results = qrp.price_portfolio(portfolio, market)
for result in valuation_results:
    print(result.trade_id, result.npv, result.currency, result.support_status)
```

Use `python/examples/demo_platform.py` as the reference for building factor definitions, factor bindings, scenarios,
covariance matrices, P&L explain inputs, LSMC exercise-policy requests, Monte Carlo configuration, and VaR contribution
paths.

### LSMC Exercise Policy

The LSMC binding keeps path simulation, continuation regression, and exercise decisions in C++:

```python
request = qrp.AmericanOptionLsmcRequest()
request.spot = 100.0
request.strike = 100.0
request.risk_free_rate = 0.05
request.volatility = 0.20
request.maturity = 1.0
request.exercise_steps = 24
request.config.num_paths = 4096
request.config.seed = 42
request.config.discount_rate = request.risk_free_rate

result = qrp.price_american_option_lsmc(request)
print(result.value, result.standard_error)
print(result.exercise_times[:3], result.exercise_times[-1])
print(result.basis_function_names)
print(result.regression_diagnostics[0].r_squared)
```

Python receives compact run diagnostics, exercise-grid times, and path values, while C++ owns the stochastic process,
exercise policy, and regression loop. This keeps the hot path out of Python callbacks.

Production product paths use normal trade DTOs and `price_portfolio(...)`; they do not expose arbitrary Python exercise
callbacks. American equity options, Bermudan swaptions, callable fixed-rate bonds, commodity swing contracts, and gas
storage contracts are routed through C++-owned exercise or dynamic-programming policies. The platform demo prints the
American-option helper diagnostics, live production-path rows for Bermudan/swing/storage trades, and a small
straight-versus-callable bond comparison with the implied issuer-call value.

## Run The CLI

The CLI persists data and results in `var/quant_risk_platform.sqlite`.

```powershell
.\build\dev\qrp_cli.exe init-db
.\build\dev\qrp_cli.exe import-market data\market\demo_market.json
.\build\dev\qrp_cli.exe import-portfolio data\portfolios\demo_portfolio.json
.\build\dev\qrp_cli.exe import-scenarios data\scenarios\demo_scenarios.json
```

Run analytics:

```powershell
.\build\dev\qrp_cli.exe run-valuation --portfolio demo_portfolio --snapshot DEMO_MKT_2026_03_24
.\build\dev\qrp_cli.exe run-risk --portfolio demo_portfolio --snapshot DEMO_MKT_2026_03_24
.\build\dev\qrp_cli.exe run-pnl-explain --portfolio demo_portfolio --previous-snapshot DEMO_MKT_2026_03_24 --snapshot DEMO_MKT_2026_03_24
.\build\dev\qrp_cli.exe run-hvar --portfolio demo_portfolio --snapshot DEMO_MKT_2026_03_24 --scenarios demo_factor_scenarios
```

Inspect runs:

```powershell
.\build\dev\qrp_cli.exe list
.\build\dev\qrp_cli.exe report RUN_VAL_...
.\build\dev\qrp_cli.exe compare RUN_VAL_... RUN_VAL_...
```

## Use The Scripts

The scripts are the easiest way to use persisted workflows without remembering CLI paths.

```powershell
.\scripts\init.ps1 -Force
.\scripts\compute.ps1
.\scripts\inspect.ps1 summary
.\scripts\inspect.ps1 portfolios
.\scripts\inspect.ps1 runs
```

The Bash equivalents are:

```bash
./scripts/init.sh -Force
./scripts/compute.sh
./scripts/inspect.sh summary
```

See [scripts/README.md](scripts/README.md) for all parameters.

## Data Files

The sample data lives under:

- `data/market/`: market snapshots with quotes and curve specs;
- `data/portfolios/`: portfolio JSON files;
- `data/scenarios/`: factor definitions, factor bindings, and scenario shocks;
- `data/regression/`: golden outputs used by demos and tests.

Use these files as templates for your own local scenarios.

## Outputs

Common local outputs are:

- `var/quant_risk_platform.sqlite`: SQLite database used by the CLI and compute scripts;
- `var/logs/`: local application logs;
- `reports/demo_risk_dashboard.html`: optional dashboard output;
- `build/dev/python/.../quant_risk_platform.*`: compiled Python extension.

## Product And Analytics Coverage

Supported product families include rates, FX, credit, equities, and commodities. Rates coverage includes callable
fixed-rate bonds alongside the vanilla cash, swap, bond, cap/floor, and swaption paths. Supported analytics include
valuation, risk sensitivities, C++-owned revaluation sessions, historical stress, historical VaR/ES, VaR/ES
contributions, P&L explain, LSMC exercise-policy helpers, Monte Carlo simulation, and persisted run reporting.

For detailed product coverage, see [docs/asset-classes/INDEX.md](docs/asset-classes/INDEX.md). For risk conventions,
see [docs/risk/INDEX.md](docs/risk/INDEX.md).
