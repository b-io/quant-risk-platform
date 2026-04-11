# Credit Curve Construction

This chapter describes how a production platform should build and validate reusable credit curves.

## Notation used in this chapter

Unless stated otherwise:

- $t=0$ is the valuation date.
- $T_k$ is the maturity of quoted CDS tenor $k$.
- $s_k^{mkt}$ is the market CDS spread at tenor $T_k$.
- $Q(0,T)$ is survival probability to time $T$.
- $h(t)$ is the hazard rate at time $t$.
- $R$ is the recovery rate.
- $D(0,T)$ is the discount factor to maturity $T$.
- $h=(h_1,\ldots,h_m)$ is the vector of hazard parameters used in calibration.

## 1. Scope

A credit curve is not a discount curve. It represents default timing and loss-given-default assumptions, not risk-free discounting. In practice the builder must combine:

- normalized CDS or bond-spread quotes,
- contract conventions,
- a discount curve in the same currency,
- a recovery assumption,
- a chosen interpolation or bootstrap family.

## 2. Core quantity

Let $Q(0,T)$ be the survival probability from today to time $T$. In an intensity framework,

$$
Q(0,T)=\exp\left(-\int_0^T h(t)\,dt\right)
$$

Where:

- $h(t)$ is the hazard rate.
- $t$ is the integration variable.

If hazard is piecewise constant, this formula becomes especially convenient because each maturity interval contributes a simple exponential term.

## 3. Calibration target

Let $h=(h_1,\ldots,h_m)$ be the hazard-parameter vector. Let $s_k^{model}(h)$ be the model spread implied by $h$ for tenor $T_k$. The calibration problem is to solve

$$
s_k^{model}(h)=s_k^{mkt}
$$

for each quoted tenor $k$.

Where:

- $s_k^{mkt}$ is the observed market quote.
- $s_k^{model}(h)$ is the model-implied quote.
- $h=(h_1,\ldots,h_m)$ is the parameter vector being solved for.

When exact fit is not desired or the quote set is noisy, the builder may instead minimize a weighted objective such as the sum of squared quote errors.

## 4. Construction flow

A practical production flow is:

1. normalize input quotes into a typed credit-quote schema,
2. resolve contract conventions from a registry,
3. attach the appropriate discount curve,
4. choose the calibration family,
5. solve for survival or hazard nodes,
6. publish a reusable curve object with diagnostics and factor metadata.

## 5. Minimum outputs

A reusable credit curve should expose:

- $Q(0,T)$ for survival queries,
- $1-Q(0,T)$ for cumulative default probability,
- hazard-rate values or hazard segments,
- calibration residuals,
- node labels and factor identifiers,
- metadata such as issuer, seniority, currency, and recovery assumption.

## 6. Validation checks

A calibrated curve is acceptable only if it passes both numerical and economic checks.

Numerical checks:

- quoted instruments reprice within tolerance,
- solver convergence is stable under small quote perturbations,
- interpolation and bootstrap choices do not create oscillatory artifacts.

Economic checks:

- survival probability is non-increasing in maturity,
- cumulative default probability stays between 0 and 1,
- hazard rates are non-negative or economically interpretable under the chosen framework,
- CS01 and scenario behavior are stable enough to support reporting.

## 7. Why factor metadata matters

Credit risk is not identified by tenor alone. A realistic factor key normally needs at least:

- reference entity,
- seniority,
- currency,
- curve family,
- maturity bucket,
- optional sector or rating tags.

This is what allows CS01, stress, and VaR to aggregate correctly.
