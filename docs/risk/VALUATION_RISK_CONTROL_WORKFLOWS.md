# Valuation, Risk Control, and Analytics Workflows

## 1. Purpose

Pricing formulas are only useful in a platform when their inputs, approximations, controls, and outputs are explicit.
This chapter describes how valuation, risk measurement, PnL explain, and control workflows can share the same market
state without weakening the distinction between fast analytics and controlled reporting.

The goal is architectural: define reusable analytics objects, make approximations visible, and preserve enough lineage
for a result to be reproduced and challenged.

## 2. Shared Objects, Different Uses

The same trade, curve, volatility surface, scenario, and valuation result can be consumed in several ways:

- intraday valuation and hedging analytics;
- sensitivity and scenario analysis;
- official PnL explain and risk reporting;
- independent model validation and control review;
- historical replay and backtesting.

The platform should not duplicate valuation logic for each use. It should expose a common pricing core with explicit
contexts:

$$
PV = f(\text{trade state}, \text{market state}, \text{model state}, \text{valuation context}).
$$

The valuation context records choices such as pricing date, reporting currency, collateral curve, scenario overlay,
calendar treatment, model configuration, and approximation policy.

## 3. Representative Rates Metrics

For a rates or macro portfolio, common analytics include:

- PV or NPV;
- DV01 or PV01;
- key-rate DV01;
- carry and roll-down;
- curve steepener or flattener sensitivity;
- spread duration;
- scenario PnL;
- VaR and Expected Shortfall;
- explained versus unexplained PnL;
- market-data quality flags.

### First-order rates example

Suppose a portfolio has:

- USD 5Y bond DV01: $+42{,}000$ USD/bp;
- USD 2Y swap DV01: $-18{,}000$ USD/bp.

For a curve shock of:

- $+10$ bp at 5Y;
- $+5$ bp at 2Y;

the first-order PnL approximation is:

$$
\Delta P
\approx
42{,}000(-10) + (-18{,}000)(-5)
= -330{,}000.
$$

The result is a local linear estimate. A platform should preserve the estimate, the factor shocks, the sensitivities used,
and the revaluation result when full repricing is also available.

## 4. Approximation Hierarchy

### 4.1 Modified duration and DV01

These are useful for:

- fast first-order explanation;
- directional hedging;
- small market moves;
- stable linear products.

They are insufficient for:

- large shocks;
- callable or highly convex positions;
- options;
- curve reshaping beyond the chosen buckets.

### 4.2 Key-rate and bucketed risk

Bucketed sensitivities support non-parallel curve analysis. They are a better match for curve construction, hedging, and
scenario design because each reported number maps to a named market factor.

### 4.3 Full revaluation

Full repricing is required when nonlinearities, path dependence, exercise decisions, or large shocks dominate. A robust
workflow can report approximation and revaluation side by side:

$$
\Delta PV_{\text{residual}}
=
\Delta PV_{\text{full revaluation}}
-
\Delta PV_{\text{linear explain}}.
$$

The residual is not an error by itself. It is a diagnostic that indicates convexity, cross terms, missing factors, model
changes, data issues, or genuine unexplained PnL.

## 5. Platform Design Principles

### 5.1 One canonical market state

Curves, surfaces, fixings, spreads, and factor definitions should be built once and reused. Scenario analysis should be
expressed as an overlay on the base state, not as an unrelated market object with unclear provenance.

### 5.2 Reproducibility

Every material result should specify:

- which market snapshot was used;
- which curve and surface versions were built;
- which model and configuration were selected;
- which trades were included;
- which scenario or random seed was applied;
- which approximation policy was active.

### 5.3 Explainability

A useful PnL explain separates:

- carry and roll-down;
- market moves;
- new, amended, and expired trades;
- fixings and cashflows;
- residual PnL.

### 5.4 Controls

Controls belong close to the analytics:

- stale quote detection;
- curve-arbitrage sanity checks;
- negative or explosive forward diagnostics;
- missing fixing checks;
- scenario completeness checks;
- sensitivity coverage checks;
- reconciliation between aggregate and trade-level outputs.

### 5.5 Performance with transparency

Fast analytics should still preserve inputs, assumptions, and diagnostics. Controlled reporting should remain close
enough to the same pricing core that differences can be explained by context rather than by duplicated logic.

## 6. Data-release Workflow

For a macro data surprise, a portfolio workflow typically needs:

- front-end rates repricing;
- curve steepening or flattening scenarios;
- FX and spread reactions;
- live and aged PnL explain;
- key-bucket sensitivity changes;
- limit and concentration diagnostics;
- checks for stale market data or stale hedge assumptions.

The engineering consequence is that pricing, factor mapping, scenario design, and persistence must be designed together.
An isolated pricing function is insufficient for a risk platform.

## 7. Interpreting a Formula in the Platform

When adding a formula to the documentation or code, record:

- the mathematical object being defined;
- the units and market convention;
- whether the quantity is observed, calibrated, simulated, or derived;
- the implementation object that stores it;
- where the approximation is valid;
- which diagnostic reveals failure or model risk.

This is the difference between a formula collection and a platform theory document.
