# Theory Documentation Index

This folder contains mathematical and statistical foundations.

A recurring convention in these notes is to distinguish clearly between:

- a **general definition**,
- a **standardized or normalized special case**,
- and the **reason** that the normalization is useful.

For example, a general Gaussian law is $\mathcal{N}(\mu,\sigma^2)$ while the standard normal is $\mathcal{N}(0,1)$;
a general Brownian motion may have variance parameter $c$, while standard Brownian motion uses $c=1$.

## Foundation chapters

1. `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md`
2. `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md`
3. `docs/theory/PROBABILITY_LIMIT_THEOREMS.md`
4. `docs/theory/BROWNIAN_MOTION_AND_ITO.md`
5. `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md`

## How this differs from pricing and risk documentation

- `docs/theory/` contains foundations: probability, statistics, stochastic processes, and transform methods.
- `docs/pricing/` contains implementation-facing pricing and market-construction notes.
- `docs/risk/` contains implementation-facing risk, scenario, stress, VaR, and explain notes.
