# Implementation Index

This section describes delivery sequencing, validation standards, and platform
hardening priorities. It is the bridge between the theoretical documentation and
the repository implementation.

## Canonical Files

1. `docs/implementation/PHASED_BUILD_PLAN.md`  
   Platform build sequence from core trade and market-data models through
   asset-class product coverage, PnL explain, VaR contributions, LSMC, and
   production controls.

## Current Milestone

The codebase is currently through historical VaR/Expected Shortfall contribution
analytics. Rates, FX, credit, commodities, and equities are wired into the
canonical trade model and pricing registry, PnL explain is implemented as a
persisted workflow, and historical VaR/ES contributions are exposed through C++,
Python, SQLite, and the demo platform. The active next milestones are reusable
LSMC, broader event sources, generalized built-portfolio caching, Monte Carlo
contribution decomposition, and production controls. The reusable
`RevaluationSession` API already covers quote and scenario revaluation over a
C++-owned market state and instrument cache.

## Maintenance Rule

Implementation plans should be written as durable engineering documents. They
should define dependencies, deliverables, validation criteria, and architectural
constraints rather than transient task notes.
