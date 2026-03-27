# Probability Limit Theorems: Law of Large Numbers and Central Limit Theorem

## 1. Why these theorems matter

Two of the most important results in probability are:

- the **Law of Large Numbers (LLN)**, which explains why empirical averages stabilize;
- the **Central Limit Theorem (CLT)**, which explains why normalized sums often look approximately Gaussian.

These theorems are fundamental in:

- estimation and statistics,
- Monte Carlo simulation,
- risk aggregation,
- time-series analysis,
- confidence intervals and hypothesis testing,
- asymptotic reasoning in quantitative finance.

A useful way to remember the distinction is:

- **LLN** tells us where the sample average goes;
- **CLT** tells us how it fluctuates around that limit.

### How to read the normalizations in this chapter

Several formulas below contain normalizing factors such as $1/n$, $1/\sqrt{n}$, or $\sigma$. These are not arbitrary
decorations. In each case, it is helpful to separate:

1. the **raw quantity** being studied, such as $S_n$ or $\overline{X}_n$;
2. the **structural scaling** forced by variance aggregation, such as $n$ for sums or $1/n$ for averages;
3. the **standard normalization** used to obtain a clean reference limit, such as division by $\sigma$ to get a
   standard normal limit.

This same pattern reappears later in Brownian motion: linear variance growth is structural, while choosing variance
parameter $1$ is a normalization.

---

## 2. Setup: sums and sample averages

Let

$$
X_1, X_2, \dots
$$

be independent and identically distributed random variables, with

$$
\mu = \mathbb{E}[X_1],
\sigma^2 = \mathrm{Var}(X_1).
$$

Define the partial sum

$$
S_n = X_1 + X_2 + \cdots + X_n
$$

and the sample average

$$
\overline{X}_n = \frac{1}{n} S_n = \frac{1}{n}\sum_{i=1}^n X_i.
$$

Then:

- the LLN studies the limit of $\overline{X}_n$;
- the CLT studies the limit of
  $$
\frac{S_n - n\mu}{\sigma \sqrt{n}}
=
\frac{\sqrt{n}(\overline{X}_n-\mu)}{\sigma}.
$$

---

## 3. The Law of Large Numbers

## 3.1 Intuition

Suppose we flip a fair coin repeatedly and code:

$$
X_i =
\begin{cases}
1, & \text{if heads},\\
0, & \text{if tails}.
\end{cases}
$$

Then $\mathbb{E}[X_i] = \tfrac12$.

If we compute the proportion of heads after $n$ tosses,

$$
\overline{X}_n = \frac{1}{n}\sum_{i=1}^n X_i,
$$

we expect this proportion to get closer and closer to $\tfrac12$ as $n$ grows.

This is exactly the content of the LLN.

## 3.2 Weak Law of Large Numbers

A standard version says:

If $X_1, X_2, \dots$ are i.i.d. and $\mathbb{E}[|X_1|] < \infty$, then
$$
\overline{X}_n \xrightarrow{P} \mu,
$$
meaning that for every $\varepsilon > 0$,
$$
\mathbb{P}\left(|\overline{X}_n - \mu| > \varepsilon\right) \to 0.
$$

This is **convergence in probability**.

### What it means

For large $n$, the sample average is very likely to be close to the true mean.

It does **not** say that every realization is monotone or exact; it says the probability of a large deviation becomes
small.

## 3.3 Proof under finite variance

Assume $\mathrm{Var}(X_1)=\sigma^2 < \infty$.

Then

$$
\mathbb{E}[\overline{X}_n] = \mu
$$

and, since the variables are independent,

$$
\mathrm{Var}(\overline{X}_n)
=
\mathrm{Var}\left(\frac{1}{n}\sum_{i=1}^n X_i\right)
=
\frac{1}{n^2}\sum_{i=1}^n \mathrm{Var}(X_i)
=
\frac{n\sigma^2}{n^2}
=
\frac{\sigma^2}{n}.
$$

Now apply **Chebyshev's inequality**:

$$
\mathbb{P}\left(|\overline{X}_n-\mu|>\varepsilon\right)
\le
\frac{\mathrm{Var}(\overline{X}_n)}{\varepsilon^2}
=
\frac{\sigma^2}{n\varepsilon^2}.
$$

As $n\to\infty$,

$$
\frac{\sigma^2}{n\varepsilon^2} \to 0.
$$

Hence

$$
\mathbb{P}\left(|\overline{X}_n-\mu|>\varepsilon\right) \to 0,
$$

which proves the weak LLN in this case.

## 3.4 Strong Law of Large Numbers

A stronger result states:

Under suitable assumptions,
$$
\overline{X}_n \xrightarrow{a.s.} \mu.
$$

This means that with probability $1$, the entire sequence of sample averages converges to the mean.

This is **almost sure convergence**, which is stronger than convergence in probability.

### Intuition

The weak LLN says:

- for each large $n$, a big error is unlikely.

The strong LLN says:

- with probability one, if you keep observing the same sequence forever, the averages really settle down to $\mu$.

---

## 4. Why the variance of the sample mean goes to zero

The LLN is often summarized by the simple identity:

$$
\mathrm{Var}(\overline{X}_n)=\frac{\sigma^2}{n}.
$$

This shows why averaging stabilizes noise.

If each observation has variance $\sigma^2$, then averaging $n$ independent observations divides the variance by $n$.
The standard deviation becomes

$$
\sqrt{\mathrm{Var}(\overline{X}_n)}
=
\frac{\sigma}{\sqrt{n}}.
$$

So typical fluctuations shrink like $1/\sqrt{n}$.

This is one of the most important scaling laws in probability and statistics.

---

## 5. The Central Limit Theorem

## 5.1 Statement

A standard version of the CLT says:

If $X_1, X_2, \dots$ are i.i.d. with
$$
\mathbb{E}[X_1]=\mu,
\mathrm{Var}(X_1)=\sigma^2 < \infty,
$$
then
$$
\frac{S_n - n\mu}{\sigma\sqrt{n}}
\xrightarrow{d}
\mathcal{N}(0,1).
$$

Equivalently,

$$
\frac{\sqrt{n}(\overline{X}_n-\mu)}{\sigma}
\xrightarrow{d}
\mathcal{N}(0,1).
$$

This is **convergence in distribution**.

## 5.2 Interpretation

The LLN tells us:

$$
\overline{X}_n \to \mu.
$$

The CLT refines that by telling us the scale and shape of the remaining fluctuations:

$$
\overline{X}_n - \mu \approx \frac{\sigma}{\sqrt{n}} Z,
Z\sim \mathcal N(0,1).
$$

So even if the original variables are not normal, the normalized average often becomes approximately normal for
large $n$.

## 5.3 Why the normalization is $\sqrt{n}$

There are really two separate choices in the CLT formula:

1. dividing by $\sqrt{n}$, which is the **structural scaling** needed to keep fluctuations of the sum finite;
2. dividing by $\sigma$, which is a **standardization** that turns the limiting variance into $1$.

We know

$$
\mathrm{Var}(S_n)=n\sigma^2.
$$

Therefore

$$
\mathrm{Var}\left(\frac{S_n-n\mu}{\sqrt{n}}\right)
=
\frac{1}{n}\mathrm{Var}(S_n)
=
\sigma^2.
$$

So $\sqrt{n}$ is exactly the scaling that keeps the variance finite and nontrivial. This is the analogue, in discrete
time, of the statement that Brownian variance grows linearly in time.

If we then divide once more by $\sigma$, we get

$$
\mathrm{Var}\left(\frac{S_n-n\mu}{\sigma\sqrt{n}}\right)=1,
$$

which is why the CLT is usually written with a standard normal limit:

$$
\frac{S_n-n\mu}{\sigma\sqrt{n}} \xrightarrow{d} \mathcal{N}(0,1).
$$

So the factor $\sqrt{n}$ is the essential probabilistic scaling, while the factor $\sigma$ is a normalization that
makes the reference limit cleaner.

This is the same square-root scaling that appears in:

- random walks,
- Brownian motion,
- Monte Carlo error,
- statistical estimation.

If we divided by $n$, the fluctuations would collapse to zero.
If we divided by something smaller than $\sqrt{n}$, the variance would blow up.

---

## 6. Example: Bernoulli coin tosses

Let

$$
X_i \sim \mathrm{Bernoulli}(p),
$$

so

$$
\mathbb{E}[X_i]=p,
\mathrm{Var}(X_i)=p(1-p).
$$

Then

$$
\overline{X}_n = \frac{1}{n}\sum_{i=1}^n X_i
$$

is the sample proportion of successes.

### LLN

The LLN says:

$$
\overline{X}_n \to p
$$

in probability, and under stronger versions almost surely.

### CLT

The CLT says:

$$
\frac{\sqrt{n}(\overline{X}_n-p)}{\sqrt{p(1-p)}}
\xrightarrow{d}
\mathcal{N}(0,1).
$$

So for large $n$,

$$
\overline{X}_n \approx \mathcal{N}\left(p, \frac{p(1-p)}{n}\right).
$$

This approximation is the basis of standard confidence intervals for proportions.

---

## 7. Example: Monte Carlo estimation

Suppose we want to estimate

$$
\theta = \mathbb{E}[g(X)]
$$

by simulation. We generate independent samples $X_1,\dots,X_n$ and compute

$$
\hat{\theta}_n = \frac{1}{n}\sum_{i=1}^n g(X_i).
$$

Then:

- by the LLN,
  $$
\hat{\theta}_n \to \theta;
$$
- by the CLT,
  $$
\sqrt{n}(\hat{\theta}_n-\theta)
\xrightarrow{d}
\mathcal{N}(0,\tau^2),
$$
  where
  $$
\tau^2 = \mathrm{Var}(g(X)).
$$

This explains the classical Monte Carlo error rate:

$$
\text{error} = O\left(\frac{1}{\sqrt{n}}\right).
$$

That slow square-root convergence is why variance reduction is so important in quantitative finance.

---

## 8. LLN versus CLT

A useful comparison:

| Theorem | Object studied                 | Result                                    |
|---------|--------------------------------|-------------------------------------------|
| LLN     | $\overline{X}_n$               | converges to $\mu$                        |
| CLT     | $\sqrt{n}(\overline{X}_n-\mu)$ | converges in law to a normal distribution |

So:

- LLN gives the **limit**;
- CLT gives the **fluctuation around the limit**.

---

## 9. A sketch of why the CLT is true

A full proof is technical, but the high-level logic is important.

Let

$$
Y_i = \frac{X_i-\mu}{\sigma},
$$

so that

$$
\mathbb{E}[Y_i]=0,
\mathrm{Var}(Y_i)=1.
$$

We want to understand

$$
\frac{1}{\sqrt{n}}\sum_{i=1}^n Y_i.
$$

The characteristic function of $Y_i$ is

$$
\varphi_Y(u)=\mathbb{E}[e^{iuY}].
$$

Near zero, one can show

$$
\varphi_Y(u)=1-\frac{u^2}{2}+o(u^2).
$$

Then the characteristic function of the normalized sum is

$$
\left(\varphi_Y\left(\frac{u}{\sqrt{n}}\right)\right)^n
\approx
\left(1-\frac{u^2}{2n}\right)^n
\to e^{-u^2/2},
$$

which is the characteristic function of $\mathcal N(0,1)$.

That is the core mechanism: when many independent centered terms are added together and normalized appropriately, the
Gaussian law emerges.

---

## 10. Extensions

The basic CLT has many extensions:

- non-identically distributed variables under Lindeberg or Lyapunov conditions,
- multivariate CLT,
- martingale CLT,
- functional CLT, where random walks converge to Brownian motion.

The functional CLT is especially important in stochastic-process theory: it connects discrete sums to continuous-time
Brownian motion.

---

## 11. Why these theorems matter in a quant platform

A production quant and risk platform uses these ideas constantly.

### LLN-related uses

- Monte Carlo estimators converging to prices and risk measures,
- sample averages of returns or losses,
- empirical default rates,
- convergence checks in simulation.

### CLT-related uses

- standard errors of estimators,
- confidence intervals,
- approximate distribution of P&L aggregates,
- Monte Carlo error bars,
- asymptotic normality of MLEs.

---

## 12. Final summary

The Law of Large Numbers and the Central Limit Theorem answer two different questions.

### Law of Large Numbers

If we average many independent observations, where does the average go?

Answer:
$$
\overline{X}_n \to \mu.
$$

### Central Limit Theorem

How does the average fluctuate around that limit?

Answer:
$$
\sqrt{n}(\overline{X}_n-\mu)
\text{ becomes approximately Gaussian.}
$$

Together they explain why averages stabilize, why Gaussian approximations appear so often, and why the fundamental
statistical error scale is $1/\sqrt{n}$.