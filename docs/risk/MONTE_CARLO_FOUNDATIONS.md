# Monte Carlo Foundations: LLN, CLT, Error Bars, and Production Implications

## Notation used in this chapter

Unless stated otherwise:

- $X_1,\dots,X_N$ are i.i.d. samples.
- $N$ is the Monte Carlo sample size or number of paths.
- $\mu = \mathbb{E}[X]$ is the target expectation.
- $\sigma^2 = \mathrm{Var}(X)$ is the variance of the underlying random variable.
- $\hat{\mu}_N = \frac{1}{N}\sum_{i=1}^N X_i$ is the Monte Carlo estimator.
- $s_N$ is the sample standard deviation.
- $SE$ is standard error.
- $CI$ is confidence interval.
- $\xrightarrow{P}$ denotes convergence in probability.
- $\xrightarrow{d}$ denotes convergence in distribution.
- $\mathcal{N}(a,b)$ denotes a normal distribution with mean $a$ and variance $b$.

## 1. Why Monte Carlo works at all

Suppose the quantity of interest is an expectation:

$$
V = \mathbb{E}[X]
$$

where $X$ may represent, for example:

- a discounted derivative payoff
- a path-dependent cash-flow functional
- a portfolio loss under a scenario generator
- a risk measure ingredient

Monte Carlo simulates i.i.d. copies $X_1, \dots, X_N$ and estimates $V$ by the sample mean:

$$
\hat{V}_N = \frac{1}{N}\sum_{k=1}^N X_k.
$$

Where:

- $N$ is the number of simulated paths or observations.

This is the core Monte Carlo estimator.

Two fundamental probability results justify it:

- the **Law of Large Numbers (LLN)** gives **consistency**
- the **Central Limit Theorem (CLT)** gives the **error scale and confidence intervals**

A concise summary is:

> LLN tells us the Monte Carlo estimator converges to the true expectation; CLT tells us how fast the error shrinks and
> how to quantify uncertainty around the estimate.

## 1A. Running Monte Carlo pricing example

Suppose we price a 1Y European call on an equity with:

- spot $S_0 = 100$
- strike $K = 100$
- risk-free rate $r = 3\%$
- volatility $\sigma = 20\%$

If on path $i$ we simulate terminal price $S_T^{(i)}$, the discounted payoff is:

$$
X_i = e^{-rT}\max(S_T^{(i)}-K,0)
$$

Where:

- $S_T$ is the asset price at maturity $T$.
- $X_i$ is the $i$-th sampled observation or simulated payoff.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Monte Carlo estimates the price as:

$$
\hat V_N = \frac{1}{N}\sum_{i=1}^N X_i
$$

Where:

- $N$ is the number of simulated paths or observations.

### Tiny numerical illustration

Suppose 5 simulated discounted payoffs are:

$$
(0,\ 4.2,\ 0,\ 11.3,\ 6.5)
$$

Then:

$$
\hat V_5 = \frac{0+4.2+0+11.3+6.5}{5} = 4.40
$$

Interpretation:

- this is only a toy estimate,
- LLN says it stabilizes as $N$ gets large,
- CLT says the uncertainty around 4.40 shrinks at rate $1/\sqrt{N}$.

Good practice:

- always report both the price and its standard error,
- do not present a Monte Carlo number without path count, seed policy, and confidence interval.

## 2. Law of Large Numbers in Monte Carlo

Assume $X_1, X_2, \dots$ are i.i.d. with finite mean:

$$
\mathbb{E}[X_k] = \mu.
$$

Where:

- $\mu$ is the true mean, drift, or expected value depending on context.
- $\mathbb{E}[\cdot]$ denotes expectation.

Then the Monte Carlo estimator is:

$$
\hat{\mu}_N = \frac{1}{N}\sum_{k=1}^N X_k.
$$

Where:

- $\hat{\mu}_N$ is the estimator of the unknown mean $\mu$ based on $N$ samples.
- $N$ is the number of simulated paths or observations.

### 2.1 Weak law interpretation

The weak law says:

$$
\hat{\mu}_N \xrightarrow{P} \mu.
$$

In Monte Carlo language:

- if you rerun the simulation many times with larger and larger $N$
- then the probability that the estimate is far from the true value goes to zero

So LLN is what justifies saying:

> increasing the number of paths makes the Monte Carlo estimator more accurate.

### 2.2 Strong law interpretation

The strong law says:

$$
\hat{\mu}_N \xrightarrow{a.s.} \mu.
$$

In Monte Carlo language:

- along almost every realized simulation stream
- the running average actually settles to the true value as $N \to \infty$

This is why, when you plot the running Monte Carlo estimate, you expect it to stabilize eventually.

### 2.3 What LLN does and does not tell you

LLN tells you:

- the estimator is **consistent**
- more paths improve accuracy asymptotically

LLN does **not** tell you:

- how large $N$ must be in practice
- how large the error is for a given $N$
- what confidence interval to quote

That is where CLT enters.

## 3. Central Limit Theorem in Monte Carlo

Assume now that $X$ has finite variance:

$$
\mathrm{Var}(X) = \sigma^2 < \infty.
$$

Where:

- $\sigma^2$ is the variance of the underlying random variable or process.
- $\sigma$ is the volatility or standard deviation parameter.
- $X$ is a generic random variable.
- $\mathrm{Var}(\cdot)$ denotes variance.

Then the CLT says:

$$
\sqrt{N}(\hat{\mu}_N - \mu) \xrightarrow{d} \mathcal{N}(0, \sigma^2).
$$

Where:

- $\hat{\mu}_N$ is the estimator of the unknown mean $\mu$ based on $N$ samples.
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\mathcal{N}(a,b)$ denotes a normal distribution with mean $a$ and variance $b$.
- $N$ is the number of simulated paths or observations.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Equivalently, for large $N$:

$$
\hat{\mu}_N \approx \mathcal{N}\left(\mu, \frac{\sigma^2}{N}\right).
$$

This is the key quantitative result in Monte Carlo.

### 3.1 What it means

The error behaves approximately like:

$$
\hat{\mu}_N - \mu = O_p\left(\frac{1}{\sqrt{N}}\right).
$$

So Monte Carlo converges at rate:

$$
\boxed{\frac{1}{\sqrt{N}}}
$$

This is one of the most important practical facts in quant finance.

### 3.2 Why this rate matters so much

Dividing the standard error by 2 requires about 4 times more paths.

Dividing the standard error by 10 requires about 100 times more paths.

That is why pure brute-force Monte Carlo can become expensive, and why variance reduction is so important.

## 4. Standard error and confidence intervals

The Monte Carlo standard error is approximately:

$$
\mathrm{SE}(\hat{\mu}_N) = \frac{\sigma}{\sqrt{N}}.
$$

Where:

- $\hat{\mu}_N$ is the estimator of the unknown mean $\mu$ based on $N$ samples.
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\sigma$ is the volatility or standard deviation parameter.
- $N$ is the number of simulated paths or observations.

Since $\sigma$ is usually unknown, we estimate it by the sample standard deviation:

$$
s_N^2 = \frac{1}{N-1}\sum_{k=1}^N (X_k - \hat{\mu}_N)^2.
$$

Then:

$$
\widehat{\mathrm{SE}}(\hat{\mu}_N) = \frac{s_N}{\sqrt{N}}.
$$

A large-sample 95% confidence interval is:

$$
\hat{\mu}_N \pm 1.96\, \frac{s_N}{\sqrt{N}}.
$$

This is exactly how production Monte Carlo engines should report uncertainty.

A concise summary is:

> LLN tells me the estimator is converging; CLT lets me attach a confidence interval and stop once the confidence band
> is sufficiently small.

## 5. Worked Monte Carlo pricing example

Suppose under the risk-neutral measure we price a derivative with discounted payoff:

$$
X = e^{-rT} f(S_T).
$$

Where:

- $S_T$ is the asset price at maturity $T$.
- $X$ is a generic random variable.

Then the fair value is:

$$
V = \mathbb{E}[X].
$$

Where:

- $\mathbb{E}[\cdot]$ denotes expectation.

Simulate $N$ terminal values or paths and compute:

$$
\hat{V}_N = \frac{1}{N}\sum_{k=1}^N e^{-rT} f(S_T^{(k)}).
$$

Where:

- $N$ is the number of simulated paths or observations.

By LLN:

$$
\hat{V}_N \to V
$$

as $N$ becomes large.

By CLT:

$$
\hat{V}_N \approx \mathcal{N}\left(V, \frac{\sigma_X^2}{N}\right)
$$

Where:

- $\sigma$ is the volatility or standard deviation parameter.
- $\mathcal{N}(a,b)$ denotes a normal distribution with mean $a$ and variance $b$.

for large $N$, where:

$$
\sigma_X^2 = \mathrm{Var}(X).
$$

Where:

- $\mathrm{Var}(\cdot)$ denotes variance.

So the practical workflow is:

1. simulate payoffs
2. average them to get price
3. estimate sample variance
4. compute standard error and confidence interval
5. decide whether more paths are needed

## 6. Why independence matters

Classical LLN and CLT are easiest under i.i.d. assumptions. In Monte Carlo we usually design simulations so that random
draws are independent or nearly independent.

If samples are correlated, then:

- LLN and CLT may still hold under broader conditions
- but the variance formula changes
- naive standard errors can be wrong

This matters for:

- MCMC
- time-series resampling
- quasi-Monte Carlo diagnostics
- nested simulations with path reuse

For standard option pricing Monte Carlo, the clean assumption is i.i.d. paths.

## 7. Why weak LLN is still useful for Monte Carlo

The strong law is stronger, but the weak law remains useful because:

- it is often easier to establish
- consistency in probability is already enough for many asymptotic results
- many estimator arguments only need convergence in probability

In practical quant language:

> Weak convergence is already enough to justify that the estimator becomes accurate as the number of paths grows.
> The stronger pathwise statement that one running simulation average converges almost surely is provided by the strong
> law.

## 8. CLT explains Monte Carlo error bars

A common misunderstanding is to think that LLN alone justifies confidence intervals. It does not.

LLN says only:

$$
\hat{\mu}_N \to \mu.
$$

Where:

- $\hat{\mu}_N$ is the estimator of the unknown mean $\mu$ based on $N$ samples.
- $\mu$ is the true mean, drift, or expected value depending on context.

CLT says approximately:

$$
\frac{\hat{\mu}_N - \mu}{s_N/\sqrt{N}} \approx \mathcal{N}(0,1).
$$

Where:

- $\mathcal{N}(a,b)$ denotes a normal distribution with mean $a$ and variance $b$.
- $N$ is the number of simulated paths or observations.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

That is the statement behind error bars and confidence bands.

So in Monte Carlo:

- **LLN = convergence**
- **CLT = uncertainty quantification**

## 9. Greeks and finite-difference Monte Carlo

Suppose you want Delta by bump-and-revalue:

$$
\Delta \approx \frac{V(S_0+h)-V(S_0-h)}{2h}.
$$

Where:

- $S_0$ is the asset price at the valuation date.

Each bumped price is estimated by Monte Carlo, so each has sampling error.

CLT tells you the noise level of each estimate and therefore the noise level of the Greek estimator.

This matters because Greek estimation has a bias-variance trade-off:

- smaller bump $h$ reduces finite-difference bias
- but too small $h$ amplifies Monte Carlo noise

That is exactly the kind of implementation detail that matters in production analytics.

## 10. Nested Monte Carlo and why CLT matters operationally

In XVA, exposure simulation, capital, and stress calculations, one often has nested expectations:

$$
V = \mathbb{E}[\,g(\mathbb{E}[X \mid Y])\,].
$$

Where:

- $X$ is a generic random variable.
- $\mathbb{E}[\cdot]$ denotes expectation.

Then Monte Carlo error appears at more than one level.

CLT-type reasoning helps you understand:

- where sampling error is coming from
- whether outer or inner simulation dominates cost
- how to allocate computational budget

LLN still gives eventual consistency, but CLT is what helps manage runtime and error in production.

## 11. Variance reduction: improving the CLT constant, not the $1/\sqrt{N}$ rate

A very important practical point:

Most classical variance-reduction techniques do **not** change the asymptotic Monte Carlo rate $1/\sqrt{N}$.

They reduce the variance constant:

$$
\mathrm{Var}(\hat{\mu}_N) = \frac{\sigma^2}{N}
$$

Where:

- $\hat{\mu}_N$ is the estimator of the unknown mean $\mu$ based on $N$ samples.
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\sigma^2$ is the variance of the underlying random variable or process.
- $\sigma$ is the volatility or standard deviation parameter.
- $\mathrm{Var}(\cdot)$ denotes variance.
- $N$ is the number of simulated paths or observations.

by making $\sigma^2$ smaller.

Examples:

- antithetic variates
- control variates
- stratification
- importance sampling
- common random numbers

So the logic is:

$$
\text{same } \frac{1}{\sqrt{N}} \text{ rate, smaller constant} \Rightarrow \text{tighter confidence interval for same } N.
$$

#### Example for 11.1 — control variates

Suppose you know $\mathbb{E}[Y]$ exactly and define:

$$
Z = X - \beta (Y - \mathbb{E}[Y]).
$$

Where:

- $X$ is a generic random variable.
- $\mathbb{E}[\cdot]$ denotes expectation.

Then:

$$
\mathbb{E}[Z] = \mathbb{E}[X].
$$

If $X$ and $Y$ are strongly correlated, then:

$$
\mathrm{Var}(Z) < \mathrm{Var}(X),
$$

which reduces Monte Carlo error.

## 12. What happens if variance is infinite?

CLT in its standard form requires finite variance.

If $X$ has finite mean but infinite variance:

- LLN may still hold under suitable conditions
- but the standard Gaussian CLT can fail
- confidence intervals based on $s_N/\sqrt{N}$ become unreliable

This is one reason heavy-tailed models require extra care.

## 13. What happens with quasi-Monte Carlo?

Quasi-Monte Carlo replaces randomness with low-discrepancy sequences.

Then the classical i.i.d. LLN / CLT story is no longer the full picture.

The goal becomes better deterministic integration error, often superior to plain Monte Carlo in practice for smooth
moderate-dimensional problems.

- standard Monte Carlo: LLN + CLT + probabilistic error bars
- quasi-Monte Carlo: deterministic low-discrepancy error logic, sometimes randomized for error estimation

## 14. Practical production guidance

A strong implementation should:

- report the Monte Carlo estimate
- report sample variance and standard error
- report a confidence interval
- allow stopping on tolerance, for example:

$$
1.96\,\frac{s_N}{\sqrt{N}} < \varepsilon
$$

Where:

- $N$ is the number of simulated paths or observations.

- expose diagnostics for effective sample size, variance reduction, and path reuse
- make the estimator and error decomposition reproducible

For a front-office or risk platform, this is not optional. Users need to know not only the estimate but also its
reliability.

## 15. Concise summaries

### 15.1 Core explanation

> In Monte Carlo pricing, we estimate an expectation by a sample average. The Law of Large Numbers justifies
> consistency: as the number of paths increases, the sample average converges to the true price. The Central Limit
> Theorem is what makes the method operational, because it says the estimation error is asymptotically Gaussian with
> standard deviation proportional to $1/\sqrt{N}$. That lets us compute standard errors, confidence intervals, and
> stopping rules.

### 15.2 Why variance reduction matters

> The asymptotic Monte Carlo rate is usually still $1/\sqrt{N}$, so variance reduction is about shrinking the variance
> constant, not changing the rate. In practice that means tighter error bars for the same computational budget.

### 15.3 Weak vs. strong LLN in Monte Carlo

> The weak law is enough to show that the estimator becomes accurate in probability. The strong law gives the stronger
> statement that the running average converges almost surely along a realized simulation stream. For practical Monte
> Carlo consistency arguments, weak convergence is often sufficient; for pathwise intuition, the strong law is stronger.

## 16. One-line summary

$$
\boxed{\text{LLN explains why Monte Carlo converges; CLT explains how accurately it converges for finite } N.}
$$

Where:

- $N$ is the number of simulated paths or observations.

## 17. How the confidence interval is derived from the CLT

Start from the CLT for the sample mean:

$$
\sqrt{N}(\hat{\mu}_N-\mu)\xrightarrow{d}\mathcal{N}(0,\sigma^2).
$$

Where:

- $\hat{\mu}_N$ is the estimator of the unknown mean $\mu$ based on $N$ samples.
- $\mu$ is the true mean, drift, or expected value depending on context.
- $\sigma^2$ is the variance of the underlying random variable or process.
- $\sigma$ is the volatility or standard deviation parameter.
- $\mathcal{N}(a,b)$ denotes a normal distribution with mean $a$ and variance $b$.
- $N$ is the number of simulated paths or observations.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

Divide both sides by $\sigma$:

$$
\frac{\sqrt{N}(\hat{\mu}_N-\mu)}{\sigma}\xrightarrow{d}\mathcal{N}(0,1).
$$

Equivalently, for large $N$:

$$
\frac{\hat{\mu}_N-\mu}{\sigma/\sqrt{N}} \approx \mathcal{N}(0,1).
$$

Let $Z\sim\mathcal{N}(0,1)$. We know approximately:

$$
\mathbb{P}(-1.96 \le Z \le 1.96)\approx 0.95.
$$

So replacing $Z$ by the standardized Monte Carlo error gives:

$$
\mathbb{P}\left(-1.96 \le \frac{\hat{\mu}_N-\mu}{\sigma/\sqrt{N}} \le 1.96\right)\approx 0.95.
$$

Multiply through by $\sigma/\sqrt{N}$:

$$
\mathbb{P}\left(-1.96\frac{\sigma}{\sqrt{N}} \le \hat{\mu}_N-\mu \le 1.96\frac{\sigma}{\sqrt{N}}\right)\approx 0.95.
$$

Rearrange in terms of the unknown parameter $\mu$:

$$
\mathbb{P}\left(\hat{\mu}_N-1.96\frac{\sigma}{\sqrt{N}} \le \mu \le \hat{\mu}_N+1.96\frac{\sigma}{\sqrt{N}}\right)\approx 0.95.
$$

This is the large-sample 95% confidence interval.

### 17.1 In practice we do not know $\sigma$

So we replace it by the sample standard deviation $s_N$:

$$
s_N^2 = \frac{1}{N-1}\sum_{k=1}^N (X_k-\hat{\mu}_N)^2
$$

and obtain the operational interval:

$$
\mu \in \left[\hat{\mu}_N-1.96\frac{s_N}{\sqrt{N}},\ \hat{\mu}_N+1.96\frac{s_N}{\sqrt{N}}\right].
$$

For Monte Carlo pricing, $\mu$ is the true price or expectation we are estimating.

#### Example for 17.2 — CLT confidence interval for a Monte Carlo price

Suppose a Monte Carlo engine gives:

- estimate $\hat V_N = 10.24$
- sample standard deviation of discounted payoffs $s_N = 4.50$
- number of paths $N = 10{,}000$

Then the standard error is:

$$
\widehat{\mathrm{SE}}(\hat V_N)=\frac{4.50}{\sqrt{10{,}000}}=0.045.
$$

A 95% confidence interval is:

$$
10.24 \pm 1.96\times 0.045 = 10.24 \pm 0.0882
$$

so approximately:

$$
[10.152,\ 10.328].
$$

Interpretation:

- the point estimate is 10.24
- the CLT says the finite-sample Monte Carlo noise is about 8.8 cents at 95% confidence
- front office can decide whether this precision is sufficient for pricing or hedging

## 18. What confidence intervals are useful for in general

CLT-based confidence intervals are useful whenever an estimator is approximately normal for large sample size.

They are used to:

- quantify estimation uncertainty
- compare two models or two portfolios
- decide whether more simulation is needed
- set stopping rules
- report statistical reliability to model users or validators

A very useful summary is:

$$
\text{point estimate} + \text{standard error} + \text{confidence interval}
$$

This is much more informative than reporting only the point estimate.

## 19. How we use CLT to bound unknown parameters

A confidence interval is exactly a probabilistic bound for an unknown parameter.

Examples of parameters:

- the true mean return $\mu$
- the true option price $V$
- the true default probability $p$
- the true exposure expectation
- the true value of a calibrated statistic under repeated sampling logic

### 19.1 Two-sided bound

For a parameter $\theta$ estimated by $\hat{\theta}$ with estimated standard error
$\widehat{\mathrm{SE}}(\hat{\theta})$, a large-sample interval is:

$$
\theta \in \left[\hat{\theta}-z_{\alpha/2}\widehat{\mathrm{SE}}(\hat{\theta}),\ \hat{\theta}+z_{\alpha/2}\widehat{\mathrm{SE}}(\hat{\theta})\right]
$$

Where:

- $\hat{\theta}$ is an estimator of the unknown parameter $\theta$.
- $\theta$ is a generic unknown parameter or target quantity.
- $\alpha$ is a tail probability or significance level.
- $z_{\cdot}$ denotes a standard-normal quantile used in confidence intervals.

approximately with confidence level $1-\alpha$.

Typical quantiles:

- 90%: $z_{0.95}\approx 1.645$
- 95%: $z_{0.975}\approx 1.96$
- 99%: $z_{0.995}\approx 2.576$

### 19.2 One-sided upper bound

Sometimes risk or validation wants a conservative upper bound:

$$
\theta \le \hat{\theta} + z_{1-\alpha}\widehat{\mathrm{SE}}(\hat{\theta})
$$

Where:

- $\hat{\theta}$ is an estimator of the unknown parameter $\theta$.
- $\theta$ is a generic unknown parameter or target quantity.
- $\alpha$ is a tail probability or significance level.
- $z_{\cdot}$ denotes a standard-normal quantile used in confidence intervals.

approximately with confidence level $1-\alpha$.

This is useful, for example, for:

- upper-bounding simulation error
- conservative exposure or loss metrics
- testing whether a parameter exceeds a threshold

#### Example for 19.3 — confidence interval for a probability

Suppose 80 defaults are observed in 2,000 obligors. Then:

$$
\hat p = \frac{80}{2000} = 0.04.
$$

Where:

- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

For a Bernoulli variable, the asymptotic standard error is:

$$
\widehat{\mathrm{SE}}(\hat p) = \sqrt{\frac{\hat p(1-\hat p)}{n}}
= \sqrt{\frac{0.04\times 0.96}{2000}}
\approx 0.00438.
$$

Where:

- $n$ is the sample size.

A 95% interval is approximately:

$$
0.04 \pm 1.96\times 0.00438
= 0.04 \pm 0.0086
$$

so:

$$
[0.0314,\ 0.0486].
$$

This is a practical CLT-based bound for a probability parameter.

## 20. What theorem is used to prove what?

This is the clean map to remember.

### 20.1 LLN proves consistency

If the estimator is a sample average or is built from sample averages, LLN is used to prove:

$$
\hat{\theta}_N \to \theta.
$$

Where:

- $\hat{\theta}$ is an estimator of the unknown parameter $\theta$.
- $\theta$ is a generic unknown parameter or target quantity.

This answers the question:

> Does the estimator converge to the true quantity as sample size grows?

### 20.2 CLT proves the asymptotic distribution

CLT is used to prove:

$$
\sqrt{N}(\hat{\theta}_N-\theta)\xrightarrow{d}\mathcal{N}(0,\tau^2)
$$

Where:

- $\hat{\theta}$ is an estimator of the unknown parameter $\theta$.
- $\theta$ is a generic unknown parameter or target quantity.
- $\mathcal{N}(a,b)$ denotes a normal distribution with mean $a$ and variance $b$.
- $N$ is the number of simulated paths or observations.
- $0$ denotes the valuation date, or “today,” when it appears in term-structure notation.

for an appropriate asymptotic variance $\tau^2$.

This answers the questions:

- How large is the error at finite $N$?
- What is the standard error?
- how to build a confidence interval
- how to compare two estimates statistically

So the distinction is:

$$
\text{LLN} \Rightarrow \text{consistency}
\qquad
\text{CLT} \Rightarrow \text{finite-sample uncertainty and confidence intervals}
$$

## 21. Realistic front-office and risk usage

Modern front-office and risk teams do not stop at “the estimator converges eventually.”

They ask:

- How noisy is the number today?
- Is the confidence interval acceptable for trading?
- Is the numerical noise smaller than bid-offer, hedge tolerance, or P\&L explain thresholds?
- whether more paths or better variance reduction is needed

Typical operational uses:

- stop a pricing run when the relative confidence interval is below a threshold
- reject a result if the standard error is too large
- compare model changes only if the difference is statistically meaningful relative to Monte Carlo noise
- set tighter tolerances for large books or hedge-sensitive Greeks

### 21.1 Relative error stopping rule

A common practical rule is to continue simulation until:

$$
\frac{1.96\, s_N/\sqrt{N}}{\max(|\hat{\mu}_N|,\epsilon)} < \eta
$$

where:

- $\eta$ is a target relative tolerance, for example 0.5% or 1%
- $\epsilon$ avoids division by a near-zero estimate

This is often more meaningful to traders than a purely absolute error threshold.

## 22. Caveats and good practice

The CLT interval is asymptotic, so it works best when:

- $N$ is reasonably large
- variance is finite
- observations are independent or weakly dependent with the correct variance formula
- the payoff distribution is not too pathological

Good practice:

- report the path count
- report sample variance and standard error
- report confidence intervals
- note if common random numbers or control variates were used
- avoid quoting a Monte Carlo number without its uncertainty band

If the distribution is highly skewed or heavy-tailed, additional care may be needed:

- more paths
- variance reduction
- transformed estimators
- bootstrap or other robust diagnostics in some settings

## 23. One-line memory aid

$$
\boxed{\text{LLN proves that the estimator gets to the truth; CLT proves how tightly we can bound the truth around the estimate.}}
$$
