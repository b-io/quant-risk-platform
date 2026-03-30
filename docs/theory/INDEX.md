# Theory Documentation Index

This folder contains mathematical and statistical foundations.

A recurring convention in these notes is to distinguish clearly between:

- a **general definition**,
- a **standardized or normalized special case**,
- and the **reason** that the normalization is useful.

For example, a general Gaussian law is $\mathcal{N}(\mu,\sigma^2)$ while the standard normal is $\mathcal{N}(0,1)$;
a general Brownian motion may have variance parameter $c$, while standard Brownian motion uses $c=1$.

## Foundation chapters

- `docs/theory/ASSET_PRICING_FOUNDATIONS.md` — pricing-kernel view of valuation plus CAPM, APT, Black-Scholes-Merton,
  term-structure, and credit foundations.
- `docs/theory/BROWNIAN_MOTION_AND_ITO.md` — Brownian motion, Wiener processes, Itô calculus, and
  stochastic-differential-equation basics.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md` — characteristic functions and a practical introduction to
  Fourier methods in quantitative finance.
- `docs/theory/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md` — moment-generating functions, characteristic
  functions, and convergence notions used in probability and statistics.
- `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md` — mean-variance portfolio construction, efficient frontiers, and practical
  portfolio-theory limits.
- `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md` — mean reversion, Ornstein-Uhlenbeck intuition, Vasicek,
  Hull-White, CIR, and worked short-rate examples.
- `docs/theory/PROBABILITY_LIMIT_THEOREMS.md` — laws of large numbers, central limit theorem, and the convergence ideas
  behind estimation and simulation.
- `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md` — random variables, expectation, variance, covariance, and core
  probability identities.
- `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md` — statistical distributions, maximum likelihood estimation, and
  asymptotic behavior of estimators.

## Suggested reading order

1. `docs/theory/RANDOM_VARIABLES_EXPECTATION_VARIANCE.md` — probability language and identities used throughout the
   docs.
2. `docs/theory/STATISTICAL_DISTRIBUTIONS_AND_MLE.md` — distributions, likelihoods, and estimator intuition.
3. `docs/theory/PROBABILITY_LIMIT_THEOREMS.md` — convergence results behind estimation and simulation.
4. `docs/theory/BROWNIAN_MOTION_AND_ITO.md` — continuous-time stochastic-process foundations.
5. `docs/theory/MEAN_REVERSION_AND_SHORT_RATE_MODELS.md` — mean-reversion intuition and the short-rate families used
   later in the rates notes.
6. `docs/theory/CHARACTERISTIC_FUNCTIONS_AND_FOURIER.md` — transform methods and why they matter in quant finance.
7. `docs/theory/CHARACTERISTIC_FUNCTIONS_MGFS_AND_CONVERGENCE.md` — transform identities and convergence links.
8. `docs/theory/MARKOWITZ_PORTFOLIO_THEORY.md` — portfolio-construction foundations.
9. `docs/theory/ASSET_PRICING_FOUNDATIONS.md` — valuation and risk foundations spanning CAPM, factor models, option
   pricing, and credit.

## How this differs from pricing and risk documentation

- `docs/theory/` contains foundations: probability, statistics, stochastic processes, transforms, portfolio
  optimization, and asset-pricing foundations.
- `docs/pricing/` contains implementation-facing pricing and market-construction notes.
- `docs/risk/` contains implementation-facing risk, scenario, stress, VaR, and explain notes.
