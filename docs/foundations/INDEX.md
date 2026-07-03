# Foundations Documentation Index

This section contains the mathematical foundations used by market data,
valuation, modeling, risk, and implementation chapters.

The chapters are ordered so later topics can reuse earlier probability,
statistics, and stochastic-process language without redefining it.

## Recommended Reading Order

### Part 1: Probability And Statistics

1. `docs/foundations/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md`  
   Random variables, expectation, variance, covariance, conditioning, and the
   language used across the documentation.

2. `docs/foundations/STATISTICAL_DISTRIBUTIONS_AND_MLE.md`  
   Reference distributions, likelihoods, estimators, and asymptotic
   interpretation.

3. `docs/foundations/PROBABILITY_LIMIT_THEOREMS.md`  
   Law of large numbers, central limit theorem, estimator stability, and
   simulation interpretation.

### Part 2: Stochastic Processes And Continuous Time

4. `docs/foundations/BROWNIAN_MOTION_AND_ITO.md`
5. `docs/foundations/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md`

### Part 3: Transforms, Portfolio Theory, And Valuation

6. `docs/foundations/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md`
7. `docs/foundations/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md`
8. `docs/foundations/MARKOWITZ_PORTFOLIO_THEORY.md`
9. `docs/foundations/ASSET_PRICING_FOUNDATIONS.md`

## Core Formulas

Expectation and variance begin the notation chain:

$$
\mathbb{E}[X] = \int x f_X(x)\,dx
$$

$$
\mathrm{Var}(X)
=
\mathbb{E}\left[(X - \mathbb{E}[X])^2\right].
$$

Brownian-driven Ito processes extend the same probability language to
continuous time:

$$
dX_t = \mu_t dt + \sigma_t dW_t.
$$

Monte Carlo, diffusion models, term-structure models, and many pricing
approximations depend on these objects.

## Relationship To The Rest Of The Documentation

- `docs/foundations/` provides the mathematical language.
- `docs/market-data/` defines observed and constructed market objects.
- `docs/asset-classes/` applies the language to product families and market
  conventions.
- `docs/models/` describes reusable model families.
- `docs/risk/` applies the same language to shocks, explain, stress, and
  simulation.
- `docs/architecture/` explains how the concepts become reusable code and
  storage objects.

## Maintenance Rule

When a later document introduces notation that depends on probability,
statistics, or stochastic-process foundations, the canonical definition should
live here first or be linked from `docs/reference/FORMULAS.md`.
