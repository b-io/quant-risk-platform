# Theory Documentation Index

This section contains the mathematical foundations used by the design, pricing, and risk notes.

The theory notes are ordered so that each later topic builds on earlier probabilistic or stochastic language.

## Recommended study order

### Phase 1 — probability and statistics

1. `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md`  
   Start here for random variables, expectation, variance, covariance, conditioning, and the language used across the documentation.

2. `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md`  
   Continue with reference distributions, likelihoods, estimators, and asymptotic intuition.

3. `docs/theory/PROBABILITY_LIMIT_THEOREMS.md`  
   This chapter explains why sample averages and simulation estimators stabilize.

### Phase 2 — stochastic processes and continuous time

4. `docs/theory/BROWNIAN_MOTION_AND_ITO.md`
5. `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md`

### Phase 3 — transforms, portfolio theory, and valuation

6. `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md`
7. `docs/theory/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md`
8. `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md`
9. `docs/theory/ASSET_PRICING_FOUNDATIONS.md`

## Core formulas that connect the section

Expectation and variance begin the entire chain:

$$
\mathbb{E}[X] = \int x \, f_X(x)\,dx
$$

$$
\mathrm{Var}(X) = \mathbb{E}\left[(X - \mathbb{E}[X])^2\right]
$$

Brownian-driven Itô processes then extend this language to continuous time:

$$
dX_t = \mu_t \, dt + \sigma_t \, dW_t
$$

Monte Carlo, diffusion models, and many pricing approximations all depend on understanding these objects correctly.

## How this section relates to the rest of the docs

- `docs/theory/` gives the mathematical language.
- `docs/pricing/` turns that language into valuation objects and market conventions.
- `docs/risk/` turns the same language into shocks, explain, stress, and simulation outputs.
- `docs/design/` explains how those concepts become reusable code and storage objects.

## Maintenance rule

When a later document introduces notation that depends on probability or stochastic-process foundations, the canonical definitions should live here first.
