# Markowitz Portfolio Theory, Portfolio Construction, and Practical Limits

## Notation used in this chapter

Unless stated otherwise:

- $n$ is the number of assets.
- $w$ is the portfolio weight vector.
- $w_i$ is the weight of asset $i$.
- $\mu$ is the vector of expected returns and $\mu_i$ is the expected return of asset $i$.
- $\Sigma$ is the covariance matrix.
- $\sigma_p^2$ and $\sigma_p$ are portfolio variance and standard deviation.
- $\mathbf{1}$ is the vector of ones.
- $r_f$ is the risk-free rate.
- $\lambda$ is the risk-aversion parameter when mean-variance utility is used.

## 1. Why this matters

A good practical position is:

> The classical Markowitz framework is useful because it provides the mean-variance language and optimization template, but in production the harder problem is estimation error, constraints, turnover, and robustness rather than the textbook optimization itself.

---
A companion chapter, `docs/theory/ASSET_PRICING_FOUNDATIONS.md`, connects Markowitz to CAPM, APT, Black-Scholes-Merton, term-structure models, and credit models.


## 1A. Running 2-asset example used in this chapter

Use this example throughout the formulas.

Suppose we have:
- Asset 1 = rates carry strategy, expected annual return $6\%$, volatility $8\%$
- Asset 2 = credit carry strategy, expected annual return $9\%$, volatility $14\%$
- correlation $\rho_{12}=0.25$

Let portfolio weights be:
- $w_1 = 60\%$
- $w_2 = 40\%$

Then expected portfolio return is:

$$
\mathbb{E}[R_p] = w_1 \mu_1 + w_2 \mu_2 = 0.6 \times 0.06 + 0.4 \times 0.09 = 7.2\%
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\mathbb{E}[\cdot]$ denotes expectation.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Portfolio variance is:

$$
\sigma_p^2 = w_1^2 \sigma_1^2 + w_2^2 \sigma_2^2 + 2w_1w_2\rho_{12}\sigma_1\sigma_2
$$

Where:
- $\sigma$ is the volatility or standard deviation parameter.
- $\rho$ is a correlation parameter.

$$
= 0.6^2 \times 0.08^2 + 0.4^2 \times 0.14^2 + 2(0.6)(0.4)(0.25)(0.08)(0.14)
$$

$$
= 0.008224
$$

so

$$
\sigma_p \approx 9.07\%
$$

Interpretation:
- portfolio risk is lower than a naive weighted average of volatilities because imperfect correlation gives diversification.

Good practice in modern portfolio construction:
- treat Markowitz as the clean theoretical base,
- but in production use robust covariance estimation, constraints, turnover controls, concentration limits, and scenario overlays.

## 2. Core objects

Assume $n$ assets.

Portfolio weights:

$$
w \in \mathbb{R}^n
$$

Where:
- $\mathbb{R}^n$ is the $n$-dimensional real vector space.
- $n$ is the number of assets in the portfolio.

Expected returns vector:

$$
\mu \in \mathbb{R}^n
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.

Covariance matrix:

$$
\Sigma \in \mathbb{R}^{n \times n}
$$

Where:
- $\Sigma$ is a covariance matrix.

Unit vector:

$$
\mathbf{1} = (1,\dots,1)^\top
$$

Budget constraint:

$$
\mathbf{1}^\top w = 1
$$

---

## 3. Expected portfolio return

Expected portfolio return is the weighted average of expected asset returns:

$$
\mu_p = w^\top \mu
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.

This is the easy part. The difficult part in practice is estimating $\mu$.

---

## 4. Portfolio variance and volatility

The Markowitz risk measure is portfolio variance:

$$
\sigma_p^2 = w^\top \Sigma w
$$

Where:
- $\sigma$ is the volatility or standard deviation parameter.
- $\Sigma$ is a covariance matrix.

Portfolio volatility is:

$$
\sigma_p = \sqrt{w^\top \Sigma w}
$$

### 4.1 Two-asset form

For two assets:

$$
\sigma_p^2 = w_1^2 \sigma_1^2 + w_2^2 \sigma_2^2 + 2 w_1 w_2 \rho_{12} \sigma_1 \sigma_2
$$

Where:
- $\rho$ is a correlation parameter.

This formula is the diversification principle in one line.

If correlation is low or negative, total portfolio variance can be much lower than the weighted average of individual variances.

---

## 5. Minimum-variance portfolio

The classical problem is:

$$
\min_w \; w^\top \Sigma w
$$

Where:
- $\Sigma$ is a covariance matrix.

subject to:

$$
\mathbf{1}^\top w = 1
$$

and often additional constraints such as:

$$
w_i \ge 0
$$

Where:
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

for long-only portfolios.

Interpretation:
- ignore expected returns for the moment
- find the least volatile combination of assets

---

## 6. Efficient frontier

The efficient frontier is the set of portfolios with the **lowest variance for a given expected return**, or equivalently the **highest expected return for a given variance**.

One standard formulation is:

$$
\min_w \; w^\top \Sigma w
$$

Where:
- $\Sigma$ is a covariance matrix.

subject to:

$$
\mathbf{1}^\top w = 1
$$

$$
w^\top \mu = \mu^*
$$

where $\mu^*$ is a target return.

As $\mu^*$ changes, you trace out the efficient frontier.

---

## 7. Tangency portfolio and Sharpe ratio

If a risk-free asset with return $r_f$ exists, the classical objective becomes maximizing the Sharpe ratio:

$$
\max_w \; \frac{w^\top \mu - r_f}{\sqrt{w^\top \Sigma w}}
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\Sigma$ is a covariance matrix.

The optimal risky portfolio is the **tangency portfolio**.

In unconstrained form, its direction is proportional to:

$$
w \propto \Sigma^{-1}(\mu - r_f \mathbf{1})
$$

This is elegant mathematically and fragile empirically.

---

## 8. Worked numerical example

Suppose two assets have:

$$
\mu_1 = 8\%, \quad \mu_2 = 5\%
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.

$$
\sigma_1 = 20\%, \quad \sigma_2 = 10\%
$$

Where:
- $\sigma$ is the volatility or standard deviation parameter.

$$
\rho_{12} = 0.2
$$

Where:
- $\rho$ is a correlation parameter.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Take weights:

$$
w_1 = 0.6, \quad w_2 = 0.4
$$

Expected return:

$$
\mu_p = 0.6 \cdot 8\% + 0.4 \cdot 5\% = 6.8\%
$$

Variance:

$$
\sigma_p^2 = 0.6^2 \cdot 0.2^2 + 0.4^2 \cdot 0.1^2 + 2 \cdot 0.6 \cdot 0.4 \cdot 0.2 \cdot 0.2 \cdot 0.1
$$

$$
\sigma_p^2 = 0.01792
$$

So volatility is:

$$
\sigma_p = \sqrt{0.01792} \approx 13.39\%
$$

The key insight is that the portfolio volatility is lower than a naive weighted average of 20% and 10% because correlation is less than 1.

---

## 9. How to implement it in practice

### 9.1 Inputs

A practical optimizer needs:
- asset universe
- expected returns $\mu$
- covariance matrix $\Sigma$
- constraints
- transaction-cost model
- turnover / leverage rules

### 9.2 Covariance estimation

This is usually more stable than return forecasting.

Common methods:
- sample covariance
- exponentially weighted covariance
- shrinkage estimators
- factor-model covariance

A practical formula for exponentially weighted covariance uses decaying weights on recent returns.

### 9.3 Expected return estimation

This is usually the hardest and noisiest input.

Common choices:
- historical means
- carry / value / momentum signals
- macro model outputs
- analyst or PM views
- Black–Litterman style blending of equilibrium and views

### 9.4 Constraints

Real optimizers almost always include constraints such as:
- long-only or limited shorting
- weight bounds
- sector / issuer limits
- turnover limits
- tracking-error limit
- factor neutrality
- liquidity limits
- leverage cap

### 9.5 Practical optimization problem

A more realistic objective is often:

$$
\max_w \; w^\top \mu - \lambda w^\top \Sigma w - \kappa \|w - w_{prev}\|_1
$$

Where:
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\Sigma$ is a covariance matrix.

subject to constraints.

Interpretation:
- first term rewards return
- second penalizes variance
- third penalizes turnover / trading cost

---

## 10. What can go wrong

### 10.1 Estimation error

The optimizer is very sensitive to small errors in $\mu$.
This is the biggest textbook-to-production gap.

### 10.2 Unstable covariance matrix

If the covariance matrix is noisy or nearly singular, weights can become extreme.

### 10.3 Corner solutions

Without constraints, the optimizer may produce unintuitive concentrated portfolios.

### 10.4 Regime shifts

Correlations are not constant.
Assets that diversify in calm periods can become highly correlated in stress.

### 10.5 Ignoring costs and liquidity

A mathematically optimal portfolio may be operationally impossible to trade.

---

## 11. How professionals make Markowitz usable

The practical answer is not to throw away mean-variance theory.
It is to **regularize** it.

Typical robustness fixes:
- shrink covariance estimates
- cap weights
- penalize turnover
- use factor models
- smooth expected returns
- blend with benchmark or prior weights
- stress test the solution

This is often closer to how portfolio construction works in reality than pure textbook optimization.

---

## 12. Link to risk decomposition

Once you have a portfolio, you can still use covariance language for risk attribution.

Marginal contribution to variance:

$$
\frac{\partial \sigma_p^2}{\partial w_i} = 2 (\Sigma w)_i
$$

Where:
- $\sigma$ is the volatility or standard deviation parameter.
- $\Sigma$ is a covariance matrix.

Risk contribution of asset $i$ to total variance is linked to:

$$
w_i (\Sigma w)_i
$$

This is useful because it connects optimization to live portfolio risk.

---

## 13. Compact summary

A strong answer is:

> Markowitz provides the basic mean-variance language: expected return is $w^\top \mu$, variance is $w^\top \Sigma w$, and the efficient frontier is obtained by minimizing variance for a target return under constraints. In practice the difficulty is not solving the quadratic program but estimating expected returns and covariances robustly, adding realistic constraints, and controlling turnover. Production implementations therefore benefit from regularized optimization, stable covariance estimation, and explicit trading-cost and concentration constraints rather than a naive unconstrained textbook solution.
