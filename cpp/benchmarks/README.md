# C++ Benchmarks

This directory contains C++ benchmark executables for measuring platform throughput outside the unit-test suite.
Benchmarks are opt-in because timings are machine-dependent and slower than correctness tests.

## Key Files

- `benchmark_portfolio.cpp`: loads the demo market, demo portfolio, and demo scenario factor configuration, then measures
  repeated portfolio valuation throughput and one deterministic risk run.

## What It Measures

- valuation time across the current multi-asset demo portfolio;
- valuation results produced across repeated iterations;
- trades per second for the valuation loop;
- deterministic risk runtime using the current factor/binding configuration from `data/scenarios/demo_scenarios.json`.

The benchmark is a smoke/performance probe, not a correctness oracle. Use regression tests and golden fixtures for
numerical correctness.

## Usage

The easiest path is through the test scripts:

```powershell
.\scripts\test.ps1 -Performance
.\scripts\test.ps1 -Performance -PerformanceIterations 25
```

```bash
./scripts/test.sh -Performance
./scripts/test.sh -Performance -PerformanceIterations 25
```

Manual CMake usage:

```powershell
cmake -S . -B build\dev -DQRP_BUILD_BENCHMARKS=ON
cmake --build build\dev --target benchmark_portfolio --config RelWithDebInfo
.\build\dev\benchmark_portfolio.exe --iterations 100 --data-root data
```
